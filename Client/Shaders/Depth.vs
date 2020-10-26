#version 330 core
layout (location = 0) in vec3 pos;
layout (location = 6) in mat4 modelMat;

uniform mat4 model;
uniform mat4 PV;
uniform bool instancing;

void main(){
	gl_Position = PV * (instancing ? model * modelMat : model) * vec4(pos, 1.f);
}