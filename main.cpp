#include <GL/glew.h>
#include <GLFW/glfw3.h>

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <iostream>
#include <fstream>
#include <sstream>
#include <string>

#include "ObjModel.h"
#include "DebugRenderer.h"
//declararea obiectelor
ObjModel groundObj;
ObjModel houseObj;
ObjModel interiorObj;
ObjModel floorObj;
ObjModel roofObj;
ObjModel doorNewObj;
ObjModel doorNew2Obj;
ObjModel sofaObj;
ObjModel lampObj;
ObjModel tableObj;
ObjModel treeObj;

static std::string readFile(const char* path) {
    std::ifstream f(path);
    if (!f.is_open()) {
        std::cerr << "Cannot open file: " << path << "\n";
        return "";
    }
    std::stringstream ss;
    ss << f.rdbuf();
    return ss.str();
}
//compilare shader
static GLuint compileShader(GLenum type, const char* src) {
    GLuint sh = glCreateShader(type);
    glShaderSource(sh, 1, &src, nullptr);
    glCompileShader(sh);
    //daca nu a mers compilarea afiseaza eroarea
    GLint ok;
    glGetShaderiv(sh, GL_COMPILE_STATUS, &ok);
    if (!ok) {
        char log[1024];
        glGetShaderInfoLog(sh, 1024, nullptr, log);
        std::cerr << "Shader compile error:\n" << log << "\n";
    }
    return sh;
}
//creare program shader
static GLuint createProgram(const char* vp, const char* fp) {
    std::string vStr = readFile(vp);
    std::string fStr = readFile(fp);
    //compilare vertex si fragment shader
    GLuint vs = compileShader(GL_VERTEX_SHADER, vStr.c_str());
    GLuint fs = compileShader(GL_FRAGMENT_SHADER, fStr.c_str());

    GLuint prog = glCreateProgram();
    glAttachShader(prog, vs);
    glAttachShader(prog, fs);
    glLinkProgram(prog);

    GLint ok;
    glGetProgramiv(prog, GL_LINK_STATUS, &ok);
    if (!ok) {
        char log[1024];
        glGetProgramInfoLog(prog, 1024, nullptr, log);
        std::cerr << "Program link error:\n" << log << "\n";
    }
    //sterge shaderele dupa linkare
    glDeleteShader(vs);
    glDeleteShader(fs);
    return prog;
}
//incarcare textura din fisier
static GLuint loadTexture(const std::string& path) {
    //incarca imaginea folosind stb_image
    int width, height, channels;
    stbi_set_flip_vertically_on_load(true);
    //cititm textura prin bytes si configuram width, height, channels pentru openGL
    unsigned char* data = stbi_load(path.c_str(), &width, &height, &channels, 0);

    if (!data) {
        std::cerr << "Failed to load texture: " << path << "\n";
        return 0;
    }
    //determinare format textura
    GLenum format = (channels == 4) ? GL_RGBA : GL_RGB;
    //genereaza si configureaza textura OpenGL
    GLuint textureID;
    glGenTextures(1, &textureID);
    glBindTexture(GL_TEXTURE_2D, textureID);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, data);
    glGenerateMipmap(GL_TEXTURE_2D);
    //dam free la memoria imaginii
    stbi_image_free(data);
    std::cout << "Loaded texture: " << path << " (" << width << "x" << height << ", " << channels << " channels)\n";
    return textureID;
}
//varfurile cubului pentru skybox
static float skyboxVertices[] = {
    -1.0f,  1.0f, -1.0f,  -1.0f, -1.0f, -1.0f,   1.0f, -1.0f, -1.0f,
     1.0f, -1.0f, -1.0f,   1.0f,  1.0f, -1.0f,  -1.0f,  1.0f, -1.0f,
    -1.0f, -1.0f,  1.0f,  -1.0f, -1.0f, -1.0f,  -1.0f,  1.0f, -1.0f,
    -1.0f,  1.0f, -1.0f,  -1.0f,  1.0f,  1.0f,  -1.0f, -1.0f,  1.0f,
     1.0f, -1.0f, -1.0f,   1.0f, -1.0f,  1.0f,   1.0f,  1.0f,  1.0f,
     1.0f,  1.0f,  1.0f,   1.0f,  1.0f, -1.0f,   1.0f, -1.0f, -1.0f,
    -1.0f, -1.0f,  1.0f,  -1.0f,  1.0f,  1.0f,   1.0f,  1.0f,  1.0f,
     1.0f,  1.0f,  1.0f,   1.0f, -1.0f,  1.0f,  -1.0f, -1.0f,  1.0f,
    -1.0f,  1.0f, -1.0f,   1.0f,  1.0f, -1.0f,   1.0f,  1.0f,  1.0f,
     1.0f,  1.0f,  1.0f,  -1.0f,  1.0f,  1.0f,  -1.0f,  1.0f, -1.0f,
    -1.0f, -1.0f, -1.0f,  -1.0f, -1.0f,  1.0f,   1.0f, -1.0f, -1.0f,
     1.0f, -1.0f, -1.0f,  -1.0f, -1.0f,  1.0f,   1.0f, -1.0f,  1.0f
};
//creare VAO si VBO pentru skybox
static void createSkyboxCube(GLuint& vao, GLuint& vbo) {
    glGenVertexArrays(1, &vao);
    glGenBuffers(1, &vbo);
    glBindVertexArray(vao);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(skyboxVertices), &skyboxVertices, GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), nullptr);
    glBindVertexArray(0);
}
//variabile globale pentru camera, timp, usi, fizica, lampa, canapea, debug
static glm::vec3 camPos(0.0f, 1.8f, 6.0f);
static glm::vec3 camFront(0.0f, 0.0f, -1.0f);
static glm::vec3 camUp(0.0f, 1.0f, 0.0f);
static float yaw = -90.0f;
static float pitch = 0.0f;
static float fov = 60.0f;
static bool firstMouse = true;
static float lastX = 640, lastY = 360;
//pt framerui sa fie limitat
static float deltaTime = 0.0f;
static float lastFrame = 0.0f;
//variabile pentru usi
static bool door1Open = false;
static bool door2Open = false;
static bool doorTogglePressed = false;
static float door1Angle = 0.0f;
static float door2Angle = 0.0f;
static float door1TargetAngle = 0.0f;
static float door2TargetAngle = 0.0f;
static const float maxDoorAngle = 70.0f;
static const float doorSpeed = 120.0f;
static const float doorProximity = 2.0f;
//variabile pentru fizica
static float verticalVel = 0.0f;
static const float gravity = -9.81f;
static const float eyeHeight = 1.5f;
static float floorY = 0.5f;
static bool physicsEnabled = false;
static bool jumpPressed = false;
//variabile pentru lampa
static bool lampLightOn = false;
static bool fogEnabled = false;
static bool fPressed = false;
static bool lampTogglePressed = false;
static const float lampProximity = 3.0f;
static const glm::vec3 lampPosition(1.5f, 0.5f, 0.0f);
//variabile pentru canapea
static bool sofaEditMode = false;
static bool mPressed = false;
static glm::vec3 sofaPosition(3.0f, 0.0f, 1.0f);
static float sofaRotation = 180.0f;
static float sofaMovementSpeed = 0.1f;

