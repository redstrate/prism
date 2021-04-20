#include "console.hpp"

#include <unordered_map>
#include <string>
#include <optional>

#include "log.hpp"
#include "string_utils.hpp"

struct registered_command {
    registered_command(const prism::console::argument_format format, const prism::console::function_ptr function) : expected_format(format), function(function) {}

    prism::console::argument_format expected_format;
    std::function<void(prism::console::arguments)> function;
};

static std::unordered_map<std::string, registered_command> registered_commands;

struct RegisteredVariable {
    RegisteredVariable(bool& data) : type(prism::console::argument_type::Boolean), data(&data) {}

    prism::console::argument_type type;
    
    std::variant<bool*> data;
};

static std::unordered_map<std::string, RegisteredVariable> registered_variables;

void prism::console::register_command(const std::string_view name, const prism::console::argument_format expected_format, const prism::console::function_ptr function) {
    registered_commands.try_emplace(name.data(), registered_command(expected_format, function));
}

void prism::console::invoke_command(const std::string_view name, const prism::console::arguments arguments) {
    for(auto& [command_name, command_data] : registered_commands) {
        if(command_name == name) {
            bool invalid_format = false;
            
            if(arguments.size() != command_data.expected_format.num_arguments)
                invalid_format = true;
            
            if(invalid_format) {
                prism::log::info(System::Core, "Invalid command format!");
            } else {
                command_data.function(console::arguments());
            }
            
            return;
        }
    }
    
    for(auto& [variable_name, variable_data] : registered_variables) {
        if(variable_name == name) {
            bool invalid_format = false;

            if(arguments.empty())
                invalid_format = true;
            
            if(arguments[0].query_type() != variable_data.type)
                invalid_format = true;
            
            if(invalid_format) {
                prism::log::info(System::Core, "Wrong or empty variable type!");
            } else {
                auto argument = arguments[0];
                switch(argument.query_type()) {
                    case console::argument_type::Boolean:
                        *std::get<bool*>(variable_data.data) = std::get<bool>(argument.data);
                        break;
                }
            }
            
            return;
        }
    }
    
    prism::log::info(System::Core, "{} is not the name of a valid command or variable!", name.data());
}

void prism::console::parse_and_invoke_command(const std::string_view command) {
    const auto tokens = tokenize(command, " ");
    if(tokens.empty())
        return;
    
    const auto try_parse_argument = [](std::string_view arg) -> std::optional<console_argument> {
        if(arg == "true") {
            return console_argument(true);
        } else if(arg == "false") {
            return console_argument(false);
        } else if(is_numeric(arg)) {
            return console_argument(std::stoi(arg.data()));
        }
        
        return std::nullopt;
    };
    
    std::vector<console_argument> arguments;
    
    for(int i = 1; i < tokens.size(); i++) {
        if(auto arg = try_parse_argument(tokens[i]); arg)
            arguments.push_back(*arg);
    }

    console::invoke_command(tokens[0], arguments);
}

void prism::console::register_variable(const std::string_view name, bool& variable) {
    registered_variables.try_emplace(name.data(), RegisteredVariable(variable));
}
