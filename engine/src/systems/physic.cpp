#include <engine/include/ecs/implementations/systems.hpp>
#include <engine/include/ecs/ecsManager.hpp>
#include <iostream>
#include <engine/include/camera.hpp>

const float G = 8.1f;

std::vector<OverlapingShape> detectedCollisions;

void CollisionDetectionSystem::update(float deltaTime){
    narrowPhase();
}

void CollisionDetectionSystem::narrowPhase(){
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

            bool aSeeB = CollisionShape::canSee(shapeA, shapeB);
            bool bSeeA = CollisionShape::canSee(shapeB, shapeA);

            if(!aSeeB && !bSeeA) continue;

            OverlapingShape collision = CollisionShape::intersectionExist(shapeA, transformA, shapeB, transformB);

            if(collision.exist){
                shapeA.isColliding = true;
                shapeB.isColliding = true;
                collision.aSeeB = aSeeB;
                collision.bSeeA = bSeeA;
                collision.entityA = entityA;
                collision.entityB = entityB;
                detectedCollisions.push_back(collision);
            }
        }
    }
}


void PhysicSystem::solver(){
    for(auto overlapping: detectedCollisions){
        // Provisory
        if(!overlapping.aSeeB || !overlapping.bSeeA || mEntities.find(overlapping.entityA) == mEntities.end() || mEntities.find(overlapping.entityB) == mEntities.end()) continue;

        RigidBody &rbA = ecs.GetComponent<RigidBody>(overlapping.entityA);
        RigidBody &rbB = ecs.GetComponent<RigidBody>(overlapping.entityB);
        Transform &tA = ecs.GetComponent<Transform>(overlapping.entityA);
        Transform &tB = ecs.GetComponent<Transform>(overlapping.entityB);

        glm::vec3 relativeVelocity = rbA.velocity - rbB.velocity;
        float velAlongNormal = glm::dot(relativeVelocity, overlapping.normal);


        float e = std::min(rbA.restitutionCoef, rbB.restitutionCoef);
        float invMassA = 1.0f / (rbA.weight * 5.f);
        float invMassB = 1.0f / (rbB.weight * 5.f);

        
        float j = -(1 + e) * velAlongNormal / (invMassA + invMassB);
        glm::vec3 impulse = j * overlapping.normal;

        if(overlapping.aSeeB) rbA.velocity += impulse * invMassA;
        if(overlapping.bSeeA) rbB.velocity -= impulse * invMassB;

        // friction
        glm::vec3 tangent = relativeVelocity - velAlongNormal * overlapping.normal;
        if (glm::length(tangent) > 0.0001f)
            tangent = glm::normalize(tangent);
        else
            tangent = glm::vec3(0.0f);


        float frictionCoef = std::sqrt(rbA.frictionCoef * rbB.frictionCoef);

    
        float jt = -glm::dot(relativeVelocity, tangent) / (invMassA + invMassB);
        jt = glm::clamp(jt, -j, j);

        glm::vec3 frictionImpulse = jt * tangent;

        if(overlapping.aSeeB) rbA.velocity += frictionImpulse * invMassA;
        if(overlapping.bSeeA) rbB.velocity -= frictionImpulse * invMassB;



        if(rbA.isStatic && !rbB.isStatic && overlapping.bSeeA){
            tB.translate(overlapping.normal * overlapping.correctionDepth);
        } else if(rbB.isStatic && !rbA.isStatic && overlapping.aSeeB){
            tA.translate(overlapping.normal * overlapping.correctionDepth);
        } else if(!rbA.isStatic && !rbB.isStatic){
            tB.translate(overlapping.normal * overlapping.correctionDepth * 0.5f);
            tA.translate(-overlapping.normal * overlapping.correctionDepth * 0.5f);
        }
    }
}


void PhysicSystem::update(float deltaTime){
    solver();

    for(auto &entity: mEntities){
        auto& rigidBody = ecs.GetComponent<RigidBody>(entity);
        auto& transform = ecs.GetComponent<Transform>(entity);
        auto& shape = ecs.GetComponent<CollisionShape>(entity);

        if(!rigidBody.isStatic){
            float acceleration = G * rigidBody.weight;
            rigidBody.velocity += acceleration * deltaTime * rigidBody.gravityDirection;
            transform.translate(rigidBody.velocity * deltaTime);
        }
    }

}



void generateSphere(std::vector<float> &vertex_buffer_data, std::vector<unsigned int> &indices, int latitudeBands, int longitudeBands){
    std::vector<glm::vec3> indexed_vertices;
    std::vector<glm::vec2> tex_coords;

    for (int lat = 0; lat <= latitudeBands; ++lat) {
        float theta = lat * glm::pi<float>() / latitudeBands;
        float sinTheta = sin(theta);
        float cosTheta = cos(theta);
    
        for (int lon = 0; lon <= longitudeBands; ++lon) {
            float phi = lon * 2 * glm::pi<float>() / longitudeBands;
            float sinPhi = sin(phi);
            float cosPhi = cos(phi);
    
            glm::vec3 vertex = glm::vec3(cosPhi * sinTheta, cosTheta, sinPhi * sinTheta);
            indexed_vertices.push_back(vertex);
        }
    }
    
    
    for (int lat = 0; lat <= latitudeBands; ++lat) {
        for (int lon = 0; lon < longitudeBands; ++lon) {
            int current = lat * (longitudeBands + 1) + lon;
            indices.push_back(current);
            indices.push_back(current + 1);
        }
    }

    for (int lat = 0; lat < latitudeBands; ++lat) {
        for (int lon = 0; lon < longitudeBands; ++lon) {
            int first = (lat * (longitudeBands + 1)) + lon;
            int second = first + longitudeBands + 1;

            indices.push_back(first);
            indices.push_back(second);
        }
    }

    for(int i=0; i<indexed_vertices.size(); i++){
        glm::vec3 vertex = indexed_vertices[i];

        vertex_buffer_data.push_back(vertex.x);
        vertex_buffer_data.push_back(vertex.y);
        vertex_buffer_data.push_back(vertex.z);
    
        // vertex_buffer_data.push_back(tex_coords[i].x);
        // vertex_buffer_data.push_back(tex_coords[i].y);
    }
}

