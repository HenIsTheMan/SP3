#include "CubeSection.h"

CubeSection::CubeSection():
	parent(nullptr),
	origin(glm::vec3(0.f)),
	size(glm::vec3(0.f)),
	UL(nullptr),
	UR(nullptr),
	DL(nullptr),
	DR(nullptr),
	entityList(new std::vector<Entity*>()),
	count(0)
{
}

CubeSection::~CubeSection(){
	if(entityList){
		for(Entity*& entity: *entityList){
			delete entity;
			entity = nullptr;
		}
		delete entityList;
	}
}

void CubeSection::AddEntity(Entity* const& entity){
	entityList->emplace_back(entity);
}

void CubeSection::Destroy(){
	if(UL){ //If UL exists...
		UL->Destroy();
		UL = nullptr; //Make dangling ptr a null ptr
	}
	if(UR){ //If UR exists...
		UR->Destroy();
		UR = nullptr; //Make dangling ptr a null ptr
	}
	if(DL){ //If DL exists...
		DL->Destroy();
		DL = nullptr; //Make dangling ptr a null ptr
	}
	if(DR){ //If DR exists...
		DR->Destroy();
		DR = nullptr; //Make dangling ptr a null ptr
	}
	delete this;
}

void CubeSection::Partition(){
	//if(count > 3){
	//	///Set up UL
	//	UL = new CubeSection();
	//	UL->SetParent(this);
	//	UL->SetOrigin(this->origin);
	//	UL->SetWidth(width & 1 ? (width - 1) / 2 : width / 2);
	//	UL->SetHeight(height & 1 ? (height - 1) / 2 : height / 2);

	//	///Set up UR
	//	UR = new CubeSection();
	//	UR->SetParent(this);
	//	UR->SetWidth(width & 1 ? (width - 1) / 2 + 1 : width / 2);
	//	UR->SetHeight(UL->GetHeight());
	//	UR->SetOrigin({UL->GetOrigin().x + UL->GetWidth(), UL->GetOrigin().y});

	//	///Set up DL
	//	DL = new CubeSection();
	//	DL->SetParent(this);
	//	DL->SetWidth(UL->GetWidth());
	//	DL->SetHeight(height & 1 ? (height - 1) / 2 + 1 : height / 2);
	//	DL->SetOrigin({UL->GetOrigin().x, UL->GetOrigin().y + UL->GetHeight()});

	//	///Set up DR
	//	DR = new CubeSection();
	//	DR->SetParent(this);
	//	DR->SetOrigin({UR->GetOrigin().x, DL->GetOrigin().y});
	//	DR->SetWidth(UR->GetWidth());
	//	DR->SetHeight(DL->GetHeight());

	//	for(const GO* const& go: *GOList){
	//		if(go->GetPosX() >= UR->GetOrigin().x){
	//			if(go->GetPosY() >= DR->GetOrigin().y){
	//				DR->AddGO(go->GetID(), go->GetPosX(), go->GetPosY());
	//			} else{
	//				UR->AddGO(go->GetID(), go->GetPosX(), go->GetPosY());
	//			}
	//		} else{
	//			if(go->GetPosY() >= DL->GetOrigin().y){
	//				DL->AddGO(go->GetID(), go->GetPosX(), go->GetPosY());
	//			} else{
	//				UL->AddGO(go->GetID(), go->GetPosX(), go->GetPosY());
	//			}
	//		}
	//	}

	//	///Use recursion to continue forming the Quadtree
	//	UL->Partition();
	//	UR->Partition();
	//	DL->Partition();
	//	DR->Partition();
	//}
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