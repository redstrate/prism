#include "commoneditor.hpp"

#include <imgui.h>
#include <imgui_stdlib.h>
#include <imgui_internal.h>
#include <nlohmann/json.hpp>

#include "engine.hpp"
#include "imguipass.hpp"
#include "file.hpp"
#include "json_conversions.hpp"
#include "platform.hpp"
#include "transform.hpp"
#include "log.hpp"
#include "shadowpass.hpp"
#include "string_utils.hpp"
#include "assertions.hpp"
#include "gfx.hpp"
#include "gfx_commandbuffer.hpp"
#include "imgui_utility.hpp"
#include "screen.hpp"
#include "console.hpp"
#include "input.hpp"
#include "scenecapture.hpp"

const std::map<ImGuiKey, InputButton> imToPl = {
    {ImGuiKey_Tab, InputButton::Tab},
    {ImGuiKey_LeftArrow, InputButton::LeftArrow},
    {ImGuiKey_RightArrow, InputButton::RightArrow},
    {ImGuiKey_UpArrow, InputButton::Escape},
    {ImGuiKey_DownArrow, InputButton::Escape},
    {ImGuiKey_PageUp, InputButton::Escape},
    {ImGuiKey_PageDown, InputButton::Escape},
    {ImGuiKey_Home, InputButton::Escape},
    {ImGuiKey_End, InputButton::Escape},
    {ImGuiKey_Insert, InputButton::Escape},
    {ImGuiKey_Delete, InputButton::Escape},
    {ImGuiKey_Backspace, InputButton::Backspace},
    {ImGuiKey_Space, InputButton::Space},
    {ImGuiKey_Enter, InputButton::Enter},
    {ImGuiKey_Escape, InputButton::Escape},
    {ImGuiKey_A, InputButton::A},
    {ImGuiKey_C, InputButton::C},
    {ImGuiKey_V, InputButton::V},
    {ImGuiKey_X, InputButton::X},
    {ImGuiKey_Y, InputButton::Y},
    {ImGuiKey_Z, InputButton::Z}
};

CommonEditor::CommonEditor(std::string id) : id(id) {
#ifdef PLATFORM_MACOS
    file::set_domain_path(file::Domain::App, "../../../data");
#else
    file::set_domain_path(file::Domain::App, "data");
#endif
    file::set_domain_path(file::Domain::Internal, "{resource_dir}/shaders");
    
    ImGuiIO& io = ImGui::GetIO();
    
    iniFileName = (file::get_writeable_directory() / "imgui.ini").string();
    
    io.IniFilename = iniFileName.c_str();
    
    ImGui::LoadIniSettingsFromDisk(io.IniFilename);
    
    load_options();
    
    auto input = engine->get_input();
    input->add_binding("movementX");
    input->add_binding("movementY");
    
    input->add_binding_button("movementX", InputButton::A, -1.0f);
    input->add_binding_button("movementX", InputButton::D, 1.0f);
    
    input->add_binding_button("movementY", InputButton::W, -1.0f);
    input->add_binding_button("movementY", InputButton::S, 1.0f);
    
    input->add_binding("lookX");
    input->add_binding("lookY");
    
    input->add_binding_axis("lookX", prism::axis::MouseX);
    input->add_binding_axis("lookY", prism::axis::MouseY);
    
    input->add_binding("cameraLook");
    input->add_binding_button("cameraLook", InputButton::MouseRight);
    
    input->add_binding("cameraSelect");
    input->add_binding_button("cameraSelect", InputButton::MouseLeft);
}

void CommonEditor::initialize_render() {
    load_thumbnail_cache();
}

static float yaw = 0.0f;
static float pitch = 0.0f;
static bool willCaptureMouse = false;

static float previous_intersect = 0.0;

