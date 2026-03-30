/**
 * bits_to_wav.c - Flux de bits -> modulation FSK -> fichier WAV
 *
 * Usage :
 *   bits_to_wav <input.bin> <output.wav>  Lit un flux de bits depuis un fichier,
 *                                         module et écrit un WAV
 *   bits_to_wav --test                    Test intégré : génère un flux de bits,
 *                                         écrit output/test.wav
 *
 * Format du fichier de bits : octets bruts, 8 bits par octet, bit de poids fort
 * en premier (même convention que le modem).
 */

#include "../include/common.h"
#include "../include/modem.h"
#include "wav_writer.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

static int read_bits_from_file(const char *path, uint8_t *bits_out, int max_bytes, int *nbits_out)
{
    FILE *fp;
    int n;

    fp = fopen(path, "rb");
    if (!fp) return -1;
    n = (int)fread(bits_out, 1, max_bytes, fp);
    fclose(fp);
    if (n <= 0) return -1;
    *nbits_out = n * 8;
    return 0;
}

/** Test intégré : génère un flux de bits assez long, module et écrit output/test.wav. */
static int run_test(void)
{
#define TEST_BITS_BUF_SIZE  1024
    uint8_t bits_buf[TEST_BITS_BUF_SIZE];
    int nbits;
    int i;
    size_t max_samples;
    sample_t *samples_buf;
    modem_tx_handle_t mod_tx;
    const char *out_path = "output/test.wav";

    /* Ancien flux de test court (commenté) :
     * bits_buf[0] = 0x7E;
     * bits_buf[1] = 0x7E;
     * bits_buf[2] = 0x00;
     * nbits = 24;
     */

    /* Nouveau flux de test long : en‑tête de synchro + motif répété,
     * total TEST_BITS_BUF_SIZE octets. */
    bits_buf[0] = 0x7E;
    bits_buf[1] = 0x7E;
    for (i = 2; i < TEST_BITS_BUF_SIZE; i++) {
        /* Alterne 0x55/0xAA pour faire alterner basse/haute fréquence,
         * afin d’entendre un rythme ; insère aussi 0x00/0xFF entre deux. */
        if (i % 4 == 2) bits_buf[i] = 0x55;
        else if (i % 4 == 3) bits_buf[i] = 0xAA;
        else if (i % 4 == 0) bits_buf[i] = 0x00;
        else bits_buf[i] = 0xFF;
    }
    nbits = TEST_BITS_BUF_SIZE * 8;

    mod_tx = modem_tx_create();
    if (!mod_tx) {
        fprintf(stderr, "modem_tx_create failed\n");
        return -1;
    }

    max_samples = (size_t)nbits * SAMPLES_PER_BIT;
    samples_buf = (sample_t *)malloc(max_samples * sizeof(sample_t));
    if (!samples_buf) {
        modem_tx_destroy(mod_tx);
        fprintf(stderr, "malloc samples_buf failed\n");
        return -1;
    }

    int nsamples = modem_tx_modulate(mod_tx, bits_buf, nbits, samples_buf);
    modem_tx_destroy(mod_tx);
    if (nsamples <= 0) {
        free(samples_buf);
        fprintf(stderr, "modem_tx_modulate failed\n");
        return -1;
    }

    if (wav_write(samples_buf, nsamples, out_path) != 0) {
        free(samples_buf);
        fprintf(stderr, "wav_write failed: %s\n", out_path);
        return -1;
    }

    free(samples_buf);
    printf("Test OK: %d bits -> %d samples -> %s\n", nbits, nsamples, out_path);
    return 0;
}

int main(int argc, char *argv[])
{
    uint8_t *bits_buf;
    sample_t *samples_buf;
    modem_tx_handle_t mod_tx;
    int nbits = 0, nsamples, max_bytes;
    size_t max_samples;

    if (argc >= 2 && (strcmp(argv[1], "--test") == 0 || strcmp(argv[1], "-t") == 0)) {
        return run_test() == 0 ? 0 : 1;
    }

    if (argc < 3) {
        fprintf(stderr, "Usage: %s <input.bin> <output.wav>\n", argv[0]);
        fprintf(stderr, "   or: %s --test\n", argv[0]);
        return 1;
    }

    max_bytes = 1024 * 1024; /* Fichier de bits : 1 Mo max. */
    bits_buf = (uint8_t *)malloc(max_bytes);
    if (!bits_buf) {
        fprintf(stderr, "malloc bits_buf failed\n");
        return 1;
    }

    if (read_bits_from_file(argv[1], bits_buf, max_bytes, &nbits) != 0) {
        fprintf(stderr, "Failed to read: %s\n", argv[1]);
        free(bits_buf);
        return 1;
    }

    mod_tx = modem_tx_create();
    if (!mod_tx) {
        free(bits_buf);
        fprintf(stderr, "modem_tx_create failed\n");
        return 1;
    }

    max_samples = (size_t)nbits * SAMPLES_PER_BIT;
    samples_buf = (sample_t *)malloc(max_samples * sizeof(sample_t));
    if (!samples_buf) {
        modem_tx_destroy(mod_tx);
        free(bits_buf);
        fprintf(stderr, "malloc samples_buf failed\n");
        return 1;
    }

    nsamples = modem_tx_modulate(mod_tx, bits_buf, nbits, samples_buf);
    modem_tx_destroy(mod_tx);
    free(bits_buf);
    if (nsamples <= 0) {
        free(samples_buf);
        fprintf(stderr, "modem_tx_modulate failed\n");
        return 1;
    }

    if (wav_write(samples_buf, nsamples, argv[2]) != 0) {
        free(samples_buf);
        fprintf(stderr, "wav_write failed: %s\n", argv[2]);
        return 1;
    }

    free(samples_buf);
    printf("OK: %d bits -> %d samples -> %s\n", nbits, nsamples, argv[2]);
    return 0;
}
