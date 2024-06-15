//
// Created by Goh Kai Jet on 15/06/2024.
//

#include "getdirection.h"
#include <assert.h>
#include <limits.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h> // abs()

#define MAP_DIM 8
#define MAX_EDGES 32768

typedef enum { BLANK, WALL, TREASURE, PLAYER, HUNTER } tile;

typedef enum { RIGHT, UP, LEFT, DOWN, UNKNOWN } dir;

typedef struct {
  int x;
  int y;
} coord;

typedef struct {
  coord pos;
  dir direction;
} player;

typedef struct {
  coord pos;
  dir direction;
} hunter;

typedef struct {
  int pts;
  bool won;
  bool lost;
} gamestate;

// tile map[MAP_DIM][MAP_DIM];
// gamestate game = {};

static coord top_left_blank(tile map[][MAP_DIM]) {
  coord ans;
  for (int i = 0; i < MAP_DIM; i++) {
    for (int j = 0; j < MAP_DIM; j++) {
      if (map[i][j] == BLANK) {
        ans.x = i;
        ans.y = j;
        return ans;
      }
    }
  }
  assert(false);
}

static coord top_right_blank(tile map[][MAP_DIM]) {
  coord ans;
  for (int i = 0; i < MAP_DIM; i++) {
    for (int j = MAP_DIM; j >= 0; j--) {
      if (map[i][j] == BLANK) {
        ans.x = i;
        ans.y = j;
        return ans;
      }
    }
  }
  assert(false);
}

static coord bot_left_blank(tile map[][MAP_DIM]) {
  coord ans;
  for (int i = MAP_DIM; i >= 0; i--) {
    for (int j = 0; j < MAP_DIM; j++) {
      if (map[i][j] == BLANK) {
        ans.x = i;
        ans.y = j;
        return ans;
      }
    }
  }
  assert(false);
}

static coord bot_right_blank(tile map[][MAP_DIM]) {
  coord ans;
  for (int i = MAP_DIM; i >= 0; i--) {
    for (int j = MAP_DIM; j >= 0; j--) {
      if (map[i][j] == BLANK) {
        ans.x = i;
        ans.y = j;
        return ans;
      }
    }
  }
  assert(false);
}

static bool check_victory(gamestate *game) { return game->pts == 4; }

