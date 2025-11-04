/**
 * Texas Hold'em Poker Server
 * Multi-client server with threading
 */

#include "poker.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <errno.h>
#include <stdbool.h>

#define MAX_PLAYERS 10
#define BUFFER_SIZE 4096
#define PORT 5555
#define SMALL_BLIND 5
#define BIG_BLIND 10
#define STARTING_CHIPS 1000

/* Player structure */
typedef struct {
    int socket;
    char name[64];
    int chips;
    Card hand[2];
    int bet;
    int total_bet;
    bool folded;
    bool all_in;
    bool active;
    pthread_mutex_t lock;
} Player;

/* Game state */
typedef struct {
    Player players[MAX_PLAYERS];
    int num_players;
    Deck deck;
    Card community_cards[5];
    int num_community_cards;
    int pot;
    int current_bet;
    int dealer_index;
    int current_player_index;
    int round;  /* 0=preflop, 1=flop, 2=turn, 3=river */
    bool game_active;
    pthread_mutex_t lock;
} GameState;

GameState game;

/* Function prototypes */
void* handle_client(void* arg);
void* game_loop(void* arg);
void init_game(void);
void broadcast_message(const char* message, int exclude_socket);
void send_message(int socket, const char* message);
int add_player(int socket, const char* name);
void remove_player(int socket);
void start_new_hand(void);
void post_blinds(void);
void send_game_state(void);
void handle_action(int player_idx, const char* action, int amount);
void next_player(void);
bool is_betting_round_complete(void);
void next_round(void);
void showdown(void);
int find_player_by_socket(int socket);

/* Initialize game state */
void init_game(void) {
    memset(&game, 0, sizeof(GameState));
    pthread_mutex_init(&game.lock, NULL);
    
    for (int i = 0; i < MAX_PLAYERS; i++) {
        game.players[i].active = false;
        pthread_mutex_init(&game.players[i].lock, NULL);
        
        /* Initialize hand with valid placeholder cards */
        game.players[i].hand[0].rank = TWO;
        game.players[i].hand[0].suit = CLUBS;
        game.players[i].hand[1].rank = TWO;
        game.players[i].hand[1].suit = CLUBS;
    }
    
    deck_init(&game.deck);
}

/* Send message to a client */
void send_message(int socket, const char* message) {
    char buffer[BUFFER_SIZE];
    snprintf(buffer, sizeof(buffer), "%s\n", message);
    send(socket, buffer, strlen(buffer), 0);
}

/* Broadcast message to all players except one */
void broadcast_message(const char* message, int exclude_socket) {
    pthread_mutex_lock(&game.lock);
    
    for (int i = 0; i < MAX_PLAYERS; i++) {
        if (game.players[i].active && game.players[i].socket != exclude_socket) {
            send_message(game.players[i].socket, message);
        }
    }
    
    pthread_mutex_unlock(&game.lock);
}

/* Add player to game */
int add_player(int socket, const char* name) {
    pthread_mutex_lock(&game.lock);
    
    int idx = -1;
    for (int i = 0; i < MAX_PLAYERS; i++) {
        if (!game.players[i].active) {
            idx = i;
            break;
        }
    }
    
    if (idx == -1) {
        pthread_mutex_unlock(&game.lock);
        return -1;
    }
    
    game.players[idx].socket = socket;
    strncpy(game.players[idx].name, name, sizeof(game.players[idx].name) - 1);
    game.players[idx].chips = STARTING_CHIPS;
    game.players[idx].active = true;
    game.players[idx].folded = false;
    game.players[idx].all_in = false;
    game.players[idx].bet = 0;
    game.players[idx].total_bet = 0;
    
    /* Initialize hand with placeholder cards */
    game.players[idx].hand[0].rank = TWO;
    game.players[idx].hand[0].suit = CLUBS;
    game.players[idx].hand[1].rank = TWO;
    game.players[idx].hand[1].suit = CLUBS;
    
    game.num_players++;
    
    pthread_mutex_unlock(&game.lock);
    
    /* Notify others */
    char msg[256];
    snprintf(msg, sizeof(msg), "PLAYER_JOINED:%s:%d", name, game.num_players);
    broadcast_message(msg, socket);
    
    return idx;
}

/* Remove player from game */
void remove_player(int socket) {
    pthread_mutex_lock(&game.lock);
    
    for (int i = 0; i < MAX_PLAYERS; i++) {
        if (game.players[i].active && game.players[i].socket == socket) {
            char msg[256];
            snprintf(msg, sizeof(msg), "PLAYER_LEFT:%s:%d", 
                    game.players[i].name, game.num_players - 1);
            
            game.players[i].active = false;
            game.num_players--;
            
            pthread_mutex_unlock(&game.lock);
            broadcast_message(msg, socket);
            return;
        }
    }
    
    pthread_mutex_unlock(&game.lock);
}

