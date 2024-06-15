//
// Created by Goh Kai Jet on 15/06/2024.
//

#include "sokol/getdirection.h"
#include <assert.h>
#include <stdlib.h> // abs()
#include <string.h> // memset()
#include <stdbool.h>
#include <limits.h>


#define MAP_DIM 32

typedef enum{
    BLANK,
    WALL,
    TREASURE,
    PLAYER,
    HUNTER
} tile;

typedef enum{
    RIGHT,
    UP,
    LEFT,
    DOWN
} dir;

typedef struct{
    int x;
    int y;
    dir direction;
} player;

typedef struct{
    int x;
    int y;
    dir direction;
} hunter;

typedef struct{
    int pts;
} gamestate;

tile map[MAP_DIM][MAP_DIM] = {};
gamestate game = {};

static bool checkVictory(void ){
    return game.pts == 4;
}

static bool canMove(player *player1){
    switch (player1->direction) {
        case LEFT: return !(player1->y <= 0 || map[player1->x][player1->y - 1] == WALL);
        case UP: return !(player1->x <= 0 || map[player1->x-1][player1->y] == WALL);
        case RIGHT: return !(player1->y >= MAP_DIM-1 || map[player1->x][player1->y+1] == WALL);
        case DOWN: return !(player1->x >= MAP_DIM-1 || map[player1->x+1][player1->y] == WALL);
    }
}

static void move(player *player1){
    if (!canMove(player1)) return;
    map[player1->x][player1->y] = BLANK;
    switch (player1->direction) {
        case LEFT: player1->y --;
            break;
        case UP: player1->x --;
            break;
        case RIGHT: player1->y ++;
            break;
        case DOWN: player1->x ++;
            break;
    }
    if ( map[player1->x][player1->y] == TREASURE) {
        game.pts++;
    }

    map[player1->x][player1->y] = PLAYER;
}

static bool checkLost(player *player1, hunter *hunter1){
    return (player1->x == hunter1->x && player1->y == hunter1->y);
}

int len[MAP_DIM][MAP_DIM][MAP_DIM][MAP_DIM];
dir next[MAP_DIM][MAP_DIM][MAP_DIM][MAP_DIM];

void routing_table(int **map){

    for (int i = 0; i < MAP_DIM; i++){
        for (int j = 0; j < MAP_DIM; j++){
            for (int x = 0; x < MAP_DIM; ++x) {
                for (int y = 0; y < MAP_DIM; ++y) {

                    len[i][j][x][y] = INT_MAX;
                    next[i][j][x][y] = 0;

                    if (map[i][j] == 0 || map[x][y] == 0) continue;

                    if (i == x && j == y){
                        len[i][j][x][y] = 0;
//                        Dummy value does not matter
                        next[i][j][x][y] = 0;
                        continue;
                    }
                    if ((abs(i-x) == 1 && abs(j-y) == 0) ||
                        (abs(i-x) == 0 && abs(j-y) == 1)  ){
                        if (i - x == 1) next[i][j][x][y] = UP;
                        if (i - x == -1) next[i][j][x][y] = DOWN;
                        if (j - y == 1) next[i][j][x][y] = LEFT;
                        if (j - y == -1) next[i][j][x][y] = RIGHT;
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
                            if (len[i][j][p][q] + len[p][q][x][y] < len[i][j][x][y]){
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

void changeHunterDir(hunter *hunter1, player *player1) {
    hunter1->direction = next[hunter1->x][hunter1->y][player1->x][player1->y];
}

void changePlayerDir(player *player1){
//    to modify getDirection to return intended values
    dir direction = getDirection();
    player1->direction = direction;
}