void CommonEditor::update(float deltaTime) {
    if(engine->get_scene() != nullptr) {
        Vector3 offset;
        
        willCaptureMouse = engine->get_input()->is_repeating("cameraLook") && accepting_viewport_input;
        
        platform::capture_mouse(willCaptureMouse);
        
        if(willCaptureMouse) {
            yaw += engine->get_input()->get_value("lookX") * 50.0f * deltaTime;
            pitch += engine->get_input()->get_value("lookY") * 50.0f * deltaTime;
            
            const float speed = 7.00f;
            
            Vector3 forward, right;
            forward = normalize(angle_axis(yaw, Vector3(0, 1, 0)) * angle_axis(pitch, Vector3(1, 0, 0)) * Vector3(0, 0, 1));
            right = normalize(angle_axis(yaw, Vector3(0, 1, 0)) * Vector3(1, 0, 0));
            
            float movX = engine->get_input()->get_value("movementX");
            float movY = engine->get_input()->get_value("movementY");
            
            auto [obj, cam] = engine->get_scene()->get_all<Camera>()[0];
            
            engine->get_scene()->get<Transform>(obj).position += right * movX * speed * deltaTime;
            engine->get_scene()->get<Transform>(obj).position += forward * -movY * speed * deltaTime;
            
            engine->get_scene()->get<Transform>(obj).rotation = angle_axis(yaw, Vector3(0, 1, 0)) * angle_axis(pitch, Vector3(1, 0, 0));
        }
        
        doing_viewport_input = willCaptureMouse;
        
        if(debugPass != nullptr) {
            if(engine->get_input()->is_pressed("cameraSelect") && accepting_viewport_input && !transforming_axis) {
                debugPass->get_selected_object(viewport_x, viewport_y, [this](SelectableObject object) {
                    if(object.object == NullObject) {
                        object_selected(NullObject);
                        return;
                    }
                    
                    if(object.type == SelectableObject::Type::Handle) {
                        transforming_axis = true;
                        axis = object.axis;
                        previous_intersect = 0.0;
                        last_object_position = engine->get_scene()->get<Transform>(selected_object).position;
                    } else {
                        object_selected(object.object);
                    }
                });
            }
            
            if(transforming_axis) {
                if(!engine->get_input()->is_pressed("cameraSelect", true))
                    transforming_axis = false;
                
                auto [obj, cam] = engine->get_scene()->get_all<Camera>()[0];
                
                const auto width = viewport_width;
                const auto height = viewport_height;
                
                Vector2 n = Vector2(((viewport_x / (float)width) * 2) - 1,
                                    ((viewport_y / (float)height) * 2) - 1); // [-1, 1] mouse coordinates
                n.y = -n.y;
                
                n.x = std::clamp(n.x, -1.0f, 1.0f);
                n.y = std::clamp(n.y, -1.0f, 1.0f);
                                
                const Matrix4x4 view_proj_inverse = inverse(cam.perspective * cam.view);
                
                Vector4 ray_start = view_proj_inverse * Vector4(n.x, n.y, 0.0f, 1.0f);
                ray_start *= 1.0f / ray_start.w;
                
                Vector4 ray_end = view_proj_inverse * Vector4(n.x, n.y, 1.0f, 1.0f);
                ray_end *= 1.0f / ray_end.w;
                
                ray camera_ray;
                camera_ray.origin = ray_start.xyz;
                camera_ray.direction = normalize(ray_end.xyz - ray_start.xyz);
                camera_ray.t = std::numeric_limits<float>::max();
                
                auto& transform = engine->get_scene()->get<Transform>(selected_object);
                
                ray transform_ray;
                transform_ray.origin = last_object_position;
                transform_ray.t = std::numeric_limits<float>::max();
                
                switch(axis) {
                    case SelectableObject::Axis::X:
                        transform_ray.direction = Vector3(1, 0, 0);
                        break;
                    case SelectableObject::Axis::Y:
                        transform_ray.direction = Vector3(0, 1, 0);
                        break;
                    case SelectableObject::Axis::Z:
                        transform_ray.direction = Vector3(0, 0, 1);
                        break;
                }
                
                closest_distance_between_lines(camera_ray, transform_ray);
                
                const float current_intersect = transform_ray.t;
                if(previous_intersect == 0.0)
                    previous_intersect = current_intersect;
                
                const float delta = current_intersect - previous_intersect;
                                
                previous_intersect = current_intersect;
                
                switch(axis) {
                    case SelectableObject::Axis::X:
                        transform.position.x -= delta;
                        break;
                    case SelectableObject::Axis::Y:
                        transform.position.y -= delta;
                        break;
                    case SelectableObject::Axis::Z:
                        transform.position.z -= delta;
                        break;
                }
            }
        }
    }
    
    updateEditor(deltaTime);
}

void CommonEditor::prepare_quit() {
    save_options();
    
    ImGui::SaveIniSettingsToDisk(iniFileName.c_str());
    
    save_thumbnail_cache();
}

bool CommonEditor::should_quit() {
    return true;
}

bool filesystem_cached = false;

std::map<std::filesystem::path, AssetType> asset_files;

void cacheAssetFilesystem();

void CommonEditor::begin_frame() {    
    drawUI();
        
    if(open_asset_popup) {
        ImGui::OpenPopup("Select Asset");
        open_asset_popup = false;
    }
    
    if(ImGui::BeginPopupModal("Select Asset", nullptr, ImGuiWindowFlags_NoMove)) {
        if(!filesystem_cached)
            cacheAssetFilesystem();
        
        ImGui::SetWindowSize(ImVec2(500, 500));
        
        ImGui::BeginChild("assetbox", ImVec2(-1, 400), true);
        
        int column = 0;
        for(auto& [p, a_type] : asset_files) {
            if(current_asset_type == a_type) {
                if(ImGui::ImageButton(get_asset_thumbnail(file::app_domain / p), ImVec2(64, 64))) {
                    on_asset_select(p);
                    ImGui::CloseCurrentPopup();
                }
                
                column++;
                
                if(column != 5)
                    ImGui::SameLine();
                else
                    column = 0;
            }
        }
        
        ImGui::EndChild();
        
        if(ImGui::Button("Cancel"))
            ImGui::CloseCurrentPopup();
        
        ImGui::EndPopup();
    }
}

int CommonEditor::getDefaultX() const {
    return defaultX;
}

int CommonEditor::getDefaultY() const {
    return defaultY;
}

int CommonEditor::getDefaultWidth() const {
    return defaultWidth;
}

int CommonEditor::getDefaultHeight() const {
    return defaultHeight;
}

void CommonEditor::addOpenedFile(std::string path) {
    lastOpenedFiles.push_back(path);
}

