#include "mafia/Config.hpp"

#include <algorithm>
#include <fstream>
#include <stdexcept>
#include <string>

namespace mafia {

namespace {

std::string trim(std::string text) {
    const std::size_t first = text.find_first_not_of(" \t\r\n");

    if (first == std::string::npos) {
        return "";
    }

    const std::size_t last = text.find_last_not_of(" \t\r\n");

    return text.substr(first, last - first + 1);
}

Role role_from_string(const std::string& name) {
    if (name == "Ninja") {
        return Role::Ninja;
    }

    if (name == "Bull") {
        return Role::Bull;
    }

    if (name == "Elder") {
        return Role::Elder;
    }

    throw std::runtime_error("Unknown role in config: " + name);
}

} // namespace

std::vector<Role> load_extra_roles(const std::string& filename) {
    std::ifstream file(filename);

    if (!file) {
        throw std::runtime_error("Cannot open config file: " + filename);
    }

    std::vector<Role> roles;
    std::string line;
    bool reading_extra_roles = false;

    while (std::getline(file, line)) {
        const std::size_t comment = line.find('#');

        if (comment != std::string::npos) {
            line.erase(comment);
        }

        line = trim(line);

        if (line.empty()) {
            continue;
        }

        if (line == "extra_roles:") {
            reading_extra_roles = true;
            continue;
        }

        if (!reading_extra_roles) {
            continue;
        }

        if (line.front() != '-') {
            break;
        }

        const std::string role_name = trim(line.substr(1));
        const Role role = role_from_string(role_name);

        if (std::find(roles.begin(), roles.end(), role) == roles.end()) {
            roles.push_back(role);
        }
    }

    return roles;
}

} // namespace mafia