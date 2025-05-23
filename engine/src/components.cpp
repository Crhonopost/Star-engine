#include <engine/include/ecs/implementations/components.hpp>
#include <iostream>
#include <glm/gtx/norm.hpp>


void Drawable::draw(float renderDistance){
    if (switchDistance > 0 && renderDistance > switchDistance) {
        lodLower->draw(renderDistance);
        return;
    }


    glBindVertexArray(VAO);
    
    glDrawElements(
                GL_TRIANGLES,      // mode
                indexCount,
                GL_UNSIGNED_SHORT,   // type
                (void*)0           // element array buffer offset
                );

    glBindVertexArray(0);
}

void Drawable::init(std::vector<Vertex> &vertices, std::vector<short unsigned int> &indices){
    glGenVertexArrays(1,&VAO);
    glGenBuffers(1, &VBO);
    glGenBuffers(1, &EBO);

    glBindVertexArray(VAO);
    
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(Vertex), vertices.data(), GL_STATIC_DRAW);
    
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(unsigned short), &indices[0], GL_STATIC_DRAW);
    
    int vertexSize = 12 * sizeof(float) + 4 * sizeof(int);

    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, vertexSize, (void*)0);

    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, vertexSize, (void*)(3 * sizeof(float)));

    glEnableVertexAttribArray(2);
    glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, vertexSize, (void*)(5 * sizeof(float)));

    glEnableVertexAttribArray(3);
    glVertexAttribPointer(3, 4, GL_FLOAT, GL_FALSE, vertexSize, (void*)(8 * sizeof(float)));

    glEnableVertexAttribArray(4);
    glVertexAttribIPointer(4, 4, GL_INT, vertexSize, (void*)(12 * sizeof(float)));


    glBindVertexArray(0);
}


Material::Material(){
    albedoTex = &Texture::emptyTexture;
    normalTex = &Texture::emptyTexture;
    metallicTex = &Texture::emptyTexture;
    roughnessTex = &Texture::emptyTexture;
    aoTex = &Texture::emptyTexture;
}

CustomProgram::CustomProgram(Program *progPtr): Component(){
    programPtr = progPtr;
}

glm::mat4 Transform::getLocalModelMatrix(){
    glm::mat4 R = glm::toMat4(rotationQuat);
    return glm::translate(glm::mat4(1.0f), pos)
         * R
         * glm::scale(glm::mat4(1.0f), scale);
}



void Transform::computeModelMatrix(){
    modelMatrix = getLocalModelMatrix();
    dirty = false;

    if(eulerStorageDirty) eulerRotStorage = getLocalRotation();
    eulerStorageDirty = false;
}

void Transform::computeModelMatrix(const glm::mat4& parentGlobalModelMatrix){
    modelMatrix = parentGlobalModelMatrix * getLocalModelMatrix();
    dirty = false;

    if(eulerStorageDirty) eulerRotStorage = getLocalRotation();
    eulerStorageDirty = false;
}


bool Transform::isDirty(){return dirty;}
    
void Transform::setLocalPosition(glm::vec3 position){
    pos = position;
    dirty = true;
}

glm::vec3 Transform::getLocalPosition(){
    return pos;
}

void Transform::setScale(glm::vec3 value){
    scale = value;
    dirty = true;
}

glm::vec3 Transform::getGlobalPosition() {
    return glm::vec3(modelMatrix[3]);
}
    
void Transform::setLocalRotation(glm::vec3 rotationAngles){
    const glm::mat4 transformX = glm::rotate(glm::mat4(1.0f),
            glm::radians(rotationAngles.x),
            glm::vec3(1.0f, 0.0f, 0.0f));
    const glm::mat4 transformY = glm::rotate(glm::mat4(1.0f),
            glm::radians(rotationAngles.y),
            glm::vec3(0.0f, 1.0f, 0.0f));
    const glm::mat4 transformZ = glm::rotate(glm::mat4(1.0f),
            glm::radians(rotationAngles.z),
            glm::vec3(0.0f, 0.0f, 1.0f));
    
    glm::mat4 rotationMatrix;
    switch (rotationOrder)
    {
        case YXZ:
        rotationMatrix = transformY * transformX * transformZ;
        break;
        case XYZ:
        rotationMatrix = transformX * transformY * transformZ;
        break;
        case ZYX:
        rotationMatrix = transformZ * transformY * transformX;
        break;
    }

    rotationQuat = glm::toQuat(rotationMatrix);
    dirty = true;
}

// void Transform::setLocalRotation(glm::quat rotationQuat){
//     eulerRot = glm::degrees(glm::eulerAngles(rotationQuat)); // conversion radians → degrés
//     dirty = true;
// }
void Transform::setLocalRotation(const glm::quat &q) {
    rotationQuat = glm::normalize(q);
    dirty = true;
}

glm::vec3 Transform::getLocalRotation(){
    return glm::degrees(glm::eulerAngles(rotationQuat));
}

// void Transform::rotate(glm::vec3 rotations){
//     eulerRot += rotations;
//     dirty = true;
// }
void Transform::rotate(glm::vec3 rotations){
    glm::vec3 radians = glm::radians(rotations);
    glm::quat dq = glm::quat(radians);  
    rotationQuat = glm::normalize(dq * rotationQuat);
    dirty = true;
}

glm::vec3 Transform::applyRotation(glm::vec3 vector){
    // glm::vec4 result = glm::toMat4(rotationQuat) * glm::vec4(vector, 0.0f); // vecteur direction, w = 0
    glm::mat3 rotationMatrix = glm::mat3(modelMatrix);
    return rotationMatrix * vector;
    // return rotationQuat * vector;
}

