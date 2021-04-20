#pragma once

#include <string_view>
#include <functional>
#include <vector>
#include <variant>
#include <string>

namespace prism::console {
    enum class argument_type {
        String,
        Integer,
        Boolean
    };

    struct console_argument {
        explicit console_argument(bool data) : data(data) {}

        explicit console_argument(int data) : data(data) {}

        [[nodiscard]] argument_type query_type() const {
            if (std::holds_alternative<std::string>(data))
                return argument_type::String;
            else if (std::holds_alternative<int>(data))
                return argument_type::Integer;
            else if (std::holds_alternative<bool>(data))
                return argument_type::Boolean;
        }

        std::variant<std::string, int, bool> data;
    };

    using arguments = std::vector<console_argument>;

    using function_ptr = std::function<void(console::arguments)>;

    struct argument_format {
        explicit argument_format(const int num_arguments) : num_arguments(num_arguments) {}

        int num_arguments = 0;
        std::vector<argument_type> argument_types;
    };

    void
    register_command(std::string_view name, argument_format expected_format, function_ptr function);

    void invoke_command(std::string_view name, arguments arguments);

    void parse_and_invoke_command(std::string_view command);

    void register_variable(std::string_view name, bool &variable);
}