#include "Client.hpp"
#include <iostream>
#include <sstream>
#include <cstring>
#include <unistd.h>
#include <sys/time.h>

Client::Client() : shm(false), root(shm.root()), current_game_id(-1), in_game(false) {
    std::cout << "=== Bulls and Cows Client ===" << std::endl;
}

Client::~Client() {
}

ClientSlot* Client::my_slot() {
    for (size_t i = 0; i < MAX_CLIENTS; i++) {
        if (root->clients[i].used && strcmp(root->clients[i].login, login.c_str()) == 0) {
            return &root->clients[i];
        }
    }
    return nullptr;
}

bool Client::enqueue_message(const Message& m) {
    pthread_mutex_lock(&root->mutex);
    
    size_t next = (root->q_tail + 1) % QUEUE_SIZE;
    if (next == root->q_head) {
        pthread_mutex_unlock(&root->mutex);
        std::cerr << "Queue is full" << std::endl;
        return false;
    }
    
    root->queue[root->q_tail] = m;
    root->queue[root->q_tail].used = true;
    root->q_tail = next;
    
    pthread_cond_signal(&root->server_cond);
    pthread_mutex_unlock(&root->mutex);
    
    return true;
}

bool Client::wait_for_response(std::string &out, int timeout_ms) {
    ClientSlot* slot = my_slot();
    if (!slot) return false;
    
    pthread_mutex_lock(&root->mutex);
    
    struct timespec ts;
    struct timeval tv;
    gettimeofday(&tv, nullptr);
    ts.tv_sec = tv.tv_sec + timeout_ms / 1000;
    ts.tv_nsec = tv.tv_usec * 1000 + (timeout_ms % 1000) * 1000000;
    
    while (!slot->has_response) {
        int ret = pthread_cond_timedwait(&slot->cond, &root->mutex, &ts);
        if (ret != 0) {
            pthread_mutex_unlock(&root->mutex);
            return false;
        }
    }
    
    out = std::string(slot->response);
    slot->has_response = false;
    
    pthread_mutex_unlock(&root->mutex);
    
    return true;
}

void Client::run() {
    std::cout << "Enter your name: ";
    std::getline(std::cin, login);
    
    if (login.empty()) {
        std::cout << "Invalid name" << std::endl;
        return;
    }
    
    cmd_register();
    
    while (true) {
        if (in_game) {
            show_game_menu();
        } else {
            show_main_menu();
        }
    }
}

void Client::show_main_menu() {
    std::cout << "\n=== Main Menu ===" << std::endl;
    std::cout << "1. List games" << std::endl;
    std::cout << "2. Create game" << std::endl;
    std::cout << "3. Join game" << std::endl;
    std::cout << "4. Find any game" << std::endl;
    std::cout << "5. Exit" << std::endl;
    std::cout << "Choice: ";
    
    std::string choice;
    std::getline(std::cin, choice);
    
    if (choice == "1") {
        cmd_list_games();
    } else if (choice == "2") {
        cmd_create_game();
    } else if (choice == "3") {
        cmd_join_game();
    } else if (choice == "4") {
        cmd_find_game();
    } else if (choice == "5") {
        exit(0);
    }
}

void Client::show_game_menu() {
    std::cout << "\n=== Game Menu ===" << std::endl;
    std::cout << "1. Make guess (enter 4-digit number)" << std::endl;
    std::cout << "2. Game status" << std::endl;
    std::cout << "3. Leave game" << std::endl;
    std::cout << "Choice: ";
    
    std::string choice;
    std::getline(std::cin, choice);
    
    if (choice == "1") {
        cmd_guess();
    } else if (choice == "2") {
        cmd_game_status();
    } else if (choice == "3") {
        cmd_leave_game();
    } else if (choice.length() == 4 && isdigit(choice[0])) {
        Message m;
        m.used = true;
        strncpy(m.from, login.c_str(), LOGIN_MAX - 1);
        strcpy(m.to, "server");
        m.type = MSG_GUESS;
        strncpy(m.payload, choice.c_str(), CMD_MAX - 1);
        
        enqueue_message(m);
        
        std::string response;
        if (wait_for_response(response)) {
            std::cout << response << std::endl;
        } else {
            std::cout << "Timeout waiting for response" << std::endl;
        }
    }
}

void Client::cmd_register() {
    Message m;
    m.used = true;
    strncpy(m.from, login.c_str(), LOGIN_MAX - 1);
    m.from[LOGIN_MAX - 1] = '\0';
    strcpy(m.to, "server");
    m.type = MSG_REGISTER;
    m.payload[0] = '\0';
    
    enqueue_message(m);
    
    std::string response;
    if (wait_for_response(response)) {
        std::cout << response << std::endl;
    } else {
        std::cout << "Failed to register" << std::endl;
        exit(1);
    }
}

