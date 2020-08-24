#include "Scene.h"
#include "Vendor/stb_image.h"
#include "../Game/Pistol.h"
#include "../Game/AssaultRifle.h"
#include "../Game/SniperRifle.h"

extern bool leftMB;
extern bool rightMB;
extern float angularFOV;
extern float dt;
extern int winWidth;
extern int winHeight;

constexpr float terrainXScale = 500.f;
constexpr float terrainYScale = 100.f;
constexpr float terrainZScale = 500.f;

glm::vec3 Light::globalAmbient = glm::vec3(.2f);

Scene::Scene() :
	cam(glm::vec3(0.f, 0.f, 5.f), glm::vec3(0.f), glm::vec3(0.f, 1.f, 0.f), 0.f, 150.f),
	dCam(glm::vec3(0.f, 150.f, 0.f), glm::vec3(0.f), glm::vec3(0.f, 0.f, 1.f), 0.f, 0.f),
	sCam(glm::vec3(0.f, 100.f, 200.f), glm::vec3(0.f, 100.f, 0.f), glm::vec3(0.f, 1.f, 0.f), 1.f, 0.f),
	waterCam(glm::vec3(-15.f, -20.f, -20.f), glm::vec3(-15.f, 0.f, -20.f), glm::vec3(0.f, 0.f, 1.f), 1.f, 0.f),
	enCam(glm::vec3(0.f), glm::vec3(0.f), glm::vec3(0.f), 1.f, 0.f),
	minimapcam(glm::vec3(0.f), glm::vec3(0.f), glm::vec3(0.f), 0.f, 0.f),
	soundEngine(nullptr),
	music(nullptr),
	soundFX(nullptr),
	entityManager(nullptr),
	weapon(nullptr),
	meshes{
		new Mesh(Mesh::MeshType::Quad, GL_TRIANGLES, {
			{"Imgs/BoxAlbedo.png", Mesh::TexType::Diffuse, 0},
			{"Imgs/BoxSpec.png", Mesh::TexType::Spec, 0},
			{"Imgs/BoxEmission.png", Mesh::TexType::Emission, 0},
			{"Imgs/Heart.png", Mesh::TexType::Diffuse, 0},
			{"Imgs/Slot.tga", Mesh::TexType::Diffuse, 0},
			{"Imgs/ReticlePri.png", Mesh::TexType::Diffuse, 0},
			{"Imgs/ReticleSec.png", Mesh::TexType::Diffuse, 0},
		}),
		new Mesh(Mesh::MeshType::Cube, GL_TRIANGLES, {
			{"Imgs/BoxAlbedo.png", Mesh::TexType::Diffuse, 0},
			{"Imgs/Grey.png", Mesh::TexType::Reflection, 0},
		}),
		new Mesh(Mesh::MeshType::Sphere, GL_TRIANGLE_STRIP, {
			{"Imgs/Skydome.hdr", Mesh::TexType::Diffuse, 0},
		}),
		new Mesh(Mesh::MeshType::Cylinder, GL_TRIANGLE_STRIP, {
			{"Imgs/BoxAlbedo.png", Mesh::TexType::Diffuse, 0},
		}),
		new SpriteAni(4, 8),
		new Terrain("Imgs/hMap.raw", 8.f, 8.f),
		new Water(24.f, 2.f, 2.f, .5f),
	},
	models{
		new Model("ObjsAndMtls/Pistol.obj", {
			aiTextureType_DIFFUSE,
		}),
		new Model("ObjsAndMtls/AR.obj", {
			aiTextureType_DIFFUSE,
		}),
		new Model("ObjsAndMtls/Sniper.obj", {
			aiTextureType_DIFFUSE,
		}),
		new Model("ObjsAndMtls/Virus.obj", {
			aiTextureType_DIFFUSE,
		}),
	},
	blurSP{"Shaders/Quad.vs", "Shaders/Blur.fs"},
	depthSP{"Shaders/Depth.vs", "Shaders/Depth.fs"},
	forwardSP{"Shaders/Forward.vs", "Shaders/Forward.fs"},
	geoPassSP{"Shaders/GeoPass.vs", "Shaders/GeoPass.fs"},
	lightingPassSP{"Shaders/Quad.vs", "Shaders/LightingPass.fs"},
	normalsSP{"Shaders/Normals.vs", "Shaders/Normals.fs", "Shaders/Normals.gs"}, //??
	screenSP{"Shaders/Quad.vs", "Shaders/Screen.fs"},
	textSP{"Shaders/Text.vs", "Shaders/Text.fs"},
	ptLights({}),
	directionalLights({}),
	spotlights({}),
	view(glm::mat4(1.f)),
	projection(glm::mat4(1.f)),
	elapsedTime(0.f),
	//polyMode(0),
	playerCurrHealth(100.f),
	playerMaxHealth(100.f),
	playerCurrLives(5.f),
	playerMaxLives(5.f),
	enemyCount(0),
	waves{},
	playerStates((int)PlayerState::NoMovement | (int)PlayerState::Standing),
	sprintOn(false),
	reticleColour(glm::vec4(1.f))
{
}

Scene::~Scene() {
	const size_t& pSize = ptLights.size();
	const size_t& dSize = directionalLights.size();
	const size_t& sSize = spotlights.size();
	for (size_t i = 0; i < pSize; ++i) {
		if (ptLights[i]) {
			delete ptLights[i];
			ptLights[i] = nullptr;
		}
	}
	for (size_t i = 0; i < dSize; ++i) {
		if (directionalLights[i]) {
			delete directionalLights[i];
			directionalLights[i] = nullptr;
		}
	}
	for (size_t i = 0; i < sSize; ++i) {
		if (spotlights[i]) {
			delete spotlights[i];
			spotlights[i] = nullptr;
		}
	}

	for (int i = 0; i < (int)MeshType::Amt; ++i) {
		if (meshes[i]) {
			delete meshes[i];
			meshes[i] = nullptr;
		}
	}
	for (int i = 0; i < (int)ModelType::Amt; ++i) {
		if (models[i]) {
			delete models[i];
			models[i] = nullptr;
		}
	}
	if (music) {
		music->drop();
	}
	if (soundEngine) {
		soundEngine->drop();
	}
	if(entityManager){
		entityManager->Destroy();
		entityManager = nullptr;
	}
	if(weapon){
		delete weapon;
		weapon = nullptr;
	}
}

