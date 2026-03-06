/*
 * auth.c - Authentication Module Implementation
 *
 * Implements user registration, authentication, and deregistration.
 * All credential data is stored in the flat file "users.txt" with
 * the format: username|password (one entry per line).
 *
 * Dependencies: <stdio.h>, <string.h>, <stdlib.h>, <unistd.h>
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>

#include "auth.h"
#include "utils.h"

/*
 * register_user - Creates a new user account in the system.
 *
 * Algorithm Design (Top-Down Decomposition):
 *   Phase 1 - Duplicate Check:
 *     1a. Open users.txt for reading.
 *     1b. Iterate through each line, extracting the username field.
 *     1c. Compare with the new username; if a match is found, return error.
 *
 *   Phase 2 - Account Creation:
 *     2a. Open users.txt in append mode.
 *     2b. Write "username|password\n" to the file.
 *     2c. Close the file.
 *
 *   Phase 3 - Inbox Initialization:
 *     3a. Build the inbox filename: inbox_{username}.txt.
 *     3b. Create the file (fopen with "w" mode) and close immediately.
 *
 *   Phase 4 - Audit:
 *     4a. Log the registration event to system.log.
 */
int register_user(const User *user) {
    FILE *fp;
    char line[MAX_LINE];
    char stored_username[MAX_USERNAME];
    char inbox_path[MAX_LINE];

    /* --- Phase 1: Check for duplicate username --- */
    fp = fopen(USERS_FILE, "r");
    if (fp != NULL) {
        /* Iterate through all existing entries */
        while (fgets(line, sizeof(line), fp) != NULL) {
            clean_newline(line);

            /* Extract username before the '|' delimiter */
            char *delimiter = strchr(line, '|');
            if (delimiter != NULL) {
                size_t uname_len = (size_t)(delimiter - line);
                if (uname_len < MAX_USERNAME) {
                    strncpy(stored_username, line, uname_len);
                    stored_username[uname_len] = '\0';

                    /* Selection: if username matches, reject registration */
                    if (strcmp(stored_username, user->username) == 0) {
                        printf("Error: Username '%s' already exists.\n",
                               user->username);
                        fclose(fp);
                        return -1;
                    }
                }
            }
        }
        fclose(fp);
    }
    /* If file doesn't exist yet, that's OK — first registration */

    /* --- Phase 2: Append new credentials to users.txt --- */
    fp = fopen(USERS_FILE, "a");
    if (fp == NULL) {
        perror("Error: Cannot open users.txt for writing");
        return -2;
    }
    fprintf(fp, "%s|%s\n", user->username, user->password);
    fclose(fp);

    /* --- Phase 3: Create the user's empty inbox file --- */
    build_inbox_path(user->username, inbox_path, sizeof(inbox_path));
    fp = fopen(inbox_path, "w");
    if (fp == NULL) {
        perror("Error: Cannot create inbox file");
        return -2;
    }
    fclose(fp);

    /* --- Phase 4: Log the action --- */
    log_action(user->username, "registered a new account");

    printf("Registration successful! Welcome, %s.\n", user->username);
    return 0;
}

/*
 * authenticate_user - Verifies a user's login credentials.
 *
 * Algorithm Design:
 *   Step 1: Open users.txt for reading.
 *   Step 2: For each line, split on '|' to get stored username and password.
 *   Step 3: Compare both fields against the provided credentials.
 *   Step 4: If a match is found, return success; otherwise, return failure.
 *
 * Flow Control: Uses a while loop for iteration and if/else for selection.
 */
