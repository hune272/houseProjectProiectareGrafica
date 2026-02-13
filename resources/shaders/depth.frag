#version 330 core

in vec2 TexCoord;

uniform sampler2D textureSampler;

void main()
{
    // Alpha test for leaves (discard transparent pixels)
    //copacii au frunze pe care se vede prin ele, unde alpha e mica le disardam sa nu le mai desenam si umbra sa fie corecta
    float alpha = texture(textureSampler, TexCoord).a;
    if (alpha < 0.5) {
        discard;
    }
}
