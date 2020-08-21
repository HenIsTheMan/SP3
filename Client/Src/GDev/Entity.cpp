#include "Entity.h"

Entity::Entity(EntityType type, bool active, bool rendered, glm::vec3 pos, glm::vec3 scale, glm::vec4 rotate)
{
	this->type = type;
	this->active = active;
	this->rendered = rendered;
	this->pos = pos;
	this->scale = scale;
	this->rotate = rotate;
}

Entity::~Entity()
{

}