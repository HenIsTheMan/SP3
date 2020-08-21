#version 330 core
out vec4 fragColour;

in myInterface{
	vec3 pos;
	vec4 colour;
	vec2 texCoords;
	vec3 normal;
    flat int diffuseTexIndex;
} fsIn;

struct PtLight{ //Positional light source
    vec3 ambient;
    vec3 diffuse;
    vec3 spec;
    vec3 pos;
    float constant; //Constant term //Makes sure the denominator >= 1.f
    float linear; //Linear term //Multiplied with dist to reduce light intensity in a linear fashion
    float quadratic; //Quadratic term //Multiplied with the quadrant of the dist to set quadratic decreases in light intensity //Less significant compared to linear term when dist is small
};

struct DirectionalLight{ //Directional light source
    vec3 ambient;
    vec3 diffuse;
    vec3 spec;
    vec3 dir;
};

struct Spotlight{ //Positional light that shoots light rays in 1 dir, objs within its cut-offs (a certain radius of its dir) are lit
    vec3 ambient;
    vec3 diffuse;
    vec3 spec;
    vec3 pos;
    vec3 dir;
    float cosInnerCutoff;
    float cosOuterCutoff;
};

#define maxAmtP 8
#define maxAmtD 8
#define maxAmtS 8

uniform float shininess;
uniform vec3 globalAmbient;
uniform int pAmt;
uniform int dAmt;
uniform int sAmt;
uniform PtLight ptLights[maxAmtP];
uniform DirectionalLight directionalLights[maxAmtD];
uniform Spotlight spotlights[maxAmtS];

uniform bool water;
uniform sampler2D planarReflectionTex;
uniform samplerCube cubemapSampler;
uniform vec3 camPos;

///Can be set by client
uniform bool useCustomDiffuseTexIndex;
uniform bool useCustomColour;
uniform int customDiffuseTexIndex;
uniform vec4 customColour;

uniform bool useDiffuseMap;
uniform bool useSpecMap;
uniform bool useEmissionMap;
uniform bool useReflectionMap;

uniform sampler2D diffuseMaps[29];
uniform sampler2D specMap;
uniform sampler2D emissionMap;
uniform sampler2D reflectionMap;

vec3 WorldSpacePos = fsIn.pos;
vec4 Colour = fsIn.colour;
vec3 Normal = fsIn.normal;
vec3 Spec = useSpecMap ? texture(specMap, fsIn.texCoords).rgb : vec3(0.f); //Use full white tex for full spec
vec3 Reflection = useReflectionMap ? texture(reflectionMap, fsIn.texCoords).rgb : vec3(0.f); //Use full white tex for full reflection

in vec4 posFromDirectionalLight;
in vec4 posFromSpotlight;
uniform sampler2D dDepthTexSampler;
uniform sampler2D sDepthTexSampler;

vec2 poissonDisk[16] = vec2[](
    vec2(-.94201624f, -.39906216f),
    vec2(.94558609f, -.76890725f),
    vec2(-.094184101f, -.92938870f),
    vec2(.34495938f, .29387760f),
    vec2(-.91588581f, .45771432f),
    vec2(-.81544232f, -.87912464f),
    vec2(-.38277543f, .27676845f),
    vec2(.97484398f, .75648379f),
    vec2(.44323325f, -.97511554f),
    vec2(.53742981f, -.47373420f),
    vec2(-.26496911f, -.41893023f),
    vec2(.79197514f, .19090188f),
    vec2(-.24188840f, .99706507f),
    vec2(-.81409955f, .91437590f),
    vec2(.19984126f, .78641367f),
    vec2(.14383161f, -.14100790f)
);

float randFract(vec3 seed, int i){
    return fract(sin(dot(vec4(seed, i), vec4(12.9898f, 78.233f, 45.164f, 94.673f))) * 43758.5453f);
} 

float NotInShadow(vec3 lightDir, sampler2D shadowMap, vec4 FragPosFromLight){
    vec3 projectedFragCoords = FragPosFromLight.xyz / FragPosFromLight.w;
    projectedFragCoords = projectedFragCoords * .5f + .5f;
    if(projectedFragCoords.z > 1.f){
        return 1.f;
    }

    float currDepth = projectedFragCoords.z;
    float shadowBias = max(.05f * (1.f - dot(Normal, -lightDir)), .005f);

    ///PCF
    float notInShadowSum = 0.f;
    vec2 texelSize = 1.f / textureSize(shadowMap, 0); //Reciprocal of size of tex at mipmap lvl 0
    for(float x = -1.f; x <= 1.f; ++x){
        for(float y = -1.f; y <= 1.f; ++y){
            float closestDepth = texture(shadowMap, projectedFragCoords.xy + vec2(x, y) * texelSize).r;
            notInShadowSum += float(currDepth - shadowBias <= closestDepth);
        }
    }
    float result = notInShadowSum / 9.f; //9 depth samples taken

    ///Poisson-Disk Sampling
    for(int i = 0; i < 16; ++i){
        if(texture2D(shadowMap, projectedFragCoords.xy + poissonDisk[i] / 1000.f).x < currDepth - shadowBias){
            result -= .0625f;
        }
    }
    for(int i = 0; i < 16; ++i){ //Produces noise??
        int index = int(16.f * randFract(gl_FragCoord.xyz, i)) % 16; 
        if(texture2D(shadowMap, projectedFragCoords.xy + poissonDisk[index] / 9999.f).x < currDepth - shadowBias){
            result -= .07f * (gl_FragCoord.x * gl_FragCoord.x + gl_FragCoord.z * gl_FragCoord.z); //??
        }
    }
    return max(result, .37f);
}