void CommonEditor::clearOpenedFiles() {
    lastOpenedFiles.clear();
}

std::vector<std::string> CommonEditor::getOpenedFiles() const {
    return lastOpenedFiles;
}

void CommonEditor::walkObject(Object object, Object) {
    static ImGuiTreeNodeFlags base_flags = ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_OpenOnDoubleClick | ImGuiTreeNodeFlags_SpanAvailWidth;
    
    auto& data = engine->get_scene()->get<Data>(object);
    if(data.editor_object)
        return;
    
    const auto text_color = ImGui::GetStyle().Colors[ImGuiCol_Text];
    const auto active_color = ImGui::GetStyle().Colors[ImGuiCol_ButtonActive];
    
    ImGui::PushStyleColor(ImGuiCol_Text, selected_object == object ? active_color : text_color);
    
    bool is_open = ImGui::TreeNodeEx((void*)object, base_flags, "%s", data.name.c_str());
    
    if(ImGui::IsItemClicked()) {
        if(current_stack != nullptr) {
            auto& command = current_stack->new_command<SelectionCommand>();
            command.editor = this;
            command.old_selection = selected_object;
            command.new_selection = object;
        }
        
        selected_object = object;
    }
    
    if (ImGui::BeginPopupContextItem()) {
        if(ImGui::Button("Delete")) {
            engine->get_scene()->remove_object(object);
            ImGui::CloseCurrentPopup();
        }
        
        if(ImGui::Button("Duplicate")) {
            Object obj = engine->get_scene()->duplicate_object(object);
            engine->get_scene()->get(obj).name += "duplicate";
            
            selected_object = obj;
            
            ImGui::CloseCurrentPopup();
        }
        
        ImGui::EndPopup();
    }
    
    if(is_open) {
        for(auto& child : engine->get_scene()->children_of(object))
            walkObject(child);
        
        ImGui::TreePop();
    }
    
    ImGui::PopStyleColor();
}

void CommonEditor::drawOutline() {
    if(engine->get_scene() != nullptr) {
        ImGui::BeginChild("outlineinner", ImVec2(-1, -1), true);
        
        for(auto& object : engine->get_scene()->get_objects()) {
            if(engine->get_scene()->get(object).parent == NullObject)
                walkObject(object);
        }
        
        ImGui::EndChild();
    } else {
        ImGui::TextDisabled("No scene loaded.");
    }
}

Transform stored_transform;

bool IsItemActiveLastFrame() {
    ImGuiContext& g = *GImGui;
    if (g.ActiveIdPreviousFrame)
        return g.ActiveIdPreviousFrame == g.CurrentWindow->DC.LastItemId;
    
    return false;
}

void CommonEditor::editTransform(Object object, Transform transform) {
    bool is_done_editing = false;
    bool started_edit = false;
    auto changed = ImGui::DragFloat3("Position", transform.position.ptr());
    is_done_editing = ImGui::IsItemDeactivatedAfterEdit();
    started_edit = ImGui::IsItemActive() && !IsItemActiveLastFrame();
    changed |= ImGui::DragQuat("Rotation", &transform.rotation);
    is_done_editing |= ImGui::IsItemDeactivatedAfterEdit();
    started_edit |= ImGui::IsItemActive() && !IsItemActiveLastFrame();
    changed |= ImGui::DragFloat3("Scale", transform.scale.ptr());
    is_done_editing |= ImGui::IsItemDeactivatedAfterEdit();
    started_edit |= ImGui::IsItemActive() && !IsItemActiveLastFrame();
    
    if (started_edit)
        stored_transform = transform;
    
    if(changed)
        engine->get_scene()->get<Transform>(object) = transform;
    
    if (is_done_editing && current_stack != nullptr) {
        auto& command = current_stack->new_command<TransformCommand>();
        command.transformed = object;
        command.new_transform = transform;
        command.old_transform = stored_transform;
    }
}

void CommonEditor::editRenderable(Renderable& mesh) {
    edit_asset("Mesh", mesh.mesh);
    
    for(auto [i, material] : utility::enumerate(mesh.materials)) {
        std::string str = "Material " + std::to_string(i);
        edit_asset(str.c_str(), material);
    }
    
    if(ImGui::Button("Add material"))
        mesh.materials.push_back({});
}

void editLight(Light& light) {
    ImGui::ColorEdit3("Color", light.color.ptr());
    
    ImGui::ComboEnum("Type", &light.type);
    ImGui::DragFloat("Size", &light.size, 0.1f, 0.0f, 1000.0f);
    ImGui::DragFloat("Power", &light.power, 0.5f, 0.0f, 100.0f);
    
    if(light.type == Light::Type::Spot)
        ImGui::DragFloat("Spot Size", &light.spot_size, 0.5f, 40.0f, 56.0f);
    
    ImGui::Checkbox("Enable shadows", &light.enable_shadows);
    ImGui::Checkbox("Use dynamic shadows", &light.use_dynamic_shadows);
}

void editCamera(Camera& camera) {
    ImGui::DragFloat("Field of View", &camera.fov);
}

void editCollision(Collision& collision) {
    ImGui::DragFloat3("Size", collision.size.ptr());
    
    ImGui::Checkbox("Is Trigger", &collision.is_trigger);
    
    if(collision.is_trigger)
        ImGui::InputText("Trigger ID", &collision.trigger_id);
}

