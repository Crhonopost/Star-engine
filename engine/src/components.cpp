#include <engine/include/ecs/implementations/components.hpp>
#include <iostream>
#include <glm/gtx/norm.hpp>

glm::mat4 Transform::getLocalModelMatrix(){
    rot = glm::normalize(rot);
    glm::mat4 rotationMatrix = glm::mat4_cast(rot);
    
    return glm::translate(glm::mat4(1.0f), pos) * 
           rotationMatrix * 
           glm::scale(glm::mat4(1.0f), scale);
}


void Transform::computeModelMatrix(){
    modelMatrix = getLocalModelMatrix();
    dirty = false;
}
    
void Transform::computeModelMatrix(const glm::mat4& parentGlobalModelMatrix){
    modelMatrix = parentGlobalModelMatrix * getLocalModelMatrix();
    dirty = false;
}


bool Transform::isDirty(){return dirty;}
    
void Transform::setLocalPosition(glm::vec3 position){
    pos = position;
    dirty = true;
}

glm::vec3 Transform::getLocalPosition(){
    return pos;
}
glm::vec3 Transform::getGlobalPosition(){
    return glm::vec3(modelMatrix[3]);
}
    
void Transform::setLocalRotation(glm::vec3 rotationAngles){
    rot = glm::quat(glm::radians(rotationAngles));
    dirty = true;
}

void Transform::setLocalRotation(glm::quat rotationQuat){
    rot = rotationQuat;
    dirty = true;
}

glm::quat Transform::getLocalRotation(){
    return rot;
}

void Transform::rotate(glm::vec3 rotations){
    float qx = sin(rotations.z/2) * cos(rotations.y/2) * cos(rotations.x/2) - cos(rotations.z/2) * sin(rotations.y/2) * sin(rotations.x/2);
    float qy = cos(rotations.z/2) * sin(rotations.y/2) * cos(rotations.x/2) + sin(rotations.z/2) * cos(rotations.y/2) * sin(rotations.x/2);
    float qz = cos(rotations.z/2) * cos(rotations.y/2) * sin(rotations.x/2) - sin(rotations.z/2) * sin(rotations.y/2) * cos(rotations.x/2);
    float qw = cos(rotations.z/2) * cos(rotations.y/2) * cos(rotations.x/2) + sin(rotations.z/2) * sin(rotations.y/2) * sin(rotations.x/2);

    rot *= glm::quat(qx, qy, qz, qw);//glm::quat(glm::radians(rotations));
    dirty = true;
}

glm::vec3 Transform::applyRotation(glm::vec3 vector){
    // glm::mat4 rotationMatrix = glm::mat4_cast(rot);

    glm::vec3 u(rot.x, rot.y, rot.z);
    float s = rot.w;

    return 2.f * glm::dot(u, vector) * u
            + (s * s - glm::dot(u, vector)) * vector
            + 2.f * s * glm::cross(u, vector);


    // glm::vec4 result = rotationMatrix * glm::vec4(vector, 0.0f);
    // return glm::vec3(result);
}

glm::mat4 Transform::getModelMatrix(){
    return modelMatrix;
}

void Transform::translate(glm::vec3 translation){
    pos += translation;
    dirty = true;
}



bool CollisionShape::canSee(CollisionShape &checker, CollisionShape &checked){
    return (checker.mask & checked.layer) != 0;
}

OverlapingShape spherePlaneIntersection(Sphere &sphereA, Transform &transformA, Plane &planeB, Transform &transformB){
    OverlapingShape res;
    glm::vec3 planeToSphere = transformA.getLocalPosition() - transformB.getLocalPosition();
    
    float distToPlane = glm::dot(planeToSphere, planeB.normal);

    if(distToPlane < sphereA.radius){
        res.exist = true;
        res.correctionDepth = abs(distToPlane - sphereA.radius);
        res.normal = -planeB.normal;
        res.position = transformA.getLocalPosition() - sphereA.radius * res.normal;
    }

    return res;
}

OverlapingShape raySphereIntersection(Ray &rayA, Transform &transformA, Sphere &sphereB, Transform &transformB){
    OverlapingShape res;

    glm::vec3 difference = transformB.getLocalPosition() - transformA.getLocalPosition();

    float rSq = sphereB.radius * sphereB.radius;
    float eSq = glm::length(difference);
    eSq *= eSq;

    float a = glm::dot(difference, rayA.ray_direction);

    float bSq = eSq - (a*a);
    float f = sqrt(rSq - bSq);

    if(rSq - (eSq - (a*a)) < 0.0f) return res;

    res.exist = true;
    res.correctionDepth = 0;
    res.normal = glm::normalize(-difference);
    res.position = transformA.getLocalPosition() + (a-f) * rayA.ray_direction;
    return res;
}

