#include "CubeSection.h"

std::vector<CubeSection*> CubeSection::cubeSectionPool;

CubeSection::CubeSection():
	parent(nullptr),
	origin(glm::vec3(0.f)),
	size(glm::vec3(0.f)),
	UL(nullptr),
	UR(nullptr),
	DL(nullptr),
	DR(nullptr),
	entityList(new std::vector<Entity*>())
{
}

CubeSection::~CubeSection(){
	if(entityList){
		for(Entity*& entity: *entityList){
			if(entity){ //Deleted in EntityManager
				entity = nullptr;
			}
		}
		delete entityList;
		entityList = nullptr;
	}
}

void CubeSection::AddEntity(Entity* const& entity){
	entityList->emplace_back(entity);
}

void CubeSection::Deactivate(){
	entityList->clear();
	if(UL){ //If UL exists...
		UL->entityList->clear();
		UL->Deactivate();
	}
	if(UR){ //If UR exists...
		UR->entityList->clear();
		UR->Deactivate();
	}
	if(DL){ //If DL exists...
		DL->entityList->clear();
		DL->Deactivate();
	}
	if(DR){ //If DR exists...
		DR->entityList->clear();
		DR->Deactivate();
	}
}

void CubeSection::Partition(){
	if(entityList->size() > 4){
		///Set up UL
		UL = FetchCubeSection();
		UL->SetParent(this);
		UL->SetOrigin(glm::vec3(origin.x - size.x / 4.f, origin.y + size.y / 4.f, origin.z));
		UL->SetSize(size / 2.f);

		///Set up UR
		UR = FetchCubeSection();
		UR->SetParent(this);
		UR->SetOrigin(glm::vec3(origin.x + size.x / 4.f, origin.y + size.y / 4.f, origin.z));
		UR->SetSize(size / 2.f);

		///Set up DL
		DL = FetchCubeSection();
		DL->SetParent(this);
		DL->SetOrigin(glm::vec3(origin.x - size.x / 4.f, origin.y - size.y / 4.f, origin.z));
		DL->SetSize(size / 2.f);

		///Set up DR
		DR = FetchCubeSection();
		DR->SetParent(this);
		DR->SetOrigin(glm::vec3(origin.x + size.x / 4.f, origin.y - size.y / 4.f, origin.z));
		DR->SetSize(size / 2.f);

		if(entityList){
			for(Entity*& entity: *entityList){
				if(entity->pos == origin){
					switch(PseudorandMinMax(0, 3)){
						case 0:
							UR->AddEntity(entity);
							break;
						case 1:
							UL->AddEntity(entity);
							break;
						case 2:
							DR->AddEntity(entity);
							break;
						case 3:
							DL->AddEntity(entity);
							break;
					}
					continue;
				}
				if(entity->pos.y > origin.y){
					if(entity->pos.x > origin.x){
						UR->AddEntity(entity);
					} else{
						UL->AddEntity(entity);
					}
				} else{
					if(entity->pos.x > origin.x){
						DR->AddEntity(entity);
					} else{
						DL->AddEntity(entity);
					}
				}
			}
		}

		///Use recursion to continue forming the Quadtree
		UL->Partition();
		UR->Partition();
		DL->Partition();
		DR->Partition();
	}
}

const CubeSection* const CubeSection::SearchForEntity(Entity* const& entity) const{ //Search children 1st
	if(UL){ //If UL exists...
		const CubeSection* const& CubeSection = UL->SearchForEntity(entity);
		if(CubeSection){ //If entity was found in a CubeSection...
			return CubeSection;
		}
	}
	if(UR){ //If UR exists...
		const CubeSection* const& CubeSection = UR->SearchForEntity(entity);
		if(CubeSection){ //If entity was found in a CubeSection...
			return CubeSection;
		}
	}
	if(DL){ //If DL exists...
		const CubeSection* const& CubeSection = DL->SearchForEntity(entity);
		if(CubeSection){ //If entity was found in a CubeSection...
			return CubeSection;
		}
	}
	if(DR){ //If DR exists...
		const CubeSection* const& CubeSection = DR->SearchForEntity(entity);
		if(CubeSection){ //If entity was found in a CubeSection...
			return CubeSection;
		}
	}
	if(entityList){
		for(Entity*& myEntity: *entityList){
			if(myEntity == entity){
				return this;
			}
		}
	}
	return nullptr;
}

const glm::vec3& CubeSection::GetOrigin() const{
	return origin;
}

const glm::vec3& CubeSection::GetSize() const{
	return size;
}

void CubeSection::SetParent(CubeSection* const& parent){
	this->parent = parent;
}

void CubeSection::SetOrigin(const glm::vec3& origin){
	this->origin = origin;
}

void CubeSection::SetSize(const glm::vec3& size){
	this->size = size;
}

void CubeSection::InitCubeSectionPool(const int& amt){
	cubeSectionPool = std::vector<CubeSection*>(amt);
	for(int i = 0; i < amt; ++i){
		cubeSectionPool[i] = new CubeSection();
	}
}

void CubeSection::DestroyCubeSectionPool(){
	const size_t& size = cubeSectionPool.size();
	for(size_t i = 0; i < size; ++i){
		if(cubeSectionPool[i]){
			delete cubeSectionPool[i];
			cubeSectionPool[i] = nullptr;
		}
	}
}

CubeSection* const& CubeSection::FetchCubeSection(){
	for(CubeSection* const& cubeSection: cubeSectionPool){
		if(!cubeSection->entityList->size()){
			return cubeSection;
		}
	}
	cubeSectionPool.emplace_back(new CubeSection());
	(void)puts("1 cube section was added to cubeSectionPool!\n");
	return cubeSectionPool.back();
}