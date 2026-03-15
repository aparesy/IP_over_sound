/**
 * utils.h - Interface des fonctions utilitaires (CRC, impression de debug, etc.).
 *
 * Utilisé par les modules protocol, main, etc., sans dépendance matérielle
 * spécifique.
 */

#ifndef UTILS_H
#define UTILS_H

#include <stdint.h>
#include <stddef.h>

/**
 * Calcule un CRC‑16 (CCITT), utilisé pour la vérification en fin de trame.
 * @param data pointeur vers les données à vérifier
 * @param len  longueur des données (en octets)
 * @return     valeur CRC 16 bits
 */
uint16_t crc16(const uint8_t *data, size_t len);

/**
 * Affichage de débogage : imprime une zone mémoire en hexadécimal (optionnel,
 * utile pour inspecter le contenu des trames).
 * @param tag  chaîne préfixe, par ex. "TX" / "RX"
 * @param data pointeur vers les données
 * @param len  longueur en octets
 */
void debug_hex_dump(const char *tag, const uint8_t *data, size_t len);

#endif /* UTILS_H */