OverlapingShape rayPlaneIntersection(Ray &rayA, Transform &transformA, Plane &planeB, Transform &transformB){
    OverlapingShape res;

    glm::vec3 globalPosA = transformA.getGlobalPosition();
    glm::vec3 globalPosB = transformB.getGlobalPosition();

    glm::vec3 planeNormal(glm::normalize(transformB.getModelMatrix() * glm::vec4(planeB.normal, 1)));
    glm::vec3 rayDirection(glm::normalize(transformA.getModelMatrix() * glm::vec4(rayA.ray_direction, 1)));

    float t = glm::dot(globalPosB - globalPosA, planeNormal) / glm::dot(rayDirection, planeNormal);

    if(t <= rayA.length){
        res.exist = true;
        res.position = globalPosA + t * rayDirection;
        res.normal = glm::normalize(globalPosB - globalPosA);
        res.correctionDepth = t;
    }


    return res;
}

OverlapingShape aabbIntersection(Aabb &aabbA, Transform &transformA, Aabb &aabbB, Transform &transformB){
    OverlapingShape res;


    glm::vec3 globalPosA = transformA.getGlobalPosition();
    glm::vec3 globalPosB = transformB.getGlobalPosition();

    glm::vec3 minA = globalPosA - aabbA.diag ;
    glm::vec3 minB = globalPosB - aabbB.diag ;
    glm::vec3 maxA = globalPosA + aabbA.diag ;
    glm::vec3 maxB = globalPosB + aabbB.diag ;

    if( minA.x <= maxB.x &&
        maxA.x >= minB.x &&
        minA.y <= maxB.y &&
        maxA.y >= minB.y &&
        minA.z <= maxB.z &&
        maxA.z >= minB.z
    ){
        res.exist = true;
        res.normal = glm::normalize(globalPosA - globalPosB);
        res.correctionDepth = 0;
        res.position = transformA.getLocalPosition() + (globalPosA - globalPosB) / 2.f;
    }

    return res;
}

OverlapingShape aabbSphereIntersection(Aabb &aabbA, Transform &transformA, Sphere &sphereB, Transform &transformB){
    OverlapingShape res;

    glm::vec3 globalPosA = transformA.getGlobalPosition();
    glm::vec3 globalPosB = transformB.getGlobalPosition();

    glm::vec3 minA = globalPosA - aabbA.diag ;
    glm::vec3 maxA = globalPosA + aabbA.diag ;


    float x = std::max(minA.x, std::min(globalPosB.x, maxA.x));
    float y = std::max(minA.y, std::min(globalPosB.y, maxA.y));
    float z = std::max(minA.z, std::min(globalPosB.z, maxA.z));

    float distance = glm::distance({x,y,z}, globalPosB);

    if(distance < sphereB.radius){
        res.exist = true;
        glm::vec3 direction = glm::normalize(globalPosA - globalPosB); 
        res.normal = direction;
        res.correctionDepth = sphereB.radius - distance;
        res.position = globalPosA + direction * res.correctionDepth;
    }

    return res;
}

bool testOverlap(float minA, float maxA, float minB, float maxB, float& overlap) {
    if (maxA < minB || maxB < minA) {
        return false;
    }
    
    overlap = std::min(maxA, maxB) - std::max(minA, minB);
    return true;
}


void projectObb(const Oobb& oobb, const glm::mat4& modelMatrix, const glm::vec3& axis, float& min, float& max) {
    glm::vec3 axes[3] = {
        glm::vec3(modelMatrix[0]),
        glm::vec3(modelMatrix[1]),
        glm::vec3(modelMatrix[2])
    };
    
    glm::vec3 center = glm::vec3(modelMatrix[3]);
    
    // Calcule le rayon projeté (théorème de Pythagore généralisé)
    float radius = 0.0f;
    for (int i = 0; i < 3; i++) {
        radius += fabs(glm::dot(axis, axes[i])) * oobb.halfExtents[i];
    }
    
    float centerProj = glm::dot(center, axis);
    
    min = centerProj - radius;
    max = centerProj + radius;
}

