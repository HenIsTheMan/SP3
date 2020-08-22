#include "Scene.h"
#include "Vendor/stb_image.h"
#include "../GDev/Pistol.h"
#include "../GDev/AssaultRifle.h"
#include "../GDev/SniperRifle.h"

extern float angularFOV;
extern float dt;
extern int winWidth;
extern int winHeight;

glm::vec3 Light::globalAmbient = glm::vec3(.2f);

Scene::Scene() :
	cam(glm::vec3(0.f, 0.f, 5.f), glm::vec3(0.f), glm::vec3(0.f, 1.f, 0.f), 0.f, 150.f),
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
		}),
		new Mesh(Mesh::MeshType::Cube, GL_TRIANGLES, {
			{"Imgs/BoxAlbedo.png", Mesh::TexType::Diffuse, 0},
		}),
		new Mesh(Mesh::MeshType::Sphere, GL_TRIANGLE_STRIP, {
			{"Imgs/Skydome.hdr", Mesh::TexType::Diffuse, 0},
		}),
		new Mesh(Mesh::MeshType::Cylinder, GL_TRIANGLE_STRIP, {
			{"Imgs/BoxAlbedo.png", Mesh::TexType::Diffuse, 0},
		}),
		new SpriteAni(4, 8),
		new Terrain("Imgs/hMap.raw", 8.f, 8.f),
},
models{
	new Model("ObjsAndMtls/Skydome.obj", {
		aiTextureType_DIFFUSE,
	}),
	new Model("ObjsAndMtls/nanosuit.obj", {
		aiTextureType_DIFFUSE,
		aiTextureType_SPECULAR,
		aiTextureType_EMISSIVE,
		aiTextureType_AMBIENT,
		//aiTextureType_HEIGHT,
	}),
},
blurSP{ "Shaders/Quad.vs", "Shaders/Blur.fs" },
forwardSP{ "Shaders/Forward.vs", "Shaders/Forward.fs" },
geoPassSP{ "Shaders/GeoPass.vs", "Shaders/GeoPass.fs" },
lightingPassSP{ "Shaders/Quad.vs", "Shaders/LightingPass.fs" },
normalsSP{ "Shaders/Normals.vs", "Shaders/Normals.fs", "Shaders/Normals.gs" }, //??
screenSP{ "Shaders/Quad.vs", "Shaders/Screen.fs" },
textSP{ "Shaders/Text.vs", "Shaders/Text.fs" },
ptLights({}),
directionalLights({}),
spotlights({}),
view(glm::mat4(1.f)),
projection(glm::mat4(1.f)),
elapsedTime(0.f),
//polyMode(0),
modelStack(),
playerCurrHealth(100.f),
playerMaxHealth(100.f),
playerCurrLives(5.f),
playerMaxLives(5.f)
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
	if (entityManager) {
		entityManager->Destroy();
		entityManager = nullptr;
	}
	if (weapon) {
		weapon = NULL;
	}
}

