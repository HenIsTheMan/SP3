#pragma once
#include <Core.h>
#include "../World/Cam.h"
#include "CubeSection.h"

class EntityManager final: public Singleton<EntityManager>{
	friend class Singleton<EntityManager>;
public:
	struct UpdateParams final{ //Add on here if needed
	};

	~EntityManager(); //Default dtor

	const std::vector<Entity*>& GetEntityList() const;
	Entity* const& FetchEntity();

	void CreateEntities(const int& amt);
	void UpdateEntities(const UpdateParams& params);
	void RenderEntities(const ShaderProg& SP);
protected:
	EntityManager(); //Default ctor
	CubeSection* root; //Ptr to root quad
	std::vector<Entity*> entityList;
};