#pragma once

#include "mafia/Player.hpp"

namespace mafia {

class Civilian : public Player {
public:
    explicit Civilian(int id);

    std::string discuss(const GameView& view) override;
    int vote(const GameView& view) override;
    Action act(const GameView& view) override;
};

class Mafia : public Player {
public:
    explicit Mafia(int id);

    std::string discuss(const GameView& view) override;
    int vote(const GameView& view) override;
    Action act(const GameView& view) override;
};

class Doctor : public Player {
public:
    explicit Doctor(int id);

    std::string discuss(const GameView& view) override;
    int vote(const GameView& view) override;
    Action act(const GameView& view) override;

private:
    int last_healed_id_ = -1;
};

class Commissioner : public Player {
public:
    explicit Commissioner(int id);

    std::string discuss(const GameView& view) override;
    int vote(const GameView& view) override;
    Action act(const GameView& view) override;
};

class Maniac : public Player {
public:
    explicit Maniac(int id);

    std::string discuss(const GameView& view) override;
    int vote(const GameView& view) override;
    Action act(const GameView& view) override;
};

} // namespace mafia