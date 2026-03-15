/**
 * wav_writer.h - Écriture d'échantillons float dans un fichier WAV
 * (PCM 16 bits, mono), avec la même fréquence SAMPLE_RATE que définie
 * dans common.h, utilisé par bits_to_wav.
 */

#ifndef WAV_WRITER_H
#define WAV_WRITER_H

/**
 * Écrit une séquence d'échantillons float dans un fichier WAV
 * (mono, PCM 16 bits, 44100 Hz).
 * @param samples  tableau d'échantillons float (environ dans [-1.0, 1.0],
 *                 la modulation de ce projet produit typiquement [-0.3, 0.3])
 * @param nsamples nombre d'échantillons
 * @param path     chemin du fichier de sortie
 * @return         0 en cas de succès, -1 en cas d'échec
 */
int wav_write(const float *samples, int nsamples, const char *path);

#endif /* WAV_WRITER_H */
