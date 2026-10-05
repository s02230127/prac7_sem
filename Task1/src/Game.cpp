#include "mafia/Game.hpp"
#include "mafia/Roles.hpp"
#include "mafia/Concepts.hpp"
#include "mafia/Config.hpp"

#include <algorithm>
#include <iostream>
#include <random>
#include <ranges>
#include <sstream>
#include <stdexcept>
#include <string>
#include <thread>
#include <utility>
#include <vector>

namespace mafia {
namespace {

struct DayDecision {
    int actor_id = -1;
    std::string message;
    int target_id = -1;

};

class ThreadGroup {
public:
    ~ThreadGroup() { join(); }

    template <typename Function>
    void start(Function&& function) {
        workers_.emplace_back(std::forward<Function>(function));
    }

    void join() {
        for (std::thread& worker : workers_) {
            if (worker.joinable()) {
                worker.join();
            }
        }
    }

private:
    std::vector<std::thread> workers_;
};

enum class Winner { None, Town, Mafia, Maniac, Draw };

const char* winner_name(Winner winner) {
    switch (winner) {
        case Winner::Town:
            return "мирные жители";
        case Winner::Mafia:
            return "мафия";
        case Winner::Maniac:
            return "маньяк";
        case Winner::Draw:
            return "ничья";
        case Winner::None:
            return "не определён";
    }
    return "не определён";
}

Winner winner_for(const GameState& state) {
    auto living_players = state.players |
        std::views::filter([](const PlayerState& entry) {
            return entry.alive && entry.player.get() != nullptr;
        });

    const auto mafia_count = std::ranges::count_if(
        living_players, [](const PlayerState& entry) {
            return is_mafia_role(entry.player->role());
        });
    const auto maniac_count = std::ranges::count_if(
        living_players, [](const PlayerState& entry) {
            return entry.player->role() == Role::Maniac;
        });
    const auto town_count = std::ranges::count_if(
        living_players, [](const PlayerState& entry) {
            return !is_mafia_role(entry.player->role()) &&
                   entry.player->role() != Role::Maniac;
        });

    if (mafia_count == 0 && maniac_count == 0) {
        return town_count == 0 ? Winner::Draw : Winner::Town;
    }
    if (mafia_count == 0 && maniac_count == 1 && town_count <= 1) {
        return Winner::Maniac;
    }
    if (maniac_count == 0 && mafia_count >= town_count) {
        return Winner::Mafia;
    }
    return Winner::None;
}

const char* role_name(Role role) {
    switch (role) {
        case Role::Civilian: 
            return "Мирный житель";

        case Role::Mafia: 
            return "Мафия";

        case Role::Commissioner: 
            return "Комиссар";

        case Role::Doctor: 
            return "Доктор";

        case Role::Maniac: 
            return "Маньяк";
        case Role::Ninja:
            return "Ниндзя";

        case Role::Bull:
            return "Бык";

        case Role::Elder:
            return "Старейшина";
    }
    return "Неизвестно";
}

const char* death_cause_name(DeathCause cause) {
    switch (cause) {
        case DeathCause::Mafia: 
            return "мафии";

        case DeathCause::Commissioner: 
            return "комиссара";

        case DeathCause::Maniac: 
            return "маньяка";
    }
    return "неизвестного";
}

const char* action_name(ActionType type) {
    switch (type) {
        case ActionType::None: 
            return "нет действия";

        case ActionType::MafiaKill: 
            return "выстрел мафии";

        case ActionType::Check:
             return "проверка";

        case ActionType::Shoot: 
            return "выстрел комиссара";

        case ActionType::Heal: 
            return "лечение";

        case ActionType::ManiacKill: 
            return "выстрел маньяка";
    }
    return "неизвестное действие";
}

bool replace_role(std::vector<Role>& roles, Role old_role, Role new_role) {
    auto it = std::find(roles.begin(), roles.end(), old_role);

    if (it == roles.end()) {
        return false;
    }

    *it = new_role;
    return true;
}

SharedPtr<Player> make_player(Role role, int id) {
    switch (role) {
        case Role::Civilian:
            return make_role<Civilian>(id);

        case Role::Mafia:
            return make_role<Mafia>(id);

        case Role::Commissioner:
            return make_role<Commissioner>(id);

        case Role::Doctor:
            return make_role<Doctor>(id);

        case Role::Maniac:
            return make_role<Maniac>(id);
        
        case Role::Ninja:
            return make_role<Ninja>(id);

        case Role::Bull:
            return make_role<Bull>(id);

        case Role::Elder:
            return make_role<Elder>(id);
    }

    throw std::runtime_error("Unknown role");
}

bool contains(const std::vector<int>& ids, int id) {
    return std::find(ids.begin(), ids.end(), id) != ids.end();
}

std::vector<int> eligible_targets(const GameView& view, int actor_id,
                                  bool allow_self = false, int excluded_id = -1,
                                  const std::vector<int>* forbidden = nullptr) {
    std::vector<int> result;
    for (const VisiblePlayer& player : view.players) {
        if (player.alive && player.id != excluded_id &&
            (allow_self || player.id != actor_id) &&
            (forbidden == nullptr || !contains(*forbidden, player.id))) {
            result.push_back(player.id);
        }
    }
    return result;
}

int ask_target(const std::vector<int>& candidates, const std::string& prompt) {
    if (candidates.empty()) {
        return -1;
    }
    for (;;) {
        std::cout << prompt << " (ID:";
        for (int id : candidates) {
            std::cout << ' ' << id;
        }
        std::cout << "): " << std::flush;

        std::string line;
        if (!std::getline(std::cin, line)) {
            return candidates.front();
        }
        std::istringstream input(line);
        int chosen = -1;
        if (input >> chosen && contains(candidates, chosen)) {
            return chosen;
        }
        std::cout << "Выберите ID из списка.\n";
    }
}

Action human_night_action(const Player& player, const GameView& view,
                          int& last_healed_id) {
    const int actor_id = player.id();
    int target = -1;
    ActionType type = ActionType::None;

    switch (player.role()) {
    case Role::Civilian:
    case Role::Elder:
        return {};
    case Role::Mafia:
    case Role::Ninja:
    case Role::Bull:
        target = ask_target(
            eligible_targets(view, actor_id, false, -1, &view.mafia_allies),
            "Мафия: кого убить ночью?");
        type = ActionType::MafiaKill;
        break;
    case Role::Doctor:
        target = ask_target(
            eligible_targets(view, actor_id, true, last_healed_id),
            "Доктор: кого лечить?");
        if (target != -1) {
            last_healed_id = target;
        }
        type = ActionType::Heal;
        break;
    case Role::Commissioner: {
        std::cout << "Комиссар: проверка (c) или выстрел (s)? " << std::flush;
        std::string choice;
        std::getline(std::cin, choice);
        type = (!choice.empty() && (choice[0] == 's' || choice[0] == 'S'))
                   ? ActionType::Shoot : ActionType::Check;
        target = ask_target(eligible_targets(view, actor_id),
                            "Комиссар: выберите цель");
        break;
    }
    case Role::Maniac:
        target = ask_target(eligible_targets(view, actor_id),
                            "Маньяк: кого убить ночью?");
        type = ActionType::ManiacKill;
        break;
    }

    return target == -1 ? Action{} : Action{type, target};
}

} // namespace

Game::Game(GameOptions options)
    : options_(options), state_(new GameState) {
    if (options_.player_count <= 4 || options_.mafia_divisor < 3) {
        throw std::invalid_argument("players must be > 4 and mafia divisor >= 3");
    }
    stats_.resize(static_cast<std::size_t>(options_.player_count));

    const int mafia_count =
        std::max(1, options_.player_count / options_.mafia_divisor);
    std::vector<Role> roles(mafia_count, Role::Mafia);
    roles.push_back(Role::Civilian);
    roles.push_back(Role::Commissioner);
    roles.push_back(Role::Doctor);
    roles.push_back(Role::Maniac);

    while (static_cast<int>(roles.size()) < options_.player_count) {
        roles.push_back(Role::Civilian);
    }

    const std::vector<Role> extra_roles =
    load_extra_roles(options_.config_file);
    
    for (Role extra_role : extra_roles) {
        bool replaced = false;

        switch (extra_role) {
            case Role::Ninja:
            case Role::Bull:
                replaced = replace_role(
                    roles,
                    Role::Mafia,
                    extra_role
                );
                break;

            case Role::Elder:
                replaced = replace_role(
                    roles,
                    Role::Civilian,
                    Role::Elder
                );
                break;

            default:
                break;
        }

        if (!replaced) {
            std::cerr << "Предупреждение: для роли " << role_name(extra_role)
                      << " не хватило мест при N=" << options_.player_count
                      << "; роль пропущена.\n";
        }
    }

    std::mt19937 generator(std::random_device{}());
    std::shuffle(roles.begin(), roles.end(), generator);
    state_->players.reserve(roles.size());
    for (std::size_t index = 0; index < roles.size(); ++index) {
        state_->players.push_back({
            make_player(roles[index], static_cast<int>(index) + 1), true
        });
    }
}

Game::Game(int player_count) : Game(GameOptions{player_count}) {}

GameView Game::make_view_for(const Player& player) const {
    GameView view;
    view.round = state_->round;
    view.phase = state_->phase;
    view.players.reserve(state_->players.size());

    for (const PlayerState& entry : state_->players) {
        view.players.push_back({entry.player->id(), entry.alive});
        if (is_mafia_role(player.role()) && entry.alive &&
            is_mafia_role(entry.player->role())) {
            view.mafia_allies.push_back(entry.player->id());
        }
    }
    return view;
}

void Game::day_phase() {
    logger_.log_round(state_->round, "=== DAY ===");
    std::cout << "\nДень " << state_->round << ". Обсуждение и голосование.\n";
    std::vector<SharedPtr<Player>> participants;
    std::vector<GameView> views;
    for (const PlayerState& entry :
        state_->players |
        std::views::filter([](const PlayerState& entry) {
            return entry.alive;
        })) {

        participants.push_back(entry.player);
        views.push_back(make_view_for(*entry.player));
    }

    std::vector<DayDecision> decisions(participants.size());
    ThreadGroup threads;
    for (std::size_t index = 0; index < participants.size(); ++index) {
        threads.start([&, index] {
            const SharedPtr<Player>& player = participants[index];
            const GameView& view = views[index];
            DayDecision& decision = decisions[index];
            decision.actor_id = player->id();

            if (options_.interactive && player->id() == 1) {
                std::cout << "Ваша дневная реплика: " << std::flush;
                std::getline(std::cin, decision.message);
                if (decision.message.empty()) {
                    decision.message = player->discuss(view);
                }
                decision.target_id = ask_target(
                    eligible_targets(view, player->id()),
                    "За кого вы голосуете?");
            } else {
                decision.message = player->discuss(view);
                decision.target_id = player->vote(view);
            }
        });
    }
    
    threads.join();

    std::vector<Vote> votes;
    votes.reserve(decisions.size());
    for (const DayDecision& decision : decisions) {
        std::cout << "Игрок " << decision.actor_id << ": "
                  << decision.message << '\n';
        votes.push_back({decision.actor_id, decision.target_id});

        logger_.log_round(
            state_->round,
            "Игрок " + std::to_string(decision.actor_id) +
            ": " + decision.message
        );

        logger_.log_round(
            state_->round,
            "Игрок " + std::to_string(decision.actor_id) +
            " проголосовал за " + std::to_string(decision.target_id)
        );

        if (decision.actor_id > 0 &&
            static_cast<std::size_t>(decision.actor_id) <= stats_.size()) {
            ++stats_[static_cast<std::size_t>(decision.actor_id - 1)].votes_cast;
        }

        if (decision.target_id > 0 &&
            static_cast<std::size_t>(decision.target_id) <= stats_.size()) {
            ++stats_[static_cast<std::size_t>(decision.target_id - 1)].votes_received;
        }

        if (options_.full_log) {
            std::cout << "  Голос: " << decision.target_id << '\n';
        }
    }

    const VoteResult result = moderator_.resolveVotes(*state_, votes);

    if (result.protected_id != -1) {
        if (options_.open_announcements) {
            std::cout << "Игрок " << result.protected_id
                    << " — Старейшина и не может быть исключён голосованием.\n";
        } else {
            std::cout << "Игрок " << result.protected_id
                    << " не выбыл по итогам голосования.\n";
        }

        logger_.log_round(
            state_->round,
            "Игрок " + std::to_string(result.protected_id) +
            " оказался Старейшиной и избежал исключения"
        );

    } else if (result.tie) {
        std::cout << "Голоса разделились поровну. Никто не выбыл.\n";

        logger_.log_round(
            state_->round,
            "Vote result: tie. Nobody eliminated."
        );


    } else if (result.eliminated_id == -1) {
        std::cout << "Днём никто не выбыл.\n";

        logger_.log_round(
            state_->round,
            "Nobody eliminated during the day."
        );

    } else {
        for (const PlayerState& entry : state_->players) {
            if (entry.player->id() == result.eliminated_id) {

                std::string announcement;

                if (options_.open_announcements) {
                    announcement = role_name(entry.player->role());
                } else if (is_mafia_role(entry.player->role())) {
                    announcement = "мафия";
                } else {
                    announcement = "не мафия";
                }

                std::cout << "Голосованием выбыл игрок "
                        << result.eliminated_id
                        << " (" << announcement << ").\n";

                logger_.log_round(
                    state_->round,
                    "Выбыл игрок " +
                    std::to_string(result.eliminated_id) +
                    " (роль: " + role_name(entry.player->role()) +
                    ", причина: дневное голосование)"
                );

                break;
            }
        }
    }
}

void Game::night_phase() {
    logger_.log_round(state_->round, "=== NIGHT ===");
    std::cout << "\nНочь " << state_->round << ".\n";
    std::vector<SharedPtr<Player>> participants;
    std::vector<GameView> views;
    for (const PlayerState& entry :
        state_->players |
        std::views::filter([](const PlayerState& entry) {
            return entry.alive;
        })) {

        participants.push_back(entry.player);
        views.push_back(make_view_for(*entry.player));
    }

    std::vector<PlayerAction> actions(participants.size());
    ThreadGroup threads;
    for (std::size_t index = 0; index < participants.size(); ++index) {
        threads.start([&, index] {
            const SharedPtr<Player>& player = participants[index];
            actions[index].actor_id = player->id();
            actions[index].action =
                (options_.interactive && player->id() == 1)
                    ? human_night_action(*player, views[index],
                                         human_last_healed_id_)
                    : player->act(views[index]);
        });
    }
    threads.join();

    // The other mafia members agree with the human member's private decision.
    if (options_.interactive) {
        int human_mafia_target = -1;
        for (const PlayerAction& action : actions) {
            if (action.actor_id == 1 &&
                action.action.type == ActionType::MafiaKill) {
                human_mafia_target = action.action.target_id;
                break;
            }
        }
        if (human_mafia_target != -1) {
            for (PlayerAction& action : actions) {
                for (const PlayerState& entry : state_->players) {
                    if (entry.alive && entry.player->id() == action.actor_id &&
                        is_mafia_role(entry.player->role())) {
                        action.action = {ActionType::MafiaKill,
                                         human_mafia_target};
                    }
                }
            }
        }
    }

    for (const PlayerAction& action : actions) {
        std::string text =
            "Игрок " + std::to_string(action.actor_id) +
            ": " + action_name(action.action.type);

        if (action.action.target_id != -1) {
            text += " -> " + std::to_string(action.action.target_id);
        }

        logger_.log_round(state_->round, text);

        if (action.action.type != ActionType::None &&
            action.actor_id > 0 &&
            static_cast<std::size_t>(action.actor_id) <= stats_.size()) {
            ++stats_[static_cast<std::size_t>(action.actor_id - 1)].night_actions;
        }

        if (options_.full_log) {
            std::cout << text << '\n';
        }
    }

    const NightResult result = moderator_.resolveNight(*state_, actions);

    if (result.healed_id != -1) {
        logger_.log_round(
            state_->round,
            "Доктор защищал игрока " + std::to_string(result.healed_id)
        );

        if (options_.open_announcements) {
            std::cout << "Доктор защищал игрока " << result.healed_id << ".\n";
        }
    }

    if (result.deaths.empty()) {
        std::cout << "Ночью никто не выбыл.\n";
        logger_.log_round(state_->round, "Ночью никто не выбыл.");
    }

    for (const Death& death : result.deaths) {
        for (const PlayerState& entry : state_->players) {
            if (entry.player->id() == death.player_id) {
                std::string announcement;

                if (options_.open_announcements) {
                    announcement = std::string(role_name(entry.player->role())) +
                                   ", выстрел " + death_cause_name(death.cause);
                } else if (is_mafia_role(entry.player->role())) {
                    announcement = "мафия";
                } else {
                    announcement = "не мафия";
                }

                std::cout << "Ночью выбыл игрок " << death.player_id
                          << " (" << announcement << ").\n";

                logger_.log_round(
                    state_->round,
                    "Ночью выбыл игрок " + std::to_string(death.player_id) +
                    " (роль: " + role_name(entry.player->role()) +
                    ", причина: выстрел " + death_cause_name(death.cause) + ")"
                );

                break;
            }
        }
    }

    for (const CheckResult& check : result.checks) {
        const std::string check_text =
            "Проверка комиссара " + std::to_string(check.commissioner_id) +
            " -> " + std::to_string(check.target_id) + ": " +
            (check.is_mafia ? "мафия" : "не мафия");

        logger_.log_round(state_->round, check_text);

        if (options_.interactive && check.commissioner_id == 1) {
            std::cout << "Проверка игрока " << check.target_id << ": "
                      << (check.is_mafia ? "мафия" : "не мафия") << ".\n";
        }

        if (options_.full_log) {
            std::cout << check_text << '\n';
        }
    }
}

bool Game::is_game_over() const {
    return winner_for(*state_) != Winner::None;
}

void Game::print_winner() const {
    std::cout << "\nИгра окончена. Победитель: "
              << winner_name(winner_for(*state_)) << ".\n";
}

void Game::run() {
    std::cout << "Игра начинается. Игроков: " << options_.player_count << ".\n";
    if (options_.interactive) {
        std::cout << "Вы играете за игрока 1. Ваша роль: "
                  << role_name(state_->players.front().player->role()) << ".\n";
    }

    while (!is_game_over()) {
        state_->phase = Phase::Day;
        day_phase();
        if (is_game_over()) {
            break;
        }
        state_->phase = Phase::Night;
        night_phase();
        ++state_->round;
    }
    print_winner();

    std::ostringstream summary;
    const Winner winner = winner_for(*state_);
    const int completed_rounds =
        state_->phase == Phase::Night ? state_->round - 1 : state_->round;

    summary << "Итог игры\n";
    summary << "Победитель: " << winner_name(winner) << '\n';
    summary << "Раундов сыграно: " << completed_rounds << "\n\n";
    summary << "Игроки:\n";

    for (const PlayerState& entry : state_->players) {
        const int id = entry.player->id();
        const PlayerStats& stats =
            stats_[static_cast<std::size_t>(id - 1)];

        summary << "Игрок " << id
                << ": " << role_name(entry.player->role())
                << ", " << (entry.alive ? "жив" : "мёртв") << '\n'
                << "  Голосов отдал: " << stats.votes_cast << '\n'
                << "  Голосов получил: " << stats.votes_received << '\n'
                << "  Ночных действий: " << stats.night_actions << '\n';
    }

    logger_.log_summary(summary.str());
}

} // namespace mafia
