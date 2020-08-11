#include "log.hpp"

#include <iostream>
#include <chrono>
#include <iomanip>
#include "string_utils.hpp"
#include "utility.hpp"

void console::process_message(const Level level, const System system, const std::string_view message) {
    auto now = std::chrono::system_clock::now();
    std::time_t t_c = std::chrono::system_clock::to_time_t(now);
    
    std::string date;
    date.resize(30);
    
    std::strftime(&date[0], date.size(), "%Y-%m-%d %H:%M:%S", std::localtime(&t_c));

    std::string s = utility::format("{} {} {}: {}", date, utility::enum_to_string(system), utility::enum_to_string(level), message);
    
    std::cout << s << '\n';
    
    stored_output.push_back(s);
}