bool Scene::Init(){
	// Create weapons to be put in the inventory
	weapon = new Weapon();

	Pistol* pistol = new Pistol();
	pistol->Init();
	weapon->SetInventory(0, pistol);

	AssaultRifle* assaultRifle = new AssaultRifle();
	assaultRifle->Init();
	weapon->SetInventory(1, assaultRifle);

	SniperRifle* sniperRifle = new SniperRifle();
	sniperRifle->Init();
	weapon->SetInventory(2, sniperRifle);

	entityManager = EntityManager::GetObjPtr();
	entityManager->CreateEntities(100);

	Entity* const& enemy = entityManager->FetchEntity();
	enemy->type = Entity::EntityType::MOVING_ENEMY;
	enemy->active = true;
	enemy->life = 0.f;
	enemy->maxLife = 0.f;
	enemy->colour = glm::vec4(1.f, 0.f, 0.f, 1.f);
	enemy->diffuseTexIndex = -1;
	enemy->rotate = glm::vec4(0.f, 1.f, 0.f, 0.f);
	enemy->scale = glm::vec3(20.f);
	enemy->light = nullptr;
	enemy->mesh = meshes[(int)MeshType::Sphere];
	enemy->pos = glm::vec3(-100.f, 200.f, 0.f);
	enemy->vel = glm::vec3(0.f, 6.f, 3.f);
	enemy->mass = 1.f;
	enemy->force = glm::vec3(0.f);

	Entity* const& bullet = entityManager->FetchEntity();
	bullet->type = Entity::EntityType::BULLET;
	bullet->active = true;
	bullet->life = 0.f;
	bullet->maxLife = 0.f;
	bullet->colour = glm::vec4(0.f, 0.f, 1.f, 1.f);
	bullet->diffuseTexIndex = -1;
	bullet->rotate = glm::vec4(0.f, 1.f, 0.f, 0.f);
	bullet->scale = glm::vec3(20.f);
	bullet->light = nullptr;
	bullet->mesh = meshes[(int)MeshType::Sphere];
	bullet->pos = glm::vec3(-100.f, 200.f, 150.f);
	bullet->vel = glm::vec3(0.f, 4.f, -12.f);
	bullet->mass = 1.f;
	bullet->force = glm::vec3(0.f);

	Entity* const& player = entityManager->FetchEntity();
	player->type = Entity::EntityType::PLAYER;
	player->active = true;
	player->life = 0.f;
	player->maxLife = 0.f;
	player->colour = glm::vec4(0.f, 1.f, 1.f, 1.f);
	player->diffuseTexIndex = -1;
	player->rotate = glm::vec4(0.f, 1.f, 0.f, 0.f);
	player->scale = glm::vec3(5.f);
	player->light = nullptr;
	player->mesh = meshes[(int)MeshType::Sphere];
	player->pos = cam.GetPos();
	player->vel = glm::vec3(0.f);
	player->mass = 10.f;
	player->force = glm::vec3(0.f);

	//glGetIntegerv(GL_POLYGON_MODE, &polyMode);

	soundEngine = createIrrKlangDevice(ESOD_AUTO_DETECT, ESEO_MULTI_THREADED | ESEO_LOAD_PLUGINS | ESEO_USE_3D_BUFFERS | ESEO_PRINT_DEBUG_INFO_TO_DEBUGGER);
	if (!soundEngine) {
		(void)puts("Failed to init soundEngine!\n");
	}
	//soundEngine->play2D("Audio/Music/YellowCafe.mp3", true);

	music = soundEngine->play3D("Audio/Music/YellowCafe.mp3", vec3df(0.f, 0.f, 0.f), true, true, true, ESM_AUTO_DETECT, true);
	if (music) {
		music->setMinDistance(5.f);
		music->setVolume(0);

		soundFX = music->getSoundEffectControl();
		if (!soundFX) {
			(void)puts("No soundFX support!\n");
		}
	}
	else {
		(void)puts("Failed to init music!\n");
	}

	meshes[(int)MeshType::SpriteAni]->AddTexMap({ "Imgs/Fire.png", Mesh::TexType::Diffuse, 0 });
	static_cast<SpriteAni*>(meshes[(int)MeshType::SpriteAni])->AddAni("FireSpriteAni", 0, 32);
	static_cast<SpriteAni*>(meshes[(int)MeshType::SpriteAni])->Play("FireSpriteAni", -1, .5f);

	meshes[(int)MeshType::Terrain]->AddTexMap({"Imgs/GrassGround.jpg", Mesh::TexType::Diffuse, 0});
	meshes[(int)MeshType::Water]->AddTexMap({"Imgs/Water.jpg", Mesh::TexType::Diffuse, 0});
	meshes[(int)MeshType::Water]->AddTexMap({"Imgs/Grey.png", Mesh::TexType::Reflection, 0});

	directionalLights.emplace_back(CreateLight(LightType::Directional));
	spotlights.emplace_back(CreateLight(LightType::Spot));
	//spotlights.emplace_back(CreateLight(LightType::Spot));

	return true;
}

