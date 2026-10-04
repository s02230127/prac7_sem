#pragma once

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
    std::vector<VisiblePlayer> players;
};

} // namespace mafia