/* servidor.c — Chat multiusuário TCP com threads POSIX. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <pthread.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#define PORTA        9999
#define MAX_CLIENTES 10
#define TAM_BUFFER   1024
#define TAM_NOME     32

typedef struct {
    int    socket;
    struct sockaddr_in endereco;
    char   nome[TAM_NOME];
    int    ativo;
} Cliente;

static Cliente         clientes[MAX_CLIENTES];
static int             total_clientes = 0;
static pthread_mutex_t mutex_clientes = PTHREAD_MUTEX_INITIALIZER;

static void *atender_cliente(void *arg);
static void  broadcast(const char *mensagem, int socket_remetente);
static int   adicionar_cliente(int sock, struct sockaddr_in end);
static void  remover_cliente(int sock);

int main(void) {
    int socket_servidor;
    struct sockaddr_in endereco_servidor, endereco_cliente;
    socklen_t tam_endereco;
    int opcao = 1;

    socket_servidor = socket(AF_INET, SOCK_STREAM, 0);
    if (socket_servidor < 0) { perror("socket"); exit(EXIT_FAILURE); }

    if (setsockopt(socket_servidor, SOL_SOCKET, SO_REUSEADDR,
                   &opcao, sizeof(opcao)) < 0) {
        perror("setsockopt"); exit(EXIT_FAILURE);
    }

    memset(&endereco_servidor, 0, sizeof(endereco_servidor));
    endereco_servidor.sin_family      = AF_INET;
    endereco_servidor.sin_addr.s_addr = INADDR_ANY;
    endereco_servidor.sin_port        = htons(PORTA);

    if (bind(socket_servidor, (struct sockaddr *)&endereco_servidor,
             sizeof(endereco_servidor)) < 0) {
        perror("bind"); exit(EXIT_FAILURE);
    }

    if (listen(socket_servidor, 10) < 0) { perror("listen"); exit(EXIT_FAILURE); }

    printf("[SERVIDOR] Escutando na porta %d...\n", PORTA);
    fflush(stdout);

    while (1) {
        tam_endereco = sizeof(endereco_cliente);
        int socket_cliente = accept(socket_servidor,
                                    (struct sockaddr *)&endereco_cliente,
                                    &tam_endereco);
        if (socket_cliente < 0) { perror("accept"); continue; }

        int indice = adicionar_cliente(socket_cliente, endereco_cliente);
        if (indice < 0) {
            const char *msg = "Servidor cheio. Tente novamente mais tarde.\n";
            send(socket_cliente, msg, strlen(msg), 0);
            close(socket_cliente);
            continue;
        }

        pthread_t tid;
        if (pthread_create(&tid, NULL, atender_cliente,
                           (void *)(intptr_t)indice) != 0) {
            perror("pthread_create");
            remover_cliente(socket_cliente);
            close(socket_cliente);
            continue;
        }
        pthread_detach(tid);
    }
    close(socket_servidor);
    return 0;
}

static void *atender_cliente(void *arg) {
    int indice = (int)(intptr_t)arg;
    int sock    = clientes[indice].socket;
    char buffer[TAM_BUFFER];
    char mensagem[TAM_BUFFER + TAM_NOME + 16];

    const char *pedido = "Digite seu nome: ";
    send(sock, pedido, strlen(pedido), 0);

    memset(buffer, 0, sizeof(buffer));
    ssize_t n = recv(sock, buffer, sizeof(buffer) - 1, 0);
    if (n <= 0) { remover_cliente(sock); close(sock); return NULL; }

    buffer[strcspn(buffer, "\r\n")] = '\0';
    if (strlen(buffer) == 0) strncpy(buffer, "Anonimo", TAM_NOME - 1);

    pthread_mutex_lock(&mutex_clientes);
    strncpy(clientes[indice].nome, buffer, TAM_NOME - 1);
    clientes[indice].nome[TAM_NOME - 1] = '\0';
    char nome_local[TAM_NOME];
    strncpy(nome_local, clientes[indice].nome, TAM_NOME);
    pthread_mutex_unlock(&mutex_clientes);

    snprintf(mensagem, sizeof(mensagem),
             "[SERVIDOR] %s entrou no chat.\n", nome_local);
    broadcast(mensagem, sock);

    while (1) {
        memset(buffer, 0, sizeof(buffer));
        n = recv(sock, buffer, sizeof(buffer) - 1, 0);
        if (n == 0) break;
        if (n < 0) { perror("recv"); break; }

        buffer[n] = '\0';
        buffer[strcspn(buffer, "\r\n")] = '\0';

        if (strcmp(buffer, "sair") == 0) break;

        snprintf(mensagem, sizeof(mensagem), "%s: %s\n", nome_local, buffer);
        broadcast(mensagem, sock);
    }

    snprintf(mensagem, sizeof(mensagem),
             "[SERVIDOR] %s saiu do chat.\n", nome_local);
    broadcast(mensagem, sock);

    remover_cliente(sock);
    close(sock);
    return NULL;
}

static void broadcast(const char *mensagem, int socket_remetente) {
    pthread_mutex_lock(&mutex_clientes);
    for (int i = 0; i < MAX_CLIENTES; i++) {
        if (clientes[i].ativo && clientes[i].socket != socket_remetente) {
            send(clientes[i].socket, mensagem, strlen(mensagem), 0);
        }
    }
    pthread_mutex_unlock(&mutex_clientes);
}

static int adicionar_cliente(int sock, struct sockaddr_in end) {
    pthread_mutex_lock(&mutex_clientes);
    if (total_clientes >= MAX_CLIENTES) {
        pthread_mutex_unlock(&mutex_clientes);
        return -1;
    }
    for (int i = 0; i < MAX_CLIENTES; i++) {
        if (!clientes[i].ativo) {
            clientes[i].socket   = sock;
            clientes[i].endereco = end;
            clientes[i].ativo    = 1;
            clientes[i].nome[0]  = '\0';
            total_clientes++;
            pthread_mutex_unlock(&mutex_clientes);
            printf("[SERVIDOR] Novo cliente no slot %d (fd=%d)\n", i, sock);
            fflush(stdout);
            return i;
        }
    }
    pthread_mutex_unlock(&mutex_clientes);
    return -1;
}

static void remover_cliente(int sock) {
    pthread_mutex_lock(&mutex_clientes);
    for (int i = 0; i < MAX_CLIENTES; i++) {
        if (clientes[i].ativo && clientes[i].socket == sock) {
            clientes[i].ativo = 0;
            clientes[i].socket = -1;
            clientes[i].nome[0] = '\0';
            total_clientes--;
            printf("[SERVIDOR] Cliente no slot %d desconectado.\n", i);
            fflush(stdout);
            break;
        }
    }
    pthread_mutex_unlock(&mutex_clientes);
}
