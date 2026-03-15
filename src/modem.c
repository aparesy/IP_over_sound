/**
 * modem.c - Implémentation de la modulation/démodulation FSK.
 *
 * Modulation : chaque bit correspond à SAMPLES_PER_BIT échantillons.
 *              Le bit 0 utilise une sinusoïde de fréquence FSK_FREQ_0 Hz,
 *              le bit 1 une sinusoïde de fréquence FSK_FREQ_1 Hz.
 * Démodulation : pour chaque durée de bit, on applique une détection FSK
 *                simplifiée (énergie ou nombre de passages par zéro) pour
 *                décider 0/1.
 */

#include "modem.h"
#include "common.h"
#include <stdlib.h>
#include <string.h>
#include <math.h>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

/* ========== Modulateur : stocke la phase interne pour garantir la continuité de la forme d’onde ========== */
struct modem_tx {
    double phase0;  /* Phase actuelle de la porteuse 0 (radians). */
    double phase1;  /* Phase actuelle de la porteuse 1 (radians). */
};

modem_tx_handle_t modem_tx_create(void)
{
    struct modem_tx *tx = (struct modem_tx *)calloc(1, sizeof(struct modem_tx));
    return (modem_tx_handle_t)tx;
}

/* Génère une portion de sinusoïde, amplitude 0.3 pour éviter l’écrêtage,
 * et met à jour la phase. `sample_t *out` est le tampon de sortie :
 * out[i] est le i‑ème échantillon de type sample_t (float), et l’appelant
 * doit avoir préalloué un espace suffisant. */
static void gen_sine(double freq, int nsamples, sample_t *out,
                     double *phase_inout)
{
    double phase = *phase_inout;
    double step = 2.0 * M_PI * freq / SAMPLE_RATE;
    int i;

    for (i = 0; i < nsamples; i++) {
        out[i] = (sample_t)(0.3 * sin(phase));
        phase += step;
    }
    /* Normalise la phase dans [0, 2π) pour éviter l’erreur cumulée. */
    while (phase >= 2.0 * M_PI) phase -= 2.0 * M_PI;
    while (phase < 0) phase += 2.0 * M_PI;
    *phase_inout = phase;
}

/**
 * Module nbits bits d’un tampon `bits` en un signal de sortie.
 * Dans `bits`, chaque octet contient 8 bits, le bit de poids fort étant envoyé en premier.
 */
int modem_tx_modulate(modem_tx_handle_t h, const uint8_t *bits, int nbits, sample_t *out_buf)
{
    struct modem_tx *tx = (struct modem_tx *)h;
    int i, bit_idx, byte_idx, bit_in_byte;
    int out_idx = 0;

    if (!tx || !bits || !out_buf || nbits <= 0)
        return 0;

    for (bit_idx = 0; bit_idx < nbits; bit_idx++) {
        byte_idx   = bit_idx / 8;
        bit_in_byte = 7 - (bit_idx % 8);
        if (bits[byte_idx] & (1 << bit_in_byte))
            gen_sine(FSK_FREQ_1, SAMPLES_PER_BIT, out_buf + out_idx, &tx->phase1);
        else
            gen_sine(FSK_FREQ_0, SAMPLES_PER_BIT, out_buf + out_idx, &tx->phase0);
        out_idx += SAMPLES_PER_BIT;
    }
    return out_idx;
}

void modem_tx_destroy(modem_tx_handle_t h)
{
    free(h);
}

/* ========== Démodulateur : pour chaque bit, décide 0/1 à partir de l’énergie
 *            et du nombre de passages par zéro (amplitude de “tremblement”
 *            de la forme d’onde). ========== */
struct modem_rx {
    /* Extensible : pourrait stocker des échantillons partiellement traités
     * pour les concaténer avec le bloc suivant. */
    sample_t *remain_buf;
    int remain_len;
};

modem_rx_handle_t modem_rx_create(void)
{
    struct modem_rx *rx = (struct modem_rx *)calloc(1, sizeof(struct modem_rx));
    if (!rx) return NULL;
    /* Tampon pour un éventuel recollage entre blocs (simplifié ici, non utilisé). */
    rx->remain_buf = NULL;
    rx->remain_len = 0;
    return (modem_rx_handle_t)rx;
}

/**
 * Décide 0 ou 1 à partir de SAMPLES_PER_BIT échantillons :
 * compare de manière approximative l’énergie des composantes 1200 Hz et 2400 Hz.
 * Implémentation simplifiée : utilise le nombre de passages par zéro pour
 * distinguer la basse fréquence (0) de la haute fréquence (1).
 */
static int demodulate_bit(const sample_t *samples, int nsamples)
{
    int zeros = 0, i;
    double avg_abs = 0;
    /* Comptage des passages par zéro : moins nombreux en basse fréquence,
     * plus nombreux en haute fréquence. */
    for (i = 1; i < nsamples; i++) {
        if ((samples[i-1] >= 0 && samples[i] < 0) || (samples[i-1] < 0 && samples[i] >= 0))
            zeros++;
        avg_abs += fabs(samples[i]);
    }
    avg_abs /= nsamples;
    /* Seuil grossier : beaucoup de passages par zéro → 1 (haute fréquence). */
    return (zeros > nsamples / 4) ? 1 : 0;
}

int modem_rx_demodulate(modem_rx_handle_t h, const sample_t *samples, int nsamples,
                        uint8_t *bits, int max_bits)
{
    struct modem_rx *rx = (struct modem_rx *)h;
    int nbits = 0;
    int i, byte_idx, bit_in_byte;

    if (!rx || !samples || !bits || nsamples < SAMPLES_PER_BIT || max_bits <= 0)
        return 0;

    for (i = 0; i + SAMPLES_PER_BIT <= nsamples && nbits < max_bits; i += SAMPLES_PER_BIT) {
        int bit = demodulate_bit(samples + i, SAMPLES_PER_BIT);
        byte_idx   = nbits / 8;
        bit_in_byte = 7 - (nbits % 8);
        if (bit)
            bits[byte_idx] |= (1 << bit_in_byte);
        else
            bits[byte_idx] &= ~(1 << bit_in_byte);
        nbits++;
    }
    return nbits;
}

void modem_rx_destroy(modem_rx_handle_t h)
{
    struct modem_rx *rx = (struct modem_rx *)h;
    if (rx) {
        free(rx->remain_buf);
        free(rx);
    }
}
