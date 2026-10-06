#pragma once

#include <string>
#include <vector>

namespace mafia {

enum class Phase {
    Day,
    Night
};

struct VisiblePlayer {
    int id;
    bool alive;
};

struct GameView {
    int round = 0;
    Phase phase = Phase::Day;

    int random_shift = 0;
    
    std::vector<VisiblePlayer> players;
    std::vector<int> mafia_allies;
    std::vector<std::string> history;
};


} // namespace mafia
