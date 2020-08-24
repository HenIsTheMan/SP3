#include "Entity.h"

///Default values here are dummy values so all values of member vars shld be set explicitly
Entity::Entity():
	type(EntityType::NUM_TYPES),
	active(false),
	life(0.f),
	maxLife(0.f),
	colour(glm::vec4(.7f, .4f, .1f, 1.f)),
	diffuseTexIndex(-1),
	rotate(glm::vec4(0.f, 1.f, 0.f, 0.f)),
	scale(glm::vec3(1.f)),
	light(nullptr),
	mesh(nullptr),
	model(nullptr),
	pos(glm::vec3(0.f)),
	vel(glm::vec3(0.f)),
	mass(0.f),
	force(glm::vec3(0.f))
{
}