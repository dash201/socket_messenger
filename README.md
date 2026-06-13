# Socket Messenger

A client/server chat messenger written in **C**, built on BSD sockets over TCP.
It ships in two implementations of the same system: a lightweight process-based
server and a concurrent, MySQL-backed server.

🇬🇧 [English](#english) · 🇫🇷 [Français](#français)

---

## English

### Overview

A command-line chat system where clients connect to a central server to
register, log in, list connected users, and exchange messages. Two server
implementations share the same protocol idea but use different concurrency and
persistence models:

| Version | Folder | Concurrency | Persistence | Highlights |
|---------|--------|-------------|-------------|------------|
| **fork** | [`v1-fork/`](v1-fork/) | One child process per client (`fork`) | File-backed id counter | Minimal, readable reference of the socket lifecycle and a request/response protocol |
| **threaded + MySQL** | [`v2-Pthread/`](v2-Pthread/) | One POSIX thread per client (`pthread`) | MySQL (users + messages) | Register, login, list users, store-and-forward messaging, message reading |

### Architecture

**fork version** — `socket → bind → listen → accept`, then one `fork` per
client. The parent keeps accepting; each child handles a single client until it
disconnects. `SIGCHLD` is ignored so finished children are reaped automatically.

**threaded version** — a single process accepts connections and spawns one
`pthread` per client, each with its own MySQL connection. Users and messages are
persisted, so state survives across connections: a user logs back in with their
id, and messages are stored and delivered when the recipient runs `READ`.

**Protocol** — a small text protocol keyed on a command word
(`INS`, `CONN`, `LIST`, `SEND`, `READ`, `EXIT`). Registration handshake:

```
client → INS
server → Enter your username
client → <username>
server → <id>
```

### Build & run

**fork version**

```bash
cd v1-fork
gcc TPserverfork1.c -o server
gcc TPclient.c      -o client
./server 5000      # terminal 1
./client 5000      # terminal 2
```

**threaded + MySQL version**

```bash
cd v2-Pthread
mysql -u <user> -p socket < socket.sql              # create the schema
gcc TPserverfork.c -o server -lpthread -lmysqlclient
gcc TPclient.c     -o client

DB_USER=<user> DB_PASS=<pass> ./server 5000          # terminal 1
./client 5000                                        # terminal 2
```

> POSIX only (Linux / WSL / macOS): relies on `fork()`, `pthread` and BSD
> sockets. On Windows, run it inside WSL. The threaded version also needs the
> MySQL client library (`libmysqlclient-dev`). DB credentials are read from the
> environment (`DB_HOST`, `DB_USER`, `DB_PASS`, `DB_NAME`).

### Security

The threaded version applies several security practices — prepared statements
against SQL injection, integer validation of ids, environment-based credentials,
and bounds-checked buffers. See **[SECURITY.md](SECURITY.md)** for the full note,
including the threat model and limitations.

### Tech

C · BSD sockets · TCP client/server IPC · processes (`fork`) · POSIX threads
(`pthread`) · MySQL.

---

## Français

### Présentation

Un système de messagerie en ligne de commande : des clients se connectent à un
serveur central pour s'inscrire, se connecter, lister les utilisateurs connectés
et échanger des messages. Deux implémentations du serveur partagent la même idée
de protocole mais utilisent des modèles de concurrence et de persistance
différents :

| Version | Dossier | Concurrence | Persistance | Points clés |
|---------|---------|-------------|-------------|-------------|
| **fork** | [`v1-fork/`](v1-fork/) | Un processus fils par client (`fork`) | Compteur d'id dans un fichier | Référence minimale et lisible du cycle de vie d'un socket et d'un protocole requête/réponse |
| **threads + MySQL** | [`v2-Pthread/`](v2-Pthread/) | Un thread POSIX par client (`pthread`) | MySQL (utilisateurs + messages) | Inscription, connexion, liste, messagerie « stockage et remise », lecture des messages |

### Architecture

**Version fork** — `socket → bind → listen → accept`, puis un `fork` par client.
Le parent continue d'accepter ; chaque fils gère un seul client jusqu'à sa
déconnexion. `SIGCHLD` est ignoré pour récupérer automatiquement les fils.

**Version threads** — un processus unique accepte les connexions et crée un
`pthread` par client, chacun avec sa propre connexion MySQL. Les utilisateurs et
les messages sont persistés : l'état survit aux connexions, un utilisateur se
reconnecte avec son id, et les messages sont stockés puis remis quand le
destinataire lance `READ`.

**Protocole** — un petit protocole texte basé sur un mot-clé de commande
(`INS`, `CONN`, `LIST`, `SEND`, `READ`, `EXIT`). Poignée de main d'inscription :

```
client → INS
serveur → Entrez votre pseudo
client → <pseudo>
serveur → <id>
```

### Compilation & exécution

**Version fork**

```bash
cd v1-fork
gcc TPserverfork1.c -o server
gcc TPclient.c      -o client
./server 5000      # terminal 1
./client 5000      # terminal 2
```

**Version threads + MySQL**

```bash
cd v2-Pthread
mysql -u <user> -p socket < socket.sql              # créer le schéma
gcc TPserverfork.c -o server -lpthread -lmysqlclient
gcc TPclient.c     -o client

DB_USER=<user> DB_PASS=<pass> ./server 5000          # terminal 1
./client 5000                                        # terminal 2
```

> POSIX uniquement (Linux / WSL / macOS) : utilise `fork()`, `pthread` et les
> sockets BSD. Sous Windows, lancer dans WSL. La version threads requiert aussi
> la bibliothèque cliente MySQL (`libmysqlclient-dev`). Les identifiants de la
> base sont lus dans l'environnement (`DB_HOST`, `DB_USER`, `DB_PASS`, `DB_NAME`).

### Sécurité

La version threads applique plusieurs bonnes pratiques — requêtes préparées
contre l'injection SQL, validation entière des id, identifiants via
l'environnement, et buffers bornés. Voir **[SECURITY.md](SECURITY.md)** pour la
note complète, avec le modèle de menace et les limites.

### Technologies

C · sockets BSD · communication client/serveur TCP · processus (`fork`) ·
threads POSIX (`pthread`) · MySQL.
