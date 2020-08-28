#pragma once
#include "Mesh.h"

class Terrain final: public Mesh{
public:
	Terrain();
	Terrain(cstr const& fPath, const float& tileH, const float& tileV);
	~Terrain() = default;

	float BarycentricInterpolation(const glm::vec3& pt1, const glm::vec3& pt2, const glm::vec3& pt3, const glm::vec3& pos) const;
	float GetHeightAtPt(const float& x, const float& z, const bool& barycentric = false) const;
	void Render(ShaderProg& SP, const bool& autoConfig = true, const bool& setInstancing = true) override;
private:
	cstr terrainPath;
	float tileH;
	float tileV;
	std::vector<unsigned char> data;

	bool Load();
	void Create();
};