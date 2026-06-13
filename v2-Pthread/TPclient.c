/*
 * Client de chat (compagnon du serveur multithread + MySQL).
 *
 * Menu interactif : inscription, connexion, liste des utilisateurs connectés,
 * envoi et lecture de messages. Chaque commande est un échange strict
 * requête/réponse avec le serveur.
 *
 * Compilation : gcc TPclient.c -o client
 * Lancement   : ./client <port>     (se connecte à 127.0.0.1)
 */
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#define BUF 1024

static int sock;

/* Lit une ligne complète depuis stdin (espaces permis), sans le saut de ligne. */
static void read_line(char *buf, size_t cap) {
    if (fgets(buf, cap, stdin))
        buf[strcspn(buf, "\n")] = '\0';
    else
        buf[0] = '\0';
}

/* Lit le choix du menu en entier (via une ligne, pour éviter les soucis de scanf). */
static int read_choice(void) {
    char line[16];
    read_line(line, sizeof line);
    return atoi(line);
}

/* Envoie un message terminé par NUL au serveur. */
static void send_cmd(const char *s) {
    send(sock, s, strlen(s) + 1, 0);
}

/* Reçoit une réponse du serveur et l'affiche ; renvoie 0 si le serveur a fermé. */
static int recv_show(void) {
    char buf[BUF];
    int n = recv(sock, buf, sizeof buf - 1, 0);
    if (n <= 0) {
        printf("Connexion fermée par le serveur.\n");
        return 0;
    }
    buf[n] = '\0';
    printf("\nServeur ---> %s\n", buf);
    return 1;
}

/* ---- commandes ------------------------------------------------------- */

static int do_inscription(void) {
    char pseudo[64];
    send_cmd("INS");
    if (!recv_show()) return 0;               /* le serveur demande le pseudo */
    printf("Pseudo : ");
    read_line(pseudo, sizeof pseudo);
    send_cmd(pseudo);
    return recv_show();                       /* l'id attribué */
}

static int do_connexion(void) {
    char id[32];
    send_cmd("CONN");
    if (!recv_show()) return 0;
    printf("Votre id : ");
    read_line(id, sizeof id);
    send_cmd(id);
    return recv_show();
}

static int do_liste(void) {
    send_cmd("LIST");
    return recv_show();
}

static int do_envoyer(void) {
    char ligne[BUF];
    send_cmd("SEND");
    if (!recv_show()) return 0;               /* le serveur indique le format */
    printf("Message (id/texte) : ");
    read_line(ligne, sizeof ligne);
    send_cmd(ligne);
    return recv_show();
}

static int do_lire(void) {
    send_cmd("READ");
    return recv_show();
}

static void menu(void) {
    printf("\n\t⁕‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾⁕\n");
    printf("\t|              Entrez votre choix :              |\n");
    printf("\t| 1 — 👉 INSCRIPTION                             |\n");
    printf("\t| 2 — 👉 CONNEXION                               |\n");
    printf("\t| 3 — 👉 LISTE                                   |\n");
    printf("\t| 4 — 👉 ENVOYER                                 |\n");
    printf("\t| 5 — 👉 LIRE                                    |\n");
    printf("\t| 6 — 👉 QUITTER                                 |\n");
    printf("\t⁕_______________________________________________⁕\n");
    printf("\t> ");
}

int main(int argc, char **argv) {
    struct sockaddr_in addr;

    if (argc < 2) {
        fprintf(stderr, "Usage: %s <port>\n", argv[0]);
        return 1;
    }

    sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) { perror("socket"); return 1; }

    memset(&addr, 0, sizeof addr);
    addr.sin_family      = AF_INET;
    addr.sin_port        = htons(atoi(argv[1]));
    addr.sin_addr.s_addr = inet_addr("127.0.0.1");

    if (connect(sock, (struct sockaddr *)&addr, sizeof addr) < 0) {
        perror("connect");
        close(sock);
        return 1;
    }
    printf("Connecté au serveur.\n");

    int running = 1;
    while (running) {
        menu();
        switch (read_choice()) {
            case 1: running = do_inscription(); break;
            case 2: running = do_connexion();   break;
            case 3: running = do_liste();        break;
            case 4: running = do_envoyer();      break;
            case 5: running = do_lire();         break;
            case 6: send_cmd("EXIT"); running = 0; break;
            default: printf("Choix invalide.\n"); break;
        }
    }

    close(sock);
    printf("Déconnecté.\n");
    return 0;
}