void editRigidbody(Rigidbody& rigidbody) {
    ImGui::DragInt("Mass", &rigidbody.mass);
}

void editUI(UI& ui) {
    ImGui::DragInt("Width", &ui.width);
    ImGui::DragInt("Height", &ui.height);
    ImGui::InputText("Path", &ui.ui_path);
    
    if(ImGui::Button("Reload")) {
        ui.screen = engine->load_screen(file::app_domain / ui.ui_path);
        engine->get_renderer()->init_screen(ui.screen);
        ui.screen->extent.width = ui.width;
        ui.screen->extent.height = ui.height;
        ui.screen->calculate_sizes();
    }
}

void editProbe(EnvironmentProbe& probe) {
    ImGui::Checkbox("Is Sized", &probe.is_sized);
    if(probe.is_sized)
        ImGui::DragFloat3("Size", probe.size.ptr());
    
    ImGui::SliderFloat("Intensity", &probe.intensity, 0.0f, 1.0f);
}

template<typename T>
bool componentHeader(Scene& scene, Object& object, const char* name, const bool removable = true) {
    if(!scene.has<T>(object))
        return false;
    
    const auto draw_list = ImGui::GetWindowDrawList();
    const auto window_pos = ImGui::GetCursorScreenPos();
    const auto window_size = ImGui::GetWindowSize();
    const auto state_store = ImGui::GetStateStorage();
    
    auto is_open = state_store->GetBoolRef(ImGui::GetID((std::string(name) + "_open").c_str()), true);
    if(is_open == nullptr)
        return false;
    
    const auto window_padding = ImGui::GetStyle().WindowPadding;
    const auto frame_padding = ImGui::GetStyle().FramePadding;
    const auto widget_color = ImGui::GetStyle().Colors[ImGuiCol_Button];
    const auto text_color = ImGui::GetStyle().Colors[ImGuiCol_Text];
    
    const auto header_start = window_pos;
    const auto header_end = ImVec2(window_pos.x + window_size.x - (window_padding.x * 2), window_pos.y + 25);
    
    const auto button_size = removable ? 25 : 0;
    
    const auto line_height = ImGui::GetTextLineHeight();
    
    draw_list->AddRectFilled(header_start, header_end, ImColor(widget_color), 5.0f);
    draw_list->AddText(ImVec2(header_start.x + frame_padding.x, header_start.y + (25.0f / 2.0f) - (line_height / 2.0f)), ImColor(text_color), name);
    
    ImGui::Dummy(ImVec2(window_size.x - window_pos.x, 25));
    
    ImGui::ItemAdd(ImRect(header_start, ImVec2(header_end.x - button_size, header_end.y)), ImGui::GetID(name));
    
    if (ImGui::IsItemClicked()) {
        *is_open = !(*is_open);
    }
    
    if(removable) {
        draw_list->AddText(ImVec2(header_end.x - (button_size / 2.0f), header_start.y + (25.0f / 2.0f) - (line_height / 2.0f)), ImColor(text_color), "X");
        
        ImGui::ItemAdd(ImRect(ImVec2(header_end.x - button_size, header_start.y), header_end), ImGui::GetID(name));
        
        if (ImGui::IsItemClicked()) {
            // remove
            engine->get_scene()->remove<T>(object);
            return false;
        }
    }
    
    return *is_open;
}

