#include "Entity.h"

Entity::Entity(EntityType type, bool active, glm::vec3 pos, glm::vec3 scale, glm::vec4 rotate, glm::vec3 storeCamFront, glm::vec3 storeCamPos)
{
	this->type = type;
	this->active = active;
	this->pos = pos;
	this->scale = scale;
	this->rotate = rotate;
	this->storeCamFront = storeCamFront;
	this->storeCamPos = storeCamPos;
}

Entity::~Entity()
{

}