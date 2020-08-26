//#include "SceneManager.h"
//
//SceneManager* SceneManager::instance = 0;
//
//SceneManager::SceneManager(SCENES_TYPES currentScene) : currentSceneID(currentScene)
//{
//
//}
//
//SceneManager* SceneManager::getInstance()
//{
//	if (!instance)
//	{
//		instance = new SceneManager(SCENE_MAIN);
//		instance->scene_list[SCENE_MAIN] = new Scene();
//
//		instance->scene_list[instance->currentSceneID]->Init();
//
//		return instance;
//	}
//	else
//	{
//		return instance;
//	}
//}
//
//void SceneManager::SetNextScene(SCENES_TYPES sceneID)
//{
//	if (sceneID != currentSceneID)
//	{
//		scene_list[currentSceneID]->Exit();
//		currentSceneID = sceneID;
//		scene_list[currentSceneID]->Init();
//	}
//}
//
//void SceneManager::Update(double dt)
//{
//	scene_list[currentSceneID]->Update(dt);
//	scene_list[currentSceneID]->Render();
//}
//
//void SceneManager::DeleteAllScenes()
//{
//	scene_list[currentSceneID]->Exit();
//	for (int i = 0; i < NUM_SCENES; ++i)
//		delete scene_list[i];
//	delete instance;
//}