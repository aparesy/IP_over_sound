/**
 * common.h - Constantes et configurations globales du projet IP over Sound
 *
 * Ce fichier d'en-tête définit les paramètres partagés par tout le projet, facilitant le débogage et les modifications.
 * Pour changer la fréquence d'échantillonnage, la taille du buffer, la fréquence FSK, il suffit de modifier ce fichier.
 */

#ifndef COMMON_H
#define COMMON_H

/* ========== Paramètres audio ========== */
/** Fréquence d'échantillonnage (Hz). Doit être supportée par la carte son,
 *  et la fréquence porteuse FSK doit être < SAMPLE_RATE/2. */
#define SAMPLE_RATE         44100   /** Indique que 44100 échantillons sont pris par seconde. */

/** Nombre de trames d'échantillons lues/écrites à chaque appel à la carte son.
 *  Influence la latence et l'occupation CPU. */
#define AUDIO_FRAMES_PER_BUFFER  1024   /** Taille du buffer pour lecture/écriture audio (1024 échantillons). */

/** Type de données des échantillons audio : chaque échantillon est un flottant dans [-1.0, 1.0]. */
typedef float sample_t;

/* ========== Paramètres de modulation FSK (couche physique) ========== */
/** FSK (Frequency Shift Keying) est une technique de modulation numérique qui
 *  transmet des signaux numériques en changeant la fréquence de la porteuse.
 *  Elle utilise ici deux fréquences pour représenter les bits 0 et 1 :
 *  le bit 0 correspond à 1200 Hz, le bit 1 à 2400 Hz. */
/** Fréquence porteuse correspondant au bit 0 (Hz). */
#define FSK_FREQ_0         1200

/** Fréquence porteuse correspondant au bit 1 (Hz). */
#define FSK_FREQ_1         2400

/** Débit binaire (bps), détermine le nombre d'échantillons par bit. */
#define FSK_BAUD_RATE      1200  /** 1200 bits sont envoyés par seconde, chaque bit dure 1/1200 s. */

/** Nombre d'échantillons correspondant à un bit : SAMPLE_RATE / FSK_BAUD_RATE. */
#define SAMPLES_PER_BIT    (SAMPLE_RATE / FSK_BAUD_RATE)

/* ========== Paramètres de protocole/trame (couche liaison) ========== */
/** Ici, une trame désigne une unité de données encapsulée comprenant :
 *  mot de synchronisation, longueur, charge utile et CRC. Sur le lien sonore,
 *  on encapsule "un bloc de données à transmettre" dans cette entité formatée. */
/** Longueur maximale de la charge utile d'une trame (c’est‑à‑dire la taille
 *  maximale d’un paquet IP), identique au MTU du TUN. */
/** "Trame" = mot de synchronisation + longueur + paquet IP (charge utile) + CRC.
 *  C'est l’unité de transmission de base sur le lien sonore. */
/** Relation avec le "paquet" (pour aider à comprendre) :
Paquet IP : concept de couche réseau, le noyau/TUN vous donne un "paquet IP".
Trame : concept de couche liaison, c'est "l'enveloppe pour transmettre un paquet IP sur le lien sonore".
On peut retenir :
En émission : un paquet IP → encapsulé dans une trame (ajout tête et queue) → devient un flux de bits → modulation → onde sonore.
En réception : onde sonore → démodulation → flux de bits → trouver tête de trame, extraire une trame selon la longueur → vérifier CRC → extraire le paquet IP de la trame et le donner à TUN. */
#define MAX_FRAME_PAYLOAD  1500  /** Indique que la charge utile maximale d'une trame est de 1500 octets. */

/** Longueur du mot de synchronisation en octets, utilisé pour repérer le début de trame côté réception. */
#define SYNC_LEN           2

/** Valeur du mot de synchronisation (0x7E, couramment utilisée en HDLC pour éviter les conflits avec les données usuelles). */
#define SYNC_BYTE          0x7E

/** Nombre d'octets occupés par le champ longueur. */
#define LEN_FIELD_BYTES    2  /** 2 octets suffisent pour représenter 1500 octets de charge utile. */

/** Nombre d'octets occupés par le CRC (CRC‑16). */
#define CRC_BYTES          2

/** Longueur totale de l'en‑tête de trame : mot de synchronisation + longueur = SYNC_LEN + LEN_FIELD_BYTES. */
#define FRAME_HEADER_LEN   (SYNC_LEN + LEN_FIELD_BYTES)

/** Nombre maximal d'octets pour une trame (en‑tête + charge utile + CRC). */
#define MAX_FRAME_LEN      (FRAME_HEADER_LEN + MAX_FRAME_PAYLOAD + CRC_BYTES)

/* ========== Périphérique TUN ========== */
/** Nom par défaut du périphérique TUN. Si /dev/net/tun existe déjà,
 *  on utilisera par exemple tun0. */
#define TUN_DEV_NAME        "tun0"

#endif /* COMMON_H */
