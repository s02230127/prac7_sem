#include "mafia/Roles.hpp"
#include "mafia/GameView.hpp"

#include <string>
#include <vector>
#include <cstddef>

namespace mafia {
namespace {

int choose_target(const GameView& view, int self_id, int excluded_id = -1,
                  bool allow_self = false) {
    std::vector<int> candidates;
    for (const VisiblePlayer& player : view.players) {
        if (player.alive && player.id != excluded_id &&
            (allow_self || player.id != self_id)) {
            candidates.push_back(player.id);
        }
    }

    if (candidates.empty()) {
        return -1;
    }
    const auto index = static_cast<std::size_t>(view.round + self_id) % candidates.size();
    return candidates[index];
}

bool is_mafia_ally(const GameView& view, int id) {
    for (int ally_id : view.mafia_allies) {
        if (ally_id == id) {
            return true;
        }
    }
    return false;
}

int choose_mafia_target(const GameView& view) {
    std::vector<int> candidates;

    for (const VisiblePlayer& player : view.players) {
        if (player.alive && !is_mafia_ally(view, player.id)) {
            candidates.push_back(player.id);
        }
    }

    if (candidates.empty()) {
        return -1;
    }

    const auto index =
        static_cast<std::size_t>(view.round) % candidates.size();

    return candidates[index];
}

std::string day_statement(const GameView& view, int self_id) {
    const int target = choose_target(view, self_id);
    if (target == -1) {
        return "Других живых игроков не осталось.";
    }
    return "Предлагаю обсудить игрока " + std::to_string(target) + ".";
}

} // namespace

Civilian::Civilian(int id) : Player(id, Role::Civilian) {}

std::string Civilian::discuss(const GameView& view) {
    return day_statement(view, id());
}

int Civilian::vote(const GameView& view) {
    return choose_target(view, id());
}

Action Civilian::act(const GameView&) {
    return {ActionType::None, -1};
}

Mafia::Mafia(int id) : Player(id, Role::Mafia) {}

std::string Mafia::discuss(const GameView& view) {
    return day_statement(view, id());
}

int Mafia::vote(const GameView& view) {
    return choose_target(view, id());
}

Action Mafia::act(const GameView& view) {
    const int target = choose_mafia_target(view);

    if (target == -1) {
        return {ActionType::None, -1};
    }

    return {ActionType::MafiaKill, target};
}

Doctor::Doctor(int id) : Player(id, Role::Doctor) {}

std::string Doctor::discuss(const GameView& view) {
    return day_statement(view, id());
}

int Doctor::vote(const GameView& view) {
    return choose_target(view, id());
}

Action Doctor::act(const GameView& view) {
    const int target = choose_target(view, id(), last_healed_id_, true);
    if (target == -1) {
        return {ActionType::None, -1};
    }
    last_healed_id_ = target;
    return {ActionType::Heal, target};
}

Commissioner::Commissioner(int id) : Player(id, Role::Commissioner) {}

std::string Commissioner::discuss(const GameView& view) {
    return day_statement(view, id());
}

int Commissioner::vote(const GameView& view) {
    return choose_target(view, id());
}

Action Commissioner::act(const GameView& view) {
    const int target = choose_target(view, id());
    if (target == -1) {
        return {ActionType::None, -1};
    }
    const ActionType type = view.round % 2 == 0
                                ? ActionType::Shoot
                                : ActionType::Check;
    return {type, target};
}

Maniac::Maniac(int id) : Player(id, Role::Maniac) {}

std::string Maniac::discuss(const GameView& view) {
    return day_statement(view, id());
}

int Maniac::vote(const GameView& view) {
    return choose_target(view, id());
}

Action Maniac::act(const GameView& view) {
    const int target = choose_target(view, id());
    return {target == -1 ? ActionType::None : ActionType::ManiacKill, target};
}

} // namespace mafia
