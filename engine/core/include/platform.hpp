#pragma once

#include <functional>
#include <string>
#include <string_view>

#include "common.hpp"

/// Requestable window flags, which may or may not be respected by the platform.
enum class WindowFlags {
    None,
    Resizable,
    Borderless
};

/// Represents a button. This includes keyboard, gamepad and mouse buttons.
enum class InputButton {
    Invalid,
    C,
    V,
    X,
    Y,
    Z,
    Backspace,
    Enter,
    W,
    A,
    S,
    D,
    Q,
    Shift,
    Alt,
    Super,
    Escape,
    Tab,
    Ctrl,
    Space,
    LeftArrow,
    RightArrow,

    // gamepad inputs
    ButtonA,
    ButtonB,
    ButtonX,
    ButtonY,

    DPadUp,
    DPadDown,
    DPadLeft,
    DPadRight,

    // mouse inputs
    MouseLeft,
    MouseRight
};

enum class PlatformFeature {
    Windowing
};

namespace platform {
    /// Returns a human readable platform name, e.g. Linux.
    const char* get_name();

    /// Queries whether or not the platform supports a certain feature.
    bool supports_feature(const PlatformFeature feature);

    /** Opens a new window.
     @param title The title of the window.
     @param rect The requested size and position of the window.
     @param flags The requested window flags.
     @note Depending on the platform, some of these parameters might be unused. The best practice is to always assume that none of them may be used.
     @note On platforms that do not support the Windowing feature, calling open_window more than once is not supported. In this case, the same identifier is returned.
     @return A valid window identifier.
     */
    int open_window(const std::string_view title, const Rectangle rect, const WindowFlags flags);

    /** Closes a window.
     @param index The window to close.
     */
    void close_window(const int index);

    /// Forces the platform to quit the application. This is not related to Engine::quit().
    void force_quit();

    /// Gets the content scale for the window. 1.0 would be 1x scale, 2.0 would be 2x scale, etc.
    float get_window_dpi(const int index);

    /// Gets the content scale for the monitor. 1.0 would be 1x scale, 2.0 would be 2x scale, etc.
    float get_monitor_dpi();
    
    /// Get the monitor resolution.
    Rectangle get_monitor_resolution();

    /// Get the monitor work area. For example on macOS this may exclude the areas of the menu bar and dock.
    Rectangle get_monitor_work_area();

    /// Get the window position.
    Offset get_window_position(const int index);

    /// Get the window size, note that in hidpi scenarios this is the non-scaled resolution.
    Extent get_window_size(const int index);

    /// Get the window's drawable size. Always use this instead of manually multiplying the window size by the content scale.
    Extent get_window_drawable_size(const int index);

    /// Query whether or not the window is focused.
    bool is_window_focused(const int index);

    /// If possible, try to manually focus the window.
    void set_window_focused(const int index);

    /// Sets the window position to the offset provided.
    void set_window_position(const int index, const Offset offset);

    /// Sets the window to the specified size. The platform will handle the subsequent resize events.
    void set_window_size(const int index, const Extent extent);

    /// Sets the window title.
    void set_window_title(const int index, const std::string_view title);

    /// Queries whether or not the button is currently pressed.
    bool get_key_down(const InputButton key);

    /// If available for the InputButton, returns the platform-specific keycode.
    int get_keycode(const InputButton key);

    /// Returns the current moue cursor position, relative to the window.
    Offset get_cursor_position();

    /// Returns the current moue cursor position, relative to the monitor.
    Offset get_screen_cursor_position();

    /// Queries whether or not the mouse button requested is pressed or not.
    bool get_mouse_button_down(const int button);

    /// Returns the current mouse wheel delta on both axes.
    std::tuple<float, float> get_wheel_delta();

    /// Returns the current right stick axes values from 0->1
    std::tuple<float, float> get_right_stick_position();

    /// Returns the current left stick axes values  from 0->1
    std::tuple<float, float> get_left_stick_position();

    /// On platforms that support moue capture, this will lock the mouse cursor to the window and hide it.
    void capture_mouse(const bool capture);

    /**Opens a file dialog to select a file. This will not block.
     @param existing Whether or not to limit to existing files.
     @param returnFunction The callback function when a file is selected or the dialog is cancelled. An empy string is returned when cancelled.
     @param openDirectory Whether or not to allow selecting directories as well.
     */
    void open_dialog(const bool existing, std::function<void(std::string)> returnFunction, bool openDirectory = false);

    /**Opens a file dialog to select a save location for a file. This will not block.
     @param returnFunction The callback function when a file is selected or the dialog is cancelled. An empy string is returned when cancelled.
     */
    void save_dialog(std::function<void(std::string)> returnFunction);

    /**Translates a virtual keycode to it's character equivalent.
     @note Example: translateKey(0x01) = 'a'; 0x01 in this example is a platform and language specific keycode for a key named 'a'.
     @return A char pointer to the string if translated correctly.
     @note Manually freeing the string returned is not needed.
     */
    char* translate_keycode(const unsigned int keycode);

    /// Mute standard output
    void mute_output();

    /// Unmute standard output
    void unmute_output();
}
