#include "Game.hpp"
#include <sstream>

Game::Game(GameData* data) : data(data) {
}

std::string Game::generate_secret() {
    std::vector<std::string> words = {
        "apple", "beach", "chair", "dance", "earth",
        "flame", "grace", "house", "image", "juice",
        "knife", "lemon", "music", "night", "ocean",
        "pearl", "queen", "river", "stone", "table",
        "unity", "voice", "water", "youth", "zebra",
        "bread", "cloud", "dream", "field", "globe",
        "heart", "light", "magic", "paint", "smile",
        "storm", "tiger", "tower", "whale", "world"
    };
    
    std::random_device rd;
    std::mt19937 g(rd());
    std::uniform_int_distribution<> dis(0, words.size() - 1);
    
    return words[dis(g)];
}

bool Game::add_player(const std::string& login) {
    if (is_full() || data->state != GAME_WAITING) {
        return false;
    }
    
    for (int i = 0; i < data->player_count; i++) {
        if (strcmp(data->players[i], login.c_str()) == 0) {
            return false;
        }
    }
    
    strncpy(data->players[data->player_count], login.c_str(), LOGIN_MAX - 1);
    data->players[data->player_count][LOGIN_MAX - 1] = '\0';
    data->player_count++;
    
    return true;
}

bool Game::remove_player(const std::string& login) {
    int idx = find_player_index(login);
    if (idx == -1) return false;
    
    for (int i = idx; i < data->player_count - 1; i++) {
        strcpy(data->players[i], data->players[i + 1]);
        data->attempts[i] = data->attempts[i + 1];
        data->finished[i] = data->finished[i + 1];
    }
    data->player_count--;
    
    if (data->winner_index == idx) {
        data->winner_index = -1;
    } else if (data->winner_index > idx) {
        data->winner_index--;
    }
    
    if (data->player_count == 0) {
        data->used = false;
    }
    
    return true;
}

bool Game::is_full() const {
    return data->player_count >= data->max_players;
}

bool Game::can_start() const {
    return data->player_count > 0 && data->state == GAME_WAITING;
}

void Game::start_game() {
    if (!can_start()) return;
    
    std::string secret = generate_secret();
    strncpy(data->secret, secret.c_str(), SECRET_LENGTH);
    data->secret[SECRET_LENGTH] = '\0';
    
    for (int i = 0; i < data->player_count; i++) {
        data->attempts[i] = 0;
        data->finished[i] = false;
    }
    
    data->winner_index = -1;
    data->state = GAME_ACTIVE;
    data->start_time = time(nullptr);
}

int Game::find_player_index(const std::string& player) const {
    for (int i = 0; i < data->player_count; i++) {
        if (strcmp(data->players[i], player.c_str()) == 0) {
            return i;
        }
    }
    return -1;
}

bool Game::is_valid_guess(const std::string& guess) const {
    if (guess.length() != SECRET_LENGTH) return false;
    
    for (char c : guess) {
        if (!islower(c) || !isalpha(c)) return false;
    }
    
    return true;
}

std::pair<int, int> Game::calculate_bulls_and_cows(const std::string& secret, const std::string& guess) {
    int bulls = 0, cows = 0;
    
    for (int i = 0; i < SECRET_LENGTH; i++) {
        if (secret[i] == guess[i]) {
            bulls++;
        } else {
            for (int j = 0; j < SECRET_LENGTH; j++) {
                if (i != j && secret[i] == guess[j]) {
                    cows++;
                    break;
                }
            }
        }
    }
    
    return {bulls, cows};
}

std::string Game::make_guess(const std::string& player, const std::string& guess) {
    if (data->state != GAME_ACTIVE) {
        return "ERROR: Game is not active";
    }
    
    int idx = find_player_index(player);
    if (idx == -1) {
        return "ERROR: You are not in this game";
    }
    
    if (data->finished[idx]) {
        return "INFO: You already finished!";
    }
    
    if (!is_valid_guess(guess)) {
        return "ERROR: Invalid guess. Must be a 5-letter word in lowercase";
    }
    
    data->attempts[idx]++;
    
    std::string secret = std::string(data->secret);
    auto [bulls, cows] = calculate_bulls_and_cows(secret, guess);
    
    std::ostringstream oss;
    oss << "Attempt #" << data->attempts[idx] << " - Bulls: " << bulls << ", Cows: " << cows;
    
    if (bulls == SECRET_LENGTH) {
        data->finished[idx] = true;
        
        if (data->winner_index == -1) {
            data->winner_index = idx;
            oss << "\n🎉 CONGRATULATIONS! You are the WINNER! You guessed the word: \"" << secret 
                << "\" in " << data->attempts[idx] << " attempts!";
            
            data->state = GAME_FINISHED;
            data->end_time = time(nullptr);
        } else {
            oss << "\nYou guessed the word \"" << secret << "\", but " 
                << data->players[data->winner_index] << " was faster!";
        }
    }
    
    return oss.str();
}

bool Game::is_player_finished(const std::string& player) const {
    int idx = find_player_index(player);
    if (idx == -1) return false;
    return data->finished[idx];
}

bool Game::is_game_finished() const {
    return data->state == GAME_FINISHED;
}

std::string Game::get_status() const {
    std::ostringstream oss;
    oss << "Game: " << data->game_name << "\n";
    oss << "State: ";
    
    switch (data->state) {
        case GAME_WAITING: oss << "Waiting for players"; break;
        case GAME_ACTIVE: oss << "Active"; break;
        case GAME_FINISHED: oss << "Finished"; break;
    }
    
    oss << "\nPlayers (" << data->player_count << "/" << data->max_players << "):\n";
    
    for (int i = 0; i < data->player_count; i++) {
        oss << "  - " << data->players[i] 
            << " (attempts: " << data->attempts[i];
        if (i == data->winner_index) {
            oss << ", 🏆 WINNER! ✓";
        } else if (data->finished[i]) {
            oss << ", finished";
        }
        oss << ")\n";
    }
    
    if (data->state == GAME_FINISHED && data->winner_index >= 0) {
        oss << "\n🏆 Winner: " << data->players[data->winner_index] 
            << " guessed the word in " << data->attempts[data->winner_index] << " attempts!";
    }
    
    return oss.str();
}
