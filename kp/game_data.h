#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <pthread.h>
#include <semaphore.h>

#define GAME_MAP_FILE "game_state.dat"
#define MAX_PLAYERS 4
#define WORD_LEN 256

typedef struct {
    int gameStarted;
    char word[WORD_LEN];
    int numPlayers;
    int currentPlayer;
    int gameOver;
    int scores[MAX_PLAYERS];
    int playerTurn;
    char guess[WORD_LEN];
    int bulls;
    int cows;
    int hasResult;
    sem_t shm_mutex;
} GameData;