void CommonEditor::drawPropertyEditor() {
    auto scene = engine->get_scene();
    if(scene != nullptr) {
        if(selected_object != NullObject) {
            if(!scene->has<Data>(selected_object)) {
                selected_object = NullObject;
                return;
            }
            
            if(scene->is_object_prefab(selected_object)) {
                ImGui::TextDisabled("Cannot edit prefab, so certain data is uneditable.");
            } else {
                auto& data = scene->get(selected_object);
                
                ImGui::InputText("Name", &data.name);
                
                static std::string stored_name;
                if(ImGui::IsItemActive() && !IsItemActiveLastFrame())
                    stored_name = data.name;
                
                if(ImGui::IsItemDeactivatedAfterEdit() && current_stack != nullptr) {
                    auto& command = current_stack->new_command<RenameCommand>();
                    command.object = selected_object;
                    command.new_name = data.name;
                    command.old_name = stored_name;
                }
                
                std::string preview_value = data.parent == NullObject ? "None" : scene->get(data.parent).name;
                if(ImGui::BeginCombo("Parent", preview_value.c_str())) {
                    for (auto& object : scene->get_objects()) {
                        // dont allow selecting a node to be the parent of itself
                        if (object == selected_object)
                            continue;
                        
                        if (ImGui::Selectable(scene->get(object).name.c_str(), data.parent == object))
                            data.parent = object;
                    }
                    
                    ImGui::Separator();
                    
                    if (ImGui::Selectable("None", data.parent == NullObject))
                        data.parent = NullObject;
                    
                    ImGui::EndCombo();
                }
                
                if (ImGui::Button("Add Component..."))
                    ImGui::OpenPopup("add_popup");
                
                if (ImGui::BeginPopup("add_popup")) {
                    if(ImGui::Selectable("Renderable")) {
                        scene->add<Renderable>(selected_object);
                        ImGui::CloseCurrentPopup();
                    }
                    
                    if(ImGui::Selectable("Light")) {
                        scene->add<Light>(selected_object);
                        ImGui::CloseCurrentPopup();
                    }
                    
                    if(ImGui::Selectable("Camera")) {
                        scene->add<Camera>(selected_object);
                        ImGui::CloseCurrentPopup();
                    }
                    
                    if(ImGui::Selectable("Collision")) {
                        scene->add<Collision>(selected_object);
                        ImGui::CloseCurrentPopup();
                    }
                    
                    if(ImGui::Selectable("Rigidbody")) {
                        scene->add<Rigidbody>(selected_object);
                        ImGui::CloseCurrentPopup();
                    }
                    
                    if(ImGui::Selectable("UI")) {
                        scene->add<UI>(selected_object);
                        ImGui::CloseCurrentPopup();
                    }
                    
                    if(ImGui::Selectable("Environment Probe")) {
                        scene->add<EnvironmentProbe>(selected_object);
                        ImGui::CloseCurrentPopup();
                    }
                    
                    ImGui::EndPopup();
                }
                
                if(componentHeader<Transform>(*scene, selected_object, "Transform", false))
                    editTransform(selected_object, scene->get<Transform>(selected_object));
                
                if(componentHeader<Renderable>(*scene, selected_object, "Renderable"))
                    editRenderable(scene->get<Renderable>(selected_object));
                
                if(componentHeader<Light>(*scene, selected_object, "Light"))
                    editLight(scene->get<Light>(selected_object));
                
                if(componentHeader<Camera>(*scene, selected_object, "Camera"))
                    editCamera(scene->get<Camera>(selected_object));
                
                if(componentHeader<Collision>(*scene, selected_object, "Collision"))
                    editCollision(scene->get<Collision>(selected_object));
                
                if(componentHeader<Rigidbody>(*scene, selected_object, "Rigidbody"))
                    editRigidbody(scene->get<Rigidbody>(selected_object));
                
                if(componentHeader<UI>(*scene, selected_object, "UI"))
                    editUI(scene->get<UI>(selected_object));
                
                if(componentHeader<EnvironmentProbe>(*scene, selected_object, "Environment Probe"))
                    editProbe(scene->get<EnvironmentProbe>(selected_object));
            }
        } else {
            ImGui::TextDisabled("No object selected.");
        }
    } else {
        ImGui::TextDisabled("No scene loaded.");
    }
}

void CommonEditor::createDockArea() {
    ImGuiViewport* viewport = ImGui::GetMainViewport();
    ImGui::SetNextWindowPos(viewport->Pos);
    ImGui::SetNextWindowSize(viewport->Size);
    ImGui::SetNextWindowViewport(viewport->ID);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
    
    ImGuiWindowFlags window_flags = ImGuiWindowFlags_MenuBar | ImGuiWindowFlags_NoDocking |
    ImGuiWindowFlags_NoTitleBar |
    ImGuiWindowFlags_NoCollapse |
    ImGuiWindowFlags_NoResize |
    ImGuiWindowFlags_NoMove |
    ImGuiWindowFlags_NoBringToFrontOnFocus |
    ImGuiWindowFlags_NoNavFocus;
    
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
    ImGui::Begin("dockspace", nullptr, window_flags);
    ImGui::PopStyleVar();
    ImGui::PopStyleVar(2);
    
    ImGuiID dockspace_id = ImGui::GetID("dockspace");
    ImGui::DockSpace(dockspace_id, ImVec2(0.0f, 0.0f), ImGuiDockNodeFlags_PassthruCentralNode);
}

void CommonEditor::drawViewport(Scene* scene) {
    const auto size = ImGui::GetContentRegionAvail();
    const auto real_size = ImVec2(size.x * platform::get_window_dpi(0), size.y * platform::get_window_dpi(0));
    
    if(real_size.x <= 0 || real_size.y <= 0)
        return;
    
    auto window = ImGui::GetCurrentWindowRead();
    auto window_id = window->ID;
    
    if(!viewport_render_targets.count(window_id)) {
        ViewportRenderTarget new_target;
        new_target.target = engine->get_renderer()->allocate_render_target(real_size);
        
        viewport_render_targets[window_id] = new_target;
    }
    
    auto target = &viewport_render_targets[window_id];
    
    if(real_size.x != target->target->extent.width || real_size.y != target->target->extent.height)
        engine->get_renderer()->resize_render_target(*target->target, real_size);
    
    target->scene = scene;
    
    viewport_width = size.x;
    viewport_height = size.y;
    
    const auto mouse_pos = ImGui::GetMousePos();
    const auto real_pos = ImGui::GetCursorScreenPos();
    
    viewport_x = mouse_pos.x - real_pos.x;
    viewport_y = mouse_pos.y - real_pos.y;
    
    ImGui::Image((ImTextureID)target->target->offscreenColorTexture, size);
    
    accepting_viewport_input = ImGui::IsWindowHovered();
    
    if(doing_viewport_input)
        ImGui::SetWindowFocus();
}

void CommonEditor::set_undo_stack(UndoStack *stack) {
    current_stack = stack;
}

