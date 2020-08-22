#pragma once
#include <Core.h>
#include "Entity.h"

class CubeSection final{
public:
	CubeSection();
	~CubeSection();

	void AddEntity(Entity* const& entity);
	void Deactivate();
	void Partition();

	const CubeSection* const SearchForEntity(Entity* const& entity) const;
	void DetectAndResolveCollision(Entity* const& entity);

	///Getters
	const glm::vec3& GetOrigin() const;
	const glm::vec3& GetSize() const;

	///Setters
	void SetParent(CubeSection* const& parent);
	void SetOrigin(const glm::vec3& origin);
	void SetSize(const glm::vec3& size);

	static void InitCubeSectionPool(const int& amt);
	static void DestroyCubeSectionPool();
private:
	static std::vector<CubeSection*> cubeSectionPool;
	static CubeSection* const& FetchCubeSection();

	CubeSection* parent;
	glm::vec3 origin;
	glm::vec3 size;

	CubeSection* UL;
	CubeSection* UR;
	CubeSection* DL;
	CubeSection* DR;
	std::vector<Entity*>* entityList;
};