/* Find player index by socket */
int find_player_by_socket(int socket) {
    for (int i = 0; i < MAX_PLAYERS; i++) {
        if (game.players[i].active && game.players[i].socket == socket) {
            return i;
        }
    }
    return -1;
}

/* Start a new hand */
void start_new_hand(void) {
    pthread_mutex_lock(&game.lock);
    
    /* Count active players with chips */
    int active_count = 0;
    for (int i = 0; i < MAX_PLAYERS; i++) {
        if (game.players[i].active && game.players[i].chips > 0) {
            active_count++;
        }
    }
    
    if (active_count < 2) {
        pthread_mutex_unlock(&game.lock);
        return;
    }
    
    /* Reset game state */
    deck_reset(&game.deck);
    game.num_community_cards = 0;
    game.pot = 0;
    game.current_bet = 0;
    game.round = 0;
    
    /* Reset players */
    for (int i = 0; i < MAX_PLAYERS; i++) {
        if (game.players[i].active && game.players[i].chips > 0) {
            game.players[i].folded = false;
            game.players[i].all_in = false;
            game.players[i].bet = 0;
            game.players[i].total_bet = 0;
        } else if (game.players[i].active) {
            game.players[i].folded = true;
        }
    }
    
    /* Deal hole cards */
    for (int i = 0; i < MAX_PLAYERS; i++) {
        if (game.players[i].active && !game.players[i].folded) {
            game.players[i].hand[0] = deck_deal(&game.deck);
            game.players[i].hand[1] = deck_deal(&game.deck);
        }
    }
    
    pthread_mutex_unlock(&game.lock);
    
    post_blinds();
    send_game_state();
}

/* Post blinds */
void post_blinds(void) {
    pthread_mutex_lock(&game.lock);
    
    /* Find small and big blind positions */
    int sb_idx = (game.dealer_index + 1) % MAX_PLAYERS;
    while (!game.players[sb_idx].active || game.players[sb_idx].folded) {
        sb_idx = (sb_idx + 1) % MAX_PLAYERS;
    }
    
    int bb_idx = (sb_idx + 1) % MAX_PLAYERS;
    while (!game.players[bb_idx].active || game.players[bb_idx].folded) {
        bb_idx = (bb_idx + 1) % MAX_PLAYERS;
    }
    
    /* Post small blind */
    int sb_amount = (game.players[sb_idx].chips < SMALL_BLIND) ? 
                    game.players[sb_idx].chips : SMALL_BLIND;
    game.players[sb_idx].chips -= sb_amount;
    game.players[sb_idx].bet = sb_amount;
    game.players[sb_idx].total_bet = sb_amount;
    game.pot += sb_amount;
    
    if (game.players[sb_idx].chips == 0) {
        game.players[sb_idx].all_in = true;
    }
    
    /* Post big blind */
    int bb_amount = (game.players[bb_idx].chips < BIG_BLIND) ? 
                    game.players[bb_idx].chips : BIG_BLIND;
    game.players[bb_idx].chips -= bb_amount;
    game.players[bb_idx].bet = bb_amount;
    game.players[bb_idx].total_bet = bb_amount;
    game.pot += bb_amount;
    game.current_bet = bb_amount;
    
    if (game.players[bb_idx].chips == 0) {
        game.players[bb_idx].all_in = true;
    }
    
    /* Set first player to act */
    game.current_player_index = (bb_idx + 1) % MAX_PLAYERS;
    while (game.players[game.current_player_index].folded) {
        game.current_player_index = (game.current_player_index + 1) % MAX_PLAYERS;
    }
    
    pthread_mutex_unlock(&game.lock);
}

