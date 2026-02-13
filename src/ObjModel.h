#pragma once

#include <GL/glew.h>
#include <glm/glm.hpp>

#include <string>
#include <vector>
#include <map>

struct ObjVertex {
    glm::vec3 pos;
    glm::vec3 normal;
    glm::vec2 uv;
};

struct MaterialGroup {
    int startIndex;
    int vertexCount;
    GLuint textureID;
};

class ObjModel {
public:
    bool load(const std::string& path);
    void setTexture(GLuint texID);
    void draw() const;

private:
    std::vector<ObjVertex> vertices;
    //ca sa pot  desene mai mult materiale din acelasi obiect
    //unifrom exemple, model, view, projection apllicam pentru toate la fel
    //la attribute aplicam diferit pentru fiecare
    std::vector<MaterialGroup> materialGroups;

    GLuint VAO = 0;
    GLuint VBO = 0;
    GLuint textureID = 0;

    std::string basePath;

    void uploadToGPU();
    GLuint loadTextureFromFile(const std::string& filename);
};
