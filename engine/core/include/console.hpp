#pragma once

#include <string_view>
#include <functional>
#include <vector>
#include <variant>

namespace console {
    using ConsoleArgument = std::variant<std::string, int, bool>;
    
    enum class ArgType {
        String,
        Integer,
        Boolean
    };
    
    struct Arguments {
        std::vector<ConsoleArgument> arguments;
    };
    
    using FunctionPtr = std::function<void(console::Arguments)>;

    struct ArgumentFormat {
        ArgumentFormat(const int num_arguments) : num_arguments(num_arguments) {}
        
        int num_arguments = 0;
        std::vector<ArgType> argument_types;
    };
    
    void register_command(const std::string_view name, const ArgumentFormat expected_format, const FunctionPtr function);
    
    void invoke_command(const std::string_view name, const Arguments arguments);
}
