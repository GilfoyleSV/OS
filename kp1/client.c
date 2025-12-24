короче я так подумал, странно что у меня на клиенте такие функции, поэтому давай сделаем так что, если пользователь хочет создать игру, то он в структуре делает запрос на создание игры и именно сервер выдаёт ему айди игры и тд

вот код клиента

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
    if (!fgets(buf, size, stdin)) {
        return 0;
    }
    
    size_t len = strlen(buf);
    if (len > 0 && buf[len - 1] == '\n') {
        buf[len - 1] = '\0';
    } else {
        int c;
        while ((c = getchar()) != '\n' && c != EOF);
    }
    
    return 1;
}

void read_name(const char *prompt, char *buf, size_t size) {
    while (1) {
        printf("%s", prompt);

        if (!read_line(buf, size)) {
            printf("Ошибка ввода\n");
            continue;
        }

        if (buf[0] == '\0') {
            printf("Имя не может быть пустым\n");
            continue;
        }

        break;
    }
}

void* monitor_events(void* arg) {
    if (my_game_idx == -1) return NULL;
    Game *g = &shm_ptr->games[my_game_idx];
    
    while (1) {
        pthread_mutex_lock(&g->game_lock);
        
        if (g->event_id > last_seen_event) {
            printf("\n[EVENT]: %s\n", g->last_event);
            last_seen_event = g->event_id;
            printf("> "); 
            fflush(stdout);
        }
        
        if (!g->active) {
            printf("\nИгра завершилась\n");
            pthread_mutex_unlock(&g->game_lock);
            break;
        }

        pthread_mutex_unlock(&g->game_lock);
        usleep(100000);
    }
    return NULL;
}

void create_game() {
    char gname[NAME_LEN];
    char pname[NAME_LEN];

    read_name("Game Name: ", gname, NAME_LEN);
    read_name("Your Name: ", pname, NAME_LEN);

    pthread_mutex_lock(&shm_ptr->global_lock);
    for (int i = 0; i < MAX_GAMES; i++) {
        if (!shm_ptr->games[i].active) {
            Game *g = &shm_ptr->games[i];
            pthread_mutex_lock(&g->game_lock);
            
            strncpy(g->game_name, gname, NAME_LEN);
            strncpy(g->target_word, "apple", WORD_LEN); 
            g->player_count = 1;
            g->event_id = 0;
            g->current_turn = 0;
            
            g->first_player.active = true;
            strncpy(g->first_player.name, pname, NAME_LEN);
            
            my_game_idx = i;
            my_player_idx = 0;
            g->active = true;
            
            snprintf(g->last_event, sizeof(g->last_event),
                    "Игра '%s' создана. Игрок [%s] присоединился. Ход: [%s]", 
                    gname, pname, pname);
            g->event_id++;
            
            pthread_mutex_unlock(&g->game_lock);
            break;
        }
    }
    pthread_mutex_unlock(&shm_ptr->global_lock);
}

