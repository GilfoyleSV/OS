#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
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

int main(int argc, char *argv[]) {
    int fd;
    GameData* gameData;
    int myPlayerIndex = -1;

    fd = open(GAME_MAP_FILE, O_RDWR);
    if (fd == -1) {
        perror("open failed. Is server running?");
        return 1;
    }

    gameData = (GameData*)mmap(NULL, sizeof(GameData), PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    if (gameData == MAP_FAILED) {
        perror("mmap failed");
        close(fd);
        return 1;
    }

    printf("--- Client Connected (File Mapping) ---\n");

    if (sem_wait(&gameData->shm_mutex) == -1) {
        perror("sem_wait failed");
        munmap(gameData, sizeof(GameData));
        close(fd);
        return 1;
    }

    if (gameData->numPlayers < MAX_PLAYERS) {
        myPlayerIndex = gameData->numPlayers;
        gameData->numPlayers++;
        gameData->scores[myPlayerIndex] = 0;
        printf("Joined as Player %d (Index: %d).\n", myPlayerIndex + 1, myPlayerIndex);
    } else {
        printf("Game is full. Cannot join.\n");
        sem_post(&gameData->shm_mutex);
        munmap(gameData, sizeof(GameData));
        close(fd);
        return 1;
    }

    sem_post(&gameData->shm_mutex);

    printf("Waiting for the game to start...\n");
    while (!gameData->gameStarted) {
        if (gameData->gameOver) {
            printf("Game was shut down.\n");
            munmap(gameData, sizeof(GameData));
            close(fd);
            return 0;
        }
        sleep(1);
    }

    while (!gameData->gameOver) {
        if (gameData->currentPlayer == myPlayerIndex) {
            if (gameData->hasResult) {
                printf("\n--- Last Guess Result ---\n");
                printf("Bulls: %d, Cows: %d\n", gameData->bulls, gameData->cows);
                gameData->hasResult = 0;
            }
            
            printf("\n>>> Player %d, enter your guess: ", myPlayerIndex + 1);
            char guess[WORD_LEN];
            if (scanf("%s", guess) != 1) break;
            
            sem_wait(&gameData->shm_mutex);
            strncpy(gameData->guess, guess, WORD_LEN - 1);
            gameData->guess[WORD_LEN - 1] = '\0';
            gameData->playerTurn = myPlayerIndex;
            sem_post(&gameData->shm_mutex);
            
            printf("Waiting for server...\n");
            while (gameData->currentPlayer == myPlayerIndex && !gameData->gameOver) {
                usleep(100000);
            }
        } else {
            if (gameData->hasResult) {
                printf("\n--- Result from Player %d: Bulls: %d, Cows: %d ---\n", 
                        (gameData->currentPlayer == 0 ? gameData->numPlayers : gameData->currentPlayer), 
                        gameData->bulls, gameData->cows);
                gameData->hasResult = 0;
            }
            printf("\rWaiting for Player %d...     ", gameData->currentPlayer + 1);
            fflush(stdout);
            sleep(1);
        }
    }

    printf("\n--- Game Over ---\n");
    printf("Final Scores:\n");
    for (int i = 0; i < gameData->numPlayers; i++) {
        printf("Player %d: %d\n", i + 1, gameData->scores[i]);
    }

    munmap(gameData, sizeof(GameData));
    close(fd);
    
    return 0;
}