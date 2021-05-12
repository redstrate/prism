#pragma once

#include <string>
#include <filesystem>

#include "path.hpp"

namespace prism::log {
    inline void internal_format(std::string& msg, const prism::path& arg) {
        auto pos = msg.find_first_of("{}");
        msg.replace(pos, 2, arg.string());
    }
}