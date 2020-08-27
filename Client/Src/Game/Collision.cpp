#include "Collision.h"

glm::vec3 responseVec = glm::vec3(0.f);

void Collision::DetectAndResolveCollision(Entity* const& entity, Entity* const& instance){
	if(entity->mesh->GetMeshType() == instance->mesh->GetMeshType()){
		switch(entity->mesh->GetMeshType()){
			case Mesh::MeshType::Sphere:
				CollisionSphereSphere(entity, instance);
				break;
			case Mesh::MeshType::Cube:
				CollisionCubeCube(entity, instance);
				break;
		}
	}
}

bool Collision::IsSeparatedSphereSphere(Entity* const& entity, Entity* const& instance){
	const glm::vec3& relativeVel = entity->vel - instance->vel;
	const glm::vec3& relativeVelXY = glm::vec3(relativeVel.x, relativeVel.y, 0.f);
	const glm::vec3& relativeVelXZ = glm::vec3(relativeVel.x, 0.f, relativeVel.z);
	const glm::vec3& relativeVelYZ = glm::vec3(0.f, relativeVel.y, relativeVel.z);
	const glm::vec3& displacementVec = entity->pos - instance->pos;
	const glm::vec3& displacementVecXY = glm::vec3(displacementVec.x, displacementVec.y, 0.f);
	const glm::vec3& displacementVecXZ = glm::vec3(displacementVec.x, 0.f, displacementVec.z);
	const glm::vec3& displacementVecYZ = glm::vec3(0.f, displacementVec.y, displacementVec.z);
	return !(glm::dot(displacementVec, displacementVec) <= (entity->scale.x + instance->scale.x) * (entity->scale.x + instance->scale.x)
		&& (glm::dot(relativeVelXY, -displacementVecXY) > 0.f
		|| glm::dot(relativeVelXZ, -displacementVecXZ) > 0.f
		|| glm::dot(relativeVelYZ, -displacementVecYZ) > 0.f));
}

bool Collision::IsSeparatedCubeCube(Entity* const& entity, Entity* const& instance){
	glm::vec3 entityPts[8]{
		glm::vec3(entity->scale.x, entity->scale.y, entity->scale.z),
		glm::vec3(entity->scale.x, entity->scale.y, -entity->scale.z),
		glm::vec3(-entity->scale.x, entity->scale.y, entity->scale.z),
		glm::vec3(-entity->scale.x, entity->scale.y, -entity->scale.z),
		glm::vec3(entity->scale.x, -entity->scale.y, entity->scale.z),
		glm::vec3(entity->scale.x, -entity->scale.y, -entity->scale.z),
		glm::vec3(-entity->scale.x, -entity->scale.y, entity->scale.z),
		glm::vec3(-entity->scale.x, -entity->scale.y, -entity->scale.z),
	};
	glm::vec3 instancePts[8]{
		glm::vec3(instance->scale.x, instance->scale.y, instance->scale.z),
		glm::vec3(instance->scale.x, instance->scale.y, -instance->scale.z),
		glm::vec3(-instance->scale.x, instance->scale.y, instance->scale.z),
		glm::vec3(-instance->scale.x, instance->scale.y, -instance->scale.z),
		glm::vec3(instance->scale.x, -instance->scale.y, instance->scale.z),
		glm::vec3(instance->scale.x, -instance->scale.y, -instance->scale.z),
		glm::vec3(-instance->scale.x, -instance->scale.y, instance->scale.z),
		glm::vec3(-instance->scale.x, -instance->scale.y, -instance->scale.z),
	};
	for(short i = 0; i < 8; ++i){
		entityPts[i] = entity->pos + glm::vec3(glm::rotate(glm::mat4(1.f), glm::radians(entity->rotate.w), glm::vec3(entity->rotate)) * glm::vec4(entityPts[i], 1.f));
	}
	for(short i = 0; i < 8; ++i){
		instancePts[i] = instance->pos + glm::vec3(glm::rotate(glm::mat4(1.f), glm::radians(instance->rotate.w), glm::vec3(instance->rotate)) * glm::vec4(instancePts[i], 1.f));
	}

	glm::vec3 front1 = glm::vec3(glm::rotate(glm::mat4(1.f), glm::radians(entity->rotate.w), glm::vec3(entity->rotate)) * glm::vec4(glm::vec3(0.f, 0.f, 1.f), 1.f));
	glm::vec3 right1 = glm::vec3(glm::rotate(glm::mat4(1.f), glm::radians(entity->rotate.w), glm::vec3(entity->rotate)) * glm::vec4(glm::vec3(1.f, 0.f, 0.f), 1.f));
	glm::vec3 up1 = glm::vec3(glm::rotate(glm::mat4(1.f), glm::radians(entity->rotate.w), glm::vec3(entity->rotate)) * glm::vec4(glm::vec3(0.f, 1.f, 0.f), 1.f));
	glm::vec3 front2 = glm::vec3(glm::rotate(glm::mat4(1.f), glm::radians(instance->rotate.w), glm::vec3(instance->rotate)) * glm::vec4(glm::vec3(0.f, 0.f, 1.f), 1.f));
	glm::vec3 right2 = glm::vec3(glm::rotate(glm::mat4(1.f), glm::radians(instance->rotate.w), glm::vec3(instance->rotate)) * glm::vec4(glm::vec3(1.f, 0.f, 0.f), 1.f));
	glm::vec3 up2 = glm::vec3(glm::rotate(glm::mat4(1.f), glm::radians(instance->rotate.w), glm::vec3(instance->rotate)) * glm::vec4(glm::vec3(0.f, 1.f, 0.f), 1.f));

	glm::vec3 axes[]{
		front1,
		right1,
		up1,
		front2,
		right2,
		up2,
		-front1,
		-right1,
		-up1,
		-front2,
		-right2,
		-up2,
		glm::cross(front1, front2),
		glm::cross(front1, right2),
		glm::cross(front1, up2),
		glm::cross(right1, front2),
		glm::cross(right1, right2),
		glm::cross(right1, up2),
		glm::cross(up1, front2),
		glm::cross(up1, right2),
		glm::cross(up1, up2),
		-glm::cross(front1, front2),
		-glm::cross(front1, right2),
		-glm::cross(front1, up2),
		-glm::cross(right1, front2),
		-glm::cross(right1, right2),
		-glm::cross(right1, up2),
		-glm::cross(up1, front2),
		-glm::cross(up1, right2),
		-glm::cross(up1, up2),
	};

	bool separated = false;
	const size_t& size = sizeof(axes) / sizeof(axes[0]);
	for(size_t i = 0; i < size; ++i){
		const glm::vec3& axis = axes[i];
		if(axis == glm::vec3(0.f)){
			return false;
		}

		float aMin = FLT_MAX;
		float aMax = FLT_MIN;
		float bMin = FLT_MAX;
		float bMax = FLT_MIN;

		for(short i = 0; i < 8; ++i){
			float aDist = glm::dot(entityPts[i], axis);
			aMin = aDist < aMin ? aDist : aMin;
			aMax = aDist > aMax ? aDist : aMax;
			float bDist = glm::dot(instancePts[i], axis);
			bMin = bDist < bMin ? bDist : bMin;
			bMax = bDist > bMax ? bDist : bMax;
		}

		///Test for overlap
		float longSpan = std::max(aMax, bMax) - std::min(aMin, bMin);
		float sumSpan = aMax - aMin + bMax - bMin;

		if(responseVec == glm::vec3(0.f) || (sumSpan > longSpan && glm::length(responseVec) > sumSpan - longSpan)){
			responseVec = (sumSpan - longSpan) * axis;
		}
		separated |= (sumSpan <= longSpan);
	}
	return separated;
}

