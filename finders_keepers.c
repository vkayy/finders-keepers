#include "getdirection.h"
#include <assert.h>
#include <limits.h>
#include <mach/mach_port.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#define MAP_DIM 33

typedef enum { BLANK, WALL, TREASURE, PLAYER, HUNTER } Tile;

typedef enum { RIGHT, UP, LEFT, DOWN, UNKNOWN } Dir;

typedef struct {
  int x;
  int y;
} Coord;

typedef struct {
  Coord pos;
  Dir direction;
} Player;

typedef struct {
  Coord pos;
  Dir direction;
} Hunter;

typedef struct {
  int pts;
  bool won;
  bool lost;
} GameState;

static Coord top_left_blank(Tile map[][MAP_DIM]) {
  Coord ans;
  for (int i = 1; i < MAP_DIM; i++) {
    for (int j = 1; j < MAP_DIM; j++) {
      if (map[i][j] == BLANK) {
        ans.x = i;
        ans.y = j;
        return ans;
      }
    }
  }
  assert(false);
}

static Coord top_right_blank(Tile map[][MAP_DIM]) {
  Coord ans;
  for (int i = 1; i < MAP_DIM; i++) {
    for (int j = MAP_DIM - 1; j >= 1; j--) {
      if (map[i][j] == BLANK) {
        ans.x = i;
        ans.y = j;
        return ans;
      }
    }
  }
  assert(false);
}

static Coord bot_left_blank(Tile map[][MAP_DIM]) {
  Coord ans;
  for (int i = MAP_DIM - 1; i >= 1; i--) {
    for (int j = 1; j < MAP_DIM; j++) {
      if (map[i][j] == BLANK) {
        ans.x = i;
        ans.y = j;
        return ans;
      }
    }
  }
  assert(false);
}

static Coord bot_right_blank(Tile map[][MAP_DIM]) {
  Coord ans;
  for (int i = MAP_DIM - 1; i >= 1; i--) {
    for (int j = MAP_DIM - 1; j >= 1; j--) {
      if (map[i][j] == BLANK) {
        ans.x = i;
        ans.y = j;
        return ans;
      }
    }
  }
  assert(false);
}

static bool check_victory(GameState *game) { return game->pts == 4; }

static void set_treasure(Tile map[][MAP_DIM], GameState *game) {
  Coord tl = top_left_blank(map);
  Coord tr = top_right_blank(map);
  Coord bl = bot_left_blank(map);
  Coord br = bot_right_blank(map);
  static bool used[4];
  int r;

  if (game->pts == 0) {
    r = rand() % 1;
    (r == 0) ? (map[tl.x][tl.y] = TREASURE) : (map[br.x][br.y] = TREASURE);
    used[r] = true;
    return;
  }

  do {
    r = rand() % 3;
  } while (used[r]);

  switch (r) {
  case 0:
    (map[tl.x][tl.y] = TREASURE);
    break;
  case 1:
    (map[tr.x][tr.y] = TREASURE);
    break;
  case 2:
    (map[bl.x][bl.y] = TREASURE);
    break;
  case 3:
    (map[br.x][br.y] = TREASURE);
    break;
  }

  used[r] = true;
}

static bool can_move(Tile map[][MAP_DIM], int x, int y, Dir direction) {
  switch (direction) {
  case LEFT:
    return !(y <= 0 || map[x][y - 1] == WALL);
  case UP:
    return !(x <= 0 || map[x - 1][y] == WALL);
  case RIGHT:
    return !(y >= MAP_DIM - 1 || map[x][y + 1] == WALL);
  case DOWN:
    return !(x >= MAP_DIM - 1 || map[x + 1][y] == WALL);
  default:
    return false;
  }
}

