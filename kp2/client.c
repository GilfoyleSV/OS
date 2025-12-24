#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <unistd.h>
#include <pthread.h>
#include "common.h"

SharedData *shm_ptr = NULL;
int my_game_idx = -1;
int my_player_idx = -1;
int last_seen_event = 0;

int read_line(char *buf, size_t size) {
    if (!fgets(buf, size, stdin)) return 0;
    
    size_t len = strlen(buf);
    if (len > 0 && buf[len - 1] == '\n') {
        buf[len - 1] = '\0';
    }
    return 1;
}

void* monitor_events(void* arg) {
    if (my_game_idx == -1) return NULL;
    Game *g = &shm_ptr->games[my_game_idx];
    
    while (1) {
        pthread_mutex_lock(&g->game_lock);
        
        if (g->event_id > last_seen_event) {
            printf("\n[EVENT]: %s\n", g->last_event);
            last_seen_event = g->event_id;
            
            if (!g->active) {
                printf("\n--- The game has ended.");
                pthread_mutex_unlock(&g->game_lock);
                break;
            }
            
            printf("> "); 
            fflush(stdout);
        }
        
        if (!g->active) {
            printf("\nGame session closed by server.\n");
            pthread_mutex_unlock(&g->game_lock);
            break;
        }

        pthread_mutex_unlock(&g->game_lock);
        usleep(100000);
    }
    return NULL;
}

void create_game() {
    char gname[NAME_LEN], pname[NAME_LEN];
    printf("Game Name: "); scanf("%s", gname);
    printf("Your Name: "); scanf("%s", pname);

    pthread_mutex_lock(&shm_ptr->global_lock);
    for (int i = 0; i < MAX_GAMES; i++) {
        if (!shm_ptr->games[i].active) {
            Game *g = &shm_ptr->games[i];
            pthread_mutex_lock(&g->game_lock);
            strncpy(g->game_name, gname, NAME_LEN);
            strncpy(g->target_word, "apple", WORD_LEN); 
            g->max_players = MAX_PLAYERS;
            g->player_count = 1;
            g->event_id = 0;
            
            g->players[0].active = true;
            strncpy(g->players[0].name, pname, NAME_LEN);
            
            my_game_idx = i;
            my_player_idx = 0;
            g->active = true;
            pthread_mutex_unlock(&g->game_lock);
            break;
        }
    }
    pthread_mutex_unlock(&shm_ptr->global_lock);
}

void join_game() {
    char gname[NAME_LEN], pname[NAME_LEN];
    printf("Join Game Name: "); scanf("%s", gname);
    printf("Your Name: "); scanf("%s", pname);

    pthread_mutex_lock(&shm_ptr->global_lock);
    for (int i = 0; i < MAX_GAMES; i++) {
        Game *g = &shm_ptr->games[i];
        if (g->active && strcmp(g->game_name, gname) == 0) {
            pthread_mutex_lock(&g->game_lock);
            if (g->player_count < g->max_players) {
                int p_idx = g->player_count++;
                g->players[p_idx].active = true;
                strncpy(g->players[p_idx].name, pname, NAME_LEN);
                my_game_idx = i;
                my_player_idx = p_idx;
                last_seen_event = g->event_id;
                printf("Joined successfully!\n");
            } else {
                printf("Game full!\n");
            }
            pthread_mutex_unlock(&g->game_lock);
            break;
        }
    }
    pthread_mutex_unlock(&shm_ptr->global_lock);
}

int main() {
    int fd = shm_open(SHM_NAME, O_RDWR, 0666);
    if (fd == -1) { perror("shm_open"); return 1; }
    shm_ptr = mmap(NULL, sizeof(SharedData), PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);

    printf("1. Create\n2. Join\n> ");
    int choice; scanf("%d", &choice); 
    if (choice == 1){
        create_game();
    } else if (choice == 2){
        join_game();
    } else {
        printf("Error input");
    }

    if (my_game_idx == -1) return 1;

    pthread_t tid;
    pthread_create(&tid, NULL, monitor_events, NULL);

    Game *g = &shm_ptr->games[my_game_idx];
    Player *me = &g->players[my_player_idx];

    while (1) {

        pthread_mutex_lock(&g->game_lock);
        if (!g->active) {
            pthread_mutex_unlock(&g->game_lock);
            printf("\nGame is over. Exiting...\n");
            break; 
        }
        pthread_mutex_unlock(&g->game_lock);

        printf("\nCommands: 'guess', 'exit'\n> ");
        char cmd[16]; scanf("%s", cmd);

        if (strcmp(cmd, "guess") == 0) {
            char word[16];
            printf("Word (%d letters): ", WORD_LEN); scanf("%s", word);
            pthread_mutex_lock(&g->game_lock);
            strncpy(me->last_guess, word, WORD_LEN);
            me->pending_cmd = CMD_GUESS;
            me->has_new_result = false;
            pthread_mutex_unlock(&g->game_lock);
        } 
        else if (strcmp(cmd, "exit") == 0) {
            pthread_mutex_lock(&g->game_lock);
            me->active = false;
            pthread_mutex_unlock(&g->game_lock);
            break;
        }
    }
    return 0;
}