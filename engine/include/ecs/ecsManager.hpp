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
};

