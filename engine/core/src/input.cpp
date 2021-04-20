#include "input.hpp"

#include <string>

#include "engine.hpp"
#include "log.hpp"

using prism::input_system;

bool is_in_range(int value, int cond, int range) {
    int low = cond - range;
    int high = cond + range;

    return value >= low && value <= high;
}

void input_system::update() {
	const auto& [x, y] = platform::get_cursor_position();
	auto& [oldX, oldY] = _last_cursor_position;
    
    const auto [width, height] = platform::get_window_size(0);

	float xDelta = (x - oldX) / (float)width;
	float yDelta = (y - oldY) / (float)height;

	if(is_in_range(x, width / 2, 3) && is_in_range(y, height / 2, 3)) {
		xDelta = 0.0f;
		yDelta = 0.0f;
    }

    _last_cursor_position = { x, y };

	for (auto& binding : _input_bindings) {
		binding.value = 0.0f;

		for (auto& [key, value] : binding.buttons) {
            bool is_pressed = false;
            switch(key) {
                case InputButton::MouseLeft:
                    is_pressed = platform::get_mouse_button_down(0);
                    break;
                case InputButton::MouseRight:
                    is_pressed = platform::get_mouse_button_down(1);
                    break;
                default:
                    is_pressed = platform::get_key_down(key);
                    break;
            }

			if (is_pressed) {
				binding.value = value;

				if (binding.last_button == key) {
					binding.repeat = true;
				}
				else {
					binding.repeat = false;
				}

				binding.last_button = key;
			}
		}

		if (binding.value == 0.0f) {
			binding.last_button = InputButton::Invalid;
			binding.repeat = false;
		}

        for(auto& axis : binding.axises) {
            auto [lx, ly] = platform::get_left_stick_position();
            auto [rx, ry] = platform::get_right_stick_position();
            auto [sx, sy] = platform::get_wheel_delta();

            float nextValue = 0.0f;

            switch (axis) {
            case axis::MouseX:
                nextValue = xDelta;
                break;
            case axis::MouseY:
                nextValue = yDelta;
                break;
            case axis::ScrollX:
                nextValue = sx;
                break;
            case axis::ScrollY:
                nextValue = sy;
                break;
            case axis::LeftStickX:
                nextValue = lx;
                break;
            case axis::LeftStickY:
                nextValue = ly;
                break;
            case axis::RightStickX:
                nextValue = rx;
                break;
            case axis::RightStickY:
                nextValue = ry;
                break;
            }

            if(nextValue != 0.0f)
                binding.value = nextValue;
        }
	}
}

void input_system::add_binding(const std::string& name) {
	input_binding data;
	data.name = name;

	_input_bindings.push_back(data);
}

void input_system::add_binding_button(const std::string& name, InputButton key, float value) {
	for (auto& binding : _input_bindings) {
		if (binding.name == name)
			binding.buttons[key] = value;
	}
}

void input_system::add_binding_axis(const std::string& name, axis axis) {
	for (auto& binding : _input_bindings) {
		if (binding.name == name) {
            binding.axises.push_back(axis);
		}
	}
}

float input_system::get_value(const std::string& name) {
	for (auto& binding : _input_bindings) {
		if (binding.name == name) {
			return binding.value;
		}
	}

	return 0.0f;
}

bool input_system::is_pressed(const std::string &name, bool repeating) {
    return get_value(name) == 1.0 && (repeating ? true : !is_repeating(name));
}

bool input_system::is_repeating(const std::string& name) {
	for (auto& binding : _input_bindings) {
		if (binding.name == name) {
			return binding.repeat;
		}
	}

	return false;
}

std::vector<prism::input_binding> input_system::get_bindings() const {
    return _input_bindings;
}
