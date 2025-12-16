#pragma once
#include "../include/SharedTypes.hpp"
#include "../include/SharedMemory.hpp"
#include "Game.hpp"
#include <string>
#include <vector>
#include <unordered_map>

class Server {
public:
    Server();
    ~Server();
    void run();

private:
    SharedMemory shm;
    SharedMemoryRoot* root;
    
    std::unordered_map<int, Game*> games_map;
    
    void handle_message(const Message &m);
    void send_response_to(const char* login, const char* text);
    
    ClientSlot* find_or_create_client(const char* login);
    ClientSlot* find_client(const char* login);
    
    void handle_register(const Message &m);
    void handle_list_games(const Message &m);
    void handle_create_game(const Message &m);
    void handle_join_game(const Message &m);
    void handle_find_game(const Message &m);
    void handle_guess(const Message &m);
    void handle_leave_game(const Message &m);
    void handle_game_status(const Message &m);
    
    int create_game(const std::string& game_name, const std::string& creator, int max_players);
    Game* get_game(int game_id);
    void remove_game(int game_id);
};