bool Scene::Init() {
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

	meshes[(int)MeshType::Terrain]->AddTexMap({ "Imgs/GrassGround.jpg", Mesh::TexType::Diffuse, 0 });

	spotlights.emplace_back(CreateLight(LightType::Spot));

	entityManager = EntityManager::GetObjPtr();
	entityManager->Init();
	// Create Player
	Entity* player = new Entity(Entity::EntityType::PLAYER,
		true, // Always active
		cam.GetPos(), // Need to check first
		glm::vec3(5.f),
		glm::vec4(0.f),
		glm::vec3(0.f));
	entityManager->AddEntity(player);

	// For the weapon hold -> Anyhow create the variables because the rendering of entities changed, so the real values are used in the forward render function
	Entity* weaponhold = new Entity(Entity::EntityType::WEAPONHOLD,
		true,
		glm::vec3(0.f),
		glm::vec3(5.f),
		glm::vec4(0.f),
		glm::vec3(0.f));
	entityManager->AddEntity(weaponhold);

	// Create Particles
	for (int i = 0; i < 100; ++i)
	{
		Entity* particle = new Entity(Entity::EntityType::PARTICLE,  
			false, // False first to be able to control the rendering from the start
			glm::vec3(PseudorandMinMax(-100.f, 100.f), PseudorandMinMax(100.f, 200.f), 0.f),  
			glm::vec3(5.f), 
			glm::vec4(0.f),
			glm::vec3(0.f));
		entityManager->AddEntity(particle);
	}

	// Create HealthBar
	Entity* healthbar = new Entity(Entity::EntityType::HEALTHBAR,
		true, // Always active
		glm::vec3(-float(winWidth) / 2.5f, float(winHeight) / 2.5f, -10.f),
		glm::vec3(float(winWidth) / 15.f, float(winHeight) / 50.f, 1.f),
		glm::vec4(0.f),
		glm::vec3(0.f));
	entityManager->AddEntity(healthbar);

	// Create the player lives on top of the HealthBar
	Entity* playerlives = new Entity(Entity::EntityType::PLAYERLIVES,
		true, // Always active at first
		glm::vec3(-float(winWidth) / 2.2f, float(winHeight) / 2.2f, -9.f),
		glm::vec3(25.f, 25.f, 1.f),
		glm::vec4(0.f),
		glm::vec3(0.f));
	entityManager->AddEntity(playerlives);

	// Create Ammo Bar
	Entity* ammoBar = new Entity(Entity::EntityType::AMMOBAR,
		true,
		glm::vec3(float(winWidth) / 3.f, -float(winHeight) / 2.2f, -10.f),
		glm::vec3(float(winWidth) / 15.f, float(winHeight) / 50.f, 1.f),
		glm::vec4(0.f),
		glm::vec3(0.f));
	entityManager->AddEntity(ammoBar);

	// Create Inventory Slots
	Entity* inventory = new Entity(Entity::EntityType::INVENTORY,
			true,
		glm::vec3(-float(winWidth) / 6.f, -float(winHeight) / 2.2f, -11.f),
		glm::vec3(35.f, 35.f, 1.f),
		glm::vec4(0.f),
		glm::vec3(0.f));
	entityManager->AddEntity(inventory);

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

	return true;
}

void Scene::Update() {
	elapsedTime += dt;
	if (winHeight) { //Avoid division by 0 when win is minimised
		cam.SetDefaultAspectRatio(float(winWidth) / float(winHeight));
		cam.ResetAspectRatio();
	}
	cam.Update(GLFW_KEY_Q, GLFW_KEY_E, GLFW_KEY_A, GLFW_KEY_D, GLFW_KEY_W, GLFW_KEY_S);
	view = cam.LookAt();
	projection = glm::perspective(glm::radians(angularFOV), cam.GetAspectRatio(), .1f, 9999.f);

	const glm::vec3& camPos = cam.GetPos();
	const glm::vec3& camFront = cam.CalcFront();
	soundEngine->setListenerPosition(vec3df(camPos.x, camPos.y, camPos.z), vec3df(camFront.x, camFront.y, camFront.z));

	spotlights[0]->ambient = glm::vec3(.05f);
	spotlights[0]->diffuse = glm::vec3(.8f);
	spotlights[0]->spec = glm::vec3(1.f);
	static_cast<Spotlight*>(spotlights[0])->pos = camPos;
	static_cast<Spotlight*>(spotlights[0])->dir = camFront;
	static_cast<Spotlight*>(spotlights[0])->cosInnerCutoff = cosf(glm::radians(12.5f));
	static_cast<Spotlight*>(spotlights[0])->cosOuterCutoff = cosf(glm::radians(17.5f));

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

	if (soundFX) {
		if (Key(GLFW_KEY_I) && distortionBT <= elapsedTime) {
			soundFX->isDistortionSoundEffectEnabled() ? soundFX->disableDistortionSoundEffect() : (void)soundFX->enableDistortionSoundEffect();
			distortionBT = elapsedTime + .5f;
		}
		if (Key(GLFW_KEY_O) && echoBT <= elapsedTime) {
			soundFX->isEchoSoundEffectEnabled() ? soundFX->disableEchoSoundEffect() : (void)soundFX->enableEchoSoundEffect();
			echoBT = elapsedTime + .5f;
		}
		if (Key(GLFW_KEY_P) && wavesReverbBT <= elapsedTime) {
			soundFX->isWavesReverbSoundEffectEnabled() ? soundFX->disableWavesReverbSoundEffect() : (void)soundFX->enableWavesReverbSoundEffect();
			wavesReverbBT = elapsedTime + .5f;
		}
		if (Key(GLFW_KEY_L) && resetSoundFXBT <= elapsedTime) {
			soundFX->disableAllEffects();
			resetSoundFXBT = elapsedTime + .5f;
		}
	}
	// TESTING ONLY FOR HEALTHBAR
	if (Key(GLFW_KEY_SPACE))
	{
		playerCurrHealth -= 1.f;
	}

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
	if (Key(GLFW_KEY_5))
	{
		if (weapon->GetCurrentWeapon()->GetCanShoot() && weapon->GetCurrentWeapon()->GetCurrentAmmoRound() > 0)
		{
			Entity* bullet = new Entity(Entity::EntityType::BULLET,
				false, 
				glm::vec3(cam.GetPos() + 10.f * cam.CalcFront()),
				glm::vec3(1.f),
				glm::vec4(0.f),
				cam.CalcFront());
			entityManager->AddEntity(bullet);
			weapon->GetCurrentWeapon()->SetCanShoot(false); // For the shooting cooldown time
			weapon->GetCurrentWeapon()->SetCurrentAmmoRound(weapon->GetCurrentWeapon()->GetCurrentAmmoRound() - 1); // Decrease the ammo
			lastTime = elapsedTime;
			bullet->lifeTime = 200;
		}
	}

	if (Key(GLFW_KEY_R)) // Reload the current weapon
		weapon->GetCurrentWeapon()->Reload();

	// Can be modified to be used for other entities too
	entityManager->Update(1, cam.CalcFront()); // Number of particles to be rendered every frame
	
}

