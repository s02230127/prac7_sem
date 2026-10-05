#pragma once

#include <vector>

#include "mafia/Player.hpp"
#include "mafia/SharedPtr.hpp"
#include "mafia/GameView.hpp"

namespace mafia {

struct PlayerState {
    SharedPtr<Player> player;
    bool alive = true;
};

struct GameState {
    int round = 1;
    Phase phase = Phase::Day;

    std::vector<PlayerState> players;
};

} // namespace mafia