#pragma once

#include <vector>
#include <nlohmann/json.hpp>
#include <iosfwd>
#include <map>
#include <memory>
#include <string>
#include <string_view>

#if !defined(PLATFORM_IOS) && !defined(PLATFORM_TVOS) && !defined(PLATFORM_WINDOWS)
#include <sol.hpp>
#endif

#include "scene.hpp"
#include "renderer.hpp"
#include "input.hpp"
#include "cutscene.hpp"
#include "physics.hpp"
#include "file.hpp"
#include "object.hpp"
#include "imguilayer.hpp"
#include "shadowpass.hpp"
#include "scenecapture.hpp"
#include "smaapass.hpp"
#include "gaussianhelper.hpp"
#include "common.hpp"

class GFX;

namespace ui {
    class Screen;
}

class App;
struct Timer;

struct AnimationTarget {
    float current_time = 0.0f;
    float animation_speed_modifier = 1.0f;
    bool looping = false;

    Object target;
    Animation animation;
};

/// An object that contains glue between App and systems such as the Renderer.
class Engine {
public:
    /**
     Constructs an Engine with command line arguments. Can be accessed later from command_line_arguments.
     
    @param argc Numer of arguments. Can be null.
    @param argv Array of strings containing arguments. Can be null.
     */
    Engine(const int argc, char* argv[]);

    /// Command line arguments, can be empty.
    std::vector<std::string_view> command_line_arguments;

    /** Sets the app object.
     @param app The app object to set. Must not be null.
     */
    void set_app(App* app);
    
    /** Gets the current app object
     @return The current app object. Can be null.
     */
    App* get_app() const;

    /** Call to begin the next frame.
     @param delta_time Delta time in seconds.
     */
    void begin_frame(const float delta_time);
    
    /** Call to start updating the current frame.
     @param delta_time Delta time in seconds.
     */
    void update(const float delta_time);
    
    /** Call to begin rendering for a window.
     @param index The index of the window to begin rendering for.
     */
    void render(const int index);

    /// Pause updating.
    void pause();
    
    /// Resume updating.
    void unpause();

    /** Query the current pause state.
     @return Whether or not the engine is currently paused.
     */
    bool is_paused() const;

    /// Request to begin quitting immediately. This is not forced, and can be canceled by the user, platform, or app.
    void quit();
    
    /// Call right before the platform is about to quit the application, and requests the app or other systems to save work.
    void prepare_quit();

    /// Query whether or not the engine is in the process of quitting.
    bool is_quitting() const;

    /** Set the GFX api to use.
     @param gfx The GFX object to use. Must not be null.
     */
    void set_gfx(GFX* gfx);
    
    /** Get the current GFX api.
     @return The current GFX api. Can be null.
     */
    GFX* get_gfx();

    /** Get the input system.
     @return Instance of the input system. Will not be null.
     */
    Input* get_input();
    
    /** Get the renderer for a window.
     @param index Index of the window. Default is 0.
     @return Instance of the renderer. Will not be null.
     */
    Renderer* get_renderer(const int index = 0);
    
    /** Get the physics system.
     @return Instance of the physics system. Will not be null.
     */
    Physics* get_physics();

    /// Creates an empty scene with no path. This will change the current scene.
    void create_empty_scene();
    
    /** Load a scene from disk. This will change the current scene if successful.
     @param path The scene file path.
     @return Returns a instance of the scene is successful, and nullptr on failure.
     */
    Scene* load_scene(const file::Path path);
    
    /** Save the current scene to disk.
     @param path The absolute file path.
     */
    void save_scene(const std::string_view path);

    /** Load a UI screen from disk. This will not change the current screen.
     @param path The screen file path.
     @return Returns a instance of the screen if successful, and nullptr on failure.
     */
    ui::Screen* load_screen(const file::Path path);
    
    /** Set the current screen.
     @param screen The screen object to set as current. Can be null.
     */
    void set_screen(ui::Screen* screen);
    
