#pragma once

#include "Core.h"
#include "Engine.h"

class Entity {
public:
	enum class EntityType
	{
		PLAYER,
<<<<<<< HEAD
		ENEMY,
=======
		STATIC_ENEMY,
		MOVING_ENEMY,
>>>>>>> origin/Trina-Lim
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
<<<<<<< HEAD
=======
	int health;
>>>>>>> origin/Trina-Lim

	// To be passed in as parameters for translate, scale, rotate functions in scene.cpp
	glm::vec3 pos;
	glm::vec3 scale;
	glm::vec4 rotate;
	glm::vec3 storeCamFront;
};