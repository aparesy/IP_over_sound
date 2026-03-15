/**
 * protocol.h - Encapsulation et décapsulage de trames (couche liaison).
 *
 * Envoi   : ajoute un en‑tête de trame (SYNC + longueur) et une queue (CRC)
 *           autour d’un paquet IP pour obtenir un flux d’octets structuré.
 * Réception : recherche le mot de synchronisation dans le flux de bits,
 *             lit la longueur, vérifie le CRC et extrait le paquet IP.
 */

#ifndef PROTOCOL_H
#define PROTOCOL_H

#include "common.h"
#include <stdint.h>
#include <stddef.h>  /* size_t */

/**
 * Encapsule un paquet IP dans une trame (SYNC + longueur + charge utile + CRC),
 * produisant un tableau d’octets.
 * @param payload      données du paquet IP
 * @param payload_len  longueur du paquet
 * @param frame_out    tampon de sortie, taille au moins FRAME_HEADER_LEN + payload_len + CRC_BYTES
 * @return             longueur totale de la trame produite, ou 0 en cas d’échec
 */
int protocol_encapsulate(const uint8_t *payload, int payload_len, uint8_t *frame_out);

/**
 * Extrait une trame depuis un flux d’octets : cherche la synchronisation,
 * lit la longueur, vérifie le CRC et restitue la charge utile.
 * @param frame        trame complète (en‑tête + charge utile + CRC)
 * @param frame_len    longueur de la trame
 * @param payload_out  tampon de sortie pour la charge utile (paquet IP)
 * @param max_payload  taille maximale du tampon de sortie
 * @return             nombre d’octets de charge utile, ou -1 en cas d’échec (CRC invalide, etc.)
 */
int protocol_decapsulate(const uint8_t *frame, int frame_len,
                         uint8_t *payload_out, int max_payload);

/**
 * Recherche la position de synchronisation dans un flux de bits
 * (indice du bit de départ d’une séquence de SYNC_LEN octets SYNC_BYTE).
 * @param bits    tableau de bits (8 bits par octet, bit de poids fort en premier)
 * @param nbits   nombre total de bits
 * @return        indice du premier bit de synchro, ou -1 si non trouvé
 */
int protocol_find_sync(const uint8_t *bits, int nbits);

#endif /* PROTOCOL_H */
