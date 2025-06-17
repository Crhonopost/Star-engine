#include <engine/include/rendering/common.hpp>
#include <engine/include/API/ResourceManagement/ResourceManager.hpp>
#include <engine/include/rendering/rendering.hpp>



std::shared_ptr<SingleMesh> MeshHelper::generateSphere(float radius, int nbOfVerticesTotal) {
    const std::string name = "Sphere_" + std::to_string(radius) + "_" + std::to_string(nbOfVerticesTotal);
    auto mesh = SingleMeshManager::load(name, true);
    if (mesh) {
        return mesh;
    }

    mesh = std::make_shared<SingleMesh>();
    SingleMeshManager::addPreloaded(name, mesh);

    std::vector<unsigned short> indices;
    std::vector<glm::vec3> indexed_vertices;
    std::vector<glm::vec2> tex_coords;
    std::vector<glm::vec3> normal;

    const int latitudeBands = 20;
    const int longitudeBands = 20;

    for (int lat = 0; lat <= latitudeBands; ++lat) {
        float theta = lat * glm::pi<float>() / latitudeBands;
        float sinTheta = sin(theta);
        float cosTheta = cos(theta);

        for (int lon = 0; lon <= longitudeBands; ++lon) {
            float phi = lon * 2 * glm::pi<float>() / longitudeBands;
            float sinPhi = sin(phi);
            float cosPhi = cos(phi);

            glm::vec3 vertex = radius * glm::vec3(cosPhi * sinTheta, cosTheta, sinPhi * sinTheta);
            indexed_vertices.push_back(vertex);
            normal.push_back(glm::normalize(vertex));

            tex_coords.push_back(glm::vec2(
                (float) lon / (float) longitudeBands, 
                (float) lat / (float) latitudeBands
            ));
        }
    }

    for (int lat = 0; lat < latitudeBands; ++lat) {
        for (int lon = 0; lon < longitudeBands; ++lon) {
            int first = (lat * (longitudeBands + 1)) + lon;
            int second = first + longitudeBands + 1;

            indices.push_back(first + 1);
            indices.push_back(second);
            indices.push_back(first);
            
            indices.push_back(first + 1);
            indices.push_back(second + 1);
            indices.push_back(second);
        }
    }

    std::vector<Vertex> vertex_buffer_data;

    for(int i=0; i<indexed_vertices.size(); i++){
        Vertex v;
        
        v.position = indexed_vertices[i];
        v.normal = normal[i];
        v.texCoord = tex_coords[i];
    
        v.boneIndices = {0,0,0,0};
        v.boneWeights = {0,0,0,0};
        
        vertex_buffer_data.push_back(v);
    }

    mesh->init(vertex_buffer_data, indices);

    return mesh;
}

std::shared_ptr<SingleMesh> MeshHelper::generatePlane(float sideLength, int nbOfVerticesSide){
    const std::string name = "Plane_" + std::to_string(sideLength) + "_" + std::to_string(nbOfVerticesSide);
    auto mesh = SingleMeshManager::load(name, true);
    if (mesh) {
        return mesh;
    }
    
    mesh = std::make_shared<SingleMesh>();
    SingleMeshManager::addPreloaded(name, mesh);

    std::vector<unsigned short> indices;
    std::vector<glm::vec3> indexed_vertices;
    std::vector<glm::vec2> tex_coords;
    std::vector<glm::vec3> normal;

    float edgeLength = sideLength / (nbOfVerticesSide - 1.);

    glm::vec3 offset(sideLength / 2., 0, sideLength / 2.);

    glm::vec3 min(999,999,999);
    glm::vec3 max(-999,-999,-999);

    for(size_t i=0; i<nbOfVerticesSide; i++){
        for(size_t j=0; j<nbOfVerticesSide; j++){
            glm::vec3 vertexPos = edgeLength * glm::vec3(i, 0, j) - offset;
            
            for(int i=0; i<3; i++){
                if(min[i] > vertexPos[i]) min[i] = vertexPos[i];
                if(max[i] < vertexPos[i]) max[i] = vertexPos[i];
            }

            indexed_vertices.push_back(vertexPos);
            normal.push_back(glm::vec3(0.f,1.f,0.f));
            
            glm::vec2 v_coords(
                1. - (double) i / (double) (nbOfVerticesSide - 1),
                (double) j / (double) (nbOfVerticesSide - 1));
            tex_coords.push_back(v_coords);
        }
    }

    for(size_t i=0; i<nbOfVerticesSide-1; i++){
        for(size_t j=1; j<nbOfVerticesSide; j++){
            indices.push_back(i * nbOfVerticesSide + j - 1);
            indices.push_back(i * nbOfVerticesSide + j);
            indices.push_back((i + 1) * nbOfVerticesSide + j - 1);

            indices.push_back((i + 1) * nbOfVerticesSide + j - 1);
            indices.push_back(i * nbOfVerticesSide + j);
            indices.push_back((i + 1) * nbOfVerticesSide + j);        
        }
    }

    std::vector<Vertex> vertex_buffer_data;
    
    for(int i=0; i<indexed_vertices.size(); i++){
        Vertex v;
        
        v.position = indexed_vertices[i];
        v.normal = normal[i];
        v.texCoord = tex_coords[i];
    

        v.boneIndices = {0,0,0,0};
        v.boneWeights = {0,0,0,0};
        
        vertex_buffer_data.push_back(v);
    }

    mesh->init(vertex_buffer_data, indices);

    return mesh;
}


