#include <stdlib.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <netdb.h>

int client_desc;
struct sockaddr_in addr;
char buffer[1024];

void Inscription(){
    int n;

    // 1) on previent le serveur qu'on veut s'inscrire
    strcpy(buffer, "INS");
    send(client_desc, buffer, strlen(buffer) + 1, 0);

    // 2) Reponse du serveur avec la demande du pseudo
    n = recv(client_desc, buffer, sizeof(buffer) - 1, 0);
    if (n <= 0) { printf("Connexion perdue avec le serveur.\n"); return; }
    buffer[n] = '\0';
    printf("\nReponse du serveur : %s\n", buffer);

    // 3) Envoie du pseudo saisi
    printf("Entrez votre pseudo : ");
    scanf("%1023s", buffer);
    send(client_desc, buffer, strlen(buffer) + 1, 0);

    // 4) le serveur renvoie notre ID
    n = recv(client_desc, buffer, sizeof(buffer) - 1, 0);
    if (n <= 0) { printf("Connexion perdue avec le serveur.\n"); return; }
    buffer[n] = '\0';
    printf("\nVotre ID est : %s\n", buffer);
}

void Send(){ printf("(SEND pas encore implemente)\n"); }
void Read(){ printf("(READ pas encore implemente)\n"); }
void QUITTER(){ printf("Fermeture...\n"); }

int choix;

void menu(){
    do{
        printf("\nEntrer votre choix \n");
        printf("1-Inscription\n");
        printf("2-SEND\n");
        printf("3-READ\n");
        printf("4-QUITTER\n");
        scanf("%d", &choix);
    }while(choix < 1 || choix > 4);
}

void choie(){
    switch(choix){
        case 1: Inscription(); break;
        case 2: Send();        break;
        case 3: Read();        break;
        case 4: QUITTER();     break;
    }
}

int main(int argc, char **argv){
    if (argc < 2) {
        fprintf(stderr, "Usage: %s <port>\n", argv[0]);
        return 1;
    }

    // Creation de socket
    client_desc = socket(AF_INET, SOCK_STREAM, 0);
    if (client_desc < 0) { perror("socket"); return 1; }
    printf("socket client cree avec succes \n");

    // Connexion au serveur
    memset(&addr, '\0', sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(atoi(argv[1]));
    addr.sin_addr.s_addr = inet_addr("127.0.0.1");

    if (connect(client_desc, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
        perror("connect");
        close(client_desc);
        return 1;
    }
    printf("Connexion au serveur avec succes \n");

    // Boucle du menu : on revient au menu apres chaque action, jusqu'a QUITTER
    do {
        menu();
        choie();
    } while (choix != 4);

    // Fermeture socket
    close(client_desc);
    printf("Client deconnecte \n");
    return 0;
}
