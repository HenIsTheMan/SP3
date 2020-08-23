#include "CubeSection.h"
#include "Collision.h"

std::vector<CubeSection*> CubeSection::cubeSectionPool;

CubeSection::CubeSection():
	parent(nullptr),
	origin(glm::vec3(0.f)),
	size(glm::vec3(0.f)),
	UL(nullptr),
	UR(nullptr),
	DL(nullptr),
	DR(nullptr),
	entityList(new std::vector<Entity*>()),
	active(false)
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
	if(UL){
		active = false;
		entityList->clear();
		UL->Deactivate();
	}
	if(UR){
		active = false;
		entityList->clear();
		UR->Deactivate();
	}
	if(DL){
		active = false;
		entityList->clear();
		DL->Deactivate();
	}
	if(DR){
		active = false;
		entityList->clear();
		DR->Deactivate();
	}
}

void CubeSection::Partition(){
	if(entityList->size() > 7){
		///Set up UL
		UL = FetchCubeSection();
		UL->active = true;
		UL->SetParent(this);
		UL->SetOrigin(glm::vec3(origin.x - size.x / 4.f, origin.y, origin.z - size.z / 4.f));
		UL->SetSize(glm::vec3(size.x / 2.f, size.y, size.z / 2.f));

		///Set up UR
		UR = FetchCubeSection();
		UR->active = true;
		UR->SetParent(this);
		UR->SetOrigin(glm::vec3(origin.x + size.x / 4.f, origin.y, origin.z - size.z / 4.f));
		UR->SetSize(glm::vec3(size.x / 2.f, size.y, size.z / 2.f));

		///Set up DL
		DL = FetchCubeSection();
		DL->active = true;
		DL->SetParent(this);
		DL->SetOrigin(glm::vec3(origin.x - size.x / 4.f, origin.y, origin.z + size.z / 4.f));
		DL->SetSize(glm::vec3(size.x / 2.f, size.y, size.z / 2.f));

		///Set up DR
		DR = FetchCubeSection();
		DR->active = true;
		DR->SetParent(this);
		DR->SetOrigin(glm::vec3(origin.x + size.x / 4.f, origin.y, origin.z + size.z / 4.f));
		DR->SetSize(glm::vec3(size.x / 2.f, size.y, size.z / 2.f));

		glm::vec3* prevPos = nullptr;
		if(entityList){
			for(Entity*& entity : *entityList){
				if(entity && entity->active){
					if(prevPos && entity->pos == *prevPos){
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
					} else{
						switch(entity->mesh->GetMeshType()){
							case Mesh::MeshType::Sphere:
								if(entity->pos.z - entity->scale.z <= origin.z){
									if(entity->pos.x - entity->scale.x <= origin.x){
										UR->AddEntity(entity);
									}
									if(entity->pos.x + entity->scale.x >= origin.x){
										UL->AddEntity(entity);
									}
								}
								if(entity->pos.z + entity->scale.z >= origin.z){
									if(entity->pos.x - entity->scale.x <= origin.x){
										DR->AddEntity(entity);
									}
									if(entity->pos.x + entity->scale.x >= origin.x){
										DL->AddEntity(entity);
									}
								}
								break;
						}
					}
					prevPos = &entity->pos;
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
		const CubeSection* const& cubeSection = UL->SearchForEntity(entity);
		if(cubeSection){ //If entity was found in a cubeSection...
			return cubeSection;
		}
	}
	if(UR){ //If UR exists...
		const CubeSection* const& cubeSection = UR->SearchForEntity(entity);
		if(cubeSection){ //If entity was found in a cubeSection...
			return cubeSection;
		}
	}
	if(DL){ //If DL exists...
		const CubeSection* const& cubeSection = DL->SearchForEntity(entity);
		if(cubeSection){ //If entity was found in a cubeSection...
			return cubeSection;
		}
	}
	if(DR){ //If DR exists...
		const CubeSection* const& cubeSection = DR->SearchForEntity(entity);
		if(cubeSection){ //If entity was found in a cubeSection...
			return cubeSection;
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

void CubeSection::DetectAndResolveCollision(Entity* const& entity){
	const CubeSection* const& cubeSection = SearchForEntity(entity);
	if(!cubeSection){
		return (void)puts("Entity was not found in quadtree!");
	}

	const size_t& size = cubeSection->entityList->size();
	for(size_t i = 0; i < size; ++i){
		Entity* const& instance = (*cubeSection->entityList)[i];
		if(instance && instance->active && entity != instance){
			Collision::DetectAndResolveCollision(entity, instance);
		}
	}
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

void CubeSection::SetActive(const bool& active){
	this->active = active;
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
		if(!cubeSection->active){
			return cubeSection;
		}
	}
	cubeSectionPool.emplace_back(new CubeSection());
	(void)puts("1 cube section was added to cubeSectionPool!\n");
	return cubeSectionPool.back();
}