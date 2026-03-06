/*
 * main.c - Main Application Controller
 *
 * This is the entry point for the One-on-One Chat Application.
 * It implements the primary control flow using two menu loops:
 *
 *   Loop 1 (Login Menu):
 *     Selection options: Register, Login, Exit
 *
 *   Loop 2 (Dashboard Menu - after successful login):
 *     Selection options: Search Users, Read Inbox, Send Message,
 *                        Reply to Message, Logout, Deregister Account
 *
 * Design Pattern: Top-down modular design. main.c acts purely as a
 * controller, delegating all logic to the auth, user_mgr, and messenger
 * modules. No business logic is implemented here.
 *
 * Dependencies: All four modules (auth, user_mgr, messenger, utils).
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "auth.h"
#include "user_mgr.h"
#include "messenger.h"
#include "utils.h"

/* ---------- Helper Function Prototypes ---------- */
static void display_login_menu(void);
static void display_dashboard_menu(const char *username);
static void handle_registration(void);
static int  handle_login(char *logged_in_user, size_t user_buf_len);
static void run_dashboard(const char *username);

/* ========================================================================= */
/*                              MAIN ENTRY POINT                             */
/* ========================================================================= */

/*
 * main - Application entry point and primary selection loop.
 *
 * Algorithm Design:
 *   Step 1: Display a welcome banner.
 *   Step 2: Enter the Login Menu loop (iteration with while).
 *     2a. Display menu options.
 *     2b. Read user's choice (selection with switch).
 *     2c. Dispatch to the appropriate handler:
 *         Case 1: Register → handle_registration()
 *         Case 2: Login → handle_login() → if success, run_dashboard()
 *         Case 3: Exit → break the loop
 *   Step 3: Print farewell message and return.
 */
int main(void) {
    int choice;
    int running = 1;
    char logged_in_user[MAX_USERNAME];

    /* Step 1: Welcome banner */
    printf("╔══════════════════════════════════════════╗\n");
    printf("║     One-on-One Chat Application          ║\n");
    printf("║     ─────────────────────────────        ║\n");
    printf("║     A Modular C Messaging System         ║\n");
    printf("╚══════════════════════════════════════════╝\n\n");

    /* Ensure the data/ directory exists before any file operations */
    init_data_dir();

    log_action("SYSTEM", "Application started");

    /* Step 2: Login Menu loop — continues until user selects Exit */
    while (running) {
        display_login_menu();

        printf("Enter your choice: ");
        if (scanf("%d", &choice) != 1) {
            printf("Invalid input. Please enter a number.\n");
            /* Clear invalid input from stdin buffer */
            while (getchar() != '\n');
            continue;
        }
        /* Clear the newline character left by scanf */
        while (getchar() != '\n');

        /* Step 2c: Selection — dispatch based on choice */
        switch (choice) {
            case 1:
                handle_registration();
                break;

            case 2:
                if (handle_login(logged_in_user,
                                 sizeof(logged_in_user)) == 0) {
                    /* Successful login → enter dashboard */
                    run_dashboard(logged_in_user);
                }
                break;

            case 3:
                running = 0;  /* Exit the loop */
                break;

            default:
                printf("Invalid choice. Please select 1-3.\n");
                break;
        }
    }

    /* Step 3: Farewell */
    log_action("SYSTEM", "Application terminated");
    printf("\nThank you for using the Chat Application. Goodbye!\n");

    return 0;
}

/* ========================================================================= */
/*                            MENU DISPLAY FUNCTIONS                         */
/* ========================================================================= */

/*
 * display_login_menu - Prints the pre-login menu options.
 */
static void display_login_menu(void) {
    printf("\n┌──────────────────────────────┐\n");
    printf("│        LOGIN MENU            │\n");
    printf("├──────────────────────────────┤\n");
    printf("│  1. Register New Account     │\n");
    printf("│  2. Login                    │\n");
    printf("│  3. Exit                     │\n");
    printf("└──────────────────────────────┘\n");
}

/*
 * display_dashboard_menu - Prints the post-login dashboard menu.
 *
 * Parameters:
 *   username - the currently logged-in user (displayed in header)
 */
static void display_dashboard_menu(const char *username) {
    printf("\n┌──────────────────────────────────┐\n");
    printf("│  DASHBOARD — Logged in as: %-5s │\n", username);
    printf("├──────────────────────────────────┤\n");
    printf("│  1. Search Users                 │\n");
    printf("│  2. Read Inbox                   │\n");
    printf("│  3. Send Message                 │\n");
    printf("│  4. Reply to Message             │\n");
    printf("│  5. Logout                       │\n");
    printf("│  6. Deregister Account           │\n");
    printf("└──────────────────────────────────┘\n");
}

/* ========================================================================= */
/*                           HANDLER FUNCTIONS                               */
/* ========================================================================= */

/*
 * handle_registration - Prompts for credentials and registers a new user.
 *
 * Algorithm:
 *   1. Prompt for username and password (using fgets for safety).
 *   2. Sanitize input (remove trailing newlines).
 *   3. Populate a User struct by value.
 *   4. Call register_user() with a pointer to the struct (pass-by-reference).
 */
