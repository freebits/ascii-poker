#define main server_main
#include "../src/server.c"
#undef main

#include <assert.h>
#include <stdio.h>

static void reset_test_game(void) {
    init_game();
    for (int i = 0; i < MAX_PLAYERS; i++) {
        game.players[i].active = false;
        game.players[i].status = PLAYER_EMPTY;
        game.players[i].folded = true;
        game.players[i].all_in = false;
        game.players[i].acted_this_round = false;
        game.players[i].chips = 0;
        game.players[i].bet = 0;
        game.players[i].total_bet = 0;
    }
}

static void seat_player(int idx, const char *name, int committed, bool folded) {
    game.players[idx].active = true;
    game.players[idx].status = PLAYER_ACTIVE;
    snprintf(game.players[idx].name, sizeof(game.players[idx].name), "%s", name);
    game.players[idx].folded = folded;
    game.players[idx].total_bet = committed;
}

static void test_side_pot_with_folded_contributor(void) {
    Pot pots[MAX_PLAYERS];

    reset_test_game();
    seat_player(0, "Alice", 100, false);
    seat_player(1, "Bob", 250, false);
    seat_player(2, "Carol", 250, true);

    int pot_count = build_pots_locked(pots, MAX_PLAYERS);

    assert(pot_count == 2);
    assert(pots[0].amount == 300);
    assert(pots[0].eligible[0]);
    assert(pots[0].eligible[1]);
    assert(!pots[0].eligible[2]);
    assert(pots[1].amount == 300);
    assert(!pots[1].eligible[0]);
    assert(pots[1].eligible[1]);
    assert(!pots[1].eligible[2]);
}

static void test_three_way_all_in_layers(void) {
    Pot pots[MAX_PLAYERS];

    reset_test_game();
    seat_player(0, "Alice", 50, false);
    seat_player(1, "Bob", 120, false);
    seat_player(2, "Carol", 300, false);

    int pot_count = build_pots_locked(pots, MAX_PLAYERS);

    assert(pot_count == 3);
    assert(pots[0].amount == 150);
    assert(pots[0].eligible[0]);
    assert(pots[0].eligible[1]);
    assert(pots[0].eligible[2]);
    assert(pots[1].amount == 140);
    assert(!pots[1].eligible[0]);
    assert(pots[1].eligible[1]);
    assert(pots[1].eligible[2]);
    assert(pots[2].amount == 180);
    assert(!pots[2].eligible[0]);
    assert(!pots[2].eligible[1]);
    assert(pots[2].eligible[2]);
}

int main(void) {
    test_side_pot_with_folded_contributor();
    test_three_way_all_in_layers();
    printf("rules: side-pot tests passed\n");
    return 0;
}
