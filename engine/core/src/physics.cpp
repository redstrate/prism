#include "physics.hpp"

#include <btBulletDynamicsCommon.h>

#include "engine.hpp"
#include "scene.hpp"

Physics::Physics() {
    reset();
}

Physics::~Physics() {}

void Physics::update(float deltaTime) {
    if(engine->get_scene() == nullptr)
        return;
    
    for(auto [obj, rigidbody] : engine->get_scene()->get_all<Rigidbody>()) {     
        if(rigidbody.body != nullptr) {
            if(rigidbody.type == Rigidbody::Type::Dynamic) {
                if(rigidbody.stored_force != Vector3(0.0f)) {
                    rigidbody.body->setLinearVelocity(btVector3(rigidbody.stored_force.x, rigidbody.stored_force.y, rigidbody.stored_force.z));
                    rigidbody.stored_force = Vector3(0.0f);
                }
            }
        }
    }
    
    world->stepSimulation(deltaTime);
    
    for(auto [object, collision] : engine->get_scene()->get_all<Collision>()) {
        if(collision.shape == nullptr && !collision.is_trigger) {
            switch(collision.type) {
                case Collision::Type::Cube:
                    collision.shape = new btBoxShape(btVector3(collision.size.x / 2.0f, collision.size.y / 2.0f, collision.size.z / 2.0f));
                    break;
                case Collision::Type::Capsule:
                    collision.shape = new btCapsuleShape(0.5f, collision.size.y);
                    break;
            }
        }
    }
    
    for(auto [obj, rigidbody] : engine->get_scene()->get_all<Rigidbody>()) {
        auto& transform = engine->get_scene()->get<Transform>(obj);
        
        if(rigidbody.body == nullptr) {
            btTransform t;
            t.setIdentity();
            
            t.setOrigin(btVector3(transform.position.x, transform.position.y, transform.position.z));
            t.setRotation(btQuaternion(transform.rotation.x, transform.rotation.y, transform.rotation.z, transform.rotation.w));

            btDefaultMotionState* motionState = new btDefaultMotionState(t);

            btScalar bodyMass = rigidbody.mass;
            btVector3 bodyInertia;
            
            if(rigidbody.mass != 0)
               engine->get_scene()->get<Collision>(obj).shape->calculateLocalInertia(bodyMass, bodyInertia);

            btRigidBody::btRigidBodyConstructionInfo bodyCI = btRigidBody::btRigidBodyConstructionInfo(bodyMass, motionState, engine->get_scene()->get<Collision>(obj).shape, bodyInertia);
            
            bodyCI.m_friction = rigidbody.friction;

            rigidbody.body = new btRigidBody(bodyCI);
            
            if(!rigidbody.enable_deactivation)
                rigidbody.body->setActivationState(DISABLE_DEACTIVATION);
            
            if(!rigidbody.enable_rotation)
                rigidbody.body->setAngularFactor(btVector3(0, 0, 0));
            
            world->addRigidBody(rigidbody.body);
            
            rigidbody.body->setUserIndex(obj);
        }
        
        if(rigidbody.type == Rigidbody::Type::Dynamic) {
            btTransform trans = rigidbody.body->getWorldTransform();
            transform.position = Vector3(trans.getOrigin().x(), trans.getOrigin().y(), trans.getOrigin().z());
        } else {
            btTransform t;
            t.setIdentity();

            t.setOrigin(btVector3(transform.position.x, transform.position.y, transform.position.z));
            t.setRotation(btQuaternion(transform.rotation.x, transform.rotation.y, transform.rotation.z, transform.rotation.w));

            rigidbody.body->setWorldTransform(t);
        }
    }
}

void Physics::remove_object(Object object) {
    if(engine->get_scene()->has<Rigidbody>(object)) {
        for(int i = 0 ; i < world->getNumCollisionObjects(); i++) {
            auto obj = world->getCollisionObjectArray()[i];
            
            if(obj->getUserIndex() == object) {
                world->removeCollisionObject(obj);
                delete obj;
            }
        }
    }
}

Physics::RayResult Physics::raycast(Vector3 from, Vector3 to) {
    Physics::RayResult result;
    
    btVector3 btFrom(from.x, from.y, from.z);
    btVector3 btTo(to.x, to.y, to.z);
    
    btCollisionWorld::AllHitsRayResultCallback res(btFrom, btTo);

    world->rayTest(btFrom, btTo, res);

    int closestCollisionObject = NullObject;
    float closestHitFraction = 1000.0f;
    
    for(int i = 0; i < res.m_collisionObjects.size(); i++) {
        if(!engine->get_scene()->get<Collision>(res.m_collisionObjects[i]->getUserIndex()).exclude_from_raycast) {
            if(res.m_hitFractions[i] < closestHitFraction) {
                closestCollisionObject = i;
                closestHitFraction = res.m_hitFractions[i];
            }
        }
    }

    if(closestCollisionObject != NullObject) {
        result.hasHit = true;
        
        auto vec = res.m_hitPointWorld[closestCollisionObject];;
        
        result.location = Vector3(vec.x(), vec.y(), vec.z());
    }
    
    return result;
}

void Physics::reset() {
    if(world != nullptr)
        world.reset();
    
    broadphase = std::make_unique<btDbvtBroadphase>();

    collisionConfiguration = std::make_unique<btDefaultCollisionConfiguration>();
    dispatcher = std::make_unique<btCollisionDispatcher>(collisionConfiguration.get());

    solver = std::make_unique<btSequentialImpulseConstraintSolver>();

    world = std::make_unique<btDiscreteDynamicsWorld>(dispatcher.get(), broadphase.get(), solver.get(), collisionConfiguration.get());
    world->setGravity(btVector3(0.0f, -9.8f, 0.0f));
}