/* Send game state to all players */
void send_game_state(void) {
    pthread_mutex_lock(&game.lock);
    
    char buffer[BUFFER_SIZE];
    
    for (int i = 0; i < MAX_PLAYERS; i++) {
        if (!game.players[i].active) continue;
        
        /* Build game state message */
        char community[128] = "";
        for (int j = 0; j < game.num_community_cards; j++) {
            char card_str[4];
            card_to_string(game.community_cards[j], card_str);
            strcat(community, card_str);
            if (j < game.num_community_cards - 1) strcat(community, ",");
        }
        
        char hand[16];
        char card1[4], card2[4];
        card_to_string(game.players[i].hand[0], card1);
        card_to_string(game.players[i].hand[1], card2);
        snprintf(hand, sizeof(hand), "%s,%s", card1, card2);
        
        /* Build player list */
        char players_info[1024] = "";
        for (int j = 0; j < MAX_PLAYERS; j++) {
            if (!game.players[j].active) continue;
            
            char player_str[128];
            snprintf(player_str, sizeof(player_str), "%s:%d:%d:%d:%d:%d:%d",
                    game.players[j].name,
                    game.players[j].chips,
                    game.players[j].total_bet,
                    game.players[j].folded ? 1 : 0,
                    game.players[j].all_in ? 1 : 0,
                    (j == game.dealer_index) ? 1 : 0,
                    (j == game.current_player_index) ? 1 : 0);
            
            strcat(players_info, player_str);
            strcat(players_info, "|");
        }
        
        const char* round_names[] = {"preflop", "flop", "turn", "river"};
        
        snprintf(buffer, sizeof(buffer), 
                "STATE:%d:%d:%s:%s:%s:%d:%d:%s",
                game.pot,
                game.current_bet,
                round_names[game.round],
                community,
                hand,
                game.players[i].chips,
                game.players[i].total_bet,
                players_info);
        
        /* Debug output */
        printf("Sending to %s: %s\n", game.players[i].name, buffer);
        
        send_message(game.players[i].socket, buffer);
    }
    
    pthread_mutex_unlock(&game.lock);
}

/* Handle player action */
void handle_action(int player_idx, const char* action, int amount) {
    pthread_mutex_lock(&game.lock);
    
    if (player_idx != game.current_player_index) {
        pthread_mutex_unlock(&game.lock);
        send_message(game.players[player_idx].socket, "ERROR:Not your turn");
        return;
    }
    
    Player* player = &game.players[player_idx];
    
    if (player->folded || player->all_in) {
        pthread_mutex_unlock(&game.lock);
        return;
    }
    
    char msg[256];
    
    if (strcmp(action, "fold") == 0) {
        player->folded = true;
        snprintf(msg, sizeof(msg), "ACTION:%s:fold:0", player->name);
        pthread_mutex_unlock(&game.lock);
        broadcast_message(msg, -1);
        next_player();
    }
    else if (strcmp(action, "call") == 0) {
        int call_amount = game.current_bet - player->total_bet;
        if (call_amount > player->chips) {
            call_amount = player->chips;
        }
        
        player->chips -= call_amount;
        player->total_bet += call_amount;
        player->bet = call_amount;
        game.pot += call_amount;
        
        if (player->chips == 0) {
            player->all_in = true;
        }
        
        snprintf(msg, sizeof(msg), "ACTION:%s:call:%d", player->name, call_amount);
        pthread_mutex_unlock(&game.lock);
        broadcast_message(msg, -1);
        next_player();
    }
    else if (strcmp(action, "raise") == 0) {
        if (amount > player->chips) {
            amount = player->chips;
        }
        
        player->chips -= amount;
        player->total_bet += amount;
        player->bet = amount;
        game.pot += amount;
        game.current_bet = player->total_bet;
        
        if (player->chips == 0) {
            player->all_in = true;
        }
        
        snprintf(msg, sizeof(msg), "ACTION:%s:raise:%d:%d", 
                player->name, amount, game.current_bet);
        pthread_mutex_unlock(&game.lock);
        broadcast_message(msg, -1);
        next_player();
    }
    else if (strcmp(action, "check") == 0) {
        if (player->total_bet < game.current_bet) {
            pthread_mutex_unlock(&game.lock);
            send_message(player->socket, "ERROR:Cannot check");
            return;
        }
        
        snprintf(msg, sizeof(msg), "ACTION:%s:check:0", player->name);
        pthread_mutex_unlock(&game.lock);
        broadcast_message(msg, -1);
        next_player();
    }
    else {
        pthread_mutex_unlock(&game.lock);
    }
}

/* Move to next player */
void next_player(void) {
    pthread_mutex_lock(&game.lock);
    
    int starting_idx = game.current_player_index;
    
    do {
        game.current_player_index = (game.current_player_index + 1) % MAX_PLAYERS;
        
        if (!game.players[game.current_player_index].active) {
            continue;
        }
        
        if (!game.players[game.current_player_index].folded && 
            !game.players[game.current_player_index].all_in) {
            
            if (is_betting_round_complete()) {
                pthread_mutex_unlock(&game.lock);
                next_round();
                return;
            }
            break;
        }
        
        if (game.current_player_index == starting_idx) {
            pthread_mutex_unlock(&game.lock);
            next_round();
            return;
        }
    } while (1);
    
    pthread_mutex_unlock(&game.lock);
    send_game_state();
}

