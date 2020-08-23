#include "Collision.h"

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

bool Collision:: IsSeparatedCubeCube(Entity* const& entity, Entity* const& instance){
	return false;
}

void Collision::CollisionSphereSphere(Entity* const& entity, Entity* const& instance){
	if(!IsSeparatedSphereSphere(entity, instance)){
		entity->vel = glm::vec3(0.f);
	}
}

void Collision::CollisionCubeCube(Entity* const& entity, Entity* const& instance){
}