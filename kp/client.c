#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <semaphore.h>

int main() {
    int fd = open("game_shm.dat", O_RDWR);
    if (fd == -1) { perror("Run server first"); return 1; }
    SharedMemory* shm = mmap(NULL, sizeof(SharedMemory), PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);

    printf("1. Create Game\n2. Join Game\nChoice: ");
    int choice; scanf("%d", &choice);

    GameData* game = NULL;
    int myIdx = -1;

    if (choice == 1) {
        char name[64]; int count;
        printf("Enter game name: "); scanf("%s", name);
        printf("Number of players: "); scanf("%d", &count);
        
        for (int i = 0; i < MAX_GAMES; i++) {
            if (strlen(shm->games[i].gameName) == 0) {
                game = &shm->games[i];
                strcpy(game->gameName, name);
                game->maxPlayers = count;
                game->joinedPlayers = 1;
                game->playerTurn = -1;
                sem_init(&game->shm_mutex, 1, 1);
                myIdx = 0;
                game->players[myIdx].active = 1;
                break;
            }
        }
    } else {
        char name[64];
        printf("Enter game name to join: "); scanf("%s", name);
        for (int i = 0; i < MAX_GAMES; i++) {
            if (strcmp(shm->games[i].gameName, name) == 0) {
                game = &shm->games[i];
                sem_wait(&game->shm_mutex);
                if (game->joinedPlayers < game->maxPlayers) {
                    myIdx = game->joinedPlayers++;
                    game->players[myIdx].active = 1;
                }
                sem_post(&game->shm_mutex);
                break;
            }
        }
    }

    if (!game || myIdx == -1) { printf("Error joining game.\n"); return 1; }

    printf("Joined '%s' as Player %d. Waiting for start...\n", game->gameName, myIdx + 1);

    while (!game->gameOver) {
        if (!game->gameStarted) { usleep(500000); continue; }

        if (game->currentPlayer == myIdx) {
            if (game->hasResult) {
                printf("Bulls: %d, Cows: %d\n", game->bulls, game->cows);
                game->hasResult = 0;
            }
            printf("Your guess: ");
            char g[WORD_LEN]; scanf("%s", g);
            
            sem_wait(&game->shm_mutex);
            strcpy(game->guess, g);
            game->playerTurn = myIdx;
            sem_post(&game->shm_mutex);

            while (game->playerTurn == myIdx && !game->gameOver) usleep(100000);
        } else {
            printf("\rWaiting for Player %d...", game->currentPlayer + 1);
            fflush(stdout);
            sleep(1);
        }
    }

    printf("\nGame Over! Winner is Player %d\n", game->currentPlayer + 1);
    game->players[myIdx].active = 0;
    return 0;
}