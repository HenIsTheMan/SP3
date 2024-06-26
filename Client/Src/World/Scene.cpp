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

constexpr float terrainXScale = 1000.f;
constexpr float terrainYScale = 100.f;
constexpr float terrainZScale = 1000.f;

glm::vec3 Light::globalAmbient = glm::vec3(.2f);

Scene::Scene():
	cam(glm::vec3(0.f, 0.f, 5.f), glm::vec3(0.f), glm::vec3(0.f, 1.f, 0.f), 0.f, 150.f),
	dCam(glm::vec3(0.f, 110.f, 0.f), glm::vec3(0.f), glm::vec3(0.f, 0.f, 1.f), 0.f, 0.f),
	sCam(glm::vec3(0.f, 200.f, 0.f), glm::vec3(0.f, 0.f, 300.f), glm::normalize(glm::vec3(0.f, 1.f, -1.f)), 1.f, 0.f),
	waterCam(glm::vec3(-15.f, -20.f, -20.f), glm::vec3(-15.f, 0.f, -20.f), glm::vec3(0.f, 0.f, 1.f), 1.f, 0.f),
	enCam(glm::vec3(0.f), glm::vec3(0.f), glm::vec3(0.f), 1.f, 0.f),
	minimapCam(glm::vec3(0.f), glm::vec3(0.f), glm::vec3(0.f), 0.f, 0.f),
	soundEngine(nullptr),
	music({}),
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
			{"Imgs/Dmg.png", Mesh::TexType::Diffuse, 0},
			{"Imgs/HealthUp.png", Mesh::TexType::Diffuse, 0},
			{"Imgs/LifeUp.png", Mesh::TexType::Diffuse, 0},
			{"Imgs/Immunity.png", Mesh::TexType::Diffuse, 0},
		}),
		new Mesh(Mesh::MeshType::Cube, GL_TRIANGLES, {
			{"Imgs/BoxAlbedo.png", Mesh::TexType::Diffuse, 0},
			{"Imgs/Grey.png", Mesh::TexType::Reflection, 0},
		}),
		new Mesh(Mesh::MeshType::Sphere, GL_TRIANGLE_STRIP, {
			{"Imgs/Skydome.hdr", Mesh::TexType::Diffuse, 0},
			{"Imgs/Grey.png", Mesh::TexType::Reflection, 0},
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
		new Model("ObjsAndMtls/Grass.obj", {
			aiTextureType_DIFFUSE,
		}),
	},
	blurSP{"Shaders/Quad.vs", "Shaders/Blur.fs"},
	depthSP{"Shaders/Depth.vs", "Shaders/Depth.fs"},
	forwardSP(nullptr),
	geoPassSP(nullptr),
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
	currentEnemyCount(0),
	score(0),
	scores({}),
	waves{},
	waveCount(0),
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
	},
	addAmmo(false),
	weaponType(0),
	goldCoinAmt(0),
	silverCoinAmt(0),
	pinkCoinAmt(0),
	greenCoinAmt(0),
	blueCoinAmt(0),
	ammoPickupAmt(0),
	ammoPickup2Amt(0),
	takingDmg(false),
	healthUp(0.f),
	lifeUp(0.f),
	immune(false)
{
}

