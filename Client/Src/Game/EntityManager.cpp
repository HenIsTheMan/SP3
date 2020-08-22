#include "EntityManager.h"
#include <iostream>

EntityManager::EntityManager():
	entityList(),
	root(new CubeSection())
{
	root->SetOrigin(glm::vec3(0.f));
	root->SetSize(glm::vec3(500.f, 150.f, 500.f));
	root->InitCubeSectionPool(50);
}

EntityManager::~EntityManager(){
	const size_t& size = entityList.size();
	for(size_t i = 0; i < size; ++i){
		if(entityList[i]){
			delete entityList[i];
			entityList[i] = nullptr;
		}
	}
	root->DestroyCubeSectionPool();
	if(root){
		delete root;
		root = nullptr;
	}
}

const std::vector<Entity*>& EntityManager::GetEntityList() const{
	return entityList;
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

void EntityManager::CreateEntities(const int& amt){ //Shld only be called once in Scene::Init()
	entityList = std::vector<Entity*>(amt); //Prealloc mem
	for(int i = 0; i < amt; ++i){
		entityList[i] = new Entity();
		root->AddEntity(entityList[i]);
	}
}

void EntityManager::UpdateEntities(const UpdateParams& params){
	root->Deactivate();
	root->Partition();
	//Collision

	const size_t& size = entityList.size();
	for(size_t i = 0; i < size; ++i){
		Entity* const& entity = entityList[i];
		switch(entity->type){
			case Entity::EntityType::ENEMY:
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
				case Entity::EntityType::BULLET:
				//	PushModel({
				//		Translate(glm::vec3(entity->pos.x, entity->pos.y, entity->pos.z)),
				//		//Rotate(glm::vec4(entity->rotate.x, entity->rotate.y, entity->rotate.z, entity->rotate.w)), // Not sure about the x,y,z etc
				//		Scale(glm::vec3(entity->scale.x, entity->scale.y, entity->scale.z)),
				//		});
				//	// Change the mesh or model accordingly
				//	meshes[(int)MeshType::Sphere]->SetModel(GetTopModel());
				//	meshes[(int)MeshType::Sphere]->Render(forwardSP); // Remember to change forwardSP etc accordingly
				//	PopModel();
					break;
				default:
					break;
			}
		}
	}
}