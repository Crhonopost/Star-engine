#include <engine/include/spatial.hpp>

using namespace spatial;

void SpatialNode::AddChild(SpatialNode *child) {
    children_.emplace_back(std::move(child));
    children_.back()->parent_ = this;
}

void SpatialNode::RemoveChild(SpatialNode* component) {
    for (auto it = children_.begin(); it != children_.end(); ++it) {
        if ((*it) == component) {
            (*it)->SetParent(nullptr);
            children_.erase(it);
            break;        
        }
    }
}

void SpatialNode::updateSelfAndChildTransform(){
    if(transform.isDirty()){
        forceUpdateSelfAndChild();
        return;
    }

    for(auto &&child: children_){
        child->updateSelfAndChildTransform();
    }
}

void SpatialNode::forceUpdateSelfAndChild(){
    if (parent_){
        transform.computeModelMatrix(parent_->transform.getModelMatrix());
    }
    else
        transform.computeModelMatrix();

    for (auto&& child : children_)
    {
        child->forceUpdateSelfAndChild();
    }
}



MeshInstance::MeshInstance(){
    indices = vector<unsigned short>();
    indexed_vertices = vector<glm::vec3>();
    tex_coords = vector<glm::vec2>();
}
    
MeshInstance::MeshInstance(vector<unsigned short> &indices, vector<glm::vec3> &indexed_vertices, vector<glm::vec2> &tex_coords): MeshInstance(){
    this->indexed_vertices = indexed_vertices;
    this->indices = indices;
    this->tex_coords = tex_coords;
    this->transform = Transform();
}


void MeshInstance::getBufferData(std::vector<float> &vertex_buffer_data){
    vertex_buffer_data.resize(0);
    for(int i=0; i<indexed_vertices.size(); i++){
        glm::vec3 vertex = indexed_vertices[i];

        vertex_buffer_data.push_back(vertex.x);
        vertex_buffer_data.push_back(vertex.y);
        vertex_buffer_data.push_back(vertex.z);
    
        vertex_buffer_data.push_back(tex_coords[i].x);
        vertex_buffer_data.push_back(tex_coords[i].y);
    }
}

int MeshInstance::getNumberOfIndices(){
    return indices.size();
}

void* MeshInstance::getIndiceIdx(){
    return &indices[0];
}


void Sphere::generateMesh() {
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

            indices.push_back(first);
            indices.push_back(second);
            indices.push_back(first + 1);
            
            indices.push_back(second);
            indices.push_back(second + 1);
            indices.push_back(first + 1);
        }
    }

    std::vector<float> vertex_buffer_data;
    
    getBufferData(vertex_buffer_data);
    

    glGenVertexArrays(1, &meshData.VAO);
    glGenBuffers(1, &meshData.VBO);
    glGenBuffers(1, &meshData.EBO);

    glBindVertexArray(meshData.VAO);
    
    glBindBuffer(GL_ARRAY_BUFFER, meshData.VBO);
    glBufferData(GL_ARRAY_BUFFER, vertex_buffer_data.size() * sizeof(float), &vertex_buffer_data[0], GL_STATIC_DRAW);
    
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, meshData.EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, getNumberOfIndices() * sizeof(unsigned short), getIndiceIdx(), GL_STATIC_DRAW);
    
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0);

    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(3 * sizeof(float)));


    glBindVertexArray(0);

}
Sphere::Sphere(float radius) : MeshInstance(){
    this->radius = radius;
    generateMesh();
}


void Mountain::generateMesh(){
    indexed_vertices.resize(0);
    indices.resize(0);
    tex_coords.resize(0);

    double edgeLengthRow = width / (double) (nbOfVerticesSide - 1);
    double edgeLengthCol = height / (double) (nbOfVerticesSide - 1);

    glm::vec3 offset(width / 2., 0, height / 2.);

    for(size_t i=0; i<nbOfVerticesSide; i++){
        for(size_t j=0; j<nbOfVerticesSide; j++){
            glm::vec3 vertexPos = glm::vec3((double)j * edgeLengthRow, 0, (double) i * edgeLengthCol) - offset + position;
            
            indexed_vertices.push_back(vertexPos);
            
            glm::vec2 v_coords(
                (double) i / (double) (nbOfVerticesSide - 1),
                1. - (double) j / (double) (nbOfVerticesSide - 1));
            tex_coords.push_back(v_coords);
        }
    }

    for(size_t i=0; i<nbOfVerticesSide-1; i++){
        for(size_t j=1; j<nbOfVerticesSide; j++){
            indices.push_back(i * nbOfVerticesSide + j - 1);
            indices.push_back(i * nbOfVerticesSide + j);
            indices.push_back((i + 1) * nbOfVerticesSide + j - 1);


            indices.push_back(i * nbOfVerticesSide + j);
            indices.push_back((i + 1) * nbOfVerticesSide + j - 1);
            indices.push_back((i + 1) * nbOfVerticesSide + j);        
        }
    }
}

Mountain::Mountain(): MeshInstance(){
    width = height = 1;
    position = glm::vec3(0);
    nbOfVerticesSide = 2;
    scale = 1;

    generateMesh();
}

Mountain::Mountain(double width, double height, glm::vec3 position, size_t nbOfVerticesSide, float scale) : MeshInstance(){
    this->width = width;
    this->height = height;
    this->position = position;
    this->nbOfVerticesSide = nbOfVerticesSide;
    this->scale = scale;

    generateMesh();
}

void Mountain::addVerticesQuantity(int newNumber){
    nbOfVerticesSide += newNumber;
    generateMesh();
}

int Mountain::getNumberOfVerticesSide(){return this->nbOfVerticesSide;}