void Scene::Update(){
	reticleColour = glm::vec4(1.f);

	elapsedTime += dt;
	if(winHeight){ //Avoid division by 0 when win is minimised
		cam.SetDefaultAspectRatio(float(winWidth) / float(winHeight));
		cam.ResetAspectRatio();
	}

	////Control player states
	static float sprintBT = 0.f;
	static float heightBT = 0.f;

	///Toggle sprint
	if(Key(VK_SHIFT) && sprintBT <= elapsedTime){
		sprintOn = !sprintOn;
		sprintBT = elapsedTime + .5f;
	}

	///Set movement state
	if(Key(GLFW_KEY_A) || Key(GLFW_KEY_D) || Key(GLFW_KEY_W) || Key(GLFW_KEY_S)){
		if(sprintOn){
			playerStates &= ~(int)PlayerState::NoMovement;
			playerStates &= ~(int)PlayerState::Walking;
			playerStates |= (int)PlayerState::Sprinting;
		} else{
			playerStates &= ~(int)PlayerState::NoMovement;
			playerStates |= (int)PlayerState::Walking;
			playerStates &= ~(int)PlayerState::Sprinting;
		}
	} else{
		playerStates |= (int)PlayerState::NoMovement;
		playerStates &= ~(int)PlayerState::Walking;
		playerStates &= ~(int)PlayerState::Sprinting;
	}

	///Set height state
	if(heightBT <= elapsedTime){
		if(Key(GLFW_KEY_C)){
			if(playerStates & (int)PlayerState::Standing){
				playerStates |= (int)PlayerState::Crouching;
				playerStates &= ~(int)PlayerState::Standing;
			} else if(playerStates & (int)PlayerState::Crouching){
				playerStates |= (int)PlayerState::Proning;
				playerStates &= ~(int)PlayerState::Crouching;
			}
			heightBT = elapsedTime + .5f;
		}
		if(Key(VK_SPACE)){
			if(playerStates & (int)PlayerState::Proning){
				playerStates |= (int)PlayerState::Crouching;
				playerStates &= ~(int)PlayerState::Proning;
			} else if(playerStates & (int)PlayerState::Crouching){
				playerStates |= (int)PlayerState::Standing;
				playerStates &= ~(int)PlayerState::Crouching;
			} else if(playerStates & (int)PlayerState::Standing){
				soundEngine->play2D("Audio/Sounds/Jump.wav", false);
				playerStates |= (int)PlayerState::Jumping;
				playerStates &= ~(int)PlayerState::Standing;
			}
			heightBT = elapsedTime + .5f;
		} else{
			if((playerStates & (int)PlayerState::Jumping)){
				playerStates |= (int)PlayerState::Falling;
				playerStates &= ~(int)PlayerState::Jumping;
				cam.SetVel(0.f);
			}
		}
	}

	float yMin = terrainYScale * static_cast<Terrain*>(meshes[(int)MeshType::Terrain])->GetHeightAtPt(cam.GetPos().x / terrainXScale, cam.GetPos().z / terrainZScale);
	float yMax = yMin;

	///Update player according to its states
	int playerStatesTemp = playerStates;
	int bitMask = 1;
	while(playerStatesTemp){
		switch(PlayerState(playerStatesTemp & bitMask)){
			case PlayerState::NoMovement:
				cam.SetSpd(0.f);
				break;
			case PlayerState::Walking:
				cam.SetSpd(100.f);
				break;
			case PlayerState::Sprinting:
				cam.SetSpd(250.f);
				break;
			case PlayerState::Standing:
				yMin += 30.f;
				yMax += 30.f;
				break;
			case PlayerState::Jumping:
				cam.SetVel(300.f);
			case PlayerState::Falling:
				cam.SetAccel(-1500.f);
				yMin += 30.f;
				yMax += 250.f;
				break;
			case PlayerState::Crouching:
				cam.SetSpd(cam.GetSpd() / 5.f);
				yMin += 5.f;
				yMax += 5.f;
				break;
			case PlayerState::Proning:
				cam.SetSpd(5.f);
				yMin += 1.f;
				yMax += 1.f;
				break;
		}
		playerStatesTemp &= ~bitMask;
		bitMask <<= 1;
	}

	if(playerStates & (int)PlayerState::Jumping){
		if(cam.GetPos().y >= yMax){
			playerStates |= (int)PlayerState::Falling;
			playerStates &= ~(int)PlayerState::Jumping;
			cam.SetVel(0.f);
		}
	}
	if(playerStates & (int)PlayerState::Falling){
		if(cam.GetPos().y <= yMin){
			playerStates |= (int)PlayerState::Standing;
			playerStates &= ~(int)PlayerState::Falling;
			cam.SetAccel(0.f);
			cam.SetVel(0.f);
		}
	}
	cam.UpdateJumpFall();
	cam.Update(GLFW_KEY_A, GLFW_KEY_D, GLFW_KEY_W, GLFW_KEY_S, -terrainXScale / 2.f + 5.f, terrainXScale / 2.f - 5.f, yMin, yMax, -terrainZScale / 2.f + 5.f, terrainZScale / 2.f - 5.f);
	view = cam.LookAt();
	projection = glm::perspective(glm::radians(angularFOV), cam.GetAspectRatio(), .1f, 9999.f);

	minimapView = cam.LookAt();
	minimapProjection = glm::perspective(glm::radians(angularFOV), cam.GetAspectRatio(), .1f, 9999.f);
	const glm::vec3& camPos = cam.GetPos();
	const glm::vec3& camFront = cam.CalcFront();
	soundEngine->setListenerPosition(vec3df(camPos.x, camPos.y, camPos.z), vec3df(camFront.x, camFront.y, camFront.z));

	///Aim
	if(rightMB){
		Weapon* const& currWeapon = weapon->GetCurrentWeapon();
		switch(weapon->GetCurrentSlot()){
			case 0:
				angularFOV = 40.f;
				break;
			case 1:
				angularFOV = 30.f;
				break;
			case 2:
				angularFOV = 15.f;
				break;
			default:
				angularFOV = 45.f;
		}
	} else{
		angularFOV = 45.f;
	}

	directionalLights[0]->ambient = glm::vec3(.05f);
	directionalLights[0]->diffuse = glm::vec3(.8f);
	directionalLights[0]->spec = glm::vec3(1.f);
	static_cast<DirectionalLight*>(directionalLights[0])->dir = dCam.CalcFront();

	spotlights[0]->ambient = glm::vec3(.05f);
	spotlights[0]->diffuse = glm::vec3(.8f);
	spotlights[0]->spec = glm::vec3(1.f);
	static_cast<Spotlight*>(spotlights[0])->pos = sCam.GetPos();
	static_cast<Spotlight*>(spotlights[0])->dir = sCam.CalcFront();
	static_cast<Spotlight*>(spotlights[0])->cosInnerCutoff = cosf(glm::radians(12.5f));
	static_cast<Spotlight*>(spotlights[0])->cosOuterCutoff = cosf(glm::radians(17.5f));

	//spotlights[1]->ambient = glm::vec3(.05f);
	//spotlights[1]->diffuse = glm::vec3(.8f);
	//spotlights[1]->spec = glm::vec3(1.f);
	//static_cast<Spotlight*>(spotlights[1])->pos = camPos;
	//static_cast<Spotlight*>(spotlights[1])->dir = camFront;
	//static_cast<Spotlight*>(spotlights[1])->cosInnerCutoff = cosf(glm::radians(12.5f));
	//static_cast<Spotlight*>(spotlights[1])->cosOuterCutoff = cosf(glm::radians(17.5f));

	static_cast<SpriteAni*>(meshes[(int)MeshType::SpriteAni])->Update();

	static float polyModeBT = 0.f;
	static float distortionBT = 0.f;
	static float echoBT = 0.f;
	static float wavesReverbBT = 0.f;
	static float resetSoundFXBT = 0.f;

	//if(Key(GLFW_KEY_F2) && polyModeBT <= elapsedTime){
	//	polyMode += polyMode == GL_FILL ? -2 : 1;
	//	glPolygonMode(GL_FRONT_AND_BACK, polyMode);
	//	polyModeBT = elapsedTime + .5f;
	//}

	if(soundFX){
		if(Key(GLFW_KEY_I) && distortionBT <= elapsedTime){
			soundFX->isDistortionSoundEffectEnabled() ? soundFX->disableDistortionSoundEffect() : (void)soundFX->enableDistortionSoundEffect();
			distortionBT = elapsedTime + .5f;
		}
		if(Key(GLFW_KEY_O) && echoBT <= elapsedTime){
			soundFX->isEchoSoundEffectEnabled() ? soundFX->disableEchoSoundEffect() : (void)soundFX->enableEchoSoundEffect();
			echoBT = elapsedTime + .5f;
		}
		if(Key(GLFW_KEY_P) && wavesReverbBT <= elapsedTime){
			soundFX->isWavesReverbSoundEffectEnabled() ? soundFX->disableWavesReverbSoundEffect() : (void)soundFX->enableWavesReverbSoundEffect();
			wavesReverbBT = elapsedTime + .5f;
		}
		if(Key(GLFW_KEY_L) && resetSoundFXBT <= elapsedTime){
			soundFX->disableAllEffects();
			resetSoundFXBT = elapsedTime + .5f;
		}
	}

	// TESTING ONLY FOR HEALTHBAR
	//if (Key(GLFW_KEY_SPACE))
	//{
	//	playerCurrHealth -= 1.f;
	//}

	// Player gets max health again, but loses 1 life
	if (playerCurrHealth <= 0.f && playerCurrLives > 0.f)
	{
		playerCurrHealth = 100.f;
		--playerCurrLives;
	}

	// Change weapon using the inventory slots
	if (Key(GLFW_KEY_1))
		weapon->SetCurrentSlot(0);
	if (Key(GLFW_KEY_2))
		weapon->SetCurrentSlot(1);
	if (Key(GLFW_KEY_3))
		weapon->SetCurrentSlot(2);

	// Update current weapon status to see whether can shoot
	static double lastTime = elapsedTime;
	weapon->GetCurrentWeapon()->Update(elapsedTime - lastTime);

	// TESTING ONLY FOR SHOOTING
	if(leftMB){
		if(weapon->GetCurrentWeapon()->GetCanShoot() && weapon->GetCurrentWeapon()->GetCurrentAmmoRound() > 0){
			Entity* const& bullet = entityManager->FetchEntity();
			bullet->type = Entity::EntityType::BULLET;
			bullet->active = true;
			bullet->pos = glm::vec3(cam.GetPos() + 10.f * cam.CalcFront());
			bullet->vel = cam.CalcFront() * 12.f;
			bullet->mass = 1.f;
			bullet->life = 200.f;
			//const glm::vec3& camFront = cam.CalcFront();
			//bullet->rotate = glm::vec4(cam.CalcUp(), glm::degrees(atan2(camFront.z, camFront.x)));
			bullet->scale = glm::vec3(1.f);
			bullet->mesh = meshes[(int)MeshType::Sphere];
			weapon->GetCurrentWeapon()->SetCanShoot(false); // For the shooting cooldown time
			weapon->GetCurrentWeapon()->SetCurrentAmmoRound(weapon->GetCurrentWeapon()->GetCurrentAmmoRound() - 1); // Decrease the ammo
			lastTime = elapsedTime;
		}
	}

	if(Key(GLFW_KEY_R)){ // Reload the curr weapon
		weapon->GetCurrentWeapon()->Reload();
	}

	EntityManager::UpdateParams params;
	entityManager->UpdateEntities(params);
	
	for (int i = 0; i < (int)WaveNumber::Total; ++i)
	{
		if (enemyCount == 0)
		{
			switch (waves[i])
			{
			case (int)WaveNumber::One:
				for (int i = 0; i < 10; ++i)
				{
					Entity* const& stillEnemy = entityManager->FetchEntity();
					stillEnemy->type = Entity::EntityType::STATIC_ENEMY;
					stillEnemy->active = true;
					stillEnemy->pos = glm::vec3(PseudorandMinMax(-50.f, 50.f), 200.f, PseudorandMinMax(-50.f, 50.f));
					stillEnemy->vel = glm::vec3(0.f);
					stillEnemy->mass = 5.f;
					stillEnemy->scale = glm::vec3(10.f);
					stillEnemy->mesh = meshes[(int)MeshType::Sphere];
					stillEnemy->model = models[(int)ModelType::Virus];
					++enemyCount;
				}
				break;

			case (int)WaveNumber::Total:
				i = 0;
				break;
			}
		}
		
	}
}

