#include "mafia/Game.hpp"
#include "mafia/Roles.hpp"
#include "mafia/Concepts.hpp"

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

Winner winner_for(const GameState& state) {
    int mafia_count = 0;
    int maniac_count = 0;
    int town_count = 0;

    for (const PlayerState& entry : state.players) {
        if (!entry.alive || entry.player.get() == nullptr) {
            continue;
        }
        switch (entry.player->role()) {
            case Role::Mafia:
                ++mafia_count;
                break;

            case Role::Maniac:
                ++maniac_count;
                break;

            case Role::Civilian:
            case Role::Commissioner:
            case Role::Doctor:
                ++town_count;
                break;
        }
    }

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
        return {};
    case Role::Mafia:
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
        if (player.role() == Role::Mafia && entry.alive &&
            entry.player->role() == Role::Mafia) {
            view.mafia_allies.push_back(entry.player->id());
        }
    }
    return view;
}

void Game::day_phase() {
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
        if (options_.full_log) {
            std::cout << "  Голос: " << decision.target_id << '\n';
        }
    }

    const VoteResult result = moderator_.resolveVotes(*state_, votes);
    if (result.tie) {
        std::cout << "Голоса разделились поровну. Никто не выбыл.\n";
    } else if (result.eliminated_id == -1) {
        std::cout << "Днём никто не выбыл.\n";
    } else {
        for (const PlayerState& entry : state_->players) {
            if (entry.player->id() == result.eliminated_id) {
                std::cout << "Голосованием выбыл игрок " << result.eliminated_id
                          << " (" << (options_.open_announcements
                                         ? role_name(entry.player->role())
                                         : (entry.player->role() == Role::Mafia
                                                ? "мафия" : "не мафия"))
                          << ").\n";
                break;
            }
        }
    }
}

void Game::night_phase() {
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
                        entry.player->role() == Role::Mafia) {
                        action.action = {ActionType::MafiaKill,
                                         human_mafia_target};
                    }
                }
            }
        }
    }

    if (options_.full_log) {
        for (const PlayerAction& action : actions) {
            std::cout << "Игрок " << action.actor_id << ": "
                      << action_name(action.action.type);
            if (action.action.target_id != -1) {
                std::cout << " -> " << action.action.target_id;
            }
            std::cout << '\n';
        }
    }

    const NightResult result = moderator_.resolveNight(*state_, actions);
    if (options_.open_announcements && result.healed_id != -1) {
        std::cout << "Доктор защищал игрока " << result.healed_id << ".\n";
    }
    if (result.deaths.empty()) {
        std::cout << "Ночью никто не выбыл.\n";
    }
    for (const Death& death : result.deaths) {
        for (const PlayerState& entry : state_->players) {
            if (entry.player->id() == death.player_id) {
                std::cout << "Ночью выбыл игрок " << death.player_id;
                if (options_.open_announcements) {
                    std::cout << " (" << role_name(entry.player->role())
                              << ", выстрел " << death_cause_name(death.cause)
                              << ')';
                } else {
                    std::cout << " (" << (entry.player->role() == Role::Mafia
                                           ? "мафия" : "не мафия") << ')';
                }
                std::cout << ".\n";
                break;
            }
        }
    }
    if (options_.interactive) {
        for (const CheckResult& check : result.checks) {
            if (check.commissioner_id == 1) {
                std::cout << "Проверка игрока " << check.target_id << ": "
                          << (check.is_mafia ? "мафия" : "не мафия") << ".\n";
            }
        }
    }
    if (options_.full_log) {
        for (const CheckResult& check : result.checks) {
            std::cout << "Проверка комиссара " << check.commissioner_id
                      << " -> " << check.target_id << ": "
                      << (check.is_mafia ? "мафия" : "не мафия") << '\n';
        }
    }
}

bool Game::is_game_over() const {
    return winner_for(*state_) != Winner::None;
}

void Game::print_winner() const {
    std::cout << "\nИгра окончена. Победитель: ";
    switch (winner_for(*state_)) {
        case Winner::Town: 
            std::cout << "мирные жители"; break;

        case Winner::Mafia: 
            std::cout << "мафия"; 
            break;

        case Winner::Maniac: 
            std::cout << "маньяк"; 
            break;

        case Winner::Draw: 
            std::cout << "ничья"; 
            break;
        case Winner::None: 
            std::cout << "не определён"; 
            break;
    }
    std::cout << ".\n";
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
}

} // namespace mafia
