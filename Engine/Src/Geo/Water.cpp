#include "Water.h"

Water::Water(): Mesh(),
	quadSize(0.f),
	hTile(0.f),
	vTile(0.f)
{
}

Water::Water(const float& quadSize, const float& hTile, const float& vTile, const float& opacity): Mesh(),
	quadSize(quadSize),
	hTile(hTile),
	vTile(vTile)
{
}

void Water::Render(ShaderProg& SP, const bool& autoConfig){
	if(!vertices){
		Create();
	}
	Mesh::Render(SP, autoConfig);
}

void Water::Create(){
	if(!vertices){
		vertices = new std::vector<Vertex>();
		if(indices){
			delete indices;
			indices = nullptr;
		}
		indices = new std::vector<uint>();

		for(uint x = 0; x < (uint)quadSize; ++x){
			for(uint y = 0; y < (uint)quadSize; ++y){
				vertices->push_back({
					glm::vec3(float(x) / quadSize - .5f, float(y) / quadSize - .5f, 0.f),
					glm::vec4(.7f, .4f, .1f, 1.f),
					glm::vec2(float(x) / quadSize, 1.f - float(y) / quadSize),
					glm::vec3(0.f, 0.f, 1.f),
					glm::vec3(0.f), //??
					0,
				});
			}
		}

		for(uint x = 0; x < uint(quadSize - 1.f); ++x){
			for(uint y = 0; y < uint(quadSize - 1.f); ++y){
				///Triangle 1
				indices->emplace_back(uint(quadSize * y + x));
				indices->emplace_back(uint(quadSize * (y + 1) + x));
				indices->emplace_back(uint(quadSize * y + x + 1));

				///Triangle 2
				indices->emplace_back(uint(quadSize * (y + 1) + x + 1));
				indices->emplace_back(uint(quadSize * y + x + 1));
				indices->emplace_back(uint(quadSize * (y + 1) + x));
			}
		}
	}
}