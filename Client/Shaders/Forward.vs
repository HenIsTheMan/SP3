#version 330 core
layout (location = 0) in vec3 pos;
layout (location = 1) in vec4 colour;
layout (location = 2) in vec2 texCoords;
layout (location = 3) in vec3 normal;
layout (location = 4) in vec3 tangent;
layout (location = 5) in int diffuseTexIndex;
layout (location = 6) in mat4 modelMat;

out myInterface{
	vec3 pos;
	vec4 colour;
	vec2 texCoords;
	vec3 normal;
	flat int diffuseTexIndex;
} vsOut;

out vec4 posFromDirectionalLight;
out vec4 posFromSpotlight;
uniform mat4 directionalLightPV;
uniform mat4 spotlightPV;
uniform mat4 PV;
uniform mat4 model;

uniform bool useBumpMap;
uniform sampler2D bumpMap;

uniform bool water;
uniform float elapsedTime;

uniform bool instancing;
uniform bool noNormals;
uniform bool sky;

void main(){
	vsOut.pos = vec3((instancing ? model * modelMat : model) * vec4(pos, 1.f));
	vsOut.colour = colour;
	vsOut.texCoords = texCoords;
	vsOut.normal = noNormals ? vec3(0.f) : normalize(mat3(transpose(inverse(instancing ? model * modelMat : model))) * (useBumpMap ? texture(bumpMap, texCoords).rgb : normal));
	vsOut.diffuseTexIndex = diffuseTexIndex;
	gl_Position = PV * vec4(vsOut.pos, 1.f);
	posFromDirectionalLight = directionalLightPV * vec4(vsOut.pos, 1.f);
	posFromSpotlight = spotlightPV * vec4(vsOut.pos, 1.f);

	if(sky){
		vsOut.normal = vec3(0.f);
		gl_Position = gl_Position.xyww; //Resulting NDC after perspective division will have a z value (gl_FragCoord.z) equal to 1.f
	}
	if(water){
		vsOut.pos.y += sin(vsOut.pos.x + elapsedTime) * cos(vsOut.pos.z + elapsedTime) * 5.f;
		gl_Position = PV * vec4(vsOut.pos, 1.f);
		//vsOut.texCoords.x += elapsedTime / 100.f;
	}
}