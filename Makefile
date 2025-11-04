# Makefile for Texas Hold'em Poker

CC = gcc
CFLAGS = -Wall -Wextra -O2 -pthread -std=c11
LDFLAGS = -pthread

# Targets
SERVER = poker_server
CLIENT = poker_client

# Source files
SERVER_SRC = server.c poker.c
CLIENT_SRC = client.c poker.c
COMMON_SRC = poker.c

# Object files
SERVER_OBJ = $(SERVER_SRC:.c=.o)
CLIENT_OBJ = $(CLIENT_SRC:.c=.o)

# Default target
all: $(SERVER) $(CLIENT)

# Server target
$(SERVER): server.o poker.o
	$(CC) $(LDFLAGS) -o $@ $^

# Client target
$(CLIENT): client.o poker.o
	$(CC) $(LDFLAGS) -o $@ $^

# Object file compilation
%.o: %.c poker.h
	$(CC) $(CFLAGS) -c $< -o $@

# Clean
clean:
	rm -f $(SERVER) $(CLIENT) *.o

# Install (optional)
install: all
	cp $(SERVER) /usr/local/bin/
	cp $(CLIENT) /usr/local/bin/

# Uninstall (optional)
uninstall:
	rm -f /usr/local/bin/$(SERVER)
	rm -f /usr/local/bin/$(CLIENT)

# Run server
run-server: $(SERVER)
	./$(SERVER)

# Run client
run-client: $(CLIENT)
	./$(CLIENT)

# Debug build
debug: CFLAGS += -g -DDEBUG
debug: clean all

# Help
help:
	@echo "Available targets:"
	@echo "  all         - Build both server and client (default)"
	@echo "  server      - Build server only"
	@echo "  client      - Build client only"
	@echo "  clean       - Remove all built files"
	@echo "  run-server  - Build and run the server"
	@echo "  run-client  - Build and run the client"
	@echo "  debug       - Build with debug symbols"
	@echo "  install     - Install to /usr/local/bin"
	@echo "  uninstall   - Remove from /usr/local/bin"
	@echo "  help        - Show this help message"

.PHONY: all clean install uninstall run-server run-client debug help
