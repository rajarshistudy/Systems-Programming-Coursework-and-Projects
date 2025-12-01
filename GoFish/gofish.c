#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "player.h"
#include "card.h"
#include "deck.h"
#include "gofish.h"

void game(void);
void hand(const struct player *target);
void book(const struct player *target);
int  count_book(const struct player *target);
void user_turn(void);
void computer_turn(void);


void game()
{
  printf("\nPlayer 1's Hand -");
  hand(&user);
  printf("\nPlayer 1's Book - ");
  book(&user);
  printf("\nPlayer 2's Book - ");
  book(&computer);
}

void hand(const struct player *target) {
    struct hand* temp = target->card_list;
    while (temp != NULL) {
      printf(" %c%c", temp->top.rank[0], temp->top.suit);
      temp = temp->next;
    }
}

void book(const struct player *target) {
  for (int i = 0; i < 7; i++)
      if (target->book[i] != '0')
          printf("%c ", target->book[i]);
}

int count_book(const struct player *target) {
  int count = 0;
  for (int i = 0; i < 7; i++)
      if (target->book[i] != '0')
          count++;
  return count;
}

void user_turn(void) {
  for (;;) {
      game();
      char rank = user_play(&user);

      if (search(&computer, rank)) {
          printf("\n\t- Player 2 has a ");
          while (search(&computer, rank))
              transfer_cards(&computer, &user, rank);

          check_add_book(&user);
          if (game_over(&user) || game_over(&computer)) return;

          printf("\n\t- Player 1 gets another turn");
          continue;  // extra turn
      }

      printf("\t- Player 2 has no %c's", rank);
      add_card(&user, next_card());
      printf("\n\t- Gofish, Player 1 draws a %c%c\n",
             user.card_list->top.rank[0], user.card_list->top.suit);

      check_add_book(&user);
      if (game_over(&user) || game_over(&computer)) return;

      if (user.card_list->top.rank[0] == rank) {
          printf("\n\t- Player 1 gets another turn");
          continue;  // extra turn after matching draw
      }
      break;  // turn ends; pass to computer
  }
  computer_turn();
}

void computer_turn(void) {
  for (;;) {
      game();
      char rank = computer_play(&computer);

      if (search(&user, rank)) {
          printf("\n\t- Player 1 has a ");
          while (search(&user, rank))
              transfer_cards(&user, &computer, rank);

          check_add_book(&computer);
          if (game_over(&user) || game_over(&computer)) return;

          printf("\n\t- Player 2 gets another turn");
          continue;  // extra turn
      }

      printf("\n\t- Player 1 has no %c's", rank);
      add_card(&computer, next_card());
      printf("\n\t- Gofish, Player 2 draws a card\n");

      check_add_book(&computer);
      if (game_over(&user) || game_over(&computer)) return;

      if (computer.card_list->top.rank[0] == rank) {
          printf("\n\t- Player 2 gets another turn");
          continue;  // extra turn after matching draw
      }
      break;  // turn ends; pass to user
  }
  user_turn();
}



int main(int args, char *argv[]) {
  for (;;) {
      printf("Shuffling deck...\n");
      shuffle();
      deal_player_cards(&user);
      deal_player_cards(&computer);
      printf("\n");

      // play one round starting with Player 1
      user_turn();

      // end-of-game checks
      if (game_over(&user)) {
          printf("\nPlayer 1 wins!");
      } else if (game_over(&computer)) {
          printf("\nPlayer 2 wins!");
      } else if (user.card_list == NULL) {
          printf("\nPlayer 1 has no cards left in hand");
          printf("\nPlayer 2 wins!");
      } else if (computer.card_list == NULL) {
          printf("\nPlayer 2 has no cards left in hand");
          printf("\nPlayer 1 wins!");
      }

      // play again?
      printf("\nDo you want to play again? (y/n): ");
      char option = 'n';
      if (scanf(" %c", &option) != 1) break;

      if (option == 'y' || option == 'Y') {
          reset_player(&user);
          reset_player(&computer);
          printf("\n\n");
          continue;
      }
      break;
  }
  return 0;
}

