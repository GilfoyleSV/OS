#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <unistd.h>
#include <signal.h>
#include <time.h>
#include <ctype.h>
#include "common.h"

SharedData *shm_ptr = NULL;

void calculate_bulls_cows(const char *target, const char *guess, int *b, int *c) {
    *b = 0; *c = 0;
    bool target_used[WORD_LEN] = {false};
    bool guess_used[WORD_LEN] = {false};

    for (int i = 0; i < WORD_LEN; i++) {
        if (guess[i] == target[i]) {
            (*b)++;
            target_used[i] = true;
            guess_used[i] = true;
        }
    }
    for (int i = 0; i < WORD_LEN; i++) {
        if (guess_used[i]) continue;
        for (int j = 0; j < WORD_LEN; j++) {
            if (!target_used[j] && guess[i] == target[j]) {
                (*c)++;
                target_used[j] = true;
                break;
            }
        }
    }
}

char* generate_random_word() {
    static char words[][WORD_LEN + 1] = {
        "apple", "brave", "chair", "dance", "early",
        "flame", "grape", "heart", "image", "jolly",
        "knife", "lemon", "music", "night", "ocean"
    };
    static int count = sizeof(words) / sizeof(words[0]);
    return words[rand() % count];
}

void process_create_game(Game *g, Player *p) {
    if (p->pending_cmd == CMD_CREATE_GAME) {
        strncpy(g->game_name, p->requested_game_name, NAME_LEN);
        
        char *word = generate_random_word();
        
        strncpy(g->target_word, word, WORD_LEN);
        
        strncpy(p->name, p->player_name, NAME_LEN);
        p->active = true;
        p->pending_cmd = CMD_NONE;
        
        g->player_count = 1;
        g->current_turn = 0;
        g->active = true;
        
        snprintf(g->last_event, sizeof(g->last_event),
                "Game '%s' created. Player [%s] joined. Turn: [%s]",
                g->game_name, p->name, p->name);
        g->event_id++;
        
        printf("Server: Game '%s' created by player '%s'\n", 
               g->game_name, p->name);
    }
}

void process_join_game(Game *g, Player *p, int player_idx) {
    if (p->pending_cmd == CMD_JOIN_GAME) {
        strncpy(p->name, p->player_name, NAME_LEN);
        p->active = true;
        p->pending_cmd = CMD_NONE;
        
        g->player_count = 2;
        
        snprintf(g->last_event, sizeof(g->last_event),
                "Player [%s] joined the game. Current turn: [%s]",
                p->name, g->first_player.name);
        g->event_id++;
        
        printf("Server: Player '%s' joined game '%s'\n", 
               p->name, g->game_name);
    }
}

void process_player_guess(Game *g, Player *p, int player_idx) {
    if (p->active && p->pending_cmd == CMD_GUESS) {
        if (g->current_turn == player_idx) {
            calculate_bulls_cows(g->target_word, p->last_guess, &p->bulls, &p->cows);
            
            snprintf(g->last_event, sizeof(g->last_event), 
                    "Player [%s] guessed '%s' -> Bulls: %d, Cows: %d", 
                    p->name, p->last_guess, p->bulls, p->cows);
            g->event_id++;

            if (p->bulls == WORD_LEN) {
                usleep(100000); 
                snprintf(g->last_event, sizeof(g->last_event), 
                        "GAME OVER: Player [%s] WON!", p->name);
                g->event_id++;
                g->active = false;
            } else {
                g->current_turn = (g->current_turn == 0) ? 1 : 0;
            }

            p->pending_cmd = CMD_NONE;
        }
    }
    
    if (p->active && p->pending_cmd == CMD_LEAVE) {
        p->active = false;
        g->player_count--;
        
        snprintf(g->last_event, sizeof(g->last_event), 
                "Player [%s] left the game", p->name);
        g->event_id++;
        
        if (g->player_count == 0) {
            g->active = false;
        } else if (g->current_turn == player_idx) {
            g->current_turn = (player_idx == 0) ? 1 : 0;
        }
        
        p->pending_cmd = CMD_NONE;
    }
}

int main() {
    srand(time(NULL)); 
    
    int fd = shm_open(SHM_NAME, O_CREAT | O_RDWR, 0666);
    if (fd == -1) {
        fprintf(stderr, "shm_open error");
        exit(EXIT_FAILURE);
    }

    if (ftruncate(fd, sizeof(SharedData)) == -1) {
        fprintf(stderr, "ftruncate error");
        close(fd);
        exit(EXIT_FAILURE);
    }

    shm_ptr = mmap(NULL, sizeof(SharedData), PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    if (shm_ptr == MAP_FAILED) {
        fprintf(stderr, "mmap error");
        close(fd);
        exit(EXIT_FAILURE);
    }
    close(fd);

    pthread_mutexattr_t attr;
    pthread_mutexattr_init(&attr);
    pthread_mutexattr_setpshared(&attr, PTHREAD_PROCESS_SHARED);
    pthread_mutex_init(&shm_ptr->global_lock, &attr);

    for (int i = 0; i < MAX_GAMES; i++) {
        Game *g = &shm_ptr->games[i];
        pthread_mutex_init(&g->game_lock, &attr);
        
        g->active = false;
        g->player_count = 0;
        g->event_id = 0;
        g->current_turn = 0;
        g->game_name[0] = '\0';
        g->target_word[0] = '\0';
        g->last_event[0] = '\0';
        
        g->first_player.active = false;
        g->first_player.pending_cmd = CMD_NONE;
        g->first_player.name[0] = '\0';
        g->first_player.last_guess[0] = '\0';
        g->first_player.bulls = 0;
        g->first_player.cows = 0;
        
        g->second_player.active = false;
        g->second_player.pending_cmd = CMD_NONE;
        g->second_player.name[0] = '\0';
        g->second_player.last_guess[0] = '\0';
        g->second_player.bulls = 0;
        g->second_player.cows = 0;
    }
    
    pthread_mutexattr_destroy(&attr);

    printf("Server started. Waiting for connections...\n");

    while (1) {
        pthread_mutex_lock(&shm_ptr->global_lock);

        for (int i = 0; i < MAX_GAMES; i++) {
            Game *g = &shm_ptr->games[i];
            
            pthread_mutex_lock(&g->game_lock);
            
            if (!g->active && g->first_player.pending_cmd == CMD_CREATE_GAME) {
                process_create_game(g, &g->first_player);
            }
            
            if (g->active && g->player_count == 1 && 
                g->second_player.pending_cmd == CMD_JOIN_GAME) {
                process_join_game(g, &g->second_player, 1);
            }
            
            if (g->active) {
                process_player_guess(g, &g->first_player, 0);
                process_player_guess(g, &g->second_player, 1);
            }
            
            pthread_mutex_unlock(&g->game_lock);
        }

        pthread_mutex_unlock(&shm_ptr->global_lock);
        usleep(50000); 
    }
    return 0;
}