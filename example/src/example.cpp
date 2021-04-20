#include "example.hpp"

#include "platform.hpp"
#include "file.hpp"
#include "engine.hpp"
#include "scene.hpp"

void app_main(prism::engine* engine) {
    file::set_domain_path(file::Domain::App, "data");
    file::set_domain_path(file::Domain::Internal, "{resource_dir}/shaders");

    platform::open_window("Example", {-1, -1, 1280, 720}, WindowFlags::Resizable);
}

void ExampleApp::initialize_render() {
    engine->create_empty_scene();
    auto scene = engine->get_scene();

    auto camera_obj = scene->add_object();
    auto& camera = scene->add<Camera>(camera_obj);
    auto& camera_trans = scene->get<Transform>(camera_obj);
    camera_trans.position.z = -3;

    auto sun_obj = scene->add_object();
    auto& sun = scene->add<Light>(sun_obj);
    sun.type = Light::Type::Sun;
    auto& sun_trans = scene->get<Transform>(sun_obj);
    sun_trans.position = {5, 5, 5};
}