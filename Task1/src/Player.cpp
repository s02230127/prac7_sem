#include "mafia/Player.hpp"

namespace mafia {

Player::Player(int id, Role role) noexcept
    : id_(id), role_(role) {
}

int Player::id() const noexcept {
    return id_;
}

Role Player::role() const noexcept {
    return role_;
}

}