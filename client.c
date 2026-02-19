#include <arpa/inet.h>
#include <netdb.h>
#include <pthread.h>
#include <readline/history.h>
#include <readline/readline.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#define MAXBUF 1024

struct ChatMessage {
    char pseudo[64];
    char content[MAXBUF];
};

void print_message(const char* msg) {
    rl_save_prompt();        // Sauvegarde l'état courant
    rl_replace_line("", 0);  // Efface la ligne en cours
    rl_redisplay();

    printf("\r%s\n", msg);  // Affiche le message

    rl_restore_prompt();  // Restaure le prompt
    rl_redisplay();       // Réaffiche ce que l'utilisateur tapait
}

void* reading(void* arg) {
    int MaSocket = *(int*)arg;
    while (1) {
        struct ChatMessage message_received;
        socklen_t addr_len = sizeof(struct sockaddr_in);
        int retourRecv = recvfrom(MaSocket, &message_received, sizeof(message_received), 0, NULL, &addr_len);
        if (retourRecv < 0) {
            perror("Client : erreur à la réception");
            close(MaSocket);
            exit(1);
        }
        message_received.pseudo[sizeof(message_received.pseudo) - 1] = '\0';
        message_received.content[sizeof(message_received.content) - 1] = '\0';
        char buffer[1024 + 64 + 3];
        snprintf(buffer, sizeof(buffer), "%s : %s", message_received.pseudo, message_received.content);
        print_message(buffer);
    }
    return NULL;
}

int main(int argc, char* argv[]) {
    if (argc != 4) {
        printf("On vous a dit que les paramètres étaient : %s ip_serveur port_serveur et pseudo faites un effort !!!! \n", argv[0]);
        exit(1);
    }

    char* pseudo = argv[3];  // pseudo du client

    printf("Bienvenue dans le chat UDP, %s !\n", pseudo);

    int MaSocket = socket(PF_INET, SOCK_DGRAM, 0);

    if (MaSocket == -1) {
        perror("Client : pb creation socket , on arrête tout:");
        exit(1);
    }

    printf("Client : Youpi j'ai créé une socket pour le client\n");

    struct sockaddr_in client_addr;
    client_addr.sin_family = AF_INET;
    client_addr.sin_addr.s_addr = INADDR_ANY;
    client_addr.sin_port = htons(0);

    struct sockaddr_in serveur_addr;
    serveur_addr.sin_family = AF_INET;
    serveur_addr.sin_port = htons(atoi(argv[2]));
    if (inet_pton(AF_INET, argv[1], &serveur_addr.sin_addr) <= 0) {
        perror("Client : erreur lors de la conversion de l'adresse IP");
        close(MaSocket);
        exit(1);
    }

    pthread_t thread;
    pthread_create(&thread, NULL, reading, &MaSocket);

    while (1) {
        struct ChatMessage message_sent;
        memset(&message_sent, 0, sizeof(message_sent));
        strncpy(message_sent.pseudo, pseudo, sizeof(message_sent.pseudo) - 1);

        char* line = readline("> ");
        if (line == NULL) {
            break;
        }
        strncpy(message_sent.content, line, sizeof(message_sent.content) - 1);
        free(line);
        printf("\033[A\33[2K\r");  // Move cursor up and clear the line

        // printf("Client : message à envoyer : %s", message);
        if (sendto(MaSocket, &message_sent, sizeof(message_sent), 0, (struct sockaddr*)&serveur_addr, sizeof(serveur_addr)) < 0) {
            perror("Client : erreur à l'envoi");
            close(MaSocket);
            exit(1);
        }

        /*
        struct ChatMessage message_received;
        socklen_t addr_len = sizeof(serveur_addr);
        int retourRecv = recvfrom(MaSocket, &message_received, sizeof(message_received), 0, (struct sockaddr*)&serveur_addr, &addr_len);
        if (retourRecv < 0) {
            perror("Client : erreur à la réception");
            close(MaSocket);
            exit(1);
        }
        message_received.pseudo[sizeof(message_received.pseudo) - 1] = '\0';
        message_received.content[sizeof(message_received.content) - 1] = '\0';
        printf("%s : %s", message_received.pseudo, message_received.content);
        */
    }
    close(MaSocket);
    pthread_join(thread, NULL);
    return 0;
}