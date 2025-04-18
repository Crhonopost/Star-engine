#include <engine/include/ecs/implementations/components.hpp>
#include <iostream>


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

void Drawable::init(std::vector<float> &vertex_buffer_data, std::vector<short unsigned int> &indices){
    glGenVertexArrays(1,&VAO);
    glGenBuffers(1, &VBO);
    glGenBuffers(1, &EBO);

    glBindVertexArray(VAO);
    
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, vertex_buffer_data.size() * sizeof(float), &vertex_buffer_data[0], GL_STATIC_DRAW);
    
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(unsigned short), &indices[0], GL_STATIC_DRAW);
    
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)0);

    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(3 * sizeof(float)));

    glEnableVertexAttribArray(2);
    glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(5 * sizeof(float)));


    glBindVertexArray(0);
}

glm::mat4 Transform::getLocalModelMatrix(){
    const glm::mat4 transformX = glm::rotate(glm::mat4(1.0f),
    glm::radians(eulerRot.x),
    glm::vec3(1.0f, 0.0f, 0.0f));
    const glm::mat4 transformY = glm::rotate(glm::mat4(1.0f),
    glm::radians(eulerRot.y),
    glm::vec3(0.0f, 1.0f, 0.0f));
    const glm::mat4 transformZ = glm::rotate(glm::mat4(1.0f),
    glm::radians(eulerRot.z),
    glm::vec3(0.0f, 0.0f, 1.0f));
    
    // Y * X * Z
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
    
    // translation * rotation * scale (also know as TRS matrix)
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
    
void Transform::setLocalRotation(glm::vec3 rotationAngles){
    eulerRot = rotationAngles;
    dirty = true;
}

void Transform::rotate(glm::vec3 rotations){
    eulerRot += rotations;
    dirty = true;
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
        res.normal = planeB.normal;
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

OverlapingShape CollisionShape::intersectionExist(CollisionShape &shapeA, Transform &transformA, CollisionShape &shapeB, Transform &transformB){
    OverlapingShape res;

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
    } else if(shapeA.shapeType == PLANE && shapeB.shapeType == SPHERE){
        return spherePlaneIntersection(shapeB.sphere, transformB, shapeA.plane, transformA);
    } else if(shapeA.shapeType == SPHERE && shapeB.shapeType == PLANE){
        return spherePlaneIntersection(shapeA.sphere, transformA, shapeB.plane, transformB);
    } else if(shapeA.shapeType == RAY && shapeB.shapeType == SPHERE){
        return raySphereIntersection(shapeA.ray, transformA, shapeB.sphere, transformB);
    } else if(shapeA.shapeType == SPHERE && shapeB.shapeType == RAY){
        return raySphereIntersection(shapeB.ray, transformB, shapeA.sphere, transformA);
    }

    return res;
}