void join_game() {
    char gname[NAME_LEN];
    char pname[NAME_LEN];

    read_name("Join Game Name: ", gname, NAME_LEN);
    read_name("Your Name: ", pname, NAME_LEN);

    pthread_mutex_lock(&shm_ptr->global_lock);
    for (int i = 0; i < MAX_GAMES; i++) {
        Game *g = &shm_ptr->games[i];
        if (g->active && strcmp(g->game_name, gname) == 0) {
            pthread_mutex_lock(&g->game_lock);
            if (g->player_count == 1) {
                g->second_player.active = true;
                strncpy(g->second_player.name, pname, NAME_LEN);
                g->player_count = 2;
                
                my_game_idx = i;
                my_player_idx = 1;
                last_seen_event = g->event_id;
                
                snprintf(g->last_event, sizeof(g->last_event),
                        "Игрок [%s] присоединился к игре. Сейчас ход: [%s]", 
                        pname, g->first_player.name);
                g->event_id++;
                
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
    if (fd == -1) { 
        perror("shm_open");
        exit(EXIT_FAILURE);
    }

    shm_ptr = mmap(NULL, sizeof(SharedData), PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    if (shm_ptr == MAP_FAILED) {
        perror("mmap");
        close(fd);
        exit(EXIT_FAILURE);
    }

    if (close(fd) == -1) {
        perror("close");
    }

    printf("1. Create\n2. Join\n> ");
    int choice = 0;

    while (1) {
        if (scanf("%d", &choice) != 1) {
            printf("Неправильный ввод. Введите число.\n");

            int c;
            while ((c = getchar()) != '\n' && c != EOF);
            continue;
        }

        if (choice == 1 || choice == 2) {
            break;
        }

        printf("Ошибка: введите 1 или 2.\n");
    }

    int c;
    while ((c = getchar()) != '\n' && c != EOF);

    if (choice == 1) {
        create_game();
    } else {
        join_game();
    }

    if (my_game_idx == -1) {
        printf("Не удалось создать/присоединиться к игре\n");
        munmap(shm_ptr, sizeof(SharedData));
        return 1;
    }

    pthread_t tid;
    pthread_create(&tid, NULL, monitor_events, NULL);

    Game *g = &shm_ptr->games[my_game_idx];
    Player *me = (my_player_idx == 0) ? &g->first_player : &g->second_player;

    while (1) {
        pthread_mutex_lock(&g->game_lock);
        
        if (!g->active) {
            pthread_mutex_unlock(&g->game_lock);
            printf("\nGame is over. Exiting...\n");
            break;
        }
        
        int is_my_turn = (g->current_turn == my_player_idx);
        
        pthread_mutex_unlock(&g->game_lock);

        if (is_my_turn) {
            printf("\n--- YOUR TURN ---\n");
            printf("Commands: 'guess' - make a guess, 'exit' - leave game\n> ");
        } else {
            printf("\n--- Waiting for opponent's turn... ---\n");
            sleep(1);
            continue;
        }

        char cmd[MAX_LEN_COMM];
        if (!read_line(cmd, sizeof(cmd))) {
            continue;
        }

        if (strcmp(cmd, "guess") == 0) {
            char word[WORD_LEN + 1];
            printf("Word (%d letters): ", WORD_LEN);

            if (!read_line(word, sizeof(word))) {
                continue;
            }

            if (strlen(word) != WORD_LEN) {
                printf("Ошибка: слово должно быть длиной %d\n", WORD_LEN);
                continue;
            }

            pthread_mutex_lock(&g->game_lock);
            
            if (g->active && g->current_turn == my_player_idx) {
                memcpy(me->last_guess, word, WORD_LEN);
                me->last_guess[WORD_LEN] = '\0';
                me->pending_cmd = CMD_GUESS;
                
                snprintf(g->last_event, sizeof(g->last_event),
                        "Player [%s] made a move: '%s'", me->name, word);
                g->event_id++;
            } else {
                printf("Not your turn!\n");
            }
            
            pthread_mutex_unlock(&g->game_lock);
        }
        else if (strcmp(cmd, "exit") == 0) {
            pthread_mutex_lock(&g->game_lock);
            me->active = false;
            me->pending_cmd = CMD_LEAVE;
            
            if (g->current_turn == my_player_idx) {
                g->current_turn = (my_player_idx == 0) ? 1 : 0;
            }
            
            pthread_mutex_unlock(&g->game_lock);
            break;
        }
        else {
            printf("Unknown command\n");
        }
    }

    pthread_join(tid, NULL);
    munmap(shm_ptr, sizeof(SharedData));
    return 0;
}

вот код сервера
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
    int fd = shm_open(SHM_NAME, O_CREAT | O_RDWR, 0666);
    if (fd == -1) {
        fprintf(stderr, "Ошибка shm_open");
        exit(EXIT_FAILURE);
    }

    if (ftruncate(fd, sizeof(SharedData)) == -1) {
        fprintf(stderr, "Ошибка ftruncate");
        close(fd);
        exit(EXIT_FAILURE);
    }

    shm_ptr = mmap(NULL, sizeof(SharedData), PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    if (shm_ptr == MAP_FAILED) {
        fprintf(stderr, "Ошибка mmap");
        close(fd);
        exit(EXIT_FAILURE);
    }

    if (close(fd) == -1){
        perror("close failed");
    }

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
        g->needs_init = true;
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

    printf("Server started. Shared memory initialized.\n");

    while (1) {
        pthread_mutex_lock(&shm_ptr->global_lock);

        for (int i = 0; i < MAX_GAMES; i++) {
            Game *g = &shm_ptr->games[i];
            if (!g->active) continue;

            pthread_mutex_lock(&g->game_lock);
            process_player_guess(g, &g->first_player, 0);
            process_player_guess(g, &g->second_player, 1);
            pthread_mutex_unlock(&g->game_lock);
        }

        pthread_mutex_unlock(&shm_ptr->global_lock);
        usleep(50000); 
    }
    return 0;
}