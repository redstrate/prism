#include "console.hpp"

#include <unordered_map>
#include <string>
#include <optional>

#include "log.hpp"
#include "string_utils.hpp"

struct RegisteredCommand {
    RegisteredCommand(const console::ArgumentFormat format, const console::FunctionPtr function) : expected_format(format), function(function) {}
    
    console::ArgumentFormat expected_format;
    std::function<void(console::Arguments)> function;
};

static std::unordered_map<std::string, RegisteredCommand> registered_commands;

struct RegisteredVariable {
    RegisteredVariable(bool& data) : type(console::ArgType::Boolean), data(&data) {}
        
    console::ArgType type;
    
    std::variant<bool*> data;
};

static std::unordered_map<std::string, RegisteredVariable> registered_variables;

void console::register_command(const std::string_view name, const ArgumentFormat expected_format, const console::FunctionPtr function) {
    registered_commands.try_emplace(name.data(), RegisteredCommand(expected_format, function));
}

void console::invoke_command(const std::string_view name, const Arguments arguments) {
    for(auto& [command_name, command_data] : registered_commands) {
        if(command_name == name) {
            bool invalid_format = false;
            
            if(arguments.arguments.size() != command_data.expected_format.num_arguments)
                invalid_format = true;
            
            if(invalid_format) {
                console::info(System::Core, "Invalid command format!");
            } else {
                command_data.function(console::Arguments());
            }
            
            return;
        }
    }
    
    for(auto& [variable_name, variable_data] : registered_variables) {
        if(variable_name == name) {
            bool invalid_format = false;

            if(arguments.arguments.empty())
                invalid_format = true;
            
            if(arguments.arguments[0].query_type() != variable_data.type)
                invalid_format = true;
            
            if(invalid_format) {
                console::info(System::Core, "Wrong or empty variable type!");
            } else {
                auto argument = arguments.arguments[0];
                switch(argument.query_type()) {
                    case console::ArgType::Boolean:
                        *std::get<bool*>(variable_data.data) = std::get<bool>(argument.data);
                        break;
                }
            }
            
            return;
        }
    }
    
    console::info(System::Core, "{} is not the name of a valid command or variable!", name.data());
}

void console::parse_and_invoke_command(const std::string_view command) {
    const auto tokens = tokenize(command, " ");
    if(tokens.empty())
        return;
    
    const auto try_parse_argument = [](std::string_view arg) -> std::optional<ConsoleArgument> {
        if(arg == "true") {
            return ConsoleArgument(true);
        } else if(arg == "false") {
            return ConsoleArgument(false);
        } else if(is_numeric(arg)) {
            return ConsoleArgument(std::stoi(arg.data()));
        }
        
        return std::nullopt;
    };
    
    std::vector<ConsoleArgument> arguments;
    
    for(int i = 1; i < tokens.size(); i++) {
        if(auto arg = try_parse_argument(tokens[i]); arg)
            arguments.push_back(*arg);
    }
    
    console::Arguments args;
    args.arguments = arguments;
    
    console::invoke_command(tokens[0], args);
}

void console::register_variable(const std::string_view name, bool& variable) {
    registered_variables.try_emplace(name.data(), RegisteredVariable(variable));
}
