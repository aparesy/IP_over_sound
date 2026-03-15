/**
 * main.c - Point d’entrée du programme IP over Sound et gestion des threads.
 *
 * Flux global :
 * 1. Ouvrir le périphérique TUN, initialiser l’audio, créer les modems TX/RX.
 * 2. Démarrer le thread TX : lecture depuis TUN -> encapsulation en trames
 *    -> modulation -> écriture vers la carte son.
 * 3. Démarrer le thread RX : lecture audio -> démodulation -> recherche de trames
 *    / décapsulation -> écriture dans TUN.
 * 4. Le thread principal attend Ctrl+C ou un signal, puis nettoie et quitte.
 */

#include "common.h"
#include "tun_dev.h"
#include "audio_dev.h"
#include "modem.h"
#include "protocol.h"
#include "utils.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <pthread.h>
#include <stdint.h>
#include <unistd.h>

/* Indicateur global d’exécution : mis à 0 lors de la réception de SIGINT,
 * ce qui provoque la sortie de tous les threads. */
static volatile int g_running = 1;

static void signal_handler(int sig)
{
    (void)sig;
    g_running = 0;
}

/**
 * Convertit une trame octet par octet en flux de bits (8 bits par octet,
 * bit de poids fort en premier).
 * Sert à transformer `frame` en tableau de bits pour modem_tx_modulate.
 */
static void frame_to_bits(const uint8_t *frame, int frame_len, uint8_t *bits_out, int *nbits_out)
{
    int i, b;
    *nbits_out = frame_len * 8;
    for (i = 0; i < frame_len; i++) {
        for (b = 7; b >= 0; b--) {
            int bit_idx = i * 8 + (7 - b);
            int byte_idx = bit_idx / 8;
            int bit_in_byte = 7 - (bit_idx % 8);
            if (frame[i] & (1 << b))
                bits_out[byte_idx] |= (1 << bit_in_byte);
            else
                bits_out[byte_idx] &= ~(1 << bit_in_byte);
        }
    }
}

/**
 * Thread TX : lit des paquets IP depuis TUN -> les encapsule en trames
 * -> les module -> écrit les échantillons vers le haut‑parleur.
 */
static void *tx_thread_func(void *arg)
{
    int tun_fd = *(int *)arg;
    uint8_t *ip_buf;
    uint8_t *frame_buf;
    uint8_t *bits_buf;
    sample_t *samples_buf;
    int frame_len, nbits, nsamples, i, written;
    modem_tx_handle_t mod_tx;
    audio_handle_t audio = NULL;  /* Idéalement passé depuis main, ici simplifié via une variable globale/paramètre. */
    const size_t max_samples = (size_t)(MAX_FRAME_LEN * 8) * SAMPLES_PER_BIT;

    ip_buf    = (uint8_t *)malloc(MAX_FRAME_PAYLOAD);
    frame_buf = (uint8_t *)malloc(MAX_FRAME_LEN);
    bits_buf  = (uint8_t *)malloc(MAX_FRAME_LEN + 1);  /* Stocke les bits regroupés par octet. */
    samples_buf = (sample_t *)malloc(max_samples * sizeof(sample_t));
    mod_tx    = modem_tx_create();

    if (!ip_buf || !frame_buf || !bits_buf || !samples_buf || !mod_tx) {
        fprintf(stderr, "tx_thread: alloc or modem_tx_create failed\n");
        if (ip_buf) free(ip_buf);
        if (frame_buf) free(frame_buf);
        if (bits_buf) free(bits_buf);
        if (samples_buf) free(samples_buf);
        if (mod_tx) modem_tx_destroy(mod_tx);
        return NULL;
    }

    /* Récupère audio et tun_fd depuis les paramètres/main ; ici simplifié
     * avec une variable globale/statique (voir plus bas). */
    extern audio_handle_t g_audio_handle;
    audio = g_audio_handle;

    while (g_running && audio) {
        int n = tun_read(tun_fd, ip_buf, MAX_FRAME_PAYLOAD);
        if (n <= 0) continue;

        frame_len = protocol_encapsulate(ip_buf, n, frame_buf);
        if (frame_len <= 0) continue;

        nbits = frame_len * 8;
        frame_to_bits(frame_buf, frame_len, bits_buf, &nbits);
        nsamples = modem_tx_modulate(mod_tx, bits_buf, nbits, samples_buf);
        if (nsamples <= 0) continue;

        /* Écrit vers la carte son par blocs pour éviter des écritures trop grosses. */
        for (i = 0; i < nsamples && g_running; i += AUDIO_FRAMES_PER_BUFFER) {
            written = (nsamples - i) < AUDIO_FRAMES_PER_BUFFER ? (nsamples - i) : AUDIO_FRAMES_PER_BUFFER;
            if (audio_write(audio, samples_buf + i, written) != 0)
                break;
        }
    }

    free(ip_buf);
    free(frame_buf);
    free(bits_buf);
    free(samples_buf);
    modem_tx_destroy(mod_tx);
    return NULL;
}