void Scene::GeoRenderPass() {
	geoPassSP.Use();
	geoPassSP.SetMat4fv("PV", &(projection * glm::mat4(glm::mat3(view)))[0][0]);

	///Sky
	glDepthFunc(GL_LEQUAL); //Modify comparison operators used for depth test such that frags with depth <= 1.f are shown
	glCullFace(GL_FRONT);
	geoPassSP.Set1i("sky", 1);
	PushModel({
		Rotate(glm::vec4(0.f, 1.f, 0.f, glfwGetTime())),
		});
	meshes[(int)MeshType::Sphere]->SetModel(GetTopModel());
	meshes[(int)MeshType::Sphere]->Render(geoPassSP);
	PopModel();
	geoPassSP.Set1i("sky", 0);
	glCullFace(GL_BACK);
	glDepthFunc(GL_LESS);

	geoPassSP.SetMat4fv("PV", &(projection * view)[0][0]);

	///Terrain
	PushModel({
		Rotate(glm::vec4(0.f, 1.f, 0.f, 45.f)),
		Scale(glm::vec3(500.f, 100.f, 500.f)),
		});
	meshes[(int)MeshType::Terrain]->SetModel(GetTopModel());
	meshes[(int)MeshType::Terrain]->Render(geoPassSP);
	PopModel();

	///Shapes
	PushModel({
		Translate(glm::vec3(0.f, 100.f, 0.f)),
		Scale(glm::vec3(10.f)),
		});
	PushModel({
		Translate(glm::vec3(6.f, 0.f, 0.f)),
		});
	geoPassSP.Set1i("noNormals", 1);
	geoPassSP.Set1i("useCustomColour", 1);
	geoPassSP.Set4fv("customColour", glm::vec4(glm::vec3(5.f), 1.f));
	meshes[(int)MeshType::Quad]->SetModel(GetTopModel());
	meshes[(int)MeshType::Quad]->Render(geoPassSP);
	geoPassSP.Set1i("useCustomColour", 0);
	geoPassSP.Set1i("noNormals", 0);
	PushModel({
		Translate(glm::vec3(0.f, 0.f, 5.f)),
		});
	meshes[(int)MeshType::Sphere]->SetModel(GetTopModel());
	meshes[(int)MeshType::Sphere]->Render(geoPassSP);
	PopModel();
	PushModel({
		Translate(glm::vec3(0.f, 0.f, -5.f)),
		});
	meshes[(int)MeshType::Cylinder]->SetModel(GetTopModel());
	meshes[(int)MeshType::Cylinder]->Render(geoPassSP);
	PopModel();
	PopModel();
	PopModel();
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

	meshes[(int)MeshType::Quad]->SetModel(GetTopModel());
	meshes[(int)MeshType::Quad]->Render(lightingPassSP, false);
	lightingPassSP.ResetTexUnits();
}

void Scene::BlurRender(const uint& brightTexRefID, const bool& horizontal) {
	blurSP.Use();
	blurSP.Set1i("horizontal", horizontal);
	blurSP.UseTex(brightTexRefID, "texSampler");
	meshes[(int)MeshType::Quad]->SetModel(GetTopModel());
	meshes[(int)MeshType::Quad]->Render(blurSP, false);
	blurSP.ResetTexUnits();
}

