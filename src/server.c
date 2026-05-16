/**
 * Texas Hold'em Poker Server
 * Multi-client server with threading
 */

#include "poker.h"
#include "audit.h"
#include "ids.h"
#include "protocol.h"
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
#include <stdarg.h>
#include <signal.h>
#include <time.h>
#include <jansson.h>

#define MAX_PLAYERS 10
#define BUFFER_SIZE 4096
#define DEFAULT_PORT 5555
#define DEFAULT_HOST "0.0.0.0"
#define DEFAULT_AUDIT_DB "poker_audit.sqlite3"
#define DEFAULT_HAND_DELAY 2
#define MAX_PROTOCOL_ERRORS 3
#define SMALL_BLIND 5
#define BIG_BLIND 10
#define STARTING_CHIPS 1000

typedef enum {
    PLAYER_EMPTY = 0,
    PLAYER_ACTIVE,
    PLAYER_SITTING_OUT
} PlayerStatus;

typedef struct {
    int amount;
    bool eligible[MAX_PLAYERS];
} Pot;

/* Player structure */
typedef struct {
    int socket;
    char id[UUID_STRING_LEN];
    char session[65];
    char name[64];
    int chips;
    Card hand[2];
    int bet;
    int total_bet;
    bool folded;
    bool all_in;
    bool acted_this_round;
    bool active;
    PlayerStatus status;
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
    int min_raise;
    int dealer_index;
    int current_player_index;
    int round;  /* 0=preflop, 1=flop, 2=turn, 3=river */
    bool game_active;
    int sequence;
    char hand_id[UUID_STRING_LEN];
    AuditLog audit;
    bool audit_enabled;
    pthread_mutex_t lock;
} GameState;

GameState game;

typedef struct {
    char host[64];
    int port;
    char audit_db[256];
    int hand_delay;
} ServerConfig;

ServerConfig config = {
    DEFAULT_HOST,
    DEFAULT_PORT,
    DEFAULT_AUDIT_DB,
    DEFAULT_HAND_DELAY
};

volatile sig_atomic_t server_running = 1;
int listen_socket = -1;

/* Function prototypes */
void* handle_client(void* arg);
void* game_loop(void* arg);
void init_game(const char *audit_path);
void broadcast_message(const char* message, int exclude_socket);
void send_message(int socket, const char* message);
void send_error(int socket, const char *message);
void send_info(const char *message);
void send_event(const char *event, const char *player, const char *action,
                int amount, int total, int player_count);
void broadcast_payload(const char *type, json_t *payload, int exclude_socket);
void audit_payload(const char *type, const char *player_id, json_t *payload);
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
void advance_game_after_action(void);
int count_players_in_hand_locked(void);
int count_players_able_to_act_locked(void);
int next_eligible_player_locked(int start_idx);
int next_active_funded_player_locked(int start_idx);
void deal_remaining_community_locked(void);
void send_legal_actions_locked(int player_idx);
int build_pots_locked(Pot *pots, int max_pots);
void handle_control(int player_idx, const char* command, int amount);
json_t *build_state_payload_locked(int player_idx);
json_t *build_legal_payload_locked(int player_idx);
void send_player_state_locked(int player_idx);
int recv_line(int socket, char *line, size_t size);
bool session_matches_player(int player_idx, json_t *message);
bool message_requires_session(const char *type);
void log_message(const char *level, const char *fmt, ...);
void handle_signal(int sig);
void parse_server_args(int argc, char **argv);
void print_server_help(const char *program);
void close_all_clients(void);

void log_message(const char *level, const char *fmt, ...) {
    time_t now = time(NULL);
    struct tm tm_now;
    localtime_r(&now, &tm_now);

    char timestamp[32];
    strftime(timestamp, sizeof(timestamp), "%Y-%m-%d %H:%M:%S", &tm_now);
    fprintf(stdout, "[%s] %-5s ", timestamp, level);

    va_list args;
    va_start(args, fmt);
    vfprintf(stdout, fmt, args);
    va_end(args);

    fputc('\n', stdout);
    fflush(stdout);
}

void handle_signal(int sig) {
    (void)sig;
    server_running = 0;
    if (listen_socket >= 0) {
        close(listen_socket);
        listen_socket = -1;
    }
}

void print_server_help(const char *program) {
    printf("Usage: %s [--host ADDR] [--port PORT] [--audit-db PATH] [--hand-delay SECONDS]\n", program);
    printf("\nOptions:\n");
    printf("  --host ADDR          Bind address (default: %s)\n", DEFAULT_HOST);
    printf("  --port PORT          Bind port (default: %d)\n", DEFAULT_PORT);
    printf("  --audit-db PATH      SQLite audit database path (default: %s)\n", DEFAULT_AUDIT_DB);
    printf("  --hand-delay SECONDS Delay between hands (default: %d)\n", DEFAULT_HAND_DELAY);
    printf("  --help               Show this help\n");
}

void parse_server_args(int argc, char **argv) {
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--host") == 0 && i + 1 < argc) {
            snprintf(config.host, sizeof(config.host), "%s", argv[++i]);
        } else if (strcmp(argv[i], "--port") == 0 && i + 1 < argc) {
            config.port = atoi(argv[++i]);
        } else if (strcmp(argv[i], "--audit-db") == 0 && i + 1 < argc) {
            snprintf(config.audit_db, sizeof(config.audit_db), "%s", argv[++i]);
        } else if (strcmp(argv[i], "--hand-delay") == 0 && i + 1 < argc) {
            config.hand_delay = atoi(argv[++i]);
        } else if (strcmp(argv[i], "--help") == 0) {
            print_server_help(argv[0]);
            exit(0);
        } else {
            fprintf(stderr, "Unknown or incomplete option: %s\n", argv[i]);
            print_server_help(argv[0]);
            exit(1);
        }
    }

    if (config.port <= 0 || config.port > 65535) {
        fprintf(stderr, "Invalid port: %d\n", config.port);
        exit(1);
    }
    if (config.hand_delay < 0) {
        fprintf(stderr, "Invalid hand delay: %d\n", config.hand_delay);
        exit(1);
    }
}

