#pragma once
#include <Core.h>
#include <Engine.h>

class Entity{
	friend class EntityManager;
	friend class Scene;
private:
	enum class EntityType{
		PLAYER,
		ENEMY,
		PARTICLE,
		PLAYER_LIVES,
		HEALTH_BAR,
		AMMO_BAR,
		BULLET,
		INV,
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
	glm::vec3 scale;
	Light* light;

	///Linear
	glm::vec3 pos;
	glm::vec3 vel;
	float mass;
	glm::vec3 force;

	///Angular
	glm::vec3 facingDir;
	float angularVel;
	float angularMass;
	glm::vec3 torque;
};