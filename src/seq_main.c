#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <time.h>
#include "sha256.h"

#define PREFIX        "desafio"
#define MAX_MSG_LEN   256

void hash_to_hex(uint8_t hash[SHA256_BLOCK_SIZE], char hex_output[SHA256_BLOCK_SIZE * 2 + 1]) {
    for (int i = 0; i < SHA256_BLOCK_SIZE; i++)
        sprintf(hex_output + (i * 2), "%02x", hash[i]);
    hex_output[SHA256_BLOCK_SIZE * 2] = '\0';
}

int hash_has_n_zero_bits(uint8_t hash[SHA256_BLOCK_SIZE], int n) {
    int full_bytes     = n / 8;
    int remaining_bits = n % 8;
    for (int i = 0; i < full_bytes; ++i)
        if (hash[i] != 0) return 0;
    if (remaining_bits > 0) {
        uint8_t mask = 0xFF << (8 - remaining_bits);
        if ((hash[full_bytes] & mask) != 0) return 0;
    }
    return 1;
}

void compute_sha256(const char *input, uint8_t output[SHA256_BLOCK_SIZE]) {
    SHA256_CTX ctx;
    sha256_init(&ctx);
    sha256_update(&ctx, (const uint8_t *)input, strlen(input));
    sha256_final(&ctx, output);
}

int main(int argc, char **argv) {
    if (argc < 3) {
        fprintf(stderr, "Uso: %s <numero_solucoes> <bits_zero>\n", argv[0]);
        return EXIT_FAILURE;
    }

    int  total_solucoes   = atoi(argv[1]);
    int  target_zero_bits = atoi(argv[2]);
    unsigned long long found_count = 0;
    unsigned long long idx = 0;

    // Vetores para armazenar soluções e índices
    char **sols = malloc(total_solucoes * sizeof *sols);
    unsigned long long *indices = malloc(total_solucoes * sizeof *indices);
    for (int i = 0; i < total_solucoes; i++)
        sols[i] = malloc(MAX_MSG_LEN);

    // Marca o início do tempo
    struct timespec t_start, t_end;
    clock_gettime(CLOCK_MONOTONIC, &t_start);

    // Loop até achar total_solucoes
    while (found_count < (unsigned long long)total_solucoes) {
        char txt[1024];
        uint8_t hash[SHA256_BLOCK_SIZE];

        // Gera a string “desafio<idx>”
        snprintf(txt, sizeof(txt), PREFIX "%llu", idx);
        compute_sha256(txt, hash);

        // Verifica se tem n bits zero
        if (hash_has_n_zero_bits(hash, target_zero_bits)) {
            strncpy(sols[found_count], txt, MAX_MSG_LEN-1);
            sols[found_count][MAX_MSG_LEN-1] = '\0';
            indices[found_count] = idx;
            found_count++;
        }
        idx++;
    }

    // Marca o fim e calcula elapsed
    clock_gettime(CLOCK_MONOTONIC, &t_end);
    double elapsed = (t_end.tv_sec  - t_start.tv_sec)
                   + (t_end.tv_nsec - t_start.tv_nsec) / 1e9;

    // Imprime as soluções encontradas
    printf("Soluções encontradas:\n");
    for (int i = 0; i < total_solucoes; i++) {
        printf("%2d) %s  (idx=%llu)\n",
               i+1, sols[i], indices[i]);
    }
    printf("\nTempo total: %.6f s\n", elapsed);

    // Libera memória
    for (int i = 0; i < total_solucoes; i++)
        free(sols[i]);
    free(sols);
    free(indices);

    return EXIT_SUCCESS;
}