/* Initialize game state */
void init_game(const char *audit_path) {
    memset(&game, 0, sizeof(GameState));
    pthread_mutex_init(&game.lock, NULL);
    game.sequence = 1;
    game.audit_enabled = audit_open(&game.audit, audit_path);
    if (!game.audit_enabled) {
        log_message("WARN", "audit disabled; failed to open %s", audit_path);
    }
    generate_uuid_string(game.hand_id, sizeof(game.hand_id));
    
    for (int i = 0; i < MAX_PLAYERS; i++) {
        game.players[i].active = false;
        game.players[i].status = PLAYER_EMPTY;
        pthread_mutex_init(&game.players[i].lock, NULL);
        
        /* Initialize hand with valid placeholder cards */
        game.players[i].hand[0].rank = TWO;
        game.players[i].hand[0].suit = CLUBS;
        game.players[i].hand[1].rank = TWO;
        game.players[i].hand[1].suit = CLUBS;
    }
    
    deck_init(&game.deck);
}

static int next_sequence(void) {
    return game.sequence++;
}

void audit_payload(const char *type, const char *player_id, json_t *payload) {
    if (!game.audit_enabled || !payload) {
        return;
    }

    char *dump = json_dumps(payload, JSON_COMPACT);
    if (!dump) {
        return;
    }
    audit_append(&game.audit, game.sequence, type, game.hand_id, player_id, dump);
    free(dump);
}

static void audit_payload_sequence(int sequence, const char *type,
                                   const char *player_id, json_t *payload) {
    if (!game.audit_enabled || !payload) {
        return;
    }

    char *dump = json_dumps(payload, JSON_COMPACT);
    if (!dump) {
        return;
    }
    audit_append(&game.audit, sequence, type, game.hand_id, player_id, dump);
    free(dump);
}

/* Send raw newline-framed text; kept for compatibility with low-level helpers. */
void send_message(int socket, const char* message) {
    send(socket, message, strlen(message), 0);
    send(socket, "\n", 1, 0);
}

void send_error(int socket, const char *message) {
    json_t *payload = json_pack("{s:s}", "message", message ? message : "Unknown error");
    protocol_send_server(socket, next_sequence(), NULL, "error", payload);
}

void send_info(const char *message) {
    broadcast_payload("info", json_pack("{s:s}", "message", message), -1);
}

void broadcast_payload(const char *type, json_t *payload, int exclude_socket) {
    pthread_mutex_lock(&game.lock);

    int audit_seq = game.sequence;
    audit_payload_sequence(audit_seq, type, NULL, payload);
    for (int i = 0; i < MAX_PLAYERS; i++) {
        if (game.players[i].active && game.players[i].socket != exclude_socket) {
            protocol_send_server(game.players[i].socket, next_sequence(), NULL,
                                 type, json_deep_copy(payload));
        }
    }

    pthread_mutex_unlock(&game.lock);
    json_decref(payload);
}

void send_event(const char *event, const char *player, const char *action,
                int amount, int total, int player_count) {
    json_t *payload = json_pack("{s:s}", "event", event);
    if (player) {
        json_object_set_new(payload, "player", json_string(player));
    }
    if (action) {
        json_object_set_new(payload, "action", json_string(action));
    }
    if (amount >= 0) {
        json_object_set_new(payload, "amount", json_integer(amount));
    }
    if (total >= 0) {
        json_object_set_new(payload, "total", json_integer(total));
    }
    if (player_count >= 0) {
        json_object_set_new(payload, "player_count", json_integer(player_count));
    }
    broadcast_payload("event", payload, -1);
}

int recv_line(int socket, char *line, size_t size) {
    size_t used = 0;
    while (used + 1 < size) {
        char c;
        int bytes = recv(socket, &c, 1, 0);
        if (bytes <= 0) {
            return bytes;
        }
        if (c == '\n') {
            line[used] = '\0';
            return (int)used;
        }
        line[used++] = c;
    }
    line[used] = '\0';
    char c;
    while (recv(socket, &c, 1, 0) > 0 && c != '\n') {
        continue;
    }
    return -2;
}

bool message_requires_session(const char *type) {
    return type &&
           (strcmp(type, "action") == 0 ||
            strcmp(type, "table_command") == 0 ||
            strcmp(type, "chat") == 0);
}

bool session_matches_player(int player_idx, json_t *message) {
    const char *session = protocol_get_string(message, "session");
    if (!session) {
        return false;
    }

    bool ok = false;
    pthread_mutex_lock(&game.lock);
    if (player_idx >= 0 && player_idx < MAX_PLAYERS && game.players[player_idx].active) {
        ok = strcmp(session, game.players[player_idx].session) == 0;
    }
    pthread_mutex_unlock(&game.lock);
    return ok;
}

