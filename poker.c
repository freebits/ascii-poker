/**
 * Texas Hold'em Poker - Core Game Logic Implementation
 */

#include "poker.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <limits.h>

/* Card functions */

char rank_to_char(Rank rank) {
    switch (rank) {
        case TWO: return '2';
        case THREE: return '3';
        case FOUR: return '4';
        case FIVE: return '5';
        case SIX: return '6';
        case SEVEN: return '7';
        case EIGHT: return '8';
        case NINE: return '9';
        case TEN: return 'T';
        case JACK: return 'J';
        case QUEEN: return 'Q';
        case KING: return 'K';
        case ACE: return 'A';
        default: return '?';
    }
}

char suit_to_char(Suit suit) {
    switch (suit) {
        case CLUBS: return 'C';
        case DIAMONDS: return 'D';
        case HEARTS: return 'H';
        case SPADES: return 'S';
        default: return '?';
    }
}

Rank char_to_rank(char c) {
    switch (c) {
        case '2': return TWO;
        case '3': return THREE;
        case '4': return FOUR;
        case '5': return FIVE;
        case '6': return SIX;
        case '7': return SEVEN;
        case '8': return EIGHT;
        case '9': return NINE;
        case 'T': case 't': return TEN;
        case 'J': case 'j': return JACK;
        case 'Q': case 'q': return QUEEN;
        case 'K': case 'k': return KING;
        case 'A': case 'a': return ACE;
        default: return TWO;
    }
}

Suit char_to_suit(char c) {
    switch (c) {
        case 'C': case 'c': return CLUBS;
        case 'D': case 'd': return DIAMONDS;
        case 'H': case 'h': return HEARTS;
        case 'S': case 's': return SPADES;
        default: return CLUBS;
    }
}

void card_to_string(Card card, char *buffer) {
    buffer[0] = rank_to_char(card.rank);
    buffer[1] = suit_to_char(card.suit);
    buffer[2] = '\0';
}

Card card_from_string(const char *str) {
    Card card;
    card.rank = char_to_rank(str[0]);
    card.suit = char_to_suit(str[1]);
    return card;
}

/* Deck functions */

void deck_init(Deck *deck) {
    int index = 0;
    for (Suit suit = CLUBS; suit <= SPADES; suit++) {
        for (Rank rank = TWO; rank <= ACE; rank++) {
            deck->cards[index].rank = rank;
            deck->cards[index].suit = suit;
            index++;
        }
    }
    deck->cards_remaining = 52;
}

void deck_shuffle(Deck *deck) {
    /* Use /dev/urandom for cryptographically secure random numbers */
    FILE *urandom = fopen("/dev/urandom", "rb");
    if (!urandom) {
        /* Fallback to time-based seed if /dev/urandom unavailable */
        srand(time(NULL));
        for (int i = 51; i > 0; i--) {
            int j = rand() % (i + 1);
            Card temp = deck->cards[i];
            deck->cards[i] = deck->cards[j];
            deck->cards[j] = temp;
        }
        return;
    }
    
    /* Fisher-Yates shuffle with cryptographically secure random numbers */
    for (int i = 51; i > 0; i--) {
        unsigned int rand_val;
        if (fread(&rand_val, sizeof(rand_val), 1, urandom) != 1) {
            /* If read fails, close and fallback */
            fclose(urandom);
            srand(time(NULL));
            for (int k = i; k > 0; k--) {
                int j = rand() % (k + 1);
                Card temp = deck->cards[k];
                deck->cards[k] = deck->cards[j];
                deck->cards[j] = temp;
            }
            return;
        }
        
        /* Use rejection sampling to avoid modulo bias */
        unsigned int limit = UINT_MAX - (UINT_MAX % (i + 1));
        while (rand_val >= limit) {
            if (fread(&rand_val, sizeof(rand_val), 1, urandom) != 1) {
                fclose(urandom);
                srand(time(NULL));
                for (int k = i; k > 0; k--) {
                    int j = rand() % (k + 1);
                    Card temp = deck->cards[k];
                    deck->cards[k] = deck->cards[j];
                    deck->cards[j] = temp;
                }
                return;
            }
        }
        
        int j = rand_val % (i + 1);
        Card temp = deck->cards[i];
        deck->cards[i] = deck->cards[j];
        deck->cards[j] = temp;
    }
    
    fclose(urandom);
}

void deck_reset(Deck *deck) {
    deck_init(deck);
    deck_shuffle(deck);
}

Card deck_deal(Deck *deck) {
    if (deck->cards_remaining <= 0) {
        Card invalid = {TWO, CLUBS};
        return invalid;
    }
    deck->cards_remaining--;
    return deck->cards[deck->cards_remaining];
}

