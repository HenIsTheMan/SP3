#include "Entity.h"

Entity::Entity(EntityType type, bool active, glm::vec3 pos, glm::vec3 scale, glm::vec4 rotate, glm::vec3 storeCamFront)
{
	this->type = type;
	this->active = active;
	this->pos = pos;
	this->scale = scale;
	this->rotate = rotate;
	this->storeCamFront = storeCamFront;
	lifeTime = 0.0;
}

Entity::~Entity()
{

}