/**
 * Texas Hold'em Poker - Core Game Logic Header
 * Card handling, deck management, and hand evaluation
 */

#ifndef POKER_H
#define POKER_H

#include <stdint.h>
#include <stdbool.h>

/* Card suits */
typedef enum {
    CLUBS = 0,
    DIAMONDS = 1,
    HEARTS = 2,
    SPADES = 3
} Suit;

/* Card ranks */
typedef enum {
    TWO = 2,
    THREE = 3,
    FOUR = 4,
    FIVE = 5,
    SIX = 6,
    SEVEN = 7,
    EIGHT = 8,
    NINE = 9,
    TEN = 10,
    JACK = 11,
    QUEEN = 12,
    KING = 13,
    ACE = 14
} Rank;

/* Hand rankings */
typedef enum {
    HIGH_CARD = 0,
    ONE_PAIR = 1,
    TWO_PAIR = 2,
    THREE_OF_A_KIND = 3,
    STRAIGHT = 4,
    FLUSH = 5,
    FULL_HOUSE = 6,
    FOUR_OF_A_KIND = 7,
    STRAIGHT_FLUSH = 8,
    ROYAL_FLUSH = 9
} HandRank;

/* Card structure */
typedef struct {
    Rank rank;
    Suit suit;
} Card;

/* Deck structure */
typedef struct {
    Card cards[52];
    int cards_remaining;
} Deck;

/* Hand evaluation result */
typedef struct {
    HandRank rank;
    int tiebreakers[5];  /* Up to 5 tiebreaker values */
    int num_tiebreakers;
} HandValue;

/* Function prototypes */

/* Card functions */
void card_to_string(Card card, char *buffer);
Card card_from_string(const char *str);
char rank_to_char(Rank rank);
char suit_to_char(Suit suit);
Rank char_to_rank(char c);
Suit char_to_suit(char c);

/* Deck functions */
void deck_init(Deck *deck);
void deck_shuffle(Deck *deck);
Card deck_deal(Deck *deck);
void deck_deal_multiple(Deck *deck, Card *cards, int count);
void deck_reset(Deck *deck);

/* Hand evaluation functions */
HandValue evaluate_hand(Card *cards, int num_cards);
int compare_hands(HandValue h1, HandValue h2);
const char* hand_rank_name(HandRank rank);

/* Helper functions for hand evaluation */
void sort_cards_by_rank(Card *cards, int count);
bool is_flush(Card *cards, int count);
bool is_straight(Card *cards, int count, int *high_card);
void count_ranks(Card *cards, int count, int *rank_counts);

#endif /* POKER_H */
