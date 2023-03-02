#include <stdlib.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <netdb.h>
 /* do{
        printf("Entrez votre message : ");
        //scanf("%s", buffer);
        fgets(buffer, sizeof(buffer), stdin);
        send(client_desc, buffer, sizeof(buffer), 0);
        read(client_desc, buffer, sizeof(buffer));
        printf("\nReponse du serveur : %s \n", buffer);

    }while(strcmp(buffer, "end")!=0);
    */


    int client_desc;
    struct sockaddr_in addr;
    char buffer[1024];



void Inscription(){
      do{
        printf("Inscription ");
        strcpy(buffer,"INS");
        send(client_desc, buffer, sizeof(buffer), 0);
        read(client_desc, buffer, sizeof(buffer)); // serveur demande le pseudo
        printf("\nReponse du serveur : %s \n", buffer);
        scanf("%s",buffer);
        //fgets(buffer, sizeof(buffer), stdin); //pseudo
        send(client_desc, buffer, sizeof(buffer), 0);
        read(client_desc, buffer, sizeof(buffer)); // lecture de ID
        printf("\nVotre ID est : %s \n", buffer);

    }while(strcmp(buffer, "end")!=0);

};
void Send(){};
void Read(){};
void QUITTER(){};
int choix;

void menu(){
    do{
    printf("Entrer votre choix \n");
    printf("1-Inscription\n");
    printf("2-SEND\n");
    printf("3-READ\n");
    printf("4-QUITTER\n");
    scanf("%d",&choix);
    }while(choix<1 || choix>4);
}
void choie(){
    switch(choix){
        case 1: 
        printf("1-Inscription\n");
        Inscription(); //discuter avec le serveur  | 
        break;
        case 2:
        printf("2-SEND\n");
        Send();
        break;
        case 3:
        printf("3-READ\n");
        Read();
        break;
        case 4:
        printf("4-vous quitter\n");
        QUITTER();
        break;
    }

    
}


int main(int argc, char **argv){

    


    // Creation socket
    client_desc = socket(AF_INET, SOCK_STREAM, 0);
    printf("socket client créé avec succes \n");

    // Connection au server
    memset(&addr, '\n', sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(atoi(argv[1]));
    addr.sin_addr.s_addr = inet_addr("127.0.0.1");

    connect(client_desc, (struct sockaddr*)&addr, sizeof(addr));
    printf("Connexion au Server avec succes \n");
    menu();
    choie();

    // Transfert des données
  
    

    // Fermeture socket
    close(client_desc);
    printf("Client Deconnecte \n"); 
    
}