static void set_treasure(tile map[][MAP_DIM], gamestate *game) {
  coord tl = top_left_blank(map);
  coord tr = top_right_blank(map);
  coord bl = bot_left_blank(map);
  coord br = bot_right_blank(map);
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

static bool can_move(tile map[][MAP_DIM], int x, int y, dir direction) {
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

static void move_player(tile map[][MAP_DIM], player *player1, gamestate *game) {
  if (!can_move(map, player1->pos.x, player1->pos.y, player1->direction))
    return;
  map[player1->pos.x][player1->pos.y] = BLANK;
  switch (player1->direction) {
  case LEFT:
    player1->pos.y--;
    break;
  case UP:
    player1->pos.x--;
    break;
  case RIGHT:
    player1->pos.y++;
    break;
  case DOWN:
    player1->pos.x++;
    break;
  case UNKNOWN:
    break;
  }
  if (map[player1->pos.x][player1->pos.y] == TREASURE) {
    game->pts++;
    if (!check_victory(game))
      set_treasure(map, game);
  }

  map[player1->pos.x][player1->pos.y] = PLAYER;
}

static void move_hunter(tile map[][MAP_DIM], hunter *hunter1) {
  if (!can_move(map, hunter1->pos.x, hunter1->pos.y, hunter1->direction))
    return;
  if (map[hunter1->pos.x][hunter1->pos.y] != TREASURE)
    map[hunter1->pos.x][hunter1->pos.y] = BLANK;
  switch (hunter1->direction) {
  case LEFT:
    hunter1->pos.y--;
    break;
  case UP:
    hunter1->pos.x--;
    break;
  case RIGHT:
    hunter1->pos.y++;
    break;
  case DOWN:
    hunter1->pos.x++;
    break;
  case UNKNOWN:
    break;
  }

  if (map[hunter1->pos.x][hunter1->pos.y] != TREASURE)
    map[hunter1->pos.x][hunter1->pos.y] = HUNTER;
}

static bool check_lost(player *player1, hunter *hunter1) {
  return (player1->pos.x == hunter1->pos.x && player1->pos.y == hunter1->pos.y);
}

int len[MAP_DIM][MAP_DIM][MAP_DIM][MAP_DIM];
dir next[MAP_DIM][MAP_DIM][MAP_DIM][MAP_DIM];

static void routing_table(tile map[][MAP_DIM]) {

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

static void change_hunter_dir(hunter *hunter1, player *player1) {
  hunter1->direction =
      next[hunter1->pos.x][hunter1->pos.y][player1->pos.x][player1->pos.y];
}

static void change_player_dir(player *player1) {
  int direction = getDirection();
  if (direction == UNKNOWN) {
    return;
  }
  player1->direction = direction;
}

typedef struct {
  int src;
  int dst;
  int weight;
} Edge;

typedef struct {
  int V;
  int E;
  Edge edges[MAX_EDGES];
} Graph;

typedef struct {
  int parent;
  int rank;
} Subset;

static Graph *create_graph(int V, int E) {
  Graph *graph = malloc(sizeof(Graph));
  graph->V = V;
  graph->E = E;
  return graph;
}

static int dsu_find(Subset subsets[], int i) {
  if (subsets[i].parent != i) {
    subsets[i].parent = dsu_find(subsets, subsets[i].parent);
  }
  return subsets[i].parent;
}

static void dsu_union(Subset subsets[], int u, int v) {
  int u_root = dsu_find(subsets, u);
  int v_root = dsu_find(subsets, v);
  if (subsets[u_root].rank < subsets[v_root].rank) {
    subsets[u_root].parent = v_root;
  } else if (subsets[u_root].rank > subsets[v_root].rank) {
    subsets[v_root].parent = u_root;
  } else {
    subsets[v_root].parent = u_root;
    subsets[u_root].rank++;
  }
}

static int compare(const void *a, const void *b) {
  Edge *a_edge = (Edge *)a;
  Edge *b_edge = (Edge *)b;
  return a_edge->weight - b_edge->weight;
}

static void genMap(tile map[][MAP_DIM]) {
  // TODO: gotta do
}

static player *init_player(tile map[][MAP_DIM]) {
  player *ans = malloc(sizeof(player));
  ans->pos = bot_left_blank(map);
  ans->direction = RIGHT;
  return ans;
}

static hunter *init_hunter(tile map[][MAP_DIM]) {
  hunter *ans = malloc(sizeof(hunter));
  ans->pos = top_right_blank(map);
  ans->direction = LEFT;
  return ans;
}

static gamestate *init_game() {
  gamestate *ans = malloc(sizeof(hunter));
  ans->pts = 0;
  ans->won = false;
  ans->lost = false;
  return ans;
}

int main(void) {
  while (1) {
    //        tile map[][MAP_DIM];might
    //        genMap(map);
    tile map[MAP_DIM][MAP_DIM] = {
        {WALL, WALL, WALL, WALL, WALL, WALL, WALL, WALL},
        {BLANK, BLANK, BLANK, BLANK, WALL, BLANK, BLANK, BLANK},
        {BLANK, BLANK, BLANK, BLANK, WALL, BLANK, BLANK, BLANK},
        {BLANK, BLANK, BLANK, BLANK, BLANK, BLANK, BLANK, BLANK},
        {BLANK, BLANK, BLANK, BLANK, BLANK, BLANK, BLANK, BLANK},
        {BLANK, WALL, BLANK, BLANK, BLANK, BLANK, BLANK, BLANK},
        {BLANK, WALL, BLANK, BLANK, BLANK, BLANK, BLANK, BLANK},
        {WALL, WALL, WALL, WALL, WALL, WALL, WALL, WALL},

    };

    player *player1 = init_player(map);
    hunter *hunter1 = init_hunter(map);
    gamestate *game = init_game();
    set_treasure(map, game);
    routing_table(map);
    while (!game->won && !game->lost) {
      change_player_dir(player1);
      change_hunter_dir(hunter1, player1);
      move_player(map, player1, game);
      move_hunter(map, hunter1);
      game->won = check_victory(game);
      game->lost = check_lost(player1, hunter1);
    }
    if (game->won)
      printf("You won!");
    printf("You lost!");
    free(player1);
    free(hunter1);
    free(game);
  }
}
