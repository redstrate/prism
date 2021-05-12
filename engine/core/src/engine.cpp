#include "engine.hpp"

#include <nlohmann/json.hpp>
#include <utility>
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
#include "imgui_backend.hpp"
#include "debug.hpp"
#include "timer.hpp"
#include "physics.hpp"
#include "input.hpp"

// TODO: remove these in the future
#include "shadowpass.hpp"
#include "scenecapture.hpp"

using prism::engine;

engine::engine(const int argc, char* argv[]) {
    log::info(System::Core, "Prism Engine loading...");
    
    console::register_command("test_cmd", console::argument_format(0), [](const console::arguments&) {
        log::info(System::Core, "Test cmd!");
    });
    
    console::register_variable("rs_dynamic_resolution", render_options.dynamic_resolution);
    
    console::register_command("quit", console::argument_format(0), [this](const console::arguments&) {
        quit();
    });
        
    for(int i = 0; i < argc; i++)
        command_line_arguments.emplace_back(argv[i]);

    input = std::make_unique<input_system>();
    physics = std::make_unique<Physics>();
    imgui = std::make_unique<prism::imgui_backend>();
    assetm = std::make_unique<AssetManager>();
}

engine::~engine() = default;

void engine::set_app(prism::app* p_app) {
    Expects(p_app != nullptr);

    this->app = p_app;
}

prism::app* engine::get_app() const {
    return app;
}

void engine::load_localization(const std::string_view path) {
    Expects(!path.empty());

    auto file = prism::open_file(prism::app_domain / path);
    if(file.has_value()) {
        nlohmann::json j;
        file->read_as_stream() >> j;

        strings = j["strings"].get<std::map<std::string, std::string>>();
    }
}

void engine::pause() {
    paused = true;
    push_event("engine_pause");
}

void engine::unpause() {
    paused = false;
    push_event("engine_unpause");
}

bool engine::is_paused() const {
    return paused;
}

void engine::quit() {
    if(!windows.empty())
        windows[0]->quit_requested = true;
}

bool engine::is_quitting() const {
    if(!windows.empty())
        return windows[0]->quit_requested;
}

void engine::prepare_quit() {
    app->prepare_quit();
}

void engine::set_gfx(GFX* p_gfx) {
    Expects(p_gfx != nullptr);

    this->gfx = p_gfx;
}

GFX* engine::get_gfx() {
    return gfx;
}

prism::input_system* engine::get_input() {
	return input.get();
}

prism::renderer* engine::get_renderer() {
    return renderer.get();
}

Physics* engine::get_physics() {
    return physics.get();
}

void engine::create_empty_scene() {
    auto scene = std::make_unique<Scene>();
    
    setup_scene(*scene);
    
    scenes.push_back(std::move(scene));
    current_scene = scenes.back().get();
}

Scene* engine::load_scene(const prism::path& path) {
    Expects(!path.empty());

    auto file = prism::open_file(path);
    if(!file.has_value()) {
        prism::log::error(System::Core, "Failed to load scene from {}!", path);
        return nullptr;
    }

    nlohmann::json j;
    file->read_as_stream() >> j;

    auto scene = std::make_unique<Scene>();

    std::map<Object, std::string> parentQueue;

    for(auto& obj : j["objects"]) {
        if(obj.contains("prefabPath")) {
            Object o = add_prefab(*scene, prism::app_domain / obj["prefabPath"].get<std::string_view>());

            scene->get(o).name = obj["name"];

            auto& transform = scene->get<Transform>(o);
            transform.position = obj["position"];
            transform.rotation = obj["rotation"];
            transform.scale = obj["scale"];
        } else {
            auto o = load_object(*scene, obj);

            if(obj.contains("parent"))
                parentQueue[o] = obj["parent"];
        }
    }

    for(auto& [obj, toParent] : parentQueue)
        scene->get<Data>(obj).parent = scene->find_object(toParent);
    
    setup_scene(*scene);
    
    scenes.push_back(std::move(scene));
    current_scene = scenes.back().get();
    path_to_scene[path.string()] = scenes.back().get();
    
    return scenes.back().get();
}