void initBuffer(GLuint &VAO, std::vector<float> &vertex_buffer_data, std::vector<unsigned int> &indices){
    GLuint VBO, EBO;
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);
    glGenBuffers(1, &EBO);

    glBindVertexArray(VAO);
    
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, vertex_buffer_data.size() * sizeof(float), vertex_buffer_data.data(), GL_STATIC_DRAW);
    
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(unsigned int), indices.data(), GL_STATIC_DRAW);
    
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
}

void PhysicDebugSystem::init() {
    program = Program("shaders/debug/vertex.glsl", "shaders/debug/fragment.glsl");

    auto p = Camera::getInstance().getP();
    program.updateProjectionMatrix(p);

    // Génération de la sphère
    std::vector<float> sphereVertices;
    std::vector<unsigned int> sphereIndices;
    generateSphere(sphereVertices, sphereIndices, 8, 8); // subdivision paramétrable
    sphereIndexCount = sphereIndices.size();
    initBuffer(sphereVAO, sphereVertices, sphereIndices);

    // Génération du quad
    std::vector<float> quadVertices = {
        -1.0f, 0.0f, -1.0f,
         1.0f, 0.0f, -1.0f,
         1.0f, 0.0f,  1.0f,
        -1.0f, 0.0f,  1.0f
    };
    std::vector<unsigned int> quadIndices = { 0, 1, 1, 2, 2, 3, 3, 0, 0, 2 };
    quadIndexCount = 10;
    initBuffer(quadVAO, quadVertices, quadIndices);


    std::vector<float> rayVertices = {
        0.f, 0.f, 0.f,
        0.f, 0.f, -1.0f,
    };
    std::vector<unsigned int> rayIndices = { 0, 1 };
    initBuffer(rayVAO, rayVertices, rayIndices);


    std::vector<float> boxVertices = {
        -1.0f, -1.0f, -1.0f,
         1.0f, -1.0f, -1.0f,
         1.0f,  1.0f, -1.0f,
        -1.0f,  1.0f, -1.0f,
        -1.0f, -1.0f,  1.0f,
         1.0f, -1.0f,  1.0f,
         1.0f,  1.0f,  1.0f,
        -1.0f,  1.0f,  1.0f,
    };
    
    std::vector<unsigned int> boxIndices = {
        0,1, 1,2, 2,3, 3,0, // bas
        4,5, 5,6, 6,7, 7,4, // haut
        0,4, 1,5, 2,6, 3,7  // verticales
    };
    
    boxIndexCount = static_cast<unsigned int>(boxIndices.size());
    initBuffer(boxVAO, boxVertices, boxIndices);
    

    glBindVertexArray(0);
}


void PhysicDebugSystem::update(){
    glUseProgram(program.programID);
    GLuint scaleLocation = glGetUniformLocation(program.programID, "scale");
    
    glm::mat4 V = Camera::getInstance().getV();
    program.updateViewMatrix(V);
    
    GLuint tempVBO;
    glGenBuffers(1, &tempVBO);
    GLuint colorLocation = glGetUniformLocation(program.programID, "albedo");
    
    glLineWidth(2.0f);
    
    
    
    for(auto &entity: mEntities){
        auto& shape = ecs.GetComponent<CollisionShape>(entity);
        auto& transform = ecs.GetComponent<Transform>(entity);

        glm::mat4 model;
        if(shape.shapeType != AABB){
            model = transform.getModelMatrix();
        }
        else {
            model = glm::mat4(1);
            model = glm::translate(model, transform.getGlobalPosition());
        }
        program.updateModelMatrix(model);

        if(shape.isColliding) glUniform4f(colorLocation, 1,0,0,1);
        else glUniform4f(colorLocation, 0,1,0,1);

        int indexCount = 0;
        if(shape.shapeType == SPHERE){
            indexCount = sphereIndexCount;
            float r = shape.sphere.radius;
            glUniform3f(scaleLocation, r,r,r);
            glBindVertexArray(sphereVAO);
        } else if (shape.shapeType == PLANE){
            indexCount = quadIndexCount;
            glBindVertexArray(quadVAO);
        } else if(shape.shapeType == OOBB || shape.shapeType == AABB){
            indexCount = boxIndexCount;
            glm::vec3 scale = shape.oobb.halfExtents;
            // glUniform3f(scaleLocation, scale.x, scale.y, scale.z);
            glBindVertexArray(boxVAO);
        } else if (shape.shapeType == RAY){
            glm::vec3 points[2] = { glm::vec3(0), shape.ray.ray_direction * shape.ray.length };
            
            glBindVertexArray(rayVAO);
            glBindBuffer(GL_ARRAY_BUFFER, tempVBO);
            glBufferData(GL_ARRAY_BUFFER, sizeof(points), points, GL_STATIC_DRAW);

            glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, (void*)0);
            glEnableVertexAttribArray(0);

            glDrawArrays(GL_LINES, 0, 2);

            glBindVertexArray(0);
            continue;
        }
        
        glDrawElements(
            GL_LINES,      // mode
            indexCount,
            GL_UNSIGNED_INT,   // type
            (void*)0           // element array buffer offset
        );
        
        glBindVertexArray(0);        
    }
    glDeleteBuffers(1,&tempVBO);
}
