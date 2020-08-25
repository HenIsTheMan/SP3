#include "Scene.h"
#include "Vendor/stb_image.h"
#include "../Game/Pistol.h"
#include "../Game/AssaultRifle.h"
#include "../Game/SniperRifle.h"

extern bool endLoop;
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

Scene::Scene():
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
			{"Imgs/BG.png", Mesh::TexType::Diffuse, 0},
			{"Imgs/Scope.png", Mesh::TexType::Diffuse, 0},
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
		new Terrain("Imgs/hMap.raw", 8.f, 8.f),
		new Water(24.f, 2.f, 2.f, .5f),
		new SpriteAni(4, 8),
		new SpriteAni(1, 6),
		new SpriteAni(1, 6),
		new SpriteAni(1, 6),
		new SpriteAni(1, 6),
		new SpriteAni(1, 6),
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
	polyModes{},
	playerCurrHealth(100.f),
	playerMaxHealth(100.f),
	playerCurrLives(5.f),
	playerMaxLives(5.f),
	enemyCount(0),
	score(0),
	scores({}),
	waves{},
	playerStates((int)PlayerState::NoMovement | (int)PlayerState::Standing),
	sprintOn(false),
	reticleColour(glm::vec4(1.f)),
	scope(false),
	screen(Screen::Menu),
	textScaleFactors{
		1.f,
		1.f,
		1.f,
		1.f,
		1.f,
	},
	textColours{
		glm::vec4(1.f),
		glm::vec4(1.f),
		glm::vec4(1.f),
		glm::vec4(1.f),
		glm::vec4(1.f),
	}
{
}

Scene::~Scene(){
	///Create save
	str line;
	try{
		std::ofstream stream("Data/scores.dat", std::ios::out);
		if(stream.is_open()){
			const size_t& mySize = scores.size();
			for(size_t i = 0; i < mySize; ++i){
				stream << (!i ? "" : "\n") + std::to_string(scores[i]);
			}
			stream.close();
		} else{
			throw("Failed to save scores!");
		}
	} catch(cstr const& errorMsg){
		(void)puts(errorMsg);
	}

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
	///Load save
	cstr const& fPath = "Data/scores.dat";
	str line;
	std::ifstream stream(fPath, std::ios::in);
	if(stream.is_open()){
		while(getline(stream, line)){
			try{
				scores.emplace_back(stoi(line));
			} catch(const std::invalid_argument& e){
				(void)puts(e.what());
			}
		}
		stream.close();
	}
	if(scores.size() > 1){
		std::sort(scores.begin(), scores.end(), std::greater<int>());
	}

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

	Entity* const& fire = entityManager->FetchEntity();
	fire->type = Entity::EntityType::FIRE;
	fire->active = true;
	fire->life = 0.f;
	fire->maxLife = 0.f;
	fire->colour = glm::vec4(1.f);
	fire->diffuseTexIndex = -1;
	fire->rotate = glm::vec4(0.f, 1.f, 0.f, 0.f);
	fire->scale = glm::vec3(40.f);
	fire->light = nullptr;
	fire->mesh = meshes[(int)MeshType::Fire];
	fire->pos = glm::vec3(0.f, 200.f, 0.f);
	fire->vel = glm::vec3(0.f);
	fire->mass = .0001f;
	fire->force = glm::vec3(0.f);

	glGetIntegerv(GL_POLYGON_MODE, polyModes);

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

	meshes[(int)MeshType::Fire]->AddTexMap({"Imgs/Fire.png", Mesh::TexType::Diffuse, 0});
	static_cast<SpriteAni*>(meshes[(int)MeshType::Fire])->AddAni("Fire", 0, 32);
	static_cast<SpriteAni*>(meshes[(int)MeshType::Fire])->Play("Fire", -1, .5f);

	meshes[(int)MeshType::CoinGold]->AddTexMap({"Imgs/CoinGold.png", Mesh::TexType::Diffuse, 0});
	static_cast<SpriteAni*>(meshes[(int)MeshType::CoinGold])->AddAni("CoinGold", 0, 6);
	static_cast<SpriteAni*>(meshes[(int)MeshType::CoinGold])->Play("CoinGold", -1, .5f);

	meshes[(int)MeshType::CoinSilver]->AddTexMap({"Imgs/CoinSilver.png", Mesh::TexType::Diffuse, 0});
	static_cast<SpriteAni*>(meshes[(int)MeshType::CoinSilver])->AddAni("CoinSilver", 0, 6);
	static_cast<SpriteAni*>(meshes[(int)MeshType::CoinSilver])->Play("CoinSilver", -1, .5f);

	meshes[(int)MeshType::CoinPink]->AddTexMap({"Imgs/CoinPink.png", Mesh::TexType::Diffuse, 0});
	static_cast<SpriteAni*>(meshes[(int)MeshType::CoinPink])->AddAni("CoinPink", 0, 6);
	static_cast<SpriteAni*>(meshes[(int)MeshType::CoinPink])->Play("CoinPink", -1, .5f);

	meshes[(int)MeshType::CoinGreen]->AddTexMap({"Imgs/CoinGreen.png", Mesh::TexType::Diffuse, 0});
	static_cast<SpriteAni*>(meshes[(int)MeshType::CoinGreen])->AddAni("CoinGreen", 0, 6);
	static_cast<SpriteAni*>(meshes[(int)MeshType::CoinGreen])->Play("CoinGreen", -1, .5f);

	meshes[(int)MeshType::CoinBlue]->AddTexMap({"Imgs/CoinBlue.png", Mesh::TexType::Diffuse, 0});
	static_cast<SpriteAni*>(meshes[(int)MeshType::CoinBlue])->AddAni("CoinBlue", 0, 6);
	static_cast<SpriteAni*>(meshes[(int)MeshType::CoinBlue])->Play("CoinBlue", -1, .5f);

	meshes[(int)MeshType::Terrain]->AddTexMap({"Imgs/GrassGround.jpg", Mesh::TexType::Diffuse, 0});
	meshes[(int)MeshType::Water]->AddTexMap({"Imgs/Water.jpg", Mesh::TexType::Diffuse, 0});
	meshes[(int)MeshType::Water]->AddTexMap({"Imgs/Grey.png", Mesh::TexType::Reflection, 0});

	directionalLights.emplace_back(CreateLight(LightType::Directional));
	spotlights.emplace_back(CreateLight(LightType::Spot));

	return true;
}

