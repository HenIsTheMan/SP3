#pragma once
#include <Core.h>
#include "Entity.h"

class CubeSection final{
public:
	enum struct CubeSectionType{
		UL = 0,
		UR,
		DL,
		DR,
		Root,
		Amt
	};

	CubeSection();
	~CubeSection();

	void AddEntity(Entity* const& entity);
	void Destroy();
	void Partition();
	const CubeSection* const SearchForEntity(Entity* const& entity) const;
	//void Collision

	///Getters
	const glm::vec3& GetOrigin() const;
	const glm::vec3& GetSize() const;

	///Setters
	void SetParent(CubeSection* const& parent);
	void SetOrigin(const glm::vec3& origin);
	void SetSize(const glm::vec3& size);
private:
	CubeSection* parent;
	glm::vec3 origin;
	glm::vec3 size;

	CubeSection* UL;
	CubeSection* UR;
	CubeSection* DL;
	CubeSection* DR;
	std::vector<Entity*>* entityList;
	int count;
};