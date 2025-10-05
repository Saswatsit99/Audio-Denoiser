#ifndef FFT_H
#define FFT_H

#include <stdint.h>
#include <stddef.h>

#define FFT_SIZE 256
#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    float r; // real part
    float i; // imaginary part
} complex;

// Bit-reversal permutation
static void bit_reverse(complex *x, int N);
void fft(float *input, complex *output, int N);
void ifft(complex *in, float *out, int N)
/**
 * @brief Perform FFT on 512-byte WAV buffer and suppress high/low frequencies.
 * 
 * @param wav_buffer Pointer to 512-byte buffer (PCM16LE samples)
 * @param fft_out Pointer to buffer of size FFT_SIZE for storing FFT magnitude
 */
void fft_process(int16_t *samples, int16_t *output);

#ifdef __cplusplus
}
#endif

#endif // FFT_H