// SOURCE: https://austinmorlan.com/posts/entity_component_system/

#pragma once


#include <cstdint>
#include <bitset>
#include <array>
#include <cassert>
#include <queue>


using ComponentType = std::uint8_t;

constexpr ComponentType MAX_COMPONENTS = 32;

using Signature = std::bitset<MAX_COMPONENTS>;
using Entity = uint32_t;

constexpr size_t MAX_ENTITIES = 5000;


class EntityManager
{
private:

	// Queue of unused entity IDs
	std::queue<Entity> mAvailableEntities{};

	// Array of signatures where the index corresponds to the entity ID
	std::array<Signature, MAX_ENTITIES> mSignatures{};
	std::array<std::string, MAX_ENTITIES> mNames{};

	// Total living entities - used to keep limits on how many exist
	uint32_t mLivingEntityCount{};

public:
	EntityManager()
	{
		for (Entity entity = 0; entity < MAX_ENTITIES; ++entity)
		{
			mAvailableEntities.push(entity);
		}
	}

	Entity CreateEntity()
	{
		assert(mLivingEntityCount < MAX_ENTITIES && "Too many entities in existence.");

		Entity id = mAvailableEntities.front();
		mAvailableEntities.pop();
		++mLivingEntityCount;

		mNames[id] = "Entity - " + std::to_string(id);

		return id;
	}

	void DestroyEntity(Entity entity)
	{
		assert(entity < MAX_ENTITIES && "Entity out of range.");

		// Invalidate the destroyed entity's signature
		mSignatures[entity].reset();
		mNames[entity] = "";

		// Put the destroyed ID at the back of the queue
		mAvailableEntities.push(entity);
		--mLivingEntityCount;
	}

	uint32_t getEntityCount(){
		return mLivingEntityCount;
	}

	std::vector<Entity> GetAllEntities() const {
		std::vector<Entity> entities;
		entities.reserve(mLivingEntityCount); 
	
		for (Entity entity = 0; entity < MAX_ENTITIES; ++entity) {
			if (!mSignatures[entity].none() || !mNames[entity].empty()) {
				entities.push_back(entity);
			}
		}
	
		return entities;
	}

	void SetSignature(Entity entity, Signature signature)
	{
		assert(entity < MAX_ENTITIES && "Entity out of range.");
		
		// Put this entity's signature into the array
		mSignatures[entity] = signature;
	}
	
	void SetName(Entity entity, std::string name){
		assert(entity < MAX_ENTITIES && "Entity out of range.");
		
		mNames[entity] = name;
	}
	
	std::string GetName(Entity entity){
		assert(entity < MAX_ENTITIES && "Entity out of range.");

		return mNames[entity];
	}

	Signature GetSignature(Entity entity)
	{
		assert(entity < MAX_ENTITIES && "Entity out of range.");

		// Get this entity's signature from the array
		return mSignatures[entity];
	}
};