bool mesh_readable(const file::Path path) {
    auto file = file::open(path);
    if(!file.has_value()) {
        prism::log::error(System::Renderer, "Failed to load mesh from {}!", path);
        return false;
    }
    
    int version = 0;
    file->read(&version);
    
    return version == 5 || version == 6;
}

bool material_readable(const file::Path path) {
    auto file = file::open(path);
    if(!file.has_value()) {
        prism::log::error(System::Core, "Failed to load material from {}!", path);
        return false;
    }
    
    nlohmann::json j;
    file->read_as_stream() >> j;
    
    return j.count("version") && j["version"] == 2;
}

void cacheAssetFilesystem() {
    asset_files.clear();
    
    auto data_directory = "data";
    for(auto& p : std::filesystem::recursive_directory_iterator(data_directory)) {
        if(p.path().extension() == ".model" && mesh_readable(p.path())) {
            asset_files[std::filesystem::relative(p, data_directory)] = AssetType::Mesh;
        } else if(p.path().extension() == ".material" && material_readable(p.path())) {
            asset_files[std::filesystem::relative(p, data_directory)] = AssetType::Material;
        } else if(p.path().extension() == ".png") {
            asset_files[std::filesystem::relative(p, data_directory)] = AssetType::Texture;
        }
    }
    
    filesystem_cached = true;
}

void CommonEditor::drawAssets() {
    if(!filesystem_cached)
        cacheAssetFilesystem();
    
    const float window_width = ImGui::GetWindowWidth();
    const float item_spacing = ImGui::GetStyle().ItemSpacing.x;
    const float frame_padding = ImGui::GetStyle().FramePadding.x;
    const float window_padding = ImGui::GetStyle().WindowPadding.x;
    
    const float column_width = 64.0f + item_spacing + (frame_padding * 2.0f);
    
    const int max_columns = std::floor((window_width - window_padding) / column_width);
        
    int column = 0;
    for(auto& [p, type] : asset_files) {
        ImGui::PushID(&p);
        
        if(ImGui::ImageButton(get_asset_thumbnail(file::app_domain / p), ImVec2(64, 64)))
            asset_selected(file::app_domain / p, type);
        
        if(ImGui::BeginPopupContextItem()) {
            ImGui::TextDisabled("%s", p.string().c_str());
                        
            ImGui::Separator();
            
            if(ImGui::Button("Regenerate thumbnail")) {
                asset_thumbnails.erase(asset_thumbnails.find((file::app_domain / p).string()));
            }
            
            ImGui::EndPopup();
        }
        
        column++;
        
        if(column >= max_columns) {
            column = 0;
        } else {
            ImGui::SameLine();
        }
        
        ImGui::PopID();
    }
}

GFXTexture* CommonEditor::get_material_preview(Material& material) {
    Scene scene;
    
    auto sphere = scene.add_object();
    scene.add<Renderable>(sphere).mesh = assetm->get<Mesh>(file::app_domain / "models" / "sphere.model");
    scene.get<Renderable>(sphere).materials.push_back(assetm->get<Material>(file::app_domain / material.path)); // we throw away our material handle here :-(
    
    scene.get<Transform>(sphere).rotation = euler_to_quat(Vector3(radians(90.0f), 0, 0));

    return generate_common_preview(scene, Vector3(0, 0, 3));
}

GFXTexture* CommonEditor::get_mesh_preview(Mesh& mesh) {
    Scene scene;
    
    auto mesh_obj = scene.add_object();
    scene.add<Renderable>(mesh_obj).mesh = assetm->get<Mesh>(file::app_domain / mesh.path);
    
    float biggest_component = 0.0f;
    for(const auto& part : scene.get<Renderable>(mesh_obj).mesh->parts) {
        const auto find_biggest_component = [&biggest_component](const Vector3 vec) {
            for(auto& component : vec.data) {
                if(std::fabs(component) > biggest_component)
                    biggest_component = component;
            }
        };
        
        find_biggest_component(part.aabb.min);
        find_biggest_component(part.aabb.max);
        
        scene.get<Renderable>(mesh_obj).materials.push_back(assetm->get<Material>(file::app_domain / "materials" / "Material.material"));
    }
    
    return generate_common_preview(scene, Vector3(biggest_component * 2.0f));
}

