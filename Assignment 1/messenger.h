/*
 * messenger.h - Messaging Module Header
 *
 * Provides the Message data structure and functions for:
 *   - Sending a message (with file locking for concurrency)
 *   - Reading the current user's inbox
 *   - Replying to a specific message in the inbox
 */

#ifndef MESSENGER_H
#define MESSENGER_H

#include "utils.h"

/* ---------- Data Structures ---------- */

/*
 * Message - Represents a single chat message.
 *
 * Fields:
 *   sender    - the username of the message author
 *   timestamp - formatted date/time when the message was sent
 *   content   - the body text of the message
 */
typedef struct {
    char sender[MAX_USERNAME];
    char timestamp[MAX_TIMESTAMP];
    char content[MAX_CONTENT];
} Message;

/* ---------- Function Prototypes ---------- */

/*
 * send_message - Sends a message from one user to another.
 *
 * Algorithm:
 *   1. Build the recipient's inbox path: inbox_{recipient}.txt.
 *   2. Open the file in append mode using open() for fd-level access.
 *   3. Acquire an exclusive file lock via lock_file(fd).
 *   4. Write the formatted message block:
 *        [timestamp] From: sender
 *        message content
 *        ---
 *   5. Release the lock via unlock_file(fd) and close.
 *   6. Log the action to system.log.
 *
 * Parameters:
 *   sender    - username of the person sending the message
 *   recipient - username of the person receiving the message
 *   content   - the message body text
 *
 * Returns:
 *   0 on success, -1 on failure (e.g., recipient inbox not found)
 */
int send_message(const char *sender, const char *recipient, const char *content);

/*
 * read_inbox - Reads and displays all messages in a user's inbox.
 *
 * Algorithm:
 *   1. Build the inbox path: inbox_{username}.txt.
 *   2. Open the file for reading.
 *   3. Parse message blocks (delimited by "---").
 *   4. Display each message with a numbered index.
 *
 * Parameters:
 *   username - the user whose inbox to read
 *
 * Returns:
 *   The number of messages read, or -1 on file error
 */
int read_inbox(const char *username);

/*
 * reply_message - Replies to a specific message in the inbox.
 *
 * Algorithm:
 *   1. Call read_inbox() to display messages with indices.
 *   2. Prompt the user to select a message number.
 *   3. Re-parse the inbox to extract the sender of that message.
 *   4. Prompt for reply content.
 *   5. Call send_message() with the extracted sender as recipient.
 *
 * Parameters:
 *   current_user - the username of the person replying
 *
 * Returns:
 *   0 on success, -1 on failure
 */
int reply_message(const char *current_user);

#endif /* MESSENGER_H */
