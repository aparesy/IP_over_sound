/**
 * utils.c - Implémentation des fonctions utilitaires (CRC‑16, affichage de debug).
 *
 * Le CRC utilise le polynôme CCITT 0x1021 et la valeur initiale 0xFFFF,
 * comme dans de nombreux protocoles de communication.
 */

#include "utils.h"
#include <stdio.h>
#include <string.h>

/* Polynôme CRC‑16‑CCITT : x^16 + x^12 + x^5 + 1 => 0x1021 */
#define CRC16_POLY  0x1021
#define CRC16_INIT  0xFFFF

/**
 * Calcule le CRC‑16 (CCITT) (Cyclic Redundancy Check).
 * Traite les données octet par octet, 8 bits de haut en bas, en appliquant
 * le polynôme au CRC courant (décalages et XOR).
 */
uint16_t crc16(const uint8_t *data, size_t len)
{
    uint16_t crc = CRC16_INIT;
    size_t i;
    int b;

    if (!data)
        return crc;

    for (i = 0; i < len; i++) {
        crc ^= (uint16_t)data[i] << 8;
        for (b = 0; b < 8; b++) {
            if (crc & 0x8000)
                crc = (crc << 1) ^ CRC16_POLY;
            else
                crc = crc << 1;
        }
    }
    return crc;
}

/**
 * Affiche une zone mémoire en hexadécimal pour faciliter le débogage du contenu
 * des trames.
 * Format : TAG: xx xx xx xx ...
 */
void debug_hex_dump(const char *tag, const uint8_t *data, size_t len)
{
    size_t i;
    if (!tag) tag = "";
    if (!data) return;
    printf("%s: ", tag);
    for (i = 0; i < len && i < 64; i++)  /* Affiche au maximum 64 octets. */
        printf("%02x ", data[i]);
    if (len > 64)
        printf("... (%zu bytes total)", len);
    printf("\n");
}
