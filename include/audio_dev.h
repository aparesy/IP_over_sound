/**
 * audio_dev.h - Interface d’accès à la carte son (basée sur PortAudio).
 *
 * Responsable de l’initialisation de la carte son, de l’écriture d’échantillons
 * vers le haut‑parleur et de la lecture d’échantillons depuis le microphone.
 * La fréquence d’échantillonnage est SAMPLE_RATE (définie dans common.h),
 * avec un type de données sample_t (float).
 */

#ifndef AUDIO_DEV_H
#define AUDIO_DEV_H

#include "common.h"

/** Handle audio ; encapsule les pointeurs de flux PortAudio, reste opaque à l’extérieur. */
typedef void* audio_handle_t;

/**
 * Initialise l’audio : ouvre les périphériques d’entrée et de sortie par défaut
 * en les configurant selon SAMPLE_RATE et AUDIO_FRAMES_PER_BUFFER.
 * @return handle en cas de succès, NULL en cas d’échec
 */
audio_handle_t audio_init(void);  
/** La fonction audio_init est définie dans audio_dev.c et renvoie un handle
 *  de type audio_handle_t pour les opérations audio ultérieures. */

/**
 * Écrit une trame d’échantillons vers le haut‑parleur (lecture audio).
 * @param h       handle retourné par audio_init
 * @param buf     données d’échantillons, de longueur nframes
 * @param nframes nombre d’échantillons dans cette trame
 * @return        0 en cas de succès, non‑zéro en cas d’échec
 */
int audio_write(audio_handle_t h, const sample_t *buf, int nframes);
/** Le paramètre const sample_t *buf représente les données d’échantillons
 *  en entrée, int nframes le nombre d’échantillons de la trame. */

/**
 * Lit une trame d’échantillons depuis le microphone (enregistrement).
 * @param h       handle retourné par audio_init
 * @param buf     tampon de sortie, au moins nframes éléments de type sample_t
 * @param nframes nombre d’échantillons à lire
 * @return        nombre réel d’échantillons lus, ou < 0 en cas d’échec
 */
int audio_read(audio_handle_t h, sample_t *buf, int nframes);

/**
 * Ferme les périphériques audio et libère les ressources associées.
 * @param h handle retourné par audio_init
 */
void audio_cleanup(audio_handle_t h);

#endif /* AUDIO_DEV_H */
