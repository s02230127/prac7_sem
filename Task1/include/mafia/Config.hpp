#pragma once

#include <string>
#include <vector>

#include "mafia/Player.hpp"

namespace mafia {

std::vector<Role> load_extra_roles(const std::string& filename);

} // namespace mafia