#include <arpa/inet.h>
#include <netdb.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#define MAXBUF 1024
#define MAX_CLIENTS 128

struct ChatMessage {
    char pseudo[64];
    char content[MAXBUF];
};

// ip: 127.0.0.1 car localhost
int port = 31640;

int main(int argc, char* argv[]) {

    int MaSocket = socket(PF_INET, SOCK_DGRAM, 0);

    if (MaSocket == -1) {
        perror("Client : pb creation socket , on arrête tout:");
        exit(1);
    }

    // printf("Serveur : Youpi j'ai créé une socket pour le serveur\n");

    struct sockaddr_in serveur_addr;
    serveur_addr.sin_family = AF_INET;
    serveur_addr.sin_port = htons(port);
    serveur_addr.sin_addr.s_addr = INADDR_ANY;

    if (bind(MaSocket, (struct sockaddr*)&serveur_addr, sizeof(serveur_addr)) < 0) {
        perror("Serveur : erreur lors du bind");
        close(MaSocket);
        exit(1);
    }
    printf("Serveur listening on port %d\n\n", port);

    struct sockaddr_in clients[MAX_CLIENTS];
    int client_count = 0;

    while (1) {
        struct ChatMessage message_received;
        struct sockaddr_in client_addr;

        socklen_t addr_len = sizeof(client_addr);
        int retourRecv = recvfrom(MaSocket, &message_received, sizeof(message_received), 0, (struct sockaddr*)&client_addr, &addr_len);
        if (retourRecv < 0) {
            perror("Serveur : erreur à la réception");
            close(MaSocket);
            exit(1);
        }
        // printf("Serveur : Message reçu de %s:%d\n", inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port));
        message_received.pseudo[sizeof(message_received.pseudo) - 1] = '\0';
        message_received.content[sizeof(message_received.content) - 1] = '\0';

        if (strcmp(message_received.content, "ping") == 0) {  // si le message est un ping, on ajoute le client à la liste des clients connus s'il n'y est pas déjà
            struct ChatMessage pong_message;
            strncpy(pong_message.pseudo, "system", sizeof(pong_message.pseudo) - 1);
            strncpy(pong_message.content, "pong", sizeof(pong_message.content) - 1);

            if (strcmp(message_received.pseudo, "system") == 0) {
                strncpy(pong_message.content, "unauthorized_name", sizeof(pong_message.content) - 1);
                printf("Serveur : Un client a tenté de se connecter avec le pseudo 'system', ce qui est interdit. Envoi d'un message d'erreur.\n");
            }

            int known_client = 0;
            for (int i = 0; i < client_count; i++) {
                if (clients[i].sin_addr.s_addr == client_addr.sin_addr.s_addr && clients[i].sin_port == client_addr.sin_port) {
                    known_client = 1;
                    break;
                }
            }
            if (!known_client && client_count < MAX_CLIENTS) {
                clients[client_count++] = client_addr;
            }
            if (sendto(MaSocket, &pong_message, sizeof(pong_message), 0, (struct sockaddr*)&client_addr, sizeof(client_addr)) < 0) {
                perror("Serveur : erreur à l'envoi du pong");
            }
        } else {  // Si le message n'est pas un ping, on le diffuse à tous les clients connus

            for (int i = 0; i < client_count; i++) {
                if (sendto(MaSocket, &message_received, sizeof(message_received), 0, (struct sockaddr*)&clients[i], sizeof(clients[i])) < 0) {
                    perror("Serveur : erreur à l'envoi");
                }
            }
        }
    }
    close(MaSocket);
    return 0;
}