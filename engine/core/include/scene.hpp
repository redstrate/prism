#pragma once

#include <vector>
#include <string>
#include <array>
#include <functional>

#if !defined(PLATFORM_IOS) && !defined(PLATFORM_TVOS) && !defined(PLATFORM_WINDOWS)
#include <sol.hpp>
#endif

#include <nlohmann/json.hpp>

#include "vector.hpp"
#include "matrix.hpp"
#include "quaternion.hpp"
#include "utility.hpp"
#include "transform.hpp"
#include "object.hpp"
#include "asset.hpp"
#include "components.hpp"
#include "aabb.hpp"
#include "plane.hpp"

template<class Component>
using Pool = std::unordered_map<Object, Component>;

template<class... Components>
class ObjectComponents : Pool<Components>... {
public:
    /** Adds a new object.
     @param parent The object to parent the new object to. Default is null.
     @return The newly created object. Will not be null.
     */
    Object add_object(const Object parent = NullObject) {
        const Object new_index = make_unique_index();

        add<Data>(new_index).parent = parent;
        add<Transform>(new_index);

        _objects.push_back(new_index);

        return new_index;
    }
    
    /// Adds a new object but with a manually specified id.
    Object add_object_by_id(const Object id) {
        add<Data>(id);
        add<Transform>(id);

        _objects.push_back(id);

        return id;
    }

    /// Remove an object.
    void remove_object(const Object obj) {
        recurse_remove(obj);
    }

    /// Find an object by name.
    Object find_object(const std::string_view name) const {
        for(auto& obj : _objects) {
            if(get<Data>(obj).name == name)
                return obj;
        }

        return NullObject;
    }

    /// Check whether or not an object exists.
    bool object_exists(const std::string_view name) const {
        return find_object(name) != NullObject;
    }

    /// Check if the object originated from a prefab. This works even on children of a root prefab object.
    bool is_object_prefab(const Object obj) const {
        bool is_prefab = false;
        check_prefab_parent(obj, is_prefab);

        return is_prefab;
    }

    /// Duplicates an object and all of it's components and data.
    Object duplicate_object(const Object original_object) {
        const Object duplicate_object = make_unique_index();

        (add_duplicate_component<Components>(original_object, duplicate_object), ...);

        _objects.push_back(duplicate_object);

        return duplicate_object;
    }

    /// Returns all of the children of an object, with optional recursion.
    std::vector<Object> children_of(const Object obj, const bool recursive = false) const {
        std::vector<Object> vec;

        if(recursive) {
            recurse_children(vec, obj);
        } else {
            for(auto& o : _objects) {
                if(get(o).parent == obj)
                    vec.push_back(o);
            }
        }

        return vec;
    }

    /// Adds a component.
    template<class Component>
    Component& add(const Object object) {
        return Pool<Component>::emplace(object, Component()).first->second;
    }

    /// Returns a component.
    template<class Component = Data>
    Component& get(const Object object) {
        return Pool<Component>::at(object);
    }
    
    /// Returns a component.
    template<class Component = Data>
    Component get(const Object object) const {
        return Pool<Component>::at(object);
    }

    /// Checks whether or not an object has a certain component.
    template<class Component>
    bool has(const Object object) const {
        return Pool<Component>::count(object);
    }

    /// Removes a component from an object. Is a no-op if the component wasn't attached.
    template<class Component>
    bool remove(const Object object) {
        if(has<Component>(object)) {
            Pool<Component>::erase(object);
            return true;
        } else {
            return false;
        }
    }

    /// Returns all instances of a component.
    template<class Component>
    std::vector<std::tuple<Object, Component&>> get_all() {
        std::vector<std::tuple<Object, Component&>> comps;

        for(auto it = Pool<Component>::begin(); it != Pool<Component>::end(); it++)
            comps.emplace_back(it->first, it->second);

        return comps;
    }

    /// Returns all objects.
    std::vector<Object> get_objects() const {
        return _objects;
    }

    /// Callback function when an object is removed.
    std::function<void(Object)> on_remove;

private:
    Object make_unique_index() {
        return _last_index++;
    }

    void recurse_remove(Object obj) {
        for(auto& o : children_of(obj))
            recurse_remove(o);

        utility::erase(_objects, obj);

        on_remove(obj);

        (remove<Components>(obj), ...);
    }

    void check_prefab_parent(Object obj, bool& p) const {
        if(!get(obj).prefab_path.empty())
            p = true;

        if(get(obj).parent != NullObject)
            check_prefab_parent(get(obj).parent, p);
    }

    template<class Component>
    void add_duplicate_component(const Object from, const Object to) {
        if(Pool<Component>::count(from))
            Pool<Component>::emplace(to, Pool<Component>::at(from));
    }

    void recurse_children(std::vector<Object>& vec, Object obj) const {
        for(const auto o : _objects) {
            if(get(o).parent == obj) {
                vec.push_back(o);

                recurse_children(vec, o);
            }
        }
    }

    Object _last_index = 1;

    std::vector<Object> _objects;
};

const int max_spot_shadows = 4;
const int max_point_shadows = 4;
const int max_environment_probes = 4;

class GFXFramebuffer;

/// Represents a scene consisting of Objects with varying Components.
class Scene : public ObjectComponents<Data, Transform, Renderable, Light, Camera, Collision, Rigidbody, UI, EnvironmentProbe> {
public:
    /// If loaded from disk, the path to the scene file this originated from.
    std::string path;
    
    /// Invalidates the current shadow data for all shadowed lights. This may cause stuttering while the data is regenerated.
    void reset_shadows() {
        sun_light_dirty = true;
        
        for(auto& point_dirty : point_light_dirty)
            point_dirty = true;
        
        for(auto& spot_dirty : spot_light_dirty)
            spot_dirty = true;
    }
    
    /// Invalidates the current environment probe data. This may cause stuttering while the data is regenerated.
    void reset_environment() {
        for(auto& probe_dirty : environment_dirty)
            probe_dirty = true;
    }
    
    // shadow
    GFXTexture* depthTexture = nullptr;
    GFXFramebuffer* framebuffer = nullptr;
    
    Matrix4x4 lightSpace;
    Matrix4x4 spotLightSpaces[max_spot_shadows];
    
    GFXTexture* pointLightArray = nullptr;
    GFXTexture* spotLightArray = nullptr;
    
    bool sun_light_dirty = false;
    std::array<bool, max_point_shadows> point_light_dirty;
    std::array<bool, max_spot_shadows> spot_light_dirty;
    
    // environment
    std::array<bool, max_environment_probes> environment_dirty;

    GFXTexture *irradianceCubeArray = nullptr, *prefilteredCubeArray = nullptr;
    
    // script
    std::string script_path;
    
#if !defined(PLATFORM_IOS) && !defined(PLATFORM_TVOS) && !defined(PLATFORM_WINDOWS)
    sol::environment env;
#endif
};

/** Positions and rotates a camera to look at a target from a position.
 @param scene The scene that the camera exists in.
 @param cam The camera object.
 @param pos The position the camera will be viewing the target from.
 @param target The position to be centered in the camera's view.
 @note Although this is a look at function, it applies no special attribute to the camera and simply changes it's position and rotation.
 */
inline void camera_look_at(Scene& scene, Object cam, Vector3 pos, Vector3 target) {
    scene.get<Transform>(cam).position = pos;
    scene.get<Transform>(cam).rotation = transform::quat_look_at(pos, target, Vector3(0, 1, 0));
}

Object load_object(Scene& scene, const nlohmann::json obj);
nlohmann::json save_object(Object obj);