GFXTexture* CommonEditor::get_texture_preview(Texture& texture) {
    auto gfx = engine->get_gfx();
    
    GFXTextureCreateInfo texture_create_info = {};
    texture_create_info.label = "Preview of " + texture.path;
    texture_create_info.width = thumbnail_resolution;
    texture_create_info.height = thumbnail_resolution;
    texture_create_info.format = GFXPixelFormat::RGBA8_UNORM;
    texture_create_info.usage = GFXTextureUsage::Attachment | GFXTextureUsage::Sampled;
    
    auto final_texture = gfx->create_texture(texture_create_info);
    
    GFXFramebufferCreateInfo framebuffer_create_info = {};
    framebuffer_create_info.attachments = {final_texture};
    framebuffer_create_info.render_pass = engine->get_renderer()->unorm_render_pass;
    
    auto final_framebuffer = gfx->create_framebuffer(framebuffer_create_info);
    
    auto renderer = engine->get_renderer();

    GFXCommandBuffer* command_buffer = gfx->acquire_command_buffer();
    
    GFXRenderPassBeginInfo begin_info = {};
    begin_info.render_area.extent = {thumbnail_resolution, thumbnail_resolution};
    begin_info.framebuffer = final_framebuffer;
    begin_info.render_pass = renderer->unorm_render_pass;
    
    command_buffer->set_render_pass(begin_info);
    
    Viewport viewport = {};
    viewport.width = thumbnail_resolution;
    viewport.height = thumbnail_resolution;
    
    command_buffer->set_viewport(viewport);
    
    command_buffer->set_graphics_pipeline(renderer->render_to_unorm_texture_pipeline);
    
    command_buffer->bind_texture(texture.handle, 1);
    command_buffer->bind_texture(renderer->dummy_texture, 2);
    command_buffer->bind_texture(renderer->dummy_texture, 3);
    command_buffer->bind_texture(renderer->dummy_texture, 4);
    
    struct PostPushConstants {
        Vector4 viewport;
        Vector4 options;
    } pc;
    pc.options.w = 1.0;
    pc.viewport = Vector4(1.0 / (float)thumbnail_resolution, 1.0 / (float)thumbnail_resolution, thumbnail_resolution, thumbnail_resolution);
    
    command_buffer->set_push_constant(&pc, sizeof(PostPushConstants));
    
    command_buffer->draw(0, 4, 0, 1);
    
    gfx->submit(command_buffer);
    
    return final_texture;
}

GFXTexture* CommonEditor::generate_common_preview(Scene& scene, const Vector3 camera_position) {
    auto gfx = engine->get_gfx();
    
    // setup scene
    scene.reset_environment();
    scene.reset_shadows();
    
    auto camera = scene.add_object();
    scene.add<Camera>(camera);
    
    camera_look_at(scene, camera, camera_position, Vector3(0));
    
    auto light = scene.add_object();
    scene.get<Transform>(light).position = Vector3(5);
    scene.add<Light>(light).type = Light::Type::Sun;
    
    auto probe = scene.add_object();
    scene.add<EnvironmentProbe>(probe).is_sized = false;
    scene.get<Transform>(probe).position = Vector3(3);
    
    engine->update_scene(scene);
    
    scene.get<Camera>(camera).perspective = transform::infinite_perspective(radians(45.0f), 1.0f, 0.1f);
    scene.get<Camera>(camera).view = inverse(scene.get<Transform>(camera).model);
    
    auto renderer = engine->get_renderer();
    
    RenderTarget* target = renderer->allocate_render_target({thumbnail_resolution, thumbnail_resolution});
    
    renderer->shadow_pass->create_scene_resources(scene);
    renderer->scene_capture->create_scene_resources(scene);
    
    // setup render
    GFXTextureCreateInfo texture_create_info = {};
    texture_create_info.width = thumbnail_resolution;
    texture_create_info.height = thumbnail_resolution;
    texture_create_info.format = GFXPixelFormat::RGBA8_UNORM;
    texture_create_info.usage = GFXTextureUsage::Attachment | GFXTextureUsage::Sampled;
    
    auto final_texture = gfx->create_texture(texture_create_info);
    
    texture_create_info = {};
    texture_create_info.width = thumbnail_resolution;
    texture_create_info.height = thumbnail_resolution;
    texture_create_info.format = GFXPixelFormat::RGBA_32F;
    texture_create_info.usage = GFXTextureUsage::Attachment;
    
    auto offscreen_color_texture = gfx->create_texture(texture_create_info);
    
    texture_create_info = {};
    texture_create_info.width = thumbnail_resolution;
    texture_create_info.height = thumbnail_resolution;
    texture_create_info.format = GFXPixelFormat::DEPTH_32F;
    texture_create_info.usage = GFXTextureUsage::Attachment;
    
    auto offscreen_depth_texture = gfx->create_texture(texture_create_info);
    
    GFXFramebufferCreateInfo framebuffer_create_info = {};
    framebuffer_create_info.attachments = {offscreen_color_texture, offscreen_depth_texture};
    framebuffer_create_info.render_pass = renderer->offscreen_render_pass;
    
    auto offscreen_framebuffer = gfx->create_framebuffer(framebuffer_create_info);
    
    framebuffer_create_info = {};
    framebuffer_create_info.attachments = {final_texture};
    framebuffer_create_info.render_pass = renderer->unorm_render_pass;
    
    auto final_framebuffer = gfx->create_framebuffer(framebuffer_create_info);
    
    GFXCommandBuffer* command_buffer = gfx->acquire_command_buffer();
    
    renderer->shadow_pass->render(command_buffer, scene);

    if(render_options.enable_ibl)
        renderer->scene_capture->render(command_buffer, &scene);
    
    GFXRenderPassBeginInfo begin_info = {};
    begin_info.framebuffer = offscreen_framebuffer;
    begin_info.render_pass = renderer->offscreen_render_pass;
    begin_info.render_area.extent = {thumbnail_resolution, thumbnail_resolution};
    
    command_buffer->set_render_pass(begin_info);
    
    prism::renderer::controller_continuity continuity;
    renderer->render_camera(command_buffer, scene, camera, scene.get<Camera>(camera), begin_info.render_area.extent, *target, continuity);
    
    // render post
    begin_info.framebuffer = final_framebuffer;
    begin_info.render_pass = renderer->unorm_render_pass;
    
    command_buffer->set_render_pass(begin_info);
    
    Viewport viewport = {};
    viewport.width = thumbnail_resolution;
    viewport.height = thumbnail_resolution;
    
    command_buffer->set_viewport(viewport);
    
    command_buffer->set_graphics_pipeline(renderer->render_to_unorm_texture_pipeline);
    
    command_buffer->bind_texture(offscreen_color_texture, 1);
    command_buffer->bind_texture(renderer->dummy_texture, 2);
    command_buffer->bind_texture(renderer->dummy_texture, 3);
    command_buffer->bind_texture(renderer->dummy_texture, 4);
    
    struct PostPushConstants {
        Vector4 viewport;
        Vector4 options;
    } pc;
    pc.options.w = 1.0;
    pc.viewport = Vector4(1.0 / (float)thumbnail_resolution, 1.0 / (float)thumbnail_resolution, thumbnail_resolution, thumbnail_resolution);
    
    command_buffer->set_push_constant(&pc, sizeof(PostPushConstants));
    
    command_buffer->draw(0, 4, 0, 1);
    
    gfx->submit(command_buffer);
    
    return final_texture;
}

