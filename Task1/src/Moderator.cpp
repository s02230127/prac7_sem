#include "mafia/Moderator.hpp"

#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

namespace mafia {
namespace {

PlayerState* find_player(GameState& state, int id) {
    for (PlayerState& entry : state.players) {
        if (entry.player.get() != nullptr && entry.player->id() == id) {
            return &entry;
        }
    }
    return nullptr;
}

bool is_alive(GameState& state, int id) {
    const PlayerState* entry = find_player(state, id);
    return entry != nullptr && entry->alive;
}

bool action_matches_role(Role role, ActionType type) {
    switch (role) {
    case Role::Civilian:
        return type == ActionType::None;
    case Role::Mafia:
        return type == ActionType::MafiaKill;
    case Role::Commissioner:
        return type == ActionType::Check || type == ActionType::Shoot;
    case Role::Doctor:
        return type == ActionType::Heal;
    case Role::Maniac:
        return type == ActionType::ManiacKill;
    }
    return false;
}

} // namespace

VoteResult Moderator::resolveVotes(GameState& state,
                                  const std::vector<Vote>& votes) const {
    std::unordered_set<int> counted_voters;
    std::unordered_map<int, int> vote_counts;

    for (const Vote& vote : votes) {
        if (vote.voter_id == vote.target_id ||
            !is_alive(state, vote.voter_id) ||
            !is_alive(state, vote.target_id) ||
            !counted_voters.insert(vote.voter_id).second) {
            continue;
        }
        ++vote_counts[vote.target_id];
    }

    VoteResult result;
    int highest_count = 0;
    for (const auto& [target_id, count] : vote_counts) {
        if (count > highest_count) {
            highest_count = count;
            result.eliminated_id = target_id;
            result.tie = false;
        } else if (count == highest_count) {
            result.eliminated_id = -1;
            result.tie = true;
        }
    }

    if (result.eliminated_id != -1) {
        find_player(state, result.eliminated_id)->alive = false;
    }
    return result;
}

NightResult Moderator::resolveNight(
    GameState& state, const std::vector<PlayerAction>& actions) const {
    NightResult result;
    std::unordered_set<int> acted_players;
    std::vector<int> mafia_targets;
    std::vector<std::pair<int, DeathCause>> attacks;

    int living_mafia = 0;
    for (const PlayerState& entry : state.players) {
        if (entry.alive && entry.player.get() != nullptr &&
            entry.player->role() == Role::Mafia) {
            ++living_mafia;
        }
    }

    // Collect decisions before changing alive flags: actions happen in one night.
    for (const PlayerAction& player_action : actions) {
        PlayerState* actor = find_player(state, player_action.actor_id);
        if (actor == nullptr || !actor->alive) {
            continue;
        }

        const Action& action = player_action.action;
        if (action.type == ActionType::None ||
            !action_matches_role(actor->player->role(), action.type) ||
            !is_alive(state, action.target_id) ||
            (action.target_id == player_action.actor_id &&
             action.type != ActionType::Heal) ||
            !acted_players.insert(player_action.actor_id).second) {
            continue;
        }

        switch (action.type) {
        case ActionType::MafiaKill: {
            PlayerState* target = find_player(state, action.target_id);

            if (target->player->role() != Role::Mafia) {
                mafia_targets.push_back(action.target_id);
            }

            break;
        }
        case ActionType::Check:
            result.checks.push_back({
                player_action.actor_id,
                action.target_id,
                find_player(state, action.target_id)->player->role() == Role::Mafia
            });
            break;
        case ActionType::Shoot:
            attacks.emplace_back(action.target_id, DeathCause::Commissioner);
            break;
        case ActionType::Heal:
            result.healed_id = action.target_id;
            break;
        case ActionType::ManiacKill:
            attacks.emplace_back(action.target_id, DeathCause::Maniac);
            break;
        case ActionType::None:
            break;
        }
    }

    // The mafia gets one collective kill only when every living member agrees.
    if (living_mafia > 0 &&
        mafia_targets.size() == static_cast<std::size_t>(living_mafia)) {
        const int target_id = mafia_targets.front();
        bool unanimous = true;
        for (int vote_target : mafia_targets) {
            if (vote_target != target_id) {
                unanimous = false;
                break;
            }
        }
        if (unanimous) {
            attacks.emplace_back(target_id, DeathCause::Mafia);
        }
    }

    std::unordered_set<int> killed_players;
    for (const auto& [target_id, cause] : attacks) {
        if (target_id == result.healed_id ||
            !killed_players.insert(target_id).second) {
            continue;
        }
        result.deaths.push_back({target_id, cause});
    }

    for (const Death& death : result.deaths) {
        find_player(state, death.player_id)->alive = false;
    }
    return result;
}

} // namespace mafia
