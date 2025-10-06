#include <string.h>
#include <math.h>
#include <stdio.h>
#include "fft.h"

#define SAMPLE_RATE 16000 // adjust according to your WAV file
#define LOW_FREQ_CUTOFF 300
#define HIGH_FREQ_CUTOFF 10000

#define TABLE_SIZE 360
#define TWO_PI (2.0f * M_PI)
#define HALF_PI (0.5f * M_PI)
#define INV_TWO_PI 0.1591549430918f

// --- Precomputed cosine lookup table for 0° to 359° (1° = 2π/360 radians) ---
static const float cosine_table[TABLE_SIZE] = {
    1.000000f, 0.999848f, 0.999391f, 0.998630f, 0.997564f, 0.996195f, 0.994522f, 0.992546f, 0.990268f, 0.987688f,
    0.984808f, 0.981627f, 0.978148f, 0.974370f, 0.970296f, 0.965926f, 0.961262f, 0.956305f, 0.951056f, 0.945519f,
    0.939693f, 0.933580f, 0.927184f, 0.920505f, 0.913545f, 0.906308f, 0.898794f, 0.891007f, 0.882948f, 0.874620f,
    0.866025f, 0.857167f, 0.848048f, 0.838671f, 0.829038f, 0.819152f, 0.809017f, 0.798636f, 0.788011f, 0.777146f,
    0.766044f, 0.754710f, 0.743145f, 0.731354f, 0.719340f, 0.707107f, 0.694658f, 0.681998f, 0.669131f, 0.656059f,
    0.642788f, 0.629320f, 0.615661f, 0.601815f, 0.587785f, 0.573576f, 0.559193f, 0.544639f, 0.529919f, 0.515038f,
    0.500000f, 0.484810f, 0.469472f, 0.453990f, 0.438371f, 0.422618f, 0.406737f, 0.390731f, 0.374607f, 0.358368f,
    0.342020f, 0.325568f, 0.309017f, 0.292372f, 0.275637f, 0.258819f, 0.241922f, 0.224951f, 0.207912f, 0.190809f,
    0.173648f, 0.156434f, 0.139173f, 0.121869f, 0.104528f, 0.087156f, 0.069756f, 0.052336f, 0.034899f, 0.017452f,
    0.000000f, -0.017452f, -0.034899f, -0.052336f, -0.069756f, -0.087156f, -0.104528f, -0.121869f, -0.139173f, -0.156434f,
    -0.173648f, -0.190809f, -0.207912f, -0.224951f, -0.241922f, -0.258819f, -0.275637f, -0.292372f, -0.309017f, -0.325568f,
    -0.342020f, -0.358368f, -0.374607f, -0.390731f, -0.406737f, -0.422618f, -0.438371f, -0.453990f, -0.469472f, -0.484810f,
    -0.500000f, -0.515038f, -0.529919f, -0.544639f, -0.559193f, -0.573576f, -0.587785f, -0.601815f, -0.615661f, -0.629320f,
    -0.642788f, -0.656059f, -0.669131f, -0.681998f, -0.694658f, -0.707107f, -0.719340f, -0.731354f, -0.743145f, -0.754710f,
    -0.766044f, -0.777146f, -0.788011f, -0.798636f, -0.809017f, -0.819152f, -0.829038f, -0.838671f, -0.848048f, -0.857167f,
    -0.866025f, -0.874620f, -0.882948f, -0.891007f, -0.898794f, -0.906308f, -0.913545f, -0.920505f, -0.927184f, -0.933580f,
    -0.939693f, -0.945519f, -0.951056f, -0.956305f, -0.961262f, -0.965926f, -0.970296f, -0.974370f, -0.978148f, -0.981627f,
    -0.984808f, -0.987688f, -0.990268f, -0.992546f, -0.994522f, -0.996195f, -0.997564f, -0.998630f, -0.999391f, -0.999848f,
    -1.000000f, -0.999848f, -0.999391f, -0.998630f, -0.997564f, -0.996195f, -0.994522f, -0.992546f, -0.990268f, -0.987688f,
    -0.984808f, -0.981627f, -0.978148f, -0.974370f, -0.970296f, -0.965926f, -0.961262f, -0.956305f, -0.951056f, -0.945519f,
    -0.939693f, -0.933580f, -0.927184f, -0.920505f, -0.913545f, -0.906308f, -0.898794f, -0.891007f, -0.882948f, -0.874620f,
    -0.866025f, -0.857167f, -0.848048f, -0.838671f, -0.829038f, -0.819152f, -0.809017f, -0.798636f, -0.788011f, -0.777146f,
    -0.766044f, -0.754710f, -0.743145f, -0.731354f, -0.719340f, -0.707107f, -0.694658f, -0.681998f, -0.669131f, -0.656059f,
    -0.642788f, -0.629320f, -0.615661f, -0.601815f, -0.587785f, -0.573576f, -0.559193f, -0.544639f, -0.529919f, -0.515038f,
    -0.500000f, -0.484810f, -0.469472f, -0.453990f, -0.438371f, -0.422618f, -0.406737f, -0.390731f, -0.374607f, -0.358368f,
    -0.342020f, -0.325568f, -0.309017f, -0.292372f, -0.275637f, -0.258819f, -0.241922f, -0.224951f, -0.207912f, -0.190809f,
    -0.173648f, -0.156434f, -0.139173f, -0.121869f, -0.104528f, -0.087156f, -0.069756f, -0.052336f, -0.034899f, -0.017452f
};

// --- Fast cosine approximation using lookup + interpolation ---
float cos_a(float angle) {
    // Wrap angle to [0, 2π)
    angle = fabs(angle);
    if (angle >= TWO_PI)
        angle -= TWO_PI * (int)(angle * INV_TWO_PI);

    // Convert radians to fractional index
    float index_f = (angle * INV_TWO_PI) * TABLE_SIZE;
    int index = (int)index_f;
    float frac = index_f - index;

    // Next index (circular)
    int next_index = (index + 1) ;
    if(index == TABLE_SIZE-1)
    next_index = 0;

    // Linear interpolation
    float approx = cosine_table[index] * (1.0f - frac) + cosine_table[next_index] * frac;

    return approx;
}

// --- Fast sine using cosine lookup ---
float sin_a(float angle) {
    // sin(θ) = cos(π/2 - θ)
    float sign = 1.0f;
    if(angle < 0.00) 
    {
        sign = -1.0f;
        angle *= sign;
    }
    float shifted = HALF_PI - angle;

    if (shifted < 0)
        shifted += TWO_PI;
    else if (shifted >= TWO_PI)
        shifted -= TWO_PI * (int)(shifted * INV_TWO_PI);

    return sign * cos_a(shifted);
}
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
        float angle = -1.0f * TWO_PI/ m;
        complex wm = {cos_a(angle), sin_a(angle)};
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
        float sum_imag = 0.0f,cos_x, sin_x;
        for (int k = 0; k < N; k++) {
            float angle = TWO_PI * k * n / N;
            cos_x = cos_a(angle);
            sin_x = sin_a(angle);
            sum_real += in[k].r * cos_x - in[k].i * sin_x;
            sum_imag += in[k].r * sin_x + in[k].i * cos_x;
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

