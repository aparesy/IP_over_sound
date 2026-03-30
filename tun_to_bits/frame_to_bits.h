/**
 * frame_to_bits.h - Conversion trame (octets) -> flux de bits
 * (8 bits par octet, bit de poids fort en premier).
 */

#ifndef FRAME_TO_BITS_H
#define FRAME_TO_BITS_H

#include <stdint.h>

/**
 * Convertit une trame en flux de bits.
 * @param frame     données de trame
 * @param frame_len taille de la trame en octets
 * @param bits_out  flux de bits en sortie (stocké par octets, 8 bits par octet)
 * @param nbits_out nombre total de bits en sortie (= frame_len * 8)
 */
void frame_to_bits(const uint8_t *frame, int frame_len, uint8_t *bits_out, int *nbits_out);

#endif /* FRAME_TO_BITS_H */
