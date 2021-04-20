#pragma once

#include <string>
#include <filesystem>

namespace file {
    using Path = std::filesystem::path;
}

namespace prism::log {
    inline void internal_format(std::string& msg, const file::Path& arg) {
        auto pos = msg.find_first_of("{}");
        msg.replace(pos, 2, arg.string());
    }
}