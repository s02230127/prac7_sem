#pragma once

#include <string>
#include <vector>

#include "mafia/GameState.hpp"
#include "mafia/Logger.hpp"
#include "mafia/Moderator.hpp"

namespace mafia {

struct GameOptions {
    int player_count = 10;
    int mafia_divisor = 3;
    bool interactive = false;
    bool open_announcements = false;
    bool full_log = false;
    int ai_player_id = -1;

    std::string config_file = "config.yaml";
};

class Game {
public:
    explicit Game(GameOptions options);
    explicit Game(int player_count);

    void run();

private:
    struct PlayerStats {
        int votes_cast = 0;
        int votes_received = 0;
        int night_actions = 0;
    };

    GameView make_view_for(const Player& player) const;

    void day_phase();
    void night_phase();

    bool is_game_over() const;
    void print_winner() const;

    GameOptions options_;
    SharedPtr<GameState> state_;
    Moderator moderator_;
    Logger logger_;
    std::vector<PlayerStats> stats_;
    std::vector<std::string> public_history_;
    int human_last_healed_id_ = -1;
};

} // namespace mafia
