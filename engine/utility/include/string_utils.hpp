#pragma once

#include <string>
#include <string_view>
#include <vector>

std::string remove_substring(const std::string_view string, const std::string_view substring);
std::string replace_substring(const std::string_view string, const std::string_view substring, const std::string_view replacement);

bool string_contains(const std::string_view a, const std::string_view b);
bool string_starts_with(const std::string_view haystack, const std::string_view needle);
bool is_numeric(const std::string_view string);

std::vector<std::string> tokenize(const std::string_view string, const std::string_view& delimiters = ",");

namespace utility {
    template<class Arg>
    inline void format_internal_format(std::string& msg, const Arg& arg) {
        auto pos = msg.find_first_of("{}");
        msg.replace(pos, 2, arg);
    }

    template<class... Args>
    std::string format(const std::string_view format, Args&&... args) {
        auto msg = std::string(format);
        
        ((format_internal_format<Args>(msg, args)), ...);
        
        return msg;
    }
}
