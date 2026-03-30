/**
 * encapsulate.h - Encapsule un paquet IP dans une trame
 * (mot de synchronisation + longueur + charge utile + CRC).
 */

#ifndef ENCAPSULATE_H
#define ENCAPSULATE_H

#include <stdint.h>

/**
 * Encapsule un paquet IP dans une trame.
 * @param ip_payload données du paquet IP
 * @param ip_len     longueur du paquet
 * @param frame_out  tampon de sortie de la trame (doit être assez grand, voir common.h MAX_FRAME_LEN)
 * @return           nombre d’octets de la trame produite, 0 en cas d’échec
 */
int encapsulate(const uint8_t *ip_payload, int ip_len, uint8_t *frame_out);

#endif /* ENCAPSULATE_H */
