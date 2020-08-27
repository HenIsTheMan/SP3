#pragma once
#include <Core.h>
#include <Engine.h>

class Entity{
	friend struct Collision;
	friend class CubeSection;
	friend class EntityManager;
	friend class Scene;
private:
	enum class EntityType{
		BULLET,
		BULLET2,
		BULLET3,
		STATIC_ENEMY,
		MOVING_ENEMY,
		PARTICLE,
		PARTICLE2,
		PARTICLE3,
		FIRE,
		COIN_GOLD,
		COIN_SILVER,
		COIN_PINK,
		COIN_GREEN,
		COIN_BLUE,
		NUM_TYPES
	};

	Entity();
	virtual ~Entity() = default;

	///Misc
	EntityType type;
	bool active;
	float life;
	float maxLife;
	glm::vec4 colour;
	int diffuseTexIndex;
	glm::vec4 rotate;
	glm::vec3 scale;
	Light* light;
	Mesh* mesh;
	Model* model;

	///Linear
	glm::vec3 pos;
	glm::vec3 vel;
	float mass;
	glm::vec3 force;
};