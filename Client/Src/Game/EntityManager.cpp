#include "EntityManager.h"
#include <iostream>

EntityManager::EntityManager():
	entityList()
{
}

EntityManager::~EntityManager(){
	const size_t& size = entityList.size();
	for(size_t i = 0; i < size; ++i){
		if(entityList[i]){
			delete entityList[i];
			entityList[i] = nullptr;
		}
	}
}

///Shld only be called once in Scene::Init()
void EntityManager::CreateEntities(const int& amt){
	entityList = std::vector<Entity*>(amt); //Prealloc mem
	for(int i = 0; i < amt; ++i){
		entityList[i] = new Entity();
	}
}

Entity* const& EntityManager::FetchEntity(){
	for(Entity* const& entity: entityList){
		if(!entity->active){
			return entity;
		}
	}
	entityList.emplace_back(new Entity());
	(void)puts("1 entity was added to entityList!\n");
	return entityList.back();
}

void EntityManager::Update(const UpdateParams& params){
	const size_t& size = entityList.size();
	for(size_t i = 0; i < size; ++i){
		Entity* const& entity = entityList[i];
		switch(entity->type){
			case Entity::EntityType::PARTICLE:
				// Do the movement/lifetime for the particles here
				break;
			case Entity::EntityType::ENEMY:
				// Do the movement for the enemies here
				break;
			case Entity::EntityType::BULLET:
				entity->active = true;
				entity->pos = entity->pos + entity->facingDir * .5f;
				break;
			// Add others if needed
		}
	}
}

const std::vector<Entity*>& EntityManager::GetEntityList() const{
	return entityList;
}