#include "example.hpp"

#include "platform.hpp"
#include "file.hpp"
#include "engine.hpp"
#include "scene.hpp"
#include "asset.hpp"
#include "path.hpp"

void app_main(prism::engine* engine) {
    prism::set_domain_path(prism::domain::app, "data");
    prism::set_domain_path(prism::domain::internal, "{resource_dir}/shaders");

    platform::open_window("Example", {-1, -1, 1280, 720}, WindowFlags::Resizable);
}

void ExampleApp::initialize_render() {
    engine->create_empty_scene();
    auto scene = engine->get_scene();

    auto camera_obj = scene->add_object();
    auto& camera = scene->add<Camera>(camera_obj);
    camera_look_at(*scene, camera_obj, {2, 2, -2}, {0, 0, 0});

    auto sun_obj = scene->add_object();
    auto& sun = scene->add<Light>(sun_obj);
    sun.type = Light::Type::Sun;
    auto& sun_trans = scene->get<Transform>(sun_obj);
    sun_trans.position = {5, 5, 5};

    auto sphere_obj = scene->add_object();
    auto& sphere_render = scene->add<Renderable>(sphere_obj);
    sphere_render.mesh = assetm->get<Mesh>(prism::path("data/models/sphere.model"));
    sphere_render.materials = { assetm->get<Material>(prism::path("data/materials/Material.material")) };

    auto probe_obj = scene->add_object();
    scene->add<EnvironmentProbe>(probe_obj);
}