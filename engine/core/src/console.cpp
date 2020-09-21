#include "console.hpp"

#include <unordered_map>
#include <string>

#include "log.hpp"

struct RegisteredCommand {
    RegisteredCommand(const console::ArgumentFormat format, const console::FunctionPtr function) : expected_format(format), function(function) {}
    
    console::ArgumentFormat expected_format;
    std::function<void(console::Arguments)> function;
};

static std::unordered_map<std::string, RegisteredCommand> registered_commands;

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
    
    console::info(System::Core, "{} is not a valid command!", name.data());
}
