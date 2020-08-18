#pragma once

#include <ASSIMP/Importer.hpp>
#include <ASSIMP/scene.h>
#include <ASSIMP/postprocess.h>
#include <Windows.h>
#include <GLAD/glad.h>
#include <GLFW/glfw3.h>
#include <GLM/glm.hpp>
#include <GLM/gtc/matrix_transform.hpp>
#include <GLM/gtc/type_ptr.hpp>
#include <GLM/gtx/color_space.hpp>
#include <IRRKLANG/irrKlang.h>
#include <ft2build.h>
#include FT_FREETYPE_H

#include <functional>
#include <fstream>
#include <map>
#include <stack>
#include <thread>
#include <unordered_map>
#include <vector>

using namespace irrklang;

#define STR(text) #text

#pragma comment(lib, "assimp-vc142-mtd.lib")
#pragma comment(lib, "freetype.lib")
#pragma comment(lib, "glfw3.lib")
#pragma comment(lib, "irrKlang.lib")

typedef const char* cstr;
typedef unsigned int uint;
typedef std::string str;