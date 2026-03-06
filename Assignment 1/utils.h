/*
 * utils.h - Utility Module Header
 * 
 * Provides helper functions for:
 *   - Timestamp generation
 *   - String sanitization (newline removal)
 *   - File locking/unlocking (using flock for concurrency safety)
 *   - Audit logging to system.log
 */

#ifndef UTILS_H
#define UTILS_H

#include <stddef.h>

/* ---------- File Paths (Central Definitions) ---------- */
#define DATA_DIR      "data"
#define USERS_FILE    "data/users.txt"
#define LOG_FILE      "data/system.log"
#define INBOX_PREFIX  "data/inbox_"
#define INBOX_SUFFIX  ".txt"

/* ---------- Buffer Size Constants ---------- */
#define MAX_USERNAME  50
#define MAX_PASSWORD  50
#define MAX_CONTENT   512
#define MAX_TIMESTAMP 30
#define MAX_LINE      1024

/*
 * get_timestamp - Fills the provided buffer with the current date/time.
 *
 * Algorithm:
 *   1. Call time() to get the current epoch time.
 *   2. Convert to local time with localtime().
 *   3. Format into "YYYY-MM-DD HH:MM:SS" using strftime().
 *
 * Parameters:
 *   buf - character buffer to store the formatted timestamp
 *   len - size of the buffer
 */
void get_timestamp(char *buf, size_t len);

/*
 * clean_newline - Removes the trailing newline character from a string.
 *
 * Algorithm:
 *   1. Find the length of the string with strlen().
 *   2. If the last character is '\n', replace it with '\0'.
 *
 * Parameters:
 *   str - the string to sanitize (modified in-place)
 */
void clean_newline(char *str);

/*
 * lock_file - Acquires an exclusive lock on an open file descriptor.
 *
 * Algorithm:
 *   Uses flock() with LOCK_EX to obtain an exclusive (write) lock.
 *   This call blocks until the lock is available.
 *
 * Parameters:
 *   fd - the file descriptor to lock
 *
 * Returns:
 *   0 on success, -1 on failure
 */
int lock_file(int fd);

/*
 * unlock_file - Releases the lock on an open file descriptor.
 *
 * Algorithm:
 *   Uses flock() with LOCK_UN to release the lock.
 *
 * Parameters:
 *   fd - the file descriptor to unlock
 *
 * Returns:
 *   0 on success, -1 on failure
 */
int unlock_file(int fd);

/*
 * log_action - Appends an audit entry to the system.log file.
 *
 * Algorithm:
 *   1. Generate a timestamp via get_timestamp().
 *   2. Open system.log in append mode with file locking.
 *   3. Write the entry in format: [timestamp] [username] action
 *   4. Unlock and close.
 *
 * Parameters:
 *   username - the user performing the action (or "SYSTEM")
 *   action   - a description of the action taken
 */
void log_action(const char *username, const char *action);

/*
 * build_inbox_path - Constructs the inbox filename for a given username.
 *
 * Algorithm:
 *   Concatenates INBOX_PREFIX + username + INBOX_SUFFIX into the buffer.
 *
 * Parameters:
 *   username - the target username
 *   buf      - output buffer for the resulting path
 *   len      - size of the output buffer
 */
void build_inbox_path(const char *username, char *buf, size_t len);

/*
 * init_data_dir - Creates the data/ directory if it does not exist.
 *
 * Must be called once at application startup before any file operations.
 */
void init_data_dir(void);

#endif /* UTILS_H */
