/**
 * Texas Hold'em Poker Client
 * ASCII terminal interface
 */

#define _POSIX_C_SOURCE 200809L
#include "poker.h"
#include "protocol.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <termios.h>
#include <jansson.h>

#define BUFFER_SIZE 4096
#define MAX_PLAYERS 10

typedef struct {
    char name[64];
    int chips;
    int bet;
    bool folded;
    bool all_in;
    bool is_dealer;
    bool is_current;
} PlayerInfo;

typedef struct {
    int pot;
    int current_bet;
    char round[16];
    char community_cards[128];
    Card hand[2];
    int chips;
    int bet;
    PlayerInfo players[MAX_PLAYERS];
    int num_players;
    bool my_turn;
    bool legal_active;
    bool can_check;
    int call_amount;
    int min_raise;
    int max_raise;
    bool can_raise;
} GameState;

int client_socket;
GameState game_state;
char my_name[64];
char player_id[64] = "terminal-client";
char session_token[128] = "";
int client_message_seq = 1;
pthread_mutex_t state_lock = PTHREAD_MUTEX_INITIALIZER;
bool running = true;
bool name_from_args = false;
bool host_from_args = false;
bool port_from_args = false;

/* Function prototypes */
void* receive_thread(void* arg);
void parse_message(const char* message);
void display_game_state(void);
void clear_screen(void);
void send_action(const char* action, int amount);
void send_control(const char* command, int amount);
void send_client_payload(const char *type, json_t *payload);
void display_help(void);
void print_client_help(const char *program);
void parse_client_args(int argc, char **argv, char *server_ip, size_t server_ip_size,
                       int *port);

/* Clear screen */
void clear_screen(void) {
    printf("\033[2J\033[H");
}

/* Display game state */
void display_game_state(void) {
    pthread_mutex_lock(&state_lock);
    
    clear_screen();
    
    /* Compact header - single line */
    printf("Pot: %d | Bet: %d | Round: %s\n",
           game_state.pot, game_state.current_bet, game_state.round);
    
    /* Community cards - compact */
    printf("Board: ");
    if (strlen(game_state.community_cards) > 0) {
        char* cards = strdup(game_state.community_cards);
        char* token = strtok(cards, ",");
        while (token) {
            printf("[%s] ", token);
            token = strtok(NULL, ",");
        }
        free(cards);
    } else {
        printf("[--] [--] [--] [--] [--]");
    }
    printf("\n");
    
    /* Your hand - compact */
    char card1[4], card2[4];
    card_to_string(game_state.hand[0], card1);
    card_to_string(game_state.hand[1], card2);
    printf("Hand:  [%s] [%s] | Chips: %d | Bet: %d\n",
           card1, card2, game_state.chips, game_state.bet);
    
    /* Players - compact table format */
    printf("\nPlayers:\n");
    for (int i = 0; i < game_state.num_players; i++) {
        PlayerInfo* p = &game_state.players[i];
        bool is_me = strcmp(p->name, my_name) == 0;
        
        /* Compact status indicators */
        printf("%c%c%c %-12s %6d",
               is_me ? '*' : ' ',
               p->is_dealer ? 'D' : ' ',
               p->is_current ? '>' : ' ',
               p->name,
               p->chips);
        
        if (p->bet > 0) printf(" (%d)", p->bet);
        if (p->folded) printf(" FOLD");
        if (p->all_in) printf(" ALL-IN");
        printf("\n");
    }
    
    /* Action prompt - only when it's your turn */
    if (game_state.my_turn) {
        printf("\n** YOUR TURN **\n");
        if (!game_state.legal_active) {
            printf("Waiting for legal actions from server");
        } else {
            if (game_state.call_amount > 0) {
                printf("Actions: fold | call (%d)", game_state.call_amount);
            } else if (game_state.can_check) {
                printf("Actions: fold | check");
            } else {
                printf("Actions: fold");
            }
            if (game_state.can_raise) {
                printf(" | raise <total> (min %d, max %d)",
                       game_state.min_raise, game_state.max_raise);
            }
        }
        printf("\n");
    }
    
    printf("> ");
    fflush(stdout);
    
    pthread_mutex_unlock(&state_lock);
}