vec3 CalcAmbient(vec3 lightAmbient){
    return lightAmbient * Colour.rgb;
}

vec3 CalcDiffuse(vec3 lightDir, vec3 lightDiffuse){
    float dImpact = max(dot(Normal, -lightDir), 0.f); //Diffuse impact of light on curr frag
    return dImpact * lightDiffuse * Colour.rgb; //Diffuse component (> 0.f && <= 1.f when angle between... (>= 0.f && < 90.f) || (> 270.f && <= 360.f)) of frag
}

vec3 CalcSpec(vec3 lightDir, vec3 lightSpec){
    vec3 viewDir = normalize(WorldSpacePos - camPos);
    vec3 halfwayDir = -normalize(lightDir + viewDir);
    float sImpact = pow(max(dot(Normal, halfwayDir), 0.f), shininess);
    return sImpact * lightSpec * Spec;
}

vec3 CalcPtLight(PtLight light){
    vec3 lightDir = normalize(WorldSpacePos - light.pos);
    float dist = length(WorldSpacePos - light.pos);
    float attenuation = 1.f / (light.constant + light.linear * dist + light.quadratic * dist * dist);
    return attenuation * (CalcAmbient(light.ambient) + CalcDiffuse(lightDir, light.diffuse) + CalcSpec(lightDir, light.spec));
}

vec3 CalcDirectionalLight(DirectionalLight light){
    vec3 lightDir = normalize(light.dir);
    return CalcAmbient(light.ambient) + NotInShadow(lightDir, dDepthTexSampler, posFromDirectionalLight) * (CalcDiffuse(lightDir, light.diffuse) + CalcSpec(lightDir, light.spec));
}

vec3 CalcSpotlight(Spotlight light){
    vec3 lightDir = normalize(WorldSpacePos - light.pos);
    float cosTheta = dot(lightDir, normalize(light.dir));
    float epsilon = light.cosInnerCutoff - light.cosOuterCutoff;
    float lightIntensity = clamp((cosTheta - light.cosOuterCutoff) / epsilon, 0.f, 1.f);
    return CalcAmbient(light.ambient) + NotInShadow(lightDir, sDepthTexSampler, posFromSpotlight) * lightIntensity * (CalcDiffuse(lightDir, light.diffuse) + CalcSpec(lightDir, light.spec));
}

void main(){
	if(!useCustomColour && !useDiffuseMap){
		Colour = fsIn.colour;
	} else{
		Colour = (useCustomColour ? customColour : vec4(1.f))
		* ((useDiffuseMap && (useCustomDiffuseTexIndex ? customDiffuseTexIndex : fsIn.diffuseTexIndex) >= 0
		? texture(diffuseMaps[useCustomDiffuseTexIndex ? customDiffuseTexIndex : fsIn.diffuseTexIndex], fsIn.texCoords) : vec4(1.f))
		+ (useEmissionMap && (useCustomDiffuseTexIndex ? customDiffuseTexIndex : fsIn.diffuseTexIndex) >= 0 ? texture(emissionMap, fsIn.texCoords) : vec4(0.f)));
	}

    if(Normal == vec3(0.f)){
        fragColour = Colour;
    } else if(pAmt == 0 && dAmt == 0 && sAmt == 0){
        fragColour = vec4(CalcAmbient(globalAmbient), Colour.a);
    } else{
        fragColour = vec4(vec3(0.f), Colour.a);
        for(int i = 0; i < pAmt; ++i){
            fragColour.rgb += CalcPtLight(ptLights[i]);
        }
        for(int i = 0; i < dAmt; ++i){
            fragColour.rgb += CalcDirectionalLight(directionalLights[i]);
        }
        for(int i = 0; i < sAmt; ++i){
            fragColour.rgb += CalcSpotlight(spotlights[i]);
        }

        if(Reflection != vec3(0.f)){
            const float ratio = 1.f / 1.52f; //n of air / n of glass (ratio between refractive indices of both materials)
            vec3 incidentRay = normalize(WorldSpacePos - camPos);
            vec3 reflectedRay = reflect(incidentRay, Normal);
            vec3 refractedRay = refract(incidentRay, Normal, ratio);
            if(water){
                fragColour.rgb += texture(planarReflectionTex, fsIn.texCoords).rgb * Reflection;
            } else{
                fragColour.rgb += texture(cubemapSampler, reflectedRay).rgb * Reflection;
            }
        }
    }
}