# One-on-One Chat Application

A modular, terminal-based messaging application written in C. Users can register, log in, send messages, read their inbox, and reply — all backed by the local filesystem with file-locking for concurrency safety.

## Prerequisites

- **GCC** (with C11 support)
- **Linux** (uses POSIX `flock` for file locking)
- **Make**

On Debian/Ubuntu:

```bash
sudo apt update && sudo apt install build-essential
```

## Quick Start

```bash
# 1. Build
make clean && make

# 2. Run
./chat_app
```

The application creates a `data/` directory automatically to store all runtime files (`users.txt`, inbox files, `system.log`). This directory is gitignored.

## How It Works

### Login Menu

```
1. Register New Account   – Create a username/password
2. Login                  – Authenticate with existing credentials
3. Exit                   – Quit the application
```

### Dashboard (after login)

```
1. Search Users           – Find users by name (or list all)
2. Read Inbox             – View your received messages
3. Send Message           – Send a message to another user
4. Reply to Message       – Reply to a specific inbox message
5. Logout                 – Return to the Login Menu
6. Deregister Account     – Permanently delete your account
```

### Multi-User Usage

Open **separate terminal windows** and run `./chat_app` in each. File locking ensures messages are safely delivered even when multiple instances write to the same inbox simultaneously.

## Project Structure

```
Assignment1/
├── main.c              # Controller — entry point, menu loops
├── auth.c / auth.h     # Registration, login, account deletion
├── user_mgr.c / .h     # List and search users
├── messenger.c / .h    # Send, read inbox, reply (with flock)
├── utils.c / utils.h   # Timestamps, locking, logging helpers
├── Makefile            # Build system (gcc -std=c11)
├── .gitignore          # Excludes data/ from version control
├── README.md           # This file
└── docs/               # Design documentation & diagrams
    ├── DESIGN_DOCUMENT.md
    ├── System Architecture Diagram.drawio
    ├── Data Flow Diagram.drawio
    └── Process Diagram.drawio
```

## Data Files (in `data/`)

| File               | Purpose                                 |
| ------------------ | --------------------------------------- |
| `users.txt`        | Stores `username\|password` credentials |
| `inbox_{user}.txt` | Per-user message inbox                  |
| `system.log`       | Audit trail of all actions              |

## Documentation

See [`docs/DESIGN_DOCUMENT.md`](docs/DESIGN_DOCUMENT.md) for the full system design covering:

- Architectural design & module outline
- Process and algorithm design
- Data/file design
- Concurrency design
- Testing plan

## Build Targets

| Command        | Description                                  |
| -------------- | -------------------------------------------- |
| `make`         | Compile all modules and link into `chat_app` |
| `make clean`   | Remove all `.o` files and the binary         |
| `make rebuild` | Clean + build in one step                    |

## License

This project was developed as a coursework assignment for Network & Distributed Programming.
