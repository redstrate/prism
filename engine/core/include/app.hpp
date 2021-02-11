#pragma once

class Engine;
class GFXCommandBuffer;

/// The base class for any Prism application.
class App {
public:
    /// Called when a render context is available.
	virtual void initialize_render() {}

    /// Called when the engine is about to quit. Should be overriden if an app needs to save data before qutting.
    virtual void prepare_quit() {}
    
    /// Called to check whether or not an app should intervene quitting.
	virtual bool should_quit() {
        return true;
    }

    /// Called when a engine starts a new frame. Typically used for inserting new imgui windows.
	virtual void begin_frame() {}
    
    /** Called during the engine's update cycle.
     @param delta_time Delta time in milliseconds.
     */
    virtual void update([[maybe_unused]] const float delta_time) {}

    virtual void render([[maybe_unused]] GFXCommandBuffer* command_buffer) {}
};

/// This is an app's equivalent main(). You can check command line arguments through Engine::comand_line_arguments.
void app_main(Engine* engine);
