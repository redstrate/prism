#include "engine.hpp"

#include <nlohmann/json.hpp>
#include <imgui.h>

#include "scene.hpp"
#include "console.hpp"
#include "log.hpp"
#include "asset.hpp"
#include "json_conversions.hpp"
#include "app.hpp"
#include "assertions.hpp"
#include "screen.hpp"
#include "renderer.hpp"
#include "gfx.hpp"
#include "imguilayer.hpp"
#include "debug.hpp"
#include "timer.hpp"
#include "physics.hpp"
#include "input.hpp"

// TODO: remove these in the future
#include "shadowpass.hpp"
#include "scenecapture.hpp"

Engine::Engine(const int argc, char* argv[]) {
    console::info(System::Core, "Prism Engine loading...");
    
    console::register_command("test_cmd", console::ArgumentFormat(0), [](const console::Arguments) {
        console::info(System::Core, "Test cmd!");
    });
    
    console::register_variable("rs_dynamic_resolution", render_options.dynamic_resolution);
    
    console::register_command("quit", console::ArgumentFormat(0), [this](const console::Arguments) {
        quit();
    });
        
    for(int i = 0; i < argc; i++)
        command_line_arguments.push_back(argv[i]);

	_input = std::make_unique<Input>();
    _physics = std::make_unique<Physics>();
    _imgui = std::make_unique<ImGuiLayer>();
    assetm = std::make_unique<AssetManager>();
}

Engine::~Engine() {}

void Engine::set_app(App* app) {
    Expects(app != nullptr);
    
    _app = app;
}

App* Engine::get_app() const {
    return _app;
}

void Engine::load_localization(const std::string_view path) {
    Expects(!path.empty());

    auto file = file::open(file::app_domain / path);
    if(file.has_value()) {
        nlohmann::json j;
        file->read_as_stream() >> j;

        _strings = j["strings"].get<std::map<std::string, std::string>>();
    }
}

void Engine::pause() {
    _paused = true;
    push_event("engine_pause");
}

void Engine::unpause() {
    _paused = false;
    push_event("engine_unpause");
}

bool Engine::is_paused() const {
    return _paused;
}

void Engine::quit() {
    _windows[0].quitRequested = true;
}

bool Engine::is_quitting() const {
    return _windows[0].quitRequested;
}

void Engine::prepare_quit() {
    _app->prepare_quit();
}

void Engine::set_gfx(GFX* gfx) {
    _gfx = gfx;
}

GFX* Engine::get_gfx() {
    return _gfx;
}

Input* Engine::get_input() {
	return _input.get();
}

Renderer* Engine::get_renderer() {
    return _renderer.get();
}

Physics* Engine::get_physics() {
    return _physics.get();
}

void Engine::create_empty_scene() {
    auto scene = std::make_unique<Scene>();
    
    setup_scene(*scene);
    
    _scenes.push_back(std::move(scene));
    _current_scene = _scenes.back().get();
}

Scene* Engine::load_scene(const file::Path path) {
    Expects(!path.empty());

    auto file = file::open(path);
    if(!file.has_value()) {
        console::error(System::Core, "Failed to load scene from {}!", path);
        return nullptr;
    }

    nlohmann::json j;
    file->read_as_stream() >> j;

    auto scene = std::make_unique<Scene>();

    std::map<Object, std::string> parentQueue;

    for(auto& obj : j["objects"]) {
        if(obj.contains("prefabPath")) {
            Object o = add_prefab(*scene, file::app_domain / obj["prefabPath"].get<std::string_view>());

            scene->get(o).name = obj["name"];

            auto& transform = scene->get<Transform>(o);
            transform.position = obj["position"];
            transform.rotation = obj["rotation"];
            transform.scale = obj["scale"];
        } else {
            auto o = load_object(*scene.get(), obj);

            if(obj.contains("parent"))
                parentQueue[o] = obj["parent"];
        }
    }

    for(auto& [obj, toParent] : parentQueue)
        scene->get<Data>(obj).parent = scene->find_object(toParent);
    
    setup_scene(*scene);
    
    _scenes.push_back(std::move(scene));
    _current_scene = _scenes.back().get();
    _path_to_scene[path.string()] = _scenes.back().get();
    
    return _scenes.back().get();
}

void Engine::save_scene(const std::string_view path) {
    Expects(!path.empty());
    
    nlohmann::json j;

    for(auto& obj : _current_scene->get_objects()) {
        if(!_current_scene->get(obj).editor_object)
            j["objects"].push_back(save_object(obj));
    }

    std::ofstream out(path.data());
    out << j;
}

