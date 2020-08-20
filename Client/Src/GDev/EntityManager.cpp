#include "EntityManager.h"
#include <iostream>
EntityManager::EntityManager(void)
{
	
}

EntityManager::~EntityManager(void)
{
	for (size_t i = 0; i < entityList.size(); ++i)
		delete entityList[i];
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
		if (particle->type == Entity::EntityType::PARTICLE && !particle->active && !particle->rendered) // Check for entities that are of PARTICLE type and not rendered yet
		{
			particle->active = true;
			particle->rendered = true;
			count++;
		}
		if (count == numPerFrame) // Number of particles per frame to be rendered is met
			break;
	}
}

void EntityManager::UpdateParticles()
{
	// Do the movement/lifetime for the particles here
}

void EntityManager::UpdateEnemies()
{
	// Do the movement for the enemies here
}

void EntityManager::Update(int numPerFrame)
{
	FetchParticle(numPerFrame); // Every frame, a certain number of particles will be rendered
	// Put any function here to match the enemy spawning 

	for (size_t i = 0; i < entityList.size(); ++i)
	{
		Entity* entity = entityList[i];
		switch (entity->type)
		{
		case Entity::EntityType::PARTICLE:
			UpdateParticles();
			break;

		case Entity::EntityType::ENEMY:
			UpdateEnemies();
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