/* Parse server message */
void parse_message(const char* message) {
    char parse_error[128];
    json_t *json = protocol_parse(message, parse_error, sizeof(parse_error));
    if (!json) {
        printf("\nProtocol error: %s\n> ", parse_error);
        fflush(stdout);
        return;
    }

    const char *type = protocol_get_string(json, "type");
    json_t *payload = protocol_get_payload(json);

    pthread_mutex_lock(&state_lock);

    if (strcmp(type, "welcome") == 0) {
        const char *name = protocol_get_string(payload, "name");
        const char *id = protocol_get_string(payload, "player_id");
        const char *session = protocol_get_string(payload, "session");
        int chips = (int)json_integer_value(json_object_get(payload, "chips"));
        if (id) {
            snprintf(player_id, sizeof(player_id), "%s", id);
        }
        if (session) {
            snprintf(session_token, sizeof(session_token), "%s", session);
        }

        clear_screen();
        printf("======================================================================\n");
        printf("Welcome to Texas Hold'em, %s!\n", name ? name : my_name);
        printf("Starting chips: %d\n", chips);
        printf("======================================================================\n");
        printf("Waiting for game to start...\n");
    }
    else if (strcmp(type, "state") == 0) {
        game_state.pot = (int)json_integer_value(json_object_get(payload, "pot"));
        game_state.current_bet = (int)json_integer_value(json_object_get(payload, "current_bet"));
        const char *round = protocol_get_string(payload, "round");
        snprintf(game_state.round, sizeof(game_state.round), "%s", round ? round : "");
        game_state.chips = (int)json_integer_value(json_object_get(payload, "chips"));
        game_state.bet = (int)json_integer_value(json_object_get(payload, "bet"));
        game_state.my_turn = json_is_true(json_object_get(payload, "my_turn"));

        game_state.community_cards[0] = '\0';
        json_t *community = json_object_get(payload, "community");
        if (json_is_array(community)) {
            size_t idx;
            json_t *card;
            json_array_foreach(community, idx, card) {
                const char *card_str = json_string_value(card);
                if (!card_str) continue;
                if (game_state.community_cards[0]) {
                    strncat(game_state.community_cards, ",",
                            sizeof(game_state.community_cards) - strlen(game_state.community_cards) - 1);
                }
                strncat(game_state.community_cards, card_str,
                        sizeof(game_state.community_cards) - strlen(game_state.community_cards) - 1);
            }
        }

        json_t *hand = json_object_get(payload, "hand");
        if (json_is_array(hand) && json_array_size(hand) >= 2) {
            game_state.hand[0] = card_from_string(json_string_value(json_array_get(hand, 0)));
            game_state.hand[1] = card_from_string(json_string_value(json_array_get(hand, 1)));
        }

        game_state.num_players = 0;
        json_t *players = json_object_get(payload, "players");
        if (json_is_array(players)) {
            size_t idx;
            json_t *player;
            json_array_foreach(players, idx, player) {
                if (game_state.num_players >= MAX_PLAYERS) break;
                PlayerInfo *pi = &game_state.players[game_state.num_players++];
                const char *pname = protocol_get_string(player, "name");
                snprintf(pi->name, sizeof(pi->name), "%s", pname ? pname : "");
                pi->chips = (int)json_integer_value(json_object_get(player, "chips"));
                pi->bet = (int)json_integer_value(json_object_get(player, "bet"));
                pi->folded = json_is_true(json_object_get(player, "folded"));
                pi->all_in = json_is_true(json_object_get(player, "all_in"));
                pi->is_dealer = json_is_true(json_object_get(player, "dealer"));
                pi->is_current = json_is_true(json_object_get(player, "current"));
            }
        }

        json_t *legal = json_object_get(payload, "legal");
        if (json_is_object(legal)) {
            game_state.legal_active = json_is_true(json_object_get(legal, "active"));
            game_state.can_check = json_is_true(json_object_get(legal, "can_check"));
            game_state.call_amount = (int)json_integer_value(json_object_get(legal, "call_amount"));
            game_state.min_raise = (int)json_integer_value(json_object_get(legal, "min_raise"));
            game_state.max_raise = (int)json_integer_value(json_object_get(legal, "max_raise"));
            game_state.can_raise = json_is_true(json_object_get(legal, "can_raise"));
        }

        pthread_mutex_unlock(&state_lock);
        display_game_state();
        pthread_mutex_lock(&state_lock);
    }
    else if (strcmp(type, "legal_actions") == 0) {
        game_state.legal_active = json_is_true(json_object_get(payload, "active"));
        game_state.can_check = json_is_true(json_object_get(payload, "can_check"));
        game_state.call_amount = (int)json_integer_value(json_object_get(payload, "call_amount"));
        game_state.min_raise = (int)json_integer_value(json_object_get(payload, "min_raise"));
        game_state.max_raise = (int)json_integer_value(json_object_get(payload, "max_raise"));
        game_state.can_raise = json_is_true(json_object_get(payload, "can_raise"));
    }
    else if (strcmp(type, "event") == 0) {
        const char *event = protocol_get_string(payload, "event");
        const char *player = protocol_get_string(payload, "player");
        if (event && strcmp(event, "action") == 0) {
            const char *action = protocol_get_string(payload, "action");
            int amount = (int)json_integer_value(json_object_get(payload, "amount"));
            pthread_mutex_unlock(&state_lock);
            printf("\n%s %s", player ? player : "Player", action ? action : "acts");
            if (amount > 0) {
                printf(" %d", amount);
            }
            printf("\n> ");
            fflush(stdout);
            pthread_mutex_lock(&state_lock);
        } else if (event && strcmp(event, "chat") == 0) {
            const char *text = protocol_get_string(payload, "text");
            pthread_mutex_unlock(&state_lock);
            printf("\n[%s]: %s\n> ", player ? player : "Player", text ? text : "");
            fflush(stdout);
            pthread_mutex_lock(&state_lock);
        } else if (event && strcmp(event, "player_joined") == 0) {
            int count = (int)json_integer_value(json_object_get(payload, "player_count"));
            pthread_mutex_unlock(&state_lock);
            printf("\n%s joined the game (%d players)\n> ", player ? player : "Player", count);
            fflush(stdout);
            pthread_mutex_lock(&state_lock);
        } else if (event && strcmp(event, "player_left") == 0) {
            int count = (int)json_integer_value(json_object_get(payload, "player_count"));
            pthread_mutex_unlock(&state_lock);
            printf("\n%s left the game (%d players)\n> ", player ? player : "Player", count);
            fflush(stdout);
            pthread_mutex_lock(&state_lock);
        } else if (event && strcmp(event, "timeout") == 0) {
            const char *action = protocol_get_string(payload, "action");
            pthread_mutex_unlock(&state_lock);
            printf("\n%s timed out and was auto-%s\n> ",
                   player ? player : "Player", action ? action : "acted");
            fflush(stdout);
            pthread_mutex_lock(&state_lock);
        } else if (event) {
            pthread_mutex_unlock(&state_lock);
            printf("\n%s\n> ", event);
            fflush(stdout);
            pthread_mutex_lock(&state_lock);
        }
    }
    else if (strcmp(type, "hand_result") == 0) {
        pthread_mutex_unlock(&state_lock);
        
        printf("\n======================================================================\n");
        printf("                          SHOWDOWN!\n");
        printf("======================================================================\n\n");

        json_t *winners = json_object_get(payload, "winners");
        if (json_is_array(winners)) {
            size_t idx;
            json_t *winner;
            json_array_foreach(winners, idx, winner) {
                const char *name = protocol_get_string(winner, "player");
                const char *hand_name = protocol_get_string(winner, "hand");
                const char *cards = protocol_get_string(winner, "cards");
                const char *pot_label = protocol_get_string(winner, "pot");
                int amount = (int)json_integer_value(json_object_get(winner, "amount"));
                printf("WINNER: %s\n", name ? name : "Player");
                if (hand_name && hand_name[0]) printf("  Hand: %s\n", hand_name);
                if (pot_label && pot_label[0]) printf("  Pot: %s\n", pot_label);
                if (cards && cards[0]) printf("  Cards: %s\n", cards);
                printf("  Wins: %d chips\n\n", amount);
            }
        }

        printf("======================================================================\n");
        printf("Next hand starting soon...\n");
        printf("======================================================================\n");
        printf("> ");
        fflush(stdout);
        
        pthread_mutex_lock(&state_lock);
    }
    else if (strcmp(type, "error") == 0) {
        const char *text = protocol_get_string(payload, "message");
        pthread_mutex_unlock(&state_lock);
        printf("\nERROR: %s\n> ", text ? text : "Unknown error");
        fflush(stdout);
        pthread_mutex_lock(&state_lock);
    }
    else if (strcmp(type, "info") == 0) {
        const char *text = protocol_get_string(payload, "message");
        pthread_mutex_unlock(&state_lock);
        printf("\n%s\n> ", text ? text : "");
        fflush(stdout);
        pthread_mutex_lock(&state_lock);
    }
    else if (strcmp(type, "pong") == 0) {
        pthread_mutex_unlock(&state_lock);
        printf("\nPONG\n> ");
        fflush(stdout);
        pthread_mutex_lock(&state_lock);
    }
    
    pthread_mutex_unlock(&state_lock);
    json_decref(json);
}

