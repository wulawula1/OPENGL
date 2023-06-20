#version 330

out vec4 fragcolor;

in vec3 fragpos;
in vec3 onorm;
in vec2 texcoord;

uniform sampler2D texture1;

void main()
{
	vec4 texcolor = texture(texture1, texcoord);
    fragcolor = texcolor;
}
