#pragma once

#include <string_view>
#include <functional>
#include <vector>
#include <variant>
#include <string>

namespace console {
    enum class ArgType {
        String,
        Integer,
        Boolean
    };
    
    struct ConsoleArgument {
        ConsoleArgument(bool data) : data(data) {}
        ConsoleArgument(int data) : data(data) {}
        
        ArgType query_type() const {
            if(std::holds_alternative<std::string>(data))
                return ArgType::String;
            else if(std::holds_alternative<int>(data))
                return ArgType::Integer;
            else if(std::holds_alternative<bool>(data))
                return ArgType::Boolean;
        }
        
        std::variant<std::string, int, bool> data;
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
    void parse_and_invoke_command(const std::string_view command);

    void register_variable(const std::string_view name, bool& variable);
}
