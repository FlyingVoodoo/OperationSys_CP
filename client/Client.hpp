#pragma once
#include "../include/SharedTypes.hpp"
#include "../include/SharedMemory.hpp"
#include <string>

class Client {
public:
    Client();
    ~Client();

    void run();

private:
    SharedMemory shm;
    SharedMemoryRoot* root;
    std::string login;
    int current_game_id;
    bool in_game;

    bool enqueue_message(const Message& m);
    bool wait_for_response(std::string &out, int timeout_ms = 3000);
    ClientSlot* my_slot();
    
    void show_main_menu();
    void show_game_menu();
    
    void cmd_register();
    void cmd_list_games();
    void cmd_create_game();
    void cmd_join_game();
    void cmd_find_game();
    void cmd_guess();
    void cmd_game_status();
    void cmd_leave_game();
};
