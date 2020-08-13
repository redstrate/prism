#pragma once

#include <string_view>
#include <string>
#include <vector>

enum class System {
    None,
    Core,
    Renderer,
    Game,
    File,
    GFX
};

enum class Level {
    Info,
    Warning,
    Error,
    Debug
};

namespace console {
    inline void internal_format(std::string& msg, const std::string& arg) {
        auto pos = msg.find_first_of("{}");
        msg.replace(pos, 2, arg);
    }

    inline void internal_format(std::string& msg, const char*& arg) {
        auto pos = msg.find_first_of("{}");
        msg.replace(pos, 2, arg);
    }
    
    void process_message(const Level level, const System system, const std::string_view message);
    
    template<typename... Args>
    void internal_print(const Level level, const System system, const std::string_view format, Args&&... args) {
        auto msg = std::string(format);
        
        ((internal_format(msg, args)), ...);
        
        process_message(level, system, msg);
    }
    
    template<typename... Args>
    void info(const System system, const std::string_view format, Args&&... args) {
        internal_print(Level::Info, system, format, args...);
    }
    
    template<typename... Args>
    void warning(const System system, const std::string_view format, Args&&... args) {
        internal_print(Level::Warning, system, format, args...);
    }
    
    template<typename... Args>
    void error(const System system, const std::string_view format, Args&&... args) {
        internal_print(Level::Error, system, format, args...);
    }
    
    template<typename... Args>
    void debug(const System system, const std::string_view format, Args&&... args) {
        internal_print(Level::Debug, system, format, args...);
    }

    inline std::vector<std::string> stored_output;
}