void Scene::GeoRenderPass() {
	geoPassSP.Use();
	geoPassSP.SetMat4fv("PV", &(projection * glm::mat4(glm::mat3(view)))[0][0]);

	///Sky
	glDepthFunc(GL_LEQUAL); //Modify comparison operators used for depth test such that frags with depth <= 1.f are shown
	glCullFace(GL_FRONT);
	geoPassSP.Set1i("sky", 1);
	modelStack.PushModel({
		modelStack.Rotate(glm::vec4(0.f, 1.f, 0.f, glfwGetTime())),
	});
		meshes[(int)MeshType::Sphere]->SetModel(modelStack.GetTopModel());
		meshes[(int)MeshType::Sphere]->Render(geoPassSP);
	modelStack.PopModel();
	geoPassSP.Set1i("sky", 0);
	glCullFace(GL_BACK);
	glDepthFunc(GL_LESS);

	geoPassSP.SetMat4fv("PV", &(projection * view)[0][0]);

	///Terrain
	modelStack.PushModel({
		modelStack.Scale(glm::vec3(500.f, 100.f, 500.f)),
	});
		meshes[(int)MeshType::Terrain]->SetModel(modelStack.GetTopModel());
		meshes[(int)MeshType::Terrain]->Render(geoPassSP);
	modelStack.PopModel();

	///Shapes
	modelStack.PushModel({
		modelStack.Translate(glm::vec3(0.f, 100.f, 0.f)),
		modelStack.Scale(glm::vec3(10.f)),
	});
		modelStack.PushModel({
			modelStack.Translate(glm::vec3(6.f, 0.f, 0.f)),
		});
			geoPassSP.Set1i("noNormals", 1);
			geoPassSP.Set1i("useCustomColour", 1);
			geoPassSP.Set4fv("customColour", glm::vec4(glm::vec3(5.f), 1.f));
			meshes[(int)MeshType::Quad]->SetModel(modelStack.GetTopModel());
			meshes[(int)MeshType::Quad]->Render(geoPassSP);
			geoPassSP.Set1i("useCustomColour", 0);
			geoPassSP.Set1i("noNormals", 0);
			modelStack.PushModel({
				modelStack.Translate(glm::vec3(0.f, 0.f, 5.f)),
			});
				meshes[(int)MeshType::Sphere]->SetModel(modelStack.GetTopModel());
				meshes[(int)MeshType::Sphere]->Render(geoPassSP);
				modelStack.PopModel();
			modelStack.PushModel({
				modelStack.Translate(glm::vec3(0.f, 0.f, -5.f)),
			});
				meshes[(int)MeshType::Cylinder]->SetModel(modelStack.GetTopModel());
				meshes[(int)MeshType::Cylinder]->Render(geoPassSP);
			modelStack.PopModel();
		modelStack.PopModel();
	modelStack.PopModel();
}

void Scene::LightingRenderPass(const uint& posTexRefID, const uint& coloursTexRefID, const uint& normalsTexRefID, const uint& specTexRefID, const uint& reflectionTexRefID) {
	lightingPassSP.Use();
	const int& pAmt = (int)ptLights.size();
	const int& dAmt = (int)directionalLights.size();
	const int& sAmt = (int)spotlights.size();

	lightingPassSP.Set1f("shininess", 32.f); //More light scattering if lower
	lightingPassSP.Set3fv("globalAmbient", Light::globalAmbient);
	lightingPassSP.Set3fv("camPos", cam.GetPos());
	lightingPassSP.Set1i("pAmt", pAmt);
	lightingPassSP.Set1i("dAmt", dAmt);
	lightingPassSP.Set1i("sAmt", sAmt);
	lightingPassSP.UseTex(posTexRefID, "posTex");
	lightingPassSP.UseTex(coloursTexRefID, "coloursTex");
	lightingPassSP.UseTex(normalsTexRefID, "normalsTex");
	lightingPassSP.UseTex(specTexRefID, "specTex");
	lightingPassSP.UseTex(reflectionTexRefID, "reflectionTex");

	int i;
	for (i = 0; i < pAmt; ++i) {
		const PtLight* const& ptLight = static_cast<PtLight*>(ptLights[i]);
		lightingPassSP.Set3fv(("ptLights[" + std::to_string(i) + "].ambient").c_str(), ptLight->ambient);
		lightingPassSP.Set3fv(("ptLights[" + std::to_string(i) + "].diffuse").c_str(), ptLight->diffuse);
		lightingPassSP.Set3fv(("ptLights[" + std::to_string(i) + "].spec").c_str(), ptLight->spec);
		lightingPassSP.Set3fv(("ptLights[" + std::to_string(i) + "].pos").c_str(), ptLight->pos);
		lightingPassSP.Set1f(("ptLights[" + std::to_string(i) + "].constant").c_str(), ptLight->constant);
		lightingPassSP.Set1f(("ptLights[" + std::to_string(i) + "].linear").c_str(), ptLight->linear);
		lightingPassSP.Set1f(("ptLights[" + std::to_string(i) + "].quadratic").c_str(), ptLight->quadratic);
	}
	for (i = 0; i < dAmt; ++i) {
		const DirectionalLight* const& directionalLight = static_cast<DirectionalLight*>(directionalLights[i]);
		lightingPassSP.Set3fv(("directionalLights[" + std::to_string(i) + "].ambient").c_str(), directionalLight->ambient);
		lightingPassSP.Set3fv(("directionalLights[" + std::to_string(i) + "].diffuse").c_str(), directionalLight->diffuse);
		lightingPassSP.Set3fv(("directionalLights[" + std::to_string(i) + "].spec").c_str(), directionalLight->spec);
		lightingPassSP.Set3fv(("directionalLights[" + std::to_string(i) + "].dir").c_str(), directionalLight->dir);
	}
	for (i = 0; i < sAmt; ++i) {
		const Spotlight* const& spotlight = static_cast<Spotlight*>(spotlights[i]);
		lightingPassSP.Set3fv(("spotlights[" + std::to_string(i) + "].ambient").c_str(), spotlight->ambient);
		lightingPassSP.Set3fv(("spotlights[" + std::to_string(i) + "].diffuse").c_str(), spotlight->diffuse);
		lightingPassSP.Set3fv(("spotlights[" + std::to_string(i) + "].spec").c_str(), spotlight->spec);
		lightingPassSP.Set3fv(("spotlights[" + std::to_string(i) + "].pos").c_str(), spotlight->pos);
		lightingPassSP.Set3fv(("spotlights[" + std::to_string(i) + "].dir").c_str(), spotlight->dir);
		lightingPassSP.Set1f(("spotlights[" + std::to_string(i) + "].cosInnerCutoff").c_str(), spotlight->cosInnerCutoff);
		lightingPassSP.Set1f(("spotlights[" + std::to_string(i) + "].cosOuterCutoff").c_str(), spotlight->cosOuterCutoff);
	}

	meshes[(int)MeshType::Quad]->SetModel(modelStack.GetTopModel());
	meshes[(int)MeshType::Quad]->Render(lightingPassSP, false);
	lightingPassSP.ResetTexUnits();
}

void Scene::BlurRender(const uint& brightTexRefID, const bool& horizontal) {
	blurSP.Use();
	blurSP.Set1i("horizontal", horizontal);
	blurSP.UseTex(brightTexRefID, "texSampler");
	meshes[(int)MeshType::Quad]->SetModel(modelStack.GetTopModel());
	meshes[(int)MeshType::Quad]->Render(blurSP, false);
	blurSP.ResetTexUnits();
}

void Scene::DefaultRender(const uint& screenTexRefID, const uint& blurTexRefID, const glm::vec3& translate, const glm::vec3& scale){
	screenSP.Use();
	screenSP.Set1f("exposure", 1.2f);
	screenSP.UseTex(screenTexRefID, "screenTexSampler");
	screenSP.UseTex(blurTexRefID, "blurTexSampler");
	modelStack.PushModel({
		modelStack.Translate(translate),
		modelStack.Scale(scale),
	});
		meshes[(int)MeshType::Quad]->SetModel(modelStack.GetTopModel());
		meshes[(int)MeshType::Quad]->Render(screenSP, false);
		modelStack.PopModel();
	screenSP.ResetTexUnits();
}

