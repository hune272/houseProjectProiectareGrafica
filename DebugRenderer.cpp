#include "DebugRenderer.h"

#include <iostream>

static const char* debugVertexShader = R"(
#version 330 core
layout(location = 0) in vec3 aPos;

uniform mat4 projection;
uniform mat4 view;
uniform mat4 model;

void main() {
    gl_Position = projection * view * model * vec4(aPos, 1.0);
}
)";

static const char* debugFragmentShader = R"(
#version 330 core
out vec4 FragColor;

uniform vec3 lineColor;
uniform float alpha;

void main() {
    FragColor = vec4(lineColor, alpha);
}
)";

DebugRenderer::DebugRenderer() : vao(0), vbo(0), shaderProgram(0) {}

DebugRenderer::~DebugRenderer() {
    cleanup();
}

void DebugRenderer::init() {
    glGenVertexArrays(1, &vao);
    glGenBuffers(1, &vbo);

    glBindVertexArray(vao);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    glBindVertexArray(0);

    createShader();
}

void DebugRenderer::createShader() {
    GLuint vertexShader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertexShader, 1, &debugVertexShader, nullptr);
    glCompileShader(vertexShader);

    GLint success;
    glGetShaderiv(vertexShader, GL_COMPILE_STATUS, &success);
    if (!success) {
        char infoLog[512];
        glGetShaderInfoLog(vertexShader, 512, nullptr, infoLog);
        std::cerr << "Debug Vertex Shader Compile Error:\n" << infoLog << "\n";
    }

    GLuint fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragmentShader, 1, &debugFragmentShader, nullptr);
    glCompileShader(fragmentShader);

    glGetShaderiv(fragmentShader, GL_COMPILE_STATUS, &success);
    if (!success) {
        char infoLog[512];
        glGetShaderInfoLog(fragmentShader, 512, nullptr, infoLog);
        std::cerr << "Debug Fragment Shader Compile Error:\n" << infoLog << "\n";
    }

    shaderProgram = glCreateProgram();
    glAttachShader(shaderProgram, vertexShader);
    glAttachShader(shaderProgram, fragmentShader);
    glLinkProgram(shaderProgram);

    glGetProgramiv(shaderProgram, GL_LINK_STATUS, &success);
    if (!success) {
        char infoLog[512];
        glGetProgramInfoLog(shaderProgram, 512, nullptr, infoLog);
        std::cerr << "Debug Shader Program Link Error:\n" << infoLog << "\n";
    }

    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);
}