ui::Screen* Engine::load_screen(const file::Path path) {
    Expects(!path.empty());

    return new ui::Screen(path);
}

void Engine::set_screen(ui::Screen* screen) {
    _current_screen = screen;
    
    screen->extent = _windows[0].extent;
    screen->calculate_sizes();
    
    get_renderer()->set_screen(screen);
}

ui::Screen* Engine::get_screen() const {
    return _current_screen;
}

AnimationChannel Engine::load_animation(nlohmann::json a) {
    AnimationChannel animation;

    for(auto& kf : a["frames"]) {
        PositionKeyFrame keyframe;
        keyframe.time = kf["time"];
        keyframe.value = kf["value"];

        animation.positions.push_back(keyframe);
    }

    return animation;
}

Animation Engine::load_animation(const file::Path path) {
    Expects(!path.empty());

    auto file = file::open(path, true);
    if(!file.has_value()) {
        console::error(System::Core, "Failed to load animation from {}!", path);
        return {};
    }

    Animation anim;

    file->read(&anim.duration);
    file->read(&anim.ticks_per_second);

    unsigned int num_channels;
    file->read(&num_channels);

    for(unsigned int i = 0; i < num_channels; i++) {
        AnimationChannel channel;

        file->read_string(channel.id);

        unsigned int num_positions;
        file->read(&num_positions);

        for(unsigned int j = 0; j < num_positions; j++) {
            PositionKeyFrame key;
            file->read(&key);

            channel.positions.push_back(key);
        }

        unsigned int num_rotations;
        file->read(&num_rotations);

        for(unsigned int j = 0; j < num_rotations; j++) {
            RotationKeyFrame key;
            file->read(&key);

            channel.rotations.push_back(key);
        }

        unsigned int num_scales;
        file->read(&num_scales);

        for(unsigned int j = 0; j < num_scales; j++) {
            ScaleKeyFrame key;
            file->read(&key);

            channel.scales.push_back(key);
        }

        anim.channels.push_back(channel);
    }

    return anim;
}

void Engine::load_cutscene(const file::Path path) {
    Expects(!path.empty());

    cutscene = std::make_unique<Cutscene>();

    auto file = file::open(path);
    if(!file.has_value()) {
        console::error(System::Core, "Failed to load cutscene from {}!", path);
        return;
    }

    nlohmann::json j;
    file->read_as_stream() >> j;

    for(auto& s : j["shots"]) {
        Shot shot;
        shot.begin = s["begin"];
        shot.length = s["end"];

        for(auto& animation : s["animations"]) {
            shot.channels.push_back(load_animation(animation));
        }

        if(_path_to_scene.count(s["scene"])) {
            shot.scene = _path_to_scene[s["scene"]];

            // try to find main camera
            auto [obj, cam] = shot.scene->get_all<Camera>()[0];

            for(auto& anim : shot.channels) {
                if(anim.target == NullObject)
                    anim.target = obj;
            }
        } else {
            load_scene(file::root_path(path) / s["scene"].get<std::string_view>());

            if(engine->get_scene() == nullptr)
                engine->create_empty_scene();
            
            auto cameraObj = engine->get_scene()->add_object();
            engine->get_scene()->add<Camera>(cameraObj);

            for(auto& anim : shot.channels) {
                if(anim.target == NullObject)
                    anim.target = cameraObj;
            }

            _path_to_scene[s["scene"]] = _current_scene;
            shot.scene = _current_scene;
        }

        cutscene->shots.push_back(shot);
    }
}

void Engine::save_cutscene(const std::string_view path) {
    Expects(!path.empty());

    nlohmann::json j;

    for(auto& shot : engine->cutscene->shots) {
        nlohmann::json s;
        s["begin"] = shot.begin;
        s["end"] = shot.length;

        for(auto& [path, scene] : _path_to_scene) {
            if(shot.scene == scene)
                s["scene"] = path;
        }

        j["shots"].push_back(s);
    }

    std::ofstream out(path.data());
    out << j;
}

Object Engine::add_prefab(Scene& scene, const file::Path path, const std::string_view override_name) {
    Expects(!path.empty());
    
    auto file = file::open(path);
    if(!file.has_value()) {
        console::error(System::Core, "Failed to load prefab from {}!", path);
        return NullObject;
    }

    nlohmann::json j;
    file->read_as_stream() >> j;

    Object root_node = NullObject;

    std::map<Object, std::string> parent_queue;

    for(const auto obj : j["objects"]) {
        auto o = load_object(scene, obj);

        if(obj.contains("parent")) {
            parent_queue[o] = obj["parent"];
        } else {
            root_node = o;
        }
    }

    for(auto& [obj, parent_name] : parent_queue)
        scene.get(obj).parent = scene.find_object(parent_name);

    if(!override_name.empty() && root_node != NullObject)
        scene.get(root_node).name = override_name;

    return root_node;
}

