/**
 * tun_dev.h - Interface d’accès au périphérique réseau virtuel TUN.
 *
 * Responsable de l’ouverture, de la lecture et de l’écriture sur un
 * périphérique TUN Linux pour échanger des paquets IP avec le noyau.
 * Les paquets que le noyau veut émettre sont écrits dans TUN et lus par
 * ce module ; les paquets écrits par ce module dans TUN sont vus par le
 * noyau comme reçus par une carte réseau.
 */

#ifndef TUN_DEV_H
#define TUN_DEV_H

#include <stdint.h>

/**
 * Ouvre et configure un périphérique TUN.
 * @param name nom de l’interface, par ex. "tun0"
 * @return     descripteur de fichier (>= 0) en cas de succès, -1 en cas d’échec
 */
int tun_open(const char *name);

/**
 * Lit un paquet IP depuis TUN (appel bloquant).
 * @param fd     descripteur retourné par tun_open
 * @param buf    tampon, doit faire au moins MAX_FRAME_PAYLOAD octets
 * @param maxlen taille du tampon
 * @return       nombre d’octets lus, ou -1 en cas d’échec
 */
int tun_read(int fd, uint8_t *buf, int maxlen);

/**
 * Écrit un paquet IP dans TUN (injection vers le noyau).
 * @param fd   descripteur retourné par tun_open
 * @param buf  données du paquet IP
 * @param len  longueur du paquet
 * @return     nombre d’octets écrits, ou -1 en cas d’échec
 */
int tun_write(int fd, const uint8_t *buf, int len);

/**
 * Ferme le périphérique TUN.
 * @param fd descripteur retourné par tun_open
 */
void tun_close(int fd);

#endif /* TUN_DEV_H */
