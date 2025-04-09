#include <engine/include/ecs/implementations/systems.hpp>
#include <engine/include/ecs/ecsManager.hpp>
#include <iostream>

const float G = 8.1f;

std::vector<IntersectionInfo> detectedCollisions;

void PhysicSystem::narrowPhase(){
    detectedCollisions.clear();
    for(auto &entity: mEntities){
        ecs.GetComponent<CollisionShape>(entity).isColliding = false;
    }

    for(auto itA=mEntities.begin(); itA != mEntities.end(); itA++){
        const auto &entityA = *itA;
        auto& transformA = ecs.GetComponent<Transform>(entityA);
        auto& shapeA = ecs.GetComponent<CollisionShape>(entityA);

        auto itB = itA;
        itB ++;
        for(; itB != mEntities.end(); itB++){
            const auto &entityB = *itB;
            auto& transformB = ecs.GetComponent<Transform>(entityB);
            auto& shapeB = ecs.GetComponent<CollisionShape>(entityB);

            IntersectionInfo collision = CollisionShape::intersectionExist(shapeA, transformA, shapeB, transformB);

            if(collision.exist){
                RigidBody &rbA = ecs.GetComponent<RigidBody>(entityA);
                RigidBody &rbB = ecs.GetComponent<RigidBody>(entityB);

                glm::vec3 relativeVelocity = rbA.velocity - rbB.velocity;
                float velAlongNormal = glm::dot(relativeVelocity, collision.normal);
                
                shapeA.isColliding = true;
                shapeB.isColliding = true;
                collision.tA = &transformA;
                collision.tB = &transformB;
                collision.rbA = &rbA;
                collision.rbB = &rbB;
                detectedCollisions.push_back(collision);


                float e = std::min(rbA.restitutionCoef, rbB.restitutionCoef);
                float invMassA = 1.0f / rbA.weight;
                float invMassB = 1.0f / rbB.weight;

                
                float j = -(1 + e) * velAlongNormal / (invMassA + invMassB);
                glm::vec3 impulse = j * collision.normal;

                rbA.velocity += impulse * invMassA;
                rbB.velocity -= impulse * invMassB;

                // friction
                glm::vec3 tangent = relativeVelocity - velAlongNormal * collision.normal;
                if (glm::length(tangent) > 0.0001f)
                    tangent = glm::normalize(tangent);
                else
                    tangent = glm::vec3(0.0f);


                float frictionCoef = std::sqrt(rbA.frictionCoef * rbB.frictionCoef);

           
                float jt = -glm::dot(relativeVelocity, tangent) / (invMassA + invMassB);
                jt = glm::clamp(jt, -j, j);

                glm::vec3 frictionImpulse = jt * tangent;

                rbA.velocity -= frictionImpulse * invMassA;
                rbB.velocity += frictionImpulse * invMassB;
            }
        }
    }
}


void PhysicSystem::solver(){
    for(auto collision: detectedCollisions){
        if(collision.rbA->isStatic){
            collision.tB->translate(collision.normal * collision.correctionDepth);
        } else if(collision.rbB->isStatic){
            collision.tA->translate(-collision.normal * collision.correctionDepth);
        } else {
            collision.tB->translate(collision.normal * collision.correctionDepth * 0.5f);
            collision.tA->translate(-collision.normal * collision.correctionDepth * 0.5f);
        }
    }
}


void PhysicSystem::update(float deltaTime){
    for(auto &entity: mEntities){
        auto& rigidBody = ecs.GetComponent<RigidBody>(entity);
        auto& transform = ecs.GetComponent<Transform>(entity);
        auto& shape = ecs.GetComponent<CollisionShape>(entity);

        if(rigidBody.isStatic){
            rigidBody.velocity = glm::vec3(0,0,0);
        } else {
            float acceleration = G * rigidBody.weight;
            rigidBody.velocity.y -= acceleration * deltaTime;
    
            transform.translate(rigidBody.velocity * deltaTime);
        }
    }
    
    narrowPhase();

    solver();
}
