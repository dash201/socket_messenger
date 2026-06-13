# Security Notes / Notes de sécurité

🇬🇧 [English](#english) · 🇫🇷 [Français](#français)

---

## English

This note documents the security posture of **Socket Messenger**, focused on the
threaded + MySQL server (`v2-Pthread/`), which is the version that handles
untrusted input and a database. The `v1-fork/` version is a minimal prototype
(registration only, no database) and shares the same input-handling practices.

### Implemented protections

- **No SQL injection.**
  - String inputs (username, message body) are inserted via **prepared
    statements** (`mysql_stmt_*` with `?` placeholders). The value is sent
    separately from the SQL text and can never be interpreted as code.
  - Numeric inputs (user ids) are parsed and validated as integers
    (`parse_id`) before being embedded as `%d`. A validated integer cannot
    carry an injection payload.
  - No user-controlled string is ever concatenated into a query.
- **No hardcoded credentials.** Database host/user/password/name are read from
  the environment (`DB_HOST`, `DB_USER`, `DB_PASS`, `DB_NAME`), so no secret is
  committed to the repository.
- **Memory safety.** Every `recv` return value is checked; buffers are
  NUL-terminated using the received length; all formatting uses bounded
  `snprintf`; there is no unbounded `strcat`/`strcpy` of network data and no
  integer-sized string buffer.
- **Thread safety.** Each client thread owns its own state and its own MySQL
  connection, initialised with `mysql_thread_init()` / `mysql_thread_end()`.
  There is no shared mutable state between threads, so there is no data race.
- **Resource handling.** One allocation per connection with `pthread_detach`
  (no fixed-size client array to overflow); result sets are freed and sockets
  are closed on every path.

### Threat model & limitations

This is a learning / portfolio project. The following are **out of scope** for
this version and would be required before any real deployment:

- **No transport encryption.** Traffic (usernames, ids, messages) travels in
  **clear text** over TCP. Do not use on an untrusted network. Mitigation: add
  TLS (e.g. OpenSSL) or tunnel over SSH/VPN.
- **Identity, not authentication.** `CONN` only requires knowing a numeric user
  id — there is no password. Ids are sequential (auto-increment from 2023) and
  therefore guessable, so a user can be impersonated. Mitigation: add
  password-based authentication with hashing (e.g. Argon2/bcrypt).
- **No rate limiting.** A client can open many connections/threads; there is no
  flood or abuse protection.
- **Least privilege.** The MySQL account should be restricted to the `socket`
  database and the operations actually used — not a `root`/admin account.

### Reporting

This is a personal project; please open a GitHub issue for any security concern.

---

## Français

Cette note documente la posture de sécurité de **Socket Messenger**, centrée sur
le serveur threads + MySQL (`v2-Pthread/`), la version qui traite des entrées non
fiables et une base de données. La version `v1-fork/` est un prototype minimal
(inscription seulement, sans base) et partage les mêmes pratiques de traitement
des entrées.

### Protections en place

- **Pas d'injection SQL.**
  - Les entrées texte (pseudo, contenu du message) sont insérées via des
    **requêtes préparées** (`mysql_stmt_*` avec des marqueurs `?`). La valeur est
    envoyée séparément du texte SQL et ne peut jamais être interprétée comme du
    code.
  - Les entrées numériques (id utilisateur) sont analysées et validées comme des
    entiers (`parse_id`) avant d'être réinjectées en `%d`. Un entier validé ne
    peut pas porter de charge d'injection.
  - Aucune chaîne contrôlée par l'utilisateur n'est concaténée dans une requête.
- **Aucun identifiant en dur.** L'hôte/utilisateur/mot de passe/nom de la base
  sont lus dans l'environnement (`DB_HOST`, `DB_USER`, `DB_PASS`, `DB_NAME`) :
  aucun secret n'est versionné dans le dépôt.
- **Sécurité mémoire.** Chaque retour de `recv` est vérifié ; les buffers sont
  terminés par NUL selon la taille reçue ; tout le formatage utilise des
  `snprintf` bornés ; pas de `strcat`/`strcpy` non borné de données réseau, ni de
  buffer dimensionné sur la taille d'un entier.
- **Sécurité des threads.** Chaque thread client possède son propre état et sa
  propre connexion MySQL, initialisée avec `mysql_thread_init()` /
  `mysql_thread_end()`. Aucun état mutable n'est partagé entre threads, donc pas
  de situation de compétition (*data race*).
- **Gestion des ressources.** Une allocation par connexion avec `pthread_detach`
  (pas de tableau de clients de taille fixe à déborder) ; les résultats sont
  libérés et les sockets fermés sur tous les chemins.

### Modèle de menace & limites

C'est un projet d'apprentissage / de portfolio. Les points suivants sont **hors
périmètre** pour cette version et seraient nécessaires avant tout déploiement
réel :

- **Pas de chiffrement du transport.** Le trafic (pseudos, id, messages) circule
  en **clair** sur TCP. À ne pas utiliser sur un réseau non fiable. Parade :
  ajouter TLS (par ex. OpenSSL) ou passer par un tunnel SSH/VPN.
- **Identification, pas authentification.** `CONN` ne demande que de connaître un
  id numérique — il n'y a pas de mot de passe. Les id sont séquentiels
  (auto-incrément à partir de 2023) donc devinables : un utilisateur peut être
  usurpé. Parade : authentification par mot de passe avec hachage (Argon2/bcrypt).
- **Pas de limitation de débit.** Un client peut ouvrir de nombreuses
  connexions/threads ; aucune protection contre le flood ou l'abus.
- **Moindre privilège.** Le compte MySQL devrait être restreint à la base
  `socket` et aux opérations réellement utilisées — pas un compte `root`/admin.

### Signalement

Projet personnel : merci d'ouvrir une issue GitHub pour toute question de sécurité.
