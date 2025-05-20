#include <mpi.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "sha256.h"

#define PREFIX "desafio"

#define TAG_PEDIDO 1
#define TAG_TRABALHO 2
#define TAG_SOLUCAO 3
#define TAG_PARADA 4

int hash_has_n_zero_bits(uint8_t hash[SHA256_BLOCK_SIZE], int n) {
    int full_bytes = n / 8;
    int remaining_bits = n % 8;

    for (int i = 0; i < full_bytes; ++i) {
        if (hash[i] != 0) return 0;
    }
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
    int rank, size;
    MPI_Status status;
    MPI_Init(&argc, &argv);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);

    if (argc < 3) {
        if (rank == 0)
            printf("Uso: mpirun -np <procs> ./hash_mpi <numero_solucoes> <bits_zero>\n");
        MPI_Finalize();
        return 1;
    }

    int total_solucoes = atoi(argv[1]);
    int target_zero_bits = atoi(argv[2]);

    if (rank == 0) {
        // MESTRE
        unsigned long next_start = 0;
        int stop_signal = 1;
        int found_count = 0;

        // Array para armazenar soluções (apenas para imprimir)
        char **solucoes = malloc(total_solucoes * sizeof(char *));
        for (int i = 0; i < total_solucoes; i++)
            solucoes[i] = malloc(1024);

        while (found_count < total_solucoes) {
            // Espera pedidos e soluções
            MPI_Probe(MPI_ANY_SOURCE, MPI_ANY_TAG, MPI_COMM_WORLD, &status);
            int source = status.MPI_SOURCE;
            int tag = status.MPI_TAG;

            if (tag == TAG_PEDIDO) {
                // Recebe pedido com tamanho do step
                unsigned long requested_step;
                MPI_Recv(&requested_step, 1, MPI_UNSIGNED_LONG, source, TAG_PEDIDO, MPI_COMM_WORLD, &status);

                // Envia intervalo
                MPI_Send(&next_start, 1, MPI_UNSIGNED_LONG, source, TAG_TRABALHO, MPI_COMM_WORLD);
                next_start += requested_step;
            } else if (tag == TAG_SOLUCAO) {
                // Recebe solução encontrada
                MPI_Recv(solucoes[found_count], 1024, MPI_CHAR, source, TAG_SOLUCAO, MPI_COMM_WORLD, &status);
                printf("[MESTRE] Solução %d recebida do processo %d: %s\n", found_count+1, source, solucoes[found_count]);
                found_count++;
            }
        }

        // Quando alcançou o total de soluções, envia parada para todos
        for (int i = 1; i < size; ++i) {
            MPI_Send(&stop_signal, 1, MPI_INT, i, TAG_PARADA, MPI_COMM_WORLD);
        }

        printf("[MESTRE] Encontradas todas as %d soluções.\n", total_solucoes);

        // Libera memória
        for (int i = 0; i < total_solucoes; i++) free(solucoes[i]);
        free(solucoes);

    } else {
        // TRABALHADORES
        int stop_flag = 0;
        while (!stop_flag) {
            unsigned long my_step = 10000; // ou qualquer lógica para step

            // Pede trabalho
            MPI_Send(&my_step, 1, MPI_UNSIGNED_LONG, 0, TAG_PEDIDO, MPI_COMM_WORLD);

            // Recebe intervalo para testar
            unsigned long start;
            MPI_Recv(&start, 1, MPI_UNSIGNED_LONG, 0, TAG_TRABALHO, MPI_COMM_WORLD, &status);

            char buffer[1024];
            uint8_t hash[SHA256_BLOCK_SIZE];

            for (unsigned long i = start; i < start + my_step; ++i) {
                snprintf(buffer, sizeof(buffer), "%s%lu", PREFIX, i);
                compute_sha256(buffer, hash);

                if (hash_has_n_zero_bits(hash, target_zero_bits)) {
                    printf("[Worker %d] Encontrado: %s\n", rank, buffer);
                    MPI_Send(buffer, strlen(buffer)+1, MPI_CHAR, 0, TAG_SOLUCAO, MPI_COMM_WORLD);
                    // Não para: continua buscando até mestre mandar parar
                }
            }

            // Verifica sinal de parada sem bloquear
            int flag;
            MPI_Iprobe(0, TAG_PARADA, MPI_COMM_WORLD, &flag, &status);
            if (flag) {
                int dummy;
                MPI_Recv(&dummy, 1, MPI_INT, 0, TAG_PARADA, MPI_COMM_WORLD, &status);
                stop_flag = 1;
            }
        }
    }

    MPI_Finalize();
    return 0;
}
