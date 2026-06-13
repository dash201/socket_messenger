/*
 * Serveur de chat — multithread + MySQL.
 *
 * Un thread POSIX par client. Les utilisateurs et les messages sont persistés
 * dans MySQL. Les messages suivent un modèle « stockage et remise » : chaque
 * message est écrit dans la table `msg` et remis au destinataire via READ.
 *
 * Protocole (un mot-clé de commande par requête, puis un échange guidé) :
 *   INS   inscrire un nouvel utilisateur   -> attribue un id unique
 *   CONN  se connecter avec un id existant  -> passe l'utilisateur en "connecte"
 *   LIST  lister les utilisateurs connectés
 *   SEND  envoyer "<id_destinataire>/<texte>" -> stocké pour le destinataire
 *   READ  lire (et vider) sa boîte de réception
 *   EXIT  se déconnecter                     -> passe l'utilisateur en "deconnecte"
 *
 * Compilation (Linux, nécessite libmysqlclient-dev) :
 *   gcc TPserverfork.c -o server -lpthread -lmysqlclient
 * Lancement :
 *   DB_USER=... DB_PASS=... ./server <port>
 *
 * Notes de sécurité :
 *   - Les entrées texte (pseudo, message) passent par des REQUETES PREPAREES :
 *     la valeur ne peut jamais être interprétée comme du SQL (pas d'injection).
 *   - Les entrées numériques (les id) sont validées comme des entiers puis
 *     réinjectées en %d — un entier validé ne peut pas porter d'injection.
 *   - Les identifiants de connexion à la base viennent de l'environnement,
 *     jamais codés en dur.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <limits.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <mysql/mysql.h>

#define BUF 1024
#define MAX_PSEUDO 20      /* correspond à users.pseudo  varchar(20) */
#define MAX_CONTENT 25     /* correspond à msg.content    varchar(25) */

/* État d'un client, possédé par un seul thread (pas de partage -> pas de race). */
struct client {
    int sock;
    struct sockaddr_in addr;
    int user_id;                 /* 0 tant que le client n'est ni inscrit ni connecté */
    char pseudo[MAX_PSEUDO + 1];
};

/* ---- utilitaires ----------------------------------------------------- */

/* Ouvre une connexion MySQL avec les identifiants de l'environnement. */
static MYSQL *db_connect(void) {
    MYSQL *c = mysql_init(NULL);
    if (!c) return NULL;

    const char *host = getenv("DB_HOST"); if (!host) host = "localhost";
    const char *user = getenv("DB_USER"); if (!user) user = "root";
    const char *pass = getenv("DB_PASS"); if (!pass) pass = "";
    const char *name = getenv("DB_NAME"); if (!name) name = "socket";

    if (!mysql_real_connect(c, host, user, pass, name, 0, NULL, 0)) {
        fprintf(stderr, "[db] échec de connexion : %s\n", mysql_error(c));
        mysql_close(c);
        return NULL;
    }
    return c;
}

/* Envoie une chaîne terminée par NUL (le NUL fait partie du cadrage). */
static void send_msg(int sock, const char *s) {
    send(sock, s, strlen(s) + 1, 0);
}

/* Reçoit un message ; renvoie le nombre d'octets (<=0 = fermé/erreur). */
static int recv_msg(int sock, char *buf, size_t cap) {
    int n = recv(sock, buf, cap - 1, 0);
    if (n <= 0) return n;
    buf[n] = '\0';
    return n;
}

/* Analyse un entier strictement positif ; renvoie 1 en cas de succès. */
static int parse_id(const char *s, int *out) {
    char *end;
    errno = 0;
    long v = strtol(s, &end, 10);
    if (errno || end == s || *end != '\0' || v <= 0 || v > INT_MAX)
        return 0;
    *out = (int)v;
    return 1;
}

/* ---- gestionnaires de commandes -------------------------------------- */
/* Renvoient -1 si le client s'est déconnecté en cours d'échange, 0 sinon. */

static int cmd_register(struct client *c, MYSQL *conn) {
    char pseudo[BUF];
    send_msg(c->sock, "Entrez votre pseudo");
    if (recv_msg(c->sock, pseudo, sizeof pseudo) <= 0) return -1;

    MYSQL_STMT *st = mysql_stmt_init(conn);
    const char *q = "INSERT INTO users(pseudo, statut) VALUES(?, 'connecte')";
    unsigned long len = strlen(pseudo);
    if (len > MAX_PSEUDO) len = MAX_PSEUDO;          /* pour tenir dans la colonne */

    MYSQL_BIND b;
    memset(&b, 0, sizeof b);
    b.buffer_type   = MYSQL_TYPE_STRING;
    b.buffer        = pseudo;
    b.buffer_length = len;
    b.length        = &len;

    if (!st || mysql_stmt_prepare(st, q, strlen(q)) ||
        mysql_stmt_bind_param(st, &b) || mysql_stmt_execute(st)) {
        fprintf(stderr, "[db] inscription : %s\n", mysql_error(conn));
        send_msg(c->sock, "Échec de l'inscription.");
        if (st) mysql_stmt_close(st);
        return 0;
    }

    c->user_id = (int)mysql_stmt_insert_id(st);
    mysql_stmt_close(st);

    snprintf(c->pseudo, sizeof c->pseudo, "%.*s", (int)len, pseudo);
    char out[64];
    snprintf(out, sizeof out, "Inscription réussie. Votre id est %d", c->user_id);
    send_msg(c->sock, out);
    printf("[+] Inscription de '%s' (id %d)\n", c->pseudo, c->user_id);
    return 0;
}

