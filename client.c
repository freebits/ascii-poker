/**
 * Texas Hold'em Poker Client
 * ASCII terminal interface
 */

#define _POSIX_C_SOURCE 200809L
#include "poker.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <termios.h>

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
} GameState;

int client_socket;
GameState game_state;
char my_name[64];
pthread_mutex_t state_lock = PTHREAD_MUTEX_INITIALIZER;
bool running = true;

/* Function prototypes */
void* receive_thread(void* arg);
void parse_message(const char* message);
void display_game_state(void);
void clear_screen(void);
void send_action(const char* action, int amount);
void display_help(void);

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
        int call_amount = game_state.current_bet - game_state.bet;
        int min_raise = game_state.current_bet * 2;
        
        printf("\n** YOUR TURN **\n");
        if (call_amount > 0) {
            printf("Actions: fold | call %d | raise <amt> (min %d)\n", call_amount, min_raise);
        } else {
            printf("Actions: fold | check | raise <amt> (min %d)\n", min_raise);
        }
    }
    
    printf("> ");
    fflush(stdout);
    
    pthread_mutex_unlock(&state_lock);
}

/* Parse server message */
void parse_message(const char* message) {
    pthread_mutex_lock(&state_lock);
    
    if (strncmp(message, "NAME_REQUEST", 12) == 0) {
        char buffer[128];
        snprintf(buffer, sizeof(buffer), "NAME:%s\n", my_name);
        send(client_socket, buffer, strlen(buffer), 0);
    }
    else if (strncmp(message, "WELCOME:", 8) == 0) {
        char name[64];
        int chips;
        sscanf(message + 8, "%63[^:]:%d", name, &chips);
        
        clear_screen();
        printf("======================================================================\n");
        printf("Welcome to Texas Hold'em, %s!\n", name);
        printf("Starting chips: %d\n", chips);
        printf("======================================================================\n");
        printf("Waiting for game to start...\n");
    }
    else if (strncmp(message, "STATE:", 6) == 0) {
        /* Debug: Print raw message */
        fprintf(stderr, "DEBUG STATE: %s\n", message);
        
        /* Parse game state */
        char* data = strdup(message + 6);
        
        /* Parse fixed fields first: pot:bet:round:community:hand:chips:bet:players */
        int pot, current_bet, chips, bet;
        char round[16] = "";
        char community[128] = "";
        char hand_str[32] = "";
        char* players_start = NULL;
        
        /* Manually parse the first 7 colon-separated fields */
        char* p = data;
        int field = 0;
        char* field_starts[8];
        
        field_starts[field++] = p;
        while (*p && field < 8) {
            if (*p == ':') {
                *p = '\0';
                p++;
                field_starts[field++] = p;
            } else {
                p++;
            }
        }
        
        fprintf(stderr, "DEBUG: Parsed %d fields\n", field);
        
        if (field >= 8) {
            /* Now we have the 8 fields split */
            game_state.pot = atoi(field_starts[0]);
            game_state.current_bet = atoi(field_starts[1]);
            strncpy(game_state.round, field_starts[2], sizeof(game_state.round) - 1);
            strncpy(game_state.community_cards, field_starts[3], sizeof(game_state.community_cards) - 1);
            strncpy(hand_str, field_starts[4], sizeof(hand_str) - 1);
            game_state.chips = atoi(field_starts[5]);
            game_state.bet = atoi(field_starts[6]);
            players_start = field_starts[7];
            
            fprintf(stderr, "DEBUG: hand_str='%s', community='%s'\n", hand_str, field_starts[3]);
            
            /* Parse hand */
            char* comma = strchr(hand_str, ',');
            if (comma) {
                *comma = '\0';
                fprintf(stderr, "DEBUG: card1='%s', card2='%s'\n", hand_str, comma + 1);
                game_state.hand[0] = card_from_string(hand_str);
                game_state.hand[1] = card_from_string(comma + 1);
            } else {
                fprintf(stderr, "DEBUG: No comma found in hand_str!\n");
            }
            
            /* Parse players - they are separated by | */
            game_state.num_players = 0;
            
            if (players_start && strlen(players_start) > 0) {
                char* player_token = strtok(players_start, "|");
                while (player_token && game_state.num_players < MAX_PLAYERS) {
                    PlayerInfo* pi = &game_state.players[game_state.num_players];
                    
                    char name[64];
                    int pchips, pbet, folded, all_in, is_dealer, is_current;
                    
                    if (sscanf(player_token, "%63[^:]:%d:%d:%d:%d:%d:%d",
                              name, &pchips, &pbet, &folded, &all_in, &is_dealer, &is_current) == 7) {
                        strncpy(pi->name, name, sizeof(pi->name) - 1);
                        pi->chips = pchips;
                        pi->bet = pbet;
                        pi->folded = folded != 0;
                        pi->all_in = all_in != 0;
                        pi->is_dealer = is_dealer != 0;
                        pi->is_current = is_current != 0;
                        
                        if (is_current && strcmp(name, my_name) == 0) {
                            game_state.my_turn = true;
                        } else if (is_current) {
                            game_state.my_turn = false;
                        }
                        
                        game_state.num_players++;
                    }
                    
                    player_token = strtok(NULL, "|");
                }
            }
        }
        
        free(data);
        
        pthread_mutex_unlock(&state_lock);
        display_game_state();
        pthread_mutex_lock(&state_lock);
    }
    else if (strncmp(message, "ACTION:", 7) == 0) {
        char player[64], action[32];
        int amount = 0;
        
        sscanf(message + 7, "%63[^:]:%31[^:]:%d", player, action, &amount);
        
        pthread_mutex_unlock(&state_lock);
        
        printf("\n%s %s", player, action);
        if (amount > 0) {
            printf(" %d", amount);
        }
        printf("\n> ");
        fflush(stdout);
        
        pthread_mutex_lock(&state_lock);
    }
    else if (strncmp(message, "WINNER:", 7) == 0) {
        pthread_mutex_unlock(&state_lock);
        
        printf("\n======================================================================\n");
        printf("                          SHOWDOWN!\n");
        printf("======================================================================\n\n");
        
        char* winners = strdup(message + 7);
        char* winner_token = strtok(winners, "|");
        
        while (winner_token) {
            char name[64], hand_name[64], cards[32];
            int amount;
            
            if (sscanf(winner_token, "%63[^:]:%d:%63[^:]:%31s", 
                      name, &amount, hand_name, cards) >= 3) {
                printf("WINNER: %s\n", name);
                printf("  Hand: %s\n", hand_name);
                if (strlen(cards) > 0) {
                    printf("  Cards: %s\n", cards);
                }
                printf("  Wins: %d chips\n\n", amount);
            }
            
            winner_token = strtok(NULL, "|");
        }
        
        free(winners);
        
        printf("======================================================================\n");
        printf("Next hand starting soon...\n");
        printf("======================================================================\n");
        printf("> ");
        fflush(stdout);
        
        pthread_mutex_lock(&state_lock);
    }
    else if (strncmp(message, "PLAYER_JOINED:", 14) == 0) {
        char name[64];
        int count;
        sscanf(message + 14, "%63[^:]:%d", name, &count);
        
        pthread_mutex_unlock(&state_lock);
        printf("\n%s joined the game (%d players)\n> ", name, count);
        fflush(stdout);
        pthread_mutex_lock(&state_lock);
    }
    else if (strncmp(message, "PLAYER_LEFT:", 12) == 0) {
        char name[64];
        int count;
        sscanf(message + 12, "%63[^:]:%d", name, &count);
        
        pthread_mutex_unlock(&state_lock);
        printf("\n%s left the game (%d players)\n> ", name, count);
        fflush(stdout);
        pthread_mutex_lock(&state_lock);
    }
    else if (strncmp(message, "CHAT:", 5) == 0) {
        char player[64], msg[512];
        sscanf(message + 5, "%63[^:]:%511[^\n]", player, msg);
        
        pthread_mutex_unlock(&state_lock);
        printf("\n[%s]: %s\n> ", player, msg);
        fflush(stdout);
        pthread_mutex_lock(&state_lock);
    }
    else if (strncmp(message, "ERROR:", 6) == 0) {
        pthread_mutex_unlock(&state_lock);
        printf("\nERROR: %s\n> ", message + 6);
        fflush(stdout);
        pthread_mutex_lock(&state_lock);
    }
    
    pthread_mutex_unlock(&state_lock);
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
        strcat(line_buffer, buffer);
        
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
void send_action(const char* action, int amount) {
    char buffer[256];
    snprintf(buffer, sizeof(buffer), "ACTION:%s:%d\n", action, amount);
    send(client_socket, buffer, strlen(buffer), 0);
}

/* Display help */
void display_help(void) {
    printf("\nCommands:\n");
    printf("  fold              Fold hand\n");
    printf("  check             Check (no bet required)\n");
    printf("  call              Match current bet\n");
    printf("  raise <amount>    Raise bet to <amount>\n");
    printf("  chat <message>    Send message to all players\n");
    printf("  help              Show this help\n");
    printf("  quit              Leave game\n");
    printf("\nPlayer status:\n");
    printf("  *   You\n");
    printf("  D   Dealer\n");
    printf("  >   Current turn\n");
    printf("> ");
    fflush(stdout);
}

/* Main function */
int main(int argc, char* argv[]) {
    (void)argc;  /* Unused parameters */
    (void)argv;
    
    struct sockaddr_in server_addr;
    char server_ip[64] = "127.0.0.1";
    int port = 5555;
    
    printf("======================================================================\n");
    printf("                    TEXAS HOLD'EM POKER\n");
    printf("======================================================================\n\n");
    
    /* Get player name */
    printf("Enter your name: ");
    fflush(stdout);
    if (fgets(my_name, sizeof(my_name), stdin)) {
        char* newline = strchr(my_name, '\n');
        if (newline) *newline = '\0';
    }
    
    if (strlen(my_name) == 0) {
        snprintf(my_name, sizeof(my_name), "Player%d", getpid());
    }
    
    /* Get server address */
    printf("Enter server address (default: 127.0.0.1): ");
    fflush(stdout);
    
    char input[128];
    if (fgets(input, sizeof(input), stdin) && input[0] != '\n') {
        char* newline = strchr(input, '\n');
        if (newline) *newline = '\0';
        strncpy(server_ip, input, sizeof(server_ip) - 1);
    }
    
    /* Get port */
    printf("Enter server port (default: 5555): ");
    fflush(stdout);
    
    if (fgets(input, sizeof(input), stdin) && input[0] != '\n') {
        port = atoi(input);
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
    printf("Commands: fold, check, call, raise <amount>, chat <message>, quit\n\n");
    
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
        char rest[480];
        
        sscanf(command, "%31s %479[^\n]", action, rest);
        
        if (strcmp(action, "quit") == 0) {
            running = false;
            break;
        }
        else if (strcmp(action, "help") == 0) {
            display_help();
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
            char buffer[512];
            snprintf(buffer, sizeof(buffer), "CHAT:%s\n", rest);
            send(client_socket, buffer, strlen(buffer), 0);
        }
        else {
            printf("Unknown command: %s (type 'help' for commands)\n", action);
        }
    }
    
    close(client_socket);
    printf("Disconnected\n");
    
    return 0;
}
