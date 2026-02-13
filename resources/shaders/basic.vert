#version 330 core
layout (location=0) in vec3 aPos;
layout (location=1) in vec3 aNormal;
layout (location=2) in vec2 aTex;
//shaderul principal de varfuri
//pt lumina,  umbra si ceata
uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;
uniform mat4 lightSpaceMatrix;
//fragposlight este pozitia varfului in coordonate din punctul de vedere al luminii

out vec3 FragPos;
out vec3 Normal;
out vec2 TexCoord;
out vec4 FragPosLightSpace;

void main() {
    //luam un vertex si ii calculam pozitia in spatiul lumii, normalala si coordonatele de textura
    //world space fragment position
    FragPos = vec3(model * vec4(aPos, 1.0));
    //transformam normalele corect in spatiul lumii
    Normal  = mat3(transpose(inverse(model))) * aNormal;
    TexCoord = aTex;
    //calculam pozitia varfului in coordonate din punctul de vedere al luminii, si transformam prin lightSpaceMatrix
    FragPosLightSpace = lightSpaceMatrix * vec4(FragPos, 1.0);
    //se calculeaza pozitia finala a varfului in coordonate de ecran si trimit date prin out catre fragment shader
    gl_Position = projection * view * vec4(FragPos, 1.0);
}
