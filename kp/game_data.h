#define MAX_GAMES 5
#define MAX_PLAYERS 4
#define WORD_LEN 256

typedef struct {
    int active;
} PlayerInfo;

typedef struct {
    char gameName[64];
    int gameStarted;
    char word[WORD_LEN];
    int maxPlayers;
    int joinedPlayers;
    int currentPlayer;
    int gameOver;
    int scores[MAX_PLAYERS];
    PlayerInfo players[MAX_PLAYERS];
    int playerTurn;
    char guess[WORD_LEN];
    int bulls;
    int cows;
    int hasResult;
    sem_t shm_mutex;
} GameData;

typedef struct {
    int numGames;
    GameData games[MAX_GAMES];
} SharedMemory;