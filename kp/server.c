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

void check_guess(const char* secret_word, const char* guess, int* bulls, int* cows) {
    int word_len = strlen(secret_word);
    int guess_len = strlen(guess);
    *bulls = 0;
    *cows = 0;

    if (word_len != guess_len) return;

    int word_used[WORD_LEN] = {0};
    int guess_used[WORD_LEN] = {0};

    for (int i = 0; i < word_len; i++) {
        if (guess[i] == secret_word[i]) {
            (*bulls)++;
            word_used[i] = 1;
            guess_used[i] = 1;
        }
    }

    for (int i = 0; i < guess_len; i++) {
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
    int fd;
    GameData* gameData;
    FILE* wordsFile;
    
    srand(time(NULL));

    fd = open(GAME_MAP_FILE, O_RDWR | O_CREAT | O_TRUNC, 0666);
    if (fd == -1) {
        perror("open failed");
        return 1;
    }

    if (ftruncate(fd, sizeof(GameData)) == -1) {
        perror("ftruncate failed");
        close(fd);
        return 1;
    }

    gameData = (GameData*)mmap(NULL, sizeof(GameData), PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    if (gameData == MAP_FAILED) {
        perror("mmap failed");
        close(fd);
        return 1;
    }

    memset(gameData, 0, sizeof(GameData));
    
    if (sem_init(&gameData->shm_mutex, 1, 1) == -1) {
        perror("sem_init failed");
        munmap(gameData, sizeof(GameData));
        close(fd);
        return 1;
    }

    printf("--- Server Started (File Mapping) ---\n");

    while (1) {
        if (!gameData->gameStarted) {
            printf("Waiting for players (Current: %d/%d)...\n", gameData->numPlayers, MAX_PLAYERS);
            while (gameData->numPlayers < 2) {
                sleep(2);
                if (gameData->gameOver) break;
            }
            if (gameData->gameOver) break;

            sem_wait(&gameData->shm_mutex);
            if (!gameData->gameStarted) {
                wordsFile = fopen("words.txt", "r");
                if (wordsFile != NULL) {
                    int numWords = 0;
                    if (fscanf(wordsFile, "%d", &numWords) != 1) {
                        rewind(wordsFile);
                        char temp[WORD_LEN];
                        while (fscanf(wordsFile, "%s", temp) == 1) numWords++;
                    }
                    
                    if (numWords > 0) {
                        int randomWordIndex = rand() % numWords;
                        rewind(wordsFile);
                        char temp[WORD_LEN];
                        
                        long startPos = ftell(wordsFile);
                        if (fscanf(wordsFile, "%d", &numWords) != 1) fseek(wordsFile, startPos, SEEK_SET);

                        for (int i = 0; i < randomWordIndex; i++) {
                            if (fscanf(wordsFile, "%s", temp) != 1) break;
                        }
                        if (fscanf(wordsFile, "%s", gameData->word) != 1) strcpy(gameData->word, "CODE");
                    } else {
                        strcpy(gameData->word, "GAME");
                    }
                    fclose(wordsFile);
                } else {
                    strcpy(gameData->word, "SERVER");
                }

                gameData->gameStarted = 1;
                gameData->currentPlayer = 0;
                gameData->gameOver = 0;
                gameData->playerTurn = -1;
                gameData->hasResult = 0;
                printf("Game started! Word length: %zu\n", strlen(gameData->word));
            }
            sem_post(&gameData->shm_mutex);
        }

        if (gameData->gameStarted && !gameData->gameOver) {
            if (gameData->playerTurn != gameData->currentPlayer) {
                usleep(100000);
                continue;
            }

            sem_wait(&gameData->shm_mutex);
            if (gameData->playerTurn == gameData->currentPlayer) {
                printf("Player %d guessed: %s\n", gameData->currentPlayer + 1, gameData->guess);

                if (strcmp(gameData->guess, gameData->word) == 0) {
                    gameData->gameOver = 1;
                    gameData->scores[gameData->currentPlayer]++;
                } else {
                    check_guess(gameData->word, gameData->guess, &gameData->bulls, &gameData->cows);
                    gameData->currentPlayer = (gameData->currentPlayer + 1) % gameData->numPlayers;
                }
                gameData->hasResult = 1;
                gameData->playerTurn = -1;
            }
            sem_post(&gameData->shm_mutex);
        } else if (gameData->gameOver) {
            printf("Game Over. Winner: Player %d\n", gameData->currentPlayer + 1);
            break;
        }
    }

    sem_destroy(&gameData->shm_mutex);
    munmap(gameData, sizeof(GameData));
    close(fd);
    unlink(GAME_MAP_FILE);

    return 0;
}