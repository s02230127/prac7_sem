#pragma once

#include <concepts>
#include <type_traits>
#include <string>

#include "mafia/Player.hpp"
#include "mafia/GameView.hpp"
#include "mafia/SharedPtr.hpp"

namespace mafia {

template <typename T>
concept PlayerRole =
    std::derived_from<T, Player> &&
    std::constructible_from<T, int> &&
    !std::is_abstract_v<T> &&
    requires(T& player, const GameView& view) {
        { player.discuss(view) } -> std::convertible_to<std::string>;
        { player.vote(view) } -> std::same_as<int>;
        { player.act(view) } -> std::same_as<Action>;
    };

template <PlayerRole T>
SharedPtr<Player> make_role(int id) {
    return SharedPtr<Player>(new T(id));
}

} // namespace mafia