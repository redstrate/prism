#pragma once

#include "assetptr.hpp"
#include "object.hpp"
#include "quaternion.hpp"
#include "matrix.hpp"

class btCollisionShape;
class btRigidBody;
class Scene;
class Mesh;
class Material;

struct Collision {
    enum class Type {
        Cube,
        Capsule
    } type = Type::Cube;

    Vector3 size;

    bool is_trigger = false;
    std::string trigger_id;
    bool exclude_from_raycast = false;

    bool trigger_entered = false;
    btCollisionShape* shape = nullptr;
};

struct Rigidbody {
    enum class Type {
        Dynamic,
        Kinematic
    } type = Type::Dynamic;

    int mass = 0;
    float friction = 0.5f;

    bool enable_deactivation = true;
    bool enable_rotation = true;

    void add_force(const Vector3 force) {
        stored_force += force;
    }

    btRigidBody* body = nullptr;
    Vector3 stored_force;
};

struct Data {
    std::string name, prefab_path;
    Object parent = NullObject;

    bool editor_object = false;
};

struct Transform {
    Vector3 position, scale = Vector3(1);
    Quaternion rotation;

    Matrix4x4 model;

    Vector3 get_world_position() const {
        return {
            model[3][0],
            model[3][1],
            model[3][2]
        };
    }
};

struct Renderable {
    AssetPtr<Mesh> mesh;
    std::vector<AssetPtr<Material>> materials;
    
    std::vector<Matrix4x4> temp_bone_data;
};

struct Light {
    enum class Type : int{
        Point = 0,
        Spot = 1,
        Sun = 2
    } type = Type::Point;

    Vector3 color = Vector3(1);

    float power = 10.0f;
    float size = 1.0f;
    float spot_size = 40.0f;

    bool enable_shadows = true;
    bool use_dynamic_shadows = false;
};
 
struct Camera {
    float fov = 75.0f;
    float near = 1.0f;
    float exposure = 1.0f;

    Matrix4x4 view, perspective;
};

namespace ui {
    class Screen;
}

struct UI {
    int width = 1920, height = 1080;

    std::string ui_path;
    ui::Screen* screen = nullptr;
};

struct EnvironmentProbe {
    bool is_sized = true;
    Vector3 size = Vector3(10);
    
    float intensity = 1.0;
};