OverlapingShape oobbIntersection(Oobb &oobbA, Transform &transformA, Oobb &oobbB, Transform &transformB) {
    OverlapingShape res;
    
    glm::mat4 modelA = transformA.getModelMatrix();
    glm::mat4 modelB = transformB.getModelMatrix();
    
    glm::vec3 axesA[3] = {
        glm::normalize(glm::vec3(modelA[0])),
        glm::normalize(glm::vec3(modelA[1])),
        glm::normalize(glm::vec3(modelA[2]))
    };
    
    glm::vec3 axesB[3] = {
        glm::normalize(glm::vec3(modelB[0])),
        glm::normalize(glm::vec3(modelB[1])),
        glm::normalize(glm::vec3(modelB[2]))
    };
    
    std::vector<glm::vec3> testAxes;
    
    for (int i = 0; i < 3; i++) {
        testAxes.push_back(axesA[i]);
    }
    for (int i = 0; i < 3; i++) {
        testAxes.push_back(axesB[i]);
    }
    
    for (int i = 0; i < 3; i++) {
        for (int j = 0; j < 3; j++) {
            glm::vec3 crossAxis = glm::cross(axesA[i], axesB[j]);
            if (glm::length(crossAxis) > 0.0001f) { // Évite les axes nuls
                testAxes.push_back(glm::normalize(crossAxis));
            }
        }
    }
    
    for (const glm::vec3& axis : testAxes) {
        if (glm::length(axis) < 0.0001f) continue; // Ignore les axes nuls
        
        float minA, maxA, minB, maxB;
        projectObb(oobbA, modelA, axis, minA, maxA);
        projectObb(oobbB, modelB, axis, minB, maxB);
        
        float overlap;
        if (!testOverlap(minA, maxA, minB, maxB, overlap)) {
            return res;
        }
        
        // Garde la plus petite pénétration
        if (overlap < res.correctionDepth) {
            res.correctionDepth = overlap;
            res.normal = axis;
        }
    }
    
    res.exist = true;
    
    glm::vec3 centerA = glm::vec3(modelA[3]);
    glm::vec3 centerB = glm::vec3(modelB[3]);
    glm::vec3 dir = centerB - centerA;
    
    if (glm::dot(dir, res.normal) < 0.0f) {
        res.normal = -res.normal;
    }

    res.position = transformA.getLocalPosition() + res.normal * res.correctionDepth;
    
    return res;
}

// OverlapingShape oobbIntersection(Oobb &oobbA, Transform &transformA, Oobb &oobbB, Transform &transformB) {
//     OverlapingShape res;

//     glm::mat4 modelA = transformA.getModelMatrix();
//     glm::mat4 modelB = transformB.getModelMatrix();

//     glm::vec3 axesA[3] = {
//         glm::normalize(glm::vec3(modelA * glm::vec4(1, 0, 0, 0))),
//         glm::normalize(glm::vec3(modelA * glm::vec4(0, 1, 0, 0))),
//         glm::normalize(glm::vec3(modelA * glm::vec4(0, 0, 1, 0)))
//     };

//     glm::vec3 axesB[3] = {
//         glm::normalize(glm::vec3(modelB * glm::vec4(1, 0, 0, 0))),
//         glm::normalize(glm::vec3(modelB * glm::vec4(0, 1, 0, 0))),
//         glm::normalize(glm::vec3(modelB * glm::vec4(0, 0, 1, 0)))
//     };

//     glm::vec3 centerA = glm::vec3(modelA[3]);
//     glm::vec3 centerB = glm::vec3(modelB[3]);

//     glm::vec3 halfExtentsA = oobbA.halfExtents;
//     glm::vec3 halfExtentsB = oobbB.halfExtents;

//     // Liste des 15 axes de séparation
//     glm::vec3 axesToTest[15];
//     int index = 0;

//     for (int i = 0; i < 3; ++i) axesToTest[index++] = axesA[i];
//     for (int i = 0; i < 3; ++i) axesToTest[index++] = axesB[i];
//     for (int i = 0; i < 3; ++i)
//         for (int j = 0; j < 3; ++j)
//             axesToTest[index++] = glm::cross(axesA[i], axesB[j]);

//     for (int i = 0; i < 15; ++i) {
//         if (!testAxis(axesToTest[i], centerA, axesA, halfExtentsA, centerB, axesB, halfExtentsB)) {
//             return res; // Axe séparateur trouvé
//         }
//     }

//     res.exist = true;
//     res.normal = glm::normalize(centerA - centerB );
//     res.correctionDepth = 0;
//     res.position = transformA.getLocalPosition() + (centerB - centerA) / 2.f;

//     return res;
// }

