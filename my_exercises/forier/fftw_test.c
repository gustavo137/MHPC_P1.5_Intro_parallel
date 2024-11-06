#include <fftw3.h>
#include <stdio.h>

int main() {
    int N = 8;  // Tamaño de la señal
    fftw_complex in[N], out[N];  // Arreglos de entrada y salida
    fftw_plan plan;

    // Inicializar la señal de entrada con valores de ejemplo
    for (int i = 0; i < N; i++) {
        in[i][0] = i + 1;  // Parte real
        in[i][1] = 0.0;    // Parte imaginaria
    }

    // Crear el plan de FFT
    plan = fftw_plan_dft_1d(N, in, out, FFTW_FORWARD, FFTW_ESTIMATE);

    // Ejecutar la FFT
    fftw_execute(plan);

    // Imprimir los resultados
    printf("Resultado de la FFT:\n");
    for (int i = 0; i < N; i++) {
        printf("out[%d] = %f + %fi\n", i, out[i][0], out[i][1]);
    }

    // Liberar recursos
    fftw_destroy_plan(plan);
    fftw_cleanup();

    return 0;
}

