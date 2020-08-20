#pragma once
#include "Mesh.h"

class Water final: public Mesh{
public:
	Water();
	Water(const float& quadSize, const float& hTile, const float& vTile, const float& opacity);
	~Water() = default;

	void Render(ShaderProg& SP, const bool& autoConfig = true) override;
private:
	float quadSize;
	float hTile;
	float vTile;

	void Create();
};