void CommonEditor::drawConsole() {
    static std::string command_buffer;
    ImGui::InputText("Command", &command_buffer);
    
    ImGui::SameLine();
    
    if(ImGui::Button("Run")) {
        prism::console::parse_and_invoke_command(command_buffer);
        command_buffer.clear();
    }
    
    ImGui::BeginChild("console_output", ImVec2(-1, -1), true);
    
    for(const auto& message : prism::log::stored_output)
        ImGui::TextWrapped("%s", message.c_str());
    
    ImGui::EndChild();
}

void CommonEditor::load_options() {
    std::ifstream i(file::get_writeable_directory() / (id + "options.json"));
    if (i.is_open()) {
        nlohmann::json j;
        i >> j;
        
        defaultX = j["x"];
        defaultY = j["y"];
        defaultWidth = j["width"];
        defaultHeight = j["height"];
        
        for (auto& file : j["files"])
            lastOpenedFiles.push_back(file.get<std::string>());
    }
    else {
        defaultX = -1;
        defaultY = -1;
        defaultWidth = 1280;
        defaultHeight = 720;
    }
}

void CommonEditor::save_options() {
    const auto& [x, y] = platform::get_window_position(0);
    const auto& [width, height] = platform::get_window_size(0);
    
    nlohmann::json j;
    j["x"] = x;
    j["y"] = y;
    j["width"] = width;
    j["height"] = height;
    j["files"] = lastOpenedFiles;
    
    std::ofstream out(file::get_writeable_directory() / (id + "options.json"));
    out << j;
}

void CommonEditor::load_thumbnail_cache() {
    auto thumbnail_cache = file::open("./thumbnail-cache");
    if(thumbnail_cache != std::nullopt) {
        int size;
        thumbnail_cache->read(&size);
        
        for(int i = 0; i < size; i++) {
            std::string filename;
            thumbnail_cache->read_string(filename);
            
            std::vector<uint8_t> image(thumbnail_resolution * thumbnail_resolution * 4);
            thumbnail_cache->read(image.data(), thumbnail_resolution * thumbnail_resolution * 4);
            
            GFXTextureCreateInfo info;
            info.label = "Preview of " + filename;
            info.width = thumbnail_resolution;
            info.height = thumbnail_resolution;
            info.format = GFXPixelFormat::RGBA8_UNORM;
            info.usage = GFXTextureUsage::Sampled;
            
            GFXTexture* texture = engine->get_gfx()->create_texture(info);
            
            engine->get_gfx()->copy_texture(texture, image.data(), image.size());
            
            asset_thumbnails[filename] = texture;
        }
    }
}

void CommonEditor::save_thumbnail_cache() {
    GFXBuffer* thumbnailBuffer = engine->get_gfx()->create_buffer(nullptr, thumbnail_resolution * thumbnail_resolution * 4, false, GFXBufferUsage::Storage);
    
    FILE* file = fopen("thumbnail-cache", "wb");
    if(file == nullptr) {
        prism::log::error(System::Core, "Failed to write thumbnail cache!");
        return;
    }
    
    int size = asset_thumbnails.size();
    fwrite(&size, sizeof(int), 1, file);
    
    for(auto [p, thumbnail] : asset_thumbnails) {
        unsigned int len = strlen(p.c_str());
        fwrite(&len, sizeof(unsigned int), 1, file);
        
        const char* str = p.c_str();
        fwrite(str, sizeof(char) * len, 1, file);
        
        // dump image to memory
        engine->get_gfx()->copy_texture(thumbnail, thumbnailBuffer);
        
        uint8_t* map = reinterpret_cast<uint8_t*>(engine->get_gfx()->get_buffer_contents(thumbnailBuffer));
        
        fwrite(map, thumbnail_resolution * thumbnail_resolution * 4, 1, file);
        
        engine->get_gfx()->release_buffer_contents(thumbnailBuffer, map);
    }
    
    fclose(file);
}
