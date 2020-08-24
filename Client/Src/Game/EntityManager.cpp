#include "EntityManager.h"
#include <iostream>

extern float dt;

EntityManager::EntityManager():
	entityList(),
	root(new CubeSection())
{
	root->SetActive(true);
	root->SetOrigin(glm::vec3(0.f));
	root->SetSize(glm::vec3(500.f, 500.f, 500.f));
	root->InitCubeSectionPool(80);
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

	const size_t& size = entityList.size();
	for(size_t i = 0; i < size; ++i){
		Entity* const& entity = entityList[i];
		if(entity && entity->active && entity->type != Entity::EntityType::NUM_TYPES){
			switch(entity->type){
				case Entity::EntityType::PLAYER:
					break;
				case Entity::EntityType::BULLET:
					break;
				case Entity::EntityType::STATIC_ENEMY:
					break;
				case Entity::EntityType::MOVING_ENEMY:
					break;
			}

			if(entity->vel != glm::vec3(0.f)){
				root->DetectAndResolveCollision(entity);
			}
			entity->vel += (entity->force / entity->mass) * dt;
			entity->pos += entity->vel * dt;
		}
	}
}

void EntityManager::RenderEntities(ShaderProg& SP){
	const size_t& size = entityList.size();
	for(size_t i = 0; i < size; ++i){
		Entity* entity = entityList[i];
		if(entity && entity->active){
			switch(entity->type){
				case Entity::EntityType::BULLET:
				case Entity::EntityType::MOVING_ENEMY:
					SP.Set1i("noNormals", 1);
					SP.Set1i("useCustomColour", 1);
					SP.Set1i("useCustomDiffuseTexIndex", 1);
					modelStack.PushModel({
						modelStack.Translate(entity->pos),
						modelStack.Rotate(entity->rotate),
						modelStack.Scale(entity->scale),
					});
						SP.Set4fv("customColour", entity->colour);
						SP.Set1i("customDiffuseTexIndex", entity->diffuseTexIndex);
						entity->mesh->SetModel(modelStack.GetTopModel());
						entity->mesh->Render(SP);
					modelStack.PopModel();
					SP.Set1i("useCustomDiffuseTexIndex", 0);
					SP.Set1i("useCustomColour", 0);
					SP.Set1i("noNormals", 0);
					break;
				case Entity::EntityType::STATIC_ENEMY:
					SP.Set1i("noNormals", 1);
					SP.Set1i("useCustomColour", 1);
					SP.Set1i("useCustomDiffuseTexIndex", 1);
					modelStack.PushModel({
						modelStack.Translate(entity->pos),
						modelStack.Rotate(entity->rotate),
						modelStack.Scale(entity->scale),
					});
						SP.Set4fv("customColour", entity->colour);
						SP.Set1i("customDiffuseTexIndex", entity->diffuseTexIndex);
						entity->model->SetModelForAll(modelStack.GetTopModel());
						entity->model->Render(SP);
					modelStack.PopModel();
					SP.Set1i("useCustomDiffuseTexIndex", 0);
					SP.Set1i("useCustomColour", 0);
					SP.Set1i("noNormals", 0);
					break;
				default:
					break;
			}
		}
	}
}