void close_all_clients(void) {
    pthread_mutex_lock(&game.lock);
    for (int i = 0; i < MAX_PLAYERS; i++) {
        if (game.players[i].active) {
            close(game.players[i].socket);
            game.players[i].active = false;
            game.players[i].status = PLAYER_EMPTY;
            game.players[i].folded = true;
        }
    }
    game.num_players = 0;
    pthread_mutex_unlock(&game.lock);
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
    if (!generate_uuid_string(game.players[idx].id, sizeof(game.players[idx].id))) {
        snprintf(game.players[idx].id, sizeof(game.players[idx].id), "player-%d", idx);
    }
    if (!generate_session_token(game.players[idx].session,
                                sizeof(game.players[idx].session), 32)) {
        snprintf(game.players[idx].session, sizeof(game.players[idx].session),
                 "session-%d", idx);
    }
    snprintf(game.players[idx].name, sizeof(game.players[idx].name), "%.63s", name);
    game.players[idx].chips = STARTING_CHIPS;
    game.players[idx].active = true;
    game.players[idx].status = PLAYER_ACTIVE;
    game.players[idx].folded = false;
    game.players[idx].all_in = false;
    game.players[idx].acted_this_round = false;
    game.players[idx].bet = 0;
    game.players[idx].total_bet = 0;
    
    /* Initialize hand with placeholder cards */
    game.players[idx].hand[0].rank = TWO;
    game.players[idx].hand[0].suit = CLUBS;
    game.players[idx].hand[1].rank = TWO;
    game.players[idx].hand[1].suit = CLUBS;
    
    game.num_players++;
    int current_count = game.num_players;
    
    pthread_mutex_unlock(&game.lock);
    log_message("INFO", "player joined: %s (%d players)", name, current_count);
    
    broadcast_payload("event",
            json_pack("{s:s, s:s, s:i}",
                      "event", "player_joined",
                      "player", name,
                      "player_count", current_count),
            socket);
    
    return idx;
}

