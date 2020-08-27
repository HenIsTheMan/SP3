#pragma once
#include "Entity.h"

class EntitySpawner final{
public:
	///Getters
	const bool& GetCanSpawn() const;
	const float& GetSpawnDelay() const;
	const int& GetSpawnAmt() const;

	///Setters
	void SetCanSpawn(const bool& canSpawn);
	void SetSpawnDelay(const float& spawnDelay);
	void SetSpawnAmt(const int& spawnAmt);

private:
	bool canSpawn;
	Entity* entity;
	float spawnDelay;
	int spawnAmt;
};

/////Create coins
//for(short i = 0; i < 5; ++i){
//	const float scaleFactor = 15.f;
//	const float xPos = PseudorandMinMax(-terrainXScale / 2.f + 5.f, terrainXScale / 2.f - 5.f);
//	const float zPos = PseudorandMinMax(-terrainZScale / 2.f + 5.f, terrainZScale / 2.f - 5.f);
//	const glm::vec3 pos = glm::vec3(xPos, terrainYScale * static_cast<Terrain*>(meshes[(int)MeshType::Terrain])->GetHeightAtPt(xPos / terrainXScale, zPos / terrainZScale) + scaleFactor, zPos);
//
//	Entity* const& coin = entityManager->FetchEntity();
//	coin->active = true;
//	coin->life = 0.f;
//	coin->maxLife = 0.f;
//	coin->colour = glm::vec4(1.f);
//	coin->diffuseTexIndex = -1;
//	coin->rotate = glm::vec4(0.f, 1.f, 0.f, 0.f);
//	coin->scale = glm::vec3(scaleFactor);
//	coin->light = nullptr;
//	coin->pos = pos;
//	coin->vel = glm::vec3(0.f);
//	coin->mass = .0001f;
//	coin->force = glm::vec3(0.f);
//
//	switch(PseudorandMinMax(1, 5)){
//		case 1:
//			coin->type = Entity::EntityType::COIN_GOLD;
//			coin->mesh = meshes[(int)MeshType::CoinGold];
//			break;
//		case 2:
//			coin->type = Entity::EntityType::COIN_SILVER;
//			coin->mesh = meshes[(int)MeshType::CoinSilver];
//			break;
//		case 3:
//			coin->type = Entity::EntityType::COIN_PINK;
//			coin->mesh = meshes[(int)MeshType::CoinPink];
//			break;
//		case 4:
//			coin->type = Entity::EntityType::COIN_GREEN;
//			coin->mesh = meshes[(int)MeshType::CoinGreen];
//			break;
//		case 5:
//			coin->type = Entity::EntityType::COIN_BLUE;
//			coin->mesh = meshes[(int)MeshType::CoinBlue];
//			break;
//	}
//
//	ISound* myMusic = soundEngine->play3D("Audio/Music/Spin.mp3", vec3df(pos.x, pos.y, pos.z), true, true, true, ESM_AUTO_DETECT, true);
//	if(myMusic){
//		myMusic->setMinDistance(2.f);
//		myMusic->setVolume(3);
//		music.emplace_back(myMusic);
//	} else{
//		(void)puts("Failed to init music!\n");
//	}
//}