void Engine::save_prefab(const Object root, const std::string_view path) {
    Expects(root != NullObject);
    Expects(!path.empty());
    
    nlohmann::json j;

    for(auto& obj : _current_scene->get_objects()) {
        if(!_current_scene->get(obj).editor_object)
            j["objects"].push_back(save_object(obj));
    }

    std::ofstream out(path.data());
    out << j;
}

void Engine::add_window(void* native_handle, const int identifier, const prism::Extent extent) {
    Expects(native_handle != nullptr);
    Expects(identifier >= 0);
    
    if(identifier == 0) {
        _renderer = std::make_unique<Renderer>(_gfx);
    }
    
    const auto drawable_extent = platform::get_window_drawable_size(identifier);
    
    _gfx->initialize_view(native_handle, identifier, drawable_extent.width, drawable_extent.height);

    Window& window = _windows.emplace_back();
    window.identifier = identifier;
    window.extent = extent;
    window.render_target = _renderer->allocate_render_target(drawable_extent);

    render_ready = true;
}

void Engine::remove_window(const int identifier) {
    Expects(identifier >= 0);

    utility::erase_if(_windows, [identifier](Window& w) {
        return w.identifier == identifier;
    });
}

void Engine::resize(const int identifier, const prism::Extent extent) {
    Expects(identifier >= 0);
    
    auto window = get_window(identifier);
    if (window == nullptr)
        return;

    window->extent = extent;
    
    const auto drawable_extent = platform::get_window_drawable_size(identifier);

    _gfx->recreate_view(identifier, drawable_extent.width, drawable_extent.height);
    _renderer->resize_render_target(*window->render_target, drawable_extent);

    if(identifier == 0) {
        if(_current_screen != nullptr) {
            _current_screen->extent = extent;
            _current_screen->calculate_sizes();
        }
    }
}

void Engine::process_key_down(const unsigned int keyCode) {
    Expects(keyCode >= 0);
    
    _imgui->process_key_down(keyCode);

    if(keyCode == platform::get_keycode(debug_button) && !ImGui::GetIO().WantTextInput)
        debug_enabled = !debug_enabled;
}

void Engine::process_key_up(const unsigned int keyCode) {
    Expects(keyCode >= 0);
    
    _imgui->process_key_up(keyCode);
}

void Engine::process_mouse_down(const int button, const prism::Offset offset) {
    Expects(offset.x >= 0);
    Expects(offset.y >= 0);
    
	if(_current_screen != nullptr && button == 0)
        _current_screen->process_mouse(offset.x, offset.y);
}

void Engine::push_event(const std::string_view name, const std::string_view data) {
    Expects(!name.empty());
    
	if(_current_screen != nullptr)
        _current_screen->process_event(name.data(), data.data());
}

bool Engine::has_localization(const std::string_view id) const {
    Expects(!id.empty());
    
    return _strings.count(id.data());
}

std::string Engine::localize(const std::string id) {
    Expects(!id.empty());
    
    return _strings[id];
}

void Engine::calculate_bone(Mesh& mesh, const Mesh::Part& part, Bone& bone, const Bone* parent_bone) {
    if(part.offset_matrices.empty())
        return;

    Matrix4x4 parent_matrix;
    if(parent_bone != nullptr)
        parent_matrix = parent_bone->local_transform;

    Matrix4x4 local = transform::translate(Matrix4x4(), bone.position);
    local *= matrix_from_quat(bone.rotation);
    local = transform::scale(local, bone.scale);

    bone.local_transform = parent_matrix * local;

    bone.final_transform = mesh.global_inverse_transformation * bone.local_transform * part.offset_matrices[bone.index];

    for(auto& b : mesh.bones) {
        if(b.parent != nullptr && b.parent->index == bone.index)
            calculate_bone(mesh, part, b, &bone);
    }
}

