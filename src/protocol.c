/**
 * protocol.c - Implémentation de l’encapsulation et du décapsulage de trames.
 *
 * Format de trame :
 *   [SYNC_BYTE répété SYNC_LEN fois][champ longueur 2 octets big‑endian]
 *   [charge utile][CRC16 2 octets big‑endian].
 * Le champ longueur est le nombre d’octets de charge utile (hors en‑tête et CRC),
 * ce qui facilite l’allocation côté réception et la vérification du CRC.
 */

#include "protocol.h"
#include "utils.h"
#include <string.h>

/**
 * Encapsulation : SYNC + longueur (big‑endian) + charge utile + CRC16 (big‑endian).
 *
 * @param payload       charge utile, c’est‑à‑dire les données du paquet IP à encapsuler
 * @param payload_len   longueur de la charge utile
 * @param frame_out     tampon de sortie pour la trame
 * @return              longueur totale de la trame produite
 */
int protocol_encapsulate(const uint8_t *payload, int payload_len, uint8_t *frame_out)
{
    uint16_t crc;
    int i;

    if (!payload || !frame_out || payload_len <= 0 || payload_len > MAX_FRAME_PAYLOAD)
        return 0;

    /* 1. Mot(s) de synchronisation. */
    for (i = 0; i < SYNC_LEN; i++)
        frame_out[i] = SYNC_BYTE; /** Équivalent à *(frame_out + i), modifie le contenu. */

    /* 2. Champ longueur (big‑endian). */
    frame_out[SYNC_LEN]     = (payload_len >> 8) & 0xFF; /** Octet de poids fort masqué avec 0xFF. */
    frame_out[SYNC_LEN + 1] = payload_len & 0xFF;        /** Octet de poids faible masqué avec 0xFF. */

    /* 3. Copie de la charge utile dans le tampon de trame. */
    memcpy(frame_out + FRAME_HEADER_LEN, payload, payload_len); /** Copie à partir du 4ᵉ octet pour une longueur payload_len. */

    /* 4. CRC : calculé sur « longueur + charge utile », placé en fin de trame (big‑endian). */
    crc = crc16(frame_out + SYNC_LEN, LEN_FIELD_BYTES + payload_len);
    frame_out[FRAME_HEADER_LEN + payload_len]     = (crc >> 8) & 0xFF;
    frame_out[FRAME_HEADER_LEN + payload_len + 1] = crc & 0xFF;

    return FRAME_HEADER_LEN + payload_len + CRC_BYTES;
}

/**
 * Décapsulation : vérifie le CRC puis extrait la charge utile.
 * @param frame        tampon contenant une trame en entrée
 * @param frame_len    longueur totale de la trame
 * @param payload_out  tampon de sortie pour la charge utile (paquet IP)
 * @param max_payload  taille maximale du tampon de sortie
 * @return             longueur de la charge utile extraite, ou -1 en cas d’erreur
 */
int protocol_decapsulate(const uint8_t *frame, int frame_len,
                         uint8_t *payload_out, int max_payload)
{
    uint16_t len_u16, crc_stored, crc_computed;

    if (!frame || !payload_out || frame_len < FRAME_HEADER_LEN + CRC_BYTES)
        return -1;

    /* Lecture du champ longueur (big‑endian). */
    len_u16 = (frame[SYNC_LEN] << 8) | frame[SYNC_LEN + 1];
    if (len_u16 <= 0 || len_u16 > MAX_FRAME_PAYLOAD)
        return -1;
    if (frame_len < FRAME_HEADER_LEN + len_u16 + CRC_BYTES)
        return -1;
    if (len_u16 > max_payload)
        return -1;

    /* Vérification du CRC : calcul sur « longueur + charge utile » et comparaison
     * avec les deux octets en fin de trame. */
    crc_computed = crc16(frame + SYNC_LEN, LEN_FIELD_BYTES + len_u16);
    crc_stored   = (frame[FRAME_HEADER_LEN + len_u16] << 8)
                   | frame[FRAME_HEADER_LEN + len_u16 + 1];
    if (crc_computed != crc_stored)
        return -1;  /* CRC incorrect, trame rejetée. */

    memcpy(payload_out, frame + FRAME_HEADER_LEN, len_u16); /** Copie de la charge utile dans le tampon de sortie (à partir du 4ᵉ octet). */
    return (int)len_u16;
}

/**
 * Recherche la synchronisation dans un flux de bits : SYNC_BYTE répété SYNC_LEN fois,
 * apparié bit par bit.
 * Dans `bits`, chaque octet contient 8 bits, le bit de poids fort en premier.
 */
int protocol_find_sync(const uint8_t *bits, int nbits)
{
    int i, j, k;
    int need_bits = SYNC_LEN * 8;

    if (!bits || nbits < need_bits)
        return -1;

    for (i = 0; i <= nbits - need_bits; i++) {
        for (j = 0; j < SYNC_LEN; j++) {
            int byte_val = 0;
            for (k = 0; k < 8; k++) {
                int bit_idx = i + j * 8 + k;
                int byte_idx = bit_idx / 8;
                int bit_in_byte = 7 - (bit_idx % 8);
                if (bits[byte_idx] & (1 << bit_in_byte))
                    byte_val |= (1 << (7 - k));
            }
            if (byte_val != SYNC_BYTE)
                break;
        }
        if (j == SYNC_LEN)
            return i;
    }
    return -1;
}
