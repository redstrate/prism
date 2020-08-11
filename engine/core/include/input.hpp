#pragma once

#include <vector>
#include <map>

#include "platform.hpp"

enum class Axis {
	MouseX,
	MouseY,
    
    ScrollX,
    ScrollY,
    
    LeftStickX,
    LeftStickY,
    
    RightStickX,
    RightStickY
};

struct InputBindingData {
    std::string name;
    std::map<InputButton, float> buttons;
    
    std::vector<Axis> axises;
    
    float value = 0.0f;

    bool repeat = false;
    InputButton last_button = InputButton::Invalid;
};

/** Input system designed for handling events in a cross platform manner and across different input types.
 */
class Input {
public:
    /// Updates input bindings and their values.
	void update();

    /** Add a new binding.
     @param name The binding name.
     */
	void add_binding(const std::string& name);

    /** Ties a button-type input to the binding.
     @param name The binding name.
     @param button The button to associate with.
     @param value The resulting input value when the button is pressed. Default is 1.0.
     */
	void add_binding_button(const std::string& name, InputButton button, float value = 1.0f);
    
    /** Ties an axis-type input to the button.
     @param name The binding name.
     @param button The axis to associate with.
     */
	void add_binding_axis(const std::string& name, Axis axis);

    /** Gets the input value of a binding.
     @param name The binding name.
     @return The input value associated with that binding, or 0.0 if none is found.
     @note If this binding is tied to a button-type input, assume that 0.0 means not pressed.
     @note If this binding is tied to an axis-type input, assume that 0.0 means the stick is centered.
     */
	float get_value(const std::string& name);
    
    /** Gets whether or not the binding is pressed.
     @param name The binding name.
     @param repeating If true, return a correct value regardless if it was identical for multiple frames. If false, any continued input past the first frame of being pressed is ignored and the input is considered released.
     @return If the binding is found, return true if the binding value is 1.0. If no binding is found or if the binding has continued pressing and repeating is turned off, returns false.
     */
    bool is_pressed(const std::string& name, bool repeating = false);
    
    /** Queries if the binding is repeating it's input.
     @param name The binding name.
     @return If the binding is found, returns true if the binding's values have been identical for multiple frames. Returns false if the binding is not found.
     */
	bool is_repeating(const std::string& name);
    
    /// Returns all of the bindings registered with this Input system.
    std::vector<InputBindingData> get_bindings() const;
    
private:
    std::vector<InputBindingData> _input_bindings;
    std::tuple<int, int> _last_cursor_position;
};