/* Check if betting round is complete */
bool is_betting_round_complete(void) {
    int active_count = 0;
    
    for (int i = 0; i < MAX_PLAYERS; i++) {
        if (game.players[i].active && !game.players[i].folded && 
            !game.players[i].all_in) {
            active_count++;
            
            if (game.players[i].total_bet < game.current_bet) {
                return false;
            }
        }
    }
    
    return active_count <= 1;
}

/* Advance to next round */
void next_round(void) {
    pthread_mutex_lock(&game.lock);
    
    /* Reset bets */
    for (int i = 0; i < MAX_PLAYERS; i++) {
        game.players[i].bet = 0;
    }
    
    if (game.round == 0) {  /* Preflop -> Flop */
        deck_deal_multiple(&game.deck, &game.community_cards[0], 3);
        game.num_community_cards = 3;
        game.round = 1;
    }
    else if (game.round == 1) {  /* Flop -> Turn */
        game.community_cards[3] = deck_deal(&game.deck);
        game.num_community_cards = 4;
        game.round = 2;
    }
    else if (game.round == 2) {  /* Turn -> River */
        game.community_cards[4] = deck_deal(&game.deck);
        game.num_community_cards = 5;
        game.round = 3;
    }
    else {  /* River -> Showdown */
        pthread_mutex_unlock(&game.lock);
        showdown();
        return;
    }
    
    game.current_bet = 0;
    
    /* First to act after dealer */
    game.current_player_index = (game.dealer_index + 1) % MAX_PLAYERS;
    while (game.players[game.current_player_index].folded || 
           game.players[game.current_player_index].all_in) {
        game.current_player_index = (game.current_player_index + 1) % MAX_PLAYERS;
    }
    
    pthread_mutex_unlock(&game.lock);
    send_game_state();
}

/* Showdown */
void showdown(void) {
    pthread_mutex_lock(&game.lock);
    
    /* Find active players */
    int active_indices[MAX_PLAYERS];
    int active_count = 0;
    
    for (int i = 0; i < MAX_PLAYERS; i++) {
        if (game.players[i].active && !game.players[i].folded) {
            active_indices[active_count++] = i;
        }
    }
    
    char msg[BUFFER_SIZE];
    
    if (active_count == 1) {
        /* Only one player left */
        int winner_idx = active_indices[0];
        game.players[winner_idx].chips += game.pot;
        
        snprintf(msg, sizeof(msg), "WINNER:%s:%d:Everyone folded",
                game.players[winner_idx].name, game.pot);
    }
    else {
        /* Evaluate hands */
        HandValue values[MAX_PLAYERS];
        Card all_cards[7];
        
        for (int i = 0; i < active_count; i++) {
            int idx = active_indices[i];
            all_cards[0] = game.players[idx].hand[0];
            all_cards[1] = game.players[idx].hand[1];
            
            for (int j = 0; j < 5; j++) {
                all_cards[j + 2] = game.community_cards[j];
            }
            
            values[idx] = evaluate_hand(all_cards, 7);
        }
        
        /* Find best hand */
        HandValue best = values[active_indices[0]];
        
        for (int i = 1; i < active_count; i++) {
            int idx = active_indices[i];
            if (compare_hands(values[idx], best) > 0) {
                best = values[idx];
            }
        }
        
        /* Find all winners (ties) */
        int winners[MAX_PLAYERS];
        int winner_count = 0;
        
        for (int i = 0; i < active_count; i++) {
            int idx = active_indices[i];
            if (compare_hands(values[idx], best) == 0) {
                winners[winner_count++] = idx;
            }
        }
        
        /* Split pot */
        int share = game.pot / winner_count;
        
        snprintf(msg, sizeof(msg), "WINNER:");
        for (int i = 0; i < winner_count; i++) {
            game.players[winners[i]].chips += share;
            
            char card1[4], card2[4];
            card_to_string(game.players[winners[i]].hand[0], card1);
            card_to_string(game.players[winners[i]].hand[1], card2);
            
            char winner_info[256];
            snprintf(winner_info, sizeof(winner_info), "%s:%d:%s:%s,%s|",
                    game.players[winners[i]].name,
                    share,
                    hand_rank_name(best.rank),
                    card1, card2);
            
            strcat(msg, winner_info);
        }
    }
    
    /* Move dealer button */
    game.dealer_index = (game.dealer_index + 1) % MAX_PLAYERS;
    
    pthread_mutex_unlock(&game.lock);
    
    broadcast_message(msg, -1);
    
    /* Wait and start new hand */
    sleep(5);
    start_new_hand();
}

