#pragma once
#include <Core.h>
#include "../World/Cam.h"
#include "CubeSection.h"
#include "../World/ModelStack.h"

class EntityManager final: public Singleton<EntityManager>{
	friend class Singleton<EntityManager>;
public:
	struct UpdateParams final{ //Add on here if needed
		bool camCanMove = true;
		float playerCurrHealth = 0.f;
		float playerCurrLives = 0.f;
		int enemyCount = 0;
		int score = 0;
		glm::vec3 camPos = glm::vec3(0.f);
		glm::vec3 camFront = glm::vec3(0.f);
		glm::vec3 camTrueVel = glm::vec3(0.f);
		glm::vec4 reticleColour = glm::vec4(0.f);
		Mesh* terrainMesh = nullptr;
		float terrainXScale = 1.f;
		float terrainYScale = 1.f;
		float terrainZScale = 1.f;
		float yGround = 0.f;
		Mesh* quadMesh = nullptr;
		bool addAmmo = false;
		int goldCoinAmt = 0;
		int silverCoinAmt = 0;
		int pinkCoinAmt = 0;
		int greenCoinAmt = 0;
		int blueCoinAmt = 0;
		bool takingDmg = false;
		float healthUp = false;
		float lifeUp = false;
		bool immune = false;
	};
	struct RenderParams final{ //Add on here if needed
		bool minimap = false;
		glm::vec3 camPos = glm::vec3(0.f);
		uint depthDTexRefID = 0;
		uint depthSTexRefID = 0;
		Mesh* quadMesh = nullptr;
	};

	~EntityManager(); //Default dtor

	const std::vector<Entity*>& GetEntityList() const;
	Entity* const& FetchEntity();

	void CreateEntities(const int& amt);
	void DeactivateAll();
	void UpdateEntities(UpdateParams& params);
	void RenderEntities(ShaderProg& SP, RenderParams& params);
protected:
	EntityManager(); //Default ctor

	float iFrames;
	CubeSection* root; //Ptr to root quad
	ModelStack modelStack;
	std::vector<Entity*> entityList;
};