void deck_deal_multiple(Deck *deck, Card *cards, int count) {
    for (int i = 0; i < count; i++) {
        cards[i] = deck_deal(deck);
    }
}

/* Helper functions */

static int compare_cards_desc(const void *a, const void *b) {
    Card *ca = (Card *)a;
    Card *cb = (Card *)b;
    return cb->rank - ca->rank;
}

void sort_cards_by_rank(Card *cards, int count) {
    qsort(cards, count, sizeof(Card), compare_cards_desc);
}

bool is_flush(Card *cards, int count) {
    if (count < 5) return false;
    
    Suit first_suit = cards[0].suit;
    for (int i = 1; i < count; i++) {
        if (cards[i].suit != first_suit) {
            return false;
        }
    }
    return true;
}

bool is_straight(Card *cards, int count, int *high_card) {
    if (count < 5) return false;
    
    /* Remove duplicates and sort */
    int unique_ranks[13] = {0};
    for (int i = 0; i < count; i++) {
        unique_ranks[cards[i].rank - 2] = 1;
    }
    
    /* Check for 5 consecutive cards */
    int consecutive = 0;
    int high = 0;
    
    for (int i = 12; i >= 0; i--) {
        if (unique_ranks[i]) {
            consecutive++;
            if (consecutive == 1) {
                high = i + 2;
            }
            if (consecutive == 5) {
                *high_card = high;
                return true;
            }
        } else {
            consecutive = 0;
        }
    }
    
    /* Check for wheel (A-2-3-4-5) */
    if (unique_ranks[12] && unique_ranks[0] && unique_ranks[1] && 
        unique_ranks[2] && unique_ranks[3]) {
        *high_card = FIVE;
        return true;
    }
    
    return false;
}

void count_ranks(Card *cards, int count, int *rank_counts) {
    memset(rank_counts, 0, 13 * sizeof(int));
    for (int i = 0; i < count; i++) {
        rank_counts[cards[i].rank - 2]++;
    }
}

/* Hand evaluation */

static HandValue evaluate_five_cards(Card *cards) {
    HandValue result;
    result.num_tiebreakers = 0;
    
    Card sorted[5];
    memcpy(sorted, cards, 5 * sizeof(Card));
    sort_cards_by_rank(sorted, 5);
    
    int rank_counts[13];
    count_ranks(sorted, 5, rank_counts);
    
    /* Count pairs, trips, quads */
    int pairs = 0, trips = 0, quads = 0;
    int pair_ranks[2] = {0, 0};
    int trip_rank = 0, quad_rank = 0;
    
    for (int i = 12; i >= 0; i--) {
        if (rank_counts[i] == 4) {
            quads++;
            quad_rank = i + 2;
        } else if (rank_counts[i] == 3) {
            trips++;
            trip_rank = i + 2;
        } else if (rank_counts[i] == 2) {
            if (pairs < 2) {
                pair_ranks[pairs] = i + 2;
            }
            pairs++;
        }
    }
    
    bool flush = is_flush(sorted, 5);
    int straight_high = 0;
    bool straight = is_straight(sorted, 5, &straight_high);
    
    /* Royal Flush */
    if (flush && straight && sorted[0].rank == ACE && sorted[4].rank == TEN) {
        result.rank = ROYAL_FLUSH;
        result.tiebreakers[0] = ACE;
        result.num_tiebreakers = 1;
        return result;
    }
    
    /* Straight Flush */
    if (flush && straight) {
        result.rank = STRAIGHT_FLUSH;
        result.tiebreakers[0] = straight_high;
        result.num_tiebreakers = 1;
        return result;
    }
    
    /* Four of a Kind */
    if (quads == 1) {
        result.rank = FOUR_OF_A_KIND;
        result.tiebreakers[0] = quad_rank;
        result.num_tiebreakers = 1;
        
        /* Add kicker */
        for (int i = 12; i >= 0; i--) {
            if (rank_counts[i] == 1) {
                result.tiebreakers[1] = i + 2;
                result.num_tiebreakers = 2;
                break;
            }
        }
        return result;
    }
    
    /* Full House */
    if (trips == 1 && pairs >= 1) {
        result.rank = FULL_HOUSE;
        result.tiebreakers[0] = trip_rank;
        result.num_tiebreakers = 1;
        
        /* Add pair rank */
        for (int i = 12; i >= 0; i--) {
            if (rank_counts[i] == 2) {
                result.tiebreakers[1] = i + 2;
                result.num_tiebreakers = 2;
                break;
            }
        }
        return result;
    }
    
    /* Flush */
    if (flush) {
        result.rank = FLUSH;
        result.num_tiebreakers = 0;
        for (int i = 0; i < 5; i++) {
            result.tiebreakers[result.num_tiebreakers++] = sorted[i].rank;
        }
        return result;
    }
    
    /* Straight */
    if (straight) {
        result.rank = STRAIGHT;
        result.tiebreakers[0] = straight_high;
        result.num_tiebreakers = 1;
        return result;
    }
    
    /* Three of a Kind */
    if (trips == 1) {
        result.rank = THREE_OF_A_KIND;
        result.tiebreakers[0] = trip_rank;
        result.num_tiebreakers = 1;
        
        /* Add kickers */
        for (int i = 12; i >= 0; i--) {
            if (rank_counts[i] == 1) {
                result.tiebreakers[result.num_tiebreakers++] = i + 2;
            }
        }
        return result;
    }
    
    /* Two Pair */
    if (pairs >= 2) {
        result.rank = TWO_PAIR;
        result.tiebreakers[0] = pair_ranks[0];
        result.tiebreakers[1] = pair_ranks[1];
        result.num_tiebreakers = 2;
        
        /* Add kicker */
        for (int i = 12; i >= 0; i--) {
            if (rank_counts[i] == 1) {
                result.tiebreakers[2] = i + 2;
                result.num_tiebreakers = 3;
                break;
            }
        }
        return result;
    }
    
    /* One Pair */
    if (pairs == 1) {
        result.rank = ONE_PAIR;
        result.tiebreakers[0] = pair_ranks[0];
        result.num_tiebreakers = 1;
        
        /* Add kickers */
        for (int i = 12; i >= 0; i--) {
            if (rank_counts[i] == 1) {
                result.tiebreakers[result.num_tiebreakers++] = i + 2;
            }
        }
        return result;
    }
    
    /* High Card */
    result.rank = HIGH_CARD;
    result.num_tiebreakers = 0;
    for (int i = 0; i < 5; i++) {
        result.tiebreakers[result.num_tiebreakers++] = sorted[i].rank;
    }
    
    return result;
}

