# NDP One-on-One Chat Application — System Design Document

> **Course:** Network & Distributed Programming  
> **Date:** March 2026  
> **Language:** C (C11 Standard)  
> **Platform:** Linux (Single Machine, Local Filesystem)

---

## Table of Contents

1. [Architectural Design](#1-architectural-design)
2. [Module Outline](#2-module-outline)
3. [Process Design](#3-process-design)
4. [Algorithm Design](#4-algorithm-design)
5. [Data/File Design](#5-datafile-design)
6. [Concurrency Design](#6-concurrency-design)
7. [Implementation](#7-implementation)
8. [Testing Plan/Report](#8-testing-planreport)

---

## 1. Architectural Design

### 1.1 Overall Architecture

The application follows a **Modular, Layered Architecture** with three distinct tiers:

```
┌─────────────────────────────────────────────────┐
│            Presentation Layer                   │
│               main.c (Controller)               │
│       Login Menu  ←→  Dashboard Menu            │
├────────┬──────────┬──────────────┬──────────────┤
│  Auth  │ User Mgr │  Messenger   │    Utils     │
│ Module │  Module  │   Module     │   Module     │
│ auth.c │user_mgr.c│ messenger.c  │  utils.c     │
├────────┴──────────┴──────────────┴──────────────┤
│              Data Layer (Filesystem)            │
│    users.txt  │  inbox_*.txt  │  system.log     │
└─────────────────────────────────────────────────┘
```

**Design Pattern:** Top-Down Analysis with Modular Decomposition.

- **Presentation Layer** (`main.c`): Handles all user interaction through menu-driven loops. Acts as a pure controller — contains no business logic. Delegates every operation to one of the four functional modules.
- **Business Logic Layer** (four modules): Each module is a self-contained compilation unit with a `.h` (interface) and `.c` (implementation) pair. Modules communicate only through well-defined function signatures.
- **Data Layer** (filesystem): All persistent state is stored in plain-text flat files inside the `data/` directory. No in-memory databases or binary formats are used.

### 1.2 Key Design Principles

| Principle                  | How It Is Applied                                                           |
| -------------------------- | --------------------------------------------------------------------------- |
| **No Global Variables**    | All state is passed via structs and function parameters (pass-by-reference) |
| **Separation of Concerns** | Each module handles exactly one responsibility                              |
| **Encapsulation**          | Modules expose only their header prototypes; internal logic is hidden       |
| **File-Based Persistence** | Data survives between program runs without any external database            |
| **Concurrency Safety**     | File locking (`flock`) prevents race conditions across multiple instances   |

### 1.3 Architectural Diagram Reference

See: [`docs/System Architecture Diagram.drawio`](System%20Architecture%20Diagram.drawio) — visualizes the module hierarchy and data layer connections.

---

## 2. Module Outline

### 2.1 Module Summary

| Module             | File(s)                      | Responsibility                                           |
| ------------------ | ---------------------------- | -------------------------------------------------------- |
| **Controller**     | `main.c`                     | Entry point, menu loops, dispatches to other modules     |
| **Authentication** | `auth.h`, `auth.c`           | User registration, login verification, account deletion  |
| **User Manager**   | `user_mgr.h`, `user_mgr.c`   | Listing and searching registered users                   |
| **Messenger**      | `messenger.h`, `messenger.c` | Sending, reading, and replying to messages               |
| **Utilities**      | `utils.h`, `utils.c`         | Timestamps, string cleaning, file locking, audit logging |

### 2.2 Module Dependency Graph

```
main.c ──────┬──→ auth.c ──────→ utils.c
             │
             ├──→ user_mgr.c ──→ utils.c
             │
             ├──→ messenger.c ─→ utils.c
             │
             └──→ utils.c
```

All business modules depend on `utils.c` for file-path construction, locking, and logging. No module depends on another business module — they only communicate through `main.c`.

### 2.3 Detailed Module Descriptions

#### 2.3.1 Controller Module (`main.c`)

**Purpose:** The single entry point for the application.

**Responsibilities:**

- Display and handle the Login Menu (Register / Login / Exit)
- Display and handle the Dashboard Menu (Search / Inbox / Send / Reply / Logout / Deregister)
- Read user input and dispatch to the appropriate module function
- Never performs any direct file I/O

**Exposed Functions:** `main()` (entry point), plus static helpers for menu display and input handling.

#### 2.3.2 Authentication Module (`auth.c` / `auth.h`)

**Purpose:** Manage user accounts and credentials.

**Data Structures:**

```c
typedef struct {
    char username[MAX_USERNAME];  // 50 chars max
    char password[MAX_PASSWORD];  // 50 chars max
} User;
```

**Functions:**

| Function                                                        | Purpose                                           | Returns                                |
| --------------------------------------------------------------- | ------------------------------------------------- | -------------------------------------- |
| `register_user(const User *user)`                               | Creates account, checks duplicates, creates inbox | 0 success, -1 duplicate, -2 file error |
| `authenticate_user(const char *username, const char *password)` | Validates credentials against `users.txt`         | 0 success, -1 failure                  |
| `deregister_user(const char *username)`                         | Removes account and inbox file                    | 0 success, -1 not found, -2 file error |

#### 2.3.3 User Manager Module (`user_mgr.c` / `user_mgr.h`)

**Purpose:** User discovery — list and search registered users.

**Functions:**

| Function                          | Purpose                                        | Returns                     |
| --------------------------------- | ---------------------------------------------- | --------------------------- |
| `list_users(void)`                | Displays all registered usernames with indices | Count, or -1 on error       |
| `search_users(const char *query)` | Substring search on usernames                  | Match count, or -1 on error |

#### 2.3.4 Messenger Module (`messenger.c` / `messenger.h`)

**Purpose:** Core one-on-one messaging functionality.

**Data Structures:**

```c
typedef struct {
    char sender[MAX_USERNAME];     // 50 chars max
    char timestamp[MAX_TIMESTAMP]; // 30 chars max
    char content[MAX_CONTENT];     // 512 chars max
} Message;
```

**Functions:**

| Function                                   | Purpose                                                | Returns                       |
| ------------------------------------------ | ------------------------------------------------------ | ----------------------------- |
| `send_message(sender, recipient, content)` | Appends message to recipient's inbox with file locking | 0 success, -1 failure         |
| `read_inbox(const char *username)`         | Displays all messages in user's inbox                  | Message count, or -1 on error |
| `reply_message(const char *current_user)`  | Extracts sender from chosen message, sends reply       | 0 success, -1 failure         |

#### 2.3.5 Utilities Module (`utils.c` / `utils.h`)

**Purpose:** Shared helper functions used across all modules.

**Functions:**

| Function                                                        | Purpose                                           |
| --------------------------------------------------------------- | ------------------------------------------------- |
| `get_timestamp(char *buf, size_t len)`                          | Generates `YYYY-MM-DD HH:MM:SS` timestamp         |
| `clean_newline(char *str)`                                      | Strips trailing `\n` from `fgets()` input         |
| `lock_file(int fd)`                                             | Acquires exclusive lock via `flock(LOCK_EX)`      |
| `unlock_file(int fd)`                                           | Releases lock via `flock(LOCK_UN)`                |
| `log_action(const char *username, const char *action)`          | Appends audit entry to `system.log`               |
| `build_inbox_path(const char *username, char *buf, size_t len)` | Constructs inbox filename                         |
| `init_data_dir(void)`                                           | Creates the `data/` directory if it doesn't exist |

**Constants Defined in `utils.h`:**

| Constant        | Value               | Purpose                        |
| --------------- | ------------------- | ------------------------------ |
| `DATA_DIR`      | `"data"`            | Runtime data directory         |
| `USERS_FILE`    | `"data/users.txt"`  | Credentials storage            |
| `LOG_FILE`      | `"data/system.log"` | Audit log                      |
| `INBOX_PREFIX`  | `"data/inbox_"`     | Inbox filename prefix          |
| `INBOX_SUFFIX`  | `".txt"`            | Inbox filename suffix          |
| `MAX_USERNAME`  | `50`                | Maximum username length        |
| `MAX_PASSWORD`  | `50`                | Maximum password length        |
| `MAX_CONTENT`   | `512`               | Maximum message content length |
| `MAX_TIMESTAMP` | `30`                | Timestamp buffer size          |
| `MAX_LINE`      | `1024`              | General line buffer size       |

---

## 3. Process Design

### 3.1 Application Lifecycle

The application follows a **two-phase process** as depicted in the Process Diagram:

```
        ┌──────────┐
        │ Startup  │
        └────┬─────┘
             ▼
     ┌───────────────┐
     │  Login Menu   │◄──────────────────────┐
     │  (Phase 1)    │                       │
     └──┬─────┬──┬───┘                       │
        │     │  │                           │
   Register  Login  Exit                     │
        │     │     │                        │
        │     ▼     ▼                        │
        │  Success? ──No──→ (retry)          │
        │     │                              │
        │    Yes                             │
        │     ▼                              │
        │  ┌──────────────────┐              │
        │  │   Dashboard      │              │
        │  │   (Phase 2)      │              │
        │  │                  │              │
        │  │  1. Search Users │              │
        │  │  2. Read Inbox   │              │
        │  │  3. Send Message │              │
        │  │  4. Reply to Msg │              │
        │  │  5. Logout ──────┼──────────────┘
        │  │  6. Deregister ──┼──────────────┘
        │  └──────────────────┘
        │
        └──→ (back to Login Menu)
```

### 3.2 Phase 1: Login Menu Process

**Flow Control:** `while` loop with `switch` statement.

```
WHILE running:
    DISPLAY login menu (Register / Login / Exit)
    READ choice
    SWITCH choice:
        CASE 1: → handle_registration()
        CASE 2: → handle_login()
                   IF success: → run_dashboard()
        CASE 3: → SET running = false
        DEFAULT: → print error, continue
```

### 3.3 Phase 2: Dashboard Process

**Flow Control:** `while` loop with `switch` statement.

```
WHILE active:
    DISPLAY dashboard menu
    READ choice
    SWITCH choice:
        CASE 1: → READ query
                   IF empty: list_users()
                   ELSE:     search_users(query)
        CASE 2: → read_inbox(current_user)
        CASE 3: → READ recipient, content
                   send_message(current_user, recipient, content)
        CASE 4: → reply_message(current_user)
        CASE 5: → log_action(), SET active = false (Logout)
        CASE 6: → CONFIRM, deregister_user(), SET active = false
```

### 3.4 Process Diagram Reference

See: [`docs/Process Diagram.drawio`](Process%20Diagram.drawio) — visualizes the full state machine including Login, Registration, Authentication, Dashboard states (Idle, SearchUsers, ViewInbox, SendMessage, ReplyToMsg, Logout).

---

## 4. Algorithm Design

### 4.1 `register_user(const User *user)`

```
ALGORITHM: Register User
INPUT: User struct (username, password)
OUTPUT: 0 = success, -1 = duplicate, -2 = file error

Phase 1 — Duplicate Check:
    1. OPEN users.txt for reading
    2. FOR EACH line in file:
        a. EXTRACT username (substring before '|')
        b. IF username == new_user.username:
             RETURN -1 (duplicate)
    3. CLOSE file

Phase 2 — Account Creation:
    4. OPEN users.txt in APPEND mode
    5. WRITE "username|password\n"
    6. CLOSE file

Phase 3 — Inbox Initialization:
    7. CONSTRUCT path: "data/inbox_{username}.txt"
    8. CREATE empty file at that path
    9. CLOSE file

Phase 4 — Audit:
    10. CALL log_action(username, "registered a new account")
    11. RETURN 0
```

### 4.2 `authenticate_user(const char *username, const char *password)`

```
ALGORITHM: Authenticate User
INPUT: username, password (both strings)
OUTPUT: 0 = match found, -1 = no match

    1. OPEN users.txt for reading
    2. IF file not found: RETURN -1
    3. FOR EACH line in file:
        a. STRIP newline
        b. FIND '|' delimiter
        c. EXTRACT stored_username (before '|')
        d. EXTRACT stored_password (after '|')
        e. IF stored_username == username AND stored_password == password:
             LOG "logged in"
             RETURN 0
    4. CLOSE file
    5. RETURN -1 (no match found)
```

### 4.3 `deregister_user(const char *username)`

```
ALGORITHM: Deregister User
INPUT: username (string)
OUTPUT: 0 = success, -1 = not found, -2 = file error

Phase 1 — Filter:
    1. OPEN users.txt for reading (input)
    2. OPEN users_tmp.txt for writing (output)
    3. SET found = false
    4. FOR EACH line in input:
        a. EXTRACT stored_username
        b. IF stored_username == username:
             SET found = true
             SKIP this line (do NOT write to output)
        c. ELSE:
             WRITE line to output
    5. CLOSE both files

Phase 2 — Replace:
    6. IF NOT found: DELETE temp file, RETURN -1
    7. DELETE users.txt
    8. RENAME users_tmp.txt → users.txt

Phase 3 — Cleanup:
    9. DELETE inbox_{username}.txt

Phase 4 — Audit:
    10. LOG "deregistered account"
    11. RETURN 0
```

### 4.4 `send_message(const char *sender, const char *recipient, const char *content)`

```
ALGORITHM: Send Message (with File Locking)
INPUT: sender, recipient, content (strings)
OUTPUT: 0 = success, -1 = failure

    1. CONSTRUCT path: "data/inbox_{recipient}.txt"
    2. OPEN file with open() using O_WRONLY | O_CREAT | O_APPEND
    3. IF open fails: RETURN -1 (recipient not found)
    4. CALL lock_file(fd)          ← ACQUIRE exclusive lock
    5. WRAP fd in FILE* via fdopen()
    6. GENERATE timestamp
    7. WRITE "[timestamp] From: sender\ncontent\n---\n"
    8. FLUSH stream
    9. CALL unlock_file(fd)        ← RELEASE lock
    10. CLOSE file (fclose)
    11. LOG "sent message to recipient"
    12. RETURN 0
```

### 4.5 `read_inbox(const char *username)`

```
ALGORITHM: Read Inbox
INPUT: username (string)
OUTPUT: message count, or -1 on error

    1. CONSTRUCT path: "data/inbox_{username}.txt"
    2. OPEN file for reading
    3. SET msg_count = 0
    4. FOR EACH line in file:
        a. IF line starts with '[':
             INCREMENT msg_count
             PRINT "--- Message #N ---"
             PRINT the header line (timestamp + sender)
        b. ELSE IF line == "---":
             SKIP (delimiter)
        c. ELSE IF line is non-empty:
             PRINT as message body
    5. IF msg_count == 0: PRINT "inbox is empty"
    6. RETURN msg_count
```

### 4.6 `reply_message(const char *current_user)`

```
ALGORITHM: Reply to Message
INPUT: current_user (string)
OUTPUT: 0 = success, -1 = failure

Phase 1 — Display:
    1. CALL read_inbox(current_user) → msg_count
    2. IF msg_count <= 0: RETURN -1

Phase 2 — Selection:
    3. PROMPT for message number (1 to msg_count)
    4. VALIDATE input range

Phase 3 — Sender Extraction:
    5. RE-OPEN inbox file
    6. SET current_msg = 0
    7. FOR EACH line in file:
        a. IF line starts with '[':
             INCREMENT current_msg
             IF current_msg == target_msg:
                 FIND "From: " substring
                 EXTRACT sender name (everything after "From: ")
                 BREAK
    8. CLOSE file

Phase 4 — Reply:
    9. PROMPT for reply content
    10. CALL send_message(current_user, sender, reply_content)
    11. RETURN result
```

### 4.7 `list_users()` and `search_users(const char *query)`

```
ALGORITHM: List Users
    1. OPEN users.txt
    2. FOR EACH line: EXTRACT username, PRINT with index
    3. RETURN count

ALGORITHM: Search Users
    1. OPEN users.txt
    2. FOR EACH line:
        a. EXTRACT username
        b. IF strstr(username, query) != NULL:
             PRINT match with index
    3. RETURN match count
```

### 4.8 `lock_file(int fd)` / `unlock_file(int fd)`

```
ALGORITHM: Lock File
    1. CALL flock(fd, LOCK_EX)     ← blocks until lock acquired
    2. IF error: RETURN -1
    3. RETURN 0

ALGORITHM: Unlock File
    1. CALL flock(fd, LOCK_UN)
    2. IF error: RETURN -1
    3. RETURN 0
```

### 4.9 `log_action(const char *username, const char *action)`

```
ALGORITHM: Log Action
    1. GENERATE timestamp
    2. OPEN system.log with open() (O_WRONLY | O_CREAT | O_APPEND)
    3. ACQUIRE exclusive lock (lock_file)
    4. WRAP fd in FILE* (fdopen)
    5. WRITE "[timestamp] [username] action\n"
    6. FLUSH, UNLOCK, CLOSE
```

---

## 5. Data/File Design

### 5.1 File Storage Overview

All runtime data is stored in the `data/` directory (created automatically at startup). This directory is excluded from version control via `.gitignore`.

```
data/
├── users.txt               # User credentials store
├── inbox_alice.txt          # Alice's message inbox
├── inbox_bob.txt            # Bob's message inbox
├── inbox_{username}.txt     # One file per registered user
└── system.log               # Audit trail
```

### 5.2 `users.txt` — Credentials Store

**Format:** One line per user, pipe-delimited.

```
username|password
```

**Example:**

```
alice|mypassword123
bob|securepass456
charlie|pass789
```

**Operations:**

- **Register:** Append a new line
- **Login:** Sequential scan, compare both fields
- **Deregister:** Copy all lines except target to temp file, then replace

### 5.3 `inbox_{username}.txt` — Message Inbox

**Format:** Messages are stored as blocks separated by `---`:

```
[YYYY-MM-DD HH:MM:SS] From: sender_username
message content text here
---
```

**Example:**

```
[2026-03-06 15:30:00] From: bob
Hey Alice, how are you?
---
[2026-03-06 15:35:00] From: charlie
Don't forget the meeting tomorrow!
---
```

**Operations:**

- **Send:** Append a new message block (with file locking)
- **Read:** Parse blocks by detecting `[` header lines and `---` delimiters
- **Reply:** Re-parse to extract sender from the Nth message header

**Lifecycle:**

- Created (empty) when a user registers
- Deleted when a user deregisters

### 5.4 `system.log` — Audit Trail

**Format:** Timestamped entries with the acting user:

```
[YYYY-MM-DD HH:MM:SS] [username] action_description
```

**Example:**

```
[2026-03-06 15:00:00] [SYSTEM] Application started
[2026-03-06 15:00:05] [alice] registered a new account
[2026-03-06 15:00:10] [bob] registered a new account
[2026-03-06 15:01:00] [alice] logged in
[2026-03-06 15:01:30] [alice] sent message to bob
[2026-03-06 15:02:00] [alice] logged out
[2026-03-06 15:03:00] [SYSTEM] Application terminated
```

**Logged Events:** Registration, login, message sent, logout, deregistration, application start/stop.

### 5.5 Data Structure Definitions

```c
/* User credential record */
typedef struct {
    char username[50];
    char password[50];
} User;

/* Message record */
typedef struct {
    char sender[50];
    char timestamp[30];
    char content[512];
} Message;
```

---

## 6. Concurrency Design

### 6.1 Why Concurrency Control Is Required

This application is designed to run as **multiple independent instances on the same machine**. Two or more users can each run `./chat_app` in separate terminal windows simultaneously. When both users send a message to the same recipient at the same time, their writes target the **same inbox file**, creating a potential **race condition**.

Without locking, two concurrent writes could:

- Interleave partial message blocks, corrupting the file
- Overwrite each other's data
- Result in lost messages

### 6.2 Concurrency Mechanism: `flock()`

The application uses **POSIX file locking** via `flock()` from `<sys/file.h>`.

**Lock Types Used:**
| Lock | Flag | Purpose |
|---|---|---|
| Exclusive Lock | `LOCK_EX` | Blocks other processes from reading/writing the file |
| Unlock | `LOCK_UN` | Releases the held lock |

### 6.3 Where Locking Is Applied

| Operation        | File Locked             | Lock Type | Rationale                                                |
| ---------------- | ----------------------- | --------- | -------------------------------------------------------- |
| `send_message()` | `inbox_{recipient}.txt` | `LOCK_EX` | Prevents two senders from corrupting the same inbox      |
| `log_action()`   | `system.log`            | `LOCK_EX` | Prevents interleaved log entries from multiple instances |

### 6.4 Locking Sequence Diagram

```
Process A (alice sends to bob)       Process B (charlie sends to bob)
────────────────────────────────     ────────────────────────────────
1. open("data/inbox_bob.txt")        1. open("data/inbox_bob.txt")
2. flock(fd, LOCK_EX) → acquired    2. flock(fd, LOCK_EX) → BLOCKED
3. write(message_A)                        (waiting for lock...)
4. fflush()                                (waiting for lock...)
5. flock(fd, LOCK_UN) → released     3. flock(fd, LOCK_EX) → acquired
6. close(fd)                         4. write(message_B)
                                     5. fflush()
                                     6. flock(fd, LOCK_UN) → released
                                     7. close(fd)

Result: inbox_bob.txt contains message_A followed by message_B (no corruption)
```

### 6.5 Design Decisions

| Decision                    | Rationale                                                                                                                     |
| --------------------------- | ----------------------------------------------------------------------------------------------------------------------------- |
| `flock()` over `fcntl()`    | `flock()` is simpler and sufficient — we lock entire files, not byte ranges                                                   |
| Blocking locks (`LOCK_EX`)  | We want writers to wait, not fail — messages should never be dropped                                                          |
| Lock at write points only   | Read operations (`read_inbox`, `list_users`) do not lock, because append-only writes are inherently safe to read concurrently |
| `O_APPEND` flag on `open()` | Even without locking, `O_APPEND` ensures the OS moves the write pointer to the end atomically                                 |

---

## 7. Implementation

### 7.1 Project File Structure

```
Chat_App(1on1)/
├── main.c              # Controller — entry point, menu loops
├── auth.c              # Authentication logic
├── auth.h              # Auth module interface
├── user_mgr.c          # User discovery logic
├── user_mgr.h          # User manager interface
├── messenger.c         # Messaging logic with file locking
├── messenger.h         # Messenger interface
├── utils.c             # Shared utility functions
├── utils.h             # Utility interface + constants
├── Makefile            # Build system
├── .gitignore          # Excludes data/ and build artifacts
├── README.md           # Setup & run guide
├── docs/               # Design documentation & diagrams
│   ├── DESIGN_DOCUMENT.md
│   ├── System Architecture Diagram.drawio
│   ├── Data Flow Diagram.drawio
│   └── Process Diagram.drawio
└── data/               # Runtime data (auto-created, gitignored)
    ├── users.txt
    ├── inbox_*.txt
    └── system.log
```

### 7.2 Build System

**Compiler:** GCC  
**Standard:** C11 (`-std=c11`)  
**POSIX Compliance:** `-D_POSIX_C_SOURCE=200809L` (required for `flock`, `fdopen`)  
**Warnings:** `-Wall -Wextra -pedantic`

**Build Command:** `make` (or `make clean && make` for a full rebuild)

**Compilation produces zero errors and zero warnings.**

### 7.3 Compilation Dependency Model

```
main.o     ← main.c     + auth.h, user_mgr.h, messenger.h, utils.h
auth.o     ← auth.c     + auth.h, utils.h
user_mgr.o ← user_mgr.c + user_mgr.h, utils.h
messenger.o← messenger.c+ messenger.h, utils.h
utils.o    ← utils.c    + utils.h

chat_app   ← main.o + auth.o + user_mgr.o + messenger.o + utils.o
```

### 7.4 Key Implementation Details

**Input Handling:**

- `scanf()` for numeric menu choices, followed by clearing the stdin buffer
- `fgets()` for string input (username, password, messages) — prevents buffer overflows
- `clean_newline()` strips the trailing `\n` from all `fgets()` input

**Error Handling:**

- All `fopen()`/`open()` calls check for `NULL`/`-1` and report errors via `perror()`
- Functions return negative error codes; the caller decides how to handle them
- Invalid menu selections re-prompt the user without crashing

**Memory Management:**

- All buffers are stack-allocated (no `malloc`/`free` required)
- Fixed-size buffers prevent heap fragmentation and memory leaks

---

## 8. Testing Plan/Report

### 8.1 Testing Strategy

Testing is divided into three categories:

| Category                | Scope                            | Method                                    |
| ----------------------- | -------------------------------- | ----------------------------------------- |
| **Build Verification**  | Compilation correctness          | `make clean && make` with strict warnings |
| **Functional Testing**  | All features work correctly      | Manual interactive testing                |
| **Concurrency Testing** | File locking prevents corruption | Two simultaneous instances                |

### 8.2 Build Verification

| Test        | Command              | Expected Result                                        | Status  |
| ----------- | -------------------- | ------------------------------------------------------ | ------- |
| Clean build | `make clean && make` | Zero errors, zero warnings, `chat_app` binary produced | ✅ PASS |
| Rebuild     | `make rebuild`       | Same as above                                          | ✅ PASS |

### 8.3 Functional Test Cases

#### TC-01: User Registration

| Step | Action                                       | Expected Result                    |
| ---- | -------------------------------------------- | ---------------------------------- |
| 1    | Select "Register" from Login Menu            | Prompted for username and password |
| 2    | Enter username: `alice`, password: `pass123` | "Registration successful!" printed |
| 3    | Verify `data/users.txt`                      | Contains `alice\|pass123`          |
| 4    | Verify `data/inbox_alice.txt`                | Empty file exists                  |
| 5    | Verify `data/system.log`                     | Contains registration entry        |

#### TC-02: Duplicate Registration Prevention

| Step | Action                               | Expected Result                           |
| ---- | ------------------------------------ | ----------------------------------------- |
| 1    | Register with username `alice` again | "Error: Username 'alice' already exists." |

#### TC-03: User Authentication (Success)

| Step | Action                                    | Expected Result                           |
| ---- | ----------------------------------------- | ----------------------------------------- |
| 1    | Select "Login", enter `alice` / `pass123` | "Login successful!" — Dashboard displayed |

#### TC-04: User Authentication (Failure)

| Step | Action                            | Expected Result                        |
| ---- | --------------------------------- | -------------------------------------- |
| 1    | Login with `alice` / `wrongpass`  | "Error: Invalid username or password." |
| 2    | Login with `nonexistent` / `pass` | Same error message                     |

#### TC-05: List and Search Users

| Step | Action                                           | Expected Result                  |
| ---- | ------------------------------------------------ | -------------------------------- |
| 1    | Register `bob` and `charlie`                     | Both accounts created            |
| 2    | Dashboard → Search (press Enter for empty query) | All 3 users listed               |
| 3    | Search with query `al`                           | Only `alice` shown               |
| 4    | Search with query `xyz`                          | "No users matching 'xyz' found." |

#### TC-06: Send Message

| Step | Action                                        | Expected Result                            |
| ---- | --------------------------------------------- | ------------------------------------------ |
| 1    | Login as `alice`, select "Send Message"       | Prompted for recipient                     |
| 2    | Enter recipient: `bob`, message: `Hello Bob!` | "Message sent to 'bob' successfully."      |
| 3    | Verify `data/inbox_bob.txt`                   | Contains message with timestamp and sender |

#### TC-07: Read Inbox

| Step | Action                                     | Expected Result                 |
| ---- | ------------------------------------------ | ------------------------------- |
| 1    | Login as `bob`, select "Read Inbox"        | Message #1 from alice displayed |
| 2    | Login as `alice` (no messages sent to her) | "Your inbox is empty."          |

#### TC-08: Reply to Message

| Step | Action                                    | Expected Result                         |
| ---- | ----------------------------------------- | --------------------------------------- |
| 1    | Login as `bob`, select "Reply to Message" | Inbox displayed with numbered messages  |
| 2    | Select message #1                         | "Replying to: alice" shown              |
| 3    | Enter reply: `Hi Alice!`                  | "Message sent to 'alice' successfully." |
| 4    | Login as `alice`, read inbox              | Reply from bob displayed                |

#### TC-09: Self-Message Prevention

| Step | Action                                    | Expected Result                                 |
| ---- | ----------------------------------------- | ----------------------------------------------- |
| 1    | Login as `alice`, send message to `alice` | "Error: You cannot send a message to yourself." |

#### TC-10: Logout and Re-Login

| Step | Action                            | Expected Result                               |
| ---- | --------------------------------- | --------------------------------------------- |
| 1    | Select "Logout" from Dashboard    | "Logged out successfully." — Login Menu shown |
| 2    | Login again with same credentials | Dashboard accessible again                    |

#### TC-11: Account Deregistration

| Step | Action                                          | Expected Result                        |
| ---- | ----------------------------------------------- | -------------------------------------- |
| 1    | Login as `charlie`, select "Deregister Account" | Confirmation prompt shown              |
| 2    | Enter `y`                                       | "Account 'charlie' has been deleted."  |
| 3    | Verify `data/users.txt`                         | `charlie` entry removed                |
| 4    | Verify `data/inbox_charlie.txt`                 | File deleted                           |
| 5    | Attempt to login as `charlie`                   | "Error: Invalid username or password." |

#### TC-12: Audit Log Verification

| Step | Action                                           | Expected Result                   |
| ---- | ------------------------------------------------ | --------------------------------- |
| 1    | After all above tests, inspect `data/system.log` | All events chronologically logged |

### 8.4 Concurrency Test

| Step | Action                                                   | Expected Result                                              |
| ---- | -------------------------------------------------------- | ------------------------------------------------------------ |
| 1    | Open two terminal windows                                | Two independent sessions                                     |
| 2    | Run `./chat_app` in both terminals                       | Both instances start independently                           |
| 3    | Login as `alice` in Terminal 1, `bob` in Terminal 2      | Both logged in                                               |
| 4    | Simultaneously send messages from both to user `charlie` | Both messages successfully appended                          |
| 5    | Inspect `data/inbox_charlie.txt`                         | Both messages present, no corruption, clean block boundaries |
| 6    | Inspect `data/system.log`                                | Both send events logged without interleaving                 |

### 8.5 Test Results Summary

| Category           | Tests | Passed | Failed |
| ------------------ | ----- | ------ | ------ |
| Build Verification | 2     | 2      | 0      |
| Functional         | 0     | —      | —      |
| Concurrency        | 0     | —      | —      |
| **Total**          | **2** | —      | —      |

> **Note:** Functional and concurrency tests are designed for manual execution by the tester. Run each test case sequentially and mark the result.

---

_End of Design Document_
