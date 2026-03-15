/**
 * audio_dev.c - Implémentation de l’accès à la carte son via PortAudio.
 *
 * Utilise PortAudio pour ouvrir les périphériques d’entrée/sortie par défaut
 * et effectuer les lectures/écritures selon SAMPLE_RATE et
 * AUDIO_FRAMES_PER_BUFFER. L’édition doit être liée avec -lportaudio.
 */

#include "audio_dev.h"
#include "common.h"
#include <stdlib.h>
#include <stdio.h>
#include <portaudio.h>

/** Dans l’API PortAudio, une “frame” est l’ensemble des échantillons de tous
 *  les canaux à un instant donné ; cela n’a rien à voir avec une trame de
 *  couche liaison. */

/** Handle interne : stocke les pointeurs PaStream. */
struct audio_handle {
    PaStream *stream_in;   /* Flux d’entrée (microphone). */
    PaStream *stream_out;  /* Flux de sortie (haut‑parleur). */
    int opened;            /* Indique si les flux ont été ouverts avec succès. */
};


/** Initialise les périphériques audio, ouvre les entrées/sorties par défaut
 *  et les configure selon SAMPLE_RATE et AUDIO_FRAMES_PER_BUFFER. */
audio_handle_t audio_init(void)
{
    struct audio_handle *h;
    PaError err;

    err = Pa_Initialize();
    if (err != paNoError) {
        fprintf(stderr, "audio_init: Pa_Initialize failed: %s\n", Pa_GetErrorText(err));
        return NULL;
    }

    h = (struct audio_handle *)calloc(1, sizeof(struct audio_handle));
    if (!h) {
        Pa_Terminate();
        return NULL;
    }

    /* Ouvre le périphérique d’entrée par défaut (microphone). */
    err = Pa_OpenDefaultStream(&h->stream_in,
                               1,   /* Entrée mono. */
                               0,   /* Pas de sortie. */
                               paFloat32,
                               SAMPLE_RATE,
                               AUDIO_FRAMES_PER_BUFFER,
                               NULL, NULL);
    if (err != paNoError) {
        fprintf(stderr, "audio_init: OpenDefaultStream (input) failed: %s\n", Pa_GetErrorText(err));
        free(h);
        Pa_Terminate();
        return NULL;
    }

    /* Ouvre le périphérique de sortie par défaut (haut‑parleur). */
    err = Pa_OpenDefaultStream(&h->stream_out,
                               0,   /* Pas d’entrée. */
                               1,   /* Sortie mono. */
                               paFloat32,
                               SAMPLE_RATE,
                               AUDIO_FRAMES_PER_BUFFER,
                               NULL, NULL);
    if (err != paNoError) {
        fprintf(stderr, "audio_init: OpenDefaultStream (output) failed: %s\n", Pa_GetErrorText(err));
        Pa_CloseStream(h->stream_in);
        free(h);
        Pa_Terminate();
        return NULL;
    }

    err = Pa_StartStream(h->stream_in);
    if (err != paNoError) {
        fprintf(stderr, "audio_init: StartStream input failed: %s\n", Pa_GetErrorText(err));
        Pa_CloseStream(h->stream_in);
        Pa_CloseStream(h->stream_out);
        free(h);
        Pa_Terminate();
        return NULL;
    }

    err = Pa_StartStream(h->stream_out);
    if (err != paNoError) {
        fprintf(stderr, "audio_init: StartStream output failed: %s\n", Pa_GetErrorText(err));
        Pa_StopStream(h->stream_in);
        Pa_CloseStream(h->stream_in);
        Pa_CloseStream(h->stream_out);
        free(h);
        Pa_Terminate();
        return NULL;
    }

    h->opened = 1;
    return (audio_handle_t)h;
}

/** Écrit nframes échantillons vers le haut‑parleur. */
int audio_write(audio_handle_t handle, const sample_t *buf, int nframes)
{
    struct audio_handle *h = (struct audio_handle *)handle;
    PaError err;

    if (!h || !h->opened || !buf || nframes <= 0)
        return -1;

    err = Pa_WriteStream(h->stream_out, buf, nframes);
    if (err != paNoError && err != paOutputUnderflowed) {
        fprintf(stderr, "audio_write: %s\n", Pa_GetErrorText(err));
        return -1;
    }
    return 0;
}

/** Lit nframes échantillons depuis le microphone. */
int audio_read(audio_handle_t handle, sample_t *buf, int nframes)
{
    struct audio_handle *h = (struct audio_handle *)handle;
    PaError err;
    unsigned long n = (unsigned long)nframes;

    if (!h || !h->opened || !buf || nframes <= 0)
        return -1;

    err = Pa_ReadStream(h->stream_in, buf, n);
    if (err != paNoError && err != paInputOverflowed) {
        fprintf(stderr, "audio_read: %s\n", Pa_GetErrorText(err));
        return -1;
    }
    return (int)n;
}

/** Ferme les périphériques audio et libère les ressources. */
void audio_cleanup(audio_handle_t handle)
{
    struct audio_handle *h = (struct audio_handle *)handle;

    if (!h) return;

    if (h->opened) {
        if (h->stream_in)  { Pa_StopStream(h->stream_in);  Pa_CloseStream(h->stream_in);  h->stream_in = NULL; }
        if (h->stream_out) { Pa_StopStream(h->stream_out); Pa_CloseStream(h->stream_out); h->stream_out = NULL; }
        h->opened = 0;
    }
    free(h);
    Pa_Terminate();
}
