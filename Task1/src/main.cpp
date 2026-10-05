#include "mafia/Game.hpp"

#include <iostream>
#include <stdexcept>
#include <string>

int main(int argc, char* argv[]) {
    mafia::GameOptions options;

    try {
        for (int i = 1; i < argc; ++i) {
            std::string argument = argv[i];

            if (argument == "--players") {
                if (i + 1 >= argc) {
                    throw std::invalid_argument(
                        "--players requires a number"
                    );
                }

                options.player_count = std::stoi(argv[++i]);
            }
            else if (argument == "--interactive") {
                options.interactive = true;
            }
            else if (argument == "--open-announcements") {
                options.open_announcements = true;
            }
            else if (argument == "--full-log") {
                options.full_log = true;
            }
            else if (argument == "--config") {
                if (i + 1 >= argc) {
                    throw std::invalid_argument(
                        "--config requires a YAML file path"
                    );
                }

                options.config_file = argv[++i];
            }
            else if (argument == "--help") {
                std::cout
                    << "Usage:\n"
                    << "  mafia [--players N] [options]\n\n"
                    << "Options:\n"
                    << "  --players N              number of players (default: 10, N > 4)\n"
                    << "  --interactive            player 1 is controlled by user\n"
                    << "  --open-announcements     reveal full roles\n"
                    << "  --full-log               show detailed log\n"
                    << "  --config FILE            YAML role config (default: config.yaml)\n"
                    << "  --help                    show this message\n";

                return 0;
            }
            else {
                throw std::invalid_argument(
                    "Unknown argument: " + argument
                );
            }
        }

        mafia::Game game(options);
        game.run();
    }
    catch (const std::exception& error) {
        std::cerr << "Error: " << error.what() << '\n';
        return 1;
    }

    return 0;
}
