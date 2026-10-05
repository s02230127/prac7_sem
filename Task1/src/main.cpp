#include "mafia/Game.hpp"

int main() {
    mafia::GameOptions options;
    options.player_count = 6;
    options.full_log = true;

    mafia::Game game(options);
    game.run();

    return 0;
}