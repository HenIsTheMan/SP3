#include "EntityManager.h"
#include <iostream>

extern float dt;

EntityManager::EntityManager():
	iFrames(0.f),
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
	root->AddEntity(entityList.back());
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

void EntityManager::DeactivateAll(){
	for(Entity* const& entity: entityList){
		if(entity && entity->active){
			entity->active = false;
		}
	}
}

void EntityManager::UpdateEntities(UpdateParams& params){
	//assert(root->entityList->size() != 0);
	root->Deactivate();
	//assert(root->entityList->size() != 0);
	root->Partition();
	//assert(root->entityList->size() != 0);

	params.takingDmg = false;
	if(params.healthUp > 0.f){
		params.healthUp -= dt;
		params.healthUp = std::max(0.f, params.healthUp);
	}
	if(params.lifeUp > 0.f){
		params.lifeUp -= dt;
		params.lifeUp = std::max(0.f, params.lifeUp);
	}
	if(iFrames > 0.f){
		iFrames -= dt;
		iFrames = std::max(0.f, iFrames);
		if(!iFrames){
			params.immune = false;
		}
	}

	///Spawn rain particles
	static float tmp = (float)glfwGetTime();
	if((float)glfwGetTime() - tmp >= 0.2f){
		Entity* const& particle = FetchEntity();
		particle->type = Entity::EntityType::PARTICLE;
		particle->active = true;
		particle->life = 0.f;
		particle->maxLife = 0.f;
		particle->colour = glm::vec4(0.f, 1.f, 1.f, 1.f);
		particle->diffuseTexIndex = -1;
		particle->rotate = glm::vec4(0.f, 1.f, 0.f, 0.f);
		particle->scale = glm::vec3(1.f);
		particle->light = nullptr;
		particle->mesh = params.quadMesh;
		particle->pos = glm::vec3(PseudorandMinMax(-200.f, 200.f), 500.f, PseudorandMinMax(-200.f, 200.f));
		particle->vel = glm::vec3(0.f, -100.f, 0.f);
		particle->mass = 10.f;
		particle->force = glm::vec3(0.f);
		tmp = (float)glfwGetTime();
	}

	for(size_t i = 0; i < entityList.size(); ++i){
		Entity* const& entity = entityList[i];
		if(entity && entity->active){
			switch(entity->type){
				case Entity::EntityType::BULLET:
				case Entity::EntityType::BULLET2:
				case Entity::EntityType::BULLET3: {
					entity->life -= dt;
					if(entity->life <= 0){
						entity->active = false;
					}
					break;
				}
				case Entity::EntityType::MOVING_ENEMY: {
					if(entity->life <= 0.f){
						entity->active = false;
						--params.enemyCount;
						params.score += 20;
					}

					///Enemy movement
					entity->pos.y += float(sin(glfwGetTime())) / 10.f;
					entity->pos.y = std::max(entity->pos.y, params.terrainYScale
						* static_cast<Terrain*>(params.terrainMesh)->GetHeightAtPt(entity->pos.x / params.terrainXScale, entity->pos.z / params.terrainZScale) + entity->scale.x);
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
					if(glm::dot(displacementVec, displacementVec) <= (entity->scale.x + 5.f) * (entity->scale.x + 5.f)){
						if((glm::dot(relativeVelXY, -displacementVecXY) > 0.f
							|| glm::dot(relativeVelXZ, -displacementVecXZ) > 0.f
							|| glm::dot(relativeVelYZ, -displacementVecYZ) > 0.f)){
							params.camCanMove = false;
						}
						if(iFrames <= 0.f){
							params.takingDmg = true;
							params.playerCurrHealth -= 2.f;
						}
					}

					///Change reticle colour
					const float b = glm::dot(params.camFront, displacementVec);
					const float c = glm::dot(displacementVec, displacementVec) - entity->scale.x * entity->scale.x;
					if(b * b - c >= 0.f){
						params.reticleColour = glm::vec4(1.f, 1.f, 0.f, 1.f);
					}

					///Create particle blood splashes from shot enemy
					if(entity->isShot){
						for(int i = 0; i < 2; ++i){
							Entity* const& particle = FetchEntity();
							particle->type = Entity::EntityType::PARTICLE3;
							particle->active = true;
							particle->life = 1.f;
							particle->maxLife = 0.f;
							particle->colour = glm::vec4(1.f, 0.f, 0.f, 1.f);
							particle->diffuseTexIndex = -1;
							particle->rotate = glm::vec4(0.f, 1.f, 0.f, 0.f);
							particle->scale = glm::vec3(1.f);
							particle->light = nullptr;
							particle->mesh = params.quadMesh;
							particle->pos = entity->pos;
							particle->vel = glm::vec3(PseudorandMinMax(-20.f, 20.f), 30.f, PseudorandMinMax(-20.f, 20.f));
							particle->mass = .001f;
							particle->force = glm::vec3(0.f, -0.02f, 0.f);
						}
						entity->isShot = false;
					}

					break;
				}
				case Entity::EntityType::PARTICLE: {
					if(entity->pos.y < params.yGround){
						entity->active = false;
					}
					break;
				}
				case Entity::EntityType::PARTICLE2: {
					if(entity->pos.y > 200.f){
						entity->active = false;
					}
					if (entity->vel.x > 0 && entity->pos.x - 10.f > entity->originalPos.x) // Particle going to the right
						entity->vel.x = -entity->vel.x;
					else if (entity->vel.x < 0 && entity->pos.x + 10.f < entity->originalPos.x) // Particle going to the left
						entity->vel.x = -entity->vel.x;
					entity->scale *= glm::vec3(0.992f); // As the particles travel upwards, they get smaller
					break;
				}
				case Entity::EntityType::PARTICLE3: {
					entity->life -= dt;
					if(entity->life <= 0.f){
						entity->active = false;
					}
					break;
				}
				case Entity::EntityType::AMMO_PICKUP: {
					///Check for collision with cam
					const glm::vec3& displacementVec = params.camPos - entity->pos;
					if(glm::dot(displacementVec, displacementVec) <= (entity->scale.x + 5.f) * (entity->scale.x + 5.f)){
						params.addAmmo = true;
						params.weaponType = 1;
						--params.ammoPickupAmt;
						entity->active = false;
					}
					break;
				}
				case Entity::EntityType::AMMO_PICKUP2: {
					///Check for collision with cam
					const glm::vec3& displacementVec = params.camPos - entity->pos;
					if(glm::dot(displacementVec, displacementVec) <= (entity->scale.x + 5.f) * (entity->scale.x + 5.f)){
						params.addAmmo = true;
						params.weaponType = 2;
						--params.ammoPickup2Amt;
						entity->active = false;
					}
					break;
				}
				case Entity::EntityType::FIRE: {
					///Check for collision with cam
					const glm::vec3& displacementVec = params.camPos - entity->pos;
					if(glm::dot(displacementVec, displacementVec) <= (entity->scale.x + 5.f) * (entity->scale.x + 5.f)){
						if(iFrames <= 0.f){
							params.takingDmg = true;
							params.playerCurrHealth -= 5.f;
						}
					}

					///Spawn fire particles
					if(entity->spawnSmokeBT <= (float)glfwGetTime()){
						Entity* const& particle = FetchEntity();
						particle->type = Entity::EntityType::PARTICLE2;
						particle->active = true;
						particle->life = 0.f;
						particle->maxLife = 0.f;
						particle->colour = glm::vec4(glm::vec3(.4f), 1.f);
						particle->diffuseTexIndex = -1;
						particle->rotate = glm::vec4(0.f, 1.f, 0.f, 0.f);
						particle->scale = glm::vec3(3.f);
						particle->light = nullptr;
						particle->mesh = params.quadMesh;
						particle->pos = entity->pos + glm::vec3(0.f, 50.f, 0.f);
						particle->vel = glm::vec3(7.f, 15.f, 0.f);
						particle->mass = .0001f;
						particle->force = glm::vec3(0.f);
						particle->originalPos = particle->pos;
						particle->originalScale = particle->scale;
						particle->originalVel = particle->vel;
						entity->spawnSmokeBT = (float)glfwGetTime() + .5f;
					}

					break;
				}
				case Entity::EntityType::COIN_GOLD: {
					///Check for collision with cam
					const glm::vec3& displacementVec = params.camPos - entity->pos;
					if(glm::dot(displacementVec, displacementVec) <= (entity->scale.x + 5.f) * (entity->scale.x + 5.f)){
						params.score += 100;
						--params.goldCoinAmt;
						entity->active = false;
					}

					break;
				}
				case Entity::EntityType::COIN_SILVER: {
					///Check for collision with cam
					const glm::vec3& displacementVec = params.camPos - entity->pos;
					if(glm::dot(displacementVec, displacementVec) <= (entity->scale.x + 5.f) * (entity->scale.x + 5.f)){
						params.score += 10;
						--params.silverCoinAmt;
						entity->active = false;
					}

					break;
				}
				case Entity::EntityType::COIN_PINK: {
					///Check for collision with cam
					const glm::vec3& displacementVec = params.camPos - entity->pos;
					if(glm::dot(displacementVec, displacementVec) <= (entity->scale.x + 5.f) * (entity->scale.x + 5.f)){
						++params.playerCurrLives;
						--params.pinkCoinAmt;
						params.lifeUp = 1.f;
						entity->active = false;
					}

					break;
				}
				case Entity::EntityType::COIN_GREEN: {
					///Check for collision with cam
					const glm::vec3& displacementVec = params.camPos - entity->pos;
					if(glm::dot(displacementVec, displacementVec) <= (entity->scale.x + 5.f) * (entity->scale.x + 5.f)){
						params.playerCurrHealth += 50.f;
						--params.greenCoinAmt;
						params.healthUp = 1.f;
						entity->active = false;
					}

					break;
				}
				case Entity::EntityType::COIN_BLUE: {
					///Check for collision with cam
					const glm::vec3& displacementVec = params.camPos - entity->pos;
					if(glm::dot(displacementVec, displacementVec) <= (entity->scale.x + 5.f) * (entity->scale.x + 5.f)){
						if(iFrames < 10.f){
							iFrames = 10.f;
						}
						params.immune = true;
						--params.blueCoinAmt;
						entity->active = false;
					}

					break;
				}
			}
			if(entity->type == Entity::EntityType::BULLET
				|| entity->type == Entity::EntityType::BULLET2
				|| entity->type == Entity::EntityType::BULLET3
			){
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
	std::map<int, Entity*> entityListForSorting;
	for(size_t i = 0; i < size; ++i){
		Entity* const& entity = entityList[i];
		if(entity && entity->active){
			glm::vec3 displacementVec = entity->pos - params.camPos;
			if(glm::dot(displacementVec, params.camFront) > 0.f){
				entityListForSorting[(const int)glm::dot(displacementVec, displacementVec)] = entity;
			}
		}
	}

	for(std::map<int, Entity*>::reverse_iterator iter = entityListForSorting.rbegin(); iter != entityListForSorting.rend(); ++iter){
		Entity* const& entity = iter->second;
		//if(entity && entity->active){
			switch(entity->type){
				case Entity::EntityType::BULLET:
				case Entity::EntityType::BULLET2:
				case Entity::EntityType::BULLET3: {
					if(params.minimap){
						continue;
					}

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
				case Entity::EntityType::PARTICLE3: {
					if(params.minimap){
						continue;
					}

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
						params.quadMesh->SetModel(modelStack.GetTopModel());
						params.quadMesh->Render(SP);
						SP.Set1i("useCustomDiffuseTexIndex", 0);
						SP.Set1i("useCustomColour", 0);
						SP.Set1i("noNormals", 0);
					modelStack.PopModel();
					break;
				}
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
				case Entity::EntityType::AMMO_PICKUP: {
					if(params.minimap){
						continue;
					}

					modelStack.PushModel({
						modelStack.Translate(entity->pos),
						modelStack.Scale(entity->scale),
					});
						SP.UseTex(params.depthDTexRefID, "dDepthTexSampler");
						SP.UseTex(params.depthSTexRefID, "sDepthTexSampler");
						SP.Set1i("useCustomColour", 1);
						SP.Set1i("useCustomDiffuseTexIndex", 1);
						SP.Set4fv("customColour", glm::vec4(.8f, .4f, .1f, 1.f));
						SP.Set1i("customDiffuseTexIndex", entity->diffuseTexIndex);
						entity->mesh->SetModel(modelStack.GetTopModel());
						entity->mesh->Render(SP);
						SP.Set1i("useCustomColour", 0);
						SP.Set1i("useCustomDiffuseTexIndex", 0);
					modelStack.PopModel();
					break;
				}
				case Entity::EntityType::AMMO_PICKUP2: {
					if(params.minimap){
						continue;
					}

					modelStack.PushModel({
						modelStack.Translate(entity->pos),
						modelStack.Scale(entity->scale),
					});
						SP.UseTex(params.depthDTexRefID, "dDepthTexSampler");
						SP.UseTex(params.depthSTexRefID, "sDepthTexSampler");
						SP.Set1i("useCustomColour", 1);
						SP.Set1i("useCustomDiffuseTexIndex", 1);
						SP.Set4fv("customColour", glm::vec4(0.f, 0.f, .7f, 1.f));
						SP.Set1i("customDiffuseTexIndex", entity->diffuseTexIndex);
						entity->mesh->SetModel(modelStack.GetTopModel());
						entity->mesh->Render(SP);
						SP.Set1i("useCustomColour", 0);
						SP.Set1i("useCustomDiffuseTexIndex", 0);
					modelStack.PopModel();
					break;
				}
				case Entity::EntityType::FIRE: {
					if(params.minimap){
						continue;
					}

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
				}
				case Entity::EntityType::COIN_GOLD:
				case Entity::EntityType::COIN_SILVER:
				case Entity::EntityType::COIN_PINK:
				case Entity::EntityType::COIN_GREEN:
				case Entity::EntityType::COIN_BLUE: {
					if(params.minimap){
						continue;
					}

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
				}
				default:
					break;
			}
		//}
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