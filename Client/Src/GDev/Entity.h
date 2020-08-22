#pragma once

#include "Core.h"
#include "Engine.h"

class Entity {
public:
	enum class EntityType
	{
		PLAYER,
		STATIC_ENEMY,
		//STRUCTURE,
		//COLLECTIBLE,
		PARTICLE,
		PLAYERLIVES,
		HEALTHBAR,
		AMMOBAR,
		BULLET,
		INVENTORY,

		NUM_TYPES
	};

	Entity(EntityType type, bool active, glm::vec3 pos, glm::vec3 scale, glm::vec4 rotate, glm::vec3 storeCamFront);
	virtual ~Entity();

	EntityType type; // Entity type
	bool active; // True when entity is being rendered, used to know which particles to reuse
	int health;

	// To be passed in as parameters for translate, scale, rotate functions in scene.cpp
	glm::vec3 pos;
	glm::vec3 scale;
	glm::vec4 rotate;
	glm::vec3 storeCamFront;
};