void Scene::Update(GLFWwindow* const& win){
	elapsedTime += dt;
	if(winHeight){ //Avoid division by 0 when win is minimised
		cam.SetDefaultAspectRatio(float(winWidth) / float(winHeight));
		cam.ResetAspectRatio();
	}

	POINT mousePos;
	if(GetCursorPos(&mousePos)){
		HWND hwnd = ::GetActiveWindow();
		(void)ScreenToClient(hwnd, &mousePos);
	} else{
		(void)puts("Failed to get mouse pos relative to screen!");
	}
	static float buttonBT = 0.f;

	switch(screen){
		case Screen::End:
		case Screen::Menu: {
			cam.SetPos(glm::vec3(0.f, 0.f, 5.f));
			cam.SetTarget(glm::vec3(0.f));
			cam.SetUp(glm::vec3(0.f, 1.f, 0.f));
			view = cam.LookAt();
			projection = glm::ortho(-float(winWidth) / 2.f, float(winWidth) / 2.f, -float(winHeight) / 2.f, float(winHeight) / 2.f, .1f, 9999.f);

			if(mousePos.x >= 25.f && mousePos.x <= (screen == Screen::Menu ? 100.f : 230.f) && mousePos.y >= winHeight - 260.f && mousePos.y <= winHeight - 225.f){
				if(textScaleFactors[0] != 1.1f){
					soundEngine->play2D("Audio/Sounds/Pop.flac", false);
					textScaleFactors[0] = 1.1f;
					textColours[0] = glm::vec4(1.f, 1.f, 0.f, 1.f);
				}
				if(leftMB - rightMB > 0.f && buttonBT <= elapsedTime){
					soundEngine->play2D("Audio/Sounds/Select.wav", false);
					glfwSetInputMode(win, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

					score = 0;
					playerCurrHealth = playerMaxHealth;
					playerCurrLives = playerMaxLives;
					screen = Screen::Game;

					cam.SetPos(glm::vec3(0.f, 0.f, 50.f));
					cam.SetTarget(glm::vec3(0.f));
					cam.SetUp(glm::vec3(0.f, 1.f, 0.f));

					buttonBT = elapsedTime + .3f;
					break;
				}
			} else{
				textScaleFactors[0] = 1.f;
				textColours[0] = glm::vec4(1.f);
			}
			if(mousePos.x >= 25.f && mousePos.x <= 130.f && mousePos.y >= winHeight - 210.f && mousePos.y <= winHeight - 175.f){
				if(textScaleFactors[1] != 1.1f){
					soundEngine->play2D("Audio/Sounds/Pop.flac", false);
					textScaleFactors[1] = 1.1f;
					textColours[1] = glm::vec4(1.f, 1.f, 0.f, 1.f);
				}
				if(leftMB - rightMB > 0.f && buttonBT <= elapsedTime){
					soundEngine->play2D("Audio/Sounds/Select.wav", false);
					screen = Screen::Score;
					buttonBT = elapsedTime + .3f;
				}
			} else{
				textScaleFactors[1] = 1.f;
				textColours[1] = glm::vec4(1.f);
			}
			if(mousePos.x >= 25.f && mousePos.x <= 230.f && mousePos.y >= winHeight - 160.f && mousePos.y <= winHeight - 125.f){
				if(textScaleFactors[2] != 1.1f){
					soundEngine->play2D("Audio/Sounds/Pop.flac", false);
					textScaleFactors[2] = 1.1f;
					textColours[2] = glm::vec4(1.f, 1.f, 0.f, 1.f);
				}
				if(leftMB - rightMB > 0.f && buttonBT <= elapsedTime){
					soundEngine->play2D("Audio/Sounds/Select.wav", false);
					screen = Screen::Instructions;
					buttonBT = elapsedTime + .3f;
				}
			} else{
				textScaleFactors[2] = 1.f;
				textColours[2] = glm::vec4(1.f);
			}
			if(mousePos.x >= 25.f && mousePos.x <= 150.f && mousePos.y >= winHeight - 110.f && mousePos.y <= winHeight - 75.f){
				if(textScaleFactors[3] != 1.1f){
					soundEngine->play2D("Audio/Sounds/Pop.flac", false);
					textScaleFactors[3] = 1.1f;
					textColours[3] = glm::vec4(1.f, 1.f, 0.f, 1.f);
				}
				if(leftMB - rightMB > 0.f && buttonBT <= elapsedTime){
					soundEngine->play2D("Audio/Sounds/Select.wav", false);
					screen = Screen::Credits;
					buttonBT = elapsedTime + .3f;
				}
			} else{
				textScaleFactors[3] = 1.f;
				textColours[3] = glm::vec4(1.f);
			}
			if(mousePos.x >= 25.f && mousePos.x <= 100.f && mousePos.y >= winHeight - 60.f && mousePos.y <= winHeight - 25.f){
				if(textScaleFactors[4] != 1.1f){
					soundEngine->play2D("Audio/Sounds/Pop.flac", false);
					textScaleFactors[4] = 1.1f;
					textColours[4] = glm::vec4(1.f, 1.f, 0.f, 1.f);
				}
				if(leftMB - rightMB > 0.f && buttonBT <= elapsedTime){
					soundEngine->play2D("Audio/Sounds/Select.wav", false);
					endLoop = true;
					buttonBT = elapsedTime + .3f;
				}
			} else{
				textScaleFactors[4] = 1.f;
				textColours[4] = glm::vec4(1.f);
			}

			break;
		}
		case Screen::Game: {
			if(Key(GLFW_KEY_P) && buttonBT <= elapsedTime){
				screen = Screen::Pause;
				glfwSetInputMode(win, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
				buttonBT = elapsedTime + .4f;
				break;
			}

			reticleColour = glm::vec4(1.f);
			if(score < 0){
				score = 0;
			}
			if(Key(GLFW_KEY_9)){
				screen = Screen::End;
				const size_t& mySize = scores.size();
				if(mySize == 5){ //Max no. of scores saved
					std::sort(scores.begin(), scores.end(), std::greater<int>());
					if(score > scores.back()){
						scores.pop_back();
						scores.emplace_back(score);
					}
					std::sort(scores.begin(), scores.end(), std::greater<int>());
				} else{
					scores.emplace_back(score);
					std::sort(scores.begin(), scores.end(), std::greater<int>());
				}
			}

			static_cast<SpriteAni*>(meshes[(int)MeshType::Fire])->Update();
			static_cast<SpriteAni*>(meshes[(int)MeshType::CoinGold])->Update();
			static_cast<SpriteAni*>(meshes[(int)MeshType::CoinSilver])->Update();
			static_cast<SpriteAni*>(meshes[(int)MeshType::CoinPink])->Update();
			static_cast<SpriteAni*>(meshes[(int)MeshType::CoinGreen])->Update();
			static_cast<SpriteAni*>(meshes[(int)MeshType::CoinBlue])->Update();

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
						scope = true;
						break;
					default:
						angularFOV = 45.f;
				}
			} else{
				scope = false;
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

			static float polyModeBT = 0.f;
			static float distortionBT = 0.f;
			static float echoBT = 0.f;
			static float wavesReverbBT = 0.f;
			static float resetSoundFXBT = 0.f;

			if(Key(VK_F2) && polyModeBT <= elapsedTime){
				polyModes[0] += polyModes[0] == GL_FILL ? -2 : 1;
				glPolygonMode(GL_FRONT_AND_BACK, polyModes[0]);
				polyModeBT = elapsedTime + .5f;
			}

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
			if(playerCurrHealth <= 0.f && playerCurrLives > 0.f)
			{
				playerCurrHealth = 100.f;
				--playerCurrLives;
			}

			// Change weapon using the inventory slots
			if(Key(GLFW_KEY_1))
				weapon->SetCurrentSlot(0);
			if(Key(GLFW_KEY_2))
				weapon->SetCurrentSlot(1);
			if(Key(GLFW_KEY_3))
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

			// Rain particles
			static float tmp = elapsedTime;
			static int count = 0; // To limit the number of rain particles
			if (elapsedTime - tmp >= 3.f && count < 100)
			{
				Entity* const& particle = entityManager->FetchEntity();
				particle->type = Entity::EntityType::PARTICLE;
				particle->active = true;
				particle->life = 0.f;
				particle->maxLife = 0.f;
				particle->colour = glm::vec4(0.f, 1.f, 1.f, 1.f);
				particle->diffuseTexIndex = -1;
				particle->rotate = glm::vec4(0.f, 1.f, 0.f, 0.f);
				particle->scale = glm::vec3(3.f);
				particle->light = nullptr;
				particle->mesh = meshes[(int)MeshType::Quad];
				//particle->pos = glm::vec3(150.f, 100.f, 50.f);
				particle->pos = glm::vec3(PseudorandMinMax(-150.f, 150.f), 350.f, PseudorandMinMax(-150.f, 0.f));
				particle->vel = glm::vec3(0.f, -20.f, 0.f);
				particle->mass = 10.f;
				particle->force = glm::vec3(0.f);
				tmp = elapsedTime;
				count++;
			}
			static float tmp2 = elapsedTime;
			static int count2 = 0; // To limit the number of fire particles
			if (elapsedTime - tmp2 >= 0.5f && count2 < 10)
			{
				// Fire particles
				Entity* const& particle2 = entityManager->FetchEntity();
				particle2->type = Entity::EntityType::PARTICLE2;
				particle2->active = true;
				particle2->life = 0.f;
				particle2->maxLife = 0.f;
				particle2->colour = glm::vec4(glm::vec3(.4f), 1.f);
				particle2->diffuseTexIndex = -1;
				particle2->rotate = glm::vec4(0.f, 1.f, 0.f, 0.f);
				particle2->scale = glm::vec3(3.f);
				particle2->light = nullptr;
				particle2->mesh = meshes[(int)MeshType::Quad];
				// Position depends on where the fire is
				particle2->pos = glm::vec3(0.f, 300.f, 0.f);
				// Will change accordingly to make it look better
				particle2->vel = glm::vec3(0.f, 20.f, 0.f);
				particle2->mass = .0001f;
				particle2->force = glm::vec3(0.f);
				tmp2 = elapsedTime;
				count2++;
			}
			// TESTING ONLY -> PARTICLES SPAWN WHEN ENEMY DIE
			// Need a check statement here to see which enemy died
			if (Key(GLFW_KEY_0))
			{
				for (int i = 0; i < 4; ++i)
				{
					Entity* const& particle = entityManager->FetchEntity();
					particle->type = Entity::EntityType::PARTICLE3;
					particle->active = true;
					particle->life = 0.f;
					particle->maxLife = 0.f;
					particle->colour = glm::vec4(0.f, 1.f, 1.f, 1.f);
					particle->diffuseTexIndex = -1;
					particle->rotate = glm::vec4(0.f, 1.f, 0.f, 0.f);
					particle->scale = glm::vec3(2.f);
					particle->light = nullptr;
					particle->mesh = meshes[(int)MeshType::Quad];
					// Position depends on where the enemy is
					particle->pos = glm::vec3(150.f, 0.f, 50.f);
					// Will change accordingly to make it look better
					particle->vel = glm::vec3(PseudorandMinMax(-100.f, 100.f),
						PseudorandMinMax(1.f, 100.f), PseudorandMinMax(-100.f, 100.f));
					particle->mass = 10.f;
					particle->force = glm::vec3(0.f, -100.f, 0.f);
				}
			}

			EntityManager::UpdateParams params;
			params.camPos = cam.GetPos();
			params.camFront = cam.CalcFront();
			params.reticleColour = reticleColour;
			entityManager->UpdateEntities(params);
			reticleColour = params.reticleColour;

			for(int i = 0; i < (int)WaveNumber::Total; ++i){
				if(enemyCount == 0){
					switch(waves[i]){
						case (int)WaveNumber::One:
							for(int i = 0; i < 10; ++i){
								Entity* const& stillEnemy = entityManager->FetchEntity();
								stillEnemy->type = Entity::EntityType::STATIC_ENEMY;
								stillEnemy->active = true;
								stillEnemy->life = 20.f;
								stillEnemy->maxLife = 20.f;
								stillEnemy->colour = glm::vec4(1.f);
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
							return;
					}
				}
			}

			break;
		}
		case Screen::Pause: {
			if(Key(GLFW_KEY_P) && buttonBT <= elapsedTime){
				screen = Screen::Game;
				glfwSetInputMode(win, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

				cam.SetPos(glm::vec3(0.f, 0.f, 50.f));
				cam.SetTarget(glm::vec3(0.f));
				cam.SetUp(glm::vec3(0.f, 1.f, 0.f));

				buttonBT = elapsedTime + .4f;
				break;
			}
		}
		case Screen::Instructions:
		case Screen::Credits:
		case Screen::Score: {
			glfwSetInputMode(win, GLFW_CURSOR, GLFW_CURSOR_NORMAL);

			cam.SetPos(glm::vec3(0.f, 0.f, 5.f));
			cam.SetTarget(glm::vec3(0.f));
			cam.SetUp(glm::vec3(0.f, 1.f, 0.f));
			view = cam.LookAt();
			projection = glm::ortho(-float(winWidth) / 2.f, float(winWidth) / 2.f, -float(winHeight) / 2.f, float(winHeight) / 2.f, .1f, 9999.f);

			if(mousePos.x >= 25.f && mousePos.x <= 110.f && mousePos.y >= winHeight - 60.f && mousePos.y <= winHeight - 25.f){
				if(textScaleFactors[2] != 1.1f){
					soundEngine->play2D("Audio/Sounds/Pop.flac", false);
					textScaleFactors[2] = 1.1f;
					textColours[2] = glm::vec4(1.f, 1.f, 0.f, 1.f);
				}
				if(leftMB - rightMB > 0.f && buttonBT <= elapsedTime){
					soundEngine->play2D("Audio/Sounds/Select.wav", false);
					screen = Screen::Menu;
					buttonBT = elapsedTime + .3f;
				}
			} else{
				textScaleFactors[2] = 1.f;
				textColours[2] = glm::vec4(1.f);
			}

			break;
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
	if(!glm::length(translate) || (screen == Screen::Game && !scope)){
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
	forwardSP.Set1i("pAmt", screen == Screen::Game ? pAmt : 0);
	forwardSP.Set1i("dAmt", screen == Screen::Game ? dAmt : 0);
	forwardSP.Set1i("sAmt", screen == Screen::Game ? sAmt : 0);

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

	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	switch(screen){
		case Screen::End:
		case Screen::Menu: {
			forwardSP.SetMat4fv("PV", &(projection * view)[0][0]);

			///BG
			modelStack.PushModel({
				modelStack.Scale(glm::vec3(float(winWidth) / 2.f, float(winHeight) / 2.f, 1.f)),
			});
				forwardSP.Set1i("useCustomDiffuseTexIndex", 1);
				forwardSP.Set1i("customDiffuseTexIndex", 5);
				forwardSP.Set1i("noNormals", 1);
				meshes[(int)MeshType::Quad]->SetModel(modelStack.GetTopModel());
				meshes[(int)MeshType::Quad]->Render(forwardSP);
				forwardSP.Set1i("noNormals", 0);
				forwardSP.Set1i("useCustomDiffuseTexIndex", 0);
			modelStack.PopModel();
			
			glDepthFunc(GL_NOTEQUAL);
			textChief.RenderText(textSP, {
				screen == Screen::Menu ? "Play" : "Play Again",
				25.f,
				225.f,
				textScaleFactors[0],
				textColours[0],
				0,
			});
			textChief.RenderText(textSP, {
				"Scores",
				25.f,
				175.f,
				textScaleFactors[1],
				textColours[1],
				0,
			});
			textChief.RenderText(textSP, {
				"Instructions",
				25.f,
				125.f,
				textScaleFactors[2],
				textColours[2],
				0,
			});
			textChief.RenderText(textSP, {
				"Credits",
				25.f,
				75.f,
				textScaleFactors[3],
				textColours[3],
				0,
			});
			textChief.RenderText(textSP, {
				"Exit",
				25.f,
				25.f,
				textScaleFactors[4],
				textColours[4],
				0,
			});
			glDepthFunc(GL_LESS);

			break;
		}
		case Screen::Game: {
			///Sky
			forwardSP.SetMat4fv("PV", &(projection * glm::mat4(glm::mat3(view)))[0][0]);
			glDepthFunc(GL_LEQUAL); //Modify comparison operators used for depth test such that frags with depth <= 1.f are shown
			glCullFace(GL_FRONT);
			modelStack.PushModel({
				modelStack.Rotate(glm::vec4(0.f, 1.f, 0.f, glfwGetTime())),
			});
				forwardSP.Set1i("sky", 1);
				meshes[(int)MeshType::Sphere]->SetModel(modelStack.GetTopModel());
				meshes[(int)MeshType::Sphere]->Render(forwardSP);
				forwardSP.Set1i("sky", 0);
			modelStack.PopModel();
			glCullFace(GL_BACK);
			glDepthFunc(GL_LESS);

			forwardSP.SetMat4fv("PV", &(projection * view)[0][0]);

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

			EntityManager::RenderParams params;
			params.camPos = cam.GetPos();
			params.depthDTexRefID = depthDTexRefID;
			params.depthSTexRefID = depthSTexRefID;
			params.quadMesh = meshes[(int)MeshType::Quad];
			entityManager->RenderEntities(forwardSP, params); //Render entities

			if(scope){ //If scoped in...
				forwardSP.SetMat4fv("PV", &(glm::ortho(-float(winWidth) / 2.f, float(winWidth) / 2.f, -float(winHeight) / 2.f, float(winHeight) / 2.f, .1f, 9999.f))[0][0]);
				forwardSP.Set1i("useCustomDiffuseTexIndex", 1);
				forwardSP.Set1i("noNormals", 1);

				modelStack.PushModel({
					modelStack.Translate(glm::vec3(0.f, 0.f, -9.f)),
					modelStack.Scale(glm::vec3(float(winHeight) / 2.f, float(winHeight) / 2.f, 1.f)),
				});
					forwardSP.Set1i("customDiffuseTexIndex", 6);
					meshes[(int)MeshType::Quad]->SetModel(modelStack.GetTopModel());
					meshes[(int)MeshType::Quad]->Render(forwardSP);
				modelStack.PopModel();
				modelStack.PushModel({
					modelStack.Translate(glm::vec3(0.f, 0.f, -9.1f)),
					modelStack.Scale(glm::vec3(float(winWidth) / 2.f, float(winHeight) / 2.f, 1.f)),
				});
					forwardSP.Set1i("customDiffuseTexIndex", -1);
					forwardSP.Set1i("useCustomColour", 1);
					forwardSP.Set4fv("customColour", glm::vec4(glm::vec3(0.f), 1.f));
					meshes[(int)MeshType::Quad]->SetModel(modelStack.GetTopModel());
					meshes[(int)MeshType::Quad]->Render(forwardSP);
					forwardSP.Set1i("useCustomColour", 0);
				modelStack.PopModel();

				forwardSP.Set1i("noNormals", 0);
				forwardSP.Set1i("useCustomDiffuseTexIndex", 0);
				glBlendFunc(GL_ONE, GL_ZERO);
				return;
			}

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
				forwardSP.Set4fv("customColour", glm::vec4(glm::vec3(.3f), 1.f));
				forwardSP.Set1i("customDiffuseTexIndex", -1);
				meshes[(int)MeshType::Quad]->SetModel(modelStack.GetTopModel());
				meshes[(int)MeshType::Quad]->Render(forwardSP);

				modelStack.PushModel({
					modelStack.Translate(glm::vec3((playerCurrHealth - playerMaxHealth) / playerMaxHealth, 0.f, 1.f)), // Translate to the left based on the amount of health to go back to max health
					modelStack.Scale(glm::vec3(playerCurrHealth / playerMaxHealth, 1.f, 1.f)), // Scale the x component based on the current health
				});
					forwardSP.Set4fv("customColour", glm::vec4(0.f, 1.f, 0.f, 1.f));
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
				forwardSP.Set4fv("customColour", glm::vec4(glm::vec3(.3f), 1.f));
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
					forwardSP.Set4fv("customColour", glm::vec4(glm::vec3(1.f, 0.f, 1.f), 1.f));
					forwardSP.Set1i("customDiffuseTexIndex", -1);
					meshes[(int)MeshType::Quad]->SetModel(modelStack.GetTopModel());
					meshes[(int)MeshType::Quad]->Render(forwardSP);
				modelStack.PopModel();
			modelStack.PopModel();

			///Render inv slots and guns in inv
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
					meshes[(int)MeshType::Quad]->Render(forwardSP);	///Render guns in inv

					forwardSP.Set1i("useCustomDiffuseTexIndex", 0);
					forwardSP.Set1i("useCustomColour", 0);
					switch(j){
						case 0:
							modelStack.PushModel({
								modelStack.Translate(glm::vec3(-.2f, -.6f, 0.f)),
								modelStack.Rotate(glm::vec4(0.f, 0.f, 1.f, 45.f)),
								modelStack.Rotate(glm::vec4(0.f, 1.f, 0.f, 90.f)),
								modelStack.Scale(glm::vec3(.6f)),
							});
								models[(int)ModelType::Pistol]->SetModelForAll(modelStack.GetTopModel());
								models[(int)ModelType::Pistol]->Render(forwardSP);
							modelStack.PopModel();
							break;
						case 1:
							modelStack.PushModel({
								modelStack.Translate(glm::vec3(0.f, -.1f, 0.f)),
								modelStack.Rotate(glm::vec4(0.f, 0.f, 1.f, 45.f)),
								modelStack.Rotate(glm::vec4(0.f, 1.f, 0.f, 90.f)),
								modelStack.Scale(glm::vec3(.3f)),
							});
								models[(int)ModelType::AR]->SetModelForAll(modelStack.GetTopModel());
								models[(int)ModelType::AR]->Render(forwardSP);
							modelStack.PopModel();
							break;
						case 2:
							modelStack.PushModel({
								modelStack.Translate(glm::vec3(0.f, -.4f, 0.f)),
								modelStack.Rotate(glm::vec4(0.f, 0.f, 1.f, 45.f)),
								modelStack.Rotate(glm::vec4(0.f, 1.f, 0.f, 90.f)),
								modelStack.Scale(glm::vec3(.3f)),
							});
								models[(int)ModelType::Sniper]->SetModelForAll(modelStack.GetTopModel());
								models[(int)ModelType::Sniper]->Render(forwardSP);
							modelStack.PopModel();
							break;
					}
					forwardSP.Set1i("useCustomDiffuseTexIndex", 1);
					forwardSP.Set1i("useCustomColour", 1);
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

			textChief.RenderText(textSP, { //Weapon type
				temp,
				float(winWidth) / 1.3f,
				75.f,
				1.f,
				glm::vec4(1.f),
				0
			});
			textChief.RenderText(textSP, { //Weapon ammo
				std::to_string(weapon->GetCurrentWeapon()->GetCurrentAmmoRound()) + "/" + std::to_string(weapon->GetCurrentWeapon()->GetCurrentTotalAmmo()),
				float(winWidth) / 1.1f,
				25.f,
				1.f,
				glm::vec4(1.f),
				0
			});
			textChief.RenderText(textSP, {
				"Score: " + std::to_string(score),
				25.f,
				75.f,
				1.f,
				glm::vec4(1.f, 1.f, 0.f, 1.f),
				0
			});
			textChief.RenderText(textSP, {
				"FPS: " + std::to_string(1.f / dt).substr(0, 2),
				25.f,
				25.f,
				1.f,
				glm::vec4(1.f, 1.f, 0.f, 1.f),
				0
			});
			break;
		}
		case Screen::Instructions: {
			forwardSP.SetMat4fv("PV", &(projection * view)[0][0]);

			///BG
			modelStack.PushModel({
				modelStack.Scale(glm::vec3(float(winWidth) / 2.f, float(winHeight) / 2.f, 1.f)),
			});
				forwardSP.Set1i("useCustomDiffuseTexIndex", 1);
				forwardSP.Set1i("customDiffuseTexIndex", 5);
				forwardSP.Set1i("noNormals", 1);
				meshes[(int)MeshType::Quad]->SetModel(modelStack.GetTopModel());
				meshes[(int)MeshType::Quad]->Render(forwardSP);
				forwardSP.Set1i("noNormals", 0);
				forwardSP.Set1i("useCustomDiffuseTexIndex", 0);
			modelStack.PopModel();

			glDepthFunc(GL_GREATER);
			textChief.RenderText(textSP, {
				"Back",
				25.f,
				25.f,
				textScaleFactors[2],
				textColours[2],
				0,
			});

			textChief.RenderText(textSP, {
				"Instructions",
				30.f,
				float(winHeight) / 1.2f,
				1.f,
				glm::vec4(1.f, .5f, 0.f, 1.f),
				0,
			});
			glDepthFunc(GL_LESS);
			break;
		}
		case Screen::Credits: {
			forwardSP.SetMat4fv("PV", &(projection * view)[0][0]);

			///BG
			modelStack.PushModel({
				modelStack.Scale(glm::vec3(float(winWidth) / 2.f, float(winHeight) / 2.f, 1.f)),
			});
				forwardSP.Set1i("useCustomDiffuseTexIndex", 1);
				forwardSP.Set1i("customDiffuseTexIndex", 5);
				forwardSP.Set1i("noNormals", 1);
				meshes[(int)MeshType::Quad]->SetModel(modelStack.GetTopModel());
				meshes[(int)MeshType::Quad]->Render(forwardSP);
				forwardSP.Set1i("noNormals", 0);
				forwardSP.Set1i("useCustomDiffuseTexIndex", 0);
			modelStack.PopModel();

			glDepthFunc(GL_GREATER);
			textChief.RenderText(textSP, {
				"Back",
				25.f,
				25.f,
				textScaleFactors[2],
				textColours[2],
				0,
			});

			textChief.RenderText(textSP, {
				"Credits",
				30.f,
				float(winHeight) / 1.2f,
				1.f,
				glm::vec4(1.f, .5f, 0.f, 1.f),
				0,
			});
			glDepthFunc(GL_LESS);
			break;
		}
		case Screen::Score: {
			forwardSP.SetMat4fv("PV", &(projection * view)[0][0]);

			///BG
			modelStack.PushModel({
				modelStack.Scale(glm::vec3(float(winWidth) / 2.f, float(winHeight) / 2.f, 1.f)),
			});
				forwardSP.Set1i("useCustomDiffuseTexIndex", 1);
				forwardSP.Set1i("customDiffuseTexIndex", 5);
				forwardSP.Set1i("noNormals", 1);
				meshes[(int)MeshType::Quad]->SetModel(modelStack.GetTopModel());
				meshes[(int)MeshType::Quad]->Render(forwardSP);
				forwardSP.Set1i("noNormals", 0);
				forwardSP.Set1i("useCustomDiffuseTexIndex", 0);
			modelStack.PopModel();

			glDepthFunc(GL_GREATER);
			textChief.RenderText(textSP, {
				"Back",
				25.f,
				25.f,
				textScaleFactors[2],
				textColours[2],
				0,
			});

			float currOffset = 0.f;
			textChief.RenderText(textSP, {
				"Scores",
				30.f,
				float(winHeight) / 1.2f,
				1.f,
				glm::vec4(1.f, .5f, 0.f, 1.f),
				0,
			});
			const size_t& mySize = scores.size();
			for(size_t i = 0; i < mySize; ++i){
				currOffset += 80.f;
				textChief.RenderText(textSP, {
					std::to_string(scores[i]),
					30.f,
					float(winHeight) / 1.2f - currOffset,
					1.f,
					glm::vec4(1.f, .5f, 0.f, 1.f),
					0,
				});
			}
			glDepthFunc(GL_LESS);
			break;
		}
	}
	glBlendFunc(GL_ONE, GL_ZERO);

	if(music && music->getIsPaused()){
		music->setIsPaused(false);
	}
}

void Scene::MinimapRender(){
	forwardSP.Use();
	forwardSP.Set1f("shininess", 32.f); //More light scattering if lower
	forwardSP.Set3fv("globalAmbient", Light::globalAmbient);
	forwardSP.Set3fv("camPos", cam.GetPos());
	forwardSP.Set1i("pAmt", 0);
	forwardSP.Set1i("dAmt", 0);
	forwardSP.Set1i("sAmt", 0);

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

const Scene::Screen& Scene::GetScreen() const{
	return screen;
}