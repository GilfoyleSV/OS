#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <unistd.h>
#include <signal.h>
#include <time.h>
#include "common.h"

SharedData *shm_ptr = NULL;

void cleanup() {
    if (shm_ptr) {
        munmap(shm_ptr, sizeof(SharedData));
        shm_unlink(SHM_NAME);
    }
}

void handle_sigint(int sig) {
    printf("\nServer shutting down...\n");
    cleanup();
    exit(0);
}

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

int main() {
    signal(SIGINT, handle_sigint);

    // Создаём разделяемую память
    int fd = shm_open(SHM_NAME, O_CREAT | O_RDWR, 0666);
    if (fd == -1) {
        perror("shm_open");
        exit(EXIT_FAILURE);
    }
    
    // Устанавливаем размер
    if (ftruncate(fd, sizeof(SharedData)) == -1) {
        perror("ftruncate");
        close(fd);
        exit(EXIT_FAILURE);
    }
    
    // Отображаем в память
    shm_ptr = mmap(NULL, sizeof(SharedData), PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    if (shm_ptr == MAP_FAILED) {
        perror("mmap");
        close(fd);
        exit(EXIT_FAILURE);
    }
    
    if (close(fd) == -1) {
        perror("close");
        exit(EXIT_FAILURE);
    }

    pthread_mutexattr_t attr;
    pthread_mutexattr_init(&attr);
    pthread_mutexattr_setpshared(&attr, PTHREAD_PROCESS_SHARED);
    
    pthread_mutex_init(&shm_ptr->global_lock, &attr);
    
    for (int i = 0; i < MAX_GAMES; i++) {
        Game *g = &shm_ptr->games[i];
        
        pthread_mutex_init(&g->game_lock, &attr);
        
        g->active = false;
        g->needs_init = true;
        g->player_count = 0;
        g->max_players = MAX_PLAYERS;
        g->event_id = 0;
        g->game_name[0] = '\0';
        g->target_word[0] = '\0';
        g->last_event[0] = '\0';
        
        for (int j = 0; j < MAX_PLAYERS; j++) {
            Player *p = &g->players[j];
            p->active = false;
            p->has_new_result = false;
            p->pending_cmd = CMD_NONE;
            p->bulls = 0;
            p->cows = 0;
            p->name[0] = '\0';
            p->last_guess[0] = '\0';
        }
    }
    
    pthread_mutexattr_destroy(&attr);
    
    printf("Server started. Shared memory initialized.\n");

    while (1) {
        pthread_mutex_lock(&shm_ptr->global_lock);
        
        for (int i = 0; i < MAX_GAMES; i++) {
            Game *g = &shm_ptr->games[i];
            if (!g->active) continue;

            pthread_mutex_lock(&g->game_lock);
            
            for (int j = 0; j < g->player_count; j++) {
                Player *p = &g->players[j];
                if (!p->active) continue;

                if (p->pending_cmd == CMD_GUESS) {
                    calculate_bulls_cows(g->target_word, p->last_guess, &p->bulls, &p->cows);
                    
                    snprintf(g->last_event, sizeof(g->last_event), 
                            "Player [%s] guessed '%s' -> Bulls: %d, Cows: %d", 
                            p->name, p->last_guess, p->bulls, p->cows);
                    g->event_id++;

                    if (p->bulls == WORD_LEN) {
                        usleep(100000);
                        snprintf(g->last_event, sizeof(g->last_event), 
                                "GAME OVER: Player [%s] WON! Word was: %s", 
                                p->name, g->target_word);
                        g->event_id++;
                        g->active = false;
                    }

                    p->has_new_result = true;
                    p->pending_cmd = CMD_NONE;
                }
                else if (p->pending_cmd == CMD_LEAVE) {
                    p->active = false;
                    p->pending_cmd = CMD_NONE;
                    
                    int active_players = 0;
                    for (int k = 0; k < g->player_count; k++) {
                        if (g->players[k].active) active_players++;
                    }
                    
                    if (active_players == 0) {
                        g->active = false;
                    }
                }
            }
            
            pthread_mutex_unlock(&g->game_lock);
        }
        
        pthread_mutex_unlock(&shm_ptr->global_lock);
        usleep(50000); 
    }
    
    return 0;
}