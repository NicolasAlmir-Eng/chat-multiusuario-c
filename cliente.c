/* cliente.c — Cliente de chat TCP. Uso: ./cliente [host] */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#define PORTA      9999
#define TAM_BUFFER 1024

static int socket_servidor = -1;
static void *thread_receber(void *arg);

int main(int argc, char *argv[]) {
    const char *host = (argc > 1) ? argv[1] : "127.0.0.1";
    struct sockaddr_in endereco_servidor;
    char buffer[TAM_BUFFER];

    socket_servidor = socket(AF_INET, SOCK_STREAM, 0);
    if (socket_servidor < 0) { perror("socket"); exit(EXIT_FAILURE); }

    memset(&endereco_servidor, 0, sizeof(endereco_servidor));
    endereco_servidor.sin_family = AF_INET;
    endereco_servidor.sin_port   = htons(PORTA);

    if (inet_pton(AF_INET, host, &endereco_servidor.sin_addr) <= 0) {
        perror("inet_pton"); close(socket_servidor); exit(EXIT_FAILURE);
    }

    if (connect(socket_servidor,
                (struct sockaddr *)&endereco_servidor,
                sizeof(endereco_servidor)) < 0) {
        perror("connect"); close(socket_servidor); exit(EXIT_FAILURE);
    }

    printf("[CLIENTE] Conectado a %s:%d\n", host, PORTA);
    fflush(stdout);

    pthread_t tid;
    if (pthread_create(&tid, NULL, thread_receber, NULL) != 0) {
        perror("pthread_create"); close(socket_servidor); exit(EXIT_FAILURE);
    }
    pthread_detach(tid);

    while (1) {
        if (fgets(buffer, sizeof(buffer), stdin) == NULL) break;

        if (strncmp(buffer, "sair", 4) == 0 &&
            (buffer[4] == '\n' || buffer[4] == '\0')) {
            send(socket_servidor, "sair\n", 5, 0);
            break;
        }
        if (send(socket_servidor, buffer, strlen(buffer), 0) < 0) {
            perror("send"); break;
        }
    }

    close(socket_servidor);
    printf("[CLIENTE] Desconectado.\n");
    return 0;
}

static void *thread_receber(void *arg) {
    (void)arg;
    char buffer[TAM_BUFFER];
    ssize_t n;

    while (1) {
        memset(buffer, 0, sizeof(buffer));
        n = recv(socket_servidor, buffer, sizeof(buffer) - 1, 0);
        if (n > 0) {
            buffer[n] = '\0';
            fputs(buffer, stdout);
            fflush(stdout);
        } else if (n == 0) {
            printf("\n[CLIENTE] Servidor encerrou a conexão.\n");
            break;
        } else {
            perror("recv");
            break;
        }
    }
    exit(EXIT_SUCCESS);
    return NULL;
}