void Scene::DepthRender(const short& projectionType){
	depthSP.Use();
	if(projectionType){
		depthSP.SetMat4fv("PV", &(glm::perspective(glm::radians(45.f), sCam.GetAspectRatio(), 120.f, 5000.f) * sCam.LookAt())[0][0]);
	} else{
		depthSP.SetMat4fv("PV", &(glm::ortho(-300.f, 300.f, -300.f, 300.f, .1f, 500.f) * dCam.LookAt())[0][0]);
	}

	modelStack.PushModel({
		modelStack.Translate(glm::vec3(0.f, 100.f, -50.f)),
		modelStack.Scale(glm::vec3(50.f)),
	});
		meshes[(int)MeshType::Quad]->SetModel(modelStack.GetTopModel());
		meshes[(int)MeshType::Quad]->Render(depthSP, false);
	modelStack.PopModel();

	glCullFace(GL_FRONT);
	modelStack.PushModel({
		modelStack.Scale(glm::vec3(500.f, 100.f, 500.f)),
	});
		meshes[(int)MeshType::Terrain]->SetModel(modelStack.GetTopModel());
		meshes[(int)MeshType::Terrain]->Render(depthSP, false);
	modelStack.PopModel();

	modelStack.PushModel({
		modelStack.Translate(glm::vec3(0.f, 100.f, 0.f)),
		modelStack.Scale(glm::vec3(10.f)),
	});
			meshes[(int)MeshType::Cylinder]->SetModel(modelStack.GetTopModel());
			meshes[(int)MeshType::Cylinder]->Render(depthSP, false);
		modelStack.PushModel({
			modelStack.Translate(glm::vec3(-3.f, 0.f, 0.f)),
		});
			meshes[(int)MeshType::Sphere]->SetModel(modelStack.GetTopModel());
			meshes[(int)MeshType::Sphere]->Render(depthSP, false);
		modelStack.PopModel();
		modelStack.PushModel({
			modelStack.Translate(glm::vec3(3.f, 0.f, 0.f)),
		});
			meshes[(int)MeshType::Cube]->SetModel(modelStack.GetTopModel());
			meshes[(int)MeshType::Cube]->Render(depthSP, false);
		modelStack.PopModel();
	modelStack.PopModel();
	glCullFace(GL_BACK);
}

void Scene::PlanarReflectionRender(){
	forwardSP.Use();
	forwardSP.Set1f("shininess", 32.f); //More light scattering if lower
	forwardSP.Set3fv("globalAmbient", Light::globalAmbient);
	forwardSP.Set3fv("camPos", cam.GetPos());
	forwardSP.Set1i("pAmt", 0);
	forwardSP.Set1i("dAmt", 0);
	forwardSP.Set1i("sAmt", 0);

	forwardSP.SetMat4fv("PV", &(glm::perspective(glm::radians(90.f), waterCam.GetAspectRatio(), .1f, 9999.f) * glm::mat4(glm::mat3(waterCam.LookAt())))[0][0]);

	glDepthFunc(GL_LEQUAL); //Modify comparison operators used for depth test such that frags with depth <= 1.f are shown
	glCullFace(GL_FRONT);
	forwardSP.Set1i("sky", 1);
	modelStack.PushModel({
		modelStack.Rotate(glm::vec4(0.f, 1.f, 0.f, glfwGetTime())),
	});
		meshes[(int)MeshType::Sphere]->SetModel(modelStack.GetTopModel());
		meshes[(int)MeshType::Sphere]->Render(forwardSP);
	modelStack.PopModel();
	forwardSP.Set1i("sky", 0);
	glCullFace(GL_BACK);
	glDepthFunc(GL_LESS);

	forwardSP.SetMat4fv("PV", &(glm::perspective(glm::radians(90.f), 1.f, .1f, 9999.f) * waterCam.LookAt())[0][0]);

	modelStack.PushModel({
		modelStack.Translate(glm::vec3(0.f, 100.f, -50.f)),
		modelStack.Scale(glm::vec3(50.f)),
	});
		meshes[(int)MeshType::Quad]->SetModel(modelStack.GetTopModel());
		meshes[(int)MeshType::Quad]->Render(forwardSP);
	modelStack.PopModel();

	modelStack.PushModel({
		modelStack.Translate(glm::vec3(0.f, 100.f, 0.f)),
		modelStack.Scale(glm::vec3(10.f)),
	});
		meshes[(int)MeshType::Cylinder]->SetModel(modelStack.GetTopModel());
		meshes[(int)MeshType::Cylinder]->Render(forwardSP);
		modelStack.PushModel({
			modelStack.Translate(glm::vec3(-3.f, 0.f, 0.f)),
		});
			forwardSP.Set1i("useCustomDiffuseTexIndex", 1);
			forwardSP.Set1i("customDiffuseTexIndex", -1);
			meshes[(int)MeshType::Sphere]->SetModel(modelStack.GetTopModel());
			meshes[(int)MeshType::Sphere]->Render(forwardSP);
			forwardSP.Set1i("useCustomDiffuseTexIndex", 0);
		modelStack.PopModel();
		modelStack.PushModel({
			modelStack.Translate(glm::vec3(3.f, 0.f, 0.f)),
		});
			forwardSP.Set1i("useCustomDiffuseTexIndex", 1);
			forwardSP.Set1i("customDiffuseTexIndex", -1);
			meshes[(int)MeshType::Cube]->SetModel(modelStack.GetTopModel());
			meshes[(int)MeshType::Cube]->Render(forwardSP);
			forwardSP.Set1i("useCustomDiffuseTexIndex", 0);
		modelStack.PopModel();
	modelStack.PopModel();
}

void Scene::CubemapReflectionRender(const short& cubemapFace){
	forwardSP.Use();
	forwardSP.Set1f("shininess", 32.f); //More light scattering if lower
	forwardSP.Set3fv("globalAmbient", Light::globalAmbient);
	forwardSP.Set3fv("camPos", cam.GetPos());
	forwardSP.Set1i("pAmt", 0);
	forwardSP.Set1i("dAmt", 0);
	forwardSP.Set1i("sAmt", 0);

	enCam.SetPos(glm::vec3(60.f, 100.f, 50.f));
	switch(cubemapFace){
		case 1:
			enCam.SetTarget(enCam.GetPos() + glm::vec3(1.f, 0.f, 0.f));
			enCam.SetUp(glm::vec3(0.f, -1.f, 0.f));
			break;
		case 2:
			enCam.SetTarget(enCam.GetPos() + glm::vec3(-1.f, 0.f, 0.f));
			enCam.SetUp(glm::vec3(0.f, -1.f, 0.f));
			break;
		case 3:
			enCam.SetTarget(enCam.GetPos() + glm::vec3(0.f, 1.f, 0.f));
			enCam.SetUp(glm::vec3(0.f, 0.f, 1.f));
			break;
		case 4:
			enCam.SetTarget(enCam.GetPos() + glm::vec3(0.f, -1.f, 0.f));
			enCam.SetUp(glm::vec3(0.f, 0.f, 1.f)); 
			break;
		case 5:
			enCam.SetTarget(enCam.GetPos() + glm::vec3(0.f, 0.f, 1.f));
			enCam.SetUp(glm::vec3(0.f, -1.f, 0.f));
			break;
		case 6:
			enCam.SetTarget(enCam.GetPos() + glm::vec3(0.f, 0.f, -1.f));
			enCam.SetUp(glm::vec3(0.f, -1.f, 0.f));
			break;
	}

	forwardSP.SetMat4fv("PV", &(glm::perspective(glm::radians(90.f), enCam.GetAspectRatio(), .1f, 9999.f) * glm::mat4(glm::mat3(enCam.LookAt())))[0][0]);

	glDepthFunc(GL_LEQUAL); //Modify comparison operators used for depth test such that frags with depth <= 1.f are shown
	glCullFace(GL_FRONT);
	forwardSP.Set1i("sky", 1);
	modelStack.PushModel({
		modelStack.Rotate(glm::vec4(0.f, 1.f, 0.f, glfwGetTime())),
	});
		meshes[(int)MeshType::Sphere]->SetModel(modelStack.GetTopModel());
		meshes[(int)MeshType::Sphere]->Render(forwardSP);
	modelStack.PopModel();
	forwardSP.Set1i("sky", 0);
	glCullFace(GL_BACK);
	glDepthFunc(GL_LESS);

	forwardSP.SetMat4fv("PV", &(glm::perspective(glm::radians(90.f), 1.f, .1f, 9999.f) * enCam.LookAt())[0][0]);

	modelStack.PushModel({
		modelStack.Translate(glm::vec3(0.f, 100.f, 0.f)),
		modelStack.Scale(glm::vec3(10.f)),
	});
		modelStack.PushModel({
			modelStack.Translate(glm::vec3(6.f, 0.f, 0.f)),
		});
			modelStack.PushModel({
				modelStack.Translate(glm::vec3(5.f, 0.f, 5.f)),
			});
				meshes[(int)MeshType::Cylinder]->SetModel(modelStack.GetTopModel());
				meshes[(int)MeshType::Cylinder]->Render(forwardSP);
			modelStack.PopModel();
		modelStack.PopModel();
	modelStack.PopModel();
}

