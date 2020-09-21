#include "scene.hpp"

#include "json_conversions.hpp"
#include "file.hpp"
#include "engine.hpp"
#include "transform.hpp"
#include "asset.hpp"

void camera_look_at(Scene& scene, Object cam, Vector3 pos, Vector3 target) {
    scene.get<Transform>(cam).position = pos;
    scene.get<Transform>(cam).rotation = transform::quat_look_at(pos, target, Vector3(0, 1, 0));
}

void load_transform_component(nlohmann::json j, Transform& t) {
    t.position = j["position"];
    t.scale = j["scale"];
    t.rotation = j["rotation"];
}

void load_renderable_component(nlohmann::json j, Renderable& t) {
    if(j.contains("path"))
        t.mesh = assetm->get<Mesh>(file::app_domain / j["path"].get<std::string_view>());
    
    for(auto& material : j["materials"])
        t.materials.push_back(assetm->get<Material>(file::app_domain / material.get<std::string_view>()));
}

void load_camera_component(nlohmann::json j, Camera& camera) {
    if(j.contains("fov"))
        camera.fov = j["fov"];
}

void load_light_component(nlohmann::json j, Light& light) {
    light.color = j["color"];
    light.power = j["power"];
    light.type = j["type"];
    
    if(j.count("size") && !j.count("spot_size"))
        light.size = j["size"];
    else if(j.count("spot_size")) {
        light.size = j["size"];
        light.spot_size = j["spot_size"];
    }
    
    if(j.count("enable_shadows")) {
        light.enable_shadows = j["enable_shadows"];
        light.use_dynamic_shadows = j["use_dynamic_shadows"];
    }
}

void load_collision_component(nlohmann::json j, Collision& collision) {
    collision.type = j["type"];
    collision.size = j["size"];

    if(j.contains("is_trigger")) {
        collision.is_trigger = j["is_trigger"];

        if(collision.is_trigger)
            collision.trigger_id = j["trigger_id"];
    }
}

void load_rigidbody_component(nlohmann::json j, Rigidbody& rigidbody) {
    rigidbody.type = j["type"];
    rigidbody.mass = j["mass"];
}

void load_ui_component(nlohmann::json j, UI& ui) {
    ui.width = j["width"];
    ui.height = j["height"];
    ui.ui_path = j["path"];
}

void load_probe_component(nlohmann::json j, EnvironmentProbe& probe) {
    if(j.contains("size"))
        probe.size = j["size"];
    
    if(j.contains("is_sized"))
        probe.is_sized = j["is_sized"];
    
    if(j.contains("intensity"))
        probe.intensity = j["intensity"];
}

Object load_object(Scene& scene, const nlohmann::json obj) {
    Object o = scene.add_object();

    auto& data = scene.get(o);
    data.name = obj["name"];

    load_transform_component(obj["transform"], scene.get<Transform>(o));

    if(obj.contains("renderable"))
        load_renderable_component(obj["renderable"], scene.add<Renderable>(o));

    if(obj.contains("light"))
        load_light_component(obj["light"], scene.add<Light>(o));

    if(obj.contains("camera"))
        load_camera_component(obj["camera"], scene.add<Camera>(o));

    if(obj.contains("collision"))
        load_collision_component(obj["collision"], scene.add<Collision>(o));

    if(obj.contains("rigidbody"))
        load_rigidbody_component(obj["rigidbody"], scene.add<Rigidbody>(o));
    
    if(obj.contains("ui"))
        load_ui_component(obj["ui"], scene.add<UI>(o));

    if(obj.contains("environment_probe"))
        load_probe_component(obj["environment_probe"], scene.add<EnvironmentProbe>(o));
    
    return o;
}

void save_transform_component(nlohmann::json& j, const Transform& t) {
    j["position"] = t.position;
    j["scale"] = t.scale;
    j["rotation"] = t.rotation;
}

void save_renderable_component(nlohmann::json& j, const Renderable &mesh) {
    if(mesh.mesh)
        j["path"] = mesh.mesh->path;
    
    for(auto& material : mesh.materials) {
        if(material)
            j["materials"].push_back(material->path);
    }
}

void save_camera_component(nlohmann::json& j, const Camera& camera) {
    j["fov"] = camera.fov;
}

void save_light_component(nlohmann::json& j, const Light& light) {
    j["color"] = light.color;
    j["power"] = light.power;
    j["type"] = light.type;
    j["size"] = light.size;
    j["spot_size"] = light.spot_size;
    j["enable_shadows"] = light.enable_shadows;
    j["use_dynamic_shadows"] = light.use_dynamic_shadows;
}

void save_collision_component(nlohmann::json& j, const Collision& collision) {
    j["type"] = collision.type;
    j["size"] = collision.size;

    j["is_trigger"] = collision.is_trigger;
    if(collision.is_trigger)
        j["trigger_id"] = collision.trigger_id;
}

void save_rigidbody_component(nlohmann::json& j, const Rigidbody& rigidbody) {
    j["type"] = rigidbody.type;
    j["mass"] = rigidbody.mass;
}

void save_ui_component(nlohmann::json& j, const UI& ui) {
    j["width"] = ui.width;
    j["height"] = ui.height;
    j["path"] = ui.ui_path;
}

void save_probe_component(nlohmann::json& j, const EnvironmentProbe& probe) {
    j["size"] = probe.size;
    j["is_sized"] = probe.is_sized;
    j["intensity"] = probe.intensity;
}

nlohmann::json save_object(Object obj) {
    nlohmann::json j;

    auto& data = engine->get_scene()->get(obj);

    j["name"] = data.name;

    if(data.parent != NullObject)
        j["parent"] = engine->get_scene()->get(data.parent).name;

    save_transform_component(j["transform"], engine->get_scene()->get<Transform>(obj));

    auto scene = engine->get_scene();

    if(scene->has<Renderable>(obj))
        save_renderable_component(j["renderable"], scene->get<Renderable>(obj));

    if(scene->has<Light>(obj))
        save_light_component(j["light"], scene->get<Light>(obj));

    if(scene->has<Camera>(obj))
        save_camera_component(j["camera"], scene->get<Camera>(obj));

    if(scene->has<Collision>(obj))
        save_collision_component(j["collision"], scene->get<Collision>(obj));

    if(scene->has<Rigidbody>(obj))
        save_rigidbody_component(j["rigidbody"], scene->get<Rigidbody>(obj));
    
    if(scene->has<UI>(obj))
        save_ui_component(j["ui"], scene->get<UI>(obj));
    
    if(scene->has<EnvironmentProbe>(obj))
        save_probe_component(j["environment_probe"], scene->get<EnvironmentProbe>(obj));
    
    return j;
}
