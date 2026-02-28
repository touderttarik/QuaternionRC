# RC - Telecommande ESP32 pour le drone

Firmware ESP-IDF de la telecommande du projet quadcopter.

## Objectif

Ce projet implemente la partie "Remote" (telecommande):
- connexion Wi-Fi au point d'acces du drone,
- etablissement d'un canal de transport avec le controleur de vol,
- base logicielle pour envoyer ensuite les commandes de pilotage.

## Ce qui est deja fait

### 1) Couche liaison Wi-Fi (`components/wifi/link_layer.c`)
- Initialisation NVS, netif, boucle d'evenements ESP-IDF.
- Connexion en mode station (`WIFI_MODE_STA`) au SSID cible.
- Gestion des evenements Wi-Fi/IP avec suivi d'etat via `wifiStatus`.
- Reconnexion automatique avec nombre limite de tentatives.

### 2) Couche transport (`components/wifi/transport_layer.c`)
- Mise en place d'un client TCP vers le drone.
- Fonctions robustes `recvn()` et `sendn()` pour lire/ecrire le nombre d'octets voulu.
- Handshake initial implementé:
  - envoi de `"Hello Drone !"`
  - attente et verification de `"Hello Remote !"`
- Squelette de socket UDP cree pour les echanges non critiques.

### 3) Integration applicative (`main/main.c`)
- Sequence de boot du firmware remote.
- Connexion Wi-Fi au reseau du drone.
- Lancement de la tache transport FreeRTOS.
- Synchronisation initiale via semaphore avant la suite des operations.

## Structure du projet

- `main/`: point d'entree du firmware remote.
- `components/wifi/link_layer.*`: connexion et etat Wi-Fi.
- `components/wifi/transport_layer.*`: sockets TCP/UDP et handshake.

## Prochaines etapes

- Definir le format des paquets de commande (gaz/roll/pitch/yaw).
- Mettre en place l'envoi periodique des commandes de vol sur UDP.
- Ajouter un canal de messages critiques (arming/disarming/failsafe).
- Ajouter des timeouts/retry plus stricts et une gestion de perte de lien.
- Ajouter des tests de validation du protocole remote <-> drone.

## Build (ESP-IDF)

```bash
idf.py set-target esp32
idf.py build
idf.py flash monitor
```

## Auteur

Tarik
