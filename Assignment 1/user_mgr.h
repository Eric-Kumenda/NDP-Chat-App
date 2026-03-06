/*
 * user_mgr.h - User Discovery Module Header
 *
 * Provides functions for:
 *   - Listing all registered users
 *   - Searching for users by a substring query
 */

#ifndef USER_MGR_H
#define USER_MGR_H

/* ---------- Function Prototypes ---------- */

/*
 * list_users - Displays all registered usernames.
 *
 * Algorithm:
 *   1. Open users.txt for reading.
 *   2. For each line, extract the username (before the '|' delimiter).
 *   3. Print each username to stdout with a numbered index.
 *
 * Returns:
 *   The number of users found, or -1 on file error
 */
int list_users(void);

/*
 * search_users - Finds users whose usernames contain the given query.
 *
 * Algorithm:
 *   1. Open users.txt for reading.
 *   2. For each line, extract the username.
 *   3. Use strstr() to check if the query is a substring of the username.
 *   4. Print matching usernames.
 *
 * Parameters:
 *   query - the substring to search for in usernames
 *
 * Returns:
 *   The number of matching users, or -1 on file error
 */
int search_users(const char *query);

#endif /* USER_MGR_H */
