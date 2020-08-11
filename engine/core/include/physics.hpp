#pragma once

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Weverything"

#if defined(PLATFORM_MACOS) || defined(PLATFORM_IOS) || defined(PLATFORM_TVOS)
#include <btBulletDynamicsCommon.h>
#else
#include <bullet/btBulletDynamicsCommon.h>
#endif
#include <memory>
#pragma clang diagnostic pop

#include "vector.hpp"
#include "object.hpp"

class Physics {
public:
    Physics();

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
