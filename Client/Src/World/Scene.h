#pragma once
#include <Engine.h>
#include "Cam.h"
#include "../Game/EntityManager.h"
#include "../Game/Weapon.h"
#include "ModelStack.h"

#define BIT(x) 1 << x

class Scene final{
public:
	enum struct Screen{
		Menu = 0,
		Score,
		Instructions,
		Credits,
		Game,
		Pause,
		End,
		Amt
	};

	Scene();
	~Scene();
	bool Init();
	void Update(GLFWwindow* const& win);
	void GeoRenderPass();
	void LightingRenderPass(const uint& posTexRefID, const uint& coloursTexRefID, const uint& normalsTexRefID, const uint& specTexRefID, const uint& reflectionTexRefID);
	void BlurRender(const uint& brightTexRefID, const bool& horizontal);
	void DefaultRender(const uint& screenTexRefID, const uint& blurTexRefID, const glm::vec3& translate, const glm::vec3& scale);
	void DepthRender(const short& projectionType);
	void PlanarReflectionRender();
	void CubemapReflectionRender(const short& cubemapFace);
	void ForwardRender(const uint& depthDTexRefID, const uint& depthSTexRefID, const uint& planarReflectionTexID, const uint& cubemapReflectionTexID);
	void MinimapRender(const uint& depthDTexRefID, const uint& depthSTexRefID);
	const Screen& GetScreen() const;
private:
	Cam cam;
	Cam dCam;
	Cam sCam;
	Cam waterCam;
	Cam enCam;
	Cam minimapcam;

	ISoundEngine* soundEngine;
	std::vector<ISound*> music;
	TextChief textChief;

	enum struct MeshType{
		Quad = 0,
		Cube,
		Sphere,
		Cylinder,
		Terrain,
		Water,
		Fire,
		CoinGold,
		CoinSilver,
		CoinPink,
		CoinGreen,
		CoinBlue,
		Amt
	};
	Mesh* meshes[(int)MeshType::Amt];

	enum struct ModelType{
		Pistol,
		AR,
		Sniper,
		Virus,
		Grass,
		Amt
	};
	Model* models[(int)ModelType::Amt];

	enum struct WaveNumber{
		One,
		Two,
		Three,
		Total
	};
	int waves[(int)WaveNumber::Total];
	int waveCount;

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
	glm::mat4 minimapView;
	glm::mat4 minimapProjection;
	//std::vector<Mesh::BatchRenderParams> params;

	float elapsedTime;
	float playerCurrHealth;
	float playerMaxHealth;
	float playerCurrLives;
	float playerMaxLives;

	int polyModes[2];
	int enemyCount;
	int currentEnemyCount;
	int score;
	std::vector<int> scores;

	enum struct PlayerState{
		NoMovement = BIT(1),
		Walking = BIT(2),
		Sprinting = BIT(3),
		Standing = BIT(4),
		Jumping = BIT(5),
		Falling = BIT(6),
		Crouching = BIT(7),
		Proning = BIT(8),
	};
	int playerStates;
	bool sprintOn;
	glm::vec4 reticleColour;

	bool scope;
	Screen screen;
	float textScaleFactors[5];
	glm::vec4 textColours[5];
	bool addAmmo;
	
	int goldCoinAmt;
	int silverCoinAmt;
	int pinkCoinAmt;
	int greenCoinAmt;
	int blueCoinAmt;
	void SpawnCoin(const Entity::EntityType& type, Mesh* const& mesh, const float& scaleFactor);

	bool takingDmg;
	float healthUp;
	float lifeUp;
	bool immune;
};

enum struct Axis{
	x = 0,
	y,
	z,
	Amt
};

inline static glm::vec3 RotateVecIn2D(const glm::vec3& vec, const float& angleInRad, const Axis& axis){
	switch (axis) {
	case Axis::x:
		return glm::vec3(vec.x, vec.y * cos(angleInRad) + vec.z * -sin(angleInRad), vec.y * sin(angleInRad) + vec.z * cos(angleInRad));
	case Axis::y:
		return glm::vec3(vec.x * cos(angleInRad) + vec.z * sin(angleInRad), vec.y, vec.x * -sin(angleInRad) + vec.z * cos(angleInRad));
	case Axis::z:
		return glm::vec3(vec.x * cos(angleInRad) + vec.y * -sin(angleInRad), vec.x * sin(angleInRad) + vec.y * cos(angleInRad), vec.z);
	default:
		(void)puts("Invalid axis input for vec rotation!");
		return glm::vec3();
	}
}