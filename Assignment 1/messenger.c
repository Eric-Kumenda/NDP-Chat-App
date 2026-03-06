/*
 * messenger.c - Messaging Module Implementation
 *
 * Implements the core messaging functionality: sending messages with
 * file locking for concurrency safety, reading inbox contents, and
 * replying to specific messages.
 *
 * File Format for inbox_{username}.txt:
 *   [YYYY-MM-DD HH:MM:SS] From: sender_username
 *   message content here
 *   ---
 *
 * Dependencies: <stdio.h>, <string.h>, <fcntl.h>, <unistd.h>, <sys/file.h>
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/file.h>

#include "messenger.h"
#include "utils.h"

/*
 * send_message - Sends a message from one user to another.
 *
 * Algorithm Design (Top-Down):
 *   Step 1: Build the recipient's inbox file path.
 *   Step 2: Open the file using open() (low-level) for fd-based locking.
 *   Step 3: Acquire an exclusive lock via lock_file(fd).
 *           — This is CRITICAL for concurrency: if two users send messages
 *             to the same recipient simultaneously, the lock ensures only
 *             one write completes at a time.
 *   Step 4: Convert the fd to a FILE* stream using fdopen() for fprintf.
 *   Step 5: Generate a timestamp for the message.
 *   Step 6: Write the formatted message block to the file.
 *   Step 7: Flush the stream, release the lock, and close.
 *   Step 8: Log the send action to system.log.
 */
int send_message(const char *sender, const char *recipient, const char *content) {
    char inbox_path[MAX_LINE];
    char timestamp[MAX_TIMESTAMP];
    char log_msg[MAX_LINE];
    int fd;
    FILE *fp;

    /* Step 1: Construct inbox path, e.g., "inbox_bob.txt" */
    build_inbox_path(recipient, inbox_path, sizeof(inbox_path));

    /* Step 2: Open with O_APPEND for safe concurrent appending */
    fd = open(inbox_path, O_WRONLY | O_CREAT | O_APPEND, 0644);
    if (fd == -1) {
        printf("Error: Recipient '%s' not found or inbox inaccessible.\n",
               recipient);
        return -1;
    }

    /* Step 3: Acquire exclusive file lock (blocks until available) */
    if (lock_file(fd) == -1) {
        close(fd);
        return -1;
    }

    /* Step 4: Wrap fd in FILE* for formatted output */
    fp = fdopen(fd, "a");
    if (fp == NULL) {
        perror("Error: fdopen failed");
        unlock_file(fd);
        close(fd);
        return -1;
    }

    /* Step 5: Generate the current timestamp */
    get_timestamp(timestamp, sizeof(timestamp));

    /* Step 6: Write the message block in the defined format */
    fprintf(fp, "[%s] From: %s\n%s\n---\n", timestamp, sender, content);

    /* Step 7: Flush, unlock, close */
    fflush(fp);
    unlock_file(fd);
    fclose(fp);  /* Also closes the underlying fd */

    /* Step 8: Audit log */
    snprintf(log_msg, sizeof(log_msg), "sent message to %s", recipient);
    log_action(sender, log_msg);

    printf("Message sent to '%s' successfully.\n", recipient);
    return 0;
}

/*
 * read_inbox - Reads and displays all messages in a user's inbox.
 *
 * Algorithm Design:
 *   Step 1: Build the inbox file path.
 *   Step 2: Open the file for reading.
 *   Step 3: Initialize a message counter.
 *   Step 4: Read line by line (while loop iteration).
 *     4a. If the line starts with '[', it's a message header — increment counter
 *         and print a separator with the message number.
 *     4b. If the line is "---", it's a message delimiter — print a blank line.
 *     4c. Otherwise, it's message content — print as-is.
 *   Step 5: If no messages were found, inform the user.
 *   Step 6: Return the message count.
 */
