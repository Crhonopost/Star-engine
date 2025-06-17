#include <assimp/scene.h>
#include <assimp/postprocess.h> 
#include <assimp/Importer.hpp>
#include <engine/include/API/ResourceManagement/ResourceManager.hpp>

#include <engine/include/rendering/rendering.hpp>
#include <engine/include/API/AssetLoader.hpp>


bool API::AssetLoader::loadMesh(const std::string &path, std::vector<SingleMesh> &meshes, std::vector<Material> &materials) {

}





bool SingleMesh::load(const std::string &name) {    
    Assimp::Importer importer;
    std::string filePath = name;
    const aiScene* scene = importer.ReadFile(filePath,
        aiProcess_Triangulate |
        aiProcess_JoinIdenticalVertices |
        aiProcess_SortByPType |
        aiProcess_FlipUVs); // si besoin

    if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode) {
        std::cerr << "Assimp error: " << importer.GetErrorString() << std::endl;
    }

    subMeshes.resize(scene->mNumMeshes);
    
    for (unsigned int i = 0; i < scene->mNumMeshes; ++i) {
        subMeshes[i] = std::make_shared<SingleMesh>();
        
        std::vector<unsigned short> indices;
        std::vector<glm::vec3> indexed_vertices;
        std::vector<glm::vec2> tex_coords;
        std::vector<glm::vec3> normal;
        aiMesh* mesh = scene->mMeshes[i];
    
        for (unsigned int j = 0; j < mesh->mNumFaces; ++j) {
            aiFace& face = mesh->mFaces[j];
            for (unsigned int k = 0; k < face.mNumIndices; ++k) {
                indices.push_back(face.mIndices[k]);  // Ajoute l'indice
            }
        }
    
        for (unsigned int j = 0; j < mesh->mNumVertices; ++j) {
            aiVector3D vertex = mesh->mVertices[j];
            indexed_vertices.push_back(glm::vec3(vertex.x, vertex.y, vertex.z));  // Ajoute le vertex
        }
    
        if (mesh->HasNormals()) {
            for (unsigned int j = 0; j < mesh->mNumVertices; ++j) {
                aiVector3D normal_vec = mesh->mNormals[j];
                normal.push_back(glm::vec3(normal_vec.x, normal_vec.y, normal_vec.z));  // Ajoute la normale
            }
        }
    
        if (mesh->HasTextureCoords(0)) {
            for (unsigned int j = 0; j < mesh->mNumVertices; ++j) {
                aiVector3D tex_coord = mesh->mTextureCoords[0][j];
                tex_coords.push_back(glm::vec2(tex_coord.x, tex_coord.y));  // Ajoute la coordonnée de texture
            }
        }

        const std::string materialName = mesh->mMaterialIndex < scene->mNumMaterials ? scene->mMaterials[mesh->mMaterialIndex]->GetName().C_Str() : "default";
        subMeshes[i]->material = std::make_shared<Material>();
        extractMaterial(*subMeshes[i]->material.get(), mesh, scene);
        
        
        std::vector<Vertex> vertex_buffer_data;
        for (int i = 0; i < indexed_vertices.size(); ++i) {
            Vertex v;
            v.position = indexed_vertices[i];
            v.normal = normal[i];
            v.texCoord = tex_coords[i];
            
            v.boneWeights = glm::vec4(0.0f);
            v.boneIndices = glm::ivec4(0);
            
            vertex_buffer_data.push_back(v);
        }
    

        subMeshes[i]->init(vertex_buffer_data, indices);
    }

    return true;
}
