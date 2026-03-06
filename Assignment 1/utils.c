/*
 * utils.c - Utility Module Implementation
 *
 * Implements helper functions for timestamping, string cleaning,
 * file locking (using flock), and audit logging.
 *
 * Dependencies: <stdio.h>, <string.h>, <time.h>, <sys/file.h>, <unistd.h>
 */

#include <stdio.h>
#include <string.h>
#include <time.h>
#include <sys/file.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>

#include "utils.h"

/*
 * get_timestamp - Generates a formatted timestamp string.
 *
 * Algorithm Design:
 *   Step 1: Retrieve the current calendar time using time().
 *   Step 2: Convert the time_t value to a broken-down local time struct.
 *   Step 3: Format the struct into "YYYY-MM-DD HH:MM:SS" via strftime().
 *   Step 4: Store the result in the caller-provided buffer.
 */
void get_timestamp(char *buf, size_t len) {
    time_t now;
    struct tm *tm_info;

    time(&now);                            /* Step 1: get epoch time           */
    tm_info = localtime(&now);             /* Step 2: convert to local time    */
    strftime(buf, len, "%Y-%m-%d %H:%M:%S", tm_info); /* Step 3: format       */
}

/*
 * clean_newline - Strips a trailing newline from a string in-place.
 *
 * Algorithm Design:
 *   Step 1: Compute the string length.
 *   Step 2: If the last character is '\n', overwrite it with '\0'.
 *   This is essential after fgets() calls which retain the newline.
 */
void clean_newline(char *str) {
    size_t len = strlen(str);

    if (len > 0 && str[len - 1] == '\n') {
        str[len - 1] = '\0';              /* Replace newline with terminator  */
    }
}

/*
 * lock_file - Acquires an exclusive (blocking) lock on a file descriptor.
 *
 * Algorithm Design:
 *   Uses the POSIX flock() system call with LOCK_EX (exclusive lock).
 *   The call blocks until the lock becomes available, ensuring that
 *   concurrent processes (multiple chat_app instances) do not write
 *   to the same inbox file simultaneously.
 *
 * Concurrency Note:
 *   This is the primary mechanism preventing race conditions when
 *   two users send messages to the same recipient at the same time.
 */
int lock_file(int fd) {
    if (flock(fd, LOCK_EX) == -1) {
        perror("Error: Failed to acquire file lock");
        return -1;
    }
    return 0;
}

/*
 * unlock_file - Releases the lock on a file descriptor.
 *
 * Algorithm Design:
 *   Calls flock() with LOCK_UN to release any held lock.
 *   Must be called after every successful lock_file() to prevent deadlocks.
 */
int unlock_file(int fd) {
    if (flock(fd, LOCK_UN) == -1) {
        perror("Error: Failed to release file lock");
        return -1;
    }
    return 0;
}

/*
 * log_action - Appends a timestamped audit entry to the system log.
 *
 * Algorithm Design:
 *   Step 1: Generate a timestamp.
 *   Step 2: Open system.log with open() to get a file descriptor for locking.
 *   Step 3: Acquire an exclusive lock to prevent interleaved log writes.
 *   Step 4: Convert fd to FILE* with fdopen() for formatted writing.
 *   Step 5: Write the log entry in format: [timestamp] [username] action
 *   Step 6: Flush, unlock, and close.
 */
void log_action(const char *username, const char *action) {
    char timestamp[MAX_TIMESTAMP];
    int fd;
    FILE *fp;

    get_timestamp(timestamp, sizeof(timestamp));          /* Step 1 */

    /* Step 2: Open with O_APPEND to ensure atomic append positioning */
    fd = open(LOG_FILE, O_WRONLY | O_CREAT | O_APPEND, 0644);
    if (fd == -1) {
        perror("Error: Cannot open system.log");
        return;
    }

    /* Step 3: Lock the log file to prevent concurrent write corruption */
    if (lock_file(fd) == -1) {
        close(fd);
        return;
    }

    /* Step 4: Wrap fd in FILE* for fprintf convenience */
    fp = fdopen(fd, "a");
    if (fp == NULL) {
        perror("Error: fdopen failed on system.log");
        unlock_file(fd);
        close(fd);
        return;
    }

    /* Step 5: Write the formatted log entry */
    fprintf(fp, "[%s] [%s] %s\n", timestamp, username, action);

    /* Step 6: Flush, unlock, close (fclose closes the underlying fd too) */
    fflush(fp);
    unlock_file(fd);
    fclose(fp);
}

/*
 * build_inbox_path - Constructs the inbox filename for a given user.
 *
 * Algorithm Design:
 *   Concatenates "inbox_" + username + ".txt" using snprintf for safety.
 *   Example: username "alice" → "inbox_alice.txt"
 */
void build_inbox_path(const char *username, char *buf, size_t len) {
    snprintf(buf, len, "%s%s%s", INBOX_PREFIX, username, INBOX_SUFFIX);
}

/*
 * init_data_dir - Creates the data/ directory if it does not already exist.
 *
 * Algorithm Design:
 *   Uses mkdir() with 0755 permissions. If the directory already exists,
 *   errno will be EEXIST, which is silently ignored.
 *   Must be called once at startup before any file read/write operations.
 */
void init_data_dir(void) {
    if (mkdir(DATA_DIR, 0755) == -1 && errno != EEXIST) {
        perror("Error: Cannot create data directory");
    }
}
