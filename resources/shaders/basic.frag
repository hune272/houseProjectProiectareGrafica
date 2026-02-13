#version 330 core
out vec4 FragColor;
//fragement shader calculeaza culoarea fiecarui pixel sau alte atribute pe care le dorim, de exemplu adancimea pt shadow mapping
in vec3 FragPos;
in vec3 Normal;
in vec2 TexCoord;
in vec4 FragPosLightSpace;

uniform vec3 viewPos;
uniform sampler2D textureSampler;
uniform sampler2D shadowMap;

uniform vec3 dirLightDir;
uniform vec3 dirLightColor;

uniform vec3 pointLightPos;
uniform vec3 pointLightColor;

uniform bool fogEnabled;
uniform float fogDensity;
uniform vec3 fogColor;

// Shadow calculation with PCF (Percentage Closer Filtering)
float ShadowCalculation(vec4 fragPosLightSpace, vec3 normal, vec3 lightDir)
{
    // Perspective divide
    vec3 projCoords = fragPosLightSpace.xyz / fragPosLightSpace.w;

    // Transform to [0,1] range
    projCoords = projCoords * 0.5 + 0.5;

    // Get closest depth value from light's perspective
    float closestDepth = texture(shadowMap, projCoords.xy).r;

    // Get depth of current fragment from light's perspective
    float currentDepth = projCoords.z;

    // Bias to prevent shadow acne (reduced for better small object shadows)
    float bias = max(0.002 * (1.0 - dot(normal, lightDir)), 0.0002);

    // PCF (Percentage Closer Filtering) for softer shadows
    float shadow = 0.0;
    vec2 texelSize = 1.0 / textureSize(shadowMap, 0);
    for(int x = -1; x <= 1; ++x)
    {
        for(int y = -1; y <= 1; ++y)
        {
            float pcfDepth = texture(shadowMap, projCoords.xy + vec2(x, y) * texelSize).r;
            shadow += currentDepth - bias > pcfDepth ? 1.0 : 0.0;
        }
    }
    shadow /= 9.0;

    // Keep the shadow at 0.0 when outside the far_plane region of the light's frustum.
    if(projCoords.z > 1.0)
        shadow = 0.0;

    return shadow;
}

void main() {
    vec3 n = normalize(Normal);
    //citim textura
    vec4 texColor4 = texture(textureSampler, TexCoord);
    vec3 texColor = texColor4.rgb;
    //aplha discard pt umbrele la frunze
    // Alpha test for leaves (discard transparent pixels)
    if (texColor4.a < 0.5) {
        discard;
    }
    //lumina mai precis, daca oare normala obiectului e orientata spre camera sau nu
    vec3 viewDir = normalize(viewPos - FragPos);

    // Two-sided lighting: flip normal if facing away from camera
    if (dot(n, viewDir) < 0.0) {
        n = -n;
    }
    //daca nu folosim ce nu este luminat ar fi negru
    // Enhanced ambient lighting (simulates indirect light)
    vec3 ambient = 0.25 * texColor;
    //0 umbra totala, 1 lumina totala
    // === DIRECTIONAL LIGHT (Sun) ===
    vec3 lightDir = normalize(-dirLightDir);

    // Diffuse
    float diffD = max(dot(n, lightDir), 0.0);
    vec3 diffuse = diffD * dirLightColor * texColor;

    // Specular (Blinn-Phong)
    vec3 halfwayDir = normalize(lightDir + viewDir);
    float spec = pow(max(dot(n, halfwayDir), 0.0), 32.0);
    vec3 specular = spec * dirLightColor * 0.3; // Subtle specular

    // Shadow din perspectiva luminii
    float shadow = ShadowCalculation(FragPosLightSpace, n, lightDir);
    vec3 dirLighting = (1.0 - shadow * 0.85) * (diffuse + specular);

    // === POINT LIGHT (Interior) ===
    vec3 l = pointLightPos - FragPos;
    float dist = length(l);
    vec3 ldirP = normalize(l);

    // Diffuse
    float diffP = max(dot(n, ldirP), 0.0);

    // Specular
    vec3 halfwayDirP = normalize(ldirP + viewDir);
    float specP = pow(max(dot(n, halfwayDirP), 0.0), 16.0);

    // Attenuation (realistic falloff)
    float att = 1.0 / (1.0 + 0.09*dist + 0.032*dist*dist);
    vec3 pointLighting = (diffP * texColor + specP * 0.2) * pointLightColor * att;

    // Final color
    vec3 result = ambient + dirLighting + pointLighting;

    // Tone mapping (subtle)
    result = result / (result + vec3(0.5));

    // Apply fog if enabled (isInsideHouse is checked in CPU, not here)
    if (fogEnabled) {
        float distance = length(viewPos - FragPos);
        float fogFactor = exp(-fogDensity * distance);
        fogFactor = clamp(fogFactor, 0.0, 1.0);
        result = mix(fogColor, result, fogFactor);
    }
    //scrie culoarea finala in buffer
    FragColor = vec4(result, 1.0);
}
