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
    if(transform->isDirty()){
        forceUpdateSelfAndChild();
        return;
    }

    for(auto &&child: children_){
        child->updateSelfAndChildTransform();
    }
}

void SpatialNode::forceUpdateSelfAndChild(){
    if (parent_){
        transform->computeModelMatrix(parent_->transform->getModelMatrix());
    }
    else
        transform->computeModelMatrix();

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
