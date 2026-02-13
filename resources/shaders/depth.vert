#version 330 core
layout (location = 0) in vec3 aPos;
layout (location = 2) in vec2 aTexCoord;

out vec2 TexCoord;

uniform mat4 lightSpaceMatrix;
uniform mat4 model;
//folosim sa calculcam umberele, adica din punctul de vedere al luminii ce se vede nu are umbrire si ce nu, are umbrire
void main()
{
    TexCoord = aTexCoord;
    //lightSpaceMatrix contine proiectia si view din punctul de vedere al luminii'
    //gl_Position este in coordonate  pe care se vede din punctul de vedere al luminii
    gl_Position = lightSpaceMatrix * model * vec4(aPos, 1.0);
}
