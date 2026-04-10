#include "joystick_input.h"

#include <math.h>
#include <string.h>

#include "driver/gpio.h"
#include "esp_adc/adc_cali.h"
#include "esp_adc/adc_cali_scheme.h"
#include "esp_adc/adc_oneshot.h"
#include "esp_check.h"
#include "esp_log.h"
#include "esp_timer.h"

#define TAG "joystick_input"

#define JOY_GPIO_ROLL      35
#define JOY_GPIO_PITCH     34
#define JOY_GPIO_THROTTLE  32
#define JOY_GPIO_YAW       33

#define JOY_GPIO_SW_RIGHT  25
#define JOY_GPIO_SW_LEFT   26

#define JOY_ATTEN              ADC_ATTEN_DB_12
#define JOY_BITWIDTH           ADC_BITWIDTH_DEFAULT
#define JOY_OVERSAMPLE_COUNT   8
#define JOY_RECENTER_SAMPLES   128

#define JOY_DEFAULT_MIN_MV     150
#define JOY_DEFAULT_CENTER_MV  1650
#define JOY_DEFAULT_MAX_MV     3150

#define DEG_TO_RAD M_PI/180

typedef struct {
    int gpio;
    int sign;
    adc_channel_t channel;
    int min_mv;
    int center_mv;
    int max_mv;
    float ema_state;
    bool ema_initialized;
} axis_state_t;

static adc_oneshot_unit_handle_t s_adc_handle;
static bool s_initialized;
static float s_ema_alpha = 0.18f;
static float s_deadband = 0.05f;

static axis_state_t s_axes[JOYSTICK_AXIS_COUNT] = {
    [JOYSTICK_AXIS_ROLL] = {
        .gpio = JOY_GPIO_ROLL,
        .sign = +1,
        .min_mv = JOY_DEFAULT_MIN_MV,
        .center_mv = JOY_DEFAULT_CENTER_MV,
        .max_mv = JOY_DEFAULT_MAX_MV,
    },
    [JOYSTICK_AXIS_PITCH] = {
        .gpio = JOY_GPIO_PITCH,
        .sign = +1,
        .min_mv = JOY_DEFAULT_MIN_MV,
        .center_mv = JOY_DEFAULT_CENTER_MV,
        .max_mv = JOY_DEFAULT_MAX_MV,
    },
    [JOYSTICK_AXIS_THROTTLE] = {
        .gpio = JOY_GPIO_THROTTLE,
        .sign = +1, // throttle axis inverted relative to previous behavior (yaw remains corrected)
        .min_mv = JOY_DEFAULT_MIN_MV,
        .center_mv = JOY_DEFAULT_CENTER_MV,
        .max_mv = JOY_DEFAULT_MAX_MV,
    },
    [JOYSTICK_AXIS_YAW] = {
        .gpio = JOY_GPIO_YAW,
        .sign = -1, // left joystick was mounted 180deg, restore natural right=yaw+
        .min_mv = JOY_DEFAULT_MIN_MV,
        .center_mv = JOY_DEFAULT_CENTER_MV,
        .max_mv = JOY_DEFAULT_MAX_MV,
    },
};

static adc_cali_handle_t s_cali[JOYSTICK_AXIS_COUNT] = {0};
static bool s_cali_enabled[JOYSTICK_AXIS_COUNT] = {0};
#if ADC_CALI_SCHEME_CURVE_FITTING_SUPPORTED
static bool s_cali_is_curve[JOYSTICK_AXIS_COUNT] = {0};
#endif

static float clampf_local(float x, float lo, float hi)
{
    if (x < lo) {
        return lo;
    }
    if (x > hi) {
        return hi;
    }
    return x;
}

static float scaled_deadband(float x, float d)
{
    if (d <= 0.0f) {
        return x;
    }
    if (d >= 1.0f) {
        return 0.0f;
    }

    float ax = fabsf(x);
    if (ax < d) {
        return 0.0f;
    }

    float sign = (x < 0.0f) ? -1.0f : 1.0f;
    return sign * ((ax - d) / (1.0f - d));
}

static bool create_cali_handle(joystick_axis_t axis)
{
    esp_err_t ret = ESP_FAIL;
    adc_cali_handle_t handle = NULL;

#if ADC_CALI_SCHEME_CURVE_FITTING_SUPPORTED
    adc_cali_curve_fitting_config_t curve_cfg = {
        .unit_id = ADC_UNIT_1,
        .chan = s_axes[axis].channel,
        .atten = JOY_ATTEN,
        .bitwidth = JOY_BITWIDTH,
    };
    ret = adc_cali_create_scheme_curve_fitting(&curve_cfg, &handle);
    if (ret == ESP_OK) {
        s_cali[axis] = handle;
        s_cali_enabled[axis] = true;
        s_cali_is_curve[axis] = true;
        return true;
    }
#endif

#if ADC_CALI_SCHEME_LINE_FITTING_SUPPORTED
    adc_cali_line_fitting_config_t line_cfg = {
        .unit_id = ADC_UNIT_1,
        .atten = JOY_ATTEN,
        .bitwidth = JOY_BITWIDTH,
#if CONFIG_IDF_TARGET_ESP32
        .default_vref = 0,
#endif
    };
    ret = adc_cali_create_scheme_line_fitting(&line_cfg, &handle);
    if (ret == ESP_OK) {
        s_cali[axis] = handle;
        s_cali_enabled[axis] = true;
#if ADC_CALI_SCHEME_CURVE_FITTING_SUPPORTED
        s_cali_is_curve[axis] = false;
#endif
        return true;
    }
#endif

    s_cali[axis] = NULL;
    s_cali_enabled[axis] = false;
    ESP_LOGW(TAG, "Calibration disabled for axis %d (ret=%s)", axis, esp_err_to_name(ret));
    return false;
}