/** Tampon de bits côté réception : il faut accumuler plusieurs blocs démodulés
 *  pour reconstituer une trame complète. */
#define RX_BIT_BUF_BYTES  (MAX_FRAME_LEN * 4)
#define RX_BIT_BUF_BITS   (RX_BIT_BUF_BYTES * 8)

/** Extrait une portion de flux de bits et la convertit en octets (pour
 *  reconstituer une trame dans frame_buf à partir de bits). */
static void bits_to_bytes(const uint8_t *bits, int bit_start, int nbytes, uint8_t *bytes_out)
{
    int by, b, bi;
    for (by = 0; by < nbytes; by++) {
        bytes_out[by] = 0;
        for (b = 0; b < 8; b++) {
            bi = bit_start + by * 8 + b;
            if (bits[bi / 8] & (1 << (7 - bi % 8)))
                bytes_out[by] |= (1 << (7 - b));
        }
    }
}

/** Ajoute nbits bits de src à dest à partir de dest_bit_count, et
 *  retourne le nouveau nombre total de bits dans dest. */
static int bits_append(uint8_t *dest, int dest_bit_count, const uint8_t *src, int nbits)
{
    int i;
    for (i = 0; i < nbits; i++) {
        int di = dest_bit_count + i;
        int si_byte = i / 8, si_bit = 7 - (i % 8);
        if (src[si_byte] & (1 << si_bit))
            dest[di / 8] |= (1 << (7 - di % 8));
        else
            dest[di / 8] &= ~(1 << (7 - di % 8));
    }
    return dest_bit_count + nbits;
}

/** Supprime dans buf les bits de l’intervalle [from_bit, from_bit+n_bits)
 *  et décale le reste vers l’avant. */
static void bits_remove(uint8_t *buf, int *bit_count_inout, int from_bit, int n_bits)
{
    int new_count = *bit_count_inout - n_bits;
    int i;
    if (new_count <= 0) {
        *bit_count_inout = 0;
        return;
    }
    for (i = 0; i < new_count; i++) {
        int src_i = from_bit + n_bits + i;
        int dst_i = i;
        int bit_val = (buf[src_i / 8] & (1 << (7 - src_i % 8))) ? 1 : 0;
        if (bit_val)
            buf[dst_i / 8] |= (1 << (7 - dst_i % 8));
        else
            buf[dst_i / 8] &= ~(1 << (7 - dst_i % 8));
    }
    *bit_count_inout = new_count;
}

/**
 * Thread RX : lit les échantillons depuis le microphone -> démodule en bits
 * -> accumule les bits -> cherche la synchronisation de trame -> décapsule
 * et écrit le paquet IP résultant dans TUN.
 */
