/**
 * packet_read.h - Lit un paquet IP depuis TUN dans un tampon.
 */

#ifndef PACKET_READ_H
#define PACKET_READ_H

#include <stdint.h>

/**
 * Lit un paquet IP depuis un périphérique TUN (bloquant).
 * @param fd     descripteur TUN (retourné par tun_create)
 * @param buf    tampon de sortie
 * @param maxlen taille maximale du tampon en octets
 * @return       nombre d’octets lus, ou -1 en cas d’échec
 */
int packet_read(int fd, uint8_t *buf, int maxlen);

#endif /* PACKET_READ_H */