static esp_err_t read_axis_raw(joystick_axis_t axis, int *out_raw)
{
    if (!out_raw) {
        return ESP_ERR_INVALID_ARG;
    }

    int32_t sum = 0;
    for (int i = 0; i < JOY_OVERSAMPLE_COUNT; ++i) {
        int raw = 0;
        esp_err_t err = adc_oneshot_read(s_adc_handle, s_axes[axis].channel, &raw);
        if (err != ESP_OK) {
            return err;
        }
        sum += raw;
    }

    *out_raw = (int)(sum / JOY_OVERSAMPLE_COUNT);
    return ESP_OK;
}

static int raw_to_mv(joystick_axis_t axis, int raw)
{
    int mv = 0;
    if (s_cali_enabled[axis] && s_cali[axis] != NULL) {
        if (adc_cali_raw_to_voltage(s_cali[axis], raw, &mv) == ESP_OK) {
            return mv;
        }
    }

    // Fallback rough conversion when calibration is unavailable.
    return (raw * 3300) / 4095;
}

static float normalize_axis(joystick_axis_t axis, int mv)
{
    axis_state_t *a = &s_axes[axis];

    float den = 1.0f;
    float x = 0.0f;

    if (mv >= a->center_mv) {
        den = (float)(a->max_mv - a->center_mv);
        if (den < 1.0f) {
            den = 1.0f;
        }
        x = (float)(mv - a->center_mv) / den;
    } else {
        den = (float)(a->center_mv - a->min_mv);
        if (den < 1.0f) {
            den = 1.0f;
        }
        x = (float)(mv - a->center_mv) / den;
    }

    x = clampf_local(x, -1.0f, 1.0f);
    x *= (float)a->sign;

    if (!a->ema_initialized) {
        a->ema_state = x;
        a->ema_initialized = true;
    } else {
        a->ema_state = s_ema_alpha * x + (1.0f - s_ema_alpha) * a->ema_state;
    }

    x = scaled_deadband(a->ema_state, s_deadband);
    return clampf_local(x, -1.0f, 1.0f);
}

esp_err_t joystick_input_init(void)
{
    if (s_initialized) {
        return ESP_OK;
    }

    adc_oneshot_unit_init_cfg_t unit_cfg = {
        .unit_id = ADC_UNIT_1,
        .ulp_mode = ADC_ULP_MODE_DISABLE,
    };

    ESP_RETURN_ON_ERROR(adc_oneshot_new_unit(&unit_cfg, &s_adc_handle), TAG, "adc_oneshot_new_unit failed");

    for (int i = 0; i < JOYSTICK_AXIS_COUNT; ++i) {
        adc_unit_t unit = ADC_UNIT_1;
        adc_channel_t channel = ADC_CHANNEL_0;
        ESP_RETURN_ON_ERROR(adc_oneshot_io_to_channel(s_axes[i].gpio, &unit, &channel), TAG, "GPIO%d is not ADC capable", s_axes[i].gpio);

        if (unit != ADC_UNIT_1) {
            ESP_LOGE(TAG, "GPIO%d mapped to ADC2, not allowed with Wi-Fi", s_axes[i].gpio);
            return ESP_ERR_INVALID_ARG;
        }

        s_axes[i].channel = channel;

        adc_oneshot_chan_cfg_t chan_cfg = {
            .atten = JOY_ATTEN,
            .bitwidth = JOY_BITWIDTH,
        };

        ESP_RETURN_ON_ERROR(adc_oneshot_config_channel(s_adc_handle, channel, &chan_cfg), TAG, "adc_oneshot_config_channel failed");

        (void)create_cali_handle((joystick_axis_t)i);
    }

    gpio_config_t sw_cfg = {
        .pin_bit_mask = (1ULL << JOY_GPIO_SW_RIGHT) | (1ULL << JOY_GPIO_SW_LEFT),
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = GPIO_PULLUP_ENABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE,
    };
    ESP_RETURN_ON_ERROR(gpio_config(&sw_cfg), TAG, "gpio_config for switches failed");

    s_initialized = true;
    ESP_RETURN_ON_ERROR(joystick_input_recenter(), TAG, "initial recenter failed");

    ESP_LOGI(TAG,
             "Joystick input initialized. Mapping: roll=GPIO35 pitch=GPIO34 throttle=GPIO32 yaw=GPIO33, left stick corrected in software");
    return ESP_OK;
}

