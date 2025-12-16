#pragma once
#include "../include/SharedTypes.hpp"
#include <string>
#include <vector>
#include <random>

class Game {
public:
    Game(GameData* data);
    
    bool add_player(const std::string& login);
    bool remove_player(const std::string& login);
    bool is_full() const;
    bool can_start() const;
    void start_game();
    
    std::string make_guess(const std::string& player, const std::string& guess);
    bool is_player_finished(const std::string& player) const;
    bool is_game_finished() const;
    std::string get_status() const;
    
private:
    GameData* data;
    
    std::string generate_secret();
    bool is_valid_guess(const std::string& guess) const;
    std::pair<int, int> calculate_bulls_and_cows(const std::string& secret, const std::string& guess);
    int find_player_index(const std::string& player) const;
};