static void move_player(Tile map[][MAP_DIM], Player *player, GameState *game) {
  if (!can_move(map, player->pos.x, player->pos.y, player->direction))
    return;
  map[player->pos.x][player->pos.y] = BLANK;
  switch (player->direction) {
  case LEFT:
    player->pos.y--;
    break;
  case UP:
    player->pos.x--;
    break;
  case RIGHT:
    player->pos.y++;
    break;
  case DOWN:
    player->pos.x++;
    break;
  case UNKNOWN:
    break;
  }
  if (map[player->pos.x][player->pos.y] == TREASURE) {
    game->pts++;
    if (!check_victory(game))
      set_treasure(map, game);
  }

  map[player->pos.x][player->pos.y] = PLAYER;
}

static void move_hunter(Tile map[][MAP_DIM], Hunter *hunter) {
  if (!can_move(map, hunter->pos.x, hunter->pos.y, hunter->direction))
    return;
  if (map[hunter->pos.x][hunter->pos.y] != TREASURE)
    map[hunter->pos.x][hunter->pos.y] = BLANK;
  switch (hunter->direction) {
  case LEFT:
    hunter->pos.y--;
    break;
  case UP:
    hunter->pos.x--;
    break;
  case RIGHT:
    hunter->pos.y++;
    break;
  case DOWN:
    hunter->pos.x++;
    break;
  case UNKNOWN:
    break;
  }

  if (map[hunter->pos.x][hunter->pos.y] != TREASURE)
    map[hunter->pos.x][hunter->pos.y] = HUNTER;
}

static bool check_lost(Player *player, Hunter *hunter) {
  return (player->pos.x == hunter->pos.x && player->pos.y == hunter->pos.y);
}

int len[MAP_DIM][MAP_DIM][MAP_DIM][MAP_DIM];
Dir next[MAP_DIM][MAP_DIM][MAP_DIM][MAP_DIM];

static void routing_table(Tile map[][MAP_DIM]) {

  for (int i = 0; i < MAP_DIM; i++) {
    for (int j = 0; j < MAP_DIM; j++) {
      for (int x = 0; x < MAP_DIM; ++x) {
        for (int y = 0; y < MAP_DIM; ++y) {

          len[i][j][x][y] = INT_MAX;
          next[i][j][x][y] = 0;

          if (map[i][j] == 0 || map[x][y] == 0)
            continue;

          if (i == x && j == y) {
            len[i][j][x][y] = 0;
            //                        Dummy value does not matter
            next[i][j][x][y] = 0;
            continue;
          }
          if ((abs(i - x) == 1 && abs(j - y) == 0) ||
              (abs(i - x) == 0 && abs(j - y) == 1)) {
            if (i - x == 1)
              next[i][j][x][y] = UP;
            if (i - x == -1)
              next[i][j][x][y] = DOWN;
            if (j - y == 1)
              next[i][j][x][y] = LEFT;
            if (j - y == -1)
              next[i][j][x][y] = RIGHT;
          }
        }
      }
    }
  }

  for (int p = 0; p < MAP_DIM; ++p) {
    for (int q = 0; q < MAP_DIM; ++q) {
      for (int i = 0; i < MAP_DIM; i++) {
        for (int j = 0; j < MAP_DIM; j++) {
          for (int x = 0; x < MAP_DIM; ++x) {
            for (int y = 0; y < MAP_DIM; ++y) {
              if (len[i][j][p][q] + len[p][q][x][y] < len[i][j][x][y]) {
                len[i][j][x][y] = len[i][j][p][q] + len[p][q][x][y];
                next[i][j][x][y] = next[i][j][p][q];
              }
            }
          }
        }
      }
    }
  }
}

static void display_map(Tile map[MAP_DIM][MAP_DIM]) {
  // for (int i = 0; i < MAP_DIM + 2; i++) {
  //   printf("▉▉");
  // }
  // printf("\n");
  for (int i = 0; i < MAP_DIM; i++) {
    // printf("▉▉");
    for (int j = 0; j < MAP_DIM; j++) {
      switch (map[i][j]) {
      case BLANK:
        printf("  ");
        break;
      case WALL:
        printf("▉▉");
        break;
      case TREASURE:
        printf("⟡ ");
        break;
      case PLAYER:
        printf("☺ ");
        break;
      case HUNTER:
        printf("☹ ");
        break;
      default:
        printf("? ");
      }
    }
    // printf("▉▉");
    printf("\n");
  }
  // for (int i = 0; i < MAP_DIM + 1; i++) {
  //   printf("▉▉");
  // }
  printf("\n");
}