static void handle_registration(void) {
    User new_user;

    printf("\n--- Register New Account ---\n");

    printf("Enter username: ");
    if (fgets(new_user.username, sizeof(new_user.username), stdin) == NULL) {
        printf("Error: Failed to read username.\n");
        return;
    }
    clean_newline(new_user.username);

    printf("Enter password: ");
    if (fgets(new_user.password, sizeof(new_user.password), stdin) == NULL) {
        printf("Error: Failed to read password.\n");
        return;
    }
    clean_newline(new_user.password);

    /* Validate non-empty input */
    if (strlen(new_user.username) == 0 || strlen(new_user.password) == 0) {
        printf("Error: Username and password cannot be empty.\n");
        return;
    }

    /* Delegate to auth module (pass-by-reference) */
    register_user(&new_user);
}

/*
 * handle_login - Prompts for credentials and authenticates the user.
 *
 * Algorithm:
 *   1. Prompt for username and password.
 *   2. Call authenticate_user() with the provided credentials.
 *   3. On success, copy the username into the output buffer.
 *
 * Parameters:
 *   logged_in_user - buffer to store the authenticated username
 *   user_buf_len   - size of the buffer
 *
 * Returns:
 *   0 on success, -1 on failure
 */
static int handle_login(char *logged_in_user, size_t user_buf_len) {
    char username[MAX_USERNAME];
    char password[MAX_PASSWORD];

    printf("\n--- Login ---\n");

    printf("Enter username: ");
    if (fgets(username, sizeof(username), stdin) == NULL) {
        printf("Error: Failed to read username.\n");
        return -1;
    }
    clean_newline(username);

    printf("Enter password: ");
    if (fgets(password, sizeof(password), stdin) == NULL) {
        printf("Error: Failed to read password.\n");
        return -1;
    }
    clean_newline(password);

    /* Delegate authentication to auth module */
    if (authenticate_user(username, password) == 0) {
        printf("Login successful! Welcome back, %s.\n", username);
        strncpy(logged_in_user, username, user_buf_len - 1);
        logged_in_user[user_buf_len - 1] = '\0';
        return 0;
    }

    return -1;
}

/*
 * run_dashboard - The main dashboard loop after successful login.
 *
 * Algorithm Design:
 *   This implements the "MainDashboard" state from the Process Diagram.
 *   It runs a selection loop (while + switch) with the following options:
 *
 *   Selection 1: Search Users  → prompts for query, calls search_users()
 *   Selection 2: Read Inbox    → calls read_inbox()
 *   Selection 3: Send Message  → prompts for recipient/content, calls send_message()
 *   Selection 4: Reply to Msg  → calls reply_message() (wrapper around send)
 *   Selection 5: Logout        → breaks the loop, returns to Login Menu
 *   Selection 6: Deregister    → calls deregister_user(), returns to Login Menu
 *
 * Parameters:
 *   username - the currently logged-in user's name
 */
static void run_dashboard(const char *username) {
    int choice;
    int active = 1;
    char query[MAX_USERNAME];
    char recipient[MAX_USERNAME];
    char content[MAX_CONTENT];

    while (active) {
        display_dashboard_menu(username);

        printf("Enter your choice: ");
        if (scanf("%d", &choice) != 1) {
            printf("Invalid input. Please enter a number.\n");
            while (getchar() != '\n');
            continue;
        }
        while (getchar() != '\n');

        switch (choice) {
            /* --- Selection 1: Search Users --- */
            case 1:
                printf("Enter search query (or press Enter to list all): ");
                if (fgets(query, sizeof(query), stdin) == NULL) {
                    break;
                }
                clean_newline(query);

                if (strlen(query) == 0) {
                    list_users();
                } else {
                    search_users(query);
                }
                break;

            /* --- Selection 2: Read Inbox --- */
            case 2:
                read_inbox(username);
                break;

            /* --- Selection 3: Send Message --- */
            case 3:
                printf("Enter recipient username: ");
                if (fgets(recipient, sizeof(recipient), stdin) == NULL) {
                    break;
                }
                clean_newline(recipient);

                /* Prevent sending messages to yourself */
                if (strcmp(recipient, username) == 0) {
                    printf("Error: You cannot send a message to yourself.\n");
                    break;
                }

                printf("Enter your message:\n> ");
                if (fgets(content, sizeof(content), stdin) == NULL) {
                    break;
                }
                clean_newline(content);

                if (strlen(content) == 0) {
                    printf("Error: Cannot send an empty message.\n");
                    break;
                }

                send_message(username, recipient, content);
                break;

            /* --- Selection 4: Reply to Message --- */
            case 4:
                reply_message(username);
                break;

            /* --- Selection 5: Logout --- */
            case 5:
                log_action(username, "logged out");
                printf("Logged out successfully.\n");
                active = 0;
                break;

            /* --- Selection 6: Deregister Account --- */
            case 6:
                printf("Are you sure you want to delete your account? (y/n): ");
                {
                    char confirm[4];
                    if (fgets(confirm, sizeof(confirm), stdin) != NULL) {
                        clean_newline(confirm);
                        if (strcmp(confirm, "y") == 0 ||
                            strcmp(confirm, "Y") == 0) {
                            deregister_user(username);
                            active = 0;  /* Return to login menu */
                        } else {
                            printf("Account deletion cancelled.\n");
                        }
                    }
                }
                break;

            default:
                printf("Invalid choice. Please select 1-6.\n");
                break;
        }
    }
}
