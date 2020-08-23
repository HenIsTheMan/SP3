#include "EntityManager.h"
#include <iostream>
EntityManager::EntityManager(void)
{
	
}

EntityManager::~EntityManager(void)
{
	for (size_t i = 0; i < entityList.size(); ++i)
<<<<<<< HEAD
		if(entityList[i]){
			delete entityList[i];
			entityList[i] = nullptr;
		}
=======
		delete entityList[i];
>>>>>>> origin/Trina-Lim
}

bool EntityManager::Init(void)
{
	entityList.clear();
	return true;
}

void EntityManager::AddEntity(Entity* entity)
{
	entityList.emplace_back(entity);
}

void EntityManager::FetchParticle(int numPerFrame)
{
	int count = 0;
	for (size_t i = 0; i < entityList.size(); ++i)
	{
		Entity* particle = entityList[i];
		if (particle->type == Entity::EntityType::PARTICLE && !particle->active) // Check for entities that are of PARTICLE type and not rendered yet
		{
			particle->active = true;
			count++;
		}
		if (count == numPerFrame) // Number of particles per frame to be rendered is met
			break;
	}
}

void EntityManager::Update(int numPerFrame, glm::vec3 storeCamFront)
{
	FetchParticle(numPerFrame); // Every frame, a certain number of particles will be rendered
	// Put any function here to match the enemy spawning 

	for (size_t i = 0; i < entityList.size(); ++i)
	{
		Entity* entity = entityList[i];
		switch (entity->type)
		{
		case Entity::EntityType::PARTICLE:
			// Do the movement/lifetime for the particles here
			break;

<<<<<<< HEAD
		case Entity::EntityType::ENEMY:
			// Do the movement for the enemies here
=======
		case Entity::EntityType::STATIC_ENEMY:
			entity->active = true;
			break;

		case Entity::EntityType::MOVING_ENEMY:
			// Do the movement for the enemies here
			entity->active = true;
			
>>>>>>> origin/Trina-Lim
			break;

		case Entity::EntityType::BULLET:
			entity->active = true;
			entity->pos = entity->pos + entity->storeCamFront * 0.5f;
			break;
			// Add others if needed
		}
	}
}

//void EntityManager::Render(void)
//{
//	
//
//}

const std::vector<Entity*>& EntityManager::getVector()
{
	return entityList;
}
