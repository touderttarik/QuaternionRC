#pragma once

#include <stdbool.h>
#include <stdint.h>

#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Axis mapping used by this project:
 * - Right joystick:
 *   - GPIO35 (VRX) -> ROLL
 *   - GPIO34 (VRY) -> PITCH
 * - Left joystick (physically rotated 180 degrees on breadboard):
 *   - GPIO32 (VRY) -> THROTTLE
 *   - GPIO33 (VRX) -> YAW
 *
 * Software correction is applied so that controls feel natural for the user:
 * - Pushing throttle stick up increases throttle
 * - Pushing yaw stick right produces positive yaw command
 */
typedef enum {
    JOYSTICK_AXIS_ROLL = 0,
    JOYSTICK_AXIS_PITCH,
    JOYSTICK_AXIS_THROTTLE,
    JOYSTICK_AXIS_YAW,
    JOYSTICK_AXIS_COUNT
} joystick_axis_t;

typedef struct {
    float roll;            // [-1.0, +1.0]
    float pitch;           // [-1.0, +1.0]
    float yaw;             // [-1.0, +1.0] (corrected orientation)
    float throttle_stick;  // [-1.0, +1.0] (corrected orientation)
    float throttle;        // [0.0, +1.0]

    bool right_switch_pressed; // GPIO25, active low
    bool left_switch_pressed;  // GPIO26, active low

    uint64_t timestamp_us;
} joystick_values_t;

/** Initialize ADC, calibration and GPIO inputs for joystick switches. */
esp_err_t joystick_input_init(void);

/**
 * Read one clean sample from all joystick axes.
 * Includes oversampling, ADC calibration (if available), normalization,
 * EMA smoothing, scaled deadband and left-stick orientation correction.
 */
esp_err_t joystick_input_read(joystick_values_t *out_values);

/**
 * Update center points by sampling current joystick neutral position.
 * Keep sticks released while calling this.
 */
esp_err_t joystick_input_recenter(void);

/** Configure global filter parameters. */
esp_err_t joystick_input_set_filter(float ema_alpha, float deadband);

/** Override per-axis calibration bounds (in mV). */
esp_err_t joystick_input_set_axis_calibration(joystick_axis_t axis, int min_mv, int center_mv, int max_mv);

/** Read per-axis calibration bounds (in mV). */
esp_err_t joystick_input_get_axis_calibration(joystick_axis_t axis, int *min_mv, int *center_mv, int *max_mv);

#ifdef __cplusplus
}
#endif
