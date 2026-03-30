# IP over Sound

Transmission de paquets IP entre deux ordinateurs via des ondes sonores : les paquets IP issus de l’interface TUN locale sont modulés en FSK et joués par le haut‑parleur ; le microphone de la machine distante enregistre le signal, le démodule, reconstitue les trames, vérifie le CRC puis réinjecte les paquets IP dans son propre TUN — ce qui permet de réaliser un « réseau sur les ondes sonores ».

## Aperçu de l’architecture

- **TUN** : échange de paquets IP avec le noyau (lecture/écriture sur une interface réseau virtuelle).
- **protocol** : encapsulation/décapsulation de trames (mot de synchronisation + longueur + CRC).
- **modem** : modulation/démodulation FSK (bits ↔ forme d’onde).
- **audio** : lecture/écriture audio via PortAudio.

## Dépendances

- Linux (TUN et `linux/if_tun.h`)
- PortAudio : `libportaudio-dev` (Ubuntu/Debian)
- Compilation : `gcc`, avec liaison `-lpthread -lm -lportaudio`

## Compilation

```bash
make
```

Génère l’exécutable `ipo_sound`.

## Exécution

1. **Créer et configurer l’interface TUN** (nécessite root)  
   Lancer une première fois le programme pour que l’interface TUN soit créée, ou la créer manuellement :
   ```bash
   sudo ip tuntap add dev tun0 mode tun
   ```
   Puis configurer l’IP et la route :
   ```bash
   sudo ./scripts/setup_tun.sh tun0 10.0.0.1
   ```

2. **Lancer le programme**
   ```bash
   sudo ./ipo_sound [tun_name]
   ```
   Par défaut, `tun0` est utilisée ; un argument optionnel permet de spécifier le nom de l’interface TUN.

3. **Machine distante**  
   Sur une autre machine, configurer également TUN (par exemple `10.0.0.2/24`), lancer `ipo_sound`, et les deux hôtes peuvent communiquer sur le réseau 10.0.0.0/24 via les ondes sonores (haut‑parleur et micro doivent être en vis‑à‑vis, avec un volume adapté).

## Remarques

- Les droits **root** ou la capacité `CAP_NET_ADMIN` sont nécessaires pour manipuler TUN.
- La démonstration est **semi‑duplex** : émission et réception peuvent se chevaucher, mais sur un même canal un environnement bruyant peut entraîner des pertes.
- La fréquence d’échantillonnage, les fréquences FSK et le format de trame sont définis dans `include/common.h`.

---

# Présentation en français

## Logique d’implémentation globale

**IP over Sound** permet de transmettre des paquets IP entre deux ordinateurs via des ondes sonores. Le flux de données est le suivant :

- **Émission (TX)** : le noyau envoie un paquet IP vers l’interface TUN → le programme lit ce paquet depuis TUN → il l’encapsule dans une **trame** (en-tête de synchronisation + longueur + charge utile + CRC) → la trame est convertie en **flux de bits** → modulation **FSK** (0 → 1200 Hz, 1 → 2400 Hz) en échantillons audio → les échantillons sont envoyés à la carte son (haut-parleur).
- **Réception (RX)** : le microphone enregistre des échantillons audio → démodulation FSK (détection par passages par zéro) → flux de bits → recherche de la **synchronisation** (mot de synchro 0x7E) → extraction d’une trame complète → vérification CRC → extraction du paquet IP (charge utile) → écriture du paquet dans TUN → le noyau reçoit le paquet comme s’il venait d’une interface réseau.

Le programme utilise **deux threads** : un thread TX (émission) et un thread RX (réception), qui tournent en parallèle. L’accès à TUN et à l’audio est partagé via des descripteurs/poignées globaux.

**Couches** :
- **Réseau (IP)** : paquets IP échangés avec le noyau via TUN.
- **Liaison (trame)** : format de trame (synchro + longueur + charge + CRC), encapsulation/décapsulation, recherche de synchro dans le flux de bits.
- **Physique** : modulation/démodulation FSK, lecture/écriture audio (PortAudio).

---

## Rôle de chaque fichier

### Répertoire `include/`

| Fichier | Rôle |
|--------|------|
| **common.h** | Constantes globales : fréquence d’échantillonnage (44100 Hz), taille du buffer audio (1024), fréquences FSK (1200 Hz / 2400 Hz), débit (1200 bps), paramètres de trame (SYNC_LEN, SYNC_BYTE, MAX_FRAME_PAYLOAD, CRC_BYTES, etc.). C’est le point central pour adapter le projet. |
| **tun_dev.h** | Interface du module TUN : `tun_open`, `tun_read`, `tun_write`, `tun_close`. Déclare les fonctions d’échange de paquets IP avec le noyau. |
| **protocol.h** | Interface du module trame (couche liaison) : `protocol_encapsulate` (IP → trame), `protocol_decapsulate` (trame → IP, avec vérification CRC), `protocol_find_sync` (recherche du mot de synchro dans le flux de bits). |
| **modem.h** | Interface du modem FSK : création/destruction des poignées TX/RX, `modem_tx_modulate` (bits → échantillons), `modem_rx_demodulate` (échantillons → bits). |
| **audio_dev.h** | Interface audio (PortAudio) : `audio_init`, `audio_write`, `audio_read`, `audio_cleanup`. Lecture micro, écriture haut-parleur. |
| **utils.h** | Fonctions utilitaires : `crc16` (CRC-16 CCITT pour la trame), `debug_hex_dump` (affichage hexadécimal pour débogage). |

