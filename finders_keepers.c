#include "getdirection.h"
#include <assert.h>
#include <limits.h>
#include <mach/mach_port.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>
#include "matrix/include/led-matrix-c.h"

#define MAP_DIM 33

typedef enum { BLANK, WALL, TREASURE, PLAYER, HUNTER } Tile;

typedef enum { RIGHT, UP, LEFT, DOWN, UNKNOWN } Dir;

typedef struct {
  int row;
  int col;
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
        ans.row = i;
        ans.col = j;
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
        ans.row = i;
        ans.col = j;
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
        ans.row = i;
        ans.col = j;
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
        ans.row = i;
        ans.col = j;
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
    (r == 0) ? (map[tl.row][tl.col] = TREASURE)
             : (map[br.row][br.col] = TREASURE);
    used[r] = true;
    return;
  }

  do {
    r = rand() % 3;
  } while (used[r]);

  switch (r) {
  case 0:
    (map[tl.row][tl.col] = TREASURE);
    break;
  case 1:
    (map[tr.row][tr.col] = TREASURE);
    break;
  case 2:
    (map[bl.row][bl.col] = TREASURE);
    break;
  case 3:
    (map[br.row][br.col] = TREASURE);
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
  if (!can_move(map, player->pos.row, player->pos.col, player->direction))
    return;
  map[player->pos.row][player->pos.col] = BLANK;
  switch (player->direction) {
  case RIGHT:
    player->pos.col++;
    break;
  case UP:
    player->pos.row--;
    break;
  case LEFT:
    player->pos.col--;
    break;
  case DOWN:
    player->pos.row++;
    break;
  case UNKNOWN:
    break;
  }
  if (map[player->pos.row][player->pos.col] == TREASURE) {
    game->pts++;
    if (!check_victory(game))
      set_treasure(map, game);
  }

  map[player->pos.row][player->pos.col] = PLAYER;
}

static void move_hunter(Tile map[][MAP_DIM], Hunter *hunter) {
  if (!can_move(map, hunter->pos.row, hunter->pos.col, hunter->direction))
    return;
  if (map[hunter->pos.row][hunter->pos.col] != TREASURE)
    map[hunter->pos.row][hunter->pos.col] = BLANK;
  switch (hunter->direction) {
  case RIGHT:
    hunter->pos.col++;
    break;
  case UP:
    hunter->pos.row--;
    break;
  case LEFT:
    hunter->pos.col--;
    break;
  case DOWN:
    hunter->pos.row++;
    break;
  case UNKNOWN:
    break;
  }

  if (map[hunter->pos.row][hunter->pos.col] != TREASURE)
    map[hunter->pos.row][hunter->pos.col] = HUNTER;
}

static bool check_lost(Player *player, Hunter *hunter) {
  return (player->pos.row == hunter->pos.row &&
          player->pos.col == hunter->pos.col);
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
      if (map[p][q] == WALL) {
        continue;
      }
      for (int i = 0; i < MAP_DIM; i++) {
        for (int j = 0; j < MAP_DIM; j++) {
          if (map[i][j] == WALL) {
            continue;
          }
          for (int x = 0; x < MAP_DIM; ++x) {
            for (int y = 0; y < MAP_DIM; ++y) {
              if (map[x][y] == WALL) {
                continue;
              }
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
  int row = c->row, col = c->col;
  switch (direction) {
  case RIGHT:
    return col + 2 <= MAP_DIM - 2;
  case UP:
    return row >= 3;
  case LEFT:
    return col >= 3;
  case DOWN:
    return row + 2 <= MAP_DIM - 2;
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
  map[current.row][current.col] = BLANK;

  int total_cells = (MAP_DIM / 2) * (MAP_DIM / 2);
  total_cells--;

  while (total_cells) {
    Dir direction = rand() % 4;
    Coord next = current;

    switch (direction) {
    case RIGHT:
      next.col += 2;
      break;
    case UP:
      next.row -= 2;
      break;
    case LEFT:
      next.col -= 2;
      break;
    case DOWN:
      next.row += 2;
      break;
    default:
      break;
    }

    if (valid_move(&current, direction)) {
      if (map[next.row][next.col] == WALL) {
        map[(current.row + next.row) / 2][(current.col + next.col) / 2] = BLANK;
        map[next.row][next.col] = BLANK;
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
  map[player->pos.row][player->pos.col] = PLAYER;
  return player;
}

static Hunter *init_hunter(Tile map[][MAP_DIM]) {
  Hunter *hunter = malloc(sizeof(Hunter));
  hunter->pos = top_right_blank(map);
  hunter->direction = LEFT;
  map[hunter->pos.row][hunter->pos.col] = HUNTER;
  return hunter;
}

static void change_hunter_dir(Hunter *hunter, Player *player) {
  hunter->direction =
      next[hunter->pos.row][hunter->pos.col][player->pos.row][player->pos.col];
  printf("hunter wants to move in direction %d\n", hunter->direction);
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

int main(int argc, char **argv) {
  Tile map[MAP_DIM][MAP_DIM];
  gen_map(map);
  Player *player = init_player(map);
  Hunter *hunter = init_hunter(map);
  GameState *game = init_game();
  set_treasure(map, game);
  struct RGBLedMatrixOptions options;
  struct RGBLedMatrix *matrix;
  struct LedCanvas *offscreen_canvas;

  memset(&options, 0, sizeof(options));
  options.rows = 32;
  options.cols = 32;
  options.chain_length = 1;

  matrix = led_matrix_create_from_options(&options, &argc, &argv);
  if (matrix == NULL) return 1;

  offscreen_canvas = led_matrix_create_offscreen_canvas(matrix);

  system("clear");
  display_map(map);
  printf("calculating routes...\n");

  routing_table(map);

  system("clear");
  display_map(map);
  printf("time to play!\n");

  while (!game->won && !game->lost) {
      for (int i = 0; i < MAP_DIM; i++) {
          for (int j = 0; j < MAP_DIM; j++) {
              switch (map[i][j]) {
                  case BLANK:
                      led_canvas_set_pixel(offscreen_canvas, i, j, 0, 0, 0 ); break;
                  case WALL:
                      led_canvas_set_pixel(offscreen_canvas, i, j, 255, 255, 255 ); break;
                  case TREASURE:
                      led_canvas_set_pixel(offscreen_canvas, i, j, 255, 255, 0 ); break;
                  case PLAYER:
                      led_canvas_set_pixel(offscreen_canvas, i, j, 0, 0, 255 ); break;
                  case HUNTER:
                      led_canvas_set_pixel(offscreen_canvas, i, j, 255, 0, 0 ); break;
              }
          }
      }
      offscreen_canvas = led_matrix_swap_on_vsync(matrix, offscreen_canvas);

    change_player_dir(player);
    change_hunter_dir(hunter, player);
    move_player(map, player, game);
    move_hunter(map, hunter);
    game->won = check_victory(game);
    game->lost = check_lost(player, hunter);
    system("clear");
    display_map(map);

  }


    led_canvas_clear(offscreen_canvas);






  if (game->won) {
      draw_text(offscreen_canvas, load_font("matrix/fonts/4x6.bdf"), 0, 15, 255, 255, 255, "You win!", 0);
    printf("you won!");
  } else {
      draw_text(offscreen_canvas, load_font("matrix/fonts/4x6.bdf"), 0, 15, 255, 255, 255, "You lost!", 0);
    printf("you lost!");
  }
    led_matrix_swap_on_vsync(matrix, offscreen_canvas);
  free(player);
  free(hunter);
  free(game);
    sleep(5);
    led_matrix_delete(matrix);
}
