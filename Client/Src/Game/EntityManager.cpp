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

void EntityManager::UpdateEntities(const UpdateParams& params){
	const size_t& size = entityList.size();
	for(size_t i = 0; i < size; ++i){
		Entity* const& entity = entityList[i];
		switch(entity->type){
			//case Entity::EntityType::RAIN_PARTICLE:
			//	// Do the movement/lifetime for the particles here
			//	break;
			case Entity::EntityType::ENEMY:
				// Do the movement for the enemies here
				break;
			case Entity::EntityType::BULLET:
				entity->active = true;
				entity->pos = entity->pos + entity->facingDir * .5f;
				break;
		}
	}
}

void EntityManager::RenderEntities(const ShaderProg& SP){
	const size_t& size = entityList.size();
	for(size_t i = 0; i < size; ++i){
		Entity* entity = entityList[i];
		if(entity && entity->active){
			switch(entity->type){
				case Entity::EntityType::ENEMY:
					//PushModel({
					//	Translate(glm::vec3(entity->pos.x, entity->pos.y, entity->pos.z)),
					//	//Rotate(glm::vec4(entity->rotate.x, entity->rotate.y, entity->rotate.z, entity->rotate.w)), // Not sure about the x,y,z etc
					//	Scale(glm::vec3(entity->scale.x, entity->scale.y, entity->scale.z)),
					//	});
					//// Change the mesh or model accordingly
					//meshes[(int)MeshType::Cube]->SetModel(GetTopModel());
					//meshes[(int)MeshType::Cube]->Render(forwardSP); // Remeber to change forwardSP etc accordingly
					//PopModel();
					break;

				//case Entity::EntityType::BULLET:
				//	PushModel({
				//		Translate(glm::vec3(entity->pos.x, entity->pos.y, entity->pos.z)),
				//		//Rotate(glm::vec4(entity->rotate.x, entity->rotate.y, entity->rotate.z, entity->rotate.w)), // Not sure about the x,y,z etc
				//		Scale(glm::vec3(entity->scale.x, entity->scale.y, entity->scale.z)),
				//		});
				//	// Change the mesh or model accordingly
				//	meshes[(int)MeshType::Sphere]->SetModel(GetTopModel());
				//	meshes[(int)MeshType::Sphere]->Render(forwardSP); // Remember to change forwardSP etc accordingly
				//	PopModel();
				//	break;
				default:
					break;
			}
		}
	}
}

const std::vector<Entity*>& EntityManager::GetEntityList() const{
	return entityList;
}