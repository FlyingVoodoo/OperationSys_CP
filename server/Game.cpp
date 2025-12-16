#include "Game.hpp"
#include <algorithm>
#include <sstream>
#include <iomanip>

Game::Game(GameData* data) : data(data) {
}

std::string Game::generate_secret() {
    std::vector<int> digits = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9};
    std::random_device rd;
    std::mt19937 g(rd());
    std::shuffle(digits.begin(), digits.end(), g);
    
    std::string secret = "";
    for (int i = 0; i < SECRET_LENGTH; i++) {
        secret += std::to_string(digits[i]);
    }
    return secret;
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
        strcpy(data->secrets[i], data->secrets[i + 1]);
        data->attempts[i] = data->attempts[i + 1];
        data->finished[i] = data->finished[i + 1];
    }
    data->player_count--;
    
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
    
    for (int i = 0; i < data->player_count; i++) {
        std::string secret = generate_secret();
        strncpy(data->secrets[i], secret.c_str(), SECRET_LENGTH);
        data->secrets[i][SECRET_LENGTH] = '\0';
        data->attempts[i] = 0;
        data->finished[i] = false;
    }
    
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
        if (!isdigit(c)) return false;
    }
    
    for (size_t i = 0; i < guess.length(); i++) {
        for (size_t j = i + 1; j < guess.length(); j++) {
            if (guess[i] == guess[j]) return false;
        }
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
        return "ERROR: Invalid guess. Must be 4 unique digits";
    }
    
    data->attempts[idx]++;
    
    std::string secret = std::string(data->secrets[idx]);
    auto [bulls, cows] = calculate_bulls_and_cows(secret, guess);
    
    std::ostringstream oss;
    oss << "Bulls: " << bulls << ", Cows: " << cows;
    
    if (bulls == SECRET_LENGTH) {
        data->finished[idx] = true;
        oss << "\n🎉 CONGRATULATIONS! You guessed the number " << secret 
            << " in " << data->attempts[idx] << " attempts!";
        
        bool all_finished = true;
        for (int i = 0; i < data->player_count; i++) {
            if (!data->finished[i]) {
                all_finished = false;
                break;
            }
        }
        
        if (all_finished) {
            data->state = GAME_FINISHED;
            data->end_time = time(nullptr);
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
        if (data->finished[i]) {
            oss << ", FINISHED ✓";
        }
        oss << ")\n";
    }
    
    return oss.str();
}
