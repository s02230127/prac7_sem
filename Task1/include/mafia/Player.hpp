#pragma once

#include <string>

namespace mafia {

struct GameView;

enum class Role {
    Civilian,
    Mafia,
    Commissioner,
    Doctor,
    Maniac
};

enum class ActionType {
    None,
    MafiaKill,
    Check,
    Shoot,
    Heal,
    ManiacKill
};

struct Action {
    ActionType type = ActionType::None;
    int target_id = -1;
};

class Player {
public:
    virtual ~Player() = default;

    int id() const noexcept;
    Role role() const noexcept;

    virtual std::string discuss(const GameView& view) = 0;

    virtual int vote(const GameView& view) = 0;

    virtual Action act(const GameView& view) = 0;

protected:
    Player(int id, Role role) noexcept;

private:
    int id_;
    Role role_;
};

} // namespace mafia