/* Receive thread */
void* receive_thread(void* arg) {
    (void)arg;  /* Unused parameter */
    
    char buffer[BUFFER_SIZE];
    char line_buffer[BUFFER_SIZE] = "";
    
    while (running) {
        int bytes = recv(client_socket, buffer, sizeof(buffer) - 1, 0);
        if (bytes <= 0) {
            running = false;
            printf("\nDisconnected from server\n");
            break;
        }
        
        buffer[bytes] = '\0';
        size_t used = strlen(line_buffer);
        size_t available = sizeof(line_buffer) - used - 1;
        size_t copy_len = strlen(buffer);
        if (copy_len > available) {
            copy_len = available;
        }
        memcpy(line_buffer + used, buffer, copy_len);
        line_buffer[used + copy_len] = '\0';
        
        /* Process complete lines */
        char* newline;
        while ((newline = strchr(line_buffer, '\n')) != NULL) {
            *newline = '\0';
            
            if (strlen(line_buffer) > 0) {
                parse_message(line_buffer);
            }
            
            /* Move remaining data to start of buffer */
            memmove(line_buffer, newline + 1, strlen(newline + 1) + 1);
        }
    }
    
    return NULL;
}

/* Send action to server */
void send_client_payload(const char *type, json_t *payload) {
    char id[MESSAGE_ID_LEN];
    snprintf(id, sizeof(id), "msg-%d", client_message_seq++);
    json_t *message = protocol_client_message(id, session_token, type, payload);
    if (!message) {
        return;
    }
    protocol_send_json(client_socket, message);
    json_decref(message);
}

