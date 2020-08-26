#pragma once
#include <Core.h>
#include "../World/Cam.h"
#include "CubeSection.h"
#include "../World/ModelStack.h"

class EntityManager final: public Singleton<EntityManager>{
	friend class Singleton<EntityManager>;
public:
	struct UpdateParams final{ //Add on here if needed
		bool camCanMove;
		float playerCurrHealth;
		float playerCurrLives;
		int enemyCount;
		int score;
		glm::vec3 camPos;
		glm::vec3 camFront;
		glm::vec3 camTrueVel;
		glm::vec4 reticleColour;
		float yGround;
	};
	struct RenderParams final{ //Add on here if needed
		glm::vec3 camPos;
		uint depthDTexRefID;
		uint depthSTexRefID;
		Mesh* quadMesh;
	};

	~EntityManager(); //Default dtor

	const std::vector<Entity*>& GetEntityList() const;
	Entity* const& FetchEntity();

	void CreateEntities(const int& amt);
	void UpdateEntities(UpdateParams& params);
	void RenderEntities(ShaderProg& SP, RenderParams& params);
protected:
	EntityManager(); //Default ctor

	CubeSection* root; //Ptr to root quad
	ModelStack modelStack;
	std::vector<Entity*> entityList;
};