esp_err_t joystick_input_recenter(void)
{
    if (!s_initialized) {
        return ESP_ERR_INVALID_STATE;
    }

    for (int axis = 0; axis < JOYSTICK_AXIS_COUNT; ++axis) {
        int64_t sum_mv = 0;

        for (int i = 0; i < JOY_RECENTER_SAMPLES; ++i) {
            int raw = 0;
            ESP_RETURN_ON_ERROR(read_axis_raw((joystick_axis_t)axis, &raw), TAG, "read_axis_raw failed");
            sum_mv += raw_to_mv((joystick_axis_t)axis, raw);
        }

        s_axes[axis].center_mv = (int)(sum_mv / JOY_RECENTER_SAMPLES);
        s_axes[axis].ema_state = 0.0f;
        s_axes[axis].ema_initialized = false;
    }

    ESP_LOGI(TAG, "Recenter complete: roll=%dmV pitch=%dmV throttle=%dmV yaw=%dmV",
             s_axes[JOYSTICK_AXIS_ROLL].center_mv,
             s_axes[JOYSTICK_AXIS_PITCH].center_mv,
             s_axes[JOYSTICK_AXIS_THROTTLE].center_mv,
             s_axes[JOYSTICK_AXIS_YAW].center_mv);

    return ESP_OK;
}

esp_err_t joystick_input_set_filter(float ema_alpha, float deadband)
{
    if (ema_alpha <= 0.0f || ema_alpha > 1.0f) {
        return ESP_ERR_INVALID_ARG;
    }
    if (deadband < 0.0f || deadband >= 1.0f) {
        return ESP_ERR_INVALID_ARG;
    }

    s_ema_alpha = ema_alpha;
    s_deadband = deadband;
    return ESP_OK;
}

esp_err_t joystick_input_set_axis_calibration(joystick_axis_t axis, int min_mv, int center_mv, int max_mv)
{
    if (axis < 0 || axis >= JOYSTICK_AXIS_COUNT) {
        return ESP_ERR_INVALID_ARG;
    }
    if (!(min_mv < center_mv && center_mv < max_mv)) {
        return ESP_ERR_INVALID_ARG;
    }

    s_axes[axis].min_mv = min_mv;
    s_axes[axis].center_mv = center_mv;
    s_axes[axis].max_mv = max_mv;
    s_axes[axis].ema_state = 0.0f;
    s_axes[axis].ema_initialized = false;
    return ESP_OK;
}

esp_err_t joystick_input_get_axis_calibration(joystick_axis_t axis, int *min_mv, int *center_mv, int *max_mv)
{
    if (axis < 0 || axis >= JOYSTICK_AXIS_COUNT || !min_mv || !center_mv || !max_mv) {
        return ESP_ERR_INVALID_ARG;
    }

    *min_mv = s_axes[axis].min_mv;
    *center_mv = s_axes[axis].center_mv;
    *max_mv = s_axes[axis].max_mv;
    return ESP_OK;
}

esp_err_t joystick_input_read(joystick_values_t *out_values)
{
    if (!s_initialized) {
        return ESP_ERR_INVALID_STATE;
    }
    if (!out_values) {
        return ESP_ERR_INVALID_ARG;
    }

    int raw[JOYSTICK_AXIS_COUNT] = {0};
    int mv[JOYSTICK_AXIS_COUNT] = {0};
    float norm[JOYSTICK_AXIS_COUNT] = {0.0f};

    for (int axis = 0; axis < JOYSTICK_AXIS_COUNT; ++axis) {
        ESP_RETURN_ON_ERROR(read_axis_raw((joystick_axis_t)axis, &raw[axis]), TAG, "ADC read failed on axis %d", axis);
        mv[axis] = raw_to_mv((joystick_axis_t)axis, raw[axis]);
        norm[axis] = normalize_axis((joystick_axis_t)axis, mv[axis]);
    }

    memset(out_values, 0, sizeof(*out_values));
    out_values->angles = (angles_t) {
        .roll = norm[JOYSTICK_AXIS_ROLL] * 45 * DEG_TO_RAD,
        .pitch = norm[JOYSTICK_AXIS_PITCH] * 45 * DEG_TO_RAD,
        .yaw = norm[JOYSTICK_AXIS_YAW] * 90 * DEG_TO_RAD,
    };
    out_values->throttle_stick = norm[JOYSTICK_AXIS_THROTTLE];

    out_values->throttle = 0.5f * (out_values->throttle_stick + 1.0f);
    out_values->throttle = clampf_local(out_values->throttle, 0.0f, 1.0f);

    out_values->right_switch_pressed = (gpio_get_level(JOY_GPIO_SW_RIGHT) == 0);
    out_values->left_switch_pressed = (gpio_get_level(JOY_GPIO_SW_LEFT) == 0);

    out_values->timestamp_us = (uint64_t)esp_timer_get_time();

    return ESP_OK;
}