### Répertoire `src/`

| Fichier | Rôle |
|--------|------|
| **main.c** | Point d’entrée : ouverture TUN, initialisation audio, création des threads TX et RX, boucle principale jusqu’à Ctrl+C, puis nettoyage. Contient aussi les fonctions d’enchaînement : `frame_to_bits`, `bits_to_bytes`, `bits_append`, `bits_remove`, et les corps des threads `tx_thread_func` et `rx_thread_func`. |
| **tun_dev.c** | Implémentation TUN : `open("/dev/net/tun")`, `ioctl(TUNSETIFF)` pour créer/attacher une interface TUN (ex. tun0), puis `read`/`write` sur le descripteur pour recevoir/envoyer des paquets IP bruts. Nécessite root ou CAP_NET_ADMIN. |
| **protocol.c** | Implémentation du protocole de trame : encapsulation (synchro + longueur big-endian + charge + CRC), décapsulation (lecture longueur, vérification CRC, extraction charge), recherche de synchro dans un buffer de bits (alignement bit à bit). |
| **modem.c** | Implémentation FSK : côté TX, génération de sinusoïdes (1200 Hz / 2400 Hz) avec phase continue ; côté RX, démodulation par comptage des passages par zéro pour distinguer 0 (basse fréquence) et 1 (haute fréquence). |
| **audio_dev.c** | Implémentation PortAudio : ouverture des flux entrée (micro) et sortie (haut-parleur) par défaut, `Pa_ReadStream` / `Pa_WriteStream` avec le format float et la fréquence/taille de buffer définies dans common.h. |
| **utils.c** | Implémentation CRC-16 (CCITT) et affichage hexadécimal pour le débogage. |

### Autres fichiers

| Fichier | Rôle |
|--------|------|
| **Makefile** | Règles de compilation : compilation des .c en .o, liaison avec `-lpthread -lm -lportaudio`, production de l’exécutable `ipo_sound`. |
| **scripts/setup_tun.sh** | Script pour créer/configurer l’interface TUN (ex. tun0) et lui attribuer une adresse IP (ex. 10.0.0.1/24). À lancer en root. |

---

## Déroulement du programme (flux d’exécution)

1. **Démarrage (`main`)**  
   - Gestion du signal SIGINT (Ctrl+C) pour mettre fin proprement à l’exécution.  
   - Ouverture de l’interface TUN (`tun_open`, ex. tun0) ; en cas d’échec, message invitant à lancer en `sudo`.  
   - Initialisation de l’audio (`audio_init`) : PortAudio, flux micro et haut-parleur, démarrage des flux.  
   - Création de deux threads : `tx_thread_func` et `rx_thread_func`, auxquels on passe le descripteur TUN (et l’audio via une variable globale).  
   - La boucle principale se contente d’attendre (p.ex. `sleep(1)`) tant que `g_running` est vrai.

2. **Thread TX (émission)**  
   - Boucle tant que `g_running` et que l’audio est disponible.  
   - **tun_read** : lecture bloquante d’un paquet IP depuis TUN (aucun son n’est émis tant qu’aucun paquet n’est envoyé vers TUN).  
   - **protocol_encapsulate** : construction de la trame (synchro + longueur + paquet IP + CRC).  
   - **frame_to_bits** : conversion de la trame (octets) en flux de bits (convention : 8 bits par octet, bit de poids fort en premier).  
   - **modem_tx_modulate** : modulation FSK des bits en échantillons audio (sinusoïdes 1200 Hz / 2400 Hz).  
   - **audio_write** : envoi des échantillons vers le haut-parleur par blocs (ex. AUDIO_FRAMES_PER_BUFFER).  
   → C’est à ce moment que le son est émis.

3. **Thread RX (réception)**  
   - Boucle tant que `g_running` et que l’audio est disponible.  
   - **audio_read** : lecture d’un bloc d’échantillons depuis le microphone.  
   - **modem_rx_demodulate** : démodulation FSK → flux de bits.  
   - Les bits sont **ajoutés** à un buffer cumulatif (`bits_append`) ; si le buffer est plein, une partie est supprimée pour faire de la place (`bits_remove`).  
   - **protocol_find_sync** : recherche de la position de début de trame (mot de synchro dans le flux de bits).  
   - Vérification que la longueur de trame (lue dans l’en-tête) est valide et que suffisamment de bits sont disponibles.  
   - **bits_to_bytes** : conversion des bits de la trame en octets.  
   - **protocol_decapsulate** : vérification CRC et extraction du paquet IP.  
   - **tun_write** : injection du paquet IP dans TUN ; le noyau le traite comme un paquet reçu sur l’interface.

4. **Arrêt**  
   - L’utilisateur envoie Ctrl+C → `signal_handler` met `g_running` à 0.  
   - La boucle de `main` se termine, `main` met aussi `g_running` à 0 (redondant), puis appelle `pthread_join` sur les deux threads pour les laisser quitter proprement.  
   - **audio_cleanup** : arrêt et fermeture des flux audio, `Pa_Terminate`.  
   - **tun_close** : fermeture du descripteur TUN.  
   - Fin du programme.

En résumé : le **son n’est émis que lorsqu’il y a du trafic IP à envoyer** (paquet lu depuis TUN). La réception est continue : le micro enregistre en permanence, et dès qu’une trame valide est détectée et vérifiée, le paquet IP est renvoyé au noyau via TUN.
