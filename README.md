# bag-of-work-sha256

Este repositório apresenta uma implementação em C de um gerador de hashes baseado em SHA-256, tanto na versão **sequencial** quanto na versão **paralela** utilizando o modelo coordenador/trabalhador com OpenMPI. O programa busca, a partir de um contador incremental, as primeiras strings “desafio<N>” cujo hash SHA-256 possua um número configurável de bits iniciais zerados.

- **Versão Sequencial**: executa toda a busca em um único processo.
- **Versão Paralela (SPMD)**: o processo 0 atua como coordenador, distribuindo blocos de trabalho dinâmicos a qualquer número de workers; os workers pedem novas tarefas, gerando balanceamento de carga automático.

## Rodar Código Sequencial

Para compilar:

```bash
gcc -O2 -Wall src/seq_main.c src/sha256.c -o <nome_do_arquivo_compilado>
```

Para rodar:

```bash
./<nome_do_arquivo_compilado> <total_solucoes> <bits_zero>
```

## Rodar Código Paralelo

Para compilar:

```bash
mpicc -O2 -Wall src/par_main.c src/sha256.c -o <nome_do_arquivo_compilado>
```

Para rodar:

```bash
mpirun -np 4 ./<nome_do_arquivo_compilado> <total_solucoes> <bits_zero>
```