int read_inbox(const char *username) {
    char inbox_path[MAX_LINE];
    char line[MAX_LINE];
    FILE *fp;
    int msg_count = 0;

    /* Step 1-2: Build path and open */
    build_inbox_path(username, inbox_path, sizeof(inbox_path));
    fp = fopen(inbox_path, "r");
    if (fp == NULL) {
        printf("Error: Could not open your inbox.\n");
        return -1;
    }

    printf("\n============ Inbox for %s ============\n", username);

    /* Step 4: Parse and display messages */
    while (fgets(line, sizeof(line), fp) != NULL) {
        clean_newline(line);

        /* Step 4a: Detect message header lines starting with '[' */
        if (line[0] == '[') {
            msg_count++;
            printf("\n--- Message #%d ---\n", msg_count);
            printf("  %s\n", line);  /* Print the header with timestamp */
        }
        /* Step 4b: Message delimiter */
        else if (strcmp(line, "---") == 0) {
            /* Visual separator already printed by header detection */
        }
        /* Step 4c: Message body content */
        else if (strlen(line) > 0) {
            printf("  %s\n", line);
        }
    }

    fclose(fp);

    /* Step 5: Handle empty inbox */
    if (msg_count == 0) {
        printf("  Your inbox is empty.\n");
    }

    printf("==========================================\n");
    printf("Total messages: %d\n", msg_count);

    return msg_count;
}

/*
 * reply_message - Replies to a specific message in the user's inbox.
 *
 * Algorithm Design (Top-Down):
 *   Phase 1 - Display:
 *     1a. Call read_inbox() to show all messages with indices.
 *     1b. If inbox is empty, return early.
 *
 *   Phase 2 - Selection:
 *     2a. Prompt the user to enter a message number to reply to.
 *     2b. Validate the input is within range.
 *
 *   Phase 3 - Sender Extraction:
 *     3a. Re-open the inbox file.
 *     3b. Iterate through message headers, counting until we reach
 *         the selected message number.
 *     3c. Parse the "From: sender_name" portion of the header line.
 *
 *   Phase 4 - Reply:
 *     4a. Prompt for the reply content.
 *     4b. Call send_message() with the extracted sender as recipient.
 */
int reply_message(const char *current_user) {
    char inbox_path[MAX_LINE];
    char line[MAX_LINE];
    char sender[MAX_USERNAME];
    char reply_content[MAX_CONTENT];
    FILE *fp;
    int msg_count, target_msg, current_msg = 0;

    /* --- Phase 1: Display inbox --- */
    msg_count = read_inbox(current_user);
    if (msg_count <= 0) {
        printf("No messages to reply to.\n");
        return -1;
    }

    /* --- Phase 2: Get user selection --- */
    printf("\nEnter message number to reply to (1-%d): ", msg_count);
    if (scanf("%d", &target_msg) != 1 || target_msg < 1 ||
        target_msg > msg_count) {
        printf("Error: Invalid message number.\n");
        /* Clear stdin buffer */
        while (getchar() != '\n');
        return -1;
    }
    /* Clear the newline left by scanf */
    while (getchar() != '\n');

    /* --- Phase 3: Extract sender from the selected message --- */
    build_inbox_path(current_user, inbox_path, sizeof(inbox_path));
    fp = fopen(inbox_path, "r");
    if (fp == NULL) {
        printf("Error: Could not re-open inbox.\n");
        return -1;
    }

    sender[0] = '\0';
    while (fgets(line, sizeof(line), fp) != NULL) {
        clean_newline(line);

        /* Count message headers to find the target */
        if (line[0] == '[') {
            current_msg++;
            if (current_msg == target_msg) {
                /*
                 * Parse sender from header: "[timestamp] From: sender_name"
                 * Find "From: " and extract everything after it.
                 */
                char *from_ptr = strstr(line, "From: ");
                if (from_ptr != NULL) {
                    strncpy(sender, from_ptr + 6, MAX_USERNAME - 1);
                    sender[MAX_USERNAME - 1] = '\0';
                }
                break;
            }
        }
    }
    fclose(fp);

    /* Validate that we extracted a sender */
    if (strlen(sender) == 0) {
        printf("Error: Could not determine the sender of that message.\n");
        return -1;
    }

    printf("Replying to: %s\n", sender);

    /* --- Phase 4: Get reply content and send --- */
    printf("Enter your reply: ");
    if (fgets(reply_content, sizeof(reply_content), stdin) == NULL) {
        printf("Error: Failed to read reply.\n");
        return -1;
    }
    clean_newline(reply_content);

    return send_message(current_user, sender, reply_content);
}
