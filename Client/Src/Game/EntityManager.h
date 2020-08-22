#pragma once

#include "Core.h"
#include "Engine.h"
#include "../World/Cam.h"
#include "Entity.h"

class EntityManager final: public Singleton<EntityManager>{
	friend class Singleton<EntityManager>;
public:
	struct UpdateParams final{ //Add on here if needed
	};

	~EntityManager();

	void CreateEntities(const int& amt);
	Entity* const& FetchEntity();
	void UpdateEntities(const UpdateParams& params);
	void RenderEntities(const ShaderProg& SP);

	const std::vector<Entity*>& GetEntityList() const;
protected:
	EntityManager();
	std::vector<Entity*> entityList;
};