#include <engine/include/spatial.hpp>


void SpatialNode::AddChild(std::unique_ptr<SpatialNode> child) {
    child->parent_ = this;  // On défini le parent de l'enfant avant de l'ajouter
    children_.emplace_back(std::move(child));  // Ajoute l'enfant en prenant la propriété de l'objet
}

void SpatialNode::RemoveChild(SpatialNode* component) {
    for (auto it = children_.begin(); it != children_.end(); ++it) {
        if (it->get() == component) {  // Vérifie si l'enfant correspond
            it->get()->SetParent(nullptr);  // Déconnecte le parent
            children_.erase(it);  // Enlève l'enfant du vecteur
            break;
        }
    }
}

void SpatialNode::updateSelfAndChildTransform() {
    if (transform->isDirty()) {
        forceUpdateSelfAndChild();
        return;
    }

    for (auto&& child : children_) {
        child->updateSelfAndChildTransform();
    }
}

void SpatialNode::forceUpdateSelfAndChild() {
    if (parent_) {
        transform->computeModelMatrix(parent_->transform->getModelMatrix());
    }
    else {
        transform->computeModelMatrix();
    }

    for (auto&& child : children_) {
        child->forceUpdateSelfAndChild();
    }
}