int authenticate_user(const char *username, const char *password) {
    FILE *fp;
    char line[MAX_LINE];
    char stored_username[MAX_USERNAME];
    char stored_password[MAX_PASSWORD];

    fp = fopen(USERS_FILE, "r");
    if (fp == NULL) {
        printf("Error: No registered users found. Please register first.\n");
        return -1;
    }

    /* Iteration: scan each line for matching credentials */
    while (fgets(line, sizeof(line), fp) != NULL) {
        clean_newline(line);

        /* Parse the "username|password" format */
        char *delimiter = strchr(line, '|');
        if (delimiter != NULL) {
            size_t uname_len = (size_t)(delimiter - line);
            if (uname_len < MAX_USERNAME) {
                strncpy(stored_username, line, uname_len);
                stored_username[uname_len] = '\0';
                strncpy(stored_password, delimiter + 1, MAX_PASSWORD - 1);
                stored_password[MAX_PASSWORD - 1] = '\0';

                /* Selection: check if both username and password match */
                if (strcmp(stored_username, username) == 0 &&
                    strcmp(stored_password, password) == 0) {
                    fclose(fp);
                    log_action(username, "logged in");
                    return 0;   /* Authentication success */
                }
            }
        }
    }

    fclose(fp);
    printf("Error: Invalid username or password.\n");
    return -1;  /* Authentication failure */
}

/*
 * deregister_user - Removes a user account from the system.
 *
 * Algorithm Design (Top-Down Decomposition):
 *   Phase 1 - Filter:
 *     1a. Open users.txt for reading and a temp file for writing.
 *     1b. Copy every line EXCEPT the one matching the target username.
 *     1c. Track whether the user was actually found.
 *
 *   Phase 2 - Replace:
 *     2a. Remove the original users.txt.
 *     2b. Rename the temp file to users.txt.
 *
 *   Phase 3 - Cleanup:
 *     3a. Delete the user's inbox file.
 *
 *   Phase 4 - Audit:
 *     4a. Log the deregistration to system.log.
 */
int deregister_user(const char *username) {
    FILE *fp_in, *fp_out;
    char line[MAX_LINE];
    char stored_username[MAX_USERNAME];
    char inbox_path[MAX_LINE];
    int found = 0;
    const char *temp_file = "users_tmp.txt";

    /* --- Phase 1: Filter out the target user --- */
    fp_in = fopen(USERS_FILE, "r");
    if (fp_in == NULL) {
        printf("Error: Cannot open users file.\n");
        return -2;
    }

    fp_out = fopen(temp_file, "w");
    if (fp_out == NULL) {
        perror("Error: Cannot create temp file");
        fclose(fp_in);
        return -2;
    }

    while (fgets(line, sizeof(line), fp_in) != NULL) {
        char line_copy[MAX_LINE];
        strncpy(line_copy, line, MAX_LINE - 1);
        line_copy[MAX_LINE - 1] = '\0';
        clean_newline(line_copy);

        /* Extract username for comparison */
        char *delimiter = strchr(line_copy, '|');
        if (delimiter != NULL) {
            size_t uname_len = (size_t)(delimiter - line_copy);
            if (uname_len < MAX_USERNAME) {
                strncpy(stored_username, line_copy, uname_len);
                stored_username[uname_len] = '\0';

                /* Selection: skip the line if it matches the target */
                if (strcmp(stored_username, username) == 0) {
                    found = 1;
                    continue;   /* Do NOT copy this line to the output */
                }
            }
        }
        /* Write non-matching lines to the temp file */
        fputs(line, fp_out);
    }

    fclose(fp_in);
    fclose(fp_out);

    if (!found) {
        remove(temp_file);
        printf("Error: User '%s' not found.\n", username);
        return -1;
    }

    /* --- Phase 2: Replace original file with filtered version --- */
    remove(USERS_FILE);
    rename(temp_file, USERS_FILE);

    /* --- Phase 3: Delete the user's inbox file --- */
    build_inbox_path(username, inbox_path, sizeof(inbox_path));
    remove(inbox_path);

    /* --- Phase 4: Log the action --- */
    log_action(username, "deregistered account");

    printf("Account '%s' has been deleted.\n", username);
    return 0;
}
