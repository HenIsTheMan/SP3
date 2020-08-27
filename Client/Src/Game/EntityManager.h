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
		int enemyCount;
		int score;
		glm::vec3 camPos = glm::vec3(0.f);
		glm::vec3 camFront = glm::vec3(0.f);
		glm::vec3 camTrueVel = glm::vec3(0.f);
		glm::vec4 reticleColour = glm::vec4(0.f);
		float yGround = 0.f;
		Mesh* terrainMesh = nullptr;
		float terrainXScale = 1.f;
		float terrainYScale = 1.f;
		float terrainZScale = 1.f;
	};
	struct RenderParams final{ //Add on here if needed
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

	CubeSection* root; //Ptr to root quad
	ModelStack modelStack;
	std::vector<Entity*> entityList;
};