/* Remove player from game */
void remove_player(int socket) {
    pthread_mutex_lock(&game.lock);
    
    for (int i = 0; i < MAX_PLAYERS; i++) {
        if (game.players[i].active && game.players[i].socket == socket) {
            bool was_in_hand = game.game_active && !game.players[i].folded;
            char name[64];
            snprintf(name, sizeof(name), "%s", game.players[i].name);
            
            game.players[i].active = false;
            game.players[i].status = PLAYER_EMPTY;
            game.players[i].folded = true;
            game.num_players--;
            int current_count = game.num_players;
            
            pthread_mutex_unlock(&game.lock);
            log_message("INFO", "player left: %s (%d players)", name, current_count);
            send_event("player_left", name, NULL, -1, -1, current_count);
            if (was_in_hand) {
                advance_game_after_action();
            }
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

int count_players_in_hand_locked(void) {
    int count = 0;
    for (int i = 0; i < MAX_PLAYERS; i++) {
        if (game.players[i].active && game.players[i].status == PLAYER_ACTIVE &&
            !game.players[i].folded) {
            count++;
        }
    }
    return count;
}

int count_players_able_to_act_locked(void) {
    int count = 0;
    for (int i = 0; i < MAX_PLAYERS; i++) {
        if (game.players[i].active && game.players[i].status == PLAYER_ACTIVE &&
            !game.players[i].folded && !game.players[i].all_in) {
            count++;
        }
    }
    return count;
}

int next_eligible_player_locked(int start_idx) {
    for (int offset = 1; offset <= MAX_PLAYERS; offset++) {
        int idx = (start_idx + offset) % MAX_PLAYERS;
        if (game.players[idx].active && game.players[idx].status == PLAYER_ACTIVE &&
            !game.players[idx].folded && !game.players[idx].all_in) {
            return idx;
        }
    }
    return -1;
}

int next_active_funded_player_locked(int start_idx) {
    for (int offset = 1; offset <= MAX_PLAYERS; offset++) {
        int idx = (start_idx + offset) % MAX_PLAYERS;
        if (game.players[idx].active && game.players[idx].status == PLAYER_ACTIVE &&
            game.players[idx].chips > 0) {
            return idx;
        }
    }
    return -1;
}

void deal_remaining_community_locked(void) {
    while (game.num_community_cards < 5) {
        game.community_cards[game.num_community_cards++] = deck_deal(&game.deck);
    }
    game.round = 3;
}

int build_pots_locked(Pot *pots, int max_pots) {
    int levels[MAX_PLAYERS];
    int level_count = 0;
    int previous = 0;

    for (int i = 0; i < MAX_PLAYERS; i++) {
        int committed = game.players[i].total_bet;
        if (committed <= 0) {
            continue;
        }

        bool seen = false;
        for (int j = 0; j < level_count; j++) {
            if (levels[j] == committed) {
                seen = true;
                break;
            }
        }

        if (!seen && level_count < MAX_PLAYERS) {
            levels[level_count++] = committed;
        }
    }

    for (int i = 0; i < level_count - 1; i++) {
        for (int j = i + 1; j < level_count; j++) {
            if (levels[j] < levels[i]) {
                int tmp = levels[i];
                levels[i] = levels[j];
                levels[j] = tmp;
            }
        }
    }

    int pot_count = 0;
    for (int i = 0; i < level_count && pot_count < max_pots; i++) {
        int level = levels[i];
        int contributors = 0;
        int eligible_count = 0;

        memset(&pots[pot_count], 0, sizeof(Pot));

        for (int j = 0; j < MAX_PLAYERS; j++) {
            if (game.players[j].total_bet >= level) {
                contributors++;
                if (game.players[j].active && !game.players[j].folded) {
                    pots[pot_count].eligible[j] = true;
                    eligible_count++;
                }
            }
        }

        pots[pot_count].amount = (level - previous) * contributors;
        if (pots[pot_count].amount > 0 && eligible_count > 0) {
            pot_count++;
        }
        previous = level;
    }

    return pot_count;
}

/* Start a new hand */
void start_new_hand(void) {
    pthread_mutex_lock(&game.lock);
    
    /* Count active players with chips */
    int active_count = 0;
    for (int i = 0; i < MAX_PLAYERS; i++) {
        if (game.players[i].active && game.players[i].status == PLAYER_ACTIVE &&
            game.players[i].chips > 0) {
            active_count++;
        }
    }
    
    if (active_count < 2) {
        game.game_active = false;
        pthread_mutex_unlock(&game.lock);
        return;
    }
    
    /* Reset game state */
    deck_reset(&game.deck);
    game.num_community_cards = 0;
    game.pot = 0;
    game.current_bet = 0;
    game.min_raise = BIG_BLIND;
    game.round = 0;
    game.game_active = true;
    generate_uuid_string(game.hand_id, sizeof(game.hand_id));
    char hand_id[UUID_STRING_LEN];
    snprintf(hand_id, sizeof(hand_id), "%s", game.hand_id);
    
    /* Reset players */
    for (int i = 0; i < MAX_PLAYERS; i++) {
        if (game.players[i].active && game.players[i].status == PLAYER_ACTIVE &&
            game.players[i].chips > 0) {
            game.players[i].folded = false;
            game.players[i].all_in = false;
            game.players[i].acted_this_round = false;
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
    log_message("INFO", "starting hand %s with %d active players", hand_id, active_count);
    
    post_blinds();
    send_game_state();
}

/* Post blinds */
void post_blinds(void) {
    pthread_mutex_lock(&game.lock);
    
    int active_count = count_players_in_hand_locked();
    int sb_idx;
    int bb_idx;

    if (active_count == 2) {
        sb_idx = game.dealer_index;
        bb_idx = next_eligible_player_locked(sb_idx);
    } else {
        sb_idx = next_eligible_player_locked(game.dealer_index);
        bb_idx = next_eligible_player_locked(sb_idx);
    }
    
    /* Post small blind */
    int sb_amount = (game.players[sb_idx].chips < SMALL_BLIND) ? 
                    game.players[sb_idx].chips : SMALL_BLIND;
    game.players[sb_idx].chips -= sb_amount;
    game.players[sb_idx].bet = sb_amount;
    game.players[sb_idx].total_bet = sb_amount;
    game.players[sb_idx].acted_this_round = false;
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
    game.players[bb_idx].acted_this_round = false;
    game.pot += bb_amount;
    game.current_bet = bb_amount;
    game.min_raise = BIG_BLIND;
    
    if (game.players[bb_idx].chips == 0) {
        game.players[bb_idx].all_in = true;
    }
    
    /* Set first player to act */
    if (active_count == 2) {
        game.current_player_index = sb_idx;
    } else {
        game.current_player_index = next_eligible_player_locked(bb_idx);
    }
    
    pthread_mutex_unlock(&game.lock);
}

/* Send game state to all players */
void send_game_state(void) {
    pthread_mutex_lock(&game.lock);

    for (int i = 0; i < MAX_PLAYERS; i++) {
        if (!game.players[i].active) continue;
        send_player_state_locked(i);
    }
    
    pthread_mutex_unlock(&game.lock);
}

json_t *build_legal_payload_locked(int player_idx) {
    Player *player = &game.players[player_idx];
    int call_amount = game.current_bet - player->bet;
    int min_raise = game.current_bet + game.min_raise;
    int max_raise = player->bet + player->chips;

    if (call_amount < 0) {
        call_amount = 0;
    }

    bool active = game.game_active && player_idx == game.current_player_index &&
                  !player->folded && !player->all_in;
    bool can_check = active && player->bet == game.current_bet;
    bool can_call = active && call_amount > 0 && player->chips > 0;
    bool can_raise = active && max_raise > game.current_bet &&
                     (max_raise >= min_raise ||
                      max_raise == player->bet + player->chips);

    return json_pack("{s:b, s:b, s:i, s:i, s:i, s:b}",
                     "active", active,
                     "can_check", can_check,
                     "call_amount", can_call ? call_amount : 0,
                     "min_raise", active ? min_raise : 0,
                     "max_raise", active ? max_raise : 0,
                     "can_raise", can_raise);
}

json_t *build_state_payload_locked(int player_idx) {
    const char* round_names[] = {"preflop", "flop", "turn", "river"};
    json_t *community = json_array();
    json_t *hand = json_array();
    json_t *players = json_array();

    for (int j = 0; j < game.num_community_cards; j++) {
        char card_str[4];
        card_to_string(game.community_cards[j], card_str);
        json_array_append_new(community, json_string(card_str));
    }

    char card1[4], card2[4];
    card_to_string(game.players[player_idx].hand[0], card1);
    card_to_string(game.players[player_idx].hand[1], card2);
    json_array_append_new(hand, json_string(card1));
    json_array_append_new(hand, json_string(card2));

    for (int j = 0; j < MAX_PLAYERS; j++) {
        if (!game.players[j].active) continue;
        json_array_append_new(players,
                json_pack("{s:s, s:s, s:i, s:i, s:b, s:b, s:b, s:b, s:s}",
                          "id", game.players[j].id,
                          "name", game.players[j].name,
                          "chips", game.players[j].chips,
                          "bet", game.players[j].bet,
                          "folded", game.players[j].folded,
                          "all_in", game.players[j].all_in,
                          "dealer", j == game.dealer_index,
                          "current", j == game.current_player_index,
                          "status", game.players[j].status == PLAYER_SITTING_OUT ?
                                    "sitting_out" : "active"));
    }

    return json_pack("{s:i, s:i, s:s, s:o, s:o, s:i, s:i, s:b, s:o, s:o}",
                     "pot", game.pot,
                     "current_bet", game.current_bet,
                     "round", round_names[game.round],
                     "community", community,
                     "hand", hand,
                     "chips", game.players[player_idx].chips,
                     "bet", game.players[player_idx].bet,
                     "my_turn", player_idx == game.current_player_index,
                     "players", players,
                     "legal", build_legal_payload_locked(player_idx));
}

void send_player_state_locked(int player_idx) {
    json_t *legal = build_legal_payload_locked(player_idx);
    int legal_seq = next_sequence();
    protocol_send_server(game.players[player_idx].socket, legal_seq, NULL,
                         "legal_actions", legal);
    json_t *state = build_state_payload_locked(player_idx);
    int state_seq = next_sequence();
    audit_payload_sequence(state_seq, "state", game.players[player_idx].id, state);
    protocol_send_server(game.players[player_idx].socket, state_seq, NULL,
                         "state", state);
}

void send_legal_actions_locked(int player_idx) {
    json_t *legal = build_legal_payload_locked(player_idx);
    protocol_send_server(game.players[player_idx].socket, next_sequence(), NULL,
                         "legal_actions", legal);
}

/* Handle player action */
void handle_action(int player_idx, const char* action, int amount) {
    pthread_mutex_lock(&game.lock);

    if (!game.game_active) {
        pthread_mutex_unlock(&game.lock);
        send_error(game.players[player_idx].socket, "No active hand");
        return;
    }

    if (player_idx != game.current_player_index) {
        pthread_mutex_unlock(&game.lock);
        send_error(game.players[player_idx].socket, "Not your turn");
        return;
    }

    Player* player = &game.players[player_idx];

    if (player->folded || player->all_in) {
        pthread_mutex_unlock(&game.lock);
        return;
    }

    if (strcmp(action, "fold") == 0) {
        player->folded = true;
        player->acted_this_round = true;
        char player_name[64];
        snprintf(player_name, sizeof(player_name), "%s", player->name);
        pthread_mutex_unlock(&game.lock);
        log_message("INFO", "action: %s fold", player_name);
        send_event("action", player_name, "fold", 0, -1, -1);
        advance_game_after_action();
    }
    else if (strcmp(action, "call") == 0) {
        int call_amount = game.current_bet - player->bet;
        if (call_amount < 0) {
            call_amount = 0;
        }
        if (call_amount > player->chips) {
            call_amount = player->chips;
        }

        player->chips -= call_amount;
        player->bet += call_amount;
        player->total_bet += call_amount;
        player->acted_this_round = true;
        game.pot += call_amount;

        if (player->chips == 0) {
            player->all_in = true;
        }

        char player_name[64];
        snprintf(player_name, sizeof(player_name), "%s", player->name);
        pthread_mutex_unlock(&game.lock);
        log_message("INFO", "action: %s call %d", player_name, call_amount);
        send_event("action", player_name, "call", call_amount, -1, -1);
        advance_game_after_action();
    }
    else if (strcmp(action, "raise") == 0) {
        int min_target = game.current_bet + game.min_raise;
        int wager = amount - player->bet;
        bool is_all_in = wager == player->chips;
        bool full_raise = amount >= min_target;

        if (amount <= game.current_bet) {
            pthread_mutex_unlock(&game.lock);
            send_error(player->socket, "Raise must exceed current bet");
            return;
        }

        if (!full_raise && !is_all_in) {
            char error[128];
            snprintf(error, sizeof(error), "Minimum raise is %d", min_target);
            pthread_mutex_unlock(&game.lock);
            send_error(player->socket, error);
            return;
        }

        if (wager <= 0) {
            pthread_mutex_unlock(&game.lock);
            send_error(player->socket, "Raise must increase your bet");
            return;
        }

        if (wager > player->chips) {
            pthread_mutex_unlock(&game.lock);
            send_error(player->socket, "Not enough chips for that raise");
            return;
        }

        player->chips -= wager;
        player->bet += wager;
        player->total_bet += wager;
        player->acted_this_round = true;
        game.pot += wager;
        int raise_size = amount - game.current_bet;
        game.current_bet = player->bet;

        if (full_raise) {
            game.min_raise = raise_size;
            for (int i = 0; i < MAX_PLAYERS; i++) {
                if (i != player_idx && game.players[i].active &&
                    !game.players[i].folded && !game.players[i].all_in) {
                    game.players[i].acted_this_round = false;
                }
            }
        }

        if (player->chips == 0) {
            player->all_in = true;
        }

        char player_name[64];
        int current_total = game.current_bet;
        snprintf(player_name, sizeof(player_name), "%s", player->name);
        pthread_mutex_unlock(&game.lock);
        log_message("INFO", "action: %s raise %d total %d", player_name, wager, current_total);
        send_event("action", player_name, "raise", wager, current_total, -1);
        advance_game_after_action();
    }
    else if (strcmp(action, "check") == 0) {
        if (player->bet != game.current_bet) {
            pthread_mutex_unlock(&game.lock);
            send_error(player->socket, "Cannot check");
            return;
        }

        player->acted_this_round = true;
        char player_name[64];
        snprintf(player_name, sizeof(player_name), "%s", player->name);
        pthread_mutex_unlock(&game.lock);
        log_message("INFO", "action: %s check", player_name);
        send_event("action", player_name, "check", 0, -1, -1);
        advance_game_after_action();
    }
    else {
        pthread_mutex_unlock(&game.lock);
        send_error(player->socket, "Unknown action");
    }
}

void handle_control(int player_idx, const char* command, int amount) {
    char msg[256];
    bool needs_advance = false;

    pthread_mutex_lock(&game.lock);

    Player *player = &game.players[player_idx];
    char player_name[64];
    snprintf(player_name, sizeof(player_name), "%s", player->name);

    if (strcmp(command, "sitout") == 0) {
        player->status = PLAYER_SITTING_OUT;
        if (game.game_active && !player->folded) {
            player->folded = true;
            player->acted_this_round = true;
            needs_advance = true;
        }
        snprintf(msg, sizeof(msg), "INFO:%s is sitting out", player->name);
    }
    else if (strcmp(command, "sitin") == 0) {
        if (player->chips <= 0) {
            pthread_mutex_unlock(&game.lock);
            send_error(player->socket, "Rebuy before sitting in");
            return;
        }
        player->status = PLAYER_ACTIVE;
        snprintf(msg, sizeof(msg), "INFO:%s will be dealt into the next hand", player->name);
    }
    else if (strcmp(command, "rebuy") == 0) {
        if (amount <= 0) {
            amount = STARTING_CHIPS;
        }
        player->chips += amount;
        if (player->status == PLAYER_EMPTY) {
            player->status = PLAYER_ACTIVE;
        }
        snprintf(msg, sizeof(msg), "INFO:%s added %d chips", player->name, amount);
    }
    else {
        pthread_mutex_unlock(&game.lock);
        send_error(player->socket, "Unknown table command");
        return;
    }

    pthread_mutex_unlock(&game.lock);
    log_message("INFO", "table command: %s %s %d", player_name, command, amount);
    send_info(msg);

    if (needs_advance) {
        advance_game_after_action();
    } else {
        send_game_state();
    }
}

/* Move to next player */
void next_player(void) {
    advance_game_after_action();
}

void advance_game_after_action(void) {
    json_t *winner_payload = NULL;
    bool award_fold_winner = false;
    bool go_to_showdown = false;
    bool go_to_next_round = false;

    pthread_mutex_lock(&game.lock);

    if (count_players_in_hand_locked() <= 1) {
        for (int i = 0; i < MAX_PLAYERS; i++) {
            if (game.players[i].active && !game.players[i].folded) {
                game.players[i].chips += game.pot;
                json_t *winners = json_array();
                json_array_append_new(winners,
                        json_pack("{s:s, s:i, s:s, s:s, s:s}",
                                  "player", game.players[i].name,
                                  "amount", game.pot,
                                  "reason", "Everyone folded",
                                  "hand", "",
                                  "pot", "P1"));
                winner_payload = json_pack("{s:s, s:o}",
                                           "reason", "Everyone folded",
                                           "winners", winners);
                award_fold_winner = true;
                break;
            }
        }

        game.pot = 0;
        game.game_active = false;
        game.dealer_index = next_active_funded_player_locked(game.dealer_index);
        if (game.dealer_index < 0) {
            game.dealer_index = 0;
        }

        pthread_mutex_unlock(&game.lock);
        if (award_fold_winner) {
            log_message("INFO", "hand result: everyone folded");
            broadcast_payload("hand_result", winner_payload, -1);
            sleep((unsigned int)config.hand_delay);
            start_new_hand();
        }
        return;
    }

    if (count_players_able_to_act_locked() == 0) {
        deal_remaining_community_locked();
        go_to_showdown = true;
    } else if (is_betting_round_complete()) {
        go_to_next_round = true;
    } else {
        int next_idx = next_eligible_player_locked(game.current_player_index);
        if (next_idx < 0) {
            deal_remaining_community_locked();
            go_to_showdown = true;
        } else {
            game.current_player_index = next_idx;
        }
    }

    pthread_mutex_unlock(&game.lock);

    if (go_to_showdown) {
        showdown();
    } else if (go_to_next_round) {
        next_round();
    } else {
        send_game_state();
    }
}

/* Check if betting round is complete */
bool is_betting_round_complete(void) {
    int able_to_act = 0;

    for (int i = 0; i < MAX_PLAYERS; i++) {
        if (game.players[i].active && !game.players[i].folded && !game.players[i].all_in) {
            able_to_act++;

            if (!game.players[i].acted_this_round) {
                return false;
            }

            if (game.players[i].bet < game.current_bet) {
                return false;
            }
        }
    }

    return able_to_act > 0;
}

/* Advance to next round */
void next_round(void) {
    bool go_to_showdown = false;

    pthread_mutex_lock(&game.lock);

    for (int i = 0; i < MAX_PLAYERS; i++) {
        game.players[i].bet = 0;
        game.players[i].acted_this_round = false;
    }

    if (game.round == 0) {
        deck_deal_multiple(&game.deck, &game.community_cards[0], 3);
        game.num_community_cards = 3;
        game.round = 1;
    }
    else if (game.round == 1) {
        game.community_cards[3] = deck_deal(&game.deck);
        game.num_community_cards = 4;
        game.round = 2;
    }
    else if (game.round == 2) {
        game.community_cards[4] = deck_deal(&game.deck);
        game.num_community_cards = 5;
        game.round = 3;
    }
    else {
        go_to_showdown = true;
    }

    game.current_bet = 0;

    if (!go_to_showdown) {
        if (count_players_able_to_act_locked() == 0) {
            deal_remaining_community_locked();
            go_to_showdown = true;
        } else {
            game.current_player_index = next_eligible_player_locked(game.dealer_index);
        }
    }

    pthread_mutex_unlock(&game.lock);

    if (go_to_showdown) {
        showdown();
    } else {
        send_game_state();
    }
}

/* Showdown */
void showdown(void) {
    pthread_mutex_lock(&game.lock);

    deal_remaining_community_locked();

    int eligible_indices[MAX_PLAYERS];
    int eligible_count = 0;
    HandValue values[MAX_PLAYERS];
    Pot pots[MAX_PLAYERS];
    int pot_count = build_pots_locked(pots, MAX_PLAYERS);

    for (int i = 0; i < MAX_PLAYERS; i++) {
        if (game.players[i].active && !game.players[i].folded) {
            int idx = i;
            Card all_cards[7];
            all_cards[0] = game.players[idx].hand[0];
            all_cards[1] = game.players[idx].hand[1];

            for (int j = 0; j < 5; j++) {
                all_cards[j + 2] = game.community_cards[j];
            }

            values[idx] = evaluate_hand(all_cards, 7);
            eligible_indices[eligible_count++] = idx;
        }
    }

    json_t *winners_json = json_array();

    for (int pot_idx = 0; pot_idx < pot_count; pot_idx++) {
        HandValue best;
        bool best_set = false;

        for (int i = 0; i < eligible_count; i++) {
            int idx = eligible_indices[i];
            if (!pots[pot_idx].eligible[idx]) {
                continue;
            }
            if (!best_set || compare_hands(values[idx], best) > 0) {
                best = values[idx];
                best_set = true;
            }
        }

        if (!best_set) {
            continue;
        }

        int winners[MAX_PLAYERS];
        int winner_count = 0;

        for (int i = 0; i < eligible_count; i++) {
            int idx = eligible_indices[i];
            if (pots[pot_idx].eligible[idx] && compare_hands(values[idx], best) == 0) {
                winners[winner_count++] = idx;
            }
        }

        int share = pots[pot_idx].amount / winner_count;
        int remainder = pots[pot_idx].amount % winner_count;

        for (int i = 0; i < winner_count; i++) {
            int award = share + (i == 0 ? remainder : 0);
            game.players[winners[i]].chips += award;

            char card1[4], card2[4];
            card_to_string(game.players[winners[i]].hand[0], card1);
            card_to_string(game.players[winners[i]].hand[1], card2);

            char cards[16];
            char pot_label[16];
            snprintf(cards, sizeof(cards), "%s,%s", card1, card2);
            snprintf(pot_label, sizeof(pot_label), "P%d", pot_idx + 1);
            json_array_append_new(winners_json,
                    json_pack("{s:s, s:i, s:s, s:s, s:s}",
                              "player", game.players[winners[i]].name,
                              "amount", award,
                              "hand", hand_rank_name(best.rank),
                              "cards", cards,
                              "pot", pot_label));
        }
    }

    game.pot = 0;
    game.game_active = false;
    game.dealer_index = next_active_funded_player_locked(game.dealer_index);
    if (game.dealer_index < 0) {
        game.dealer_index = 0;
    }

    pthread_mutex_unlock(&game.lock);

    log_message("INFO", "hand result: showdown");
    broadcast_payload("hand_result",
            json_pack("{s:s, s:o}", "reason", "showdown", "winners", winners_json),
            -1);

    sleep((unsigned int)config.hand_delay);
    start_new_hand();
}

/* Game loop thread */
void* game_loop(void* arg) {
    (void)arg;  /* Unused parameter */
    
    while (server_running) {
        sleep(1);
        
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
    char parse_error[128];
    int protocol_errors = 0;

    int bytes = recv_line(client_socket, buffer, sizeof(buffer));
    if (bytes == -2) {
        log_message("WARN", "connection rejected: first message too large");
        send_error(client_socket, "Message too large");
        close(client_socket);
        return NULL;
    }
    if (bytes <= 0) {
        close(client_socket);
        return NULL;
    }

    json_t *join = protocol_parse(buffer, parse_error, sizeof(parse_error));
    if (!join) {
        log_message("WARN", "connection rejected: %s", parse_error);
        send_error(client_socket, parse_error);
        close(client_socket);
        return NULL;
    }

    const char *type = protocol_get_string(join, "type");
    json_t *payload = protocol_get_payload(join);
    if (!protocol_validate_client_message(join, false, parse_error, sizeof(parse_error)) ||
        strcmp(type, "join") != 0) {
        json_decref(join);
        log_message("WARN", "connection rejected: first message was %s", type ? type : "(missing)");
        send_error(client_socket,
                   strcmp(type, "join") == 0 ? parse_error : "First message must be join");
        close(client_socket);
        return NULL;
    }

    const char *requested_name = protocol_get_string(payload, "name");
    if (requested_name && requested_name[0]) {
        snprintf(name, sizeof(name), "%.63s", requested_name);
    }
    json_decref(join);
    
    /* Add player */
    int player_idx = add_player(client_socket, name);
    if (player_idx == -1) {
        send_error(client_socket, "Server full");
        close(client_socket);
        return NULL;
    }
    
    pthread_mutex_lock(&game.lock);
    json_t *welcome = json_pack("{s:s, s:s, s:s, s:i}",
                                "player_id", game.players[player_idx].id,
                                "session", game.players[player_idx].session,
                                "name", game.players[player_idx].name,
                                "chips", STARTING_CHIPS);
    protocol_send_server(client_socket, next_sequence(), NULL, "welcome", welcome);
    pthread_mutex_unlock(&game.lock);
    
    /* Handle client messages */
    while (1) {
        bytes = recv_line(client_socket, buffer, sizeof(buffer));
        if (bytes == -2) {
            log_message("WARN", "disconnecting %s: message too large", name);
            send_error(client_socket, "Message too large");
            break;
        }
        if (bytes <= 0) break;

        json_t *message = protocol_parse(buffer, parse_error, sizeof(parse_error));
        if (!message) {
            protocol_errors++;
            log_message("WARN", "protocol error from %s: %s (%d/%d)",
                        name, parse_error, protocol_errors, MAX_PROTOCOL_ERRORS);
            send_error(client_socket, parse_error);
            if (protocol_errors >= MAX_PROTOCOL_ERRORS) {
                break;
            }
            continue;
        }

        type = protocol_get_string(message, "type");
        payload = protocol_get_payload(message);
        if (!protocol_validate_client_message(message, message_requires_session(type),
                                              parse_error, sizeof(parse_error))) {
            protocol_errors++;
            log_message("WARN", "protocol error from %s: %s (%d/%d)",
                        name, parse_error, protocol_errors, MAX_PROTOCOL_ERRORS);
            send_error(client_socket, parse_error);
            json_decref(message);
            if (protocol_errors >= MAX_PROTOCOL_ERRORS) {
                break;
            }
            continue;
        }
        if (message_requires_session(type) && !session_matches_player(player_idx, message)) {
            protocol_errors++;
            log_message("WARN", "protocol error from %s: invalid session (%d/%d)",
                        name, protocol_errors, MAX_PROTOCOL_ERRORS);
            send_error(client_socket, "Invalid session");
            json_decref(message);
            if (protocol_errors >= MAX_PROTOCOL_ERRORS) {
                break;
            }
            continue;
        }
        protocol_errors = 0;

        if (strcmp(type, "action") == 0) {
            const char *action = protocol_get_string(payload, "action");
            json_t *amount_json = json_object_get(payload, "amount");
            int amount = json_is_integer(amount_json) ? (int)json_integer_value(amount_json) : 0;
            if (action) {
                handle_action(player_idx, action, amount);
            } else {
                send_error(client_socket, "Missing action");
            }
        }
        else if (strcmp(type, "table_command") == 0) {
            const char *command = protocol_get_string(payload, "command");
            json_t *amount_json = json_object_get(payload, "amount");
            int amount = json_is_integer(amount_json) ? (int)json_integer_value(amount_json) : 0;
            if (command) {
                handle_control(player_idx, command, amount);
            } else {
                send_error(client_socket, "Missing table command");
            }
        }
        else if (strcmp(type, "chat") == 0) {
            const char *text = protocol_get_string(payload, "text");
            broadcast_payload("event",
                    json_pack("{s:s, s:s, s:s}",
                              "event", "chat",
                              "player", name,
                              "text", text ? text : ""),
                    client_socket);
        }
        else if (strcmp(type, "ping") == 0) {
            protocol_send_server(client_socket, next_sequence(),
                                 protocol_get_string(message, "id"),
                                 "pong", json_object());
        }
        else {
            send_error(client_socket, "Unknown message type");
        }

        json_decref(message);
    }
    
    /* Remove player */
    remove_player(client_socket);
    close(client_socket);
    
    return NULL;
}

/* Main function */
int main(int argc, char **argv) {
    int server_socket, client_socket;
    struct sockaddr_in server_addr, client_addr;
    socklen_t client_len = sizeof(client_addr);

    parse_server_args(argc, argv);
    signal(SIGINT, handle_signal);
    signal(SIGTERM, handle_signal);
    
    /* Initialize game */
    init_game(config.audit_db);
    
    /* Create socket */
    server_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (server_socket < 0) {
        perror("Socket creation failed");
        exit(1);
    }
    listen_socket = server_socket;
    
    /* Set socket options */
    int opt = 1;
    setsockopt(server_socket, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
    
    /* Bind socket */
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    if (strcmp(config.host, "0.0.0.0") == 0) {
        server_addr.sin_addr.s_addr = INADDR_ANY;
    } else if (inet_pton(AF_INET, config.host, &server_addr.sin_addr) <= 0) {
        fprintf(stderr, "Invalid bind address: %s\n", config.host);
        close(server_socket);
        exit(1);
    }
    server_addr.sin_port = htons(config.port);
    
    if (bind(server_socket, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        perror("Bind failed");
        exit(1);
    }
    
    /* Listen */
    if (listen(server_socket, 10) < 0) {
        perror("Listen failed");
        exit(1);
    }
    
    log_message("INFO", "server started on %s:%d", config.host, config.port);
    log_message("INFO", "audit db: %s (%s)", config.audit_db,
                game.audit_enabled ? "enabled" : "disabled");
    log_message("INFO", "hand delay: %d seconds", config.hand_delay);
    log_message("INFO", "waiting for players");
    
    /* Start game loop thread */
    pthread_t game_thread;
    pthread_create(&game_thread, NULL, game_loop, NULL);
    pthread_detach(game_thread);
    
    /* Accept clients */
    while (server_running) {
        client_socket = accept(server_socket, (struct sockaddr*)&client_addr, &client_len);
        if (client_socket < 0) {
            if (server_running) {
                perror("Accept failed");
            }
            continue;
        }
        
        log_message("INFO", "new connection from %s", inet_ntoa(client_addr.sin_addr));
        
        /* Handle client in new thread */
        pthread_t client_thread;
        int* socket_ptr = malloc(sizeof(int));
        *socket_ptr = client_socket;
        
        if (pthread_create(&client_thread, NULL, handle_client, socket_ptr) != 0) {
            log_message("ERROR", "failed to create client thread");
            close(client_socket);
            free(socket_ptr);
            continue;
        }
        pthread_detach(client_thread);
    }
    
    log_message("INFO", "shutting down");
    close_all_clients();
    if (listen_socket >= 0) {
        close(listen_socket);
        listen_socket = -1;
    }
    audit_close(&game.audit);
    return 0;
}