static bool debugMode = false;
//poligoanele pentru podea si panta, de exemplu floor si stairs
static const std::vector<glm::vec2> floorPoly = {
    { -0.81f,  6.7f },
    { -0.80f,  4.4f },
    {  5.70f,  4.4f },
    {  5.70f, -6.0f },
    { -5.40f, -6.0f },
    { -5.40f,  6.7f }
};

static const std::vector<glm::vec2> floorPoly25 = {
    { -15.0f, -15.0f },
    {  15.0f, -15.0f },
    {  15.0f,  15.0f },
    { -15.0f,  15.0f }
};
static float floorY25 = 0.0f;
static const std::vector<glm::vec3> slopePoints3D = {
    { -0.8f, 0.4f, 6.7f },  // point 1 (higher)
    { -0.3f, 0.0f, 6.7f },  // point 2 (lower)
    { -0.3f, 0.0f, 4.4f },  // point 3 (lower)
    { -0.8f, 0.4f, 4.5f }   // point 4 (higher)
};
static const std::vector<glm::vec2> slopePoly = {
    { -0.8f, 6.7f },
    { -2.1f, 6.7f },
    { -0.3f, 4.5f },
    { -0.8f, 4.5f }
};
static const std::vector<glm::vec3> slopePoints3D2 = {
    {  1.8f, 0.5f, -6.0f },  // point 1 (higher)
    {  1.8f, 0.0f, -6.9f },  // point 2 (lower)
    { -1.4f, 0.0f, -6.9f },  // point 3 (lower)
    { -1.4f, 0.5f, -6.0f }   // point 4 (higher)
};
static const std::vector<glm::vec2> slopePoly2 = {
    {  1.8f, -6.0f },
    {  1.8f, -6.9f },
    { -1.4f, -6.9f },
    { -1.4f, -6.0f }
};
//calculam noile directii ale camerei in functie de yaw si pitch
static void updateCameraVectors() {
    glm::vec3 front;
    front.x = cos(glm::radians(yaw)) * cos(glm::radians(pitch));
    front.y = sin(glm::radians(pitch));
    front.z = sin(glm::radians(yaw)) * cos(glm::radians(pitch));
    //normalizam vectorul
    camFront = glm::normalize(front);
}
//callback pentru redimensionare fereastra
static void framebuffer_size_callback(GLFWwindow*, int w, int h) {
    glViewport(0, 0, w, h);
}
//callback pentru miscare mouse
static void mouse_callback(GLFWwindow*, double xpos, double ypos) {
    if (firstMouse) {
        lastX = (float)xpos;
        lastY = (float)ypos;
        firstMouse = false;
    }

    float xoff = (float)xpos - lastX;
    float yoff = lastY - (float)ypos;
    lastX = (float)xpos;
    lastY = (float)ypos;

    float sens = 0.10f;
    xoff *= sens;
    yoff *= sens;

    yaw += xoff;
    pitch += yoff;

    if (pitch > 89.0f) pitch = 89.0f;
    if (pitch < -89.0f) pitch = -89.0f;

    updateCameraVectors();
}
//verifica daca un punct este in interiorul unui poligon 2D, pt  colizuni, sa nu cadem prin podea
//
static bool pointInPolygon(const glm::vec2& p, const std::vector<glm::vec2>& poly) {
    bool inside = false;
    for (size_t i = 0, j = poly.size() - 1; i < poly.size(); j = i++) {
        const glm::vec2& a = poly[i];
        const glm::vec2& b = poly[j];
        //verifica daca linia dintre punct si infinit intersecteaza latura poligonului
        bool intersect =
            ((a.y > p.y) != (b.y > p.y)) &&
            (p.x < (b.x - a.x) * (p.y - a.y) / (b.y - a.y) + a.x);
        if (intersect) inside = !inside;
    }
    return inside;
}
//calculeaza inaltimea pe panta la o pozitie XZ data
static float getSlopeHeight(const glm::vec2& posXZ, const std::vector<glm::vec3>& slopePoints) {
    if (slopePoints.size() != 4) return 0.0f;
    //verificam inaltimea folosind primii 3 puncte pentru a defini planul pantei
    glm::vec3 p0 = slopePoints[0];
    glm::vec3 p1 = slopePoints[1];
    glm::vec3 p2 = slopePoints[2];

    glm::vec3 v1 = p1 - p0;
    glm::vec3 v2 = p2 - p0;
    glm::vec3 normal = glm::cross(v1, v2);
    float d = -glm::dot(normal, p0);

    if (fabs(normal.y) < 0.001f) return 0.0f;

    float heightOnSlope = -(normal.x * posXZ.x + normal.z * posXZ.y + d) / normal.y;
    return heightOnSlope;
}
//verifica coliziunea cu panta si corecteaza pozitia daca este necesar
static bool checkSlopeCollision(const glm::vec3& pos, glm::vec3& correctedPos,
                               const std::vector<glm::vec2>& slopePoly,
                               const std::vector<glm::vec3>& slopePoints) {
    glm::vec2 posXZ(pos.x, pos.z);
    if (!pointInPolygon(posXZ, slopePoly)) {
        return false;
    }
    //daca suntem in interiorul poligonului pantei, calculam inaltimea pantei la pozitia XZ
    //daca picioarele sunt sub inaltimea pantei, corectam pozitia pe Y
    float slopeHeight = getSlopeHeight(posXZ, slopePoints);
    float feetY = pos.y - eyeHeight;
    float tolerance = 1.0f;
    //aici verificam daca picioarele sunt sub inaltimea pantei plus o toleranta
    if (feetY < slopeHeight + tolerance) {
        correctedPos = pos;
        correctedPos.y = slopeHeight + eyeHeight;
        return true;
    }

    return false;
}

