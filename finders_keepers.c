#include "getspeech.h"
#include "matrix/include/led-matrix-c.h"
#include <Python.h>
#include <assert.h>
#include <limits.h>
#include <pthread.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>

#define MAP_DIM 32
#define USING_PI true
#define USING_MIC false
#define USING_AUTO false
#define USING_ALDOUS false
#define JUNCTION_STOP true
#define EASE_FACTOR 160
#define MIN_DIST 2
#define MAX_DIST 10
#define INIT_STEPS_LEFT MIN_DIST + 2 * (rand() % ((MAX_DIST - MIN_DIST) / 2))
#define NUM_SPAWNERS 16

typedef enum { PATH, WALL, TREASURE, PLAYER, HUNTER } Tile;

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
  Coord treasure_pos;
} GameState;

typedef struct {
  Coord coord;
  int dir;
  int steps_left;
} Builder;

Builder builders[NUM_SPAWNERS * 4];
int builder_count = 0;

int dir_row[] = {0, -1, 0, 1, 0};
int dir_col[] = {1, 0, -1, 0, 0};

void initializePython() {
  Py_Initialize();
  PyRun_SimpleString("import sys");
  PyRun_SimpleString("sys.path.append(\".\")");
}
void finalizePython() {
  if (Py_FinalizeEx() < 0) {
    exit(120);
  }
}

void check_for_fork() {
  pid_t pid = fork();
  if (pid == 0) {
    // Child process
    // printf("Child process: PID %d\n", getpid());
    _exit(0); // Exit child process immediately
  } else if (pid > 0) {
    // Parent process
    // printf("Parent process: PID %d\n", getpid());
  } else {
    // Fork failed
    perror("fork");
  }
}

