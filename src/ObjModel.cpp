#include "ObjModel.h"

#include <tiny_obj_loader.h>
#include <stb_image.h>

#include <iostream>
#include <cmath>

static glm::vec3 calcNormal(const glm::vec3& a, const glm::vec3& b, const glm::vec3& c) {
    return glm::normalize(glm::cross(b - a, c - a));
}

GLuint ObjModel::loadTextureFromFile(const std::string& filename) {
    if (filename.empty()) return 0;

    std::string fullPath = basePath + filename;
    //incarca imaginea folosind stb_image
    int width, height, channels;
    stbi_set_flip_vertically_on_load(true);
    unsigned char* data = stbi_load(fullPath.c_str(), &width, &height, &channels, 0);

    if (!data) {
        std::cerr << "Failed to load texture: " << fullPath << "\n";
        return 0;
    }
    //formatul texturii
    GLenum format = (channels == 4) ? GL_RGBA : GL_RGB;

    GLuint textureID;
    glGenTextures(1, &textureID);
    glBindTexture(GL_TEXTURE_2D, textureID);
    //paremetrii texturii
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    //incarca datele in textura OpenGL
    glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, data);
    glGenerateMipmap(GL_TEXTURE_2D);

    stbi_image_free(data);
    std::cout << "Loaded material texture: " << fullPath << "\n";
    return textureID;
}

bool ObjModel::load(const std::string& path) {
    //diferenenta intre atribut si uniform este ca atributele sunt specifice fiecarui varf, uniformele sunt constante pentru toate varfurile
    tinyobj::attrib_t attrib;
    std::vector<tinyobj::shape_t> shapes;
    std::vector<tinyobj::material_t> materials;
    std::string warn, err;

    std::string baseDir = ".";
    auto slash = path.find_last_of("/\\");
    if (slash != std::string::npos) baseDir = path.substr(0, slash + 1);
    basePath = baseDir;

    bool ok = tinyobj::LoadObj(&attrib, &shapes, &materials, &warn, &err,
                              path.c_str(), baseDir.c_str(), true);

    if (!warn.empty()) std::cout << "OBJ warn: " << warn << "\n";
    if (!err.empty())  std::cerr << "OBJ err: " << err << "\n";
    if (!ok) return false;

    vertices.clear();
    vertices.reserve(200000);
    materialGroups.clear();

    std::map<int, std::vector<ObjVertex>> materialVertices;

    for (const auto& shape : shapes) {
        size_t idxOffset = 0;
        for (size_t f = 0; f < shape.mesh.num_face_vertices.size(); f++) {
            int fv = shape.mesh.num_face_vertices[f];
            if (fv != 3) {
                idxOffset += fv;
                continue;
            }

            int materialID = shape.mesh.material_ids[f];
            ObjVertex v[3];
            bool hasNormals = true;

            for (int k = 0; k < 3; k++) {
                tinyobj::index_t idx = shape.mesh.indices[idxOffset + k];

                v[k].pos = glm::vec3(
                    attrib.vertices[3*idx.vertex_index + 0],
                    attrib.vertices[3*idx.vertex_index + 1],
                    attrib.vertices[3*idx.vertex_index + 2]
                );

                if (idx.normal_index >= 0) {
                    v[k].normal = glm::vec3(
                        attrib.normals[3*idx.normal_index + 0],
                        attrib.normals[3*idx.normal_index + 1],
                        attrib.normals[3*idx.normal_index + 2]
                    );
                } else {
                    hasNormals = false;
                    v[k].normal = glm::vec3(0, 1, 0);
                }

                if (idx.texcoord_index >= 0) {
                    float u = attrib.texcoords[2 * idx.texcoord_index + 0];
                    float vv = attrib.texcoords[2 * idx.texcoord_index + 1];
                    v[k].uv = glm::vec2(u, vv);
                } else {
                    v[k].uv = glm::vec2(0.0f, 0.0f);
                }
            }

            if (!hasNormals) {
                glm::vec3 n = calcNormal(v[0].pos, v[1].pos, v[2].pos);
                v[0].normal = v[1].normal = v[2].normal = n;
            }
            materialVertices[materialID].push_back(v[0]);
            materialVertices[materialID].push_back(v[1]);
            materialVertices[materialID].push_back(v[2]);
            idxOffset += 3;
        }
    }
    for (auto& pair : materialVertices) {
        int matID = pair.first;
        std::vector<ObjVertex>& matVerts = pair.second;
        MaterialGroup group;
        group.startIndex = vertices.size();
        group.vertexCount = matVerts.size();
        if (matID >= 0 && matID < materials.size()) {
            const auto& mat = materials[matID];
            if (!mat.diffuse_texname.empty()) {
                group.textureID = loadTextureFromFile(mat.diffuse_texname);
            } else {
                group.textureID = 0;
            }
        } else {
            group.textureID = 0;
        }
        materialGroups.push_back(group);
        vertices.insert(vertices.end(), matVerts.begin(), matVerts.end());
    }
    std::cout << "Loaded OBJ: " << path << " verts=" << vertices.size()
              << " materials=" << materialGroups.size() << "\n";
    uploadToGPU();
    return true;
}
// incarcare date in GPU
//aplicam mai multe materiale pt un singur obiect
void ObjModel::uploadToGPU() {
    //VBO este folosit pentru a stoca datele varfurilor
    //VAO retine configuratia atributelor varfurilor
    if (VAO == 0) glGenVertexArrays(1, &VAO);
    if (VBO == 0) glGenBuffers(1, &VBO);
    glBindVertexArray(VAO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER,
                 (GLsizeiptr)(vertices.size() * sizeof(ObjVertex)),
                 vertices.data(),
                 GL_STATIC_DRAW);
    //vertexAttribPointer configureaza modul in care datele varfurilor sunt interpretate de shader
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(ObjVertex), (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(ObjVertex), (void*)offsetof(ObjVertex, normal));
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(ObjVertex), (void*)offsetof(ObjVertex, uv));
    glEnableVertexAttribArray(2);
    glBindVertexArray(0);
}
void ObjModel::setTexture(GLuint texID) {
    textureID = texID;
}
//aplicam numai o singura textura pentru tot obiectul
//sau un textura grup pentru fiecare obiect
void ObjModel::draw() const {
    glBindVertexArray(VAO);
    if (materialGroups.empty()) {
        if (textureID) {
            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, textureID);
        }
        glDrawArrays(GL_TRIANGLES, 0, (GLsizei)vertices.size());
    } else {
        for (const auto& group : materialGroups) {
            GLuint texToUse = group.textureID ? group.textureID : textureID;
            if (texToUse) {
                glActiveTexture(GL_TEXTURE0);
                glBindTexture(GL_TEXTURE_2D, texToUse);
            }
            glDrawArrays(GL_TRIANGLES, group.startIndex, group.vertexCount);
        }
    }
    glBindVertexArray(0);
}