static bool valid_move(Coord *c, Dir direction) {
  int x = c->x, y = c->y;
  switch (direction) {
  case RIGHT:
    return x + 2 <= MAP_DIM - 2;
  case UP:
    return y >= 3;
  case LEFT:
    return x >= 3;
  case DOWN:
    return y + 2 <= MAP_DIM - 2;
  default:
    return false;
  }
}

static void gen_map(Tile map[][MAP_DIM]) {
  for (int i = 0; i < MAP_DIM; i++) {
    for (int j = 0; j < MAP_DIM; j++) {
      map[i][j] = WALL;
    }
  }

  srand(time(NULL));

  Coord current = {rand() % (MAP_DIM) / 2 * 2 + 1,
                   rand() % (MAP_DIM) / 2 * 2 + 1};
  map[current.x][current.y] = BLANK;

  int total_cells = (MAP_DIM / 2) * (MAP_DIM / 2);
  total_cells--;

  while (total_cells) {
    Dir direction = rand() % 4;
    Coord next = current;

    switch (direction) {
    case RIGHT:
      next.x += 2;
      break;
    case UP:
      next.y -= 2;
      break;
    case LEFT:
      next.x -= 2;
      break;
    case DOWN:
      next.y += 2;
      break;
    default:
      break;
    }

    if (valid_move(&current, direction)) {
      if (map[next.x][next.y] == WALL) {
        map[(current.x + next.x) / 2][(current.y + next.y) / 2] = BLANK;
        map[next.x][next.y] = BLANK;
        total_cells--;

        system("clear");
        display_map(map);

        printf("remaining cells: %d\n", total_cells);
      }
      current = next;
    }
  }
}

static Player *init_player(Tile map[][MAP_DIM]) {
  Player *player = malloc(sizeof(Player));
  player->pos = bot_left_blank(map);
  player->direction = RIGHT;
  map[player->pos.x][player->pos.y] = PLAYER;
  return player;
}

static Hunter *init_hunter(Tile map[][MAP_DIM]) {
  Hunter *hunter = malloc(sizeof(Hunter));
  hunter->pos = top_right_blank(map);
  hunter->direction = LEFT;
  map[hunter->pos.x][hunter->pos.y] = HUNTER;
  return hunter;
}

static void change_hunter_dir(Hunter *hunter, Player *player) {
  hunter->direction =
      next[hunter->pos.x][hunter->pos.y][player->pos.x][player->pos.y];
}

static void change_player_dir(Player *player) {
  // int direction = getDirection();
  int direction;
  scanf("%d", &direction);
  if (direction == UNKNOWN) {
    return;
  }
  player->direction = direction;
}

static GameState *init_game() {
  GameState *game = malloc(sizeof(GameState));
  game->pts = 0;
  game->won = false;
  game->lost = false;
  return game;
}

int main(void) {
  Tile map[MAP_DIM][MAP_DIM];
  gen_map(map);
  Player *player = init_player(map);
  Hunter *hunter = init_hunter(map);
  GameState *game = init_game();
  set_treasure(map, game);

  system("clear");
  display_map(map);
  printf("calculating routes...\n");

  routing_table(map);

  system("clear");
  display_map(map);
  printf("time to play!");

  while (!game->won && !game->lost) {
    change_player_dir(player);
    change_hunter_dir(hunter, player);
    move_player(map, player, game);
    move_hunter(map, hunter);
    game->won = check_victory(game);
    game->lost = check_lost(player, hunter);
    system("clear");
    display_map(map);
  }
  if (game->won) {
    printf("You won!");
  } else {
    printf("You lost!");
  }
  free(player);
  free(hunter);
  free(game);
}