void Scene::ForwardRender(const uint& depthDTexRefID, const uint& depthSTexRefID, const uint& planarReflectionTexID, const uint& cubemapReflectionTexID){
	forwardSP.Use();
	forwardSP.SetMat4fv("directionalLightPV", &(glm::ortho(-300.f, 300.f, -300.f, 300.f, .1f, 500.f) * dCam.LookAt())[0][0]);
	forwardSP.SetMat4fv("spotlightPV", &(glm::perspective(glm::radians(45.f), 1.f, 120.f, 5000.f) * sCam.LookAt())[0][0]);

	const int& pAmt = (int)ptLights.size();
	const int& dAmt = (int)directionalLights.size();
	const int& sAmt = (int)spotlights.size();

	forwardSP.Set1f("shininess", 32.f); //More light scattering if lower
	forwardSP.Set3fv("globalAmbient", Light::globalAmbient);
	forwardSP.Set3fv("camPos", cam.GetPos());
	forwardSP.Set1i("pAmt", pAmt);
	forwardSP.Set1i("dAmt", dAmt);
	forwardSP.Set1i("sAmt", sAmt);

	int i;
	for(i = 0; i < pAmt; ++i){
		const PtLight* const& ptLight = static_cast<PtLight*>(ptLights[i]);
		forwardSP.Set3fv(("ptLights[" + std::to_string(i) + "].ambient").c_str(), ptLight->ambient);
		forwardSP.Set3fv(("ptLights[" + std::to_string(i) + "].diffuse").c_str(), ptLight->diffuse);
		forwardSP.Set3fv(("ptLights[" + std::to_string(i) + "].spec").c_str(), ptLight->spec);
		forwardSP.Set3fv(("ptLights[" + std::to_string(i) + "].pos").c_str(), ptLight->pos);
		forwardSP.Set1f(("ptLights[" + std::to_string(i) + "].constant").c_str(), ptLight->constant);
		forwardSP.Set1f(("ptLights[" + std::to_string(i) + "].linear").c_str(), ptLight->linear);
		forwardSP.Set1f(("ptLights[" + std::to_string(i) + "].quadratic").c_str(), ptLight->quadratic);
	}
	for(i = 0; i < dAmt; ++i){
		const DirectionalLight* const& directionalLight = static_cast<DirectionalLight*>(directionalLights[i]);
		forwardSP.Set3fv(("directionalLights[" + std::to_string(i) + "].ambient").c_str(), directionalLight->ambient);
		forwardSP.Set3fv(("directionalLights[" + std::to_string(i) + "].diffuse").c_str(), directionalLight->diffuse);
		forwardSP.Set3fv(("directionalLights[" + std::to_string(i) + "].spec").c_str(), directionalLight->spec);
		forwardSP.Set3fv(("directionalLights[" + std::to_string(i) + "].dir").c_str(), directionalLight->dir);
	}
	for(i = 0; i < sAmt; ++i){
		const Spotlight* const& spotlight = static_cast<Spotlight*>(spotlights[i]);
		forwardSP.Set3fv(("spotlights[" + std::to_string(i) + "].ambient").c_str(), spotlight->ambient);
		forwardSP.Set3fv(("spotlights[" + std::to_string(i) + "].diffuse").c_str(), spotlight->diffuse);
		forwardSP.Set3fv(("spotlights[" + std::to_string(i) + "].spec").c_str(), spotlight->spec);
		forwardSP.Set3fv(("spotlights[" + std::to_string(i) + "].pos").c_str(), spotlight->pos);
		forwardSP.Set3fv(("spotlights[" + std::to_string(i) + "].dir").c_str(), spotlight->dir);
		forwardSP.Set1f(("spotlights[" + std::to_string(i) + "].cosInnerCutoff").c_str(), spotlight->cosInnerCutoff);
		forwardSP.Set1f(("spotlights[" + std::to_string(i) + "].cosOuterCutoff").c_str(), spotlight->cosOuterCutoff);
	}

	forwardSP.SetMat4fv("PV", &(projection * glm::mat4(glm::mat3(view)))[0][0]);

	///Sky
	glDepthFunc(GL_LEQUAL); //Modify comparison operators used for depth test such that frags with depth <= 1.f are shown
	glCullFace(GL_FRONT);
	forwardSP.Set1i("sky", 1);
	modelStack.PushModel({
		modelStack.Rotate(glm::vec4(0.f, 1.f, 0.f, glfwGetTime())),
	});
		meshes[(int)MeshType::Sphere]->SetModel(modelStack.GetTopModel());
		meshes[(int)MeshType::Sphere]->Render(forwardSP);
		modelStack.PopModel();
	forwardSP.Set1i("sky", 0);
	glCullFace(GL_BACK);
	glDepthFunc(GL_LESS);

	forwardSP.SetMat4fv("PV", &(projection * view)[0][0]);

	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	//Test wall
	modelStack.PushModel({
		modelStack.Translate(glm::vec3(0.f, 100.f, -50.f)),
		modelStack.Scale(glm::vec3(50.f)),
	});
		forwardSP.UseTex(depthDTexRefID, "dDepthTexSampler");
		forwardSP.UseTex(depthSTexRefID, "sDepthTexSampler");
		meshes[(int)MeshType::Quad]->SetModel(modelStack.GetTopModel());
		meshes[(int)MeshType::Quad]->Render(forwardSP);
	modelStack.PopModel();

	///Terrain
	modelStack.PushModel({
		modelStack.Scale(glm::vec3(terrainXScale, terrainYScale, terrainZScale)),
	});
		forwardSP.UseTex(depthDTexRefID, "dDepthTexSampler");
		forwardSP.UseTex(depthSTexRefID, "sDepthTexSampler");
		meshes[(int)MeshType::Terrain]->SetModel(modelStack.GetTopModel());
		meshes[(int)MeshType::Terrain]->Render(forwardSP);
	modelStack.PopModel();

	///Shapes
	modelStack.PushModel({
		modelStack.Translate(glm::vec3(0.f, 100.f, 0.f)),
		modelStack.Scale(glm::vec3(10.f)),
	});
		modelStack.PushModel({
			modelStack.Translate(glm::vec3(6.f, 0.f, 0.f)),
		});
			forwardSP.UseTex(depthDTexRefID, "dDepthTexSampler");
			forwardSP.UseTex(depthSTexRefID, "sDepthTexSampler");
			forwardSP.Set1i("noNormals", 1);
			forwardSP.Set1i("useCustomColour", 1);
			forwardSP.Set4fv("customColour", glm::vec4(glm::vec3(5.f), 1.f));
			meshes[(int)MeshType::Quad]->SetModel(modelStack.GetTopModel());
			meshes[(int)MeshType::Quad]->Render(forwardSP);
			forwardSP.Set1i("useCustomColour", 0);
			forwardSP.Set1i("noNormals", 0);
			modelStack.PushModel({
				modelStack.Translate(glm::vec3(0.f, 0.f, 5.f)),
				modelStack.Scale(glm::vec3(3.f)),
			});
				forwardSP.UseTex(depthDTexRefID, "dDepthTexSampler");
				forwardSP.UseTex(depthSTexRefID, "sDepthTexSampler");
				forwardSP.UseTex(cubemapReflectionTexID, "cubemapSampler", GL_TEXTURE_CUBE_MAP);
				forwardSP.Set1i("useCustomDiffuseTexIndex", 1);
				forwardSP.Set1i("customDiffuseTexIndex", -1);
				forwardSP.Set1i("useCustomColour", 1);
				forwardSP.Set4fv("customColour", glm::vec4(glm::vec3(0.f), 1.f));
				meshes[(int)MeshType::Cube]->SetModel(modelStack.GetTopModel());
				meshes[(int)MeshType::Cube]->Render(forwardSP);
				forwardSP.Set1i("useCustomColour", 0);
				forwardSP.Set1i("useCustomDiffuseTexIndex", 0);
			modelStack.PopModel();
			modelStack.PushModel({
				modelStack.Translate(glm::vec3(5.f, 0.f, 5.f)),
			});
				forwardSP.UseTex(depthDTexRefID, "dDepthTexSampler");
				forwardSP.UseTex(depthSTexRefID, "sDepthTexSampler");
				meshes[(int)MeshType::Cylinder]->SetModel(modelStack.GetTopModel());
				meshes[(int)MeshType::Cylinder]->Render(forwardSP);
			modelStack.PopModel();
		modelStack.PopModel();
	modelStack.PopModel();

	modelStack.PushModel({
		modelStack.Translate(glm::vec3(0.f, 100.f, 0.f)),
		modelStack.Scale(glm::vec3(10.f)),
	});
		forwardSP.UseTex(depthDTexRefID, "dDepthTexSampler");
		forwardSP.UseTex(depthSTexRefID, "sDepthTexSampler");
		meshes[(int)MeshType::Cylinder]->SetModel(modelStack.GetTopModel());
		meshes[(int)MeshType::Cylinder]->Render(forwardSP);
		modelStack.PushModel({
			modelStack.Translate(glm::vec3(-3.f, 0.f, 0.f)),
		});
			forwardSP.UseTex(depthDTexRefID, "dDepthTexSampler");
			forwardSP.UseTex(depthSTexRefID, "sDepthTexSampler");
			forwardSP.Set1i("useCustomDiffuseTexIndex", 1);
			forwardSP.Set1i("customDiffuseTexIndex", -1);
			meshes[(int)MeshType::Sphere]->SetModel(modelStack.GetTopModel());
			meshes[(int)MeshType::Sphere]->Render(forwardSP);
			forwardSP.Set1i("useCustomDiffuseTexIndex", 0);
		modelStack.PopModel();
		modelStack.PushModel({
			modelStack.Translate(glm::vec3(3.f, 0.f, 0.f)),
		});
			forwardSP.UseTex(depthDTexRefID, "dDepthTexSampler");
			forwardSP.UseTex(depthSTexRefID, "sDepthTexSampler");
			forwardSP.Set1i("useCustomDiffuseTexIndex", 1);
			forwardSP.Set1i("customDiffuseTexIndex", -1);
			meshes[(int)MeshType::Cube]->SetModel(modelStack.GetTopModel());
			meshes[(int)MeshType::Cube]->Render(forwardSP);
			forwardSP.Set1i("useCustomDiffuseTexIndex", 0);
			modelStack.PopModel();
	modelStack.PopModel();

	///SpriteAni
	modelStack.PushModel({
		modelStack.Translate(glm::vec3(0.f, 300.f, 0.f)),
		modelStack.Scale(glm::vec3(20.f, 40.f, 20.f)),
	});
		forwardSP.UseTex(depthDTexRefID, "dDepthTexSampler");
		forwardSP.UseTex(depthSTexRefID, "sDepthTexSampler");
		forwardSP.Set1i("noNormals", 1);
		forwardSP.Set1i("useCustomColour", 1);
		forwardSP.Set4fv("customColour", glm::vec4(1.f));
		meshes[(int)MeshType::SpriteAni]->SetModel(modelStack.GetTopModel());
		meshes[(int)MeshType::SpriteAni]->Render(forwardSP);
		forwardSP.Set1i("useCustomColour", 0);
		forwardSP.Set1i("noNormals", 0);
	modelStack.PopModel();

	///Water
	modelStack.PushModel({
		modelStack.Translate(glm::vec3(-15.f, 40.f, -20.f)),
		modelStack.Rotate(glm::vec4(1.f, 0.f, 0.f, -90.f)),
		modelStack.Scale(glm::vec3(180.f)),
	});
		forwardSP.UseTex(depthDTexRefID, "dDepthTexSampler");
		forwardSP.UseTex(depthSTexRefID, "sDepthTexSampler");
		forwardSP.UseTex(planarReflectionTexID, "planarReflectionTex");
		forwardSP.Set1i("water", 1);
		forwardSP.Set1f("elapsedTime", elapsedTime);
		forwardSP.Set1i("useCustomColour", 1);
		forwardSP.Set4fv("customColour", glm::vec4(glm::vec3(.2f), .7f));
		meshes[(int)MeshType::Water]->SetModel(modelStack.GetTopModel());
		meshes[(int)MeshType::Water]->Render(forwardSP);
		forwardSP.Set1i("useCustomColour", 0);
		forwardSP.Set1i("water", 0);
	modelStack.PopModel();

	entityManager->RenderEntities(forwardSP); //Render entities

	///Render curr weapon
	const glm::vec3 front = cam.CalcFront();
	const float sign = front.y < 0.f ? -1.f : 1.f;
	auto rotationMat = glm::rotate(glm::mat4(1.f), sign * acosf(glm::dot(front, glm::normalize(glm::vec3(front.x, 0.f, front.z)))), glm::normalize(glm::vec3(-front.z, 0.f, front.x)));
	modelStack.PushModel({
		modelStack.Translate(cam.GetPos() +
			glm::vec3(rotationMat * glm::vec4(RotateVecIn2D(glm::vec3(5.5f, -7.f, -13.f), atan2(front.x, front.z) + glm::radians(180.f), Axis::y), 1.f))),
		modelStack.Rotate(glm::vec4(glm::vec3(-front.z, 0.f, front.x), sign * glm::degrees(acosf(glm::dot(front, glm::normalize(glm::vec3(front.x, 0.f, front.z))))))),
		modelStack.Rotate(glm::vec4(0.f, 1.f, 0.f, glm::degrees(atan2(front.x, front.z)))),
		modelStack.Scale(glm::vec3(5.f)),
	});
		switch(weapon->GetCurrentSlot()){
			case 0:
				models[(int)ModelType::Pistol]->SetModelForAll(modelStack.GetTopModel());
				models[(int)ModelType::Pistol]->Render(forwardSP);
				break;
			case 1:
				models[(int)ModelType::AR]->SetModelForAll(modelStack.GetTopModel());
				models[(int)ModelType::AR]->Render(forwardSP);
				break;
			case 2:
				models[(int)ModelType::Sniper]->SetModelForAll(modelStack.GetTopModel());
				models[(int)ModelType::Sniper]->Render(forwardSP);
				break;
		}
	modelStack.PopModel();

	////Render GUI
	forwardSP.SetMat4fv("PV", &(glm::ortho(-float(winWidth) / 2.f, float(winWidth) / 2.f, -float(winHeight) / 2.f, float(winHeight) / 2.f, .1f, 9999.f))[0][0]);
	forwardSP.Set1i("noNormals", 1);
	forwardSP.Set1i("useCustomColour", 1);
	forwardSP.Set1i("useCustomDiffuseTexIndex", 1);

	///Render reticle
	if(weapon->GetCurrentSlot() < 2){
		modelStack.PushModel({
			modelStack.Translate(glm::vec3(0.f, 0.f, -9.f)),
			modelStack.Scale(glm::vec3(40.f, 40.f, 1.f)),
		});
			forwardSP.Set4fv("customColour", reticleColour);
			forwardSP.Set1i("customDiffuseTexIndex", 3);
			meshes[(int)MeshType::Quad]->SetModel(modelStack.GetTopModel());
			meshes[(int)MeshType::Quad]->Render(forwardSP);

			modelStack.PushModel({
				modelStack.Translate(glm::vec3(0.f, 0.f, 1.f)),
			});
			if(rightMB){
				modelStack.PushModel({
					modelStack.Scale(glm::vec3(.7f, .7f, 1.f)),
				});
			}
				forwardSP.Set1i("customDiffuseTexIndex", 4);
				meshes[(int)MeshType::Quad]->SetModel(modelStack.GetTopModel());
				meshes[(int)MeshType::Quad]->Render(forwardSP);
			if(rightMB){
				modelStack.PopModel();
			}
			modelStack.PopModel();
		modelStack.PopModel();
	}

	///Render health bar
	modelStack.PushModel({
		modelStack.Translate(glm::vec3(-float(winWidth) / 2.5f, float(winHeight) / 2.5f, -10.f)),
		modelStack.Scale(glm::vec3(float(winWidth) / 15.f, float(winHeight) / 50.f, 1.f)),
	});
		forwardSP.Set4fv("customColour", glm::vec4(glm::vec3(1.f, 0.f, 0.f), 1.f));
		forwardSP.Set1i("customDiffuseTexIndex", -1);
		meshes[(int)MeshType::Quad]->SetModel(modelStack.GetTopModel());
		meshes[(int)MeshType::Quad]->Render(forwardSP);

		modelStack.PushModel({
			modelStack.Translate(glm::vec3((playerCurrHealth - playerMaxHealth) / playerMaxHealth, 0.f, 1.f)), // Translate to the left based on the amount of health to go back to max health
			modelStack.Scale(glm::vec3(playerCurrHealth / playerMaxHealth, 1.f, 1.f)), // Scale the x component based on the current health
		});
			forwardSP.Set4fv("customColour", glm::vec4(glm::vec3(0.f, 1.f, 0.f), 1.f));
			forwardSP.Set1i("customDiffuseTexIndex", -1);
			meshes[(int)MeshType::Quad]->SetModel(modelStack.GetTopModel());
			meshes[(int)MeshType::Quad]->Render(forwardSP);
		modelStack.PopModel();
	modelStack.PopModel();

	///Render player lives
	for(float j = 0; j < playerMaxLives; ++j){
		modelStack.PushModel({
			modelStack.Translate(glm::vec3(-float(winWidth) / 2.2f, float(winHeight) / 2.2f, -9.f) + glm::vec3(75.f * (float)j, 0.f, 0.f)), //??
			modelStack.Scale(glm::vec3(25.f)),
		});
			if(i < playerCurrLives){
				forwardSP.Set4fv("customColour", glm::vec4(glm::vec3(1.f, 0.f, 0.f), 1.f));
			} else{
				forwardSP.Set4fv("customColour", glm::vec4(glm::vec3(0.3f), 1.f));
			}
			forwardSP.Set1i("customDiffuseTexIndex", 1);
			meshes[(int)MeshType::Quad]->SetModel(modelStack.GetTopModel());
			meshes[(int)MeshType::Quad]->Render(forwardSP);
		modelStack.PopModel();
	}

	///Render ammo bar
	modelStack.PushModel({
		modelStack.Translate(glm::vec3(float(winWidth) / 3.f, -float(winHeight) / 2.2f, -10.f)),
		modelStack.Scale(glm::vec3(float(winWidth) / 15.f, float(winHeight) / 50.f, 1.f)),
	});
		forwardSP.Set4fv("customColour", glm::vec4(glm::vec3(1.f, 0.f, 0.f), 1.f));
		forwardSP.Set1i("customDiffuseTexIndex", -1);
		meshes[(int)MeshType::Quad]->SetModel(modelStack.GetTopModel());
		meshes[(int)MeshType::Quad]->Render(forwardSP);

		///Show status of ammo bar(i.e. curr ammo of the round)
		modelStack.PushModel({
			modelStack.Translate(glm::vec3(-float(weapon->GetCurrentWeapon()->GetMaxAmmoRound() - weapon->GetCurrentWeapon()->GetCurrentAmmoRound())
			/ float(weapon->GetCurrentWeapon()->GetMaxAmmoRound()), 0.f, 1.f)), // Translate to the left based on the amount of ammo to go back to max ammo of the round
			modelStack.Scale(glm::vec3(float(weapon->GetCurrentWeapon()->GetCurrentAmmoRound())
			/ float(weapon->GetCurrentWeapon()->GetMaxAmmoRound()), 1.f, 1.f)), // Scale the x component based on the current ammo of the round
		});
			forwardSP.Set4fv("customColour", glm::vec4(glm::vec3(0.f, 1.f, 0.f), 1.f));
			forwardSP.Set1i("customDiffuseTexIndex", -1);
			meshes[(int)MeshType::Quad]->SetModel(modelStack.GetTopModel());
			meshes[(int)MeshType::Quad]->Render(forwardSP);
		modelStack.PopModel();
	modelStack.PopModel();
		
	for(int j = 0; j < 5; ++j){
		modelStack.PushModel({
			modelStack.Translate(glm::vec3(-float(winWidth) / 6.f, -float(winHeight) / 2.3f, -11.f) + glm::vec3(j * 100.f, 0.f, 0.f)),
			modelStack.Scale(glm::vec3(50.f)),
		});
			if(weapon->GetCurrentSlot() == j){
				forwardSP.Set4fv("customColour", glm::vec4(glm::vec3(0.f, 1.f, 0.f), 1.f));
			} else{
				forwardSP.Set4fv("customColour", glm::vec4(glm::vec3(1.f, 0.f, 0.f), 1.f));
			}
			forwardSP.Set1i("customDiffuseTexIndex", 2);
			meshes[(int)MeshType::Quad]->SetModel(modelStack.GetTopModel());
			meshes[(int)MeshType::Quad]->Render(forwardSP);
		modelStack.PopModel();
	}

	forwardSP.Set1i("useCustomDiffuseTexIndex", 0);
	forwardSP.Set1i("useCustomColour", 0);
	forwardSP.Set1i("noNormals", 0);

	str temp;
	switch(weapon->GetCurrentSlot()){
		case 0:
			temp = "Pistol";
			break;
		case 1:
			temp = "Assault Rifle";
			break;
		case 2:
			temp = "Sniper Rifle";
			break;
	}

	// Weapon type
	textChief.RenderText(textSP, {
		temp,
		1300.f,
		75.f,
		1.f,
		glm::vec4(1.f),
		0
	});
	// Weapon ammo
	textChief.RenderText(textSP, {
		std::to_string(weapon->GetCurrentWeapon()->GetCurrentAmmoRound()) + "/" + std::to_string(weapon->GetCurrentWeapon()->GetCurrentTotalAmmo()),
		1450.f,
		25.f,
		1.f,
		glm::vec4(1.f),
		0
	});
	// FPS
	textChief.RenderText(textSP, {
		"FPS: " + std::to_string(1.f / dt),
		25.f,
		25.f,
		1.f,
		glm::vec4(1.f, 1.f, 0.f, 1.f),
		0
	});

	glBlendFunc(GL_ONE, GL_ZERO);
	if(music && music->getIsPaused()){
		music->setIsPaused(false);
	}
}

