# ==============================================================================
# Makefile - One-on-One Chat Application Build System
#
# Usage:
#   make          Build the chat_app binary
#   make clean    Remove all compiled files
#   make rebuild  Clean and rebuild from scratch
# ==============================================================================

# --- Compiler Configuration ---
CC      = gcc
CFLAGS  = -Wall -Wextra -std=c11 -pedantic -D_POSIX_C_SOURCE=200809L
TARGET  = chat_app

# --- Source and Object Files ---
SRCS    = main.c auth.c user_mgr.c messenger.c utils.c
OBJS    = $(SRCS:.c=.o)
HEADERS = auth.h user_mgr.h messenger.h utils.h

# ==============================================================================
# Build Rules
# ==============================================================================

# Default target: compile and link all modules into the final binary
all: $(TARGET)

# Link all object files into the executable
$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $(TARGET) $(OBJS)

# Pattern rule: compile each .c file into a .o file
# Every object depends on its source AND all headers (simple dependency model)
%.o: %.c $(HEADERS)
	$(CC) $(CFLAGS) -c $< -o $@

# Remove all build artifacts and runtime data files
clean:
	rm -f $(OBJS) $(TARGET)

# Convenience: clean then rebuild
rebuild: clean all

# Phony targets (not actual files)
.PHONY: all clean rebuild
