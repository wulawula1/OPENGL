#version 330
out vec4 Fragcolor;

in vec3 Texcoords;

uniform samplerCube skybox;

void main()
{    
    Fragcolor = texture(skybox, Texcoords);
}