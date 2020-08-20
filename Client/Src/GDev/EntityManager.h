#pragma once

#include "Core.h"
#include "Engine.h"
#include "../World/Cam.h"
#include "Entity.h"

class EntityManager :public Singleton<EntityManager>
{
	friend class Singleton<EntityManager>;

public:
	bool Init(void);
	virtual void AddEntity(Entity* entity);
	//virtual void Update();
	//virtual void Render(void);

	void FetchParticle(int numPerFrame);
	void Update(int numPerFrame);
	const std::vector<Entity*>& getVector();

protected:
	EntityManager(void);
	virtual ~EntityManager(void);

	void UpdateParticles();
	void UpdateEnemies();

	std::vector<Entity*> entityList;
};