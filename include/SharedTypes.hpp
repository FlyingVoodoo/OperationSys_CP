#pragma once
#include <cstdint>
#include <cstring>
#include <ctime>
#include <pthread.h>

constexpr const char* SHM_NAME = "/bulls_cows_shm";
constexpr size_t MAX_CLIENTS = 10;
constexpr size_t QUEUE_SIZE = 64;
constexpr size_t LOGIN_MAX = 32;
constexpr size_t CMD_MAX = 256;
constexpr size_t RESP_MAX = 512;

constexpr int SECRET_LENGTH = 4;
constexpr int MAX_GAMES = 16;

enum GameState : uint8_t {
    GAME_WAITING = 0,
    GAME_ACTIVE = 1,
    GAME_FINISHED = 2
};

struct GameData {
    bool used;
    char game_name[LOGIN_MAX];
    char players[MAX_CLIENTS][LOGIN_MAX];
    int player_count;
    int max_players;
    GameState state;
    
    char secrets[MAX_CLIENTS][SECRET_LENGTH + 1];
    
    int attempts[MAX_CLIENTS];
    bool finished[MAX_CLIENTS];
    
    time_t start_time;
    time_t end_time;
};

enum MsgType : uint8_t {
    MSG_REGISTER = 1,
    MSG_LIST_GAMES = 2,
    MSG_CREATE_GAME = 3,
    MSG_JOIN_GAME = 4,
    MSG_FIND_GAME = 5,
    MSG_GUESS = 6,
    MSG_LEAVE_GAME = 7,
    MSG_GAME_STATUS = 8,
    MSG_QUIT = 9
};

struct Message {
    bool used;
    char from[LOGIN_MAX];
    char to[LOGIN_MAX];
    uint8_t type;
    char payload[CMD_MAX];
};

struct ClientSlot {
    bool used;
    char login[LOGIN_MAX];
    pthread_cond_t cond;
    char response[RESP_MAX];
    bool has_response;
    int current_game_id;
};

struct SharedMemoryRoot {
    pthread_mutex_t mutex;
    pthread_cond_t server_cond;
    
    Message queue[QUEUE_SIZE];
    size_t q_head;
    size_t q_tail;
    
    ClientSlot clients[MAX_CLIENTS];
    
    GameData games[MAX_GAMES];
    size_t game_count;
};
