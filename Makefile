# Makefile for ASCII Texas Hold'em Poker

CC ?= gcc
CFLAGS ?= -Wall -Wextra -O2 -pthread -std=c11
CPPFLAGS ?= -Iinclude
LDFLAGS ?= -pthread
PKG_CONFIG ?= pkg-config

OPENSSL_CFLAGS := $(shell $(PKG_CONFIG) --cflags openssl 2>/dev/null)
OPENSSL_LIBS := $(shell $(PKG_CONFIG) --libs openssl 2>/dev/null)
SQLITE_CFLAGS := $(shell $(PKG_CONFIG) --cflags sqlite3 2>/dev/null)
SQLITE_LIBS := $(shell $(PKG_CONFIG) --libs sqlite3 2>/dev/null)
UUID_CFLAGS := $(shell $(PKG_CONFIG) --cflags uuid 2>/dev/null)
UUID_LIBS := $(shell $(PKG_CONFIG) --libs uuid 2>/dev/null)
JANSSON_CFLAGS := $(shell $(PKG_CONFIG) --cflags jansson 2>/dev/null)
JANSSON_LIBS := $(shell $(PKG_CONFIG) --libs jansson 2>/dev/null)

SRC_DIR := src
INC_DIR := include
BUILD_DIR := build
BIN_DIR := bin

SERVER := $(BIN_DIR)/poker_server
CLIENT := $(BIN_DIR)/poker_client

POKER_OBJ := $(BUILD_DIR)/poker.o
SERVER_OBJ := $(BUILD_DIR)/server.o
CLIENT_OBJ := $(BUILD_DIR)/client.o
IDS_OBJ := $(BUILD_DIR)/ids.o
AUDIT_OBJ := $(BUILD_DIR)/audit.o
PROTOCOL_OBJ := $(BUILD_DIR)/protocol.o

.PHONY: all server client clean debug run-server run-client smoke integration test support-test deps-check install uninstall help

all: server client

server: $(SERVER)

client: $(CLIENT)

$(SERVER): $(SERVER_OBJ) $(POKER_OBJ) $(IDS_OBJ) $(AUDIT_OBJ) $(PROTOCOL_OBJ) | $(BIN_DIR)
	$(CC) $(LDFLAGS) -o $@ $^ $(OPENSSL_LIBS) $(UUID_LIBS) $(SQLITE_LIBS) $(JANSSON_LIBS)

$(CLIENT): $(CLIENT_OBJ) $(POKER_OBJ) $(PROTOCOL_OBJ) | $(BIN_DIR)
	$(CC) $(LDFLAGS) -o $@ $^ $(JANSSON_LIBS)

$(BUILD_DIR)/%.o: $(SRC_DIR)/%.c $(INC_DIR)/poker.h | $(BUILD_DIR)
	$(CC) $(CPPFLAGS) $(CFLAGS) -c $< -o $@

$(IDS_OBJ): $(SRC_DIR)/ids.c $(INC_DIR)/ids.h | $(BUILD_DIR)
	$(CC) $(CPPFLAGS) $(OPENSSL_CFLAGS) $(UUID_CFLAGS) $(CFLAGS) -c $< -o $@

$(AUDIT_OBJ): $(SRC_DIR)/audit.c $(INC_DIR)/audit.h | $(BUILD_DIR)
	$(CC) $(CPPFLAGS) $(SQLITE_CFLAGS) $(CFLAGS) -c $< -o $@

$(PROTOCOL_OBJ): $(SRC_DIR)/protocol.c $(INC_DIR)/protocol.h | $(BUILD_DIR)
	$(CC) $(CPPFLAGS) $(JANSSON_CFLAGS) $(CFLAGS) -c $< -o $@

$(BUILD_DIR) $(BIN_DIR):
	mkdir -p $@

clean:
	rm -rf $(BUILD_DIR) $(BIN_DIR)

debug: CFLAGS += -g -DDEBUG
debug: clean all

run-server: $(SERVER)
	./$(SERVER)

run-client: $(CLIENT)
	./$(CLIENT)

smoke: all
	./tests/smoke_two_clients.sh

integration: all
	./tests/integration_gameplay.sh

test: all
	$(CC) $(CPPFLAGS) $(JANSSON_CFLAGS) $(OPENSSL_CFLAGS) $(UUID_CFLAGS) $(SQLITE_CFLAGS) $(CFLAGS) tests/rules_test.c $(SRC_DIR)/poker.c $(SRC_DIR)/protocol.c $(SRC_DIR)/ids.c $(SRC_DIR)/audit.c $(JANSSON_LIBS) $(OPENSSL_LIBS) $(UUID_LIBS) $(SQLITE_LIBS) $(LDFLAGS) -o $(BUILD_DIR)/rules_test
	./$(BUILD_DIR)/rules_test
	$(CC) $(CPPFLAGS) $(JANSSON_CFLAGS) $(CFLAGS) tests/protocol_test.c $(SRC_DIR)/protocol.c $(JANSSON_LIBS) $(LDFLAGS) -o $(BUILD_DIR)/protocol_test
	./$(BUILD_DIR)/protocol_test
	$(MAKE) support-test
	./tests/integration_gameplay.sh

support-test: $(IDS_OBJ) $(AUDIT_OBJ)
	$(CC) $(CPPFLAGS) $(CFLAGS) tests/support_test.c $(IDS_OBJ) $(AUDIT_OBJ) $(OPENSSL_LIBS) $(UUID_LIBS) $(SQLITE_LIBS) $(LDFLAGS) -o $(BUILD_DIR)/support_test
	./$(BUILD_DIR)/support_test

deps-check:
	@missing=0; \
	for pkg in jansson uuid openssl sqlite3; do \
		if ! $(PKG_CONFIG) --exists $$pkg; then \
			echo "missing dependency: $$pkg"; \
			missing=1; \
		fi; \
	done; \
	if [ $$missing -ne 0 ]; then \
		echo "Install on Debian/Ubuntu: sudo apt-get install libjansson-dev uuid-dev libssl-dev libsqlite3-dev pkg-config"; \
		exit 1; \
	fi; \
	echo "all planned dependencies are available"

install: all
	cp $(SERVER) /usr/local/bin/poker_server
	cp $(CLIENT) /usr/local/bin/poker_client

uninstall:
	rm -f /usr/local/bin/poker_server
	rm -f /usr/local/bin/poker_client

help:
	@echo "Available targets:"
	@echo "  all         - Build server and client"
	@echo "  server      - Build bin/poker_server"
	@echo "  client      - Build bin/poker_client"
	@echo "  clean       - Remove build and bin output"
	@echo "  debug       - Rebuild with debug symbols"
	@echo "  run-server  - Build and run the server"
	@echo "  run-client  - Build and run the client"
	@echo "  smoke       - Run two-client local smoke test"
	@echo "  integration - Run scripted gameplay integration tests"
	@echo "  test        - Run rules and gameplay tests"
	@echo "  support-test - Run dependency support module tests"
	@echo "  deps-check  - Verify planned system dependencies"
	@echo "  install     - Install binaries to /usr/local/bin"
	@echo "  uninstall   - Remove installed binaries"