void DebugRenderer::drawFloorBoundary(const std::vector<glm::vec2>& polygon, float y,
                                      const glm::mat4& projection, const glm::mat4& view) {
    if (polygon.empty()) return;
    std::vector<float> vertices;
    for (size_t i = 0; i < polygon.size(); ++i) {
        size_t next = (i + 1) % polygon.size();
        vertices.push_back(polygon[i].x);
        vertices.push_back(y);
        vertices.push_back(polygon[i].y);
        vertices.push_back(polygon[next].x);
        vertices.push_back(y);
        vertices.push_back(polygon[next].y);
    }
    float ceilingY = y + 3.0f;
    for (const auto& point : polygon) {
        vertices.push_back(point.x);
        vertices.push_back(y);
        vertices.push_back(point.y);
        vertices.push_back(point.x);
        vertices.push_back(ceilingY);
        vertices.push_back(point.y);
    }
    for (size_t i = 0; i < polygon.size(); ++i) {
        size_t next = (i + 1) % polygon.size();
        vertices.push_back(polygon[i].x);
        vertices.push_back(ceilingY);
        vertices.push_back(polygon[i].y);
        vertices.push_back(polygon[next].x);
        vertices.push_back(ceilingY);
        vertices.push_back(polygon[next].y);
    }
    glBindVertexArray(vao);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(float), vertices.data(), GL_DYNAMIC_DRAW);
    glUseProgram(shaderProgram);
    glm::mat4 model(1.0f);
    glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "projection"), 1, GL_FALSE, &projection[0][0]);
    glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "view"), 1, GL_FALSE, &view[0][0]);
    glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "model"), 1, GL_FALSE, &model[0][0]);
    glUniform3f(glGetUniformLocation(shaderProgram, "lineColor"), 0.0f, 1.0f, 0.0f); // Green lines
    glLineWidth(3.0f);
    glDrawArrays(GL_LINES, 0, vertices.size() / 3);
    glLineWidth(1.0f);
    glBindVertexArray(0);
}
void DebugRenderer::drawDoorCollision(const glm::vec3& doorPos, float doorAngle,
                                     const glm::vec3& doorNormal, float doorWidth, float doorThickness,
                                     const glm::mat4& projection, const glm::mat4& view) {
    std::vector<float> vertices;
    float halfWidth = doorWidth / 2.0f;
    float halfThickness = doorThickness / 2.0f;
    float doorHeight = 2.0f;
    glm::vec3 doorRight = glm::normalize(glm::cross(doorNormal, glm::vec3(0, 1, 0)));
    glm::vec3 doorUp = glm::vec3(0, 1, 0);
    float angleRad = glm::radians(doorAngle);
    glm::mat4 rotation = glm::rotate(glm::mat4(1.0f), angleRad, glm::vec3(0, 1, 0));
    glm::vec3 corners[8] = {
        glm::vec3(-halfWidth, 0.0f, -halfThickness),
        glm::vec3( halfWidth, 0.0f, -halfThickness),
        glm::vec3( halfWidth, 0.0f,  halfThickness),
        glm::vec3(-halfWidth, 0.0f,  halfThickness),
        glm::vec3(-halfWidth, doorHeight, -halfThickness),
        glm::vec3( halfWidth, doorHeight, -halfThickness),
        glm::vec3( halfWidth, doorHeight,  halfThickness),
        glm::vec3(-halfWidth, doorHeight,  halfThickness)
    };
    glm::vec3 worldCorners[8];
    for (int i = 0; i < 8; i++) {
        glm::vec4 rotated = rotation * glm::vec4(corners[i], 1.0f);
        worldCorners[i] = glm::vec3(rotated) + doorPos;
    }
    vertices.push_back(worldCorners[0].x); vertices.push_back(worldCorners[0].y); vertices.push_back(worldCorners[0].z);
    vertices.push_back(worldCorners[1].x); vertices.push_back(worldCorners[1].y); vertices.push_back(worldCorners[1].z);
    vertices.push_back(worldCorners[1].x); vertices.push_back(worldCorners[1].y); vertices.push_back(worldCorners[1].z);
    vertices.push_back(worldCorners[2].x); vertices.push_back(worldCorners[2].y); vertices.push_back(worldCorners[2].z);
    vertices.push_back(worldCorners[2].x); vertices.push_back(worldCorners[2].y); vertices.push_back(worldCorners[2].z);
    vertices.push_back(worldCorners[3].x); vertices.push_back(worldCorners[3].y); vertices.push_back(worldCorners[3].z);
    vertices.push_back(worldCorners[3].x); vertices.push_back(worldCorners[3].y); vertices.push_back(worldCorners[3].z);
    vertices.push_back(worldCorners[0].x); vertices.push_back(worldCorners[0].y); vertices.push_back(worldCorners[0].z);
    vertices.push_back(worldCorners[4].x); vertices.push_back(worldCorners[4].y); vertices.push_back(worldCorners[4].z);
    vertices.push_back(worldCorners[5].x); vertices.push_back(worldCorners[5].y); vertices.push_back(worldCorners[5].z);
    vertices.push_back(worldCorners[5].x); vertices.push_back(worldCorners[5].y); vertices.push_back(worldCorners[5].z);
    vertices.push_back(worldCorners[6].x); vertices.push_back(worldCorners[6].y); vertices.push_back(worldCorners[6].z);
    vertices.push_back(worldCorners[6].x); vertices.push_back(worldCorners[6].y); vertices.push_back(worldCorners[6].z);
    vertices.push_back(worldCorners[7].x); vertices.push_back(worldCorners[7].y); vertices.push_back(worldCorners[7].z);
    vertices.push_back(worldCorners[7].x); vertices.push_back(worldCorners[7].y); vertices.push_back(worldCorners[7].z);
    vertices.push_back(worldCorners[4].x); vertices.push_back(worldCorners[4].y); vertices.push_back(worldCorners[4].z);
    for (int i = 0; i < 4; i++) {
        vertices.push_back(worldCorners[i].x); vertices.push_back(worldCorners[i].y); vertices.push_back(worldCorners[i].z);
        vertices.push_back(worldCorners[i+4].x); vertices.push_back(worldCorners[i+4].y); vertices.push_back(worldCorners[i+4].z);
    }
    glBindVertexArray(vao);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(float), vertices.data(), GL_DYNAMIC_DRAW);
    glUseProgram(shaderProgram);
    glm::mat4 model(1.0f);
    glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "projection"), 1, GL_FALSE, &projection[0][0]);
    glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "view"), 1, GL_FALSE, &view[0][0]);
    glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "model"), 1, GL_FALSE, &model[0][0]);
    glUniform3f(glGetUniformLocation(shaderProgram, "lineColor"), 1.0f, 0.0f, 0.0f);
    glLineWidth(2.0f);
    glDrawArrays(GL_LINES, 0, vertices.size() / 3);
    glLineWidth(1.0f);
    glBindVertexArray(0);
}
void DebugRenderer::drawDoorCollisionQuad(const std::vector<glm::vec3>& baseCorners, float doorAngle,
                                         const glm::mat4& projection, const glm::mat4& view) {
    if (baseCorners.size() != 4) return;
    glm::vec3 hingePos = baseCorners[0];
    hingePos.y = (baseCorners[0].y + baseCorners[1].y) / 2.0f; // mid-height
    std::vector<glm::vec3> rotatedCorners;
    float angleRad = glm::radians(doorAngle);
    glm::mat4 rotation = glm::rotate(glm::mat4(1.0f), angleRad, glm::vec3(0, 1, 0));
    for (const auto& corner : baseCorners) {
        glm::vec3 localPos = corner - hingePos;
        glm::vec4 rotated = rotation * glm::vec4(localPos, 1.0f);
        glm::vec3 worldPos = glm::vec3(rotated) + hingePos;
        rotatedCorners.push_back(worldPos);
    }
    std::vector<float> vertices;
    vertices.push_back(rotatedCorners[0].x); vertices.push_back(rotatedCorners[0].y); vertices.push_back(rotatedCorners[0].z);
    vertices.push_back(rotatedCorners[3].x); vertices.push_back(rotatedCorners[3].y); vertices.push_back(rotatedCorners[3].z);
    vertices.push_back(rotatedCorners[3].x); vertices.push_back(rotatedCorners[3].y); vertices.push_back(rotatedCorners[3].z);
    vertices.push_back(rotatedCorners[2].x); vertices.push_back(rotatedCorners[2].y); vertices.push_back(rotatedCorners[2].z);
    vertices.push_back(rotatedCorners[2].x); vertices.push_back(rotatedCorners[2].y); vertices.push_back(rotatedCorners[2].z);
    vertices.push_back(rotatedCorners[1].x); vertices.push_back(rotatedCorners[1].y); vertices.push_back(rotatedCorners[1].z);
    vertices.push_back(rotatedCorners[1].x); vertices.push_back(rotatedCorners[1].y); vertices.push_back(rotatedCorners[1].z);
    vertices.push_back(rotatedCorners[0].x); vertices.push_back(rotatedCorners[0].y); vertices.push_back(rotatedCorners[0].z);
    glBindVertexArray(vao);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(float), vertices.data(), GL_DYNAMIC_DRAW);
    glUseProgram(shaderProgram);
    glm::mat4 model(1.0f);
    glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "projection"), 1, GL_FALSE, &projection[0][0]);
    glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "view"), 1, GL_FALSE, &view[0][0]);
    glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "model"), 1, GL_FALSE, &model[0][0]);
    glUniform3f(glGetUniformLocation(shaderProgram, "lineColor"), 1.0f, 0.0f, 0.0f);
    glLineWidth(3.0f);
    glDrawArrays(GL_LINES, 0, vertices.size() / 3);
    glLineWidth(1.0f);
    glBindVertexArray(0);
}
void DebugRenderer::drawWalls(const std::vector<glm::vec2>& floorPoly, float floorY, float wallHeight,
                              const glm::mat4& projection, const glm::mat4& view) {
    if (floorPoly.empty()) return;
    std::vector<float> vertices;
    float topY = floorY + wallHeight;
    for (size_t i = 0; i < floorPoly.size(); ++i) {
        size_t next = (i + 1) % floorPoly.size();
        glm::vec2 p1 = floorPoly[i];
        glm::vec2 p2 = floorPoly[next];
        vertices.push_back(p1.x); vertices.push_back(floorY); vertices.push_back(p1.y);
        vertices.push_back(p2.x); vertices.push_back(floorY); vertices.push_back(p2.y);
        vertices.push_back(p1.x); vertices.push_back(topY); vertices.push_back(p1.y);
        vertices.push_back(p2.x); vertices.push_back(topY); vertices.push_back(p2.y);
        vertices.push_back(p1.x); vertices.push_back(floorY); vertices.push_back(p1.y);
        vertices.push_back(p1.x); vertices.push_back(topY); vertices.push_back(p1.y);
    }
    glBindVertexArray(vao);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(float), vertices.data(), GL_DYNAMIC_DRAW);
    glUseProgram(shaderProgram);
    glm::mat4 model(1.0f);
    glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "projection"), 1, GL_FALSE, &projection[0][0]);
    glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "view"), 1, GL_FALSE, &view[0][0]);
    glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "model"), 1, GL_FALSE, &model[0][0]);
    glUniform3f(glGetUniformLocation(shaderProgram, "lineColor"), 0.0f, 0.5f, 1.0f);
    glLineWidth(2.0f);
    glDrawArrays(GL_LINES, 0, vertices.size() / 3);
    glLineWidth(1.0f);
    glBindVertexArray(0);
}
void DebugRenderer::drawFloorPolygonFilled(const std::vector<glm::vec2>& floorPoly, float floorY,
                                          const glm::mat4& projection, const glm::mat4& view) {
    if (floorPoly.size() < 3) return;
    std::vector<float> vertices;
    glm::vec2 center(0.0f);
    for (const auto& p : floorPoly) {
        center.x += p.x;
        center.y += p.y;
    }
    center.x /= floorPoly.size();
    center.y /= floorPoly.size();
    for (size_t i = 0; i < floorPoly.size(); ++i) {
        size_t next = (i + 1) % floorPoly.size();
        vertices.push_back(center.x); vertices.push_back(floorY); vertices.push_back(center.y);
        vertices.push_back(floorPoly[i].x); vertices.push_back(floorY); vertices.push_back(floorPoly[i].y);
        vertices.push_back(floorPoly[next].x); vertices.push_back(floorY); vertices.push_back(floorPoly[next].y);
    }
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glBindVertexArray(vao);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(float), vertices.data(), GL_DYNAMIC_DRAW);
    glUseProgram(shaderProgram);
    glm::mat4 model(1.0f);
    glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "projection"), 1, GL_FALSE, &projection[0][0]);
    glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "view"), 1, GL_FALSE, &view[0][0]);
    glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "model"), 1, GL_FALSE, &model[0][0]);
    glUniform3f(glGetUniformLocation(shaderProgram, "lineColor"), 0.0f, 1.0f, 0.0f);
    glUniform1f(glGetUniformLocation(shaderProgram, "alpha"), 0.3f);
    glDrawArrays(GL_TRIANGLES, 0, vertices.size() / 3);
    glDisable(GL_BLEND);
    glBindVertexArray(0);
}
void DebugRenderer::drawWallQuad(const std::vector<glm::vec3>& wallCorners,
                                 const glm::mat4& projection, const glm::mat4& view,
                                 const glm::vec3& color) {
    if (wallCorners.size() != 4) return;
    std::vector<float> vertices;
    vertices.push_back(wallCorners[0].x); vertices.push_back(wallCorners[0].y); vertices.push_back(wallCorners[0].z);
    vertices.push_back(wallCorners[1].x); vertices.push_back(wallCorners[1].y); vertices.push_back(wallCorners[1].z);
    vertices.push_back(wallCorners[1].x); vertices.push_back(wallCorners[1].y); vertices.push_back(wallCorners[1].z);
    vertices.push_back(wallCorners[2].x); vertices.push_back(wallCorners[2].y); vertices.push_back(wallCorners[2].z);
    vertices.push_back(wallCorners[2].x); vertices.push_back(wallCorners[2].y); vertices.push_back(wallCorners[2].z);
    vertices.push_back(wallCorners[3].x); vertices.push_back(wallCorners[3].y); vertices.push_back(wallCorners[3].z);
    vertices.push_back(wallCorners[3].x); vertices.push_back(wallCorners[3].y); vertices.push_back(wallCorners[3].z);
    vertices.push_back(wallCorners[0].x); vertices.push_back(wallCorners[0].y); vertices.push_back(wallCorners[0].z);
    glBindVertexArray(vao);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(float), vertices.data(), GL_DYNAMIC_DRAW);
    glUseProgram(shaderProgram);
    glm::mat4 model(1.0f);
    glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "projection"), 1, GL_FALSE, &projection[0][0]);
    glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "view"), 1, GL_FALSE, &view[0][0]);
    glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "model"), 1, GL_FALSE, &model[0][0]);
    glUniform3f(glGetUniformLocation(shaderProgram, "lineColor"), color.r, color.g, color.b);
    glUniform1f(glGetUniformLocation(shaderProgram, "alpha"), 1.0f);
    glLineWidth(2.5f);
    glDrawArrays(GL_LINES, 0, vertices.size() / 3);
    glLineWidth(1.0f);
    glBindVertexArray(0);
}
void DebugRenderer::drawSlope(const std::vector<glm::vec3>& slopePoints,
                              const glm::mat4& projection, const glm::mat4& view,
                              const glm::vec3& color) {
    if (slopePoints.size() != 4) return;
    std::vector<float> vertices;
    vertices.push_back(slopePoints[0].x); vertices.push_back(slopePoints[0].y); vertices.push_back(slopePoints[0].z);
    vertices.push_back(slopePoints[1].x); vertices.push_back(slopePoints[1].y); vertices.push_back(slopePoints[1].z);
    vertices.push_back(slopePoints[1].x); vertices.push_back(slopePoints[1].y); vertices.push_back(slopePoints[1].z);
    vertices.push_back(slopePoints[2].x); vertices.push_back(slopePoints[2].y); vertices.push_back(slopePoints[2].z);
    vertices.push_back(slopePoints[2].x); vertices.push_back(slopePoints[2].y); vertices.push_back(slopePoints[2].z);
    vertices.push_back(slopePoints[3].x); vertices.push_back(slopePoints[3].y); vertices.push_back(slopePoints[3].z);
    vertices.push_back(slopePoints[3].x); vertices.push_back(slopePoints[3].y); vertices.push_back(slopePoints[3].z);
    vertices.push_back(slopePoints[0].x); vertices.push_back(slopePoints[0].y); vertices.push_back(slopePoints[0].z);
    vertices.push_back(slopePoints[0].x); vertices.push_back(slopePoints[0].y); vertices.push_back(slopePoints[0].z);
    vertices.push_back(slopePoints[2].x); vertices.push_back(slopePoints[2].y); vertices.push_back(slopePoints[2].z);
    vertices.push_back(slopePoints[1].x); vertices.push_back(slopePoints[1].y); vertices.push_back(slopePoints[1].z);
    vertices.push_back(slopePoints[3].x); vertices.push_back(slopePoints[3].y); vertices.push_back(slopePoints[3].z);
    glBindVertexArray(vao);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(float), vertices.data(), GL_DYNAMIC_DRAW);
    glUseProgram(shaderProgram);
    glm::mat4 model(1.0f);
    glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "projection"), 1, GL_FALSE, &projection[0][0]);
    glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "view"), 1, GL_FALSE, &view[0][0]);
    glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "model"), 1, GL_FALSE, &model[0][0]);
    glUniform3f(glGetUniformLocation(shaderProgram, "lineColor"), color.r, color.g, color.b);
    glUniform1f(glGetUniformLocation(shaderProgram, "alpha"), 1.0f);
    glLineWidth(3.0f);
    glDrawArrays(GL_LINES, 0, vertices.size() / 3);
    glLineWidth(1.0f);
    glBindVertexArray(0);
}
void DebugRenderer::cleanup() {
    if (vao) glDeleteVertexArrays(1, &vao);
    if (vbo) glDeleteBuffers(1, &vbo);
    if (shaderProgram) glDeleteProgram(shaderProgram);
    vao = vbo = shaderProgram = 0;
}
