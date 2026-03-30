/**
 * tun_create.h - Création d’un périphérique TUN.
 * Encapsule tun_dev::tun_open pour en faire une étape « créer TUN » distincte
 * dans ce module.
 */

#ifndef TUN_CREATE_H
#define TUN_CREATE_H

/**
 * Crée et attache un périphérique TUN.
 * @param name nom de l’interface, par ex. "tun0"
 * @return     descripteur de fichier (>=0) en cas de succès, -1 en cas d’échec
 */
int tun_create(const char *name);

#endif /* TUN_CREATE_H */
