#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <semaphore.h>


void check_guess(const char* secret_word, const char* guess, int* bulls, int* cows) {
    int word_len = strlen(secret_word);
    *bulls = 0; *cows = 0;
    int word_used[WORD_LEN] = {0};
    int guess_used[WORD_LEN] = {0};

    for (int i = 0; i < word_len; i++) {
        if (guess[i] == secret_word[i]) {
            (*bulls)++;
            word_used[i] = 1; guess_used[i] = 1;
        }
    }
    for (int i = 0; i < word_len; i++) {
        if (!guess_used[i]) {
            for (int j = 0; j < word_len; j++) {
                if (!word_used[j] && guess[i] == secret_word[j]) {
                    (*cows)++;
                    word_used[j] = 1;
                    break;
                }
            }
        }
    }
}

int main() {
    int fd = open("game_shm.dat", O_RDWR | O_CREAT | O_TRUNC, 0666);
    ftruncate(fd, sizeof(SharedMemory));
    SharedMemory* shm = mmap(NULL, sizeof(SharedMemory), PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    memset(shm, 0, sizeof(SharedMemory));

    printf("--- Server Started ---\n");

    while (1) {
        for (int i = 0; i < MAX_GAMES; i++) {
            GameData* game = &shm->games[i];
            if (strlen(game->gameName) == 0) continue;

            if (!game->gameStarted && game->joinedPlayers >= game->maxPlayers) {
                sem_wait(&game->shm_mutex);
                strcpy(game->word, "APPLE"); // Здесь можно сделать рандом из файла
                game->gameStarted = 1;
                game->currentPlayer = 0;
                printf("Game '%s' started! Word: %s\n", game->gameName, game->word);
                sem_post(&game->shm_mutex);
            }

            if (game->gameStarted && !game->gameOver && game->playerTurn != -1) {
                sem_wait(&game->shm_mutex);
                int pIdx = game->playerTurn;
                printf("[%s] Player %d guessed: %s\n", game->gameName, pIdx + 1, game->guess);

                if (strcmp(game->guess, game->word) == 0) {
                    game->gameOver = 1;
                    game->scores[pIdx]++;
                } else {
                    check_guess(game->word, game->guess, &game->bulls, &game->cows);
                    do {
                        game->currentPlayer = (game->currentPlayer + 1) % game->maxPlayers;
                    } while (game->players[game->currentPlayer].active == 0);
                }
                game->hasResult = 1;
                game->playerTurn = -1;
                sem_post(&game->shm_mutex);
            }
        }
        usleep(100000);
    }
    return 0;
}