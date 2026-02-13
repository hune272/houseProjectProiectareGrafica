#version 330 core
out vec4 FragColor;

in vec3 TexCoords;

uniform sampler2D skybox;

void main()
{
    // Simple spherical mapping based on direction
    //pt skybox facecm spherical mapping prin atan si asin
    vec3 dir = normalize(TexCoords);
    vec2 uv;
    //pe care aplicam o textura 2D
    //atan imi da un ungii in planul XZ
    //asin imi da un unghi in planul Y
    uv.x = atan(dir.z, dir.x) / (2.0 * 3.14159) + 0.5;
    uv.y = asin(dir.y) / 3.14159 + 0.5;

    // Sample texture
    vec3 color = texture(skybox, uv).rgb;

    // Slightly brighten
    //setam cat de luminozitate sa aiba cerul
    color *= 1.2;

    // Gamma correction
    color = pow(color, vec3(1.0/2.2));

    FragColor = vec4(color, 1.0);
}
