#ifndef COMMON_H
#define COMMON_H

#include <stdbool.h>
#include <pthread.h>

#define SHM_NAME "/bulls_cows_shm"
#define MAX_GAMES 10
#define MAX_PLAYERS 10
#define WORD_LEN 5
#define NAME_LEN 32

typedef enum {
    CMD_NONE,
    CMD_GUESS,
    CMD_LEAVE
} CommandType;

typedef struct {
    char name[NAME_LEN];
    char last_guess[WORD_LEN];
    int bulls;
    int cows;
    bool active;
    bool has_new_result;
    CommandType pending_cmd;
} Player;

typedef struct {
    char game_name[NAME_LEN];
    char target_word[WORD_LEN + 1];
    Player players[MAX_PLAYERS];
    
    char last_event[128]; 
    int event_id; 
    
    int player_count;
    int max_players;
    bool active;
    bool needs_init;
    pthread_mutex_t game_lock;
} Game;

typedef struct {
    Game games[MAX_GAMES];
    pthread_mutex_t global_lock;
} SharedData;

#endif
