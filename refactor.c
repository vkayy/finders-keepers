//
// Created by Goh Kai Jet on 15/06/2024.
//

#include "sokol/getdirection.h"
#include <assert.h>
#include <stdlib.h> // abs()
#include <string.h> // memset()
#include <stdbool.h>
#include <limits.h>
#include <stdio.h>


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
} coord;


typedef struct{
    coord pos;
    dir direction;
} player;

typedef struct{
    coord pos;
    dir direction;
} hunter;

typedef struct{
    int pts;
    bool won;
    bool lost;
} gamestate;


//tile map[MAP_DIM][MAP_DIM];
//gamestate game = {};

coord topleftblank(tile **map){
    coord ans;
    for (int i = 0; i < MAP_DIM; i++){
        for (int j = 0; j < MAP_DIM; j++) {
            if (map[i][j] == BLANK){
                ans.x = i;
                ans.y = j;
                return ans;
            }
        }
    }
    assert(false);
}

coord toprighttblank(tile **map){
    coord ans;
    for (int i = 0; i < MAP_DIM; i++){
        for (int j = MAP_DIM; j >= 0; j--) {
            if (map[i][j] == BLANK){
                ans.x = i;
                ans.y = j;
                return ans;
            }
        }
    }
    assert(false);
}

coord botleftblank(tile **map){
    coord ans;
    for (int i = MAP_DIM; i >= 0; i--){
        for (int j = 0; j < MAP_DIM; j++) {
            if (map[i][j] == BLANK){
                ans.x = i;
                ans.y = j;
                return ans;
            }
        }
    }
    assert(false);
}

coord botrightblank(tile **map){
    coord ans;
    for (int i = MAP_DIM; i >= 0; i--){
        for (int j = MAP_DIM; j >= 0; j--) {
            if (map[i][j] == BLANK){
                ans.x = i;
                ans.y = j;
                return ans;
            }
        }
    }
    assert(false);
}

static bool checkVictory(gamestate *game){
    return game->pts == 4;
}
void setTreasure(tile **map,gamestate *game){
    coord tl = topleftblank(map);
    coord tr = toprighttblank(map);
    coord bl = botleftblank(map);
    coord br = botrightblank(map);
    static bool used[4];
    int r;

    if (game->pts == 0){
        r = rand() % 1;
        (r == 0)? (map[tl.x][tl.y] = TREASURE): (map[br.x][br.y] = TREASURE);
        used[r] = true;
        return;
    }

    do {
        r = rand() % 3;
    } while (used[r]);

    switch (r) {
        case 0: (map[tl.x][tl.y] = TREASURE); break;
        case 1: (map[tr.x][tr.y] = TREASURE); break;
        case 2: (map[bl.x][bl.y] = TREASURE); break;
        case 3: (map[br.x][br.y] = TREASURE); break;
    }

    used[r] = true;

}


static bool canMove(tile **map, int x, int y , dir direction){
    switch (direction) {
        case LEFT: return !(y <= 0 || map[x][y - 1] == WALL);
        case UP: return !(x <= 0 || map[x-1][y] == WALL);
        case RIGHT: return !(y >= MAP_DIM-1 || map[x][y+1] == WALL);
        case DOWN: return !(x >= MAP_DIM-1 || map[x+1][y] == WALL);
    }
}

static void movePlayer(tile** map, player *player1, gamestate *game){
    if (!canMove(map, player1->pos.x, player1->pos.y, player1->direction)) return;
    map[player1->pos.x][player1->pos.y] = BLANK;
    switch (player1->direction) {
        case LEFT: player1->pos.y --;
            break;
        case UP: player1->pos.x --;
            break;
        case RIGHT: player1->pos.y ++;
            break;
        case DOWN: player1->pos.x ++;
            break;
    }
    if ( map[player1->pos.x][player1->pos.y] == TREASURE) {
        game->pts++;
        if(!checkVictory(game)) setTreasure(map, game);
    }

    map[player1->pos.x][player1->pos.y] = PLAYER;
}

static void moveHunter(tile** map, hunter *hunter1){
    if (!canMove(map, hunter1->pos.x, hunter1->pos.y, hunter1->direction)) return;
    if (map[hunter1->pos.x][hunter1->pos.y] != TREASURE) map[hunter1->pos.x][hunter1->pos.y] = BLANK;
    switch (hunter1->direction) {
        case LEFT: hunter1->pos.y --;
            break;
        case UP: hunter1->pos.x --;
            break;
        case RIGHT: hunter1->pos.y ++;
            break;
        case DOWN: hunter1->pos.x ++;
            break;
    }

    if (map[hunter1->pos.x][hunter1->pos.y] != TREASURE) map[hunter1->pos.x][hunter1->pos.y] = HUNTER;

}


static bool checkLost(player *player1, hunter *hunter1){
    return (player1->pos.x == hunter1->pos.x && player1->pos.y == hunter1->pos.y);
}

int len[MAP_DIM][MAP_DIM][MAP_DIM][MAP_DIM];
dir next[MAP_DIM][MAP_DIM][MAP_DIM][MAP_DIM];

void routing_table(tile **map){

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
    hunter1->direction = next[hunter1->pos.x][hunter1->pos.y][player1->pos.x][player1->pos.y];
}

void changePlayerDir(player *player1){
    int direction = getDirection();
    switch (direction) {
        case 1: player1->direction = RIGHT; break;
        case 2: player1->direction = UP; break;
        case 3: player1->direction = LEFT; break;
        case 4: player1->direction = DOWN; break;
        default:
            return;
    }
}

tile **genMap(void ){

//    TODO: IMPLEMENT MAP GENERATION
}

player *initPlayer(tile **map){
    player *ans = malloc(sizeof(player));
    ans->pos = botleftblank(map);
    ans->direction = RIGHT;
    return ans;

}

hunter *initHunter(tile **map, player *player1){
    hunter *ans = malloc(sizeof(hunter));
    ans->pos = toprighttblank(map);
    ans->direction = LEFT;
    return ans;
}

gamestate *initGame(){
    gamestate *ans = malloc(sizeof(hunter));
    ans->pts = 0;
    ans->won = false;
    ans->lost = false;
}



int main(int argc, char** argv){
    while (1){
        tile **map = genMap();
        player *player1 = initPlayer(map);
        hunter *hunter1 = initHunter(map, player1);
        gamestate *game = initGame();
        setTreasure(map, game);
        routing_table(map);
        while (!game->won && !game->lost){
            changePlayerDir(player1);
            changeHunterDir(hunter1, player1);
            movePlayer(map, player1, game);
            moveHunter(map, hunter1);
            game->won = checkVictory(game);
            game->lost = checkLost(player1, hunter1);
        }
        if (game->won) printf("You won!");
        printf("You lost!");
        free(player1);
        free(hunter1);
        free(game);
    }
}