void Engine::calculate_object(Scene& scene, Object object, const Object parent_object) {
    Matrix4x4 parent_matrix;
    if(parent_object != NullObject)
        parent_matrix = scene.get<Transform>(parent_object).model;

    auto& transform = scene.get<Transform>(object);

    Matrix4x4 local = transform::translate(Matrix4x4(), transform.position);
    local *= matrix_from_quat(transform.rotation);
    local = transform::scale(local, transform.scale);

    transform.model = parent_matrix * local;

    if(scene.has<Renderable>(object)) {
        auto& mesh = scene.get<Renderable>(object);

        if(mesh.mesh && !mesh.mesh->bones.empty()) {
            if(mesh.temp_bone_data.empty())
                mesh.temp_bone_data.resize(mesh.mesh->bones.size());
            
            for(auto& part : mesh.mesh->parts) {
                if(scene.get(object).parent != NullObject && scene.has<Renderable>(scene.get(object).parent) && !scene.get<Renderable>(scene.get(object).parent).mesh->bones.empty()) {
                    for(auto [i, ourBone] : utility::enumerate(mesh.mesh->bones)) {
                        for(auto& theirBone : scene.get<Renderable>(scene.get(object).parent).mesh->bones) {
                            if(ourBone.name == theirBone.name)
                                mesh.temp_bone_data[i] = mesh.mesh->global_inverse_transformation * theirBone.local_transform * part.offset_matrices[ourBone.index];
                        }
                    }
                } else {
                    calculate_bone(*mesh.mesh.handle, part, *mesh.mesh->root_bone);

                    for(auto [i, bone] : utility::enumerate(mesh.temp_bone_data))
                        mesh.temp_bone_data[i] = mesh.mesh->bones[i].final_transform;
                }

                engine->get_gfx()->copy_buffer(part.bone_batrix_buffer, mesh.temp_bone_data.data(), 0, mesh.temp_bone_data.size() * sizeof(Matrix4x4));
            }
        }
    }

	for(auto& child : scene.children_of(object))
        calculate_object(scene, child, object);
}

Shot* Engine::get_shot(const float time) {
    for(auto& shot : engine->cutscene->shots) {
        if(time >= shot.begin && time <= (shot.begin + shot.length))
            return &shot;
    }

    return nullptr;
}

void Engine::update_animation_channel(Scene& scene, const AnimationChannel& channel, const float time) {
    {
        int keyframeIndex = -1;
        int i = 0;
        for(auto& frame : channel.positions) {
            if(time >= frame.time)
                keyframeIndex = i;

            i++;
        }

        if(keyframeIndex != -1) {
            Vector3* targetVec = nullptr;
            if(channel.bone != nullptr)
                targetVec = &channel.bone->position;

            if(channel.target != NullObject && scene.has<Data>(channel.target))
                targetVec = &scene.get<Transform>(channel.target).position;

            auto& startFrame = channel.positions[keyframeIndex];

            int endFrameIndex = keyframeIndex + 1;
            if(endFrameIndex > channel.positions.size() - 1) {
                if(targetVec != nullptr)
                    *targetVec = startFrame.value;
            } else {
                auto& endFrame = channel.positions[endFrameIndex];

                if(targetVec != nullptr)
                    *targetVec = lerp(startFrame.value, endFrame.value, (float)(time - startFrame.time) / (float)(endFrame.time - startFrame.time));
            }
        }
    }

    {
        int keyframeIndex = -1;
        int i = 0;
        for(auto& frame : channel.rotations) {
            if(time >= frame.time)
                keyframeIndex = i;

            i++;
        }

        if(keyframeIndex != -1) {
            Quaternion* targetVec = nullptr;
            if(channel.bone != nullptr)
                targetVec = &channel.bone->rotation;

            auto& startFrame = channel.rotations[keyframeIndex];

            int endFrameIndex = keyframeIndex + 1;
            if(endFrameIndex > channel.rotations.size() - 1) {
                if(targetVec != nullptr)
                    *targetVec = startFrame.value;
            } else {
                auto& endFrame = channel.rotations[endFrameIndex];

                if(targetVec != nullptr)
                    *targetVec = lerp(startFrame.value, endFrame.value, (float)(time - startFrame.time) / (float)(endFrame.time - startFrame.time));
            }
        }
    }
}

void Engine::update_cutscene(const float time) {
    Shot* currentShot = get_shot(time);
    if(currentShot != nullptr) {
        _current_scene = currentShot->scene;

        for(auto& channel : currentShot->channels)
            update_animation_channel(*_current_scene, channel, time);
    } else {
        _current_scene = nullptr;
    }
}

void Engine::update_animation(const Animation& anim, const float time) {
    for(const auto& channel : anim.channels)
        update_animation_channel(*_current_scene, channel, time);
}

void Engine::begin_frame(const float delta_time) {
    _imgui->begin_frame(delta_time);
    
    if(debug_enabled)
        draw_debug_ui();

    if(_app != nullptr)
        _app->begin_frame();
}

int frame_delay = 0;
const int frame_delay_between_resolution_change = 60;

