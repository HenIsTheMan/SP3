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

void EntityManager::UpdateEntities(UpdateParams& params){
	root->Deactivate();
	//root->Partition();

	const size_t& size = entityList.size();
	for(size_t i = 0; i < size; ++i){
		Entity* const& entity = entityList[i];
		if(entity && entity->active && entity->type != Entity::EntityType::NUM_TYPES){
			switch(entity->type){
				case Entity::EntityType::PLAYER:
					break;
				case Entity::EntityType::BULLET:
					break;
				case Entity::EntityType::STATIC_ENEMY: {
					glm::vec3 displacementVec = params.camPos - entity->pos;
					const float b = glm::dot(params.camFront, displacementVec);
					const float c = glm::dot(displacementVec, displacementVec) - entity->scale.x * entity->scale.x;
					if(b * b - c >= 0.f){
						params.reticleColour = glm::vec4(1.f, 1.f, 0.f, 1.f);
					}
					break;
				}
				case Entity::EntityType::MOVING_ENEMY: {
					glm::vec3 displacementVec = params.camPos - entity->pos;
					const float b = glm::dot(params.camFront, displacementVec);
					const float c = glm::dot(displacementVec, displacementVec) - entity->scale.x * entity->scale.x;
					if(b * b - c >= 0.f){
						params.reticleColour = glm::vec4(1.f, 1.f, 0.f, 1.f);
					}
					break;
				}
				case Entity::EntityType::FIRE:
					static_cast<SpriteAni*>(entity->mesh)->Update();
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

void EntityManager::RenderEntities(ShaderProg& SP, RenderParams& params){
	const size_t& size = entityList.size();
	for(size_t i = 0; i < size; ++i){
		Entity* entity = entityList[i];
		if(entity && entity->active){
			switch(entity->type){
				case Entity::EntityType::BULLET:
				case Entity::EntityType::MOVING_ENEMY:
					SP.Set1i("useCustomColour", 1);
					SP.Set1i("useCustomDiffuseTexIndex", 1);
					modelStack.PushModel({
						modelStack.Translate(entity->pos),
						modelStack.Rotate(entity->rotate),
						modelStack.Scale(entity->scale),
					});
						SP.UseTex(params.depthDTexRefID, "dDepthTexSampler");
						SP.UseTex(params.depthSTexRefID, "sDepthTexSampler");
						SP.Set4fv("customColour", entity->colour);
						SP.Set1i("customDiffuseTexIndex", entity->diffuseTexIndex);
						entity->mesh->SetModel(modelStack.GetTopModel());
						entity->mesh->Render(SP);
					modelStack.PopModel();
					SP.Set1i("useCustomDiffuseTexIndex", 0);
					SP.Set1i("useCustomColour", 0);
					break;
				case Entity::EntityType::STATIC_ENEMY:
					SP.Set1i("noNormals", 1);
					modelStack.PushModel({
						modelStack.Translate(entity->pos),
						modelStack.Rotate(entity->rotate),
						modelStack.Scale(entity->scale),
					});
						SP.UseTex(params.depthDTexRefID, "dDepthTexSampler");
						SP.UseTex(params.depthSTexRefID, "sDepthTexSampler");
						entity->model->SetModelForAll(modelStack.GetTopModel());
						entity->model->Render(SP);
					modelStack.PopModel();
					SP.Set1i("noNormals", 0);
					break;
				case Entity::EntityType::FIRE:
					modelStack.PushModel({
						modelStack.Translate(entity->pos),
						modelStack.Rotate(glm::vec4(0.f, 1.f, 0.f, glm::degrees(atan2(params.camPos.x - entity->pos.x, params.camPos.z - entity->pos.z)))),
						modelStack.Scale(entity->scale),
					});
						SP.UseTex(params.depthDTexRefID, "dDepthTexSampler");
						SP.UseTex(params.depthSTexRefID, "sDepthTexSampler");
						SP.Set1i("noNormals", 1);
						entity->mesh->SetModel(modelStack.GetTopModel());
						entity->mesh->Render(SP);
						SP.Set1i("noNormals", 0);
					modelStack.PopModel();
					break;
				default:
					break;
			}
		}
	}
}