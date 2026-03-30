/**
 * packet_read.c - Lit un paquet IP depuis TUN dans un tampon
 * (appelle tun_dev::tun_read).
 */

#include "packet_read.h"
#include "../include/tun_dev.h"

int packet_read(int fd, uint8_t *buf, int maxlen)
{
    return tun_read(fd, buf, maxlen);
}
