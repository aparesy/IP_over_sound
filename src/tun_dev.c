/**
 * tun_dev.c - Implémentation de l’interface réseau virtuelle TUN sous Linux.
 *
 * Ouvre /dev/net/tun et le configure via ioctl comme périphérique TUN pour
 * échanger des paquets IP avec le noyau.
 * Nécessite les droits root ou la capacité CAP_NET_ADMIN.
 */

#include "tun_dev.h"
#include "common.h"
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <linux/if.h>
#include <linux/if_tun.h>

/**
 * Ouvre un périphérique TUN.
 * name, par ex. "tun0", permet de créer ou de rattacher cette interface TUN.
 */
int tun_open(const char *name)
{
    int fd;
    struct ifreq ifr;

    fd = open("/dev/net/tun", O_RDWR);
    if (fd < 0) {
        perror("tun_open: open /dev/net/tun");
        return -1;
    }

    memset(&ifr, 0, sizeof(ifr));
    ifr.ifr_flags = IFF_TUN | IFF_NO_PI;  /* Mode TUN, sans en‑tête supplémentaire. */
    if (name && name[0])
        strncpy(ifr.ifr_name, name, IFNAMSIZ - 1);

    if (ioctl(fd, TUNSETIFF, &ifr) < 0) {
        perror("tun_open: ioctl TUNSETIFF");
        close(fd);
        return -1;
    }

    return fd;
}

int tun_read(int fd, uint8_t *buf, int maxlen)
{
    ssize_t n;

    if (fd < 0 || !buf || maxlen <= 0)
        return -1;

    n = read(fd, buf, (size_t)maxlen);
    if (n < 0) {
        perror("tun_read");
        return -1;
    }
    return (int)n;
}

int tun_write(int fd, const uint8_t *buf, int len)
{
    ssize_t n;

    if (fd < 0 || !buf || len <= 0)
        return -1;

    n = write(fd, buf, (size_t)len);
    if (n < 0) {
        perror("tun_write");
        return -1;
    }
    return (int)n;
}

void tun_close(int fd)
{
    if (fd >= 0)
        close(fd);
}
