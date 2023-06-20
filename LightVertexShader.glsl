#version 330

layout (location = 0) in vec3 apos;
layout (location = 1) in vec2 atex;
layout (location = 2) in vec3 anorm;

out vec3 fragpos;
out vec3 onorm;
out vec2 texcoord;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

void main()
{
	gl_Position = projection * view * model * vec4(apos, 1.0);
	fragpos = vec3(model * vec4(apos,1.0));
	texcoord = atex;
	onorm = mat3(transpose(inverse(model))) * anorm;
}
