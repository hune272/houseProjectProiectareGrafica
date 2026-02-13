#ifndef DEBUG_RENDERER_H
#define DEBUG_RENDERER_H

#include <GL/glew.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <vector>

class DebugRenderer {
public:
    DebugRenderer();
    ~DebugRenderer();

    void init();
    void cleanup();

    void drawFloorBoundary(const std::vector<glm::vec2>& polygon, float y,
                          const glm::mat4& projection, const glm::mat4& view);

    void drawDoorCollision(const glm::vec3& doorPos, float doorAngle,
                          const glm::vec3& doorNormal, float doorWidth, float doorThickness,
                          const glm::mat4& projection, const glm::mat4& view);

    void drawDoorCollisionQuad(const std::vector<glm::vec3>& corners, float doorAngle,
                              const glm::mat4& projection, const glm::mat4& view);

    void drawWalls(const std::vector<glm::vec2>& floorPoly, float floorY, float wallHeight,
                  const glm::mat4& projection, const glm::mat4& view);

    void drawFloorPolygonFilled(const std::vector<glm::vec2>& floorPoly, float floorY,
                               const glm::mat4& projection, const glm::mat4& view);

    void drawWallQuad(const std::vector<glm::vec3>& wallCorners,
                     const glm::mat4& projection, const glm::mat4& view,
                     const glm::vec3& color = glm::vec3(0.0f, 1.0f, 1.0f));

    void drawSlope(const std::vector<glm::vec3>& slopePoints,
                  const glm::mat4& projection, const glm::mat4& view,
                  const glm::vec3& color = glm::vec3(1.0f, 1.0f, 0.0f));

private:
    GLuint vao;
    GLuint vbo;
    GLuint shaderProgram;

    void createShader();
};

#endif
