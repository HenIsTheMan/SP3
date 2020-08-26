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
					///Enemy movement
					entity->pos.y += float(sin(glfwGetTime())) / 5.f;
					if(glm::length(params.camPos - entity->pos) >= 50.f){
						entity->vel = glm::vec3(glm::rotate(glm::mat4(1.f), glm::radians(PseudorandMinMax(-10.f, 10.f)), {0.f, 1.f, 0.f}) *
							glm::vec4(glm::normalize((params.camPos - entity->pos)), 0.f)) * 20.f;
					} else{
						entity->vel = glm::vec3(0.f);
					}

					///Check for collision with cam
					const glm::vec3& relativeVel = params.camTrueVel - entity->vel;
					const glm::vec3& relativeVelXY = glm::vec3(relativeVel.x, relativeVel.y, 0.f);
					const glm::vec3& relativeVelXZ = glm::vec3(relativeVel.x, 0.f, relativeVel.z);
					const glm::vec3& relativeVelYZ = glm::vec3(0.f, relativeVel.y, relativeVel.z);
					const glm::vec3& displacementVec = params.camPos - entity->pos;
					const glm::vec3& displacementVecXY = glm::vec3(displacementVec.x, displacementVec.y, 0.f);
					const glm::vec3& displacementVecXZ = glm::vec3(displacementVec.x, 0.f, displacementVec.z);
					const glm::vec3& displacementVecYZ = glm::vec3(0.f, displacementVec.y, displacementVec.z);
					if(glm::dot(displacementVec, displacementVec) <= (entity->scale.x + 5.f) * (entity->scale.x + 5.f)
						&& (glm::dot(relativeVelXY, -displacementVecXY) > 0.f
						|| glm::dot(relativeVelXZ, -displacementVecXZ) > 0.f
						|| glm::dot(relativeVelYZ, -displacementVecYZ) > 0.f)){
						params.camCanMove = false;
						params.playerCurrHealth -= 2.f;
					}

					///Change reticle colour
					const float b = glm::dot(params.camFront, displacementVec);
					const float c = glm::dot(displacementVec, displacementVec) - entity->scale.x * entity->scale.x;
					if(b * b - c >= 0.f){
						params.reticleColour = glm::vec4(1.f, 1.f, 0.f, 1.f);
					}
					break;
				}
				case Entity::EntityType::FIRE:
					break;
				case Entity::EntityType::PARTICLE:
					// Don't know how to make it check with the terrain
					// Change the values accordingly to make it nicer
					if (entity->pos.y < 100.f)
						entity->pos.y = 350.f;
					break;
				case Entity::EntityType::PARTICLE2:
					if (entity->pos.y > 350.f) // 50.f more than the pos at first
						entity->pos.y = 300.f;
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
	//params.quadMesh->ClearModelMats();
	//bool particle1 = false;
	//glm::vec4 rainColour = glm::vec4(1.f);

	const size_t& size = entityList.size();
	for(size_t i = 0; i < size; ++i){
		Entity* entity = entityList[i];
		if(entity && entity->active){
			switch(entity->type){
				case Entity::EntityType::BULLET: {
					entity->life -= dt;
					if (entity->life <= 0.f) {
						entity->active = false;
					}
					else {
						SP.UseTex(params.depthDTexRefID, "dDepthTexSampler");
						SP.UseTex(params.depthSTexRefID, "sDepthTexSampler");
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
					}
					break;
				}
				case Entity::EntityType::STATIC_ENEMY: {
					SP.UseTex(params.depthDTexRefID, "dDepthTexSampler");
					SP.UseTex(params.depthSTexRefID, "sDepthTexSampler");
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
					break;
				}
				case Entity::EntityType::MOVING_ENEMY: {
					SP.UseTex(params.depthDTexRefID, "dDepthTexSampler");
					SP.UseTex(params.depthSTexRefID, "sDepthTexSampler");
					SP.Set1i("noNormals", 1);
					SP.Set1i("useCustomColour", 1);
					SP.Set1i("useCustomDiffuseTexIndex", 1);
					modelStack.PushModel({
						modelStack.Translate(entity->pos),
						modelStack.Rotate(entity->rotate),
						modelStack.Scale(entity->scale),
					});
						SP.Set1i("useCustomDiffuseTexIndex", 0);
						SP.Set4fv("customColour", glm::vec4(entity->colour));
						entity->model->SetModelForAll(modelStack.GetTopModel());
						entity->model->Render(SP);
						SP.Set1i("useCustomDiffuseTexIndex", 1);

						SP.Set1i("noNormals", 1);
						SP.Set1i("customDiffuseTexIndex", -1);
						modelStack.PushModel({
							modelStack.Translate(glm::vec3(0.f, 1.5f, 0.f)),
							modelStack.Rotate(glm::vec4(0.f, 1.f, 0.f, glm::degrees(atan2(params.camPos.x - entity->pos.x, params.camPos.z - entity->pos.z)))),
							modelStack.Scale(glm::vec3(.5f, .1f, .1f)),
						});
							SP.Set4fv("customColour", glm::vec4(glm::vec3(.3f), 1.f));
							params.quadMesh->SetModel(modelStack.GetTopModel());
							params.quadMesh->Render(SP);
						modelStack.PopModel();

						modelStack.PushModel({
							modelStack.Translate(glm::vec3(0.f, 1.5f, 0.f)),
							modelStack.Rotate(glm::vec4(0.f, 1.f, 0.f, glm::degrees(atan2(params.camPos.x - entity->pos.x, params.camPos.z - entity->pos.z)))),
							modelStack.Scale(glm::vec3(.5f, .1f, .1f)),
						});
							modelStack.PushModel({
								modelStack.Translate(glm::vec3((entity->life - entity->maxLife) / entity->maxLife, 0.f, .08f)),
								modelStack.Scale(glm::vec3(entity->life / entity->maxLife, 1.f, 1.f)),
							});
								SP.Set4fv("customColour", glm::vec4(0.f, 1.f, 0.f, 1.f));
								params.quadMesh->SetModel(modelStack.GetTopModel());
								params.quadMesh->Render(SP);
							modelStack.PopModel();
						modelStack.PopModel();
						SP.Set1i("noNormals", 0);
					modelStack.PopModel();
					SP.Set1i("useCustomDiffuseTexIndex", 0);
					SP.Set1i("useCustomColour", 0);
					SP.Set1i("noNormals", 0);
					break;
				}
				case Entity::EntityType::PARTICLE:
				case Entity::EntityType::PARTICLE2:
				case Entity::EntityType::PARTICLE3:
					modelStack.PushModel({
						modelStack.Translate(entity->pos),
						modelStack.Rotate(glm::vec4(0.f, 1.f, 0.f, glm::degrees(atan2(params.camPos.x - entity->pos.x, params.camPos.z - entity->pos.z)))),
						modelStack.Scale(entity->scale),
					});
						SP.UseTex(params.depthDTexRefID, "dDepthTexSampler");
						SP.UseTex(params.depthSTexRefID, "sDepthTexSampler");
						SP.Set1i("noNormals", 1);
						SP.Set1i("useCustomColour", 1);
						SP.Set1i("useCustomDiffuseTexIndex", 1);
						SP.Set4fv("customColour", entity->colour);
						SP.Set1i("customDiffuseTexIndex", entity->diffuseTexIndex);
						entity->mesh->SetModel(modelStack.GetTopModel());
						entity->mesh->Render(SP);
						SP.Set1i("useCustomDiffuseTexIndex", 0);
						SP.Set1i("useCustomColour", 0);
						SP.Set1i("noNormals", 0);
					modelStack.PopModel();
					break;
				//case Entity::EntityType::PARTICLE:
				//	modelStack.PushModel({
				//		modelStack.Translate(entity->pos),
				//		modelStack.Rotate(glm::vec4(0.f, 1.f, 0.f, glm::degrees(atan2(params.camPos.x - entity->pos.x, params.camPos.z - entity->pos.z)))),
				//		modelStack.Scale(entity->scale),
				//	});
				//		entity->mesh->AddModelMat(modelStack.GetTopModel());
				//	modelStack.PopModel();

				//	rainColour = entity->colour;
				//	particle1 = true;
				//	break;
				case Entity::EntityType::FIRE:
					modelStack.PushModel({
						modelStack.Translate(entity->pos + glm::vec3(0.f, entity->scale.y / 2.f, 0.f)),
						modelStack.Rotate(glm::vec4(0.f, 1.f, 0.f, glm::degrees(atan2(params.camPos.x - entity->pos.x, params.camPos.z - entity->pos.z)))),
						modelStack.Scale(glm::vec3(entity->scale.x, entity->scale.y * 2.f, entity->scale.z)),
					});
						SP.UseTex(params.depthDTexRefID, "dDepthTexSampler");
						SP.UseTex(params.depthSTexRefID, "sDepthTexSampler");
						SP.Set1i("noNormals", 1);
						entity->mesh->SetModel(modelStack.GetTopModel());
						entity->mesh->Render(SP);
						SP.Set1i("noNormals", 0);
					modelStack.PopModel();
					break;
				case Entity::EntityType::COIN_GOLD:
				case Entity::EntityType::COIN_SILVER:
				case Entity::EntityType::COIN_PINK:
				case Entity::EntityType::COIN_GREEN:
				case Entity::EntityType::COIN_BLUE:
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

	//if(particle1){
	//	SP.Set1i("useCustomColour", 1);
	//	SP.UseTex(params.depthDTexRefID, "dDepthTexSampler");
	//	SP.UseTex(params.depthSTexRefID, "sDepthTexSampler");
	//	SP.Set4fv("customColour", rainColour);
	//	params.quadMesh->InstancedRender(SP);
	//	SP.Set1i("useCustomColour", 0);
	//}
	//std::cout << quadMesh->modelMats.size() << std::endl;
}