void Collision::CollisionSphereSphere(Entity* const& entity, Entity* const& instance){
	if(!IsSeparatedSphereSphere(entity, instance)){
		int temp = 0;
		if (entity->type == Entity::EntityType::BULLET ||
			entity->type == Entity::EntityType::BULLET2 ||
			entity->type == Entity::EntityType::BULLET3) {
			switch (entity->type) {
				case Entity::EntityType::BULLET:
					entity->active = false;
					temp = 1;
					break;
				case Entity::EntityType::BULLET2:
					entity->active = false;
					temp = 2;
					break;
				case Entity::EntityType::BULLET3:
					entity->active = false;
					temp = 3;
					break;
			}
			if (instance->type == Entity::EntityType::MOVING_ENEMY) {
				switch (temp)
				{
					case 1:
						instance->life -= 2.f;
						break;
					case 2:
						instance->life -= 5.f;
						break;
					case 3:
						instance->life -= 20.f;
						break;
				}
			}
		} else if (instance->type == Entity::EntityType::BULLET ||
			instance->type == Entity::EntityType::BULLET2 ||
			instance->type == Entity::EntityType::BULLET3) {
			switch (instance->type) {
				case Entity::EntityType::BULLET:
					instance->active = false;
					temp = 1;
					break;
				case Entity::EntityType::BULLET2:
					instance->active = false;
					temp = 2;
					break;
				case Entity::EntityType::BULLET3:
					instance->active = false;
					temp = 3;
					break;
			}
			if (entity->type == Entity::EntityType::MOVING_ENEMY) {
				switch (temp)
				{
					case 1:
						entity->life -= 2.f;
						break;
					case 2:
						entity->life -= 5.f;
						break;
					case 3:
						entity->life -= 20.f;
						break;
				}
			}
		}
	}
}

void Collision::CollisionCubeCube(Entity* const& entity, Entity* const& instance){
	responseVec = glm::vec3(0.f);
	if(!IsSeparatedCubeCube(entity, instance) && glm::dot(entity->vel, responseVec) < 0.f){
		entity->pos += responseVec;
		entity->vel = glm::vec3(0.f);
	}
}

///Prevent double collision??