void Scene::DefaultRender(const uint& screenTexRefID, const uint& blurTexRefID) {
	screenSP.Use();
	screenSP.Set1f("exposure", 1.2f);
	screenSP.UseTex(screenTexRefID, "screenTexSampler");
	screenSP.UseTex(blurTexRefID, "blurTexSampler");
	meshes[(int)MeshType::Quad]->SetModel(GetTopModel());
	meshes[(int)MeshType::Quad]->Render(screenSP, false);
	screenSP.ResetTexUnits();
}

void Scene::ForwardRender() {
	forwardSP.Use();
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
	for (i = 0; i < pAmt; ++i) {
		const PtLight* const& ptLight = static_cast<PtLight*>(ptLights[i]);
		forwardSP.Set3fv(("ptLights[" + std::to_string(i) + "].ambient").c_str(), ptLight->ambient);
		forwardSP.Set3fv(("ptLights[" + std::to_string(i) + "].diffuse").c_str(), ptLight->diffuse);
		forwardSP.Set3fv(("ptLights[" + std::to_string(i) + "].spec").c_str(), ptLight->spec);
		forwardSP.Set3fv(("ptLights[" + std::to_string(i) + "].pos").c_str(), ptLight->pos);
		forwardSP.Set1f(("ptLights[" + std::to_string(i) + "].constant").c_str(), ptLight->constant);
		forwardSP.Set1f(("ptLights[" + std::to_string(i) + "].linear").c_str(), ptLight->linear);
		forwardSP.Set1f(("ptLights[" + std::to_string(i) + "].quadratic").c_str(), ptLight->quadratic);
	}
	for (i = 0; i < dAmt; ++i) {
		const DirectionalLight* const& directionalLight = static_cast<DirectionalLight*>(directionalLights[i]);
		forwardSP.Set3fv(("directionalLights[" + std::to_string(i) + "].ambient").c_str(), directionalLight->ambient);
		forwardSP.Set3fv(("directionalLights[" + std::to_string(i) + "].diffuse").c_str(), directionalLight->diffuse);
		forwardSP.Set3fv(("directionalLights[" + std::to_string(i) + "].spec").c_str(), directionalLight->spec);
		forwardSP.Set3fv(("directionalLights[" + std::to_string(i) + "].dir").c_str(), directionalLight->dir);
	}
	for (i = 0; i < sAmt; ++i) {
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
	PushModel({
		Rotate(glm::vec4(0.f, 1.f, 0.f, glfwGetTime())),
		});
	meshes[(int)MeshType::Sphere]->SetModel(GetTopModel());
	meshes[(int)MeshType::Sphere]->Render(forwardSP);
	PopModel();
	forwardSP.Set1i("sky", 0);
	glCullFace(GL_BACK);
	glDepthFunc(GL_LESS);

	forwardSP.SetMat4fv("PV", &(projection * view)[0][0]);

	///Terrain
	PushModel({
		Rotate(glm::vec4(0.f, 1.f, 0.f, 45.f)),
		Scale(glm::vec3(500.f, 100.f, 500.f)),
		});
	meshes[(int)MeshType::Terrain]->SetModel(GetTopModel());
	meshes[(int)MeshType::Terrain]->Render(forwardSP);
	PopModel();

	///Shapes
	PushModel({
		Translate(glm::vec3(0.f, 100.f, 0.f)),
		Scale(glm::vec3(10.f)),
		});
	PushModel({
		Translate(glm::vec3(6.f, 0.f, 0.f)),
		});
	forwardSP.Set1i("noNormals", 1);
	forwardSP.Set1i("useCustomColour", 1);
	forwardSP.Set4fv("customColour", glm::vec4(glm::vec3(5.f), 1.f));
	meshes[(int)MeshType::Quad]->SetModel(GetTopModel());
	meshes[(int)MeshType::Quad]->Render(forwardSP);
	forwardSP.Set1i("useCustomColour", 0);
	forwardSP.Set1i("noNormals", 0);
	PushModel({
		Translate(glm::vec3(0.f, 0.f, 5.f)),
		});
	meshes[(int)MeshType::Sphere]->SetModel(GetTopModel());
	meshes[(int)MeshType::Sphere]->Render(forwardSP);
	PopModel();
	PushModel({
		Translate(glm::vec3(0.f, 0.f, -5.f)),
		});
	meshes[(int)MeshType::Cylinder]->SetModel(GetTopModel());
	meshes[(int)MeshType::Cylinder]->Render(forwardSP);
	PopModel();
	PopModel();
	PopModel();

	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	///Entities
	for (int i = 0; i < entityManager->getVector().size(); ++i)
	{
		Entity* entity = entityManager->getVector()[i];
		if (entity->active)
		{
			switch (entity->type)
			{
			case Entity::EntityType::PARTICLE:
				PushModel({
					Translate(glm::vec3(entity->pos.x, entity->pos.y, entity->pos.z)),
					//Rotate(glm::vec4(entity->rotate.x, entity->rotate.y, entity->rotate.z, entity->rotate.w)), // Not sure about the x,y,z etc
					Scale(glm::vec3(entity->scale.x, entity->scale.y, entity->scale.z)),
					});
				// Change the mesh or model accordingly
				meshes[(int)MeshType::Quad]->SetModel(GetTopModel());
				meshes[(int)MeshType::Quad]->Render(forwardSP); // Remember to change forwardSP etc accordingly
				PopModel();
				break;

			case Entity::EntityType::HEALTHBAR:
				///Render on the screen, change the projection
				projection = glm::ortho(-float(winWidth) / 2.f, float(winWidth) / 2.f, -float(winHeight) / 2.f, float(winHeight) / 2.f, .1f, 9999.f);
				forwardSP.SetMat4fv("PV", &(projection)[0][0]);
				
				// Shows the background of the HealthBar
				PushModel({
					Translate(glm::vec3(entity->pos.x, entity->pos.y, entity->pos.z)),
					Scale(glm::vec3(entity->scale.x, entity->scale.y, entity->scale.z)),
						});
				forwardSP.Set1i("noNormals", 1);
				forwardSP.Set1i("useCustomColour", 1);
				forwardSP.Set4fv("customColour", glm::vec4(glm::vec3(1.f, 0.f, 0.f), 1.f));
				forwardSP.Set1i("useCustomDiffuseTexIndex", 1);
				forwardSP.Set1i("customDiffuseTexIndex", -1);
				meshes[(int)MeshType::Quad]->SetModel(GetTopModel());
				meshes[(int)MeshType::Quad]->Render(forwardSP);
				forwardSP.Set1i("useCustomDiffuseTexIndex", 0);
				forwardSP.Set1i("useCustomColour", 0);
				forwardSP.Set1i("noNormals", 0);
				// Shows the status of the HealthBar (Current health of the player)
				PushModel({
					Translate(glm::vec3(-(playerMaxHealth - playerCurrHealth) / playerMaxHealth, 0.f, 1.f)), // Translate to the left based on the amount of health to go back to max health
					Scale(glm::vec3(playerCurrHealth / playerMaxHealth, 1.f, 1.f)), // Scale the x component based on the current health
					});
				forwardSP.Set1i("noNormals", 1);
				forwardSP.Set1i("useCustomColour", 1);
				forwardSP.Set4fv("customColour", glm::vec4(glm::vec3(0.f, 1.f, 0.f), 1.f));
				forwardSP.Set1i("useCustomDiffuseTexIndex", 1);
				forwardSP.Set1i("customDiffuseTexIndex", -1);
				meshes[(int)MeshType::Quad]->SetModel(GetTopModel());
				meshes[(int)MeshType::Quad]->Render(forwardSP);
				forwardSP.Set1i("useCustomDiffuseTexIndex", 0);
				forwardSP.Set1i("useCustomColour", 0);
				forwardSP.Set1i("noNormals", 0);
				PopModel();
				PopModel();
				// Change back the projection
				projection = glm::perspective(glm::radians(angularFOV), cam.GetAspectRatio(), .1f, 9999.f);
				forwardSP.SetMat4fv("PV", &(projection* view)[0][0]);
				break;

			case Entity::EntityType::PLAYERLIVES:
				///Render on the screen, change the projection
				projection = glm::ortho(-float(winWidth) / 2.f, float(winWidth) / 2.f, -float(winHeight) / 2.f, float(winHeight) / 2.f, .1f, 9999.f);
				forwardSP.SetMat4fv("PV", &(projection)[0][0]);

				// Shows the number of lives the player has
				for (int i = 0; i < playerCurrLives; ++i)
				{
					PushModel({
						Translate(glm::vec3(entity->pos.x + 75.f * (float)i, entity->pos.y, entity->pos.z)),
						Scale(glm::vec3(entity->scale.x, entity->scale.y, entity->scale.z)),
						});
					forwardSP.Set1i("noNormals", 1);
					forwardSP.Set1i("useCustomColour", 1);
					forwardSP.Set4fv("customColour", glm::vec4(glm::vec3(1.f, 0.f, 0.f), 1.f));
					forwardSP.Set1i("useCustomDiffuseTexIndex", 1);
					forwardSP.Set1i("customDiffuseTexIndex", -1);
					meshes[(int)MeshType::Quad]->SetModel(GetTopModel());
					meshes[(int)MeshType::Quad]->Render(forwardSP);
					forwardSP.Set1i("useCustomDiffuseTexIndex", 0);
					forwardSP.Set1i("useCustomColour", 0);
					forwardSP.Set1i("noNormals", 0);
					PopModel();
				}
				// Shows the number of lives the player lost
				for (int i = playerCurrLives; i < playerMaxLives; ++i)
				{
					PushModel({
						Translate(glm::vec3(entity->pos.x + 75.f * (float)i, entity->pos.y, entity->pos.z)),
						Scale(glm::vec3(entity->scale.x, entity->scale.y, entity->scale.z)),
						});
					forwardSP.Set1i("noNormals", 1);
					forwardSP.Set1i("useCustomColour", 1);
					forwardSP.Set4fv("customColour", glm::vec4(glm::vec3(0.3f), 1.f));
					forwardSP.Set1i("useCustomDiffuseTexIndex", 1);
					forwardSP.Set1i("customDiffuseTexIndex", -1);
					meshes[(int)MeshType::Quad]->SetModel(GetTopModel());
					meshes[(int)MeshType::Quad]->Render(forwardSP);
					forwardSP.Set1i("useCustomDiffuseTexIndex", 0);
					forwardSP.Set1i("useCustomColour", 0);
					forwardSP.Set1i("noNormals", 0);
					PopModel();
				}
				// Change back the projection
				projection = glm::perspective(glm::radians(angularFOV), cam.GetAspectRatio(), .1f, 9999.f);
				forwardSP.SetMat4fv("PV", &(projection * view)[0][0]);
				break;

			case Entity::EntityType::AMMOBAR:
				///Render on the screen, change the projection
				projection = glm::ortho(-float(winWidth) / 2.f, float(winWidth) / 2.f, -float(winHeight) / 2.f, float(winHeight) / 2.f, .1f, 9999.f);
				forwardSP.SetMat4fv("PV", &(projection)[0][0]);

				// Shows the background of the AmmoBar
				PushModel({
					Translate(glm::vec3(entity->pos.x, entity->pos.y, entity->pos.z)),
					Scale(glm::vec3(entity->scale.x, entity->scale.y, entity->scale.z)),
					});
				forwardSP.Set1i("noNormals", 1);
				forwardSP.Set1i("useCustomColour", 1);
				forwardSP.Set4fv("customColour", glm::vec4(glm::vec3(1.f, 0.f, 0.f), 1.f));
				forwardSP.Set1i("useCustomDiffuseTexIndex", 1);
				forwardSP.Set1i("customDiffuseTexIndex", -1);
				meshes[(int)MeshType::Quad]->SetModel(GetTopModel());
				meshes[(int)MeshType::Quad]->Render(forwardSP);
				forwardSP.Set1i("useCustomDiffuseTexIndex", 0);
				forwardSP.Set1i("useCustomColour", 0);
				forwardSP.Set1i("noNormals", 0);
				// Shows the status of the AmmoBar (Current ammo of the round)
				PushModel({
					Translate(glm::vec3(-float(weapon->GetCurrentWeapon()->GetMaxAmmoRound() - weapon->GetCurrentWeapon()->GetCurrentAmmoRound())
					/ float(weapon->GetCurrentWeapon()->GetMaxAmmoRound()), 0.f, 1.f)), // Translate to the left based on the amount of ammo to go back to max ammo of the round
					Scale(glm::vec3(float(weapon->GetCurrentWeapon()->GetCurrentAmmoRound())
					/ float(weapon->GetCurrentWeapon()->GetMaxAmmoRound()), 1.f, 1.f)), // Scale the x component based on the current ammo of the round
					});
				forwardSP.Set1i("noNormals", 1);
				forwardSP.Set1i("useCustomColour", 1);
				forwardSP.Set4fv("customColour", glm::vec4(glm::vec3(0.f, 1.f, 0.f), 1.f));
				forwardSP.Set1i("useCustomDiffuseTexIndex", 1);
				forwardSP.Set1i("customDiffuseTexIndex", -1);
				meshes[(int)MeshType::Quad]->SetModel(GetTopModel());
				meshes[(int)MeshType::Quad]->Render(forwardSP);
				forwardSP.Set1i("useCustomDiffuseTexIndex", 0);
				forwardSP.Set1i("useCustomColour", 0);
				forwardSP.Set1i("noNormals", 0);
				PopModel();
				PopModel();
				// Change back the projection
				projection = glm::perspective(glm::radians(angularFOV), cam.GetAspectRatio(), .1f, 9999.f);
				forwardSP.SetMat4fv("PV", &(projection * view)[0][0]);
				break;

			case Entity::EntityType::INVENTORY:
				///Render on the screen, change the projection
				projection = glm::ortho(-float(winWidth) / 2.f, float(winWidth) / 2.f, -float(winHeight) / 2.f, float(winHeight) / 2.f, .1f, 9999.f);
				forwardSP.SetMat4fv("PV", &(projection)[0][0]);

				for (int i = 0; i < 5; ++i)
				{
					PushModel({
						Translate(glm::vec3(entity->pos.x + i * 75.f, entity->pos.y, entity->pos.z)),
						Scale(glm::vec3(entity->scale.x, entity->scale.y, entity->scale.z)),
						});
					forwardSP.Set1i("noNormals", 1);
					forwardSP.Set1i("useCustomColour", 1);
					// The selected inventory slot will be a different colour
					if(weapon->GetCurrentSlot() == i)
						forwardSP.Set4fv("customColour", glm::vec4(glm::vec3(0.f, 1.f, 0.f), 1.f));
					else
						forwardSP.Set4fv("customColour", glm::vec4(glm::vec3(1.f, 0.f, 0.f), 1.f));
					forwardSP.Set1i("useCustomDiffuseTexIndex", 1);
					forwardSP.Set1i("customDiffuseTexIndex", -1);
					meshes[(int)MeshType::Quad]->SetModel(GetTopModel());
					meshes[(int)MeshType::Quad]->Render(forwardSP);
					forwardSP.Set1i("useCustomDiffuseTexIndex", 0);
					forwardSP.Set1i("useCustomColour", 0);
					forwardSP.Set1i("noNormals", 0);
					PopModel();
				}
				// Change back the projection
				projection = glm::perspective(glm::radians(angularFOV), cam.GetAspectRatio(), .1f, 9999.f);
				forwardSP.SetMat4fv("PV", &(projection* view)[0][0]);
				break;

			case Entity::EntityType::BULLET:
				PushModel({
					Translate(glm::vec3(entity->pos.x, entity->pos.y, entity->pos.z)),
					//Rotate(glm::vec4(entity->rotate.x, entity->rotate.y, entity->rotate.z, entity->rotate.w)), // Not sure about the x,y,z etc
					Scale(glm::vec3(entity->scale.x, entity->scale.y, entity->scale.z)),
					});
				// Change the mesh or model accordingly
				meshes[(int)MeshType::Sphere]->SetModel(GetTopModel());
				meshes[(int)MeshType::Sphere]->Render(forwardSP); // Remember to change forwardSP etc accordingly
				PopModel();
				break;

			case Entity::EntityType::WEAPONHOLD:
				const glm::vec3 front = cam.CalcFront();
				const float sign = front.y < 0.f ? -1.f : 1.f;

				auto rotationMat = glm::rotate(glm::mat4(1.f), sign * acosf(glm::dot(front, glm::normalize(glm::vec3(front.x, 0.f, front.z)))),
					glm::normalize(glm::vec3(-front.z, 0.f, front.x)));
				PushModel({
					Translate(cam.GetPos() +
						glm::vec3(rotationMat * glm::vec4(RotateVecIn2D(glm::vec3(5.5f, -7.f, -13.f), atan2(front.x, front.z) + glm::radians(180.f), Axis::y), 1.f))
					),
					Rotate(glm::vec4(glm::vec3(-front.z, 0.f, front.x), sign * glm::degrees(acosf(glm::dot(front, glm::normalize(glm::vec3(front.x, 0.f, front.z))))))),
					Rotate(glm::vec4(0.f, 1.f, 0.f, glm::degrees(atan2(front.x, front.z)))),
					Scale(glm::vec3(5.f)),
					});
				// Render the weapon instead, cube is just for testing
				meshes[(int)MeshType::Cube]->SetModel(GetTopModel());
				meshes[(int)MeshType::Cube]->Render(forwardSP);
				PopModel();
				break;
			}
		}
	}

	///Shapes
	PushModel({
		Translate(glm::vec3(0.f, 100.f, 0.f)),
		Scale(glm::vec3(10.f)),
		});
	forwardSP.Set1i("useCustomColour", 1);
	forwardSP.Set4fv("customColour", glm::vec4(glm::rgbColor(glm::vec3(1.f, PseudorandMinMax(0.f, 255.f), 1.f)) * .5f, .5f));
	meshes[(int)MeshType::Cylinder]->SetModel(GetTopModel());
	meshes[(int)MeshType::Cylinder]->Render(forwardSP);
	forwardSP.Set1i("useCustomColour", 0);
	PushModel({
		Translate(glm::vec3(-3.f, 0.f, 0.f)),
		});
	forwardSP.Set1i("useCustomColour", 1);
	forwardSP.Set4fv("customColour", glm::vec4(glm::rgbColor(glm::vec3(1.f, 1.f, PseudorandMinMax(0.f, 255.f))) * 7.f, .3f));
	forwardSP.Set1i("useCustomDiffuseTexIndex", 1);
	forwardSP.Set1i("customDiffuseTexIndex", -1);
	meshes[(int)MeshType::Sphere]->SetModel(GetTopModel());
	meshes[(int)MeshType::Sphere]->Render(forwardSP);
	forwardSP.Set1i("useCustomDiffuseTexIndex", 0);
	forwardSP.Set1i("useCustomColour", 0);
	PopModel();
	PushModel({
		Translate(glm::vec3(3.f, 0.f, 0.f)),
		});
	forwardSP.Set1i("useCustomColour", 1);
	forwardSP.Set4fv("customColour", glm::vec4(glm::rgbColor(glm::vec3(PseudorandMinMax(0.f, 255.f), 1.f, 1.f)) * .5f, .7f));
	forwardSP.Set1i("useCustomDiffuseTexIndex", 1);
	forwardSP.Set1i("customDiffuseTexIndex", -1);
	meshes[(int)MeshType::Cube]->SetModel(GetTopModel());
	meshes[(int)MeshType::Cube]->Render(forwardSP);
	forwardSP.Set1i("useCustomDiffuseTexIndex", 0);
	forwardSP.Set1i("useCustomColour", 0);
	PopModel();
	PopModel();

	///SpriteAni
	PushModel({
		Translate(glm::vec3(0.f, 50.f, 0.f)),
		Scale(glm::vec3(20.f, 40.f, 20.f)),
		});
	forwardSP.Set1i("noNormals", 1);
	forwardSP.Set1i("useCustomColour", 1);
	forwardSP.Set4fv("customColour", glm::vec4(glm::vec3(1.f), 1.f));
	meshes[(int)MeshType::SpriteAni]->SetModel(GetTopModel());
	meshes[(int)MeshType::SpriteAni]->Render(forwardSP);
	forwardSP.Set1i("useCustomColour", 0);
	forwardSP.Set1i("noNormals", 0);
	PopModel();

	std::string temp;
	switch (weapon->GetCurrentSlot())
	{
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

	if (music && music->getIsPaused()) {
		music->setIsPaused(false);
	}
}

glm::mat4 Scene::Translate(const glm::vec3& translate) {
	return glm::translate(glm::mat4(1.f), translate);
}

glm::mat4 Scene::Rotate(const glm::vec4& rotate) {
	return glm::rotate(glm::mat4(1.f), glm::radians(rotate.w), glm::vec3(rotate));
}

glm::mat4 Scene::Scale(const glm::vec3& scale) {
	return glm::scale(glm::mat4(1.f), scale);
}

glm::mat4 Scene::GetTopModel() const {
	return modelStack.empty() ? glm::mat4(1.f) : modelStack.top();
}

void Scene::PushModel(const std::vector<glm::mat4>& vec) const {
	modelStack.push(modelStack.empty() ? glm::mat4(1.f) : modelStack.top());
	const size_t& size = vec.size();
	for (size_t i = 0; i < size; ++i) {
		modelStack.top() *= vec[i];
	}
}

void Scene::PopModel() const {
	if (!modelStack.empty()) {
		modelStack.pop();
	}
}