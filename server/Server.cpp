#include "Server.hpp"
#include <iostream>
#include <sstream>
#include <cstring>
#include <unistd.h>

Server::Server() : shm(true), root(shm.root()) {
    std::cout << "=== Bulls and Cows Server ===" << std::endl;
    std::cout << "Server initialized with shared memory" << std::endl;
}

Server::~Server() {
    for (auto& pair : games_map) {
        delete pair.second;
    }
}

void Server::run() {
    std::cout << "Server is running. Waiting for messages..." << std::endl;
    
    while (true) {
        pthread_mutex_lock(&root->mutex);
        
        while (root->q_head == root->q_tail) {
            pthread_cond_wait(&root->server_cond, &root->mutex);
        }
        
        Message m = root->queue[root->q_head];
        root->queue[root->q_head].used = false;
        root->q_head = (root->q_head + 1) % QUEUE_SIZE;
        
        pthread_mutex_unlock(&root->mutex);
        
        if (m.used) {
            handle_message(m);
        }
    }
}

ClientSlot* Server::find_or_create_client(const char* login) {
    for (size_t i = 0; i < MAX_CLIENTS; i++) {
        if (root->clients[i].used && strcmp(root->clients[i].login, login) == 0) {
            return &root->clients[i];
        }
    }
    
    for (size_t i = 0; i < MAX_CLIENTS; i++) {
        if (!root->clients[i].used) {
            root->clients[i].used = true;
            strncpy(root->clients[i].login, login, LOGIN_MAX - 1);
            root->clients[i].login[LOGIN_MAX - 1] = '\0';
            root->clients[i].has_response = false;
            root->clients[i].current_game_id = -1;
            return &root->clients[i];
        }
    }
    
    return nullptr;
}

ClientSlot* Server::find_client(const char* login) {
    for (size_t i = 0; i < MAX_CLIENTS; i++) {
        if (root->clients[i].used && strcmp(root->clients[i].login, login) == 0) {
            return &root->clients[i];
        }
    }
    return nullptr;
}

void Server::send_response_to(const char* login, const char* text) {
    ClientSlot* client = find_client(login);
    if (!client) return;
    
    pthread_mutex_lock(&root->mutex);
    strncpy(client->response, text, RESP_MAX - 1);
    client->response[RESP_MAX - 1] = '\0';
    client->has_response = true;
    pthread_cond_signal(&client->cond);
    pthread_mutex_unlock(&root->mutex);
}

void Server::handle_message(const Message &m) {
    std::cout << "[MSG] From: " << m.from << ", Type: " << (int)m.type << std::endl;
    
    switch (m.type) {
        case MSG_REGISTER:
            handle_register(m);
            break;
        case MSG_LIST_GAMES:
            handle_list_games(m);
            break;
        case MSG_CREATE_GAME:
            handle_create_game(m);
            break;
        case MSG_JOIN_GAME:
            handle_join_game(m);
            break;
        case MSG_FIND_GAME:
            handle_find_game(m);
            break;
        case MSG_GUESS:
            handle_guess(m);
            break;
        case MSG_LEAVE_GAME:
            handle_leave_game(m);
            break;
        case MSG_GAME_STATUS:
            handle_game_status(m);
            break;
        default:
            send_response_to(m.from, "ERROR: Unknown message type");
    }
}

void Server::handle_register(const Message &m) {
    pthread_mutex_lock(&root->mutex);
    ClientSlot* client = find_or_create_client(m.from);
    pthread_mutex_unlock(&root->mutex);
    
    if (client) {
        std::cout << "Client registered: " << m.from << std::endl;
        send_response_to(m.from, "OK: Registered successfully");
    } else {
        send_response_to(m.from, "ERROR: Server is full");
    }
}