Scene::~Scene(){
	if(forwardSP != nullptr){
		delete forwardSP;
		forwardSP = nullptr;
	}

	if(geoPassSP != nullptr){
		delete geoPassSP;
		geoPassSP = nullptr;
	}

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
	for(size_t i = 0; i < pSize; ++i){
		if(ptLights[i]){
			delete ptLights[i];
			ptLights[i] = nullptr;
		}
	}
	for(size_t i = 0; i < dSize; ++i){
		if(directionalLights[i]){
			delete directionalLights[i];
			directionalLights[i] = nullptr;
		}
	}
	for(size_t i = 0; i < sSize; ++i){
		if(spotlights[i]){
			delete spotlights[i];
			spotlights[i] = nullptr;
		}
	}

	for(int i = 0; i < (int)MeshType::Amt; ++i){
		if(meshes[i]){
			delete meshes[i];
			meshes[i] = nullptr;
		}
	}
	for(int i = 0; i < (int)ModelType::Amt; ++i){
		if(models[i]){
			delete models[i];
			models[i] = nullptr;
		}
	}

	const size_t& musicSize = music.size();
	for(size_t i = 0; i < musicSize; ++i){
		if(music[i]){
			music[i]->drop();
		}
	}
	if(soundEngine){
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
	int maxTexImgUnits;
	glGetIntegerv(GL_MAX_TEXTURE_IMAGE_UNITS, &maxTexImgUnits);

	if(maxTexImgUnits >= 32){
		forwardSP = new ShaderProg("Shaders/Forward.vs", "Shaders/Forward25.fs");
		geoPassSP = new ShaderProg("Shaders/GeoPass.vs", "Shaders/GeoPass28.fs");
	} else{
		forwardSP = new ShaderProg("Shaders/Forward.vs", "Shaders/Forward9.fs");
		geoPassSP = new ShaderProg("Shaders/GeoPass.vs", "Shaders/GeoPass12.fs");
	}

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

	soundEngine = createIrrKlangDevice(ESOD_AUTO_DETECT, ESEO_DEFAULT_OPTIONS & ~ESEO_PRINT_DEBUG_INFO_TO_DEBUGGER & ~ESEO_PRINT_DEBUG_INFO_TO_STDOUT);
	if(!soundEngine){
		(void)puts("Failed to init soundEngine!\n");
	}
	soundEngine->play2D("Audio/Music/BGM.mp3", true);

	///Gen model matrices for grass instances
	for(int i = 0; i < 9999; ++i){
		const float scaleFactor = 2.f;
		const float xPos = PseudorandMinMax(-terrainXScale / 2.f + scaleFactor, terrainXScale / 2.f - scaleFactor);
		const float zPos = PseudorandMinMax(-terrainZScale / 2.f + scaleFactor, terrainZScale / 2.f - scaleFactor);
		const glm::vec3 pos = glm::vec3(xPos, terrainYScale * static_cast<Terrain*>(meshes[(int)MeshType::Terrain])->GetHeightAtPt(xPos / terrainXScale, zPos / terrainZScale, true), zPos);
		modelStack.PushModel({
			modelStack.Translate(pos),
			modelStack.Rotate(glm::vec4(0.f, 1.f, 0.f, PseudorandMinMax(0.f, 360.f))),
			modelStack.Scale(glm::vec3(scaleFactor)),
		});
			models[(int)ModelType::Grass]->AddModelMatForAll(modelStack.GetTopModel());
		modelStack.PopModel();
	}

	entityManager = EntityManager::GetObjPtr();
	entityManager->CreateEntities(1000);

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

	glGetIntegerv(GL_POLYGON_MODE, polyModes);
	for(short i = 0; i < 20; ++i){
		ptLights.emplace_back(CreateLight(LightType::Pt));
	}
	directionalLights.emplace_back(CreateLight(LightType::Directional));
	spotlights.emplace_back(CreateLight(LightType::Spot));

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

	return true;
}

void Scene::Update(GLFWwindow* const& win){
	elapsedTime += dt;
	if(winHeight){ //Avoid division by 0 when win is minimised
		cam.SetDefaultAspectRatio(float(winWidth) / float(winHeight));
		cam.ResetAspectRatio();
	}

	static float polyModeBT = 0.f;
	if(Key(VK_F2) && polyModeBT <= elapsedTime){
		polyModes[0] += polyModes[0] == GL_FILL ? -2 : 1;
		glPolygonMode(GL_FRONT_AND_BACK, polyModes[0]);
		polyModeBT = elapsedTime + .5f;
	}

	POINT mousePos;
	if(GetCursorPos(&mousePos)){
		HWND hwnd = ::GetActiveWindow();
		(void)ScreenToClient(hwnd, &mousePos);
	} else{
		(void)puts("Failed to get mouse pos relative to screen!");
	}
	static float buttonBT = 0.f;

	static float goldCoinBT = 0.f;
	static float silverCoinBT = 0.f;
	static float pinkCoinBT = 0.f;
	static float greenCoinBT = 0.f;
	static float blueCoinBT = 0.f;
	static float ammoPickupBT = 0.f;
	static float ammoPickup2BT = 0.f;

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
					glfwSetInputMode(win, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
					soundEngine->play2D("Audio/Sounds/Select.wav", false);

					score = 0;
					playerCurrHealth = playerMaxHealth;
					playerCurrLives = playerMaxLives;

					cam.SetPos(glm::vec3(0.f, 0.f, 50.f));
					cam.SetTarget(glm::vec3(0.f));
					cam.SetUp(glm::vec3(0.f, 1.f, 0.f));

					for(int i = 1; i < 3; ++i){
						weapon->SetCurrentSlot(i);
						weapon->GetCurrentWeapon()->ResetWeapon(); // Restock all the ammo for all weapons
					}
					weapon->SetCurrentSlot(0); // Start with pistol again

					goldCoinBT = 0.f;
					silverCoinBT = 0.f;
					pinkCoinBT = 0.f;
					greenCoinBT = 0.f;
					blueCoinBT = 0.f;
					ammoPickupBT = 0.f;
					ammoPickup2BT = 0.f;

					goldCoinAmt = 0;
					silverCoinAmt = 0;
					pinkCoinAmt = 0;
					greenCoinAmt = 0;
					blueCoinAmt = 0;
					ammoPickupAmt = 0;
					ammoPickup2Amt = 0;

					////Create entities
					entityManager->DeactivateAll();

					///Create fires
					for(short i = 0; i < 20; ++i){
						const float scaleFactor = 15.f;
						const float xPos = PseudorandMinMax(-terrainXScale / 2.f + 5.f, terrainXScale / 2.f - 5.f);
						const float zPos = PseudorandMinMax(-terrainZScale / 2.f + 5.f, terrainZScale / 2.f - 5.f);
						const glm::vec3 pos = glm::vec3(xPos, terrainYScale * static_cast<Terrain*>(meshes[(int)MeshType::Terrain])->GetHeightAtPt(xPos / terrainXScale, zPos / terrainZScale) + scaleFactor, zPos);

						Entity* const& fire = entityManager->FetchEntity();
						fire->type = Entity::EntityType::FIRE;
						fire->active = true;
						fire->life = 0.f;
						fire->maxLife = 0.f;
						fire->colour = glm::vec4(1.f);
						fire->diffuseTexIndex = -1;
						fire->rotate = glm::vec4(0.f, 1.f, 0.f, 0.f);
						fire->scale = glm::vec3(scaleFactor);
						fire->mesh = meshes[(int)MeshType::Fire];
						fire->pos = pos;
						fire->vel = glm::vec3(0.f);
						fire->mass = .0001f;
						fire->force = glm::vec3(0.f);

						fire->light = ptLights[i];
						static_cast<PtLight*>(fire->light)->pos = fire->pos;
						static_cast<PtLight*>(fire->light)->pos.y -= 10.f;
						fire->light->diffuse *= 30.f;

						ISound* myMusic = soundEngine->play3D("Audio/Music/Burn.wav", vec3df(pos.x, pos.y, pos.z), true, true, true, ESM_AUTO_DETECT, true);
						if(myMusic){
							myMusic->setMinDistance(2.f);
							myMusic->setVolume(5);
							music.emplace_back(myMusic);
						} else{
							(void)puts("Failed to init music!\n");
						}
					}

					screen = Screen::Game;
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
				glfwSetInputMode(win, GLFW_CURSOR, GLFW_CURSOR_NORMAL);

				///Pause music
				const size_t& musicSize = music.size();
				for(size_t i = 0; i < musicSize; ++i){
					ISound* const& myMusic = music[i];
					if(myMusic && !myMusic->getIsPaused()){
						myMusic->setIsPaused(true);
					}
				}

				screen = Screen::Pause;
				buttonBT = elapsedTime + .4f;
				break;
			}

			cam.canMove = true;
			reticleColour = glm::vec4(1.f);
			if(score < 0){
				score = 0;
			}

			if(playerCurrLives <= 0.f){
				glfwSetInputMode(win, GLFW_CURSOR, GLFW_CURSOR_NORMAL);

				///Pause music
				const size_t& musicSize = music.size();
				for(size_t i = 0; i < musicSize; ++i){
					ISound* const& myMusic = music[i];
					if(myMusic && !myMusic->getIsPaused()){
						myMusic->setIsPaused(true);
					}
				}

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

				screen = Screen::End;
			}

			static_cast<SpriteAni*>(meshes[(int)MeshType::Fire])->Update();
			static_cast<SpriteAni*>(meshes[(int)MeshType::CoinGold])->Update();
			static_cast<SpriteAni*>(meshes[(int)MeshType::CoinSilver])->Update();
			static_cast<SpriteAni*>(meshes[(int)MeshType::CoinPink])->Update();
			static_cast<SpriteAni*>(meshes[(int)MeshType::CoinGreen])->Update();
			static_cast<SpriteAni*>(meshes[(int)MeshType::CoinBlue])->Update();

			///Spawn coins
			if(goldCoinBT <= elapsedTime && goldCoinAmt < 1){
				SpawnEntity(Entity::EntityType::COIN_GOLD, meshes[(int)MeshType::CoinGold], 15.f);
				++goldCoinAmt;
				goldCoinBT = elapsedTime + 60.f;
			}
			if(silverCoinBT <= elapsedTime && silverCoinAmt < 5){
				SpawnEntity(Entity::EntityType::COIN_SILVER, meshes[(int)MeshType::CoinSilver], 15.f);
				++silverCoinAmt;
				silverCoinBT = elapsedTime + 15.f;
			}
			if(pinkCoinBT <= elapsedTime && pinkCoinAmt < 5){
				SpawnEntity(Entity::EntityType::COIN_PINK, meshes[(int)MeshType::CoinPink], 15.f);
				++pinkCoinAmt;
				pinkCoinBT = elapsedTime + 20.f;
			}
			if(greenCoinBT <= elapsedTime && greenCoinAmt < 5){
				SpawnEntity(Entity::EntityType::COIN_GREEN, meshes[(int)MeshType::CoinGreen], 15.f);
				++greenCoinAmt;
				greenCoinBT = elapsedTime + 10.f;
			}
			if(blueCoinBT <= elapsedTime && blueCoinAmt < 5){
				SpawnEntity(Entity::EntityType::COIN_BLUE, meshes[(int)MeshType::CoinBlue], 15.f);
				++blueCoinAmt;
				blueCoinBT = elapsedTime + 60.f;
			}
			if(ammoPickupBT <= elapsedTime && ammoPickupAmt < 3){
				SpawnEntity(Entity::EntityType::AMMO_PICKUP, meshes[(int)MeshType::Sphere], 15.f);
				++ammoPickupAmt;
				ammoPickupBT = elapsedTime + 30.f;
			}
			if(ammoPickup2BT <= elapsedTime && ammoPickup2Amt < 3){
				SpawnEntity(Entity::EntityType::AMMO_PICKUP2, meshes[(int)MeshType::Sphere], 15.f);
				++ammoPickup2Amt;
				ammoPickup2BT = elapsedTime + 30.f;
			}

			///Play music
			const size_t& musicSize = music.size();
			for(size_t i = 0; i < musicSize; ++i){
				ISound* const& myMusic = music[i];
				if(myMusic && myMusic->getIsPaused()){
					myMusic->setIsPaused(false);
				}
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

			float yMin = terrainYScale * static_cast<Terrain*>(meshes[(int)MeshType::Terrain])->GetHeightAtPt(cam.GetPos().x / terrainXScale, cam.GetPos().z / terrainZScale, true);
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

			EntityManager::UpdateParams params;
			params.camCanMove = cam.canMove;
			params.playerCurrHealth = playerCurrHealth;
			params.playerCurrLives = playerCurrLives;
			params.camPos = cam.GetPos();
			params.camFront = cam.CalcFront();
			params.camTrueVel = cam.trueVel;
			params.reticleColour = reticleColour;
			params.enemyCount = enemyCount;
			params.score = score;
			params.terrainMesh = meshes[(int)MeshType::Terrain];
			params.terrainXScale = terrainXScale;
			params.terrainYScale = terrainYScale;
			params.terrainZScale = terrainZScale;
			params.yGround = yMin;
			params.quadMesh = meshes[(int)MeshType::Quad];
			params.goldCoinAmt = goldCoinAmt;
			params.silverCoinAmt = silverCoinAmt;
			params.pinkCoinAmt = pinkCoinAmt;
			params.greenCoinAmt = greenCoinAmt;
			params.blueCoinAmt = blueCoinAmt;
			params.ammoPickupAmt = ammoPickupAmt;
			params.ammoPickup2Amt = ammoPickup2Amt;
			params.addAmmo = addAmmo;
			params.weaponType = weapon->GetCurrentSlot();
			params.takingDmg = takingDmg;
			params.healthUp = healthUp;
			params.lifeUp = lifeUp;
			params.immune = immune;
			params.soundEngine = soundEngine;
			entityManager->UpdateEntities(params);
			addAmmo = params.addAmmo;
			weaponType = params.weaponType;
			reticleColour = params.reticleColour;
			cam.canMove = params.camCanMove;
			playerCurrHealth = params.playerCurrHealth;
			playerCurrLives = params.playerCurrLives;
			enemyCount = params.enemyCount;
			score = params.score;
			goldCoinAmt = params.goldCoinAmt;
			silverCoinAmt = params.silverCoinAmt;
			pinkCoinAmt = params.pinkCoinAmt;
			greenCoinAmt = params.greenCoinAmt;
			blueCoinAmt = params.blueCoinAmt;
			ammoPickupAmt = params.ammoPickupAmt;
			ammoPickup2Amt = params.ammoPickup2Amt;
			takingDmg = params.takingDmg;
			healthUp = params.healthUp;
			lifeUp = params.lifeUp;
			immune = params.immune;

			playerCurrHealth = std::min(100.f, std::max(0.f, playerCurrHealth));
			playerCurrLives = std::min(5.f, std::max(0.f, playerCurrLives));

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
						if(angularFOV != 15.f){
							soundEngine->play2D("Audio/Sounds/Scope.wav", false);
						}
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

			static float distortionBT = 0.f;
			static float echoBT = 0.f;
			static float wavesReverbBT = 0.f;
			static float resetSoundFXBT = 0.f;

			////Control soundFX
			if(Key(GLFW_KEY_I) && distortionBT <= elapsedTime){
				const size_t& musicSize = music.size();
				for(size_t i = 0; i < musicSize; ++i){
					ISoundEffectControl* soundFX = music[i]->getSoundEffectControl();
					if(soundFX){
						soundFX->isDistortionSoundEffectEnabled() ? soundFX->disableDistortionSoundEffect() : (void)soundFX->enableDistortionSoundEffect();
						distortionBT = elapsedTime + .5f;
					} else{
						(void)puts("No soundFX support!\n");
					}
				}
			}

			if(Key(GLFW_KEY_O) && echoBT <= elapsedTime){
				const size_t& musicSize = music.size();
				for(size_t i = 0; i < musicSize; ++i){
					ISoundEffectControl* soundFX = music[i]->getSoundEffectControl();
					if(soundFX){
						soundFX->isEchoSoundEffectEnabled() ? soundFX->disableEchoSoundEffect() : (void)soundFX->enableEchoSoundEffect();
						echoBT = elapsedTime + .5f;
					} else{
						(void)puts("No soundFX support!\n");
					}
				}
			}

			if(Key(GLFW_KEY_K) && wavesReverbBT <= elapsedTime){
				const size_t& musicSize = music.size();
				for(size_t i = 0; i < musicSize; ++i){
					ISoundEffectControl* soundFX = music[i]->getSoundEffectControl();
					if(soundFX){
						soundFX->isWavesReverbSoundEffectEnabled() ? soundFX->disableWavesReverbSoundEffect() : (void)soundFX->enableWavesReverbSoundEffect();
						wavesReverbBT = elapsedTime + .5f;
					} else{
						(void)puts("No soundFX support!\n");
					}
				}
			}

			if(Key(GLFW_KEY_L) && resetSoundFXBT <= elapsedTime){
				const size_t& musicSize = music.size();
				for(size_t i = 0; i < musicSize; ++i){
					ISoundEffectControl* soundFX = music[i]->getSoundEffectControl();
					if(soundFX){
						soundFX->disableAllEffects();
						resetSoundFXBT = elapsedTime + .5f;
					} else{
						(void)puts("No soundFX support!\n");
					}
				}
			}

			// Player gets max health again, but loses 1 life
			if(playerCurrHealth <= 0.f){
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

			if(addAmmo) // Player picked up the ammo collectible
			{
				int temp = weapon->GetCurrentSlot();
				weapon->SetCurrentSlot(weaponType); // Depends which weapon ammo it is
				weapon->GetCurrentWeapon()->AddAmmo();
				addAmmo = false;
				weapon->SetCurrentSlot(temp); // Get back the weapon player was holding
				weaponType = 0;
			}

			if(leftMB){ //Shoot
				if(!weapon->GetCurrentWeapon()->GetReloading() && weapon->GetCurrentWeapon()->GetCanShoot() && weapon->GetCurrentWeapon()->GetCurrentAmmoRound() > 0){
					Entity* const& bullet = entityManager->FetchEntity();
					switch(weapon->GetCurrentSlot()){
						case 0:
							bullet->type = Entity::EntityType::BULLET;
							soundEngine->play2D("Audio/Sounds/Pistol.wav", false);
							break;
						case 1:
							bullet->type = Entity::EntityType::BULLET2;
							soundEngine->play2D("Audio/Sounds/AR.wav", false);
							break;
						case 2:
							bullet->type = Entity::EntityType::BULLET3;
							soundEngine->play2D("Audio/Sounds/Sniper.wav", false);
							break;
					}
					bullet->active = true;
					bullet->pos = glm::vec3(cam.GetPos() + 10.f * cam.CalcFront());
					bullet->vel = cam.CalcFront() * 200.f;
					bullet->mass = 1.f;
					bullet->life = 2.f;
					bullet->colour = glm::vec4(1.F, 0.f, 0.f, 1.f);
					bullet->scale = glm::vec3(1.f);
					bullet->mesh = meshes[(int)MeshType::Sphere];
					weapon->GetCurrentWeapon()->SetCanShoot(false); // For the shooting cooldown time
					if(!weapon->GetCurrentSlot() == 0) // Not using the pisol, since pistol has unlimited ammo
						weapon->GetCurrentWeapon()->SetCurrentAmmoRound(weapon->GetCurrentWeapon()->GetCurrentAmmoRound() - 1); // Decrease the ammo
					lastTime = elapsedTime;
				}
			}
			static bool pressedReload = false;
			if(Key(GLFW_KEY_R)){ // Reload the curr weapon
				// Begin to reload
				if(weapon->GetCurrentWeapon()->GetCurrentAmmoRound() < weapon->GetCurrentWeapon()->GetMaxAmmoRound()
					&& weapon->GetCurrentWeapon()->GetCurrentTotalAmmo() > 0 && !weapon->GetCurrentWeapon()->GetReloading()){
					soundEngine->play2D("Audio/Sounds/Reload.wav", false);
					weapon->GetCurrentWeapon()->SetReloading(true);
					pressedReload = true;
					lastTime = elapsedTime;
				}
			}
			if(!weapon->GetCurrentWeapon()->GetReloading() && pressedReload){
				weapon->GetCurrentWeapon()->Reload();
				pressedReload = false;
			}

			///Enemy waves system
			static float enemyWavesBT = 0.f;
			switch(waves[waveCount]){
				case (int)WaveNumber::One: {
					if(enemyWavesBT <= elapsedTime && enemyCount <= 5){
						enemyWavesBT = elapsedTime + 5.f;

						for(int i = 0; i < 2; ++i) {
							const float scaleFactor = 15.f;
							const float xPos = PseudorandMinMax(-terrainXScale / 2.f + 5.f + scaleFactor, terrainXScale / 2.f - 5.f - scaleFactor);
							const float zPos = PseudorandMinMax(-terrainZScale / 2.f + 5.f + scaleFactor, terrainZScale / 2.f - 5.f - scaleFactor);
							const glm::vec3 pos = glm::vec3(xPos, terrainYScale * static_cast<Terrain*>(meshes[(int)MeshType::Terrain])->GetHeightAtPt(xPos / terrainXScale, zPos / terrainZScale) + scaleFactor, zPos);

							Entity* const& movingEnemy = entityManager->FetchEntity();
							movingEnemy->type = Entity::EntityType::MOVING_ENEMY;
							movingEnemy->active = true;
							movingEnemy->life = 20.f;
							movingEnemy->maxLife = 20.f;
							movingEnemy->colour = glm::vec4(1.f);
							movingEnemy->pos = pos;
							movingEnemy->vel = glm::vec3(0.f);
							movingEnemy->mass = 5.f;
							movingEnemy->scale = glm::vec3(10.f);
							movingEnemy->mesh = meshes[(int)MeshType::Sphere];
							movingEnemy->model = models[(int)ModelType::Virus];
							movingEnemy->isShot = false;
							++enemyCount;
						}

						if(currentEnemyCount <= 0)
						{
							++waveCount;
							enemyCount = 0;
							break;
						}

					}
				}
				case (int)WaveNumber::Two: {
					if(enemyCount == 0){
						currentEnemyCount = 20;
					}
					if(enemyWavesBT <= elapsedTime && enemyCount <= 7){
						enemyWavesBT = elapsedTime + 4.f;

						for(int i = 0; i < 2; ++i) {
							const float scaleFactor = 15.f;
							const float xPos = PseudorandMinMax(-terrainXScale / 2.f + 5.f + scaleFactor, terrainXScale / 2.f - 5.f - scaleFactor);
							const float zPos = PseudorandMinMax(-terrainZScale / 2.f + 5.f + scaleFactor, terrainZScale / 2.f - 5.f - scaleFactor);
							const glm::vec3 pos = glm::vec3(xPos, terrainYScale * static_cast<Terrain*>(meshes[(int)MeshType::Terrain])->GetHeightAtPt(xPos / terrainXScale, zPos / terrainZScale) + scaleFactor, zPos);

							Entity* const& movingEnemy = entityManager->FetchEntity();
							movingEnemy->type = Entity::EntityType::MOVING_ENEMY;
							movingEnemy->active = true;
							movingEnemy->life = 20.f;
							movingEnemy->maxLife = 20.f;
							movingEnemy->colour = glm::vec4(1.f);
							movingEnemy->pos = pos;
							movingEnemy->vel = glm::vec3(0.f);
							movingEnemy->mass = 5.f;
							movingEnemy->scale = glm::vec3(10.f);
							movingEnemy->mesh = meshes[(int)MeshType::Sphere];
							movingEnemy->model = models[(int)ModelType::Virus];
							movingEnemy->isShot = false;
							++enemyCount;
						}

						if(currentEnemyCount <= 0)
						{
							++waveCount;
							enemyCount = 0;
							break;
						}
					}
				}
				case (int)WaveNumber::Three: {
					if(enemyCount == 0){
						currentEnemyCount = 20;
					}
					if(enemyWavesBT <= elapsedTime && enemyCount <= 11)
					{
						enemyWavesBT = elapsedTime + 3.f;

						for(int i = 0; i < 2; ++i) {
							const float scaleFactor = 15.f;
							const float xPos = PseudorandMinMax(-terrainXScale / 2.f + 5.f + scaleFactor, terrainXScale / 2.f - 5.f - scaleFactor);
							const float zPos = PseudorandMinMax(-terrainZScale / 2.f + 5.f + scaleFactor, terrainZScale / 2.f - 5.f - scaleFactor);
							const glm::vec3 pos = glm::vec3(xPos, terrainYScale * static_cast<Terrain*>(meshes[(int)MeshType::Terrain])->GetHeightAtPt(xPos / terrainXScale, zPos / terrainZScale) + scaleFactor, zPos);

							Entity* const& movingEnemy = entityManager->FetchEntity();
							movingEnemy->type = Entity::EntityType::MOVING_ENEMY;
							movingEnemy->active = true;
							movingEnemy->life = 20.f;
							movingEnemy->maxLife = 20.f;
							movingEnemy->colour = glm::vec4(1.f);
							movingEnemy->pos = pos;
							movingEnemy->vel = glm::vec3(0.f);
							movingEnemy->mass = 5.f;
							movingEnemy->scale = glm::vec3(10.f);
							movingEnemy->mesh = meshes[(int)MeshType::Sphere];
							movingEnemy->model = models[(int)ModelType::Virus];
							movingEnemy->isShot = false;
							++enemyCount;
						}

						if(currentEnemyCount <= 0)
						{
							++waveCount;
							enemyCount = 0;
							break;
						}
					}
				}
				case (int)WaveNumber::Total:
					break;
			}

			break;
		}
		case Screen::Pause: {
			if(Key(GLFW_KEY_P) && buttonBT <= elapsedTime){
				glfwSetInputMode(win, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

				///Play music
				const size_t& musicSize = music.size();
				for(size_t i = 0; i < musicSize; ++i){
					ISound* const& myMusic = music[i];
					if(myMusic && myMusic->getIsPaused()){
						myMusic->setIsPaused(false);
					}
				}

				cam.SetPos(glm::vec3(0.f, 0.f, 50.f));
				cam.SetTarget(glm::vec3(0.f));
				cam.SetUp(glm::vec3(0.f, 1.f, 0.f));

				screen = Screen::Game;
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

void Scene::GeoRenderPass(){
	if(screen == Screen::Game){
		geoPassSP->Use();
		geoPassSP->SetMat4fv("PV", &(projection * view)[0][0]);

		///Terrain
		modelStack.PushModel({
			modelStack.Scale(glm::vec3(terrainXScale, terrainYScale, terrainZScale)),
		});
			meshes[(int)MeshType::Terrain]->SetModel(modelStack.GetTopModel());
			meshes[(int)MeshType::Terrain]->Render(*geoPassSP);
		modelStack.PopModel();

		///Render curr weapon
		const glm::vec3 front = cam.CalcFront();
		const float sign = front.y < 0.f ? -1.f : 1.f;
		auto rotationMat = glm::rotate(glm::mat4(1.f), sign * acosf(glm::dot(front, glm::normalize(glm::vec3(front.x, 0.f, front.z)))), glm::normalize(glm::vec3(-front.z, 0.f, front.x)));
		modelStack.PushModel({
			modelStack.Translate(cam.GetPos()
				+ glm::vec3(rotationMat * glm::vec4(RotateVecIn2D(glm::vec3(5.5f, -7.f, -13.f), atan2(front.x, front.z) + glm::radians(180.f), Axis::y), 1.f))),
			modelStack.Rotate(glm::vec4(glm::vec3(-front.z, 0.f, front.x), sign * glm::degrees(acosf(glm::dot(front, glm::normalize(glm::vec3(front.x, 0.f, front.z))))))),
			modelStack.Rotate(glm::vec4(0.f, 1.f, 0.f, glm::degrees(atan2(front.x, front.z)))),
			modelStack.Scale(glm::vec3(5.f)),
		});
		switch(weapon->GetCurrentSlot()){
			case 0:
				models[(int)ModelType::Pistol]->SetModelForAll(modelStack.GetTopModel());
				models[(int)ModelType::Pistol]->Render(*geoPassSP);
				break;
			case 1:
				models[(int)ModelType::AR]->SetModelForAll(modelStack.GetTopModel());
				models[(int)ModelType::AR]->Render(*geoPassSP);
				break;
			case 2:
				models[(int)ModelType::Sniper]->SetModelForAll(modelStack.GetTopModel());
				models[(int)ModelType::Sniper]->Render(*geoPassSP);
				break;
		}
		modelStack.PopModel();
	}
}

void Scene::LightingRenderPass(const uint& posTexRefID, const uint& coloursTexRefID, const uint& normalsTexRefID, const uint& specTexRefID, const uint& reflectionTexRefID,
	const uint& depthDTexRefID, const uint& depthSTexRefID){
	if(screen == Screen::Game){
		lightingPassSP.Use();

		lightingPassSP.SetMat4fv("directionalLightPV", &(glm::ortho(-600.f, 600.f, -600.f, 600.f, 20.f, 300.f) * dCam.LookAt())[0][0]);
		lightingPassSP.SetMat4fv("spotlightPV", &(glm::perspective(glm::radians(45.f), sCam.GetAspectRatio(), 170.f, 14000.f) * sCam.LookAt())[0][0]);
		if(scope){ //If scoped in...
			lightingPassSP.Set1i("nightVision", 1);
		}

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

		lightingPassSP.UseTex(depthDTexRefID, "dDepthTexSampler");
		lightingPassSP.UseTex(depthSTexRefID, "sDepthTexSampler");

		int i;
		for(i = 0; i < pAmt; ++i) {
			const PtLight* const& ptLight = static_cast<PtLight*>(ptLights[i]);
			lightingPassSP.Set3fv(("ptLights[" + std::to_string(i) + "].ambient").c_str(), ptLight->ambient);
			lightingPassSP.Set3fv(("ptLights[" + std::to_string(i) + "].diffuse").c_str(), ptLight->diffuse);
			lightingPassSP.Set3fv(("ptLights[" + std::to_string(i) + "].spec").c_str(), ptLight->spec);
			lightingPassSP.Set3fv(("ptLights[" + std::to_string(i) + "].pos").c_str(), ptLight->pos);
			lightingPassSP.Set1f(("ptLights[" + std::to_string(i) + "].constant").c_str(), ptLight->constant);
			lightingPassSP.Set1f(("ptLights[" + std::to_string(i) + "].linear").c_str(), ptLight->linear);
			lightingPassSP.Set1f(("ptLights[" + std::to_string(i) + "].quadratic").c_str(), ptLight->quadratic);
		}
		for(i = 0; i < dAmt; ++i) {
			const DirectionalLight* const& directionalLight = static_cast<DirectionalLight*>(directionalLights[i]);
			lightingPassSP.Set3fv(("directionalLights[" + std::to_string(i) + "].ambient").c_str(), directionalLight->ambient);
			lightingPassSP.Set3fv(("directionalLights[" + std::to_string(i) + "].diffuse").c_str(), directionalLight->diffuse);
			lightingPassSP.Set3fv(("directionalLights[" + std::to_string(i) + "].spec").c_str(), directionalLight->spec);
			lightingPassSP.Set3fv(("directionalLights[" + std::to_string(i) + "].dir").c_str(), directionalLight->dir);
		}
		for(i = 0; i < sAmt; ++i) {
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
		meshes[(int)MeshType::Quad]->Render(lightingPassSP, false, false);

		lightingPassSP.Set1i("nightVision", 0);
		lightingPassSP.ResetTexUnits();
	}
}

void Scene::BlurRender(const uint& brightTexRefID, const bool& horizontal) {
	if(screen == Screen::Game){
		blurSP.Use();
		blurSP.Set1i("horizontal", horizontal);
		blurSP.UseTex(brightTexRefID, "texSampler");
		meshes[(int)MeshType::Quad]->SetModel(modelStack.GetTopModel());
		meshes[(int)MeshType::Quad]->Render(blurSP, false, false);
		blurSP.ResetTexUnits();
	}
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
			meshes[(int)MeshType::Quad]->Render(screenSP, false, false);
		modelStack.PopModel();
		screenSP.ResetTexUnits();
	}
}

void Scene::DepthRender(const short& projectionType){
	depthSP.Use();
	if(projectionType){
		depthSP.SetMat4fv("PV", &(glm::perspective(glm::radians(45.f), sCam.GetAspectRatio(), 170.f, 14000.f) * sCam.LookAt())[0][0]);
	} else{
		depthSP.SetMat4fv("PV", &(glm::ortho(-600.f, 600.f, -600.f, 600.f, 20.f, 300.f) * dCam.LookAt())[0][0]);
	}

	///Terrain
	modelStack.PushModel({
		modelStack.Scale(glm::vec3(terrainXScale, terrainYScale, terrainZScale)),
	});
		meshes[(int)MeshType::Terrain]->SetModel(modelStack.GetTopModel());
		meshes[(int)MeshType::Terrain]->Render(depthSP, false);
	modelStack.PopModel();

	glCullFace(GL_FRONT);

	///Grass
	models[(int)ModelType::Grass]->SetModelForAll(modelStack.GetTopModel());
	models[(int)ModelType::Grass]->InstancedRender(depthSP, false);

	glCullFace(GL_BACK);
}

void Scene::PlanarReflectionRender(){
	forwardSP->Use();
	forwardSP->Set1f("shininess", 32.f); //More light scattering if lower
	forwardSP->Set3fv("globalAmbient", Light::globalAmbient);
	forwardSP->Set3fv("camPos", cam.GetPos());
	forwardSP->Set1i("pAmt", 0);
	forwardSP->Set1i("dAmt", 0);
	forwardSP->Set1i("sAmt", 0);

	forwardSP->SetMat4fv("PV", &(glm::perspective(glm::radians(90.f), waterCam.GetAspectRatio(), .1f, 9999.f) * glm::mat4(glm::mat3(waterCam.LookAt())))[0][0]);

	glDepthFunc(GL_LEQUAL); //Modify comparison operators used for depth test such that frags with depth <= 1.f are shown
	glCullFace(GL_FRONT);
	forwardSP->Set1i("sky", 1);
	modelStack.PushModel({
		modelStack.Rotate(glm::vec4(0.f, 1.f, 0.f, glfwGetTime())),
	});
		meshes[(int)MeshType::Sphere]->SetModel(modelStack.GetTopModel());
		meshes[(int)MeshType::Sphere]->Render(*forwardSP);
	modelStack.PopModel();
	forwardSP->Set1i("sky", 0);
	glCullFace(GL_BACK);
	glDepthFunc(GL_LESS);

	forwardSP->SetMat4fv("PV", &(glm::perspective(glm::radians(90.f), 1.f, .1f, 9999.f) * waterCam.LookAt())[0][0]);

	modelStack.PushModel({
		modelStack.Translate(glm::vec3(0.f, 200.f, 0.f)),
		modelStack.Scale(glm::vec3(25.f)),
	});
		meshes[(int)MeshType::Sphere]->SetModel(modelStack.GetTopModel());
		meshes[(int)MeshType::Sphere]->Render(*forwardSP);
	modelStack.PopModel();
}

void Scene::CubemapReflectionRender(const short& cubemapFace){
	forwardSP->Use();
	forwardSP->Set1f("shininess", 32.f); //More light scattering if lower
	forwardSP->Set3fv("globalAmbient", Light::globalAmbient);
	forwardSP->Set3fv("camPos", cam.GetPos());
	forwardSP->Set1i("pAmt", 0);
	forwardSP->Set1i("dAmt", 0);
	forwardSP->Set1i("sAmt", 0);

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

	forwardSP->SetMat4fv("PV", &(glm::perspective(glm::radians(90.f), enCam.GetAspectRatio(), .1f, 9999.f) * glm::mat4(glm::mat3(enCam.LookAt())))[0][0]);

	glDepthFunc(GL_LEQUAL); //Modify comparison operators used for depth test such that frags with depth <= 1.f are shown
	glCullFace(GL_FRONT);
	forwardSP->Set1i("sky", 1);
	modelStack.PushModel({
		modelStack.Rotate(glm::vec4(0.f, 1.f, 0.f, glfwGetTime())),
	});
		meshes[(int)MeshType::Sphere]->SetModel(modelStack.GetTopModel());
		meshes[(int)MeshType::Sphere]->Render(*forwardSP);
	modelStack.PopModel();
	forwardSP->Set1i("sky", 0);
	glCullFace(GL_BACK);
	glDepthFunc(GL_LESS);

	forwardSP->SetMat4fv("PV", &(glm::perspective(glm::radians(90.f), 1.f, .1f, 9999.f) * enCam.LookAt())[0][0]);

	modelStack.PushModel({
		modelStack.Scale(glm::vec3(terrainXScale, terrainYScale, terrainZScale)),
	});
		forwardSP->Set1i("noNormals", 1);
		meshes[(int)MeshType::Terrain]->SetModel(modelStack.GetTopModel());
		meshes[(int)MeshType::Terrain]->Render(*forwardSP);
		forwardSP->Set1i("noNormals", 0);
	modelStack.PopModel();
}

void Scene::ForwardRender(const uint& depthDTexRefID, const uint& depthSTexRefID, const uint& planarReflectionTexID, const uint& cubemapReflectionTexID){
	forwardSP->Use();
	forwardSP->SetMat4fv("directionalLightPV", &(glm::ortho(-600.f, 600.f, -600.f, 600.f, 20.f, 300.f) * dCam.LookAt())[0][0]);
	forwardSP->SetMat4fv("spotlightPV", &(glm::perspective(glm::radians(45.f), sCam.GetAspectRatio(), 170.f, 14000.f) * sCam.LookAt())[0][0]);

	const int& dAmt = (int)directionalLights.size();
	const int& sAmt = (int)spotlights.size();

	forwardSP->Set1f("shininess", 32.f); //More light scattering if lower
	forwardSP->Set3fv("globalAmbient", Light::globalAmbient);
	forwardSP->Set3fv("camPos", cam.GetPos());
	forwardSP->Set1i("pAmt", 0);
	forwardSP->Set1i("dAmt", screen == Screen::Game ? dAmt : 0);
	forwardSP->Set1i("sAmt", screen == Screen::Game ? sAmt : 0);

	int i;
	for(i = 0; i < dAmt; ++i){
		const DirectionalLight* const& directionalLight = static_cast<DirectionalLight*>(directionalLights[i]);
		forwardSP->Set3fv(("directionalLights[" + std::to_string(i) + "].ambient").c_str(), directionalLight->ambient);
		forwardSP->Set3fv(("directionalLights[" + std::to_string(i) + "].diffuse").c_str(), directionalLight->diffuse);
		forwardSP->Set3fv(("directionalLights[" + std::to_string(i) + "].spec").c_str(), directionalLight->spec);
		forwardSP->Set3fv(("directionalLights[" + std::to_string(i) + "].dir").c_str(), directionalLight->dir);
	}
	for(i = 0; i < sAmt; ++i){
		const Spotlight* const& spotlight = static_cast<Spotlight*>(spotlights[i]);
		forwardSP->Set3fv(("spotlights[" + std::to_string(i) + "].ambient").c_str(), spotlight->ambient);
		forwardSP->Set3fv(("spotlights[" + std::to_string(i) + "].diffuse").c_str(), spotlight->diffuse);
		forwardSP->Set3fv(("spotlights[" + std::to_string(i) + "].spec").c_str(), spotlight->spec);
		forwardSP->Set3fv(("spotlights[" + std::to_string(i) + "].pos").c_str(), spotlight->pos);
		forwardSP->Set3fv(("spotlights[" + std::to_string(i) + "].dir").c_str(), spotlight->dir);
		forwardSP->Set1f(("spotlights[" + std::to_string(i) + "].cosInnerCutoff").c_str(), spotlight->cosInnerCutoff);
		forwardSP->Set1f(("spotlights[" + std::to_string(i) + "].cosOuterCutoff").c_str(), spotlight->cosOuterCutoff);
	}

	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	switch(screen){
		case Screen::End:
		case Screen::Menu: {
			forwardSP->SetMat4fv("PV", &(projection * view)[0][0]);

			///BG
			modelStack.PushModel({
				modelStack.Scale(glm::vec3(float(winWidth) / 2.f, float(winHeight) / 2.f, 1.f)),
			});
				forwardSP->Set1i("useCustomDiffuseTexIndex", 1);
				forwardSP->Set1i("customDiffuseTexIndex", 5);
				forwardSP->Set1i("noNormals", 1);
				meshes[(int)MeshType::Quad]->SetModel(modelStack.GetTopModel());
				meshes[(int)MeshType::Quad]->Render(*forwardSP);
				forwardSP->Set1i("noNormals", 0);
				forwardSP->Set1i("useCustomDiffuseTexIndex", 0);
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
			if(scope){ //If scoped in...
				forwardSP->Set1i("nightVision", 1);
			}

			///Sky
			forwardSP->SetMat4fv("PV", &(projection * glm::mat4(glm::mat3(view)))[0][0]);
			glDepthFunc(GL_LEQUAL); //Modify comparison operators used for depth test such that frags with depth <= 1.f are shown
			glCullFace(GL_FRONT);
			modelStack.PushModel({
				modelStack.Rotate(glm::vec4(0.f, 1.f, 0.f, glfwGetTime())),
			});
				forwardSP->Set1i("sky", 1);
				meshes[(int)MeshType::Sphere]->SetModel(modelStack.GetTopModel());
				meshes[(int)MeshType::Sphere]->Render(*forwardSP);
				forwardSP->Set1i("sky", 0);
			modelStack.PopModel();
			glCullFace(GL_BACK);
			glDepthFunc(GL_LESS);

			forwardSP->SetMat4fv("PV", &(projection * view)[0][0]);

			///Crystal ball
			modelStack.PushModel({
				modelStack.Translate(glm::vec3(0.f, 200.f, 0.f)),
				modelStack.Scale(glm::vec3(25.f)),
			});
				forwardSP->UseTex(depthDTexRefID, "dDepthTexSampler");
				forwardSP->UseTex(depthSTexRefID, "sDepthTexSampler");
				forwardSP->UseTex(cubemapReflectionTexID, "cubemapSampler", GL_TEXTURE_CUBE_MAP);
				forwardSP->Set1i("useCustomDiffuseTexIndex", 1);
				forwardSP->Set1i("customDiffuseTexIndex", -1);
				forwardSP->Set1i("useCustomColour", 1);
				forwardSP->Set4fv("customColour", glm::vec4(glm::vec3(0.f), 1.f));
				meshes[(int)MeshType::Sphere]->SetModel(modelStack.GetTopModel());
				meshes[(int)MeshType::Sphere]->Render(*forwardSP);
				forwardSP->Set1i("useCustomColour", 0);
				forwardSP->Set1i("useCustomDiffuseTexIndex", 0);
			modelStack.PopModel();

			///Grass
			forwardSP->UseTex(depthDTexRefID, "dDepthTexSampler");
			forwardSP->UseTex(depthSTexRefID, "sDepthTexSampler");
			models[(int)ModelType::Grass]->SetModelForAll(modelStack.GetTopModel());
			models[(int)ModelType::Grass]->InstancedRender(*forwardSP);

			///Water
			modelStack.PushModel({
				modelStack.Translate(glm::vec3(-15.f, 40.f, -20.f)),
				modelStack.Rotate(glm::vec4(1.f, 0.f, 0.f, -90.f)),
				modelStack.Scale(glm::vec3(400.f)),
			});
				forwardSP->UseTex(depthDTexRefID, "dDepthTexSampler");
				forwardSP->UseTex(depthSTexRefID, "sDepthTexSampler");
				forwardSP->UseTex(planarReflectionTexID, "planarReflectionTex");
				forwardSP->Set1i("water", 1);
				forwardSP->Set1f("elapsedTime", elapsedTime);
				forwardSP->Set1i("useCustomColour", 1);
				forwardSP->Set4fv("customColour", glm::vec4(glm::vec3(.2f), .7f));
					meshes[(int)MeshType::Water]->SetModel(modelStack.GetTopModel());
					meshes[(int)MeshType::Water]->Render(*forwardSP);
				forwardSP->Set1i("useCustomColour", 0);
				forwardSP->Set1i("water", 0);
			modelStack.PopModel();

			///Render entities
			EntityManager::RenderParams params;
			params.minimap = false;
			params.camPos = cam.GetPos();
			params.camFront = cam.CalcFront();
			params.depthDTexRefID = depthDTexRefID;
			params.depthSTexRefID = depthSTexRefID;
			params.quadMesh = meshes[(int)MeshType::Quad];
			entityManager->RenderEntities(*forwardSP, params);

			///Border effects
			if(scope){
				forwardSP->SetMat4fv("PV", &(glm::ortho(-float(winWidth) / 2.f, float(winWidth) / 2.f, -float(winHeight) / 2.f, float(winHeight) / 2.f, .1f, 9999.f))[0][0]);
				forwardSP->Set1i("useCustomDiffuseTexIndex", 1);
				forwardSP->Set1i("noNormals", 1);

				modelStack.PushModel({
					modelStack.Translate(glm::vec3(0.f, 0.f, -9.f)),
					modelStack.Scale(glm::vec3(float(winHeight) / 2.f, float(winHeight) / 2.f, 1.f)),
				});
					forwardSP->Set1i("customDiffuseTexIndex", 6);
					meshes[(int)MeshType::Quad]->SetModel(modelStack.GetTopModel());
					meshes[(int)MeshType::Quad]->Render(*forwardSP);
				modelStack.PopModel();
				modelStack.PushModel({
					modelStack.Translate(glm::vec3(0.f, 0.f, -9.1f)),
					modelStack.Scale(glm::vec3(float(winWidth) / 2.f, float(winHeight) / 2.f, 1.f)),
				});
					forwardSP->Set1i("customDiffuseTexIndex", -1);
					forwardSP->Set1i("useCustomColour", 1);
					forwardSP->Set4fv("customColour", glm::vec4(glm::vec3(0.f), 1.f));
					meshes[(int)MeshType::Quad]->SetModel(modelStack.GetTopModel());
					meshes[(int)MeshType::Quad]->Render(*forwardSP);
					forwardSP->Set1i("useCustomColour", 0);
				modelStack.PopModel();

				forwardSP->Set1i("noNormals", 0);
				forwardSP->Set1i("useCustomDiffuseTexIndex", 0);
				glBlendFunc(GL_ONE, GL_ZERO);
				return;
			}
			if(takingDmg){
				forwardSP->SetMat4fv("PV", &(glm::ortho(-float(winWidth) / 2.f, float(winWidth) / 2.f, -float(winHeight) / 2.f, float(winHeight) / 2.f, .1f, 9999.f))[0][0]);
				forwardSP->Set1i("useCustomDiffuseTexIndex", 1);
				forwardSP->Set1i("noNormals", 1);

				modelStack.PushModel({
					modelStack.Translate(glm::vec3(0.f, 0.f, -20.f)),
					modelStack.Scale(glm::vec3(float(winWidth) / 2.f, float(winHeight) / 2.f, 1.f)),
				});
					forwardSP->Set1i("customDiffuseTexIndex", 7);
					meshes[(int)MeshType::Quad]->SetModel(modelStack.GetTopModel());
					meshes[(int)MeshType::Quad]->Render(*forwardSP);
				modelStack.PopModel();

				forwardSP->Set1i("noNormals", 0);
				forwardSP->Set1i("useCustomDiffuseTexIndex", 0);
				forwardSP->SetMat4fv("PV", &(projection * view)[0][0]);
			}
			if(healthUp > 0.f){
				forwardSP->SetMat4fv("PV", &(glm::ortho(-float(winWidth) / 2.f, float(winWidth) / 2.f, -float(winHeight) / 2.f, float(winHeight) / 2.f, .1f, 9999.f))[0][0]);
				forwardSP->Set1i("useCustomDiffuseTexIndex", 1);
				forwardSP->Set1i("noNormals", 1);

				modelStack.PushModel({
					modelStack.Translate(glm::vec3(0.f, 0.f, -20.f)),
					modelStack.Scale(glm::vec3(float(winWidth) / 2.f, float(winHeight) / 2.f, 1.f)),
				});
					forwardSP->Set1i("customDiffuseTexIndex", 8);
					meshes[(int)MeshType::Quad]->SetModel(modelStack.GetTopModel());
					meshes[(int)MeshType::Quad]->Render(*forwardSP);
				modelStack.PopModel();

				forwardSP->Set1i("noNormals", 0);
				forwardSP->Set1i("useCustomDiffuseTexIndex", 0);
				forwardSP->SetMat4fv("PV", &(projection * view)[0][0]);
			}
			if(lifeUp > 0.f){
				forwardSP->SetMat4fv("PV", &(glm::ortho(-float(winWidth) / 2.f, float(winWidth) / 2.f, -float(winHeight) / 2.f, float(winHeight) / 2.f, .1f, 9999.f))[0][0]);
				forwardSP->Set1i("useCustomDiffuseTexIndex", 1);
				forwardSP->Set1i("noNormals", 1);

				modelStack.PushModel({
					modelStack.Translate(glm::vec3(0.f, 0.f, -20.f)),
					modelStack.Scale(glm::vec3(float(winWidth) / 2.f, float(winHeight) / 2.f, 1.f)),
					});
				forwardSP->Set1i("customDiffuseTexIndex", 9);
				meshes[(int)MeshType::Quad]->SetModel(modelStack.GetTopModel());
				meshes[(int)MeshType::Quad]->Render(*forwardSP);
				modelStack.PopModel();

				forwardSP->Set1i("noNormals", 0);
				forwardSP->Set1i("useCustomDiffuseTexIndex", 0);
				forwardSP->SetMat4fv("PV", &(projection * view)[0][0]);
			}
			if(immune){
				forwardSP->SetMat4fv("PV", &(glm::ortho(-float(winWidth) / 2.f, float(winWidth) / 2.f, -float(winHeight) / 2.f, float(winHeight) / 2.f, .1f, 9999.f))[0][0]);
				forwardSP->Set1i("useCustomDiffuseTexIndex", 1);
				forwardSP->Set1i("noNormals", 1);

				modelStack.PushModel({
					modelStack.Translate(glm::vec3(0.f, 0.f, -20.f)),
					modelStack.Scale(glm::vec3(float(winWidth) / 2.f, float(winHeight) / 2.f, 1.f)),
				});
					forwardSP->Set1i("customDiffuseTexIndex", 10);
					meshes[(int)MeshType::Quad]->SetModel(modelStack.GetTopModel());
					meshes[(int)MeshType::Quad]->Render(*forwardSP);
				modelStack.PopModel();

				forwardSP->Set1i("noNormals", 0);
				forwardSP->Set1i("useCustomDiffuseTexIndex", 0);
				forwardSP->SetMat4fv("PV", &(projection * view)[0][0]);
			}

			////Render GUI
			forwardSP->SetMat4fv("PV", &(glm::ortho(-float(winWidth) / 2.f, float(winWidth) / 2.f, -float(winHeight) / 2.f, float(winHeight) / 2.f, .1f, 9999.f))[0][0]);
			forwardSP->Set1i("noNormals", 1);
			forwardSP->Set1i("useCustomColour", 1);
			forwardSP->Set1i("useCustomDiffuseTexIndex", 1);

			///Render reticle
			if(weapon->GetCurrentSlot() < 2){
				modelStack.PushModel({
					modelStack.Translate(glm::vec3(0.f, 0.f, -9.f)),
					modelStack.Scale(glm::vec3(40.f, 40.f, 1.f)),
				});
					forwardSP->Set4fv("customColour", reticleColour);
					forwardSP->Set1i("customDiffuseTexIndex", 3);
					meshes[(int)MeshType::Quad]->SetModel(modelStack.GetTopModel());
					meshes[(int)MeshType::Quad]->Render(*forwardSP);

					modelStack.PushModel({
						modelStack.Translate(glm::vec3(0.f, 0.f, 1.f)),
					});
					if(rightMB){
						modelStack.PushModel({
							modelStack.Scale(glm::vec3(.7f, .7f, 1.f)),
						});
					}
						forwardSP->Set1i("customDiffuseTexIndex", 4);
						meshes[(int)MeshType::Quad]->SetModel(modelStack.GetTopModel());
						meshes[(int)MeshType::Quad]->Render(*forwardSP);
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
				forwardSP->Set4fv("customColour", glm::vec4(glm::vec3(.3f), 1.f));
				forwardSP->Set1i("customDiffuseTexIndex", -1);
				meshes[(int)MeshType::Quad]->SetModel(modelStack.GetTopModel());
				meshes[(int)MeshType::Quad]->Render(*forwardSP);

				modelStack.PushModel({
					modelStack.Translate(glm::vec3((playerCurrHealth - playerMaxHealth) / playerMaxHealth, 0.f, 1.f)), // Translate to the left based on the amount of health to go back to max health
					modelStack.Scale(glm::vec3(playerCurrHealth / playerMaxHealth, 1.f, 1.f)), // Scale the x component based on the current health
				});
					forwardSP->Set4fv("customColour", glm::vec4(0.f, 1.f, 0.f, 1.f));
					forwardSP->Set1i("customDiffuseTexIndex", -1);
					meshes[(int)MeshType::Quad]->SetModel(modelStack.GetTopModel());
					meshes[(int)MeshType::Quad]->Render(*forwardSP);
				modelStack.PopModel();
			modelStack.PopModel();

			///Render player lives
			for(float j = 0; j < playerCurrLives; ++j){
				modelStack.PushModel({
					modelStack.Translate(glm::vec3(-float(winWidth) / 2.2f, float(winHeight) / 2.2f, -9.f) + glm::vec3(75.f * (float)j, 0.f, 0.f)), //??
					modelStack.Scale(glm::vec3(25.f)),
				});
					forwardSP->Set4fv("customColour", glm::vec4(glm::vec3(1.f, 0.f, 0.f), 1.f));
					forwardSP->Set1i("customDiffuseTexIndex", 1);
					meshes[(int)MeshType::Quad]->SetModel(modelStack.GetTopModel());
					meshes[(int)MeshType::Quad]->Render(*forwardSP);
				modelStack.PopModel();
			}

			///Render ammo bar
			modelStack.PushModel({
				modelStack.Translate(glm::vec3(float(winWidth) / 3.f, -float(winHeight) / 2.2f, -10.f)),
				modelStack.Scale(glm::vec3(float(winWidth) / 15.f, float(winHeight) / 50.f, 1.f)),
			});
				forwardSP->Set4fv("customColour", glm::vec4(glm::vec3(.3f), 1.f));
				forwardSP->Set1i("customDiffuseTexIndex", -1);
				meshes[(int)MeshType::Quad]->SetModel(modelStack.GetTopModel());
				meshes[(int)MeshType::Quad]->Render(*forwardSP);

				///Show status of ammo bar(i.e. curr ammo of the round)
				modelStack.PushModel({
					modelStack.Translate(glm::vec3(-float(weapon->GetCurrentWeapon()->GetMaxAmmoRound() - weapon->GetCurrentWeapon()->GetCurrentAmmoRound())
					/ float(weapon->GetCurrentWeapon()->GetMaxAmmoRound()), 0.f, 1.f)), // Translate to the left based on the amount of ammo to go back to max ammo of the round
					modelStack.Scale(glm::vec3(float(weapon->GetCurrentWeapon()->GetCurrentAmmoRound())
					/ float(weapon->GetCurrentWeapon()->GetMaxAmmoRound()), 1.f, 1.f)), // Scale the x component based on the current ammo of the round
				});
					forwardSP->Set4fv("customColour", glm::vec4(glm::vec3(1.f, 0.f, 1.f), 1.f));
					forwardSP->Set1i("customDiffuseTexIndex", -1);
					meshes[(int)MeshType::Quad]->SetModel(modelStack.GetTopModel());
					meshes[(int)MeshType::Quad]->Render(*forwardSP);
				modelStack.PopModel();
			modelStack.PopModel();

			///Render inv slots and guns in inv
			for(int j = 0; j < 5; ++j){
				modelStack.PushModel({
					modelStack.Translate(glm::vec3(-float(winWidth) / 6.f, -float(winHeight) / 2.3f, -11.f) + glm::vec3(j * 100.f, 0.f, 0.f)),
					modelStack.Scale(glm::vec3(50.f)),
				});
				if(weapon->GetCurrentSlot() == j){
					forwardSP->Set4fv("customColour", glm::vec4(0.f, 1.f, 1.f, 1.f));
				} else{
					forwardSP->Set4fv("customColour", glm::vec4(1.f));
				}
					forwardSP->Set1i("customDiffuseTexIndex", 2);
					meshes[(int)MeshType::Quad]->SetModel(modelStack.GetTopModel());
					meshes[(int)MeshType::Quad]->Render(*forwardSP);	///Render guns in inv

					forwardSP->Set1i("useCustomDiffuseTexIndex", 0);
					forwardSP->Set1i("useCustomColour", 0);
					switch(j){
						case 0:
							modelStack.PushModel({
								modelStack.Translate(glm::vec3(-.2f, -.6f, 0.f)),
								modelStack.Rotate(glm::vec4(0.f, 0.f, 1.f, 45.f)),
								modelStack.Rotate(glm::vec4(0.f, 1.f, 0.f, 90.f)),
								modelStack.Scale(glm::vec3(.6f)),
							});
								models[(int)ModelType::Pistol]->SetModelForAll(modelStack.GetTopModel());
								models[(int)ModelType::Pistol]->Render(*forwardSP);
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
								models[(int)ModelType::AR]->Render(*forwardSP);
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
								models[(int)ModelType::Sniper]->Render(*forwardSP);
							modelStack.PopModel();
							break;
					}
					forwardSP->Set1i("useCustomDiffuseTexIndex", 1);
					forwardSP->Set1i("useCustomColour", 1);
				modelStack.PopModel();
			}

			forwardSP->Set1i("useCustomDiffuseTexIndex", 0);
			forwardSP->Set1i("useCustomColour", 0);
			forwardSP->Set1i("noNormals", 0);
			forwardSP->Set1i("nightVision", 0);

			///Render text
			if(!scope){
				glDepthFunc(GL_NOTEQUAL);
				str temp;
				if(weapon->GetCurrentWeapon()->GetReloading())
					temp = "Reloading...";
				else{
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
					!(weapon->GetCurrentSlot()) ? "Infinite" : std::to_string(weapon->GetCurrentWeapon()->GetCurrentAmmoRound()) + "/" + std::to_string(weapon->GetCurrentWeapon()->GetCurrentTotalAmmo()),
					float(winWidth) / 1.1f,
					25.f,
					1.f,
					glm::vec4(1.f),
					0
				});
				textChief.RenderText(textSP, {
					"Wave: " + std::to_string(waves[waveCount] + 1),
					25.f,
					125.f,
					1.f,
					glm::vec4(1.f, 1.f, 0.f, 1.f),
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
					"FPS: " + std::to_string(1.f / dt).substr(0, 4),
					25.f,
					25.f,
					1.f,
					glm::vec4(1.f, 1.f, 0.f, 1.f),
					0
				});
				glDepthFunc(GL_LESS);
			}
			break;
		}
		case Screen::Instructions: {
			forwardSP->SetMat4fv("PV", &(projection * view)[0][0]);

			///BG
			modelStack.PushModel({
				modelStack.Scale(glm::vec3(float(winWidth) / 2.f, float(winHeight) / 2.f, 1.f)),
			});
				forwardSP->Set1i("useCustomDiffuseTexIndex", 1);
				forwardSP->Set1i("customDiffuseTexIndex", 5);
				forwardSP->Set1i("noNormals", 1);
				meshes[(int)MeshType::Quad]->SetModel(modelStack.GetTopModel());
				meshes[(int)MeshType::Quad]->Render(*forwardSP);
				forwardSP->Set1i("noNormals", 0);
				forwardSP->Set1i("useCustomDiffuseTexIndex", 0);
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

			textChief.RenderText(textSP, {
				"W - Move Forward",
				25.f,
				725.f,
				1.f,
				glm::vec4(1.f, 1.f, 0.f, 1.f),
				0,
			});

			textChief.RenderText(textSP, {
				"S - Move Backward",
				25.f,
				675.f,
				1.f,
				glm::vec4(1.f, 1.f, 0.f, 1.f),
				0,
			});

			textChief.RenderText(textSP, {
				"A - Move Left",
				25.f,
				625.f,
				1.f,
				glm::vec4(1.f, 1.f, 0.f, 1.f),
				0,
			});

			textChief.RenderText(textSP, {
				"D - Move Right",
				25.f,
				575.f,
				1.f,
				glm::vec4(1.f, 1.f, 0.f, 1.f),
				0,
			});

			textChief.RenderText(textSP, {
				"R - Reload Weapon",
				25.f,
				525.f,
				1.f,
				glm::vec4(1.f, 1.f, 0.f, 1.f),
				0,
			});

			textChief.RenderText(textSP, {
				"Shift - Toggle Sprint",
				25.f,
				475.f,
				1.f,
				glm::vec4(1.f, 1.f, 0.f, 1.f),
				0,
			});

			textChief.RenderText(textSP, {
				"Space - Prone --> Crouch --> Stand --> Jump",
				25.f,
				425.f,
				1.f,
				glm::vec4(1.f, 1.f, 0.f, 1.f),
				0,
			});

			textChief.RenderText(textSP, {
				"C - Stand --> Crouch --> Prone",
				25.f,
				375.f,
				1.f,
				glm::vec4(1.f, 1.f, 0.f, 1.f),
				0,
			});

			textChief.RenderText(textSP, {
				"P - Pause",
				25.f,
				325.f,
				1.f,
				glm::vec4(1.f, 1.f, 0.f, 1.f),
				0,
			});

			textChief.RenderText(textSP, {
				"F1 - Toggle Fullscreen" ,
				25.f,
				275.f,
				1.f,
				glm::vec4(1.f, 1.f, 0.f, 1.f),
				0,
			});

			textChief.RenderText(textSP, {
				"F2 - Change Polygon Mode",
				25.f,
				225.f,
				1.f,
				glm::vec4(1.f, 1.f, 0.f, 1.f),
				0,
			});
			glDepthFunc(GL_LESS);
			break;
		}
		case Screen::Credits: {
			forwardSP->SetMat4fv("PV", &(projection * view)[0][0]);

			///BG
			modelStack.PushModel({
				modelStack.Scale(glm::vec3(float(winWidth) / 2.f, float(winHeight) / 2.f, 1.f)),
			});
				forwardSP->Set1i("useCustomDiffuseTexIndex", 1);
				forwardSP->Set1i("customDiffuseTexIndex", 5);
				forwardSP->Set1i("noNormals", 1);
				meshes[(int)MeshType::Quad]->SetModel(modelStack.GetTopModel());
				meshes[(int)MeshType::Quad]->Render(*forwardSP);
				forwardSP->Set1i("noNormals", 0);
				forwardSP->Set1i("useCustomDiffuseTexIndex", 0);
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

			textChief.RenderText(textSP, {
				"NYP SIDM GDT19",
				25.f, 
				725.f,
				1.f,
				glm::vec4(1.f, 1.f, 0.f, 1.f),
				0,
				});

			textChief.RenderText(textSP, {
				"SP3 Team 01",
				25.f, 
				625.f,
				1.f,
				glm::vec4(1.f, 1.f, 0.f, 1.f),
				0,
				});

			textChief.RenderText(textSP, {
				"Studio Project Siblings (SPS)",
				25.f, 
				525.f,
				1.f,
				glm::vec4(1.f, 1.f, 0.f, 1.f),
				0,
				});

			textChief.RenderText(textSP, {
				"Ling Guan Yu (193541T, Team Leader)",
				25.f,
				475.f,
				1.f,
				glm::vec4(1.f, 1.f, 0.f, 1.f),
				0,
				});

			textChief.RenderText(textSP, {
				"Liu Hao (190597T, Game Programmer)",
				25.f,
				425.f,
				1.f,
				glm::vec4(1.f, 1.f, 0.f, 1.f),
				0,
				});

			textChief.RenderText(textSP, {
				"Lim Li Hui, Trina (193020G, Game Programmer)",
				25.f,
				375.f,
				1.f,
				glm::vec4(1.f, 1.f, 0.f, 1.f),
				0,
				});

			textChief.RenderText(textSP, {
				"Goh Bei Lai (191766P, Game Programmer)",
				25.f,
				325.f,
				1.f,
				glm::vec4(1.f, 1.f, 0.f, 1.f),
				0,
				});
			glDepthFunc(GL_LESS);
			break;
		}
		case Screen::Score: {
			forwardSP->SetMat4fv("PV", &(projection * view)[0][0]);

			///BG
			modelStack.PushModel({
				modelStack.Scale(glm::vec3(float(winWidth) / 2.f, float(winHeight) / 2.f, 1.f)),
			});
				forwardSP->Set1i("useCustomDiffuseTexIndex", 1);
				forwardSP->Set1i("customDiffuseTexIndex", 5);
				forwardSP->Set1i("noNormals", 1);
				meshes[(int)MeshType::Quad]->SetModel(modelStack.GetTopModel());
				meshes[(int)MeshType::Quad]->Render(*forwardSP);
				forwardSP->Set1i("noNormals", 0);
				forwardSP->Set1i("useCustomDiffuseTexIndex", 0);
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
}

void Scene::MinimapRender(const uint& depthDTexRefID, const uint& depthSTexRefID){
	forwardSP->Use();
	forwardSP->Set1f("shininess", 32.f); //More light scattering if lower
	forwardSP->Set3fv("globalAmbient", Light::globalAmbient);
	forwardSP->Set3fv("camPos", cam.GetPos());
	forwardSP->Set1i("pAmt", 0);
	forwardSP->Set1i("dAmt", 0);
	forwardSP->Set1i("sAmt", 0);

	minimapCam.SetPos(glm::vec3(cam.GetPos().x, 500.f, cam.GetPos().z));
	minimapCam.SetTarget(glm::vec3(cam.GetPos().x, 0.f, cam.GetPos().z));
	minimapCam.SetUp(glm::vec3(0.f, 0.f, -1.f));
	minimapView = minimapCam.LookAt();
	minimapProjection = glm::ortho(-float(winWidth) / 5.f, float(winWidth) / 5.f, -float(winHeight) / 5.f, float(winHeight) / 5.f, .1f, 99999.f);
	forwardSP->SetMat4fv("PV", &(minimapProjection * minimapView)[0][0]);
	forwardSP->Set1i("noNormals", 1);

	///Render terrain
	modelStack.PushModel({
		modelStack.Scale(glm::vec3(terrainXScale, terrainYScale, terrainZScale)),
	});
		meshes[(int)MeshType::Terrain]->SetModel(modelStack.GetTopModel());
		meshes[(int)MeshType::Terrain]->Render(*forwardSP);
	modelStack.PopModel();

	///Render self
	modelStack.PushModel({
		modelStack.Translate(minimapCam.GetTarget()),
		modelStack.Scale(glm::vec3(15.f)),
	});
		forwardSP->Set1i("useCustomDiffuseTexIndex", 1);
		forwardSP->Set1i("customDiffuseTexIndex", -1);
		forwardSP->Set1i("useCustomColour", 1);
		forwardSP->Set4fv("customColour", glm::vec4(glm::vec3(1.f), .6f));
		meshes[(int)MeshType::Sphere]->SetModel(modelStack.GetTopModel());
		meshes[(int)MeshType::Sphere]->Render(*forwardSP);
		forwardSP->Set1i("useCustomColour", 0);
		forwardSP->Set1i("useCustomDiffuseTexIndex", 0);
	modelStack.PopModel();

	///Render entities
	EntityManager::RenderParams params;
	params.minimap = true;
	params.camPos = cam.GetPos();
	params.camFront = cam.CalcFront();
	params.depthDTexRefID = depthDTexRefID;
	params.depthSTexRefID = depthSTexRefID;
	params.quadMesh = meshes[(int)MeshType::Quad];
	entityManager->RenderEntities(*forwardSP, params);

	forwardSP->Set1i("noNormals", 0);
}

const Scene::Screen& Scene::GetScreen() const{
	return screen;
}

void Scene::SpawnEntity(const Entity::EntityType& type, Mesh* const& mesh, const float& scaleFactor){
	const float xPos = PseudorandMinMax(-terrainXScale / 2.f + 5.f, terrainXScale / 2.f - 5.f);
	const float zPos = PseudorandMinMax(-terrainZScale / 2.f + 5.f, terrainZScale / 2.f - 5.f);
	const glm::vec3 pos = glm::vec3(xPos, terrainYScale * static_cast<Terrain*>(meshes[(int)MeshType::Terrain])->GetHeightAtPt(xPos / terrainXScale, zPos / terrainZScale) + scaleFactor, zPos);

	Entity* const& entity = entityManager->FetchEntity();
	entity->type = type;
	entity->active = true;
	entity->life = 0.f;
	entity->maxLife = 0.f;
	entity->colour = glm::vec4(1.f);
	entity->diffuseTexIndex = -1;
	entity->rotate = glm::vec4(0.f, 1.f, 0.f, 0.f);
	entity->scale = glm::vec3(scaleFactor);
	entity->light = nullptr;
	entity->mesh = mesh;
	entity->pos = pos;
	entity->vel = glm::vec3(0.f);
	entity->mass = .0001f;
	entity->force = glm::vec3(0.f);

	if(type != Entity::EntityType::AMMO_PICKUP && type != Entity::EntityType::AMMO_PICKUP2){
		ISound* myMusic = soundEngine->play3D("Audio/Music/Spin.mp3", vec3df(pos.x, pos.y, pos.z), true, true, true, ESM_AUTO_DETECT, true);
		if(myMusic){
			myMusic->setMinDistance(2.f);
			myMusic->setVolume(5);
			music.emplace_back(myMusic);
		} else{
			(void)puts("Failed to init music!\n");
		}
	}
}