std::shared_ptr<SingleMesh> MeshHelper::generateCube(float sideLength, int verticesPerSide, bool inward) {
    const std::string name = "Cube_" + std::to_string(sideLength) + "_" + std::to_string(verticesPerSide) + (inward ? "_inward" : "_outward");
    auto mesh = SingleMeshManager::load(name, true);
    if (mesh) {
        return mesh;
    }
    
    mesh = std::make_shared<SingleMesh>();
    SingleMeshManager::addPreloaded(name, mesh);

    if (sideLength <= 0.0f) throw std::invalid_argument("Side length must be positive");
    if (verticesPerSide < 2) throw std::invalid_argument("At least 2 vertices per side are required");

    std::vector<unsigned short> indices;
    std::vector<glm::vec3> vertices;
    std::vector<glm::vec2> texCoords;
    std::vector<glm::vec3> normals;

    const float edgeIncrement = sideLength / (verticesPerSide - 1);
    const float halfLength = sideLength / 2.0f;

    struct Face {
        glm::vec3 origin;
        glm::vec3 uDir;
        glm::vec3 vDir;
        glm::vec3 normal;
    };

    // Faces with correct outward-pointing normals
    const std::vector<Face> faces = {
        // +Y (top)
        {{-halfLength,  halfLength,  halfLength}, {edgeIncrement, 0, 0}, {0, 0, -edgeIncrement}, {0, 1, 0}},
        // -Y (bottom)
        {{-halfLength, -halfLength, -halfLength}, {edgeIncrement, 0, 0}, {0, 0, edgeIncrement}, {0, -1, 0}},
        // +X (right)
        {{ halfLength, -halfLength, -halfLength}, {0, edgeIncrement, 0}, {0, 0, edgeIncrement}, {1, 0, 0}},
        // -X (left)
        {{-halfLength, -halfLength,  halfLength}, {0, edgeIncrement, 0}, {0, 0, -edgeIncrement}, {-1, 0, 0}},
        // +Z (front)
        {{-halfLength, -halfLength,  halfLength}, {edgeIncrement, 0, 0}, {0, edgeIncrement, 0}, {0, 0, 1}},
        // -Z (back)
        {{ halfLength, -halfLength, -halfLength}, {-edgeIncrement, 0, 0}, {0, edgeIncrement, 0}, {0, 0, -1}}
    };

    int vertexOffset = 0;
    const float texCoordStep = 1.0f / (verticesPerSide - 1);

    // Reserve memory for better performance
    const int verticesPerFace = verticesPerSide * verticesPerSide;
    const int indicesPerFace = 2 * 3 * (verticesPerSide - 1) * (verticesPerSide - 1);
    vertices.reserve(6 * verticesPerFace);
    texCoords.reserve(6 * verticesPerFace);
    normals.reserve(6 * verticesPerFace);
    indices.reserve(6 * indicesPerFace);

    for (const auto& face : faces) {
        // Generate vertices and texture coordinates
        for (int i = 0; i < verticesPerSide; ++i) {
            for (int j = 0; j < verticesPerSide; ++j) {
                vertices.push_back(face.origin + static_cast<float>(i)*face.uDir + static_cast<float>(j)*face.vDir);
                normals.push_back(inward ? -face.normal : face.normal);
                texCoords.emplace_back(i * texCoordStep, j * texCoordStep);
            }
        }

        // Generate indices with correct winding order
        for (unsigned short i = 0; i < verticesPerSide - 1; ++i) {
            for (unsigned short j = 0; j < verticesPerSide - 1; ++j) {
                const unsigned short a = vertexOffset + i * verticesPerSide + j;
                const unsigned short b = a + 1;
                const unsigned short c = a + verticesPerSide;
                const unsigned short d = c + 1;

                if (!inward) {
                    // Clockwise winding
                    indices.insert(indices.end(), {a, c, b});
                    indices.insert(indices.end(), {b, c, d});
                } else {
                    // Counter-clockwise winding
                    indices.insert(indices.end(), {a, b, c});
                    indices.insert(indices.end(), {b, d, c});
                }
            }
        }

        vertexOffset += verticesPerFace;
    }

    std::vector<Vertex> vertexBuffer;
    vertexBuffer.reserve(vertices.size() * 8);
    
    for (size_t i = 0; i < vertices.size(); ++i) {
        Vertex v;
        v.position = {vertices[i].x, vertices[i].y, vertices[i].z};
        v.texCoord = {texCoords[i].x, texCoords[i].y};
        v.normal = {normals[i].x, normals[i].y, normals[i].z};
        
        v.boneIndices = {0,0,0,0};
        v.boneWeights = {0,0,0,0};

        vertexBuffer.push_back(v);
    }

    mesh->init(vertexBuffer, indices);
    
    return mesh;
}

