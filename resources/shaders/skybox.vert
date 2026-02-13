#version 330 core
layout (location = 0) in vec3 aPos;

out vec3 TexCoords;

uniform mat4 projection;
uniform mat4 view;

void main()
{

    //pozittile varfurile cubului la infinit
    TexCoords = aPos;
    //view contine translatarea, dar pentru skybox nu vrem sa fie afectat de translatie
    //astfel ca luam doar partea de rotatie a matricii view (mat3(view)) si o convertim inapoi la mat4
    //view nu trebuie sa folosim ca  skybox e pe jurul camerei
    vec4 pos = projection * mat4(mat3(view)) * vec4(aPos, 1.0);
    //setam z la w pentru a plasa skybox-ul la infinit
    gl_Position = pos.xyww;
}