void Server::handle_list_games(const Message &m) {
    std::ostringstream oss;
    oss << "Available games:\n";
    
    pthread_mutex_lock(&root->mutex);
    bool found = false;
    for (size_t i = 0; i < MAX_GAMES; i++) {
        if (root->games[i].used) {
            found = true;
            oss << "  - " << root->games[i].game_name 
                << " (" << root->games[i].player_count << "/" << root->games[i].max_players << " players)";
            
            if (root->games[i].state == GAME_WAITING) {
                oss << " [WAITING]";
            } else if (root->games[i].state == GAME_ACTIVE) {
                oss << " [ACTIVE]";
            } else {
                oss << " [FINISHED]";
            }
            oss << "\n";
        }
    }
    pthread_mutex_unlock(&root->mutex);
    
    if (!found) {
        oss << "  No games available\n";
    }
    
    send_response_to(m.from, oss.str().c_str());
}

int Server::create_game(const std::string& game_name, const std::string& creator, int max_players) {
    pthread_mutex_lock(&root->mutex);
    
    int game_id = -1;
    for (size_t i = 0; i < MAX_GAMES; i++) {
        if (!root->games[i].used) {
            game_id = i;
            break;
        }
    }
    
    if (game_id == -1) {
        pthread_mutex_unlock(&root->mutex);
        return -1;
    }
    
    GameData* gdata = &root->games[game_id];
    gdata->used = true;
    strncpy(gdata->game_name, game_name.c_str(), LOGIN_MAX - 1);
    gdata->game_name[LOGIN_MAX - 1] = '\0';
    gdata->player_count = 0;
    gdata->max_players = max_players;
    gdata->state = GAME_WAITING;
    gdata->start_time = 0;
    gdata->end_time = 0;
    
    Game* game = new Game(gdata);
    games_map[game_id] = game;
    
    root->game_count++;
    
    pthread_mutex_unlock(&root->mutex);
    
    std::cout << "Game created: " << game_name << " (ID: " << game_id << ")" << std::endl;
    
    return game_id;
}

void Server::handle_create_game(const Message &m) {
    std::istringstream iss(m.payload);
    std::string game_name;
    int max_players = 2;
    
    iss >> game_name >> max_players;
    
    if (game_name.empty()) {
        send_response_to(m.from, "ERROR: Game name required");
        return;
    }
    
    if (max_players < 1 || max_players > MAX_CLIENTS) {
        send_response_to(m.from, "ERROR: Invalid max_players");
        return;
    }
    
    int game_id = create_game(game_name, m.from, max_players);
    
    if (game_id == -1) {
        send_response_to(m.from, "ERROR: Cannot create game (server full)");
        return;
    }
    
    Game* game = get_game(game_id);
    if (game && game->add_player(m.from)) {
        pthread_mutex_lock(&root->mutex);
        ClientSlot* client = find_client(m.from);
        if (client) client->current_game_id = game_id;
        pthread_mutex_unlock(&root->mutex);
        
        std::ostringstream oss;
        oss << "OK: Game created: " << game_name << " (ID: " << game_id << ")";
        send_response_to(m.from, oss.str().c_str());
    }
}

Game* Server::get_game(int game_id) {
    auto it = games_map.find(game_id);
    if (it != games_map.end()) {
        return it->second;
    }
    return nullptr;
}

void Server::handle_join_game(const Message &m) {
    std::string game_name = m.payload;
    
    pthread_mutex_lock(&root->mutex);
    
    int game_id = -1;
    for (size_t i = 0; i < MAX_GAMES; i++) {
        if (root->games[i].used && strcmp(root->games[i].game_name, game_name.c_str()) == 0) {
            game_id = i;
            break;
        }
    }
    
    pthread_mutex_unlock(&root->mutex);
    
    if (game_id == -1) {
        send_response_to(m.from, "ERROR: Game not found");
        return;
    }
    
    Game* game = get_game(game_id);
    if (!game) {
        send_response_to(m.from, "ERROR: Game not found");
        return;
    }
    
    pthread_mutex_lock(&root->mutex);
    bool added = game->add_player(m.from);
    
    if (added) {
        ClientSlot* client = find_client(m.from);
        if (client) client->current_game_id = game_id;
        
        if (game->is_full() && game->can_start()) {
            game->start_game();
            std::cout << "Game " << game_name << " started!" << std::endl;
        }
        pthread_mutex_unlock(&root->mutex);
        
        send_response_to(m.from, "OK: Joined game successfully");
    } else {
        pthread_mutex_unlock(&root->mutex);
        send_response_to(m.from, "ERROR: Cannot join game (full or already joined)");
    }
}

