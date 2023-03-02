#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h> 
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <ctype.h>

int ID=2023;

void convert_to_uppercase(char* str) {
    int i = 0;
    while (str[i] != '\0') {
        if (islower(str[i])) {
            str[i] = toupper(str[i]);
        }
        i++;
    }
}

int main(int argc, char **argv) {
    int server_desc, client_desc;
    struct sockaddr_in server_addr, client_addr;
    socklen_t client_len;
    char buffer[1024];
    
    server_desc = socket(AF_INET, SOCK_STREAM, 0);
    printf("[+]Creation du socket serveur.\n");

    memset(&server_addr, '\0', sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(atoi(argv[1]));
    server_addr.sin_addr.s_addr = htons(INADDR_ANY);

    bind(server_desc, (struct sockaddr*)&server_addr, sizeof(server_addr));
    printf("[+]Bind to port %s\n", argv[1]);

    listen(server_desc, 5);
    printf("[+]En attente de connexion...\n");

    while(1) {
        client_len = sizeof(client_addr);
        client_desc = accept(server_desc, (struct sockaddr*)&client_addr, &client_len);
        printf("[+]Connexion de %s:%d accepter.\n", inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port));
       

        if (fork() == 0) {
            close(server_desc);
            while(1) {
                recv(client_desc, buffer, 1024, 0);
                printf("[Message recu de: %s\n", buffer);

                if(strcmp(buffer, "INS") == 0){
					printf(" Demande d'inscription de %s:%d\n", inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port));
					strcpy(buffer,"Entrer votre pseudo");
                    send(client_desc, buffer, strlen(buffer), 0);
                    ID++;
                    strcpy(buffer,"2023");
                    send(client_desc, buffer, strlen(buffer), 0);
                }
                // Envoi des données
               /* convert_to_uppercase(buffer);
                send(client_desc, buffer, strlen(buffer), 0);
                printf("Message envoye: %s\n", buffer);
                */

            }
            close(client_desc);
            exit(0);
        }
        close(client_desc);
    }
    return 0;
}