//verifica coliziunea cu un perete definit prin 4 colturi
static bool checkWallQuadCollision(const glm::vec3& pos, const std::vector<glm::vec3>& wallCorners, float thickness = 0.15f) {
    if (wallCorners.size() != 4) return false;
    //daca pozitia este in interiorul axelor AABB extinse cu grosimea peretelui, atunci este coliziune
    float minX = std::min({wallCorners[0].x, wallCorners[1].x, wallCorners[2].x, wallCorners[3].x});
    float maxX = std::max({wallCorners[0].x, wallCorners[1].x, wallCorners[2].x, wallCorners[3].x});
    float minY = std::min({wallCorners[0].y, wallCorners[1].y, wallCorners[2].y, wallCorners[3].y});
    float maxY = std::max({wallCorners[0].y, wallCorners[1].y, wallCorners[2].y, wallCorners[3].y});
    float minZ = std::min({wallCorners[0].z, wallCorners[1].z, wallCorners[2].z, wallCorners[3].z});
    float maxZ = std::max({wallCorners[0].z, wallCorners[1].z, wallCorners[2].z, wallCorners[3].z});

    return (pos.x >= minX - thickness && pos.x <= maxX + thickness &&
            pos.y >= minY - thickness && pos.y <= maxY + thickness &&
            pos.z >= minZ - thickness && pos.z <= maxZ + thickness);
}
//verifica coliziunea cu peretii interiori ai casei
static bool checkInteriorWallsCollision(const glm::vec3& pos,
                                       const std::vector<glm::vec3>& door1Corners,
                                       const std::vector<glm::vec3>& door2Corners) {
    //peretele interioare si exterioare ale casei
    std::vector<std::vector<glm::vec3>> walls = {
        {
            glm::vec3(-0.21f, 0.0f, 6.7f),
            glm::vec3(-5.4f, 0.0f, 6.7f),
            glm::vec3(-5.4f, 3.0f, 6.7f),
            glm::vec3(-2.3f, 3.0f, 6.7f)
        },
        {
            glm::vec3(-2.3f, 0.0f, 4.4f),
            glm::vec3(-2.3f, 0.0f, 6.7f),
            glm::vec3(-2.3f, 0.5f, 6.7f),
            glm::vec3(-2.3f, 0.5f, 4.4f)
        },
        {
            glm::vec3(-2.3f, 0.5f, 5.6f),
            glm::vec3(-2.3f, 0.5f, 6.7f),
            glm::vec3(-2.3f, 3.0f, 6.7f),
            glm::vec3(-2.3f, 3.0f, 5.6f)
        },
        {
            glm::vec3(-2.3f, 0.5f, 4.4f),
            glm::vec3(-2.3f, 0.5f, 4.8f),
            glm::vec3(-2.3f, 3.0f, 4.8f),
            glm::vec3(-2.3f, 3.0f, 4.4f)
        },
        {
            glm::vec3(-2.3f, 2.29f, 4.8f),
            glm::vec3(-2.3f, 2.29f, 5.6f),
            glm::vec3(-2.3f, 3.0f, 5.6f),
            glm::vec3(-2.3f, 3.0f, 4.8f)
        },
        {
            glm::vec3(-2.3f, 3.0f, 4.4f),
            glm::vec3(-2.3f, 0.5f, 4.4f),
            glm::vec3(5.7f, 0.5f, 4.4f),
            glm::vec3(5.7f, 3.0f, 4.4f)
        },
        {
            glm::vec3(5.7f, 3.0f, 4.4f),
            glm::vec3(5.7f, 0.5f, 4.4f),
            glm::vec3(5.7f, 0.5f, -6.0f),
            glm::vec3(5.7f, 3.0f, -3.7f)
        },
        {
            glm::vec3(5.7f, 0.5f, -3.7f),
            glm::vec3(1.0f, 0.5f, -3.7f),
            glm::vec3(1.0f, 3.0f, -3.7f),
            glm::vec3(5.7f, 3.0f, -3.7f)
        },
        {
            glm::vec3(0.2f, 0.5f, -3.7f),
            glm::vec3(-5.4f, 0.5f, -3.7f),
            glm::vec3(-5.4f, 3.0f, -3.7f),
            glm::vec3(0.2f, 3.0f, -3.7f)
        },
        {
            glm::vec3(0.2f, 2.28f, -3.7f),
            glm::vec3(1.0f, 2.28f, -3.7f),
            glm::vec3(1.0f, 3.0f, -3.7f),
            glm::vec3(0.2f, 3.0f, -3.7f)
        },
        {
            glm::vec3(5.7f, 0.5f, -6.0f),
            glm::vec3(1.8f, 0.5f, -6.0f),
            glm::vec3(1.8f, 3.0f, -6.0f),
            glm::vec3(5.7f, 3.0f, -6.0f)
        },
        {
            glm::vec3(-1.4f, 0.5f, -6.0f),
            glm::vec3(-5.4f, 0.5f, -6.0f),
            glm::vec3(-5.4f, 3.0f, -6.0f),
            glm::vec3(-1.4f, 3.0f, -6.0f)
        },
        {
            glm::vec3(-5.4f, 0.5f, -6.0f),
            glm::vec3(-5.4f, 0.5f, 6.7f),
            glm::vec3(-5.4f, 3.0f, 6.7f),
            glm::vec3(-5.4f, 3.0f, -6.0f)
        },
        {
            glm::vec3(-4.8f, 0.5f, 6.4f),
            glm::vec3(-4.8f, 0.5f, -3.4f),
            glm::vec3(-4.8f, 3.0f, -3.4f),
            glm::vec3(-4.8f, 3.0f, 6.4f)
        }
    };
    //adauga colturile usilor pentru coliziune
    for (size_t i = 0; i < walls.size(); ++i) {
        if (checkWallQuadCollision(pos, walls[i])) {
            static int debugCounter = 0;
            if (debugCounter++ % 30 == 0) {
                std::cout << "Wall collision: Wall #" << i << " at pos ("
                          << pos.x << ", " << pos.y << ", " << pos.z << ")\n";
            }
            return true;
        }
    }

    return false;
}
//verifica coliziunea cu o usa care se poate roti
static bool checkDoorCollision(const glm::vec3& pos, const std::vector<glm::vec3>& baseCorners, float doorAngle) {
    if (fabs(doorAngle) > 45.0f) return false;
    //dupa 45 de grade nu mai exista coliziunea usii
    glm::vec3 hingePos = baseCorners[0];
    hingePos.y = (baseCorners[0].y + baseCorners[1].y) / 2.0f;
    //calculeaza pozitiile colturilor usii dupa rotatie
    std::vector<glm::vec3> rotatedCorners;
    float angleRad = glm::radians(doorAngle);
    glm::mat4 rotation = glm::rotate(glm::mat4(1.0f), angleRad, glm::vec3(0, 1, 0));

    for (const auto& corner : baseCorners) {
        glm::vec3 localPos = corner - hingePos;
        glm::vec4 rotated = rotation * glm::vec4(localPos, 1.0f);
        glm::vec3 worldPos = glm::vec3(rotated) + hingePos;
        rotatedCorners.push_back(worldPos);
    }

    float minY = rotatedCorners[0].y;
    float maxY = rotatedCorners[1].y;
    if (pos.y < minY || pos.y > maxY) return false;

    float minX = std::min({rotatedCorners[0].x, rotatedCorners[1].x, rotatedCorners[2].x, rotatedCorners[3].x});
    float maxX = std::max({rotatedCorners[0].x, rotatedCorners[1].x, rotatedCorners[2].x, rotatedCorners[3].x});
    float minZ = std::min({rotatedCorners[0].z, rotatedCorners[1].z, rotatedCorners[2].z, rotatedCorners[3].z});
    float maxZ = std::max({rotatedCorners[0].z, rotatedCorners[1].z, rotatedCorners[2].z, rotatedCorners[3].z});

    float thickness = 0.2f;
    return (pos.x >= minX - thickness && pos.x <= maxX + thickness &&
            pos.z >= minZ - thickness && pos.z <= maxZ + thickness);
}
//callback pentru scroll mouse (zoom sau rotire canapea)
static void scroll_callback(GLFWwindow*, double, double yoff) {
    if (sofaEditMode) {
        sofaRotation += (float)yoff * 5.0f;
    } else {
        fov -= (float)yoff;
        if (fov < 20.0f) fov = 20.0f;
        if (fov > 80.0f) fov = 80.0f;
    }
}

