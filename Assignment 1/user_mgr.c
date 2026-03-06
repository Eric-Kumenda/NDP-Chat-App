/*
 * user_mgr.c - User Discovery Module Implementation
 *
 * Implements functions for listing and searching registered users
 * by reading the users.txt flat file.
 *
 * Dependencies: <stdio.h>, <string.h>
 */

#include <stdio.h>
#include <string.h>

#include "user_mgr.h"
#include "utils.h"

/*
 * list_users - Displays all registered usernames with numbered indices.
 *
 * Algorithm Design:
 *   Step 1: Open users.txt for reading.
 *   Step 2: Initialize a counter to zero.
 *   Step 3: Iterate (while loop) over each line in the file.
 *     3a. Extract the username by finding the '|' delimiter.
 *     3b. Print the username with its index number.
 *     3c. Increment the counter.
 *   Step 4: If the counter is zero, inform the user that no users exist.
 *   Step 5: Return the total count.
 */
int list_users(void) {
    FILE *fp;
    char line[MAX_LINE];
    char username[MAX_USERNAME];
    int count = 0;

    fp = fopen(USERS_FILE, "r");
    if (fp == NULL) {
        printf("No registered users found.\n");
        return -1;
    }

    printf("\n========== Registered Users ==========\n");

    /* Step 3: Iterate through all user entries */
    while (fgets(line, sizeof(line), fp) != NULL) {
        clean_newline(line);

        /* Step 3a: Extract username before the delimiter */
        char *delimiter = strchr(line, '|');
        if (delimiter != NULL) {
            size_t uname_len = (size_t)(delimiter - line);
            if (uname_len < MAX_USERNAME) {
                strncpy(username, line, uname_len);
                username[uname_len] = '\0';

                /* Step 3b: Display with numbered index */
                count++;
                printf("  %d. %s\n", count, username);
            }
        }
    }

    fclose(fp);

    /* Step 4: Handle empty user list */
    if (count == 0) {
        printf("  (No users registered yet)\n");
    }

    printf("======================================\n");
    return count;
}

/*
 * search_users - Finds users whose usernames contain the query substring.
 *
 * Algorithm Design:
 *   Step 1: Open users.txt for reading.
 *   Step 2: Initialize a match counter to zero.
 *   Step 3: Iterate (while loop) over each line in the file.
 *     3a. Extract the username field.
 *     3b. Use strstr() to check if query is a substring of the username.
 *     3c. If a match is found, print the username and increment counter.
 *   Step 4: If no matches were found, inform the user.
 *   Step 5: Return the count of matches.
 *
 * Note: The search is case-sensitive to maintain simplicity.
 */
int search_users(const char *query) {
    FILE *fp;
    char line[MAX_LINE];
    char username[MAX_USERNAME];
    int count = 0;

    fp = fopen(USERS_FILE, "r");
    if (fp == NULL) {
        printf("No registered users found.\n");
        return -1;
    }

    printf("\n===== Search Results for '%s' =====\n", query);

    /* Step 3: Iterate and filter by substring match */
    while (fgets(line, sizeof(line), fp) != NULL) {
        clean_newline(line);

        char *delimiter = strchr(line, '|');
        if (delimiter != NULL) {
            size_t uname_len = (size_t)(delimiter - line);
            if (uname_len < MAX_USERNAME) {
                strncpy(username, line, uname_len);
                username[uname_len] = '\0';

                /* Step 3b: Selection — check for substring match */
                if (strstr(username, query) != NULL) {
                    count++;
                    printf("  %d. %s\n", count, username);
                }
            }
        }
    }

    fclose(fp);

    /* Step 4: Handle no matches */
    if (count == 0) {
        printf("  No users matching '%s' found.\n", query);
    }

    printf("====================================\n");
    return count;
}