glm::mat4 Transform::getModelMatrix(){
    return modelMatrix;
}

void Transform::translate(glm::vec3 translation){
    pos += translation;
    dirty = true;
}
void Transform::setLocalScale(const glm::vec3 &s) {
    scale = s;
    dirty = true;
}

glm::vec3 Transform::getLocalScale() const {
    return scale;
}

void Transform::changeScale(const glm::vec3 &factor) {
    scale *= factor;
    dirty = true;
}


bool CollisionShape::canSee(CollisionShape &checker, CollisionShape &checked){
    return (checker.mask & checked.layer) != 0;
}

OverlapingShape spherePlaneIntersection(Sphere &sphereA, Transform &transformA, Plane &planeB, Transform &transformB){
    OverlapingShape res;
    
    
    glm::vec3 globalPlaneNormal = glm::normalize(transformB.applyRotation(planeB.normal));
    glm::vec3 globalSpherePos = transformA.getGlobalPosition();
    glm::vec3 globalPlanePos = transformB.getGlobalPosition();

    float distanceFromPlane = glm::dot(globalPlaneNormal, globalSpherePos - globalPlanePos);

    if(distanceFromPlane < 0.0f) return res;

    if(distanceFromPlane < sphereA.radius){
        res.exist = true;
        res.correctionDepth = abs(distanceFromPlane - sphereA.radius);
        res.normal = -globalPlaneNormal;
        res.position = transformA.getGlobalPosition() - sphereA.radius * res.normal;
    }

    return res;
}

float raycast(glm::vec3 rayOrigin, glm::vec3 rayDirection, glm::vec3 spherePosition, float sphereRadius) {
    
    glm::vec3 diff = rayOrigin - spherePosition;
    float a = glm::dot(rayDirection, rayDirection);
    float b = 2.0 * glm::dot(diff, rayDirection);
    float c = glm::dot(diff, diff) - sphereRadius * sphereRadius;
    float discriminant = b*b - 4.f*a*c;

    if(discriminant > 0){
        float sqrtDis = sqrt(discriminant);

        float t1 = (-b -sqrtDis) / (2 * a);
        float t2 = (-b +sqrtDis) / (2 * a);
        
        if (t1 > 0 && t2 > 0) return std::min(t1, t2);
        else if (t1 > 0) return t1;
        else if (t2 > 0) return t2;
        else return -1;

    } else if (discriminant == 0){
        return -b / (2. * a);
    }

    return -1.;
}


// TODO: fix
OverlapingShape raySphereIntersection(Ray &rayA, Transform &transformA, Sphere &sphereB, Transform &transformB){
    OverlapingShape res;

    glm::vec3 globalPosSphere = transformB.getGlobalPosition();
    glm::vec3 globalPosRay = transformA.getGlobalPosition();
    

    float length = raycast(globalPosRay, transformA.applyRotation(rayA.ray_direction), globalPosSphere, sphereB.radius);

    if(length < 0) return res;

    res.exist = true;
    res.correctionDepth = length;
    res.normal = glm::normalize(globalPosSphere - (globalPosRay + length * transformA.applyRotation(rayA.ray_direction)));
    res.position = globalPosRay + length * transformA.applyRotation(rayA.ray_direction);
    return res;
}

OverlapingShape rayPlaneIntersection(Ray &rayA, Transform &transformA, Plane &planeB, Transform &transformB){
    OverlapingShape res;

    glm::vec3 globalPosA = transformA.getGlobalPosition();
    glm::vec3 globalPosB = transformB.getGlobalPosition();

    
    glm::vec3 planeNormal(glm::normalize(transformB.applyRotation(planeB.normal) ));
    glm::vec3 rayDirection(glm::normalize(transformA.applyRotation(rayA.ray_direction)));
    
    float nd = glm::dot(rayDirection, planeNormal);
    float pn = glm::dot(globalPosB, planeNormal);

    if(nd >= 0.f) return res;

    float t = (dot(planeNormal, globalPosB - globalPosA)) / nd;

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
        res.position = transformA.getGlobalPosition() + (globalPosA - globalPosB) / 2.f;
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

    res.position = transformA.getGlobalPosition() + res.normal * res.correctionDepth;
    
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
    glm::vec3 direction = transformB.getGlobalPosition() - closestGlobal;
    float distance = glm::length(direction);

    if (distance > sphereB.radius) return res;

    res.exist = true;
    if (distance > 0.0001f)
        res.normal = -glm::normalize(direction);
    else
        res.normal = glm::vec3(0, 1, 0);

    res.correctionDepth = (sphereB.radius - distance);
    res.position = closestGlobal;

    return res;
}

OverlapingShape CollisionShape::intersectionExist(CollisionShape &shapeA, Transform &transformA, CollisionShape &shapeB, Transform &transformB){
    OverlapingShape res;

    // SPHERE
    if(shapeA.shapeType == SPHERE && shapeB.shapeType == SPHERE){
        float radiusSum = shapeA.sphere.radius + shapeB.sphere.radius;
        float distance = glm::length(transformA.getGlobalPosition() - transformB.getGlobalPosition());
        
        float dist = distance - radiusSum;
        if(dist < 0){
            res.exist = true;
            res.correctionDepth = dist;
            res.normal = glm::normalize(transformA.getGlobalPosition() - transformB.getGlobalPosition());
            res.position = transformA.getGlobalPosition() - shapeA.sphere.radius * res.normal;
        }
        return res;
    } 
    
    // PLANE
    else if(shapeA.shapeType == PLANE && shapeB.shapeType == SPHERE){
        res = spherePlaneIntersection(shapeB.sphere, transformB, shapeA.plane, transformA);
        res.normal = -res.normal;
        return res;
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
