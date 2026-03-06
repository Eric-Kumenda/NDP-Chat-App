/*
 * auth.h - Authentication Module Header
 *
 * Provides the User data structure and functions for:
 *   - Registering a new user (with duplicate checking and inbox creation)
 *   - Authenticating an existing user against stored credentials
 *   - Deregistering (deleting) a user account
 */

#ifndef AUTH_H
#define AUTH_H

#include "utils.h"

/* ---------- Data Structures ---------- */

/*
 * User - Represents a registered user's credentials.
 *
 * Fields:
 *   username - the unique login name (max 49 chars + null terminator)
 *   password - the user's password (max 49 chars + null terminator)
 */
typedef struct {
    char username[MAX_USERNAME];
    char password[MAX_PASSWORD];
} User;

/* ---------- Function Prototypes ---------- */

/*
 * register_user - Creates a new user account.
 *
 * Algorithm (Top-Down):
 *   1. Open users.txt and scan for duplicate usernames.
 *   2. If no duplicate found, append "username|password\n" to users.txt.
 *   3. Create an empty inbox file: inbox_{username}.txt.
 *   4. Log the registration action to system.log.
 *
 * Parameters:
 *   user - pointer to a User struct containing the credentials
 *
 * Returns:
 *   0 on success, -1 if user already exists, -2 on file error
 */
int register_user(const User *user);

/*
 * authenticate_user - Validates a user's login credentials.
 *
 * Algorithm:
 *   1. Open users.txt for reading.
 *   2. Read each line, split on '|' delimiter.
 *   3. Compare both username and password fields.
 *   4. Return success if a match is found, failure otherwise.
 *
 * Parameters:
 *   username - the username to authenticate
 *   password - the password to verify
 *
 * Returns:
 *   0 on success (credentials match), -1 on failure
 */
int authenticate_user(const char *username, const char *password);

/*
 * deregister_user - Removes a user account from the system.
 *
 * Algorithm:
 *   1. Read all lines from users.txt.
 *   2. Rewrite users.txt, excluding the line matching the target username.
 *   3. Delete the user's inbox file (inbox_{username}.txt).
 *   4. Log the deregistration action.
 *
 * Parameters:
 *   username - the username of the account to delete
 *
 * Returns:
 *   0 on success, -1 if user not found, -2 on file error
 */
int deregister_user(const char *username);

#endif /* AUTH_H */