int main() {
    if (!glfwInit()) return -1;
    //initializare fereastra
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    //initializare fereastra
    GLFWwindow* window = glfwCreateWindow(1920,  1080, "House Project", nullptr, nullptr);
    if (!window) return -1;
    //framebuffer si input callbacks
    glfwMakeContextCurrent(window);
    glfwSwapInterval(1); // vsync
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
    glfwSetCursorPosCallback(window, mouse_callback);
    glfwSetScrollCallback(window, scroll_callback);
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
    //initializare GLEW
    glewExperimental = GL_TRUE;
    if (glewInit() != GLEW_OK) {
        std::cerr << "Failed to init GLEW\n";
        return -1;
    }

    glEnable(GL_DEPTH_TEST);

    DebugRenderer debugRenderer;
    debugRenderer.init();
    //creare shadere si programe
    GLuint program = createProgram(
        "resources/shaders/basic.vert",
        "resources/shaders/basic.frag"
    );
    //shadere pentru skybox
    GLuint skyboxProgram = createProgram(
        "resources/shaders/skybox.vert",
        "resources/shaders/skybox.frag"
    );
    //incarcare textura skybox
    GLuint skyboxTexture = loadTexture("resources/models/sky/citrus_orchard_puresky.jpg");
    GLuint skyboxVAO, skyboxVBO;
    createSkyboxCube(skyboxVAO, skyboxVBO);
    //shadere pentru shadow mapping
    GLuint depthShader = createProgram(
        "resources/shaders/depth.vert",
        "resources/shaders/depth.frag"
    );
    //framebuffer pentru shadow mapping
    const unsigned int SHADOW_WIDTH = 2048, SHADOW_HEIGHT = 2048;
    GLuint depthMapFBO;
    glGenFramebuffers(1, &depthMapFBO);
    //creare textura pentru depth map
    GLuint depthMap;
    glGenTextures(1, &depthMap);
    glBindTexture(GL_TEXTURE_2D, depthMap);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT, SHADOW_WIDTH, SHADOW_HEIGHT, 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
    float borderColor[] = { 1.0f, 1.0f, 1.0f, 1.0f };
    glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, borderColor);
    //ataseaza textura la framebuffer
    glBindFramebuffer(GL_FRAMEBUFFER, depthMapFBO);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, depthMap, 0);
    glDrawBuffer(GL_NONE);
    glReadBuffer(GL_NONE);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    //incarcare modele
    GLuint groundTexture = loadTexture("resources/models/ground/10450_Rectangular_Grass_Patch_v1_Diffuse.jpg");
    GLuint houseTexture = loadTexture("resources/models/house/Cottage_Clean_Base_Color.png");
    GLuint interiorTexture = loadTexture("resources/models/interior/grey_plaster_03_diff_4k.jpg");
    GLuint floorTexture = loadTexture("resources/models/floor/wood_cabinet_worn_long_diff_4k.jpg");
    GLuint roofTexture = loadTexture("resources/models/ceiling/grey_plaster_03_diff_4k.jpg");

    if (!groundObj.load("resources/models/ground/10450_Rectangular_Grass_Patch_v1_iterations-2.obj")) return -1;
    groundObj.setTexture(groundTexture);

    if (!houseObj.load("resources/models/house/housewwindows.obj")) return -1;
    houseObj.setTexture(houseTexture);

    if (!doorNewObj.load("resources/models/furniture/DoorGoodPos1.obj")) return -1;
    doorNewObj.setTexture(houseTexture);

    if (!doorNew2Obj.load("resources/models/furniture/DoorGoodPos2.obj")) return -1;
    doorNew2Obj.setTexture(houseTexture);

    if (!interiorObj.load("resources/models/interior/wallsFixed.obj")) return -1;
    interiorObj.setTexture(interiorTexture);

    if (!floorObj.load("resources/models/floor/floorFixed.obj")) return -1;
    floorObj.setTexture(floorTexture);

    if (!roofObj.load("resources/models/ceiling/roofFixed.obj")) return -1;
    roofObj.setTexture(roofTexture);

    if (!sofaObj.load("resources/models/furniture/sofa.obj")) return -1;
    sofaObj.setTexture(houseTexture);

    if (!lampObj.load("resources/models/furniture/lamp.obj")) return -1;

    if (!tableObj.load("resources/models/furniture/table.obj")) return -1;

    if (!treeObj.load("resources/models/ground/Hazelnut.obj")) return -1;

    bool wireframe = false;
    bool wirePressed = false;
    //definire colturilor usilor pentru coliziuni
    std::vector<glm::vec3> door1Corners = {
        glm::vec3(-2.4f, 0.501f, 4.8f),
        glm::vec3(-2.4f, 2.29f,  4.8f),
        glm::vec3(-2.4f, 2.29f,  5.69f),
        glm::vec3(-2.4f, 0.501f, 5.69f)
    };

    std::vector<glm::vec3> door2Corners = {
        glm::vec3(0.2f, 0.5f,  -3.6f),
        glm::vec3(0.2f, 2.28f, -3.6f),
        glm::vec3(1.0f, 2.28f, -3.6f),
        glm::vec3(1.0f, 0.5f,  -3.6f)
    };
    //bucle principal
    while (!glfwWindowShouldClose(window)) {
        float now = (float)glfwGetTime();
        deltaTime = now - lastFrame;
        lastFrame = now;
        //limiteaza deltaTime pentru stabilitate si frameuri constante
        const float maxFrameTime = 1.0f / 50.0f;
        if (deltaTime > maxFrameTime) {
            deltaTime = maxFrameTime;
        }

        if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
            glfwSetWindowShouldClose(window, true);
        //toggle wireframe
        bool f2 = glfwGetKey(window, GLFW_KEY_F2) == GLFW_PRESS;
        if (f2 && !wirePressed) {
            wireframe = !wireframe;
            wirePressed = true;
            glPolygonMode(GL_FRONT_AND_BACK, wireframe ? GL_LINE : GL_FILL);
        }
        if (!f2) wirePressed = false;
        //toggle fizica
        static bool gPressed = false;
        bool gKey = glfwGetKey(window, GLFW_KEY_G) == GLFW_PRESS;
        if (gKey && !gPressed) {
            physicsEnabled = !physicsEnabled;
            gPressed = true;
            std::cout << "Physics " << (physicsEnabled ? "ENABLED" : "DISABLED") << "\n";
            if (physicsEnabled) {
                verticalVel = 0.0f;
            }
        }
        if (!gKey) gPressed = false;
        //toggle debug mode
        static bool pPressed = false;
        bool pKey = glfwGetKey(window, GLFW_KEY_P) == GLFW_PRESS;
        if (pKey && !pPressed) {
            debugMode = !debugMode;
            pPressed = true;
            std::cout << "Debug Mode " << (debugMode ? "ENABLED" : "DISABLED") << "\n";
        }
        if (!pKey) pPressed = false;
        //miscare camera
        float speed = 4.0f * deltaTime;
        glm::vec3 right = glm::normalize(glm::cross(camFront, camUp));
        //fizica activata
        if (physicsEnabled) {
            glm::vec3 moveDelta(0.0f);
            if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) moveDelta += camFront * speed;
            if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) moveDelta -= camFront * speed;
            if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS) moveDelta -= right * speed;
            if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS) moveDelta += right * speed;

            moveDelta.y = 0.0f;
            glm::vec3 newPos = camPos + moveDelta;
            glm::vec2 newXZ(newPos.x, newPos.z);

            bool insideFloorL = pointInPolygon(newXZ, floorPoly);
            bool insideFloor25 = pointInPolygon(newXZ, floorPoly25);
            bool insideAnyFloor = insideFloorL || insideFloor25;

            bool doorCollision = checkDoorCollision(newPos, door1Corners, door1Angle) ||
                                checkDoorCollision(newPos, door2Corners, door2Angle);
            bool wallCollision = checkInteriorWallsCollision(newPos, door1Corners, door2Corners);

            if (insideAnyFloor && !doorCollision && !wallCollision) {
                camPos.x = newPos.x;
                camPos.z = newPos.z;
            }
            //dupa ce apare coliziunea cu podeaua, aplicam gravitatia si saritura
            //ca sa nu cade prin podea sau sa urce in aer modificam pozitia pe Y in functie de coliziuni
            //verticalVel este viteza pe Y, care este afectata de gravitatie si saritura
            verticalVel += gravity * deltaTime;
            camPos.y += verticalVel * deltaTime;

            bool hasSupport = false;
            float supportMinY = -1e9f;

            glm::vec3 slopeCorrected = camPos;
            bool onSlope1 = checkSlopeCollision(camPos, slopeCorrected, slopePoly, slopePoints3D);
            bool onSlope2 = checkSlopeCollision(camPos, slopeCorrected, slopePoly2, slopePoints3D2);
            bool onAnySlope = onSlope1 || onSlope2;

            if (onAnySlope) {
                camPos = slopeCorrected;
                verticalVel = 0.0f;
                hasSupport = true;
                supportMinY = camPos.y;
            }

            if (!onAnySlope) {
                glm::vec2 curXZ(camPos.x, camPos.z);
                bool inL  = pointInPolygon(curXZ, floorPoly);
                bool in25 = pointInPolygon(curXZ, floorPoly25);
                float feetY = camPos.y - eyeHeight;

                if (inL) {
                    float y = floorY + eyeHeight;
                    if (feetY >= floorY - 0.2f) {
                        hasSupport = true;
                        supportMinY = y;
                    }
                }

                if (in25) {
                    float y = floorY25 + eyeHeight;
                    if (feetY >= floorY25 - 0.2f) {
                        if (!hasSupport || y > supportMinY) {
                            hasSupport = true;
                            supportMinY = y;
                        }
                    }
                }

                if (hasSupport && camPos.y < supportMinY) {
                    camPos.y = supportMinY;
                    verticalVel = 0.0f;
                }
            }

            bool space = glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS;
            if (space && !jumpPressed) {
                if (hasSupport && fabs(camPos.y - supportMinY) < 0.1f) {
                    verticalVel = 3.5f;
                }
                jumpPressed = true;
            }
            if (!space) jumpPressed = false;

        } else {
            //fizica dezactivata
            glm::vec3 newPos = camPos;

            if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) newPos += camFront * speed;
            if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) newPos -= camFront * speed;
            if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS) newPos -= right * speed;
            if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS) newPos += right * speed;
            if (glfwGetKey(window, GLFW_KEY_Q) == GLFW_PRESS) newPos -= camUp * speed;
            if (glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS) newPos += camUp * speed;

            bool doorCollision = checkDoorCollision(newPos, door1Corners, door1Angle) ||
                                checkDoorCollision(newPos, door2Corners, door2Angle);
            bool wallCollision = checkInteriorWallsCollision(newPos, door1Corners, door2Corners);

            if (!doorCollision && !wallCollision) {
                camPos = newPos;
            }
        }
        //afisare pozitie camera pentru debugging
        static bool kPressed = false;
        if (glfwGetKey(window, GLFW_KEY_K) == GLFW_PRESS && !kPressed) {
            kPressed = true;
            std::cout << "=== CURRENT POSITION ===\n";
            std::cout << "Camera Position: (" << camPos.x << ", " << camPos.y << ", " << camPos.z << ")\n";
            std::cout << "Camera Front: (" << camFront.x << ", " << camFront.y << ", " << camFront.z << ")\n";
            std::cout << "Yaw: " << yaw << ", Pitch: " << pitch << "\n";
            std::cout << "========================\n";
        }
        if (glfwGetKey(window, GLFW_KEY_K) == GLFW_RELEASE) kPressed = false;
        //interactiuni cu usi, lampa, mod editare canapea, ceata
        glm::vec3 door1Pos(-2.41f, 1.41f, 4.89f);
        glm::vec3 door2Pos(0.2f, 1.41f, -3.67f);
        glm::vec3 door1Normal(0.0f, 0.0f, -1.0f);
        glm::vec3 door2Normal(0.0f, 0.0f, 1.0f);
        //pozitia  usii este aproximata ca punctul de mijloc al usii
        float distToDoor1 = glm::length(camPos - door1Pos);
        float distToDoor2 = glm::length(camPos - door2Pos);
        //verifica apasare tasta E pentru deschiderea/inchiderea usii
        bool eKey = glfwGetKey(window, GLFW_KEY_E) == GLFW_PRESS;
        if (eKey && !doorTogglePressed) {
            if (distToDoor1 < doorProximity) {
                door1Open = !door1Open;
                doorTogglePressed = true;

                glm::vec3 doorToPlayer = camPos - door1Pos;
                float side = glm::dot(doorToPlayer, door1Normal);

                if (door1Open) {
                    door1TargetAngle = (side > 0.0f) ? maxDoorAngle : -maxDoorAngle;
                } else {
                    door1TargetAngle = 0.0f;
                }

                std::cout << "Door 1 toggled! Now " << (door1Open ? "OPEN" : "CLOSED")
                          << " (side: " << (side > 0 ? "inside" : "outside")
                          << ", angle: " << door1TargetAngle << "°)\n";

            } else if (distToDoor2 < doorProximity) {
                door2Open = !door2Open;
                doorTogglePressed = true;

                glm::vec3 doorToPlayer = camPos - door2Pos;
                float side = glm::dot(doorToPlayer, door2Normal);

                if (door2Open) {
                    door2TargetAngle = (side > 0.0f) ? maxDoorAngle : -maxDoorAngle;
                } else {
                    door2TargetAngle = 0.0f;
                }

                std::cout << "Door 2 toggled! Now " << (door2Open ? "OPEN" : "CLOSED")
                          << " (side: " << (side > 0 ? "inside" : "outside")
                          << ", angle: " << door2TargetAngle << "°)\n";
            }
        }
        if (!eKey) doorTogglePressed = false;
        //animatie deschidere/inchidere usi
        if (door1Angle < door1TargetAngle) {
            door1Angle += doorSpeed * deltaTime;
            if (door1Angle > door1TargetAngle) door1Angle = door1TargetAngle;
        } else if (door1Angle > door1TargetAngle) {
            door1Angle -= doorSpeed * deltaTime;
            if (door1Angle < door1TargetAngle) door1Angle = door1TargetAngle;
        }

        if (door2Angle < door2TargetAngle) {
            door2Angle += doorSpeed * deltaTime;
            if (door2Angle > door2TargetAngle) door2Angle = door2TargetAngle;
        } else if (door2Angle > door2TargetAngle) {
            door2Angle -= doorSpeed * deltaTime;
            if (door2Angle < door2TargetAngle) door2Angle = door2TargetAngle;
        }
        // interactiune lampa
        float distToLamp = glm::length(camPos - lampPosition);
        bool lKey = glfwGetKey(window, GLFW_KEY_L) == GLFW_PRESS;
        if (lKey && !lampTogglePressed && distToLamp < lampProximity) {
            lampLightOn = !lampLightOn;
            lampTogglePressed = true;
            std::cout << "Lamp light " << (lampLightOn ? "ON" : "OFF") << "\n";
        }
        if (!lKey) lampTogglePressed = false;
        //mod editare canapea
        bool mKey = glfwGetKey(window, GLFW_KEY_M) == GLFW_PRESS;
        if (mKey && !mPressed) {
            sofaEditMode = !sofaEditMode;
            mPressed = true;
            std::cout << "Sofa Edit Mode " << (sofaEditMode ? "ON - Use Arrow Keys to move, Scroll Wheel to rotate" : "OFF") << "\n";
        }
        if (!mKey) mPressed = false;

        if (sofaEditMode) {
            float currentY = sofaPosition.y;

            if (glfwGetKey(window, GLFW_KEY_UP) == GLFW_PRESS) {
                sofaPosition.z -= sofaMovementSpeed;
            }
            if (glfwGetKey(window, GLFW_KEY_DOWN) == GLFW_PRESS) {
                sofaPosition.z += sofaMovementSpeed;
            }
            if (glfwGetKey(window, GLFW_KEY_LEFT) == GLFW_PRESS) {
                sofaPosition.x -= sofaMovementSpeed;
            }
            if (glfwGetKey(window, GLFW_KEY_RIGHT) == GLFW_PRESS) {
                sofaPosition.x += sofaMovementSpeed;
            }

            sofaPosition.y = currentY;
        }
        //toggle ceata
        bool fKey = glfwGetKey(window, GLFW_KEY_F) == GLFW_PRESS;
        if (fKey && !fPressed) {
            fogEnabled = !fogEnabled;
            fPressed = true;
            std::cout << "Fog " << (fogEnabled ? "ON" : "OFF") << "\n";
        }
        if (!fKey) fPressed = false;

        glClearColor(0.08f, 0.10f, 0.12f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        glm::vec3 lightDir = glm::normalize(glm::vec3(-0.2f, -1.0f, -0.3f));
        glm::vec3 lightPos = -lightDir * 20.0f;
        glm::mat4 lightProjection = glm::ortho(-15.0f, 15.0f, -15.0f, 15.0f, 1.0f, 50.0f);
        glm::mat4 lightView = glm::lookAt(lightPos, glm::vec3(0.0f), glm::vec3(0.0f, 1.0f, 0.0f));
        glm::mat4 lightSpaceMatrix = lightProjection * lightView;

        glUseProgram(depthShader);
        glUniformMatrix4fv(glGetUniformLocation(depthShader, "lightSpaceMatrix"), 1, GL_FALSE, &lightSpaceMatrix[0][0]);

        glActiveTexture(GL_TEXTURE0);
        glUniform1i(glGetUniformLocation(depthShader, "textureSampler"), 0);

        glViewport(0, 0, SHADOW_WIDTH, SHADOW_HEIGHT);
        glBindFramebuffer(GL_FRAMEBUFFER, depthMapFBO);
        glClear(GL_DEPTH_BUFFER_BIT);
        //functie lambda pentru randarea scenei in depth map
        auto renderSceneForDepth = [&](GLuint shader) {
            glm::mat4 M = glm::mat4(1.0f);
            glUniformMatrix4fv(glGetUniformLocation(shader, "model"), 1, GL_FALSE, &M[0][0]);
            houseObj.draw();

            M = glm::translate(glm::mat4(1.0f), glm::vec3(-2.41f, 1.41f, 4.89f));
            M = glm::rotate(M, glm::radians(door1Angle), glm::vec3(0, 1, 0));
            glUniformMatrix4fv(glGetUniformLocation(shader, "model"), 1, GL_FALSE, &M[0][0]);
            doorNewObj.draw();

            M = glm::translate(glm::mat4(1.0f), glm::vec3(0.2f, 1.41f, -3.67f));
            M = glm::rotate(M, glm::radians(door2Angle), glm::vec3(0, 1, 0));
            glUniformMatrix4fv(glGetUniformLocation(shader, "model"), 1, GL_FALSE, &M[0][0]);
            doorNew2Obj.draw();

            M = glm::mat4(1.0f);
            glUniformMatrix4fv(glGetUniformLocation(shader, "model"), 1, GL_FALSE, &M[0][0]);
            interiorObj.draw();
            floorObj.draw();
            roofObj.draw();

            M = glm::translate(glm::mat4(1.0f), sofaPosition);
            M = glm::rotate(M, glm::radians(sofaRotation), glm::vec3(0, 1, 0));
            M = glm::scale(M, glm::vec3(1.5f));
            glUniformMatrix4fv(glGetUniformLocation(shader, "model"), 1, GL_FALSE, &M[0][0]);
            sofaObj.draw();

            M = glm::translate(glm::mat4(1.0f), glm::vec3(1.5f, 0.5f, 0.0f));
            M = glm::scale(M, glm::vec3(0.25f));
            glUniformMatrix4fv(glGetUniformLocation(shader, "model"), 1, GL_FALSE, &M[0][0]);
            lampObj.draw();

            M = glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, 0.0f, -1.0f));
            M = glm::rotate(M, glm::radians(-90.0f), glm::vec3(0, 1, 0));
            M = glm::scale(M, glm::vec3(0.1f));
            glUniformMatrix4fv(glGetUniformLocation(shader, "model"), 1, GL_FALSE, &M[0][0]);
            tableObj.draw();

            glEnable(GL_BLEND);
            glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

            M = glm::translate(glm::mat4(1.0f), glm::vec3(-12.0f, -0.7f, 8.0f));
            M = glm::scale(M, glm::vec3(0.8f));
            glUniformMatrix4fv(glGetUniformLocation(shader, "model"), 1, GL_FALSE, &M[0][0]);
            treeObj.draw();

            M = glm::translate(glm::mat4(1.0f), glm::vec3(12.0f, -0.7f, 6.0f));
            M = glm::scale(M, glm::vec3(1.0f));
            glUniformMatrix4fv(glGetUniformLocation(shader, "model"), 1, GL_FALSE, &M[0][0]);
            treeObj.draw();

            M = glm::translate(glm::mat4(1.0f), glm::vec3(10.0f, -0.7f, -12.0f));
            M = glm::scale(M, glm::vec3(0.9f));
            glUniformMatrix4fv(glGetUniformLocation(shader, "model"), 1, GL_FALSE, &M[0][0]);
            treeObj.draw();

            glDisable(GL_BLEND);

            M = glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, -0.7f, 0.0f));
            M = glm::rotate(M, glm::radians(-90.0f), glm::vec3(1, 0, 0));
            M = glm::scale(M, glm::vec3(0.1f));
            glUniformMatrix4fv(glGetUniformLocation(shader, "model"), 1, GL_FALSE, &M[0][0]);
            groundObj.draw();
        };

        renderSceneForDepth(depthShader);

        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        //randare scena normala
        int w, h;
        glfwGetFramebufferSize(window, &w, &h);
        glViewport(0, 0, w, h);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        glUseProgram(program);
        //setare matrici
        glm::mat4 projection = glm::perspective(glm::radians(fov), (float)w/(float)h, 0.1f, 200.0f);
        glm::mat4 view = glm::lookAt(camPos, camPos + camFront, camUp);
        glm::mat4 model(1.0f);
        //model pune obiecte la pozitia initiala pe scena
        //view seteaza pozitia si orientarea camerei
        //projection seteaza perspectiva (fov, aspect ratio, near, far), perspectiva
        glUniformMatrix4fv(glGetUniformLocation(program, "projection"), 1, GL_FALSE, &projection[0][0]);
        glUniformMatrix4fv(glGetUniformLocation(program, "view"), 1, GL_FALSE, &view[0][0]);
        glUniformMatrix4fv(glGetUniformLocation(program, "model"), 1, GL_FALSE, &model[0][0]);
        glUniformMatrix4fv(glGetUniformLocation(program, "lightSpaceMatrix"), 1, GL_FALSE, &lightSpaceMatrix[0][0]);

        glUniform3f(glGetUniformLocation(program, "viewPos"), camPos.x, camPos.y, camPos.z);

        glActiveTexture(GL_TEXTURE0);
        glUniform1i(glGetUniformLocation(program, "textureSampler"), 0);

        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, depthMap);
        glUniform1i(glGetUniformLocation(program, "shadowMap"), 1);

        glUniform3f(glGetUniformLocation(program, "dirLightDir"), -0.2f, -1.0f, -0.3f);
        glUniform3f(glGetUniformLocation(program, "dirLightColor"), 0.9f, 0.9f, 0.85f);

        glUniform3f(glGetUniformLocation(program, "pointLightPos"), lampPosition.x, lampPosition.y, lampPosition.z);
        if (lampLightOn) {
            glUniform3f(glGetUniformLocation(program, "pointLightColor"), 1.0f, 0.9f, 0.7f);
        } else {
            glUniform3f(glGetUniformLocation(program, "pointLightColor"), 0.0f, 0.0f, 0.0f);
        }

        glUniform1i(glGetUniformLocation(program, "fogEnabled"), fogEnabled);
        glUniform1f(glGetUniformLocation(program, "fogDensity"), 0.08f);
        glUniform3f(glGetUniformLocation(program, "fogColor"), 0.6f, 0.65f, 0.7f);

        if (debugMode) {
            debugRenderer.drawFloorPolygonFilled(floorPoly, floorY, projection, view);
            debugRenderer.drawFloorPolygonFilled(floorPoly25, floorY25, projection, view);

            debugRenderer.drawFloorBoundary(floorPoly, floorY, projection, view);
            debugRenderer.drawFloorBoundary(floorPoly25, floorY25, projection, view);

            std::vector<std::vector<glm::vec3>> walls = {
                {glm::vec3(-0.21f, 0.0f, 6.7f), glm::vec3(-5.4f, 0.0f, 6.7f),
                 glm::vec3(-5.4f, 3.0f, 6.7f), glm::vec3(-2.3f, 3.0f, 6.7f)},
                {glm::vec3(-2.3f, 0.0f, 4.4f), glm::vec3(-2.3f, 0.0f, 6.7f),
                 glm::vec3(-2.3f, 0.5f, 6.7f), glm::vec3(-2.3f, 0.5f, 4.4f)},
                {glm::vec3(-2.3f, 0.5f, 5.6f), glm::vec3(-2.3f, 0.5f, 6.7f),
                 glm::vec3(-2.3f, 3.0f, 6.7f), glm::vec3(-2.3f, 3.0f, 5.6f)},
                {glm::vec3(-2.3f, 0.5f, 4.4f), glm::vec3(-2.3f, 0.5f, 4.8f),
                 glm::vec3(-2.3f, 3.0f, 4.8f), glm::vec3(-2.3f, 3.0f, 4.4f)},
                {glm::vec3(-2.3f, 2.29f, 4.8f), glm::vec3(-2.3f, 2.29f, 5.6f),
                 glm::vec3(-2.3f, 3.0f, 5.6f), glm::vec3(-2.3f, 3.0f, 4.8f)},
                {glm::vec3(-2.3f, 3.0f, 4.4f), glm::vec3(-2.3f, 0.5f, 4.4f),
                 glm::vec3(5.7f, 0.5f, 4.4f), glm::vec3(5.7f, 3.0f, 4.4f)},
                {glm::vec3(5.7f, 3.0f, 4.4f), glm::vec3(5.7f, 0.5f, 4.4f),
                 glm::vec3(5.7f, 0.5f, -6.0f), glm::vec3(5.7f, 3.0f, -3.7f)},
                {glm::vec3(5.7f, 0.5f, -3.7f), glm::vec3(1.0f, 0.5f, -3.7f),
                 glm::vec3(1.0f, 3.0f, -3.7f), glm::vec3(5.7f, 3.0f, -3.7f)},
                {glm::vec3(0.2f, 0.5f, -3.7f), glm::vec3(-5.4f, 0.5f, -3.7f),
                 glm::vec3(-5.4f, 3.0f, -3.7f), glm::vec3(0.2f, 3.0f, -3.7f)},
                {glm::vec3(0.2f, 2.28f, -3.7f), glm::vec3(1.0f, 2.28f, -3.7f),
                 glm::vec3(1.0f, 3.0f, -3.7f), glm::vec3(0.2f, 3.0f, -3.7f)},
                {glm::vec3(5.7f, 0.5f, -6.0f), glm::vec3(1.8f, 0.5f, -6.0f),
                 glm::vec3(1.8f, 3.0f, -6.0f), glm::vec3(5.7f, 3.0f, -6.0f)},
                {glm::vec3(-1.4f, 0.5f, -6.0f), glm::vec3(-5.4f, 0.5f, -6.0f),
                 glm::vec3(-5.4f, 3.0f, -6.0f), glm::vec3(-1.4f, 3.0f, -6.0f)},
                {glm::vec3(-5.4f, 0.5f, -6.0f), glm::vec3(-5.4f, 0.5f, 6.7f),
                 glm::vec3(-5.4f, 3.0f, 6.7f), glm::vec3(-5.4f, 3.0f, -6.0f)},
                {glm::vec3(-4.8f, 0.5f, 6.4f), glm::vec3(-4.8f, 0.5f, -3.4f),
                 glm::vec3(-4.8f, 3.0f, -3.4f), glm::vec3(-4.8f, 3.0f, 6.4f)}
            };
            for (const auto& wall : walls) {
                debugRenderer.drawWallQuad(wall, projection, view);
            }
            std::vector<std::vector<glm::vec3>> doorFrames = {
                {glm::vec3(-2.3f, 0.5f, 4.8f), glm::vec3(-2.6f, 0.5f, 4.8f),
                 glm::vec3(-2.6f, 2.29f, 4.8f), glm::vec3(-2.3f, 2.29f, 4.8f)},
                {glm::vec3(-2.3f, 0.5f, 5.6f), glm::vec3(-2.3f, 2.29f, 5.6f),
                 glm::vec3(-2.6f, 2.29f, 5.6f), glm::vec3(-2.6f, 0.5f, 5.6f)},
                {glm::vec3(-2.3f, 2.29f, 5.6f), glm::vec3(-2.3f, 2.29f, 4.8f),
                 glm::vec3(-2.6f, 2.29f, 4.8f), glm::vec3(-2.6f, 2.29f, 5.6f)},
                {glm::vec3(0.2f, 0.5f, -3.4f), glm::vec3(0.2f, 0.5f, -3.7f),
                 glm::vec3(0.2f, 2.29f, -3.7f), glm::vec3(0.2f, 2.29f, -3.4f)},
                {glm::vec3(1.0f, 0.5f, -3.7f), glm::vec3(1.0f, 0.5f, -3.4f),
                 glm::vec3(1.0f, 2.29f, -3.4f), glm::vec3(1.0f, 2.29f, -3.7f)},
                {glm::vec3(0.2f, 2.29f, -3.7f), glm::vec3(0.2f, 2.29f, -3.4f),
                 glm::vec3(1.0f, 2.29f, -3.4f), glm::vec3(1.0f, 2.29f, -3.7f)}
            };

            for (const auto& frame : doorFrames) {
                debugRenderer.drawWallQuad(frame, projection, view, glm::vec3(1.0f, 1.0f, 0.0f));
            }

            debugRenderer.drawDoorCollisionQuad(door1Corners, door1Angle, projection, view);
            debugRenderer.drawDoorCollisionQuad(door2Corners, door2Angle, projection, view);

            debugRenderer.drawSlope(slopePoints3D, projection, view, glm::vec3(1.0f, 1.0f, 0.0f));
            debugRenderer.drawSlope(slopePoints3D2, projection, view, glm::vec3(1.0f, 0.5f, 0.0f));

        } else {
            {//randare obiecte scena
                glm::mat4 M(1.0f);
                M = glm::translate(M, glm::vec3(0.0f, 0.0f, 0.0f));
                M = glm::scale(M, glm::vec3(1.0f));
                glUniformMatrix4fv(glGetUniformLocation(program, "model"), 1, GL_FALSE, &M[0][0]);
                houseObj.draw();
            }

            {
                glm::mat4 M(1.0f);
                M = glm::translate(M, glm::vec3(-2.41f, 1.41f, 4.89f));
                M = glm::rotate(M, glm::radians(door1Angle), glm::vec3(0, 1, 0));
                M = glm::scale(M, glm::vec3(1.0f));
                glUniformMatrix4fv(glGetUniformLocation(program, "model"), 1, GL_FALSE, &M[0][0]);
                doorNewObj.draw();
            }

            {
                glm::mat4 M(1.0f);
                M = glm::translate(M, glm::vec3(0.2f, 1.41f, -3.67f));
                M = glm::rotate(M, glm::radians(door2Angle), glm::vec3(0, 1, 0));
                M = glm::scale(M, glm::vec3(1.0f));
                glUniformMatrix4fv(glGetUniformLocation(program, "model"), 1, GL_FALSE, &M[0][0]);
                doorNew2Obj.draw();
            }

            {
                glm::mat4 M(1.0f);
                M = glm::translate(M, glm::vec3(0.0f, 0.0f, 0.0f));
                M = glm::scale(M, glm::vec3(1.0f));
                glUniformMatrix4fv(glGetUniformLocation(program, "model"), 1, GL_FALSE, &M[0][0]);
                interiorObj.draw();
            }

            {
                glm::mat4 M(1.0f);
                M = glm::translate(M, glm::vec3(0.0f, 0.0f, 0.0f));
                M = glm::scale(M, glm::vec3(1.0f));
                glUniformMatrix4fv(glGetUniformLocation(program, "model"), 1, GL_FALSE, &M[0][0]);
                floorObj.draw();
            }

            {
                glm::mat4 M(1.0f);
                M = glm::translate(M, glm::vec3(0.0f, 0.0f, 0.0f));
                M = glm::scale(M, glm::vec3(1.0f));
                glUniformMatrix4fv(glGetUniformLocation(program, "model"), 1, GL_FALSE, &M[0][0]);
                roofObj.draw();
            }

            {
                glm::mat4 M(1.0f);
                M = glm::translate(M, sofaPosition);
                M = glm::rotate(M, glm::radians(sofaRotation), glm::vec3(0, 1, 0));
                M = glm::scale(M, glm::vec3(1.5f));
                glUniformMatrix4fv(glGetUniformLocation(program, "model"), 1, GL_FALSE, &M[0][0]);
                sofaObj.draw();
            }

            {
                glm::mat4 M(1.0f);
                M = glm::translate(M, glm::vec3(1.5f, 0.5f, 0.0f));
                M = glm::scale(M, glm::vec3(0.25f));
                glUniformMatrix4fv(glGetUniformLocation(program, "model"), 1, GL_FALSE, &M[0][0]);
                lampObj.draw();
            }

            {
                glm::mat4 M(1.0f);
                M = glm::translate(M, glm::vec3(4.1f, 0.5f, 3.30f));
                M = glm::rotate(M, glm::radians(90.0f), glm::vec3(0, 1, 0));
                M = glm::scale(M, glm::vec3(0.08f));
                glUniformMatrix4fv(glGetUniformLocation(program, "model"), 1, GL_FALSE, &M[0][0]);
                tableObj.draw();
            }

            {
                glm::mat4 M(1.0f);
                M = glm::translate(M, glm::vec3(-12.0f, -0.7f, 8.0f));
                M = glm::scale(M, glm::vec3(0.8f));
                glUniformMatrix4fv(glGetUniformLocation(program, "model"), 1, GL_FALSE, &M[0][0]);
                treeObj.draw();
            }

            {
                glm::mat4 M(1.0f);
                M = glm::translate(M, glm::vec3(12.0f, -0.7f, 6.0f));
                M = glm::scale(M, glm::vec3(1.0f));
                glUniformMatrix4fv(glGetUniformLocation(program, "model"), 1, GL_FALSE, &M[0][0]);
                treeObj.draw();
            }

            {
                glm::mat4 M(1.0f);
                M = glm::translate(M, glm::vec3(10.0f, -0.7f, -12.0f));
                M = glm::scale(M, glm::vec3(0.9f));
                glUniformMatrix4fv(glGetUniformLocation(program, "model"), 1, GL_FALSE, &M[0][0]);
                treeObj.draw();
            }

            {
                glm::mat4 M(1.0f);
                M = glm::translate(M, glm::vec3(0.0f, -0.7f, 0.0f));
                M = glm::rotate(M, glm::radians(-90.0f), glm::vec3(1, 0, 0));
                M = glm::scale(M, glm::vec3(0.1f));
                glUniformMatrix4fv(glGetUniformLocation(program, "model"), 1, GL_FALSE, &M[0][0]);
                groundObj.draw();
            }
        }
        //randare skybox, facem ultimul pentru a evita probleme de depth testing
        if (skyboxTexture != 0) {
            glDepthFunc(GL_LEQUAL);
            glUseProgram(skyboxProgram);

            glm::mat4 skyboxView = glm::mat4(glm::mat3(view));
            glUniformMatrix4fv(glGetUniformLocation(skyboxProgram, "projection"), 1, GL_FALSE, &projection[0][0]);
            glUniformMatrix4fv(glGetUniformLocation(skyboxProgram, "view"), 1, GL_FALSE, &skyboxView[0][0]);

            glBindVertexArray(skyboxVAO);
            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, skyboxTexture);
            glUniform1i(glGetUniformLocation(skyboxProgram, "skybox"), 0);

            glDrawArrays(GL_TRIANGLES, 0, 36);
            glBindVertexArray(0);

            glDepthFunc(GL_LESS);
        }

        glfwSwapBuffers(window);
        glfwPollEvents();
    }
    //curatare resurse
    glfwTerminate();
    return 0;
}
