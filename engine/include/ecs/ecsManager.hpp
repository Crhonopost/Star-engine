#pragma once


#include <engine/include/ecs/ecsWithoutInspector.hpp>
#include <engine/include/ecs/implementations/componentInspector.hpp>

class ecsManager: public ecsWithoutInspector
{
	public:
	std::vector<std::unique_ptr<IComponentInspector>> componentInspectors;

	template<typename T>
	void RegisterComponent(const std::string& name)
	{
		componentInspectors.push_back(
			std::make_unique<ComponentInspector<T>>()
		);
			
		mComponentManager->RegisterComponent<T>();
	}

	void DisplayUI(){
		auto entities = mEntityManager->GetAllEntities();
        for(auto entity : entities){
			if(ImGui::TreeNode(GetEntityName(entity).c_str())){
				for (auto& inspector : componentInspectors) {
					inspector->DisplayGUI(*this, entity);
				}
				ImGui::TreePop();
			}
		}
	}
};

