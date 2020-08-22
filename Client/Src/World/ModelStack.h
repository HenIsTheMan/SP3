#pragma once
#include <Core.h>
#include <Engine.h>
#include "Cam.h"
#include "../Game/EntityManager.h"
#include "../Game/Weapon.h"

class ModelStack final{
public:
	ModelStack();
	~ModelStack() = default;

	glm::mat4 Translate(const glm::vec3& translate);
	glm::mat4 Rotate(const glm::vec4& rotate);
	glm::mat4 Scale(const glm::vec3& scale);
	glm::mat4 GetTopModel() const;
	void PushModel(const std::vector<glm::mat4>& vec) const;
	void PopModel() const;
private:
	mutable std::stack<glm::mat4> modelStack;
};