void engine::save_scene(const std::string_view path) {
    Expects(!path.empty());
    
    nlohmann::json j;

    for(auto& obj : current_scene->get_objects()) {
        if(!current_scene->get(obj).editor_object)
            j["objects"].push_back(save_object(obj));
    }

    std::ofstream out(path.data());
    out << j;
}

ui::Screen* engine::load_screen(const prism::path& path) {
    Expects(!path.empty());

    return new ui::Screen(path);
}

void engine::set_screen(ui::Screen* screen) {
    current_screen = screen;
    
    screen->extent = windows[0]->extent;
    screen->calculate_sizes();
    
    get_renderer()->set_screen(screen);
}

ui::Screen* engine::get_screen() const {
    return current_screen;
}

AnimationChannel engine::load_animation(nlohmann::json a) {
    AnimationChannel animation;

    for(auto& kf : a["frames"]) {
        PositionKeyFrame keyframe;
        keyframe.time = kf["time"];
        keyframe.value = kf["value"];

        animation.positions.push_back(keyframe);
    }

    return animation;
}

Animation engine::load_animation(const prism::path& path) {
    Expects(!path.empty());

    auto file = prism::open_file(path, true);
    if(!file.has_value()) {
        prism::log::error(System::Core, "Failed to load animation from {}!", path);
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

void engine::load_cutscene(const prism::path& path) {
    Expects(!path.empty());

    cutscene = std::make_unique<Cutscene>();

    auto file = prism::open_file(path);
    if(!file.has_value()) {
        prism::log::error(System::Core, "Failed to load cutscene from {}!", path);
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

        if(path_to_scene.count(s["scene"])) {
            shot.scene = path_to_scene[s["scene"]];

            // try to find main camera
            auto [obj, cam] = shot.scene->get_all<Camera>()[0];

            for(auto& anim : shot.channels) {
                if(anim.target == NullObject)
                    anim.target = obj;
            }
        } else {
            load_scene(prism::root_path(path) / s["scene"].get<std::string_view>());

            if(get_scene() == nullptr)
                create_empty_scene();
            
            auto cameraObj = get_scene()->add_object();
            get_scene()->add<Camera>(cameraObj);

            for(auto& anim : shot.channels) {
                if(anim.target == NullObject)
                    anim.target = cameraObj;
            }

            path_to_scene[s["scene"]] = current_scene;
            shot.scene = current_scene;
        }

        cutscene->shots.push_back(shot);
    }
}

void engine::save_cutscene(const std::string_view path) {
    Expects(!path.empty());

    nlohmann::json j;

    for(auto& shot : cutscene->shots) {
        nlohmann::json s;
        s["begin"] = shot.begin;
        s["end"] = shot.length;

        for(auto& [path, scene] : path_to_scene) {
            if(shot.scene == scene)
                s["scene"] = path;
        }

        j["shots"].push_back(s);
    }

    std::ofstream out(path.data());
    out << j;
}

Object engine::add_prefab(Scene& scene, const prism::path& path, const std::string_view override_name) {
    Expects(!path.empty());
    
    auto file = prism::open_file(path);
    if(!file.has_value()) {
        prism::log::error(System::Core, "Failed to load prefab from {}!", path);
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

void engine::save_prefab(const Object root, const std::string_view path) {
    Expects(root != NullObject);
    Expects(!path.empty());
    
    nlohmann::json j;

    for(auto& obj : current_scene->get_objects()) {
        if(!current_scene->get(obj).editor_object)
            j["objects"].push_back(save_object(obj));
    }

    std::ofstream out(path.data());
    out << j;
}

void engine::add_window(void* native_handle, const int identifier, const prism::Extent extent) {
    Expects(native_handle != nullptr);
    Expects(identifier >= 0);
    
    if(identifier == 0) {
        renderer = std::make_unique<prism::renderer>(gfx);
    }
    
    const auto drawable_extent = platform::get_window_drawable_size(identifier);
    
    gfx->initialize_view(native_handle, identifier, drawable_extent.width, drawable_extent.height);

    auto* window = new Window();
    windows.push_back(window);
    
    window->identifier = identifier;
    window->extent = extent;
    window->render_target = renderer->allocate_render_target(drawable_extent);

    render_ready = true;
}

void engine::remove_window(const int identifier) {
    Expects(identifier >= 0);

    utility::erase_if(windows, [identifier](Window*& w) {
        return w->identifier == identifier;
    });
}

void engine::resize(const int identifier, const prism::Extent extent) {
    Expects(identifier >= 0);
    
    auto window = get_window(identifier);
    if (window == nullptr)
        return;

    window->extent = extent;
    
    const auto drawable_extent = platform::get_window_drawable_size(identifier);

    gfx->recreate_view(identifier, drawable_extent.width, drawable_extent.height);
    renderer->resize_render_target(*window->render_target, drawable_extent);

    if(identifier == 0) {
        if(current_screen != nullptr) {
            current_screen->extent = extent;
            current_screen->calculate_sizes();
        }
    }
}

void engine::process_key_down(const unsigned int keyCode) {
    Expects(keyCode >= 0);
    
    imgui->process_key_down(keyCode);

    if(keyCode == platform::get_keycode(debug_button) && !ImGui::GetIO().WantTextInput)
        debug_enabled = !debug_enabled;
}

void engine::process_key_up(const unsigned int keyCode) {
    Expects(keyCode >= 0);
    
    imgui->process_key_up(keyCode);
}

void engine::process_mouse_down(const int button, const prism::Offset offset) {
	if(current_screen != nullptr && button == 0)
        current_screen->process_mouse(offset.x, offset.y);

	imgui->process_mouse_down(button);
}

void engine::push_event(const std::string_view name, const std::string_view data) {
    Expects(!name.empty());
    
	if(current_screen != nullptr)
        current_screen->process_event(name.data(), data.data());
}

bool engine::has_localization(const std::string_view id) const {
    Expects(!id.empty());
    
    return strings.count(id.data());
}

std::string engine::localize(const std::string& id) {
    Expects(!id.empty());
    
    return strings[id];
}

void engine::calculate_bone(Mesh& mesh, const Mesh::Part& part, Bone& bone, const Bone* parent_bone) {
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

void engine::calculate_object(Scene& scene, Object object, const Object parent_object) {
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

                gfx->copy_buffer(part.bone_batrix_buffer, mesh.temp_bone_data.data(), 0, mesh.temp_bone_data.size() * sizeof(Matrix4x4));
            }
        }
    }

	for(auto& child : scene.children_of(object))
        calculate_object(scene, child, object);
}

Shot* engine::get_shot(const float time) const {
    for(auto& shot : cutscene->shots) {
        if(time >= shot.begin && time <= (shot.begin + shot.length))
            return &shot;
    }

    return nullptr;
}

void engine::update_animation_channel(Scene& scene, const AnimationChannel& channel, const float time) {
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

void engine::update_cutscene(const float time) {
    Shot* currentShot = get_shot(time);
    if(currentShot != nullptr) {
        current_scene = currentShot->scene;

        for(auto& channel : currentShot->channels)
            update_animation_channel(*current_scene, channel, time);
    } else {
        current_scene = nullptr;
    }
}

void engine::update_animation(const Animation& anim, const float time) {
    for(const auto& channel : anim.channels)
        update_animation_channel(*current_scene, channel, time);
}

void engine::begin_frame(const float delta_time) {
    imgui->begin_frame(delta_time);
    
    if(debug_enabled)
        draw_debug_ui();

    if(app != nullptr)
        app->begin_frame();
}

void engine::end_frame() {
    ImGui::UpdatePlatformWindows();
}

int frame_delay = 0;
const int frame_delay_between_resolution_change = 60;

void engine::update(const float delta_time) {
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
    
    for(auto& timer : timers) {
        if(!paused || (paused && timer->continue_during_pause))
            timer->current_time += delta_time;

        if(timer->current_time >= timer->duration) {
            timer->callback();

            timer->current_time = 0.0f;

            if(timer->remove_on_trigger)
                timers_to_remove.push_back(timer);
        }
    }

    for(auto& timer : timers_to_remove)
        utility::erase(timers, timer);

    timers_to_remove.clear();

    input->update();

    app->update(delta_time);
    
    if(current_scene != nullptr) {
        if(update_physics && !paused)
            physics->update(delta_time);
        
        if(cutscene != nullptr && play_cutscene && !paused) {
            update_cutscene(current_cutscene_time);
            
            current_cutscene_time += delta_time;
        }
        
        for(auto& target : animation_targets) {
            if((target.current_time * target.animation.ticks_per_second) > target.animation.duration) {
                if(target.looping) {
                    target.current_time = 0.0f;
                } else {
                    utility::erase(animation_targets, target);
                }
            } else {
                update_animation(target.animation, target.current_time * target.animation.ticks_per_second);
                
                target.current_time += delta_time * target.animation_speed_modifier;
            }
        }
    
        update_scene(*current_scene);
    }
    
    assetm->perform_cleanup();
}

void engine::update_scene(Scene& scene) {
    for(auto& obj : scene.get_objects()) {
        if(scene.get(obj).parent == NullObject)
            calculate_object(scene, obj);
    }
}

void engine::render(const int index) {
    Expects(index >= 0);
    
    auto window = get_window(index);
    if(window == nullptr)
        return;
    
    GFXCommandBuffer* commandbuffer = gfx->acquire_command_buffer(true);
    
    if(index == 0) {
        if(current_screen != nullptr && current_screen->view_changed) {
            renderer->update_screen();
            current_screen->view_changed = false;
        }
        
        imgui->render(0);
        
        app->render(commandbuffer);
    }

    if(renderer != nullptr)
        renderer->render(commandbuffer, app->wants_no_scene_rendering() ? nullptr : current_scene, *window->render_target, index);

    gfx->submit(commandbuffer, index);
}

void engine::add_timer(Timer& timer) {
    timers.push_back(&timer);
}

Scene* engine::get_scene() {
    return current_scene;
}

Scene* engine::get_scene(const std::string_view name) {
    Expects(!name.empty());
    
    return path_to_scene[name.data()];
}

void engine::set_current_scene(Scene* scene) {
    current_scene = scene;
}

std::string_view engine::get_scene_path() const {
    for(auto& [path, scene] : path_to_scene) {
        if(scene == this->current_scene)
            return path;
    }

    return "";
}

void engine::on_remove(Object object) {
    Expects(object != NullObject);

    physics->remove_object(object);
}

void engine::play_animation(Animation animation, Object object, bool looping) {
    Expects(object != NullObject);
    
    if(!current_scene->has<Renderable>(object))
        return;
    
    AnimationTarget target;
    target.animation = std::move(animation);
    target.target = object;
    target.looping = looping;

    for(auto& channel : target.animation.channels) {
        for(auto& bone : current_scene->get<Renderable>(object).mesh->bones) {
            if(channel.id == bone.name)
                channel.bone = &bone;
        }
    }

    animation_targets.push_back(target);
}

void engine::set_animation_speed_modifier(Object target, float modifier) {
    for(auto& animation : animation_targets) {
        if(animation.target == target)
            animation.animation_speed_modifier = modifier;
    }
}

void engine::stop_animation(Object target) {
    for(auto& animation : animation_targets) {
        if(animation.target == target)
            utility::erase(animation_targets, animation);
    }
}

void engine::setup_scene(Scene& scene) {
    physics->reset();

    scene.on_remove = [this](Object obj) {
        on_remove(obj);
    };
    
    get_renderer()->shadow_pass->create_scene_resources(scene);
    get_renderer()->scene_capture->create_scene_resources(scene);
    
    scene.reset_shadows();
    scene.reset_environment();
}