OverlapingShape oobbSphereIntersection(Oobb &oobbA, Transform &transformA, Sphere &sphereB, Transform &transformB){
    OverlapingShape res;

    glm::mat4 invTransfoA = glm::inverse(transformA.getModelMatrix());
    glm::vec4 localSphereCenter = invTransfoA * glm::vec4(transformB.getGlobalPosition(), 1.f);

    glm::vec3 closest;
    closest.x = std::max(-oobbA.halfExtents.x, std::min(localSphereCenter.x, oobbA.halfExtents.x));
    closest.y = std::max(-oobbA.halfExtents.y, std::min(localSphereCenter.y, oobbA.halfExtents.y));
    closest.z = std::max(-oobbA.halfExtents.z, std::min(localSphereCenter.z, oobbA.halfExtents.z));

    glm::vec3 closestGlobal = glm::vec3(transformA.getModelMatrix() * glm::vec4(closest, 1.f));
    // TODO : fix getGlobalPosition !!
    glm::vec3 direction = transformB.getLocalPosition() - closestGlobal;
    float distance = glm::length(direction);

    if (distance > sphereB.radius) return res;

    res.exist = true;
    if (distance > 0.0001f)
        res.normal = -glm::normalize(direction);
    else
        res.normal = glm::vec3(0, 1, 0);

    res.correctionDepth = sphereB.radius - distance;
    res.position = closestGlobal;

    return res;
}

OverlapingShape CollisionShape::intersectionExist(CollisionShape &shapeA, Transform &transformA, CollisionShape &shapeB, Transform &transformB){
    OverlapingShape res;

    // SPHERE
    if(shapeA.shapeType == SPHERE && shapeB.shapeType == SPHERE){
        float radiusSum = shapeA.sphere.radius + shapeB.sphere.radius;
        float distance = glm::length(transformA.getLocalPosition() - transformB.getLocalPosition());
        
        float dist = distance - radiusSum;
        if(dist < 0){
            res.exist = true;
            res.correctionDepth = -dist;
            res.normal = glm::normalize(transformA.getLocalPosition() - transformB.getLocalPosition());
            res.position = transformA.getLocalPosition() - shapeA.sphere.radius * res.normal;
        }
    } 
    
    // PLANE
    else if(shapeA.shapeType == PLANE && shapeB.shapeType == SPHERE){
        return spherePlaneIntersection(shapeB.sphere, transformB, shapeA.plane, transformA);
    } else if(shapeA.shapeType == SPHERE && shapeB.shapeType == PLANE){
        return spherePlaneIntersection(shapeA.sphere, transformA, shapeB.plane, transformB);
    } 
    
    // RAY
    else if(shapeA.shapeType == RAY && shapeB.shapeType == SPHERE){
        return raySphereIntersection(shapeA.ray, transformA, shapeB.sphere, transformB);
    } else if(shapeA.shapeType == SPHERE && shapeB.shapeType == RAY){
        return raySphereIntersection(shapeB.ray, transformB, shapeA.sphere, transformA);
    } else if(shapeA.shapeType == RAY && shapeB.shapeType == PLANE){
        return rayPlaneIntersection(shapeA.ray, transformA, shapeB.plane, transformB);
    } else if(shapeA.shapeType == PLANE && shapeB.shapeType == RAY){
        return rayPlaneIntersection(shapeB.ray, transformB, shapeA.plane, transformA);
    }
    
    // AABB
    else if(shapeA.shapeType == AABB && shapeB.shapeType == AABB){
        return aabbIntersection(shapeA.aabb, transformA, shapeB.aabb, transformB);
    } else if(shapeA.shapeType == AABB && shapeB.shapeType == SPHERE){
        return aabbSphereIntersection(shapeA.aabb, transformA, shapeB.sphere, transformB);
    } else if(shapeA.shapeType == SPHERE && shapeB.shapeType == AABB){
        return aabbSphereIntersection(shapeB.aabb, transformB, shapeA.sphere, transformA);
    } 
    
    // OOBB
    else if(shapeA.shapeType == OOBB && shapeB.shapeType == OOBB){
        return oobbIntersection(shapeA.oobb, transformA, shapeB.oobb, transformB);
    } else if(shapeA.shapeType == OOBB && shapeB.shapeType == SPHERE){
        return oobbSphereIntersection(shapeA.oobb, transformA, shapeB.sphere, transformB);
    } else  if(shapeA.shapeType == SPHERE && shapeB.shapeType == OOBB){
        return oobbSphereIntersection(shapeB.oobb, transformB, shapeA.sphere, transformA);
    }

    return res;
}

uint16_t CollisionShape::ENV_LAYER = 1 << 0;
uint16_t CollisionShape::PLAYER_LAYER = 1 << 1;
uint16_t CollisionShape::GRAVITY_SENSITIVE_LAYER = 1 << 2;
