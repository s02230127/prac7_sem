#pragma once

#include "mafia/GameView.hpp"

#include <optional>
#include <string>
#include <string_view>

namespace mafia {

struct AiDecision {
    std::string message;
    int target_id = -1;
    std::string reasoning;
};

class AiAgent {
public:
    static std::optional<AiDecision> decide(
        const GameView& view,
        int self_id,
        std::string_view role
    );
};

} // namespace mafia