static int cmd_login(struct client *c, MYSQL *conn) {
    char in[BUF];
    send_msg(c->sock, "Entrez votre id utilisateur");
    if (recv_msg(c->sock, in, sizeof in) <= 0) return -1;

    int id;
    if (!parse_id(in, &id)) { send_msg(c->sock, "Id invalide."); return 0; }

    char q[128];
    snprintf(q, sizeof q, "SELECT pseudo FROM users WHERE usr_id = %d", id);
    if (mysql_query(conn, q)) {
        send_msg(c->sock, "Échec de la connexion."); return 0;
    }
    MYSQL_RES *res = mysql_store_result(conn);
    MYSQL_ROW row = res ? mysql_fetch_row(res) : NULL;
    if (!row) {
        if (res) mysql_free_result(res);
        send_msg(c->sock, "Id inconnu."); return 0;
    }
    c->user_id = id;
    snprintf(c->pseudo, sizeof c->pseudo, "%s", row[0] ? row[0] : "");
    mysql_free_result(res);

    snprintf(q, sizeof q, "UPDATE users SET statut='connecte' WHERE usr_id = %d", id);
    mysql_query(conn, q);
    send_msg(c->sock, "Connexion réussie.");
    printf("[+] Connexion id %d (%s)\n", id, c->pseudo);
    return 0;
}

static void cmd_list(struct client *c, MYSQL *conn) {
    if (mysql_query(conn,
            "SELECT usr_id, pseudo FROM users WHERE statut='connecte'")) {
        send_msg(c->sock, "Échec de la liste."); return;
    }
    MYSQL_RES *res = mysql_store_result(conn);
    char out[BUF];
    size_t l = 0;
    l += snprintf(out + l, sizeof out - l, "Utilisateurs connectés [id] pseudo :\n");
    MYSQL_ROW row;
    while (res && (row = mysql_fetch_row(res)) && l < sizeof out - 1) {
        int w = snprintf(out + l, sizeof out - l, "[%s] %s\n",
                         row[0] ? row[0] : "", row[1] ? row[1] : "");
        if (w < 0) break;
        l += (size_t)w;
    }
    if (res) mysql_free_result(res);
    send_msg(c->sock, out);
}

static int cmd_send(struct client *c, MYSQL *conn) {
    char in[BUF];
    if (c->user_id == 0) {
        send_msg(c->sock, "Veuillez d'abord vous inscrire ou vous connecter."); return 0;
    }
    send_msg(c->sock, "Envoyez sous la forme <id_destinataire>/<message>");
    if (recv_msg(c->sock, in, sizeof in) <= 0) return -1;

    char *slash = strchr(in, '/');
    if (!slash) { send_msg(c->sock, "Format invalide."); return 0; }
    *slash = '\0';
    const char *text = slash + 1;

    int rcv;
    if (!parse_id(in, &rcv)) { send_msg(c->sock, "Id de destinataire invalide."); return 0; }

    /* le destinataire doit exister */
    char q[128];
    snprintf(q, sizeof q, "SELECT 1 FROM users WHERE usr_id = %d", rcv);
    if (mysql_query(conn, q)) { send_msg(c->sock, "Échec de l'envoi."); return 0; }
    MYSQL_RES *res = mysql_store_result(conn);
    int exists = res && mysql_fetch_row(res);
    if (res) mysql_free_result(res);
    if (!exists) { send_msg(c->sock, "Destinataire inconnu."); return 0; }

    /* stockage du message (requête préparée : le texte n'est jamais lu comme du SQL) */
    MYSQL_STMT *st = mysql_stmt_init(conn);
    const char *iq = "INSERT INTO msg(content, usr_send, usr_rcv) VALUES(?, ?, ?)";
    unsigned long tlen = strlen(text);
    if (tlen > MAX_CONTENT) tlen = MAX_CONTENT;

    MYSQL_BIND b[3];
    memset(b, 0, sizeof b);
    b[0].buffer_type = MYSQL_TYPE_STRING; b[0].buffer = (char *)text;
    b[0].buffer_length = tlen;            b[0].length = &tlen;
    b[1].buffer_type = MYSQL_TYPE_LONG;   b[1].buffer = &c->user_id;
    b[2].buffer_type = MYSQL_TYPE_LONG;   b[2].buffer = &rcv;

    if (!st || mysql_stmt_prepare(st, iq, strlen(iq)) ||
        mysql_stmt_bind_param(st, b) || mysql_stmt_execute(st)) {
        fprintf(stderr, "[db] envoi : %s\n", mysql_error(conn));
        send_msg(c->sock, "Échec de l'envoi.");
    } else {
        send_msg(c->sock, "Message envoyé.");
    }
    if (st) mysql_stmt_close(st);
    return 0;
}