void Server::handle_find_game(const Message &m) {
    pthread_mutex_lock(&root->mutex);
    
    int game_id = -1;
    for (size_t i = 0; i < MAX_GAMES; i++) {
        if (root->games[i].used && root->games[i].state == GAME_WAITING && 
            root->games[i].player_count < root->games[i].max_players) {
            game_id = i;
            break;
        }
    }
    
    pthread_mutex_unlock(&root->mutex);
    
    if (game_id == -1) {
        send_response_to(m.from, "ERROR: No available games found");
        return;
    }
    
    Game* game = get_game(game_id);
    if (!game) {
        send_response_to(m.from, "ERROR: Game not found");
        return;
    }
    
    pthread_mutex_lock(&root->mutex);
    bool added = game->add_player(m.from);
    
    if (added) {
        ClientSlot* client = find_client(m.from);
        if (client) client->current_game_id = game_id;
        
        if (game->is_full() && game->can_start()) {
            game->start_game();
        }
        pthread_mutex_unlock(&root->mutex);
        
        std::ostringstream oss;
        oss << "OK: Joined game: " << root->games[game_id].game_name;
        send_response_to(m.from, oss.str().c_str());
    } else {
        pthread_mutex_unlock(&root->mutex);
        send_response_to(m.from, "ERROR: Cannot join game");
    }
}

void Server::handle_guess(const Message &m) {
    pthread_mutex_lock(&root->mutex);
    ClientSlot* client = find_client(m.from);
    
    if (!client || client->current_game_id == -1) {
        pthread_mutex_unlock(&root->mutex);
        send_response_to(m.from, "ERROR: You are not in a game");
        return;
    }
    
    int game_id = client->current_game_id;
    pthread_mutex_unlock(&root->mutex);
    
    Game* game = get_game(game_id);
    if (!game) {
        send_response_to(m.from, "ERROR: Game not found");
        return;
    }
    
    std::string guess = m.payload;
    std::string result = game->make_guess(m.from, guess);
    send_response_to(m.from, result.c_str());
}

void Server::handle_leave_game(const Message &m) {
    pthread_mutex_lock(&root->mutex);
    ClientSlot* client = find_client(m.from);
    
    if (!client || client->current_game_id == -1) {
        pthread_mutex_unlock(&root->mutex);
        send_response_to(m.from, "ERROR: You are not in a game");
        return;
    }
    
    int game_id = client->current_game_id;
    client->current_game_id = -1;
    pthread_mutex_unlock(&root->mutex);
    
    Game* game = get_game(game_id);
    if (game) {
        game->remove_player(m.from);
        std::cout << "Player " << m.from << " left game " << game_id << std::endl;
    }
    
    send_response_to(m.from, "OK: Left game");
}

void Server::handle_game_status(const Message &m) {
    pthread_mutex_lock(&root->mutex);
    ClientSlot* client = find_client(m.from);
    
    if (!client || client->current_game_id == -1) {
        pthread_mutex_unlock(&root->mutex);
        send_response_to(m.from, "ERROR: You are not in a game");
        return;
    }
    
    int game_id = client->current_game_id;
    pthread_mutex_unlock(&root->mutex);
    
    Game* game = get_game(game_id);
    if (!game) {
        send_response_to(m.from, "ERROR: Game not found");
        return;
    }
    
    std::string status = game->get_status();
    send_response_to(m.from, status.c_str());
}

void Server::remove_game(int game_id) {
    auto it = games_map.find(game_id);
    if (it != games_map.end()) {
        delete it->second;
        games_map.erase(it);
    }
    
    pthread_mutex_lock(&root->mutex);
    root->games[game_id].used = false;
    root->game_count--;
    pthread_mutex_unlock(&root->mutex);
}
