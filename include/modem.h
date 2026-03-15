/**
 * modem.h - Interface du modem FSK (couche physique).
 *
 * Modulation : convertit un flux de bits en forme d’onde audio
 * (chaque bit correspond à une portion de sinusoïde).
 * Démodulation : reconvertit la forme d’onde audio en flux de bits.
 * Le modem ne gère pas la structure des trames, il ne fait que la
 * conversion 0/1 ↔ signal.
 *
 * Où se situent les « bits déjà présents » ?
 * - Couche réseau : uniquement des paquets IP (octets), lus/écrits via TUN.
 * - Couche liaison : `protocol` encapsule le paquet IP en trame
 *   (SYNC, longueur, CRC), toujours en octets ; puis, pour passer à
 *   la couche physique, on déroule cette trame en flux de bits 0/1.
 *
 * Les « bits déjà présents » sont donc la représentation en bits d’une
 * trame de couche liaison (y compris SYNC, longueur, IP, CRC), qui
 * sert d’entrée au modem pour la modulation physique.
 */

#ifndef MODEM_H
#define MODEM_H

#include "common.h"
#include <stdint.h>

/** Handle/état du modulateur ; stocke notamment la phase, reste opaque à l’extérieur. */
typedef void* modem_tx_handle_t;

/** Handle/état du démodulateur ; stocke les tampons et l’état interne. */
typedef void* modem_rx_handle_t;

/**
 * Crée un modulateur (pour l’émission) et initialise son état interne.
 * @return handle, ou NULL en cas d’échec
 */
modem_tx_handle_t modem_tx_create(void);

/**
 * Module un flux de bits en échantillons audio et les écrit dans out_buf.
 * @param h        handle retourné par modem_tx_create
 * @param bits     tableau de bits, 8 bits par octet, bit de poids fort en premier
 * @param nbits    nombre total de bits
 * @param out_buf  tampon d’échantillons de sortie, préalloué (taille ≈ nbits * SAMPLES_PER_BIT)
 * @return         nombre d’échantillons effectivement écrits
 */
int modem_tx_modulate(modem_tx_handle_t h, const uint8_t *bits, int nbits, sample_t *out_buf);

/**
 * Détruit le modulateur.
 * @param h handle
 */
void modem_tx_destroy(modem_tx_handle_t h);

/**
 * Crée un démodulateur (pour la réception).
 * @return handle, ou NULL en cas d’échec
 */
modem_rx_handle_t modem_rx_create(void);

/**
 * Démodule une portion d’échantillons audio en bits et les écrit dans bits.
 * @param h         handle retourné par modem_rx_create
 * @param samples   échantillons d’entrée
 * @param nsamples  nombre d’échantillons
 * @param bits      tableau de bits en sortie (8 bits par octet)
 * @param max_bits  nombre maximum de bits à produire (évite les dépassements)
 * @return          nombre réel de bits produits
 */
int modem_rx_demodulate(modem_rx_handle_t h, const sample_t *samples, int nsamples,
                        uint8_t *bits, int max_bits);

/**
 * Détruit le démodulateur.
 * @param h handle
 */
void modem_rx_destroy(modem_rx_handle_t h);

#endif /* MODEM_H */
