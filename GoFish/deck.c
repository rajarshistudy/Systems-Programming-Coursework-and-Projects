#include "deck.h"
#include <time.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "player.h"
#include "card.h"
#include "gofish.h"

/*
 * Variable: deck_instance
 * -----------------------
 *  
 * Go Fish uses a single deck
 */
struct deck deck_instance;

int shuffle(){
    //initialising deck_instance using for loop
    char rank[13] = {'A', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'J', 'Q', 'K'};
    char suits[4] = {'S', 'C', 'H', 'D'};

    int index = 0;
    for(int i=0; i<4; i++){
        for(int j=0; j<13; j++){
            deck_instance.list[index].suit = suits[i];
            deck_instance.list[index].rank[0] = rank[j];
            deck_instance.list[index].rank[1] = '\0';
            index++;
        }
    }
    //shuffling deck using random number generator - reference from code provided in class
    srand(time(NULL));
    for(int i = 0; i < 52; i++)
    {
        int j = rand() % 52;
        struct card temp = deck_instance.list[i];
        deck_instance.list[i] = deck_instance.list[j];
        deck_instance.list[j] = temp;
    }
    deck_instance.top_card = 52;
    
    return 0;
}

int deal_player_cards(struct player* target){
    //in this function we want to deal the player 7 cards from the top of the deck
    //uses the function next_card() to point to the next card at the top of the deck
    //uses the function add_card() from player.h to add the card to the players hand

    for(int i=0; i<=6; i++){
        if(deck_instance.top_card==0){
            return -1;
        }
        add_card(target, next_card()); //adding the card to the player's deck
    }
    return 0;
}
struct card* next_card(){
    deck_instance.top_card--;
    return &deck_instance.list[deck_instance.top_card];
}

size_t deck_size(){
    return deck_instance.top_card;
}