static Coord top_left_blank(Tile map[][MAP_DIM]) {
  Coord ans;
  for (int i = 1; i < MAP_DIM; i++) {
    for (int j = 1; j < MAP_DIM; j++) {
      if (map[i][j] == PATH) {
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
      if (map[i][j] == PATH) {
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
      if (map[i][j] == PATH) {
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
      if (map[i][j] == PATH) {
        ans.row = i;
        ans.col = j;
        return ans;
      }
    }
  }
  assert(false);
}

static void display_map(Tile map[MAP_DIM][MAP_DIM]) {
  for (int i = 0; i < MAP_DIM; i++) {
    for (int j = 0; j < MAP_DIM; j++) {
      switch (map[i][j]) {
      case PATH:
        printf("  ");
        break;
      case WALL:
        printf("▉▉");
        break;
      case TREASURE:
        printf(" ⟡");
        break;
      case PLAYER:
        printf("☺ ");
        break;
      case HUNTER:
        printf("☹ ");
        break;
      default:
        printf(" ?");
      }
    }
    printf("\n");
  }
  printf("\n");
}

static bool check_victory(GameState *game) { return game->pts == 4; }

static void set_treasure(Tile map[][MAP_DIM], GameState *game) {
  Coord tl = top_left_blank(map);
  Coord tr = top_right_blank(map);
  Coord bl = bot_left_blank(map);
  Coord br = bot_right_blank(map);
  static bool used[4];
  int r;

  srand(time(NULL));

  if (game->pts == 0) {
    r = rand() % 1;
    (r == 0) ? (map[tl.row][tl.col] = TREASURE)
             : (map[br.row][br.col] = TREASURE);
    used[r] = true;
    return;
  }

  do {
    r = rand() % 4;
  } while (used[r]);

  switch (r) {
  case 0:
    (map[tl.row][tl.col] = TREASURE);
    game->treasure_pos = tl;
    break;
  case 1:
    (map[tr.row][tr.col] = TREASURE);
    game->treasure_pos = tr;
    break;
  case 2:
    (map[bl.row][bl.col] = TREASURE);
    game->treasure_pos = bl;
    break;
  case 3:
    (map[br.row][br.col] = TREASURE);
    game->treasure_pos = br;
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
  if (!can_move(map, player->pos.row, player->pos.col, player->direction)) {
    return;
  }

  Tile left_tile = PATH;
  Tile right_tile = PATH;
  bool junction_condition = false;

  do {
    map[player->pos.row][player->pos.col] = PATH;

    player->pos.row += dir_row[player->direction];
    player->pos.col += dir_col[player->direction];

    if (map[player->pos.row][player->pos.col] == TREASURE) {
      game->pts++;
      if (!check_victory(game)) {
        set_treasure(map, game);
      }
    }

    map[player->pos.row][player->pos.col] = PLAYER;
    system("clear");
    display_map(map);

    Dir turn_left = (player->direction + 3) % 4;
    Dir turn_right = (player->direction + 1) % 4;

    int left_row = player->pos.row + dir_row[turn_left];
    int left_col = player->pos.col + dir_col[turn_left];
    int right_row = player->pos.row + dir_row[turn_right];
    int right_col = player->pos.col + dir_col[turn_right];

    left_tile = map[left_row][left_col];
    right_tile = map[right_row][right_col];
    usleep(20000);

    junction_condition = left_tile != PATH && right_tile != PATH;

  } while ((!JUNCTION_STOP || junction_condition) &&
           can_move(map, player->pos.row, player->pos.col, player->direction));
}

static void move_hunter(Tile map[][MAP_DIM], Hunter *hunter) {
  if (!can_move(map, hunter->pos.row, hunter->pos.col, hunter->direction)) {
    return;
  }
  Tile left_tile = PATH;
  Tile right_tile = PATH;

  do {
    if (map[hunter->pos.row][hunter->pos.col] != TREASURE) {
      map[hunter->pos.row][hunter->pos.col] = PATH;
    }

    hunter->pos.row += dir_row[hunter->direction];
    hunter->pos.col += dir_col[hunter->direction];

    if (map[hunter->pos.row][hunter->pos.col] != TREASURE) {
      map[hunter->pos.row][hunter->pos.col] = HUNTER;
    }

    system("clear");
    display_map(map);

    Dir turn_left = (hunter->direction + 3) % 4;
    Dir turn_right = (hunter->direction + 1) % 4;

    int left_row = hunter->pos.row + dir_row[turn_left];
    int left_col = hunter->pos.col + dir_col[turn_left];
    int right_row = hunter->pos.row + dir_row[turn_right];
    int right_col = hunter->pos.col + dir_col[turn_right];

    left_tile = map[left_row][left_col];
    right_tile = map[right_row][right_col];
    usleep(20000);

  } while (left_tile != PATH && right_tile != PATH &&
           can_move(map, hunter->pos.row, hunter->pos.col, hunter->direction));
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
          next[i][j][x][y] = UNKNOWN;

          if (map[i][j] == WALL || map[x][y] == WALL)
            continue;

          if (i == x && j == y) {
            len[i][j][x][y] = 0;
            continue;
          }

          if ((abs(i - x) == 1 && j == y) || (abs(j - y) == 1 && i == x)) {
            len[i][j][x][y] = 1;
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
              if (len[i][j][p][q] != INT_MAX && len[p][q][x][y] != INT_MAX &&
                  len[i][j][p][q] + len[p][q][x][y] < len[i][j][x][y]) {
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

static void LED_map(Tile map[MAP_DIM][MAP_DIM],
                    struct LedCanvas *offscreen_canvas,
                    struct RGBLedMatrix *matrix) {
  for (int i = 0; i < MAP_DIM; i++) {
    for (int j = 0; j < MAP_DIM; j++) {
      switch (map[i][j]) {
      case PATH:
        led_canvas_set_pixel(offscreen_canvas, i, j, 0, 0, 0);
        break;
      case WALL:
    led_canvas_set_pixel(offscreen_canvas, i, j, 255, 255, 0);
        break;
      case TREASURE:
        led_canvas_set_pixel(offscreen_canvas, i, j, 255, 255, 0);
        break;
      case PLAYER:
        led_canvas_set_pixel(offscreen_canvas, i, j, 0, 0, 255);
        break;
      case HUNTER:
        led_canvas_set_pixel(offscreen_canvas, i, j, 255, 0, 0);
        break;
      }
    }
  }
  offscreen_canvas = led_matrix_swap_on_vsync(matrix, offscreen_canvas);
}

static bool position_valid(int x, int y) {
  return x > 0 && x < MAP_DIM - 1 && y > 0 && y < MAP_DIM - 1;
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

static void init_map(Tile map[][MAP_DIM]) {
  for (int i = 0; i < MAP_DIM; ++i) {
    for (int j = 0; j < MAP_DIM; ++j) {
      map[i][j] = WALL;
    }
  }
}

static void gen_map_aldous_broder(Tile map[][MAP_DIM]) {
  init_map(map);

  srand(time(NULL));

  Coord current = {rand() % (MAP_DIM) / 2 * 2 + 1,
                   rand() % (MAP_DIM) / 2 * 2 + 1};
  map[current.row][current.col] = PATH;

  int total_cells = (MAP_DIM / 2) * (MAP_DIM / 2) - MAP_DIM + 1;
  total_cells--;

  while (total_cells) {
    Dir direction = rand() % 4;
    Coord next = current;

    next.row += 2 * dir_row[direction];
    next.col += 2 * dir_col[direction];

    if (valid_move(&current, direction)) {
      if (map[next.row][next.col] == WALL) {
        map[(current.row + next.row) / 2][(current.col + next.col) / 2] = PATH;
        map[next.row][next.col] = PATH;
        total_cells--;

        system("clear");
        display_map(map);

        printf("remaining cells: %d\n", total_cells);
      }
      current = next;
    }
  }
}

static void place_spawners(Tile map[][MAP_DIM]) {
  srand(time(NULL));
  for (int s = 0; s < NUM_SPAWNERS; s++) {
    int row = 2 * (rand() % ((MAP_DIM - 2) / 2)) + 2;
    int col = 2 * (rand() % ((MAP_DIM - 2) / 2)) + 2;

    map[row][col] = PATH;

    bool used[4] = {0};

    int num_builders = 2 + (rand() % 2 + 1);
    for (int b = 0; b < num_builders; ++b) {

      Builder new_builder;

      do {
        new_builder.dir = rand() % 4;
      } while (used[new_builder.dir]);

      new_builder.steps_left = INIT_STEPS_LEFT;
      new_builder.coord.row = row;
      new_builder.coord.col = col;

      builders[builder_count++] = new_builder;
      used[new_builder.dir] = true;
    }
  }
}

static void move_builders(Tile map[][MAP_DIM]) {
  while (builder_count) {
    for (int i = 0; i < builder_count;) {
      int old_row = builders[i].coord.row;
      int old_col = builders[i].coord.col;
      int new_row = old_row + 2 * dir_row[builders[i].dir];
      int new_col = old_col + 2 * dir_col[builders[i].dir];

      while (builders[i].steps_left == 0 || !position_valid(new_row, new_col)) {
        int direction_change = 2 * (rand() % 2) + 1;
        builders[i].dir = (builders[i].dir + direction_change) % 4;
        builders[i].steps_left = INIT_STEPS_LEFT;
        new_row = old_row + 2 * dir_row[builders[i].dir];
        new_col = old_col + 2 * dir_col[builders[i].dir];
      }

      map[old_row][old_col] = PATH;
      map[(old_row + new_row) / 2][(old_col + new_col) / 2] = PATH;
      builders[i].coord.row = new_row;
      builders[i].coord.col = new_col;
      builders[i].steps_left -= 2;

      if (map[new_row][new_col] == PATH) {
        builders[i] = builders[--builder_count];
        continue;
      }

      system("clear");
      display_map(map);
      i++;
    }
  }
}

static void gen_map_imperfect(Tile map[][MAP_DIM]) {
  init_map(map);
  place_spawners(map);
  move_builders(map);
}

static Player *init_player(Tile map[][MAP_DIM]) {
  Player *player = malloc(sizeof(Player));
  player->pos = bot_left_blank(map);
  player->direction = UNKNOWN;
  map[player->pos.row][player->pos.col] = PLAYER;
  return player;
}

static Hunter *init_hunter(Tile map[][MAP_DIM]) {
  Hunter *hunter = malloc(sizeof(Hunter));
  hunter->pos = top_right_blank(map);
  hunter->direction = UNKNOWN;
  map[hunter->pos.row][hunter->pos.col] = HUNTER;
  return hunter;
}

static void change_hunter_dir(Hunter *hunter, Player *player) {
  hunter->direction =
      next[hunter->pos.row][hunter->pos.col][player->pos.row][player->pos.col];
  int random_boundary = rand() % EASE_FACTOR;
  int shortest_distance =
      len[hunter->pos.row][hunter->pos.col][player->pos.row][player->pos.col];
  // printf("shortest distance from hunter to player: %d\n", shortest_distance);
  if (random_boundary < shortest_distance) {
    hunter->direction = rand() % 4;
    // printf("randomised!\n");
  }
  // printf("hunter wants to move in direction %d\n", hunter->direction);
}

static void change_player_dir(Player *player) {
  int direction;
  if (USING_MIC) {
    direction = getDirection();
  } else {
    scanf("%d", &direction);
  }
  printf("direction chosen is %d\n", direction);
  if (direction == UNKNOWN) {
    return;
  }
  player->direction = direction;
  // printf("dir set");
}

static void auto_change_player_dir(Tile map[][MAP_DIM], GameState *game,
                                   Player *player, Hunter *hunter) {
  Coord treasure_pos = game->treasure_pos;
  Coord current_pos = player->pos;

  int best_score = INT_MIN;
  Dir best_direction = UNKNOWN;

  for (int i = 0; i < 4; ++i) {
    Dir dir = i;

    if (!can_move(map, current_pos.row, current_pos.col, dir)) {
      continue;
    }

    Coord new_pos = current_pos;
    new_pos.row += dir_row[dir];
    new_pos.col += dir_col[dir];

    int distance_to_treasure =
        len[new_pos.row][new_pos.col][treasure_pos.row][treasure_pos.col];
    int distance_to_hunter =
        len[new_pos.row][new_pos.col][hunter->pos.row][hunter->pos.col];

    int score = 2 * distance_to_hunter - distance_to_treasure;

    if (score > best_score) {
      best_score = score;
      best_direction = dir;
    }
  }

  if (best_direction != UNKNOWN) {
    player->direction = best_direction;
  }

  // printf("player wants to move in direction %d\n", best_direction);
}

static GameState *init_game(Tile map[][MAP_DIM]) {
  GameState *game = malloc(sizeof(GameState));
  game->pts = 0;
  game->treasure_pos = top_left_blank(map);
  game->won = false;
  game->lost = false;
  return game;
}

int i = 0;
int j = 0;

int main(int argc, char **argv) {
  Tile map[MAP_DIM][MAP_DIM];
  Player *player;
  Hunter *hunter;
  GameState *game;

  struct RGBLedMatrixOptions options;
  struct RGBLedMatrix *matrix;
  struct LedCanvas *offscreen_canvas;

  if (USING_PI) {
    memset(&options, 0, sizeof(options));
    options.rows = 32;
    options.cols = 32;
    options.chain_length = 1;

    matrix = led_matrix_create_from_options(&options, &argc, &argv);
    if (matrix == NULL)
      return 1;

    offscreen_canvas = led_matrix_create_offscreen_canvas(matrix);
  }

  if (USING_MIC) {
    initializePython();
    check_for_fork();
    // printf("%s: Thread ID: %lu\n", "counting", (unsigned
    // long)pthread_self());

    do {
      bool selected = false;
      bool pass = false;
      if (USING_ALDOUS) {
        gen_map_aldous_broder(map);
      } else {
        gen_map_imperfect(map);
      }
      player = init_player(map);
      hunter = init_hunter(map);
      game = init_game(map);
      set_treasure(map, game);
      if (USING_PI) {
        LED_map(map, offscreen_canvas, matrix);
      } else {
        system("clear");
        display_map(map);
      }
      while (!selected && !pass) {
        int query = getChoice();

        if (query == 1) {
          pass = true;
        } else if (query == 2) {
          selected = true;
        }
      }
      if (selected) {
        break;
      }
      free(player);
      free(hunter);
      free(game);
      if (USING_PI) {
        led_canvas_clear(offscreen_canvas);
      }
    } while (1);
  } else {
    if (USING_ALDOUS) {
      gen_map_aldous_broder(map);
    } else {
      gen_map_imperfect(map);
    }
    player = init_player(map);
    hunter = init_hunter(map);
    game = init_game(map);
    set_treasure(map, game);

    system("clear");
    display_map(map);
  }

  printf("calculating routes...\n");
  routing_table(map);

  system("clear");
  display_map(map);
  // printf("time to play!\n");

  while (!game->won && !game->lost) {
    if (USING_PI) {
      LED_map(map, offscreen_canvas, matrix);
    }
    if (USING_AUTO) {
      auto_change_player_dir(map, game, player, hunter);
    } else {
      change_player_dir(player);
    }
    change_hunter_dir(hunter, player);

    move_player(map, player, game);
    game->won = check_victory(game);
    game->lost = check_lost(player, hunter);

    if (game->won || game->lost) {
      system("clear");
      display_map(map);
      break;
    }

    move_hunter(map, hunter);
    game->won = check_victory(game);
    game->lost = check_lost(player, hunter);

    system("clear");
    display_map(map);
    usleep(20000);
  }

  if (USING_PI) {
    led_canvas_clear(offscreen_canvas);
  }

  if (game->won) {
    if (USING_PI) {
      draw_text(offscreen_canvas, load_font("matrix/fonts/4x6.bdf"), 0, 15, 255,
                255, 255, "You win!", 0);
    }
    printf("you won!\n");
  } else {
    if (USING_PI) {
      draw_text(offscreen_canvas, load_font("matrix/fonts/4x6.bdf"), 0, 15, 255,
                255, 255, "You lost!", 0);
    }
    printf("you lost with a score of %d/4!\n", game->pts);
  }

  if (USING_PI) {
    led_matrix_swap_on_vsync(matrix, offscreen_canvas);
  }

  free(player);
  free(hunter);
  free(game);

  if (USING_PI) {
    led_matrix_delete(matrix);
  }
  if (USING_MIC) {
    finalizePython();
  }
}