static void *rx_thread_func(void *arg)
{
    int tun_fd = *(int *)arg;
    sample_t *audio_buf;
    uint8_t *demod_buf;   /* Petit bloc de bits obtenu à chaque démodulation. */
    uint8_t *rx_bits_buf; /* Flux de bits accumulé. */
    uint8_t *frame_buf;
    uint8_t *payload_buf;
    int rx_bit_count = 0; /* Nombre actuel de bits valides accumulés. */
    int nread, nbits, sync_pos, payload_len;
    int frame_byte_len, frame_len_bits;
    modem_rx_handle_t mod_rx;
    audio_handle_t audio = NULL;

    extern audio_handle_t g_audio_handle;
    audio = g_audio_handle;

    audio_buf   = (sample_t *)malloc(AUDIO_FRAMES_PER_BUFFER * sizeof(sample_t));
    demod_buf   = (uint8_t *)malloc(RX_BIT_BUF_BYTES);
    rx_bits_buf = (uint8_t *)malloc(RX_BIT_BUF_BYTES);
    frame_buf   = (uint8_t *)malloc(MAX_FRAME_LEN);
    payload_buf = (uint8_t *)malloc(MAX_FRAME_PAYLOAD);
    mod_rx      = modem_rx_create();

    if (!audio_buf || !demod_buf || !rx_bits_buf || !frame_buf || !payload_buf || !mod_rx) {
        fprintf(stderr, "rx_thread: alloc or modem_rx_create failed\n");
        if (audio_buf) free(audio_buf);
        if (demod_buf) free(demod_buf);
        if (rx_bits_buf) free(rx_bits_buf);
        if (frame_buf) free(frame_buf);
        if (payload_buf) free(payload_buf);
        if (mod_rx) modem_rx_destroy(mod_rx);
        return NULL;
    }

    while (g_running && audio) {
        nread = audio_read(audio, audio_buf, AUDIO_FRAMES_PER_BUFFER);
        if (nread <= 0) continue;

        memset(demod_buf, 0, RX_BIT_BUF_BYTES);
        nbits = modem_rx_demodulate(mod_rx, audio_buf, nread, demod_buf, RX_BIT_BUF_BITS);
        if (nbits <= 0) continue;

        /* Ajoute ce bloc de bits au tampon accumulé. */
        if (rx_bit_count + nbits > RX_BIT_BUF_BITS) {
            /* Tampon plein : on jette la première moitié pour libérer de la place. */
            bits_remove(rx_bits_buf, &rx_bit_count, 0, rx_bit_count / 2);
        }
        rx_bit_count = bits_append(rx_bits_buf, rx_bit_count, demod_buf, nbits);

        /* Recherche de la synchronisation dans le flux de bits accumulé. */
        sync_pos = protocol_find_sync(rx_bits_buf, rx_bit_count);
        if (sync_pos < 0) continue;

        /* Il faut au moins FRAME_HEADER_LEN pour pouvoir lire le champ longueur. */
        if (sync_pos + FRAME_HEADER_LEN * 8 > rx_bit_count) continue;

        bits_to_bytes(rx_bits_buf, sync_pos, FRAME_HEADER_LEN, frame_buf);
        frame_byte_len = (frame_buf[SYNC_LEN] << 8) | frame_buf[SYNC_LEN + 1];
        if (frame_byte_len <= 0 || frame_byte_len > MAX_FRAME_PAYLOAD) {
            /* Longueur invalide : on jette les bits jusqu’à la fin des SYNC
             * pour éviter un blocage. */
            bits_remove(rx_bits_buf, &rx_bit_count, 0, sync_pos + SYNC_LEN * 8);
            continue;
        }
        frame_len_bits = (FRAME_HEADER_LEN + frame_byte_len + CRC_BYTES) * 8;
        if (sync_pos + frame_len_bits > rx_bit_count) continue;

        bits_to_bytes(rx_bits_buf, sync_pos, FRAME_HEADER_LEN + frame_byte_len + CRC_BYTES, frame_buf);
        payload_len = protocol_decapsulate(frame_buf, FRAME_HEADER_LEN + frame_byte_len + CRC_BYTES,
                                           payload_buf, MAX_FRAME_PAYLOAD);
        if (payload_len > 0)
            tun_write(tun_fd, payload_buf, payload_len);

        /* Consomme les bits correspondant à cette trame. */
        bits_remove(rx_bits_buf, &rx_bit_count, sync_pos, frame_len_bits);
    }

    free(audio_buf);
    free(demod_buf);
    free(rx_bits_buf);
    free(frame_buf);
    free(payload_buf);
    modem_rx_destroy(mod_rx);
    return NULL;
}

/* Handle audio global utilisé par les threads TX/RX (pourrait aussi être passé
 * en paramètre). */
audio_handle_t g_audio_handle = NULL;

int main(int argc, char *argv[])
{
    int tun_fd;
    pthread_t tx_tid, rx_tid;
    const char *tun_name = TUN_DEV_NAME;

    if (argc >= 2)
        tun_name = argv[1];

    signal(SIGINT, signal_handler);

    printf("IP over Sound: opening TUN %s, initializing audio...\n", tun_name);
    tun_fd = tun_open(tun_name);
    if (tun_fd < 0) {
        fprintf(stderr, "Failed to open TUN. Try: sudo ./ipo_sound\n");
        return 1;
    }

    g_audio_handle = audio_init();
    if (!g_audio_handle) {
        fprintf(stderr, "Failed to init audio (PortAudio).\n");
        tun_close(tun_fd);
        return 1;
    }

    pthread_create(&tx_tid, NULL, tx_thread_func, &tun_fd);
    pthread_create(&rx_tid, NULL, rx_thread_func, &tun_fd);

    printf("Running. Press Ctrl+C to stop.\n");
    while (g_running)
        sleep(1);

    g_running = 0;
    pthread_join(tx_tid, NULL);
    pthread_join(rx_tid, NULL);

    audio_cleanup(g_audio_handle);
    g_audio_handle = NULL;
    tun_close(tun_fd);
    printf("Exited.\n");
    return 0;
}
