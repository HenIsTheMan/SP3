#pragma once
#include <Engine.h>
#include "Cam.h"
#include "../Game/EntityManager.h"
#include "../Game/Weapon.h"
#include "ModelStack.h"

class Scene final{
public:
	Scene();
	~Scene();
	bool Init();
	void Update();
	void GeoRenderPass();
	void LightingRenderPass(const uint& posTexRefID, const uint& coloursTexRefID, const uint& normalsTexRefID, const uint& specTexRefID, const uint& reflectionTexRefID);
	void BlurRender(const uint& brightTexRefID, const bool& horizontal);
	void DefaultRender(const uint& screenTexRefID, const uint& blurTexRefID);
	void DepthRender(const short& projectionType);
	void PlanarReflectionRender();
	void CubemapReflectionRender(const short& cubemapFace);
	void ForwardRender(const uint& depthDTexRefID, const uint& depthSTexRefID, const uint& planarReflectionTexID, const uint& cubemapReflectionTexID);
private:
	Cam cam;
	Cam dCam;
	Cam sCam;
	Cam waterCam;
	Cam enCam;
	ISoundEngine* soundEngine;
	ISound* music;
	ISoundEffectControl* soundFX;
	TextChief textChief;

	enum struct MeshType{
		Quad = 0,
		Cube,
		Sphere,
		Cylinder,
		SpriteAni,
		Terrain,
		Water,
		Amt
	};
	Mesh* meshes[(int)MeshType::Amt];

	enum struct ModelType{
		Skydome = 0,
		Suit,
		Amt
	};
	Model* models[(int)ModelType::Amt];

	EntityManager* entityManager;
	Weapon* weapon;

	ShaderProg blurSP;
	ShaderProg depthSP;
	ShaderProg forwardSP;
	ShaderProg geoPassSP;
	ShaderProg lightingPassSP;
	ShaderProg normalsSP;
	ShaderProg screenSP;
	ShaderProg textSP;

	std::vector<Light*> ptLights;
	std::vector<Light*> directionalLights;
	std::vector<Light*> spotlights;

	ModelStack modelStack;
	glm::mat4 view;
	glm::mat4 projection;
	//std::vector<Mesh::BatchRenderParams> params;

	float elapsedTime;
	float playerCurrHealth;
	float playerMaxHealth;
	float playerCurrLives;
	float playerMaxLives;
	//int polyMode;
};