    /** Gets the current screen.
     @return The current screen. Can be null.
     */
    ui::Screen* get_screen() const;

    /** Load a prefab from disk.
     @param scene The scene to add the prefab to.
     @param path The prefab file path.
     @param override_name If not empty, the root object's new name. Defaulted to a empty string.
     */
    Object add_prefab(Scene& scene, const file::Path path, const std::string_view override_name = "");
    
    /** Save a tree of objects as a prefab to disk.
     @param root The parent object to save as a prefab.
     @param path The absolue file path.
     */
    void save_prefab(const Object root, const std::string_view path);

    /** Deserializes a animation channel from JSON.
     @param j The animation channel json to deserialize.
     @return An animation channel.
     */
    AnimationChannel load_animation(nlohmann::json j);
    
    /** Load an animation from disk.
     @param path The animation file path.
     @return An animation.
     */
    Animation load_animation(const file::Path path);

    /** Load a cutscene from disk. This changes the current cutscene.
     @param path The cutscene file path.
     */
    void load_cutscene(const file::Path path);
    
    /** Saves the current cutscene to disk.
     @param path The absolute file path.
     */
    void save_cutscene(const std::string_view path);

    /** Adds the window for engine management.
     @param native_handle The platform's native handle to it's window equivalent.
     @param identifier The identifier of the new window.
     @param extent The extent of the window.
     */
    void add_window(void* native_handle, const int identifier, const prism::Extent extent);
    
    /** Removes the window from engine management. Should be called before the window is actually closed.
     @param identifier The identifier of the window to remove.
     */
    void remove_window(const int identifier);
    
    /** Called when the window has changed size.
     @param identifier The window that has been resized.
     @param extent The new extent of the window.
     */
    void resize(const int identifier, const prism::Extent extent);

    /** Called when a key has been pressed.
     @param keyCode A platform-specific key code.
     @note Use platform::get_keycode to get a InputButton equivalent if needed.
     @note This function is only intended for debug purposes and all production code should be using the Input system instead.
     */
    void process_key_down(const unsigned int keyCode);
    
    /** Called when a key has been released.
     @param keyCode A platform-specific key code.
     @note Use platform::get_keycode to get a InputButton equivalent if needed.
     @note This function is only intended for debug purposes and all production code should be using the Input system instead.
     */
    void process_key_up(const unsigned int keyCode);
    
    /** Called when a mouse button has been clicked..
     @param button The mouse button.
     @param offset The mouse position relative to the window where the click occured.
     @note This function is only intended for debug purposes and all production code should be using the Input system instead.
     */
    void process_mouse_down(const int button, const prism::Offset offset);

    /** Pushes a UI event for the current screen. Does nothing if there is no screen set.
     @param name The name of the event.
     @param data Data for the event. Defaulted to an empty string.
     */
    void push_event(const std::string_view name, const std::string_view data = "");

    /** Load a localization file from disk. This will change the current localization.
     @param path The localization file path.
     */
    void load_localization(const std::string_view path);
    
    /** Queries whether or not the current localization loaded has a key.
     @param id The key to query.
     @return Whether or not the locale has the key specified.
     @note Having no localization loaded will always return false.
     */
    bool has_localization(const std::string_view id) const;
    
    /** Localizes a string.
     @param id The locale key to use.
     @return A localized string if the key is found, or an empty string if not found.
     @note Having no localization loaded will always return a empty string.
     */
    std::string localize(const std::string id);

    /** Adds a timer to the list of timers.
     @param timer The timer to add.
     @note The timer instance is passed by reference. Use this to keep track of your timers without having to query it's state back.
     */
    void add_timer(Timer& timer);

    /** Gets the current scene.
     @return The current scene if set, or nullptr if there isn't one.
     */
    Scene* get_scene();
    
