#include <string.h>
#include <math.h>
#include <stdio.h>
#include "fft.h"

#define SAMPLE_RATE 16000 // adjust according to your WAV file
#define LOW_FREQ_CUTOFF 300
#define HIGH_FREQ_CUTOFF 10000
// Bit-reversal permutation
static void bit_reverse(complex *x, int N) {
    int j = 0;
    for (int i = 0; i < N; i++) {
        if (i < j) {
            complex tmp = x[i];
            x[i] = x[j];
            x[j] = tmp;
        }
        int m = N >> 1;
        while (m >= 1 && j >= m) {
            j -= m;
            m >>= 1;
        }
        j += m;
    }
}
// FFT function
// input: real array of length N (power of 2)
// output: complex array of length N
void fft(float *input, complex *output, int N) {
    // Copy input to complex array
    for (int i = 0; i < N; i++) {
        output[i].r = input[i];
        output[i].i = 0.0f;
    }

    bit_reverse(output, N);

    for (int s = 1; (1 << s) <= N; s++) {
        int m = 1 << s;
        float angle = -2.0f * M_PI / m;
        complex wm = {cosf(angle), sinf(angle)};
        for (int k = 0; k < N; k += m) {
            complex w = {1.0f, 0.0f};
            for (int j = 0; j < m/2; j++) {
                complex t, u;
                t.r = w.r * output[k + j + m/2].r - w.i * output[k + j + m/2].i;
                t.i = w.r * output[k + j + m/2].i + w.i * output[k + j + m/2].r;

                u = output[k + j];

                output[k + j].r = u.r + t.r;
                output[k + j].i = u.i + t.i;

                output[k + j + m/2].r = u.r - t.r;
                output[k + j + m/2].i = u.i - t.i;

                // w = w * wm
                float tmp = w.r;
                w.r = w.r * wm.r - w.i * wm.i;
                w.i = tmp * wm.i + w.i * wm.r;
            }
        }
    }
}
// Computes the inverse DFT (IFFT) of input array `in` of length `N`
// and stores the result in `out` as real values (assuming original signal was real)
void ifft(complex *in, float *out, int N) {
    for (int n = 0; n < N; n++) {
        float sum_real = 0.0f;
        float sum_imag = 0.0f,cos_a, sin_a;
        for (int k = 0; k < N; k++) {
            float angle = 2.0f * M_PI * k * n / N;
            cos_a = cosf(angle);
            sin_a = sinf(angle);
            sum_real += in[k].r * cos_a - in[k].i * sin_a;
            sum_imag += in[k].r * sin_a + in[k].i * cos_a;
        }
        // Normalize by N
        out[n] = sum_real / N; // discard small imaginary part if input was from real FFT
    }
}
void fft_process(int16_t *samples, int16_t *output)
{
    float real[FFT_SIZE];

    // Convert input PCM to float
    for (int i = 0; i < FFT_SIZE; i++) {
        real[i] = (float)samples[i];
    }
    complex temp[FFT_SIZE];
    // Perform FFT
    fft(real, temp, FFT_SIZE);
    // Determine bin indices for 300Hz and 10kHz
    float freq_res = 44000.0f / FFT_SIZE; // Frequency resolution
    int k_min = (int)((float)LOW_FREQ_CUTOFF / freq_res);
    int k_max = (int)((float)HIGH_FREQ_CUTOFF / freq_res);
    // Suppress low and high frequencies
    for (int i = 0; i < FFT_SIZE; i++) {
        if (i < k_min || i > k_max) {
            temp[i].r = 0;
            temp[i].i = 0;
        }
    }

    // Perform IFFT
       
    ifft(temp, real, FFT_SIZE);
    // Convert back to int16_t
    for (int i = 0; i < FFT_SIZE; i++) {
        output[i] = (int16_t)real[i];
    }
}
