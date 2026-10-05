#pragma once

#include <vector>

#include "mafia/GameState.hpp"
#include "mafia/Player.hpp"

namespace mafia {

struct Vote {
    int voter_id;
    int target_id;
};

struct PlayerAction {
    int actor_id;
    Action action;
};

struct VoteResult {
    int eliminated_id = -1;
    bool tie = false;
};

struct CheckResult {
    int commissioner_id = -1;
    int target_id = -1;
    bool is_mafia = false;
};

enum class DeathCause {
    Mafia,
    Commissioner,
    Maniac
};

struct Death {
    int player_id = -1;
    DeathCause cause;
};

struct NightResult {
    std::vector<Death> deaths;
    int healed_id = -1;
    std::vector<CheckResult> checks;
};

class Moderator {
public:
    VoteResult resolveVotes(
        GameState& state,
        const std::vector<Vote>& votes
    ) const;

    NightResult resolveNight(
        GameState& state,
        const std::vector<PlayerAction>& actions
    ) const;
};

} // namespace mafia