void Client::cmd_list_games() {
    Message m;
    m.used = true;
    strncpy(m.from, login.c_str(), LOGIN_MAX - 1);
    strcpy(m.to, "server");
    m.type = MSG_LIST_GAMES;
    m.payload[0] = '\0';
    
    enqueue_message(m);
    
    std::string response;
    if (wait_for_response(response)) {
        std::cout << response << std::endl;
    }
}

void Client::cmd_create_game() {
    std::cout << "Enter game name: ";
    std::string game_name;
    std::getline(std::cin, game_name);
    
    std::cout << "Enter max players (1-10, default 2): ";
    std::string max_str;
    std::getline(std::cin, max_str);
    int max_players = max_str.empty() ? 2 : std::stoi(max_str);
    
    Message m;
    m.used = true;
    strncpy(m.from, login.c_str(), LOGIN_MAX - 1);
    strcpy(m.to, "server");
    m.type = MSG_CREATE_GAME;
    
    std::ostringstream oss;
    oss << game_name << " " << max_players;
    strncpy(m.payload, oss.str().c_str(), CMD_MAX - 1);
    m.payload[CMD_MAX - 1] = '\0';
    
    enqueue_message(m);
    
    std::string response;
    if (wait_for_response(response)) {
        std::cout << response << std::endl;
        if (response.find("OK") == 0) {
            in_game = true;
            
            std::cout << "Waiting for other players or starting game..." << std::endl;
            sleep(1);
            
            cmd_game_status();
        }
    }
}

void Client::cmd_join_game() {
    std::cout << "Enter game name: ";
    std::string game_name;
    std::getline(std::cin, game_name);
    
    Message m;
    m.used = true;
    strncpy(m.from, login.c_str(), LOGIN_MAX - 1);
    strcpy(m.to, "server");
    m.type = MSG_JOIN_GAME;
    strncpy(m.payload, game_name.c_str(), CMD_MAX - 1);
    m.payload[CMD_MAX - 1] = '\0';
    
    enqueue_message(m);
    
    std::string response;
    if (wait_for_response(response)) {
        std::cout << response << std::endl;
        if (response.find("OK") == 0) {
            in_game = true;
            
            std::cout << "Waiting for game to start..." << std::endl;
            sleep(1);
            cmd_game_status();
        }
    }
}

void Client::cmd_find_game() {
    Message m;
    m.used = true;
    strncpy(m.from, login.c_str(), LOGIN_MAX - 1);
    strcpy(m.to, "server");
    m.type = MSG_FIND_GAME;
    m.payload[0] = '\0';
    
    enqueue_message(m);
    
    std::string response;
    if (wait_for_response(response)) {
        std::cout << response << std::endl;
        if (response.find("OK") == 0) {
            in_game = true;
            
            std::cout << "Waiting for game to start..." << std::endl;
            sleep(1);
            cmd_game_status();
        }
    }
}

void Client::cmd_guess() {
    std::cout << "Enter your guess (4 unique digits): ";
    std::string guess;
    std::getline(std::cin, guess);
    
    Message m;
    m.used = true;
    strncpy(m.from, login.c_str(), LOGIN_MAX - 1);
    strcpy(m.to, "server");
    m.type = MSG_GUESS;
    strncpy(m.payload, guess.c_str(), CMD_MAX - 1);
    m.payload[CMD_MAX - 1] = '\0';
    
    enqueue_message(m);
    
    std::string response;
    if (wait_for_response(response)) {
        std::cout << response << std::endl;
    }
}

void Client::cmd_game_status() {
    Message m;
    m.used = true;
    strncpy(m.from, login.c_str(), LOGIN_MAX - 1);
    strcpy(m.to, "server");
    m.type = MSG_GAME_STATUS;
    m.payload[0] = '\0';
    
    enqueue_message(m);
    
    std::string response;
    if (wait_for_response(response)) {
        std::cout << response << std::endl;
    }
}

void Client::cmd_leave_game() {
    Message m;
    m.used = true;
    strncpy(m.from, login.c_str(), LOGIN_MAX - 1);
    strcpy(m.to, "server");
    m.type = MSG_LEAVE_GAME;
    m.payload[0] = '\0';
    
    enqueue_message(m);
    
    std::string response;
    if (wait_for_response(response)) {
        std::cout << response << std::endl;
        in_game = false;
    }
}