void Engine::update(const float delta_time) {
    const float ideal_delta_time = 0.01667f;
    
    if(render_options.dynamic_resolution) {
        frame_delay++;
        
        if(frame_delay >= frame_delay_between_resolution_change) {            
            if(delta_time > ideal_delta_time) {
                render_options.render_scale -= 0.1f;
                
                render_options.render_scale = std::fmax(render_options.render_scale, 0.1f);
            } else {
                render_options.render_scale += 0.1f;
                
                render_options.render_scale = std::fmin(render_options.render_scale, 1.0f);
            }
                
            frame_delay = 0;
        }
    }
    
    for(auto& timer : _timers) {
        if(!_paused || (_paused && timer->continue_during_pause))
            timer->current_time += delta_time;

        if(timer->current_time >= timer->duration) {
            timer->callback();

            timer->current_time = 0.0f;

            if(timer->remove_on_trigger)
                _timersToRemove.push_back(timer);
        }
    }

    for(auto& timer : _timersToRemove)
        utility::erase(_timers, timer);

    _timersToRemove.clear();

    _input->update();

    _app->update(delta_time);
    
    if(_current_scene != nullptr) {
        if(update_physics && !_paused)
            _physics->update(delta_time);
        
        if(cutscene != nullptr && play_cutscene && !_paused) {
            update_cutscene(current_cutscene_time);
            
            current_cutscene_time += delta_time;
        }
        
        for(auto& target : _animation_targets) {
            if((target.current_time * target.animation.ticks_per_second) > target.animation.duration) {
                if(target.looping) {
                    target.current_time = 0.0f;
                } else {
                    utility::erase(_animation_targets, target);
                }
            } else {
                update_animation(target.animation, target.current_time * target.animation.ticks_per_second);
                
                target.current_time += delta_time * target.animation_speed_modifier;
            }
        }
    
        update_scene(*_current_scene);
    }
    
    assetm->perform_cleanup();
}

void Engine::update_scene(Scene& scene) {
    for(auto& obj : scene.get_objects()) {
        if(scene.get(obj).parent == NullObject)
            calculate_object(scene, obj);
    }
}

void Engine::render(const int index) {
    Expects(index >= 0);
    
    auto window = get_window(index);
    if(window == nullptr)
        return;
    
    if(index == 0) {
        if(_current_screen != nullptr && _current_screen->view_changed) {
            _renderer->update_screen();
            _current_screen->view_changed = false;
        }
        
        _imgui->render(0);
    }

    GFXCommandBuffer* commandbuffer = _gfx->acquire_command_buffer();

    _app->render(commandbuffer);

    if(_renderer != nullptr)
        _renderer->render(commandbuffer, _current_scene, *window->render_target, index);

    _gfx->submit(commandbuffer, index);
}

void Engine::add_timer(Timer& timer) {
    _timers.push_back(&timer);
}

Scene* Engine::get_scene() {
    return _current_scene;
}

Scene* Engine::get_scene(const std::string_view name) {
    Expects(!name.empty());
    
    return _path_to_scene[name.data()];
}

void Engine::set_current_scene(Scene* scene) {
    _current_scene = scene;
}

std::string_view Engine::get_scene_path() const {
    for(auto& [path, scene] : _path_to_scene) {
        if(scene == this->_current_scene)
            return path;
    }

    return "";
}

void Engine::on_remove(Object object) {
    Expects(object != NullObject);

    _physics->remove_object(object);
}

void Engine::play_animation(Animation animation, Object object, bool looping) {
    Expects(object != NullObject);
    
    if(!_current_scene->has<Renderable>(object))
        return;
    
    AnimationTarget target;
    target.animation = animation;
    target.target = object;
    target.looping = looping;

    for(auto& channel : target.animation.channels) {
        for(auto& bone : _current_scene->get<Renderable>(object).mesh->bones) {
            if(channel.id == bone.name)
                channel.bone = &bone;
        }
    }

    _animation_targets.push_back(target);
}

void Engine::set_animation_speed_modifier(Object target, float modifier) {
    for(auto& animation : _animation_targets) {
        if(animation.target == target)
            animation.animation_speed_modifier = modifier;
    }
}

void Engine::stop_animation(Object target) {
    for(auto& animation : _animation_targets) {
        if(animation.target == target)
            utility::erase(_animation_targets, animation);
    }
}

void Engine::setup_scene(Scene& scene) {
    _physics->reset();

    scene.on_remove = [this](Object obj) {
        on_remove(obj);
    };
    
    get_renderer()->shadow_pass->create_scene_resources(scene);
    get_renderer()->scene_capture->create_scene_resources(scene);
    
    scene.reset_shadows();
    scene.reset_environment();
}
