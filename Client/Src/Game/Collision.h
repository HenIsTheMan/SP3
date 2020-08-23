#pragma once
#include "Entity.h"

struct Collision final{
	static void DetectAndResolveCollision(Entity* const& entity, Entity* const& instance);
	static bool IsSeparatedSphereSphere(Entity* const& entity, Entity* const& instance);
	static bool IsSeparatedCubeCube(Entity* const& entity, Entity* const& instance);
	static void CollisionSphereSphere(Entity* const& entity, Entity* const& instance);
	static void CollisionCubeCube(Entity* const& entity, Entity* const& instance);
};