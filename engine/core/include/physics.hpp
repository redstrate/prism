#pragma once

#include <memory>

#include "vector.hpp"
#include "object.hpp"

class btBroadphaseInterface;
class btDefaultCollisionConfiguration;
class btCollisionDispatcher;
class btSequentialImpulseConstraintSolver;
class btDiscreteDynamicsWorld;

class Physics {
public:
    Physics();
    ~Physics();

    void update(float deltaTime);

    void reset();
    void remove_object(Object obj);

    struct RayResult {
        bool hasHit;
        Vector3 location;
    };

    RayResult raycast(Vector3 from, Vector3 to);

private:
    std::unique_ptr<btBroadphaseInterface> broadphase;
    std::unique_ptr<btDefaultCollisionConfiguration> collisionConfiguration;
    std::unique_ptr<btCollisionDispatcher> dispatcher;
    std::unique_ptr<btSequentialImpulseConstraintSolver> solver;
    std::unique_ptr<btDiscreteDynamicsWorld> world;
};
