#include "mafia/Logger.hpp"

#include <filesystem>
#include <fstream>
#include <string>

namespace mafia {

Logger::Logger(const std::string& base_directory) {
    std::filesystem::create_directories(base_directory);

    int game_number = 1;

    while (true) {
        std::filesystem::path game_directory =
            std::filesystem::path(base_directory) /
            ("game_" + std::to_string(game_number));

        if (std::filesystem::create_directory(game_directory)) {
            directory_ = game_directory.string();
            break;
        }

        ++game_number;
    }
}

void Logger::log_round(int round, const std::string& text) {
    std::filesystem::path file_path =
        std::filesystem::path(directory_) /
        ("round_" + std::to_string(round) + ".txt");

    std::ofstream file(file_path, std::ios::app);

    file << text << '\n';
}

void Logger::log_summary(const std::string& text) {
    std::filesystem::path file_path =
        std::filesystem::path(directory_) / "summary.txt";

    std::ofstream file(file_path);

    file << text;
}

} // namespace mafia