HandValue evaluate_hand(Card *cards, int num_cards) {
    if (num_cards < 5) {
        HandValue invalid = {HIGH_CARD, {0}, 0};
        return invalid;
    }
    
    if (num_cards == 5) {
        return evaluate_five_cards(cards);
    }
    
    /* Try all 5-card combinations for 6 or 7 cards */
    HandValue best = {HIGH_CARD, {0}, 0};
    Card combo[5];
    
    /* Generate all combinations */
    if (num_cards == 7) {
        for (int i = 0; i < 7; i++) {
            for (int j = i + 1; j < 7; j++) {
                /* Skip cards i and j, use the other 5 */
                int idx = 0;
                for (int k = 0; k < 7; k++) {
                    if (k != i && k != j) {
                        combo[idx++] = cards[k];
                    }
                }
                
                HandValue current = evaluate_five_cards(combo);
                if (compare_hands(current, best) > 0) {
                    best = current;
                }
            }
        }
    } else if (num_cards == 6) {
        for (int i = 0; i < 6; i++) {
            /* Skip card i, use the other 5 */
            int idx = 0;
            for (int k = 0; k < 6; k++) {
                if (k != i) {
                    combo[idx++] = cards[k];
                }
            }
            
            HandValue current = evaluate_five_cards(combo);
            if (compare_hands(current, best) > 0) {
                best = current;
            }
        }
    }
    
    return best;
}

int compare_hands(HandValue h1, HandValue h2) {
    if (h1.rank > h2.rank) return 1;
    if (h1.rank < h2.rank) return -1;
    
    /* Same rank, compare tiebreakers */
    int min_tb = h1.num_tiebreakers < h2.num_tiebreakers ? 
                 h1.num_tiebreakers : h2.num_tiebreakers;
    
    for (int i = 0; i < min_tb; i++) {
        if (h1.tiebreakers[i] > h2.tiebreakers[i]) return 1;
        if (h1.tiebreakers[i] < h2.tiebreakers[i]) return -1;
    }
    
    return 0;  /* Tie */
}

const char* hand_rank_name(HandRank rank) {
    switch (rank) {
        case HIGH_CARD: return "High Card";
        case ONE_PAIR: return "One Pair";
        case TWO_PAIR: return "Two Pair";
        case THREE_OF_A_KIND: return "Three of a Kind";
        case STRAIGHT: return "Straight";
        case FLUSH: return "Flush";
        case FULL_HOUSE: return "Full House";
        case FOUR_OF_A_KIND: return "Four of a Kind";
        case STRAIGHT_FLUSH: return "Straight Flush";
        case ROYAL_FLUSH: return "Royal Flush";
        default: return "Unknown";
    }
}
