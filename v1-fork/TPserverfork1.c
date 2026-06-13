#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <ctype.h>
#include <signal.h>

void convert_to_uppercase(char* str) {
    int i = 0;
    while (str[i] != '\0') {
        if (islower(str[i])) {
            str[i] = toupper(str[i]);
        }
        i++;
    }
}

// Genere un ID unique en le persistant dans le fichier plan.txt.
// Necessaire car avec fork chaque enfant a sa propre copie des variables.
int next_id() {
    int id = 2023;
    FILE *f = fopen("ids.txt", "r");
    if (f) { fscanf(f, "%d", &id); fclose(f); }
    id++;
    f = fopen("ids.txt", "w");
    if (f) { fprintf(f, "%d", id); fclose(f); }
    return id;
}

int main(int argc, char **argv) {
    int server_desc, client_desc;
    struct sockaddr_in server_addr, client_addr;
    socklen_t client_len;
    char buffer[1024];
    int n;

    if (argc < 2) {
        fprintf(stderr, "Usage: %s <port>\n", argv[0]);
        return 1;
    }

    // Recuperation automatique des processus enfants -> pas de zombies
    signal(SIGCHLD, SIG_IGN);

    server_desc = socket(AF_INET, SOCK_STREAM, 0);
    if (server_desc < 0) { perror("socket"); return 1; }
    printf("[+]Creation du socket serveur.\n");

    memset(&server_addr, '\0', sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(atoi(argv[1]));
    server_addr.sin_addr.s_addr = htonl(INADDR_ANY);

    if (bind(server_desc, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        perror("bind"); return 1;
    }
    printf("[+]Bind to port %s\n", argv[1]);

    if (listen(server_desc, 5) < 0) { perror("listen"); return 1; }
    printf("[+]En attente de connexion...\n");

    while(1) {
        client_len = sizeof(client_addr);
        client_desc = accept(server_desc, (struct sockaddr*)&client_addr, &client_len);
        if (client_desc < 0) { perror("accept"); continue; }
        printf("[+]Connexion de %s:%d acceptee.\n",
               inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port));

        if (fork() == 0) {
            close(server_desc);
            while(1) {
                n = recv(client_desc, buffer, sizeof(buffer) - 1, 0);
                if (n <= 0) {            // 0 = client deconnecte, <0 = erreur
                    printf("[-]Client deconnecte.\n");
                    break;
                }
                buffer[n] = '\0';
                printf("[Message recu] : %s\n", buffer);

                if (strcmp(buffer, "INS") == 0) {
                    printf("Demande d'inscription de %s:%d\n",
                           inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port));

                    // 1) on demande le pseudo
                    strcpy(buffer, "Entrer votre pseudo");
                    send(client_desc, buffer, strlen(buffer) + 1, 0);

                    // 2) on attend et on LIT le pseudo envoye par le client
                    n = recv(client_desc, buffer, sizeof(buffer) - 1, 0);
                    if (n <= 0) break;
                    buffer[n] = '\0';
                    printf("Pseudo recu : %s\n", buffer);

                    // 3) on genere un vrai ID et on le renvoie
                    sprintf(buffer, "%d", next_id());
                    send(client_desc, buffer, strlen(buffer) + 1, 0);
                }
            }
            close(client_desc);
            exit(0);
        }
        close(client_desc);
    }
    return 0;
}
