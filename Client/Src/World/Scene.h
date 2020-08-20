#pragma once
#include <Engine.h>
#include "Cam.h"
#include "../GDev/EntityManager.h"

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
	void ForwardRender();
	bool CheckCollision(const glm::vec3& pos, const glm::vec3& scale);

private:
	Cam cam;
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

	ShaderProg blurSP;
	ShaderProg forwardSP;
	ShaderProg geoPassSP;
	ShaderProg lightingPassSP;
	ShaderProg normalsSP;
	ShaderProg screenSP;
	ShaderProg textSP;

	std::vector<Light*> ptLights;
	std::vector<Light*> directionalLights;
	std::vector<Light*> spotlights;

	glm::mat4 view;
	glm::mat4 projection;
	//std::vector<Mesh::BatchRenderParams> params;

	float elapsedTime;
	//int polyMode;
	mutable std::stack<glm::mat4> modelStack;
	glm::mat4 Translate(const glm::vec3& translate);
	glm::mat4 Rotate(const glm::vec4& rotate);
	glm::mat4 Scale(const glm::vec3& scale);
	glm::mat4 GetTopModel() const;
	void PushModel(const std::vector<glm::mat4>& vec) const;
	void PopModel() const;
};