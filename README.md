
Chat Multiusuário TCP/IP em C

Este projeto é um servidor e cliente de chat multiusuário operando via terminal. Ele foi construído inteiramente na linguagem C puro, utilizando a API de Sockets POSIX e a biblioteca pthreads para garantir concorrência e processamento paralelo de clientes.

Tecnologias e Arquitetura

Linguagem: C

Comunicação: Protocolo TCP/IP (IPv4).

Concorrência: Threads POSIX (pthread_create, pthread_detach).

Sincronização: Mutex (pthread_mutex_t) para controle seguro do array de clientes e broadcast de mensagens.

Infraestrutura: Suporte a Dockerização (docker-compose).

Funcionalidades

Conexão simultânea de até 10 clientes.

Sistema de Broadcast: uma mensagem enviada por um cliente é retransmitida a todos os outros ativos.

Definição de usernames ao conectar.

Notificações globais de entrada e saída de usuários no servidor.

Tratamento contra esgotamento de conexões (rejeita conexões quando o servidor atinge a capacidade máxima).

Como Executar

Usando Docker (Recomendado)

Sobe o servidor e dois clientes prontos para uso:

docker compose up --build


Para interagir com os clientes, abra novos terminais e faça o attach:

docker attach chat_cliente1
docker attach chat_cliente2


Localmente (Compilação Nativa via Makefile)

No terminal 1 (Servidor):

make
./servidor


Em outros terminais (Clientes):

./cliente 127.0.0.1


Para sair do chat, digite sair.