    /** Get a scene by path. This does not change the current scene.
     @param name The path to the scene file.
     @return Returns a scene if it was previously loaded and found, or nullptr on failure.
     */
    Scene* get_scene(const std::string_view name);
    
    /** Set the current scene.
     @param scene The scene to set. Can be null.
     */
    void set_current_scene(Scene* scene);
    
    /** Get the current scene's path.
     @return If a scene is loaded, the path to the scene. Can be an empty string if there is no scene loaded, or if it's an empty scene.
     */
    std::string_view get_scene_path() const;
    
    /** Updates the Transform hierarchy for a scene.
     @param scene The scene to update.
     */
    void update_scene(Scene& scene);

    /// The current cutscene.
    std::unique_ptr<Cutscene> cutscene;
    
    /// The current time in seconds for cutscene playback.
    float current_cutscene_time = 0.0f;
    
    /// The cutscene playback state.
    bool play_cutscene = false;

    /** Start playback of an animation.
     @param animation The animation to play.
     @param object The animation's target.
     @param looping Whether or not the animation should loop or be discarded when finished. Default is false.
     */
    void play_animation(const Animation animation, const Object target, const bool looping = false);
    
    /** Sets the animation speed of an object.
     @param target The object you want to change the animation speed of.
     @param modifier The speed to play the object's animations at. A modifier of 2.0 would be 2x the speed, and 0.5 would be 1/2x the speed.
     */
    void set_animation_speed_modifier(const Object target, const float modifier);
    
    /** Stops all animation for an object.
     @param target The object you want the animations to stop for.
     */
    void stop_animation(const Object target);

    /// If there is a render context available. If this is false, avoid doing any GFX or Renderer work as it could crash or cause undefined behavior.
    bool render_ready = false;
    
    /// If physics should upate. This is a control indepentent of the pause state.
    bool update_physics = true;

#if !defined(PLATFORM_IOS) && !defined(PLATFORM_TVOS) && !defined(PLATFORM_WINDOWS)
    sol::state lua;
#endif
    
#if defined(PLATFORM_TVOS) || defined(PLATFORM_IOS)
    bool debug_enabled = true;
#else
    bool debug_enabled = false;
#endif

private:
    void create_lua_interface();
    void setup_scene(Scene& scene);
    
    void on_remove(Object object);
    
    bool _paused = false;

    ui::Screen* _current_screen = nullptr;

    Scene* _current_scene = nullptr;
    std::vector<std::unique_ptr<Scene>> _scenes;
    std::map<std::string, Scene*> _path_to_scene;

    struct Window {
        int identifier = -1;
        prism::Extent extent;
        bool quitRequested = false;

        std::unique_ptr<Renderer> renderer = nullptr;
    };

    std::vector<Window> _windows;

    Window* get_window(const int identifier) {
        for(auto& window : _windows) {
            if(window.identifier == identifier)
                return &window;
        }
        
        return nullptr;
    }

    void calculate_bone(Mesh& mesh, const Mesh::Part& part, Bone& bone, const Bone* parent_bone = nullptr);
    void calculate_object(Scene& scene, Object object, const Object parent_object = NullObject);

    Shot* get_shot(const float time);
    void update_animation(const Animation& anim, const float time);
    void update_cutscene(const float time);

    void update_animation_channel(Scene& scene, const AnimationChannel& channel, const float time);

    App* _app = nullptr;
    GFX* _gfx = nullptr;

    std::unique_ptr<Input> _input = nullptr;
    std::unique_ptr<Physics> _physics = nullptr;

    std::vector<Timer*> _timers, _timersToRemove;

    std::map<std::string, std::string> _strings;

    std::vector<AnimationTarget> _animation_targets;

    std::unique_ptr<ImGuiLayer> _imgui = nullptr;

    const InputButton debug_button = InputButton::Q;
};

inline bool operator==(const AnimationTarget& a1, const AnimationTarget& a2) {
    return a1.current_time == a2.current_time;
}

inline Engine* engine = nullptr;