void Scene::MinimapRender()
{
	forwardSP.Use();
	const int& pAmt = 0;
	const int& dAmt = 0;
	const int& sAmt = 0;
	//forwardSP.Set1i("nightVision", 0);
	forwardSP.Set1f("shininess", 32.f); //More light scattering if lower
	forwardSP.Set3fv("globalAmbient", Light::globalAmbient);
	forwardSP.Set3fv("camPos", cam.GetPos());
	forwardSP.Set1i("pAmt", pAmt);
	forwardSP.Set1i("dAmt", dAmt);
	forwardSP.Set1i("sAmt", sAmt);

	minimapcam.SetPos(glm::vec3(cam.GetPos().x, 1000.f, cam.GetPos().z));
	minimapcam.SetTarget(glm::vec3(cam.GetPos().x, 0.f, cam.GetPos().z));
	minimapcam.SetUp(glm::vec3(0.f, 0.f, -1.f));
	minimapView = minimapcam.LookAt();
	minimapProjection = glm::ortho(-float(winWidth) / 5.f, float(winWidth) / 5.f, -float(winHeight) / 5.f, float(winHeight) / 5.f, .1f, 99999.f);

	forwardSP.SetMat4fv("PV", &(minimapProjection * minimapView)[0][0]);

	modelStack.PushModel({
		modelStack.Scale(glm::vec3(500.f, 100.f, 500.f)),
	});
	meshes[(int)MeshType::Terrain]->SetModel(modelStack.GetTopModel());
	meshes[(int)MeshType::Terrain]->Render(forwardSP);
	modelStack.PopModel();
}

bool Scene::CheckCollision(const glm::vec3& pos, const glm::vec3& scale){
	glm::vec3 displacementVec = cam.GetPos() - pos;
	const float b = glm::dot(cam.CalcFront(), displacementVec);
	const float c = glm::dot(displacementVec, displacementVec) - scale.x * scale.x;
	if (b * b - c >= 0.f) {
		return true;
	}
	return false;
}