#pragma once

#include "Core.h"
#include "Engine.h"

class Entity {
public:
	enum class EntityType
	{
		ENEMY,
		//STRUCTURE,
		//COLLECTIBLE,
		PARTICLE,

		NUM_TYPES
	};

	Entity(EntityType type, bool active, bool rendered, glm::vec3 pos, glm::vec3 scale, glm::vec4 rotate);
	virtual ~Entity();

	EntityType type; // Entity type

	bool active; // True when entity is being rendered, used to know which particles to reuse
	bool rendered; // True when entity has been rendered, used to ensure reuseability of the particles

	// To be passed in as parameters for translate, scale, rotate functions in scene.cpp
	glm::vec3 pos;
	glm::vec3 scale;
	glm::vec4 rotate;
};