void send_action(const char* action, int amount) {
    send_client_payload("action",
            json_pack("{s:s, s:i}", "action", action, "amount", amount));
}

void send_control(const char* command, int amount) {
    send_client_payload("table_command",
            json_pack("{s:s, s:i}", "command", command, "amount", amount));
}

/* Display help */
void display_help(void) {
    printf("\nCommands:\n");
    printf("  fold              Fold hand\n");
    printf("  check             Check (no bet required)\n");
    printf("  call              Match current bet\n");
    printf("  raise <amount>    Raise bet to total amount\n");
    printf("  chat <message>    Send message to all players\n");
    printf("  sitout            Sit out after folding current hand\n");
    printf("  sitin             Return for the next hand\n");
    printf("  rebuy [amount]    Add chips to your stack\n");
    printf("  ping              Check server heartbeat\n");
    printf("  status            Redraw current table state\n");
    printf("  help              Show this help\n");
    printf("  quit/leave        Leave game\n");
    printf("\nPlayer status:\n");
    printf("  *   You\n");
    printf("  D   Dealer\n");
    printf("  >   Current turn\n");
    printf("> ");
    fflush(stdout);
}

void print_client_help(const char *program) {
    printf("Usage: %s [--host ADDR] [--port PORT] [--name NAME]\n", program);
    printf("\nOptions:\n");
    printf("  --host ADDR  Server address (default: 127.0.0.1)\n");
    printf("  --port PORT  Server port (default: 5555)\n");
    printf("  --name NAME  Player name; skips the name prompt\n");
    printf("  --help       Show this help\n");
}

void parse_client_args(int argc, char **argv, char *server_ip, size_t server_ip_size,
                       int *port) {
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--host") == 0 && i + 1 < argc) {
            snprintf(server_ip, server_ip_size, "%s", argv[++i]);
            host_from_args = true;
        } else if (strcmp(argv[i], "--port") == 0 && i + 1 < argc) {
            *port = atoi(argv[++i]);
            port_from_args = true;
        } else if (strcmp(argv[i], "--name") == 0 && i + 1 < argc) {
            snprintf(my_name, sizeof(my_name), "%.63s", argv[++i]);
            name_from_args = true;
        } else if (strcmp(argv[i], "--help") == 0) {
            print_client_help(argv[0]);
            exit(0);
        } else {
            fprintf(stderr, "Unknown or incomplete option: %s\n", argv[i]);
            print_client_help(argv[0]);
            exit(1);
        }
    }

    if (*port <= 0 || *port > 65535) {
        fprintf(stderr, "Invalid port: %d\n", *port);
        exit(1);
    }
}

/* Main function */
int main(int argc, char* argv[]) {
    struct sockaddr_in server_addr;
    char server_ip[64] = "127.0.0.1";
    int port = 5555;
    char input[128];

    parse_client_args(argc, argv, server_ip, sizeof(server_ip), &port);
    
    printf("======================================================================\n");
    printf("                    TEXAS HOLD'EM POKER\n");
    printf("======================================================================\n\n");
    
    /* Get player name */
    if (!name_from_args) {
        printf("Enter your name: ");
        fflush(stdout);
        if (fgets(my_name, sizeof(my_name), stdin)) {
            char* newline = strchr(my_name, '\n');
            if (newline) *newline = '\0';
        }
    }
    
    if (strlen(my_name) == 0) {
        snprintf(my_name, sizeof(my_name), "Player%d", getpid());
    }
    
    /* Get server address */
    if (!host_from_args) {
        printf("Enter server address (default: 127.0.0.1): ");
        fflush(stdout);

        if (fgets(input, sizeof(input), stdin) && input[0] != '\n') {
            char* newline = strchr(input, '\n');
            if (newline) *newline = '\0';
            snprintf(server_ip, sizeof(server_ip), "%.63s", input);
        }
    }
    
    /* Get port */
    if (!port_from_args) {
        printf("Enter server port (default: 5555): ");
        fflush(stdout);

        if (fgets(input, sizeof(input), stdin) && input[0] != '\n') {
            port = atoi(input);
        }
    }
    
    /* Connect to server */
    client_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (client_socket < 0) {
        perror("Socket creation failed");
        exit(1);
    }
    
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port);
    
    if (inet_pton(AF_INET, server_ip, &server_addr.sin_addr) <= 0) {
        perror("Invalid address");
        exit(1);
    }
    
    if (connect(client_socket, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        perror("Connection failed");
        exit(1);
    }
    
    printf("\nConnected to %s:%d\n", server_ip, port);
    printf("Waiting for game to start...\n");
    printf("Commands: fold, check, call, raise <amount>, chat <message>, sitout, sitin, rebuy, ping, status, quit\n\n");

    send_client_payload("join", json_pack("{s:s}", "name", my_name));
    
    /* Start receive thread */
    pthread_t recv_thread;
    pthread_create(&recv_thread, NULL, receive_thread, NULL);
    pthread_detach(recv_thread);
    
    /* Input loop */
    char command[512];
    
    while (running) {
        if (fgets(command, sizeof(command), stdin) == NULL) {
            break;
        }
        
        /* Remove newline */
        char* newline = strchr(command, '\n');
        if (newline) *newline = '\0';
        
        if (strlen(command) == 0) continue;
        
        /* Parse command */
        char action[32];
        char rest[480] = "";
        
        sscanf(command, "%31s %479[^\n]", action, rest);
        
        if (strcmp(action, "quit") == 0 || strcmp(action, "leave") == 0) {
            running = false;
            break;
        }
        else if (strcmp(action, "help") == 0) {
            display_help();
        }
        else if (strcmp(action, "status") == 0) {
            display_game_state();
        }
        else if (strcmp(action, "fold") == 0 || strcmp(action, "check") == 0 || 
                 strcmp(action, "call") == 0) {
            send_action(action, 0);
        }
        else if (strcmp(action, "raise") == 0) {
            int amount = atoi(rest);
            if (amount > 0) {
                send_action("raise", amount);
            } else {
                printf("Usage: raise <amount>\n");
            }
        }
        else if (strcmp(action, "chat") == 0) {
            send_client_payload("chat", json_pack("{s:s}", "text", rest));
        }
        else if (strcmp(action, "ping") == 0) {
            send_client_payload("ping", json_object());
        }
        else if (strcmp(action, "sitout") == 0 || strcmp(action, "sitin") == 0) {
            send_control(action, 0);
        }
        else if (strcmp(action, "rebuy") == 0) {
            int amount = atoi(rest);
            send_control("rebuy", amount);
        }
        else {
            printf("Unknown command: %s (type 'help' for commands)\n", action);
        }
    }
    
    close(client_socket);
    printf("Disconnected\n");
    
    return 0;
}