static void cmd_read(struct client *c, MYSQL *conn) {
    if (c->user_id == 0) {
        send_msg(c->sock, "Veuillez d'abord vous inscrire ou vous connecter."); return;
    }
    char q[256];
    snprintf(q, sizeof q,
        "SELECT u.usr_id, u.pseudo, m.content "
        "FROM msg m JOIN users u ON m.usr_send = u.usr_id "
        "WHERE m.usr_rcv = %d", c->user_id);
    if (mysql_query(conn, q)) { send_msg(c->sock, "Échec de la lecture."); return; }

    MYSQL_RES *res = mysql_store_result(conn);
    char out[BUF];
    size_t l = 0;
    l += snprintf(out + l, sizeof out - l, "Vos messages [id_exp pseudo] texte :\n");
    MYSQL_ROW row;
    while (res && (row = mysql_fetch_row(res)) && l < sizeof out - 1) {
        int w = snprintf(out + l, sizeof out - l, "[%s %s] %s\n",
                         row[0] ? row[0] : "", row[1] ? row[1] : "",
                         row[2] ? row[2] : "");
        if (w < 0) break;
        l += (size_t)w;
    }
    if (res) mysql_free_result(res);

    /* les messages sont remis une seule fois, puis effacés */
    snprintf(q, sizeof q, "DELETE FROM msg WHERE usr_rcv = %d", c->user_id);
    mysql_query(conn, q);

    send_msg(c->sock, out);
}

/* ---- thread par client ----------------------------------------------- */

static void *handle_client(void *arg) {
    struct client *c = arg;
    char cmd[BUF];

    mysql_thread_init();                 /* obligatoire pour MySQL dans un thread */
    MYSQL *conn = db_connect();
    if (!conn) {
        send_msg(c->sock, "Serveur indisponible.");
        close(c->sock); free(c); mysql_thread_end();
        return NULL;
    }

    while (recv_msg(c->sock, cmd, sizeof cmd) > 0) {
        if      (strcmp(cmd, "INS")  == 0) { if (cmd_register(c, conn) < 0) break; }
        else if (strcmp(cmd, "CONN") == 0) { if (cmd_login(c, conn)    < 0) break; }
        else if (strcmp(cmd, "LIST") == 0) cmd_list(c, conn);
        else if (strcmp(cmd, "SEND") == 0) { if (cmd_send(c, conn)     < 0) break; }
        else if (strcmp(cmd, "READ") == 0) cmd_read(c, conn);
        else if (strcmp(cmd, "EXIT") == 0 || strcmp(cmd, "end") == 0) break;
        else send_msg(c->sock, "Commande inconnue.");
    }

    if (c->user_id) {
        char q[128];
        snprintf(q, sizeof q,
                 "UPDATE users SET statut='deconnecte' WHERE usr_id = %d", c->user_id);
        mysql_query(conn, q);
    }
    printf("[-] %s:%d déconnecté.\n",
           inet_ntoa(c->addr.sin_addr), ntohs(c->addr.sin_port));

    mysql_close(conn);
    close(c->sock);
    free(c);
    mysql_thread_end();
    return NULL;
}

/* ---- main ------------------------------------------------------------ */

int main(int argc, char **argv) {
    if (argc < 2) {
        fprintf(stderr, "Usage: %s <port>\n", argv[0]);
        return 1;
    }
    if (mysql_library_init(0, NULL, NULL)) {
        fprintf(stderr, "mysql_library_init a échoué\n");
        return 1;
    }

    int srv = socket(AF_INET, SOCK_STREAM, 0);
    if (srv < 0) { perror("socket"); return 1; }

    int opt = 1;
    setsockopt(srv, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof opt);

    struct sockaddr_in addr;
    memset(&addr, 0, sizeof addr);
    addr.sin_family      = AF_INET;
    addr.sin_port        = htons(atoi(argv[1]));
    addr.sin_addr.s_addr = htonl(INADDR_ANY);

    if (bind(srv, (struct sockaddr *)&addr, sizeof addr) < 0) { perror("bind");   return 1; }
    if (listen(srv, 16) < 0)                                  { perror("listen"); return 1; }
    printf("[+] Serveur en écoute sur le port %s...\n", argv[1]);

    while (1) {
        struct client *c = malloc(sizeof *c);
        if (!c) { perror("malloc"); continue; }

        socklen_t len = sizeof c->addr;
        c->sock = accept(srv, (struct sockaddr *)&c->addr, &len);
        if (c->sock < 0) { perror("accept"); free(c); continue; }
        c->user_id = 0;
        c->pseudo[0] = '\0';
        printf("[+] Connexion de %s:%d\n",
               inet_ntoa(c->addr.sin_addr), ntohs(c->addr.sin_port));

        pthread_t t;
        if (pthread_create(&t, NULL, handle_client, c) != 0) {
            perror("pthread_create");
            close(c->sock); free(c);
            continue;
        }
        pthread_detach(t);          /* pas de join ; le thread se libère lui-même */
    }

    mysql_library_end();
    return 0;
}