/* Game loop thread */
void* game_loop(void* arg) {
    (void)arg;  /* Unused parameter */
    
    while (1) {
        sleep(10);
        
        pthread_mutex_lock(&game.lock);
        int active_count = 0;
        for (int i = 0; i < MAX_PLAYERS; i++) {
            if (game.players[i].active && game.players[i].chips > 0) {
                active_count++;
            }
        }
        
        bool should_start = !game.game_active && active_count >= 2;
        pthread_mutex_unlock(&game.lock);
        
        if (should_start) {
            printf("Starting new hand...\n");
            game.game_active = true;
            start_new_hand();
        }
    }
    return NULL;
}

/* Client handler thread */
void* handle_client(void* arg) {
    int client_socket = *(int*)arg;
    free(arg);
    
    char buffer[BUFFER_SIZE];
    char name[64] = "Unknown";
    
    /* Request player name */
    send_message(client_socket, "NAME_REQUEST");
    
    int bytes = recv(client_socket, buffer, sizeof(buffer) - 1, 0);
    if (bytes > 0) {
        buffer[bytes] = '\0';
        
        /* Parse name */
        char* newline = strchr(buffer, '\n');
        if (newline) *newline = '\0';
        
        if (strncmp(buffer, "NAME:", 5) == 0) {
            strncpy(name, buffer + 5, sizeof(name) - 1);
        }
    }
    
    /* Add player */
    int player_idx = add_player(client_socket, name);
    if (player_idx == -1) {
        send_message(client_socket, "ERROR:Server full");
        close(client_socket);
        return NULL;
    }
    
    char welcome[256];
    snprintf(welcome, sizeof(welcome), "WELCOME:%s:%d", name, STARTING_CHIPS);
    send_message(client_socket, welcome);
    
    /* Handle client messages */
    while (1) {
        bytes = recv(client_socket, buffer, sizeof(buffer) - 1, 0);
        if (bytes <= 0) break;
        
        buffer[bytes] = '\0';
        
        /* Parse command */
        char* newline = strchr(buffer, '\n');
        if (newline) *newline = '\0';
        
        if (strncmp(buffer, "ACTION:", 7) == 0) {
            char action[32];
            int amount = 0;
            
            sscanf(buffer + 7, "%31[^:]:%d", action, &amount);
            handle_action(player_idx, action, amount);
        }
        else if (strncmp(buffer, "CHAT:", 5) == 0) {
            char msg[512];
            snprintf(msg, sizeof(msg), "CHAT:%s:%s", name, buffer + 5);
            broadcast_message(msg, client_socket);
        }
    }
    
    /* Remove player */
    remove_player(client_socket);
    close(client_socket);
    
    return NULL;
}

/* Main function */
int main(void) {
    int server_socket, client_socket;
    struct sockaddr_in server_addr, client_addr;
    socklen_t client_len = sizeof(client_addr);
    
    /* Initialize game */
    init_game();
    
    /* Create socket */
    server_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (server_socket < 0) {
        perror("Socket creation failed");
        exit(1);
    }
    
    /* Set socket options */
    int opt = 1;
    setsockopt(server_socket, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
    
    /* Bind socket */
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(PORT);
    
    if (bind(server_socket, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        perror("Bind failed");
        exit(1);
    }
    
    /* Listen */
    if (listen(server_socket, 10) < 0) {
        perror("Listen failed");
        exit(1);
    }
    
    printf("Poker server started on port %d\n", PORT);
    printf("Waiting for players...\n");
    
    /* Start game loop thread */
    pthread_t game_thread;
    pthread_create(&game_thread, NULL, game_loop, NULL);
    pthread_detach(game_thread);
    
    /* Accept clients */
    while (1) {
        client_socket = accept(server_socket, (struct sockaddr*)&client_addr, &client_len);
        if (client_socket < 0) {
            perror("Accept failed");
            continue;
        }
        
        printf("New connection from %s\n", inet_ntoa(client_addr.sin_addr));
        
        /* Handle client in new thread */
        pthread_t client_thread;
        int* socket_ptr = malloc(sizeof(int));
        *socket_ptr = client_socket;
        
        pthread_create(&client_thread, NULL, handle_client, socket_ptr);
        pthread_detach(client_thread);
    }
    
    close(server_socket);
    return 0;
}
