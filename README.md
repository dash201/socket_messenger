# Socket Messenger

A TCP **client/server in C**, built on BSD sockets. The server handles multiple
clients concurrently with a process-per-client model (`fork`) and speaks a small
text protocol; the client offers a menu-driven interface with a working user
registration flow.

🇬🇧 [English](#english) · 🇫🇷 [Français](#français)

---

## English

### Overview

A command-line client/server application over TCP. The server accepts several
clients at once — one child process per client via `fork` — and exposes a simple
protocol keyed on a command word. The implemented command, `INS`, registers a
user and assigns them a unique, persistent id.

### Architecture

```
                 fork()                 fork()
   ┌────────┐   ─────────►  ┌────────┐ ─────────► ┌────────┐
   │ server │  accept loop  │ child  │            │ child  │
   │ (main) │ ────────────► │ client │   ...      │ client │
   └────────┘               └────────┘            └────────┘
```

- `socket → bind → listen → accept`, then one `fork` per client.
- The parent keeps accepting connections; each child handles a single client
  until it disconnects.
- `SIGCHLD` is ignored so finished children are reaped automatically (no zombies).
- Unique ids come from a file-backed counter — necessary because, under `fork`,
  children don't share memory, so a file is the only state common to all of them.

### Protocol

A small text protocol. The registration handshake:

```
client → INS
server → Enter your username
client → <username>
server → <id>
```

### Build & run

```bash
cd v1-fork
gcc TPserverfork1.c -o server
gcc TPclient.c      -o client

./server 5000      # terminal 1
./client 5000      # terminal 2  (connects to 127.0.0.1)
```

> POSIX only (Linux / WSL / macOS): relies on `fork()` and BSD sockets. On
> Windows, run it inside WSL.

### Tech

C · BSD sockets · TCP client/server IPC · processes (`fork`).

---

## Français

### Présentation

Une application client/serveur en ligne de commande au-dessus de TCP. Le serveur
accepte plusieurs clients simultanément — un processus fils par client via
`fork` — et expose un protocole simple basé sur un mot-clé de commande. La
commande implémentée, `INS`, inscrit un utilisateur et lui attribue un
identifiant unique et persistant.

### Architecture

```
                 fork()                 fork()
   ┌────────┐   ─────────►  ┌────────┐ ─────────► ┌────────┐
   │ serveur│  boucle accept│  fils  │            │  fils  │
   │ (main) │ ────────────► │ client │   ...      │ client │
   └────────┘               └────────┘            └────────┘
```

- `socket → bind → listen → accept`, puis un `fork` par client.
- Le parent continue d'accepter les connexions ; chaque fils gère un seul client
  jusqu'à sa déconnexion.
- `SIGCHLD` est ignoré pour récupérer automatiquement les fils terminés (pas de
  zombies).
- Les identifiants uniques proviennent d'un compteur stocké dans un fichier —
  nécessaire car, avec `fork`, les fils ne partagent pas leur mémoire : le
  fichier est le seul état commun à toutes les connexions.

### Protocole

Un petit protocole texte. La poignée de main d'inscription :

```
client → INS
serveur → Enter your username
client → <pseudo>
serveur → <id>
```

### Compilation & exécution

```bash
cd v1-fork
gcc TPserverfork1.c -o server
gcc TPclient.c      -o client

./server 5000      # terminal 1
./client 5000      # terminal 2  (se connecte à 127.0.0.1)
```

> POSIX uniquement (Linux / WSL / macOS) : utilise `fork()` et les sockets BSD.
> Sous Windows, lancer dans WSL.

### Technologies

C · sockets BSD · communication client/serveur TCP · processus (`fork`).
