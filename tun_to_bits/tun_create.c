/**
 * tun_create.c - Création d’un périphérique TUN (appelle tun_dev::tun_open).
 */

#include "tun_create.h"
#include "../include/tun_dev.h"

int tun_create(const char *name)
{
    return tun_open(name);
}
