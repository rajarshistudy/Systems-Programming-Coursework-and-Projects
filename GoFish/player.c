#include "player.h"  
#include <stdio.h>   
#include <stdlib.h>  
#include <time.h>   
#include <string.h>  
#include "card.h"
#include "deck.h"

/*
 * Instance Variables: user, computer   
 * ----------------------------------
 *  
 *  We only support 2 users: a human and a computer
 */
struct player user;
struct player computer;

int add_card(struct player* target, struct card* new_card){
    //adds a new card to the player's hand
    if (!target || !new_card) return -1;                    // simple safety check

    struct hand *node = malloc(sizeof *node);
    if (!node) return -1;                       // allocation failed

    node->top  = *new_card;                            // copy the card
    node->next = target->card_list;                  // push-front onto list
    target->card_list = node;
    target->hand_size++;
    return 0;
}

int remove_card(struct player* target, struct card* old_card){
    //Removes a card from the player's hand
    if(target->hand_size == 0){
        return -1;
    }
    struct hand* curr = target->card_list;
    struct hand* prev = NULL;
    while(!(curr->top.rank[0] == old_card->rank[0] && curr->top.suit == old_card->suit)){
        prev = curr;
        curr = curr->next;
    }

    if(curr == NULL){
        return -1;
    }
    if(prev == NULL){
        target->card_list = curr->next;
    }
    else{
        prev->next = curr->next;
    }
    target->hand_size--;
    free(curr);
    return 0;
}

char check_add_book(struct player* target){
    struct hand* curr = target->card_list;
    struct hand* rem = target->card_list;
    char rank;
    int count = 0;
    if(curr == NULL)
    {
        return 0;
    }

    while(curr != NULL)
    {
        rank = curr->top.rank[0];
        while(rem != NULL)
        {
            if(rem->top.rank[0] == rank)
            {
                count++;
            }
            rem = rem->next;
        }
        if(count == 4)
        {
            break;
        }
        curr = curr->next;
    }

    if(count == 4)
    {
        rem = target->card_list;
        while(rem != NULL)
        {
            if(rem->top.rank[0] == rank)
            {
                remove_card(target, &rem->top);
            }
            rem = rem->next;
        }
        int i = 0;
        while(target->book[i]!=0){
            i++;
        }
        target->book[i] = rank;
        if (target == &user)
        {
            printf("\n\t- Player 1 books - ");
        }
        if (target == &computer)
        {
            printf("\n\t- Player 2 books - ");
        }
        for(int i = 0; i < 7; i++){
            printf("%c ", target->book[i]);
        }
        return rank;
    }
    
    return 0;
}

int search(struct player* target, char rank)
{
    //search the player's hand for the requested rank
    if(!target){return 0;}
    for (struct hand* current = target->card_list; current != NULL; current = current->next) {
        if (*current->top.rank == rank) {
            return 1;
        }
    }
    return 0;
}

int transfer_cards(struct player* src, struct player* dest, char rank){
    struct hand* current = src->card_list;
    int count = 0;
    
    while (current != NULL) {
        struct hand* next = current->next;  // Save next pointer before removal
        if (*current->top.rank == rank) {
            printf("%c%c ", current->top.rank[0], current->top.suit);
            add_card(dest, &current->top);
            remove_card(src, &current->top);
            count++;
        }
        current = next;
    }
    return count;
}

int game_over(struct player* target) 
{
    int count = 0;
    for (int i = 0; i < 7; i++) {
        if (target->book[i] != 0) {
            count++;
        }
    }
    return count == 7;
}

int reset_player(struct player *p) {
    if (!p) return 0;

    // remove all cards using the existing API (no manual free)
    while (p->card_list) {
        remove_card(p, &p->card_list->top);
    }

    p->hand_size = 0;
    for (int i = 0; i < 7; ++i) p->book[i] = 0;
    return 0;
}

char computer_play(struct player* target){
    printf("\nPlayer 2's turn, enter a Rank: ");

    // If player has no cards, return '0'
    if (target == NULL || target->card_list == NULL)
        return '0';

    char pick = '0';
    int n = 0;
    struct hand *cur = target->card_list;

    // Reservoir sampling – pick one random card in a single pass
    while (cur != NULL) {
        n++;
        if (rand() % n == 0)
            pick = cur->top.rank[0];
        cur = cur->next;
    }

    // Print the chosen rank
    if (pick != '0')
        printf("%c", pick);

    return pick;
}

char user_play(struct player* target){
    char rank;
    for (;;) {
        printf("\nPlayer 1's turn, enter a Rank: ");
        if (scanf(" %c", &rank) != 1) return '0';      // EOF / input error
        if (search(target, rank)) return rank;              // have at least one? done
        puts("Error - must have at least one card from rank to play");
    }
}
