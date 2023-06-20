#version 330

out vec4 flagcolor;

struct Material{
	sampler2D diffusetexture;
	vec3 specular;
	float shiny;
};

struct Dirlight{
	vec3 direction;
	float intensity;

	vec3 ambient;
	vec3 diffuse;
	vec3 specular;
};

struct Pointlight{
	vec3 position;

	float constant;
	float linear;
	float quadratic;

	vec3 ambient;
	vec3 diffuse;
	vec3 specular;
};

struct Spotlight {
    vec3 position;
    vec3 direction;
    float cutoff;
    float outercutoff;
  
    float constant;
    float linear;
    float quadratic;
  
    vec3 ambient;
    vec3 diffuse;
    vec3 specular;       
};

in vec3 fragpos;
in vec3 onorm;
in vec2 texcoord;

#define Np 4

uniform vec3 viewpos;
uniform Material m;
uniform Dirlight dlight;
uniform Pointlight plight[Np];
uniform Spotlight slight;

uniform sampler2D NormalMap;
uniform int normalmap_flag;


vec3 finddlight(Dirlight ddlight,vec3 norm,vec3 viewdir){
	vec3 lightdir = normalize(-ddlight.direction);
	float diff = max(dot(norm,lightdir),0.0);
	vec3 reflectdir = reflect(-lightdir, norm);
	float spec = pow(max(dot(reflectdir,viewdir),0.0), m.shiny);
	vec3 ambient = ddlight.ambient * vec3(texture(m.diffusetexture, texcoord));
    vec3 diffuse = ddlight.diffuse * diff * vec3(texture(m.diffusetexture, texcoord));
    vec3 specular = ddlight.specular * spec * m.specular;
	ambient *= ddlight.intensity;
	diffuse *= ddlight.intensity;
	specular *= ddlight.intensity;
    return (ambient + (diffuse + specular));
}

vec3 findplight(Pointlight pplight,vec3 norm,vec3 viewdir){
	vec3 lightdir = normalize(pplight.position - fragpos);
	float diff = max(dot(norm,lightdir),0.0);
	vec3 reflectdir = reflect(-lightdir, norm);
	float spec = pow(max(dot(reflectdir,viewdir),0.0), m.shiny);
	
	float distance = length(pplight.position - fragpos);
    float attenuation = 1.0 / (pplight.constant + pplight.linear * distance + pplight.quadratic * (distance * distance));    

	vec3 ambient = pplight.ambient * vec3(texture(m.diffusetexture, texcoord));
    vec3 diffuse = pplight.diffuse * diff * vec3(texture(m.diffusetexture, texcoord));
    vec3 specular = pplight.specular * spec * m.specular;
	ambient *= attenuation;
	diffuse *= attenuation;
	specular *= attenuation;
    return (ambient + diffuse + specular);
}

vec3 findslight(Spotlight sslight,vec3 norm,vec3 viewdir){
	vec3 lightdir = normalize(sslight.position - fragpos);
	float diff = max(dot(norm,lightdir),0.0);
	vec3 reflectdir = reflect(-lightdir, norm);
	float spec = pow(max(dot(reflectdir,viewdir),0.0), m.shiny);
	
	float distance = length(sslight.position - fragpos);
    float attenuation = 1.0 / (sslight.constant + sslight.linear * distance + sslight.quadratic * (distance * distance));
	float theta = dot(lightdir, normalize(-sslight.direction)); 
    float epsilon = sslight.cutoff - sslight.outercutoff;
    float intensity = clamp((theta - sslight.outercutoff) / epsilon, 0.0, 1.0);

	vec3 ambient = sslight.ambient * vec3(texture(m.diffusetexture, texcoord));
    vec3 diffuse = sslight.diffuse * diff * vec3(texture(m.diffusetexture, texcoord));
    vec3 specular = sslight.specular * spec * m.specular;
	ambient *= attenuation * intensity;
	diffuse *= attenuation * intensity;
	specular *= attenuation * intensity;
	return (ambient + diffuse + specular);
}

void main()
{
	vec3 norm = normalize(onorm);
	if (normalmap_flag==1)
	{
		norm = texture(NormalMap,texcoord).rgb;
		norm = normalize(norm * 2.0 - 1.0);
	}
	vec3 viewdir = normalize(viewpos - fragpos);
	vec3 result = finddlight(dlight,norm,viewdir);
	for (int i=0;i<Np;i++)
		result += findplight(plight[i],norm,viewdir);
	result += findslight(slight, norm, viewdir);
	flagcolor = vec4(result,1.0);
}
