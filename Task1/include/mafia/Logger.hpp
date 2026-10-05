#pragma once

#include <string>

namespace mafia {

class Logger {
public:
    explicit Logger(const std::string& base_directory = "logs");

    void log_round(int round, const std::string& text);
    void log_summary(const std::string& text);

private:
    std::string directory_;
};

} // namespace mafia