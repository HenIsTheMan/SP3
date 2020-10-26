#include "ModelStack.h"

ModelStack::ModelStack():
	modelStack()
{
}

glm::mat4 ModelStack::Translate(const glm::vec3& translate){
	return glm::translate(glm::mat4(1.f), translate);
}

glm::mat4 ModelStack::Rotate(const glm::vec4& rotate){
	return glm::rotate(glm::mat4(1.f), glm::radians(rotate.w), glm::vec3(rotate));
}

glm::mat4 ModelStack::Scale(const glm::vec3& scale){
	return glm::scale(glm::mat4(1.f), scale);
}

glm::mat4 ModelStack::GetTopModel() const{
	return modelStack.empty() ? glm::mat4(1.f) : modelStack.top();
}

void ModelStack::PushModel(const std::vector<glm::mat4>& vec) const{
	modelStack.push(modelStack.empty() ? glm::mat4(1.f) : modelStack.top());
	const size_t& size = vec.size();
	for(size_t i = 0; i < size; ++i){
		modelStack.top() *= vec[i];
	}
}

void ModelStack::PopModel() const{
	if(!modelStack.empty()){
		modelStack.pop();
	}
}