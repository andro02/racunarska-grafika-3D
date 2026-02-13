#define STB_IMAGE_IMPLEMENTATION
#define _USE_MATH_DEFINES

#include <GL/glew.h>
#include <GLFW/glfw3.h>

#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <iostream>
#include <chrono>
#include <thread>
#include <vector>
#include <string>
#include <cmath>
#include <map>
#include <algorithm>
#include "Util.h"
#include "Mesh.h"
#include "Model.h"
#include "Shader.h"

GLFWwindow* window;
int screenWidth, screenHeight;
float aspect;

bool depthTestEnabled = true;
bool cullFaceEnabled = true;

const double targetFPS = 75.0f;
auto targetFrameDuration = std::chrono::duration<double>(1.0f / targetFPS);

glm::mat4 projection;
glm::mat4 view;

const float tankWidth = 10.0f;
const float tankHeight = 5.0f;
const float tankDepth = 6.0f;
const float wallThickness = 0.1f;

int sandRows = 13;
int sandCols = 13;
float sandWidth = tankWidth - 2 * wallThickness;
float sandDepth = tankDepth - 2 * wallThickness;
float sandHeight = 0.8f;

glm::vec3 goldfishInput(0.0f);
glm::vec3 clownfishInput(0.0f);

bool isZPressed = false;
bool isXPressed = false;
bool isFPressed = false;
bool isCPressed = false;

unsigned int loadTexture(const char* path) {
    unsigned int textureID;

    glGenTextures(1, &textureID);

    int width, height, nrChannels;
    stbi_set_flip_vertically_on_load(true);
    unsigned char* data = stbi_load(path, &width, &height, &nrChannels, 0);
    if (data) {
        GLenum format = (nrChannels == 4) ? GL_RGBA : GL_RGB;
        glBindTexture(GL_TEXTURE_2D, textureID);
        glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, data);
        glGenerateMipmap(GL_TEXTURE_2D);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR); // or GL_NEAREST
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR); // or GL_NEAREST
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT); // or GL_CLAMP_TO_EDGE
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT); // or GL_CLAMP_TO_EDGE
    }
    else {
        std::cerr << "Failed to load texture: " << path << std::endl;
    }
    stbi_image_free(data);
    return textureID;
}

Mesh createCubeMesh(glm::vec3 size, bool inwardNormals = false, const std::string& texturePath = "")
{
    float x = size.x / 2, y = size.y / 2, z = size.z / 2;
    float dir = inwardNormals ? -1.0f : 1.0f;

    std::vector<Vertex> vertices = {
        // Front (+Z)
        {{-x,-y, z},{0,0,dir},{0,0}}, {{x,-y, z},{0,0,dir},{1,0}},
        {{x, y, z},{0,0,dir},{1,1}}, {{-x, y, z},{0,0,dir},{0,1}},
        // Back (-Z)
        {{x,-y,-z},{0,0,-dir},{0,0}}, {{-x,-y,-z},{0,0,-dir},{1,0}},
        {{-x, y,-z},{0,0,-dir},{1,1}}, {{x, y,-z},{0,0,-dir},{0,1}},
        // Left (-X)
        {{-x,-y,-z},{-dir,0,0},{0,0}}, {{-x,-y, z},{-dir,0,0},{1,0}},
        {{-x, y, z},{-dir,0,0},{1,1}}, {{-x, y,-z},{-dir,0,0},{0,1}},
        // Right (+X)
        {{x,-y, z},{dir,0,0},{0,0}}, {{x,-y,-z},{dir,0,0},{1,0}},
        {{x, y,-z},{dir,0,0},{1,1}}, {{x, y, z},{dir,0,0},{0,1}},
        // Bottom (-Y)
        {{-x,-y,-z},{0,-dir,0},{0,0}}, {{x,-y,-z},{0,-dir,0},{1,0}},
        {{x,-y, z},{0,-dir,0},{1,1}}, {{-x,-y, z},{0,-dir,0},{0,1}},
        // Top (+Y)
        {{-x, y, z},{0,dir,0},{0,0}}, {{x, y, z},{0,dir,0},{1,0}},
        {{x, y,-z},{0,dir,0},{1,1}}, {{-x, y,-z},{0,dir,0},{0,1}}
    };

    std::vector<unsigned int> indices = {
        0,1,2, 2,3,0,       // Front
        4,7,6, 6,5,4,       // Back
        8,11,10, 10,9,8,    // Left
        12,13,14, 14,15,12,// Right
        16,18,19, 18,16,17,// Bottom
        20,21,22, 22,23,20 // Top
    };

    std::vector<Texture> textures;

    if (!texturePath.empty())
    {
        Texture tex;
        tex.id = loadTexture(texturePath.c_str());
        tex.type = "texture_diffuse";
        tex.path = texturePath;
        textures.push_back(tex);
    }

    return Mesh(vertices, indices, textures);
}

Mesh createSandMeshFilled(int rows, int cols, float width, float depth, float maxHeight)
{
    std::vector<Vertex> vertices;
    std::vector<unsigned int> indices;

    float dx = width / (cols - 1);
    float dz = depth / (rows - 1);

    // --- Vertices ---
    for (int z = 0; z < rows; z++)
    {
        for (int x = 0; x < cols; x++)
        {
            float xpos = -width / 2 + x * dx;
            float zpos = -depth / 2 + z * dz;

            // Top vertex
            float yTop = ((rand() % 100) / 100.0f + 0.3f) * maxHeight;
            Vertex topV;
            topV.Position = glm::vec3(xpos, yTop, zpos);
            topV.Normal = glm::vec3(0, 1, 0); // privremeno, izračunaće se kasnije
            topV.TexCoords = glm::vec2((float)x / (cols - 1), (float)z / (rows - 1));
            vertices.push_back(topV);

            // Bottom vertex
            Vertex botV;
            botV.Position = glm::vec3(xpos, 0.0f, zpos);
            botV.Normal = glm::vec3(0, -1, 0);
            botV.TexCoords = glm::vec2((float)x / (cols - 1), 0.0f);
            vertices.push_back(botV);
        }
    }

    // --- Indeksi i normale ---
    for (int z = 0; z < rows - 1; z++)
    {
        for (int x = 0; x < cols - 1; x++)
        {
            int i = z * cols + x;
            int iTop = i * 2;
            int iBot = iTop + 1;

            int iTopRight = iTop + 2;
            int iTopNextRow = iTop + cols * 2;
            int iTopNextRowRight = iTopNextRow + 2;

            int iBotRight = iBot + 2;
            int iBotNextRow = iBot + cols * 2;
            int iBotNextRowRight = iBotNextRow + 2;

            // --- Gornja površina ---
            indices.push_back(iTop);
            indices.push_back(iTopNextRow);
            indices.push_back(iTopRight);

            indices.push_back(iTopRight);
            indices.push_back(iTopNextRow);
            indices.push_back(iTopNextRowRight);

            // --- Dno ---
            indices.push_back(iBot);
            indices.push_back(iBotRight);
            indices.push_back(iBotNextRow);

            indices.push_back(iBotRight);
            indices.push_back(iBotNextRowRight);
            indices.push_back(iBotNextRow);

            // --- Bočne strane ---
            // Prednja
            indices.push_back(iTopRight);
            indices.push_back(iBot);
            indices.push_back(iTop);

            indices.push_back(iBotRight);
            indices.push_back(iBot);
            indices.push_back(iTopRight);

            // Leva
            indices.push_back(iTop);
            indices.push_back(iBotNextRow);
            indices.push_back(iTopNextRow);

            indices.push_back(iBot);
            indices.push_back(iBotNextRow);
            indices.push_back(iTop);

            // Zadnja
            indices.push_back(iTopNextRow);
            indices.push_back(iBotNextRowRight);
            indices.push_back(iTopNextRowRight);

            indices.push_back(iBotNextRow);
            indices.push_back(iBotNextRowRight);
            indices.push_back(iTopNextRow);

            // Desna
            indices.push_back(iTopNextRowRight);
            indices.push_back(iBotRight);
            indices.push_back(iTopRight);

            indices.push_back(iBotNextRowRight);
            indices.push_back(iBotRight);
            indices.push_back(iTopNextRowRight);
        }
    }

    // --- Normale za gornju površinu i bočne strane ---
    for (size_t i = 0; i < indices.size(); i += 3)
    {
        unsigned int i0 = indices[i];
        unsigned int i1 = indices[i + 1];
        unsigned int i2 = indices[i + 2];

        glm::vec3 v0 = vertices[i0].Position;
        glm::vec3 v1 = vertices[i1].Position;
        glm::vec3 v2 = vertices[i2].Position;

        glm::vec3 normal = glm::normalize(glm::cross(v1 - v0, v2 - v0));

        vertices[i0].Normal += normal;
        vertices[i1].Normal += normal;
        vertices[i2].Normal += normal;
    }

    // Normalize all normals
    for (auto& v : vertices)
        v.Normal = glm::normalize(v.Normal);

    std::vector<Texture> textures;
    return Mesh(vertices, indices, textures);
}

Mesh createCylinderMesh(float height, float radius, int segments)
{
    std::vector<Vertex> vertices;
    std::vector<unsigned int> indices;

    for (int i = 0; i <= segments; i++)
    {
        float angle = 2.0f * M_PI * i / segments;
        float x = cos(angle) * radius;
        float z = sin(angle) * radius;

        glm::vec3 normal = glm::normalize(glm::vec3(x, 0.0f, z));

        // bottom
        vertices.push_back({ {x, 0.0f, z}, normal, {0,0} });

        // top
        vertices.push_back({ {x, height, z}, normal, {0,1} });
    }

    int topCenter = vertices.size();
    vertices.push_back({ {0, height, 0}, {0,1,0}, {0,0} });

    for (int i = 0; i <= segments; i++)
    {
        float angle = 2.0f * M_PI * i / segments;
        float x = cos(angle) * radius;
        float z = sin(angle) * radius;

        vertices.push_back({ {x, height, z}, {0,1,0}, {0,0} });
    }

    for (int i = 0; i < segments; i++)
    {
        indices.push_back(topCenter);
        indices.push_back(topCenter + i + 2);
        indices.push_back(topCenter + i + 1);
    }

    for (int i = 0; i < segments; i++)
    {
        int b1 = i * 2;
        int t1 = b1 + 1;
        int b2 = (i + 1) * 2;
        int t2 = b2 + 1;

        indices.push_back(b1);
        indices.push_back(t1);
        indices.push_back(b2);

        indices.push_back(b2);
        indices.push_back(t1);
        indices.push_back(t2);
    }

    return Mesh(vertices, indices, {});
}

Mesh createSphereMesh(float radius = 1.0f, int sectors = 12, int stacks = 8)
{
    std::vector<Vertex> vertices;
    std::vector<unsigned int> indices;
    std::vector<Texture> textures;

    for (int i = 0; i <= stacks; ++i) {
        float stackAngle = glm::pi<float>() / 2 - i * glm::pi<float>() / stacks;
        float xy = radius * cos(stackAngle);
        float y = radius * sin(stackAngle);

        for (int j = 0; j <= sectors; ++j) {
            float sectorAngle = j * 2 * glm::pi<float>() / sectors;
            float x = xy * cos(sectorAngle);
            float z = xy * sin(sectorAngle);

            Vertex v;
            v.Position = glm::vec3(x, y, z);
            v.Normal = glm::normalize(v.Position);
            v.TexCoords = glm::vec2((float)j / sectors, (float)i / stacks);
            vertices.push_back(v);
        }
    }

    for (int i = 0; i < stacks; ++i) {
        int k1 = i * (sectors + 1);
        int k2 = k1 + sectors + 1;

        for (int j = 0; j < sectors; ++j, ++k1, ++k2) {
            if (i != 0) {
                indices.push_back(k1);
                indices.push_back(k1 + 1);
                indices.push_back(k2);
            }
            if (i != (stacks - 1)) {
                indices.push_back(k1 + 1);
                indices.push_back(k2 + 1);
                indices.push_back(k2);
            }
        }
    }

    return Mesh(vertices, indices, textures);
}

Mesh createCoinMesh(float radius, float thickness, int segments)
{
    std::vector<Vertex> vertices;
    std::vector<unsigned int> indices;

    float halfH = thickness / 2.0f;

    // =========================
    // TOP FACE
    // =========================

    int topCenterIndex = vertices.size();
    vertices.push_back({ {0.0f, halfH, 0.0f}, {0,1,0}, {0,0} });

    for (int i = 0; i <= segments; i++) {
        float angle = 2.0f * M_PI * i / segments;
        float x = cos(angle) * radius;
        float z = sin(angle) * radius;

        vertices.push_back({ {x, halfH, z}, {0,1,0}, {0,0} });
    }

    for (int i = 0; i < segments; i++) {
        indices.push_back(topCenterIndex);
        indices.push_back(topCenterIndex + i + 2);
        indices.push_back(topCenterIndex + i + 1);
    }

    // =========================
    // BOTTOM FACE
    // =========================

    int bottomCenterIndex = vertices.size();
    vertices.push_back({ {0.0f, -halfH, 0.0f}, {0,-1,0}, {0,0} });

    for (int i = 0; i <= segments; i++) {
        float angle = 2.0f * M_PI * i / segments;
        float x = cos(angle) * radius;
        float z = sin(angle) * radius;

        vertices.push_back({ {x, -halfH, z}, {0,-1,0}, {0,0} });
    }

    for (int i = 0; i < segments; i++) {
        indices.push_back(bottomCenterIndex);
        indices.push_back(bottomCenterIndex + i + 2);
        indices.push_back(bottomCenterIndex + i + 1);
    }

    // =========================
    // SIDE SURFACE
    // =========================

    int sideStart = vertices.size();

    for (int i = 0; i <= segments; i++) {
        float angle = 2.0f * M_PI * i / segments;
        float x = cos(angle) * radius;
        float z = sin(angle) * radius;

        glm::vec3 normal = glm::normalize(glm::vec3(x, 0.0f, z));

        // gornji
        vertices.push_back({ {x, halfH, z}, normal, {0,0} });
        // donji
        vertices.push_back({ {x, -halfH, z}, normal, {0,0} });
    }

    for (int i = 0; i < segments; i++) {
        int t1 = sideStart + i * 2;
        int b1 = t1 + 1;
        int t2 = sideStart + (i + 1) * 2;
        int b2 = t2 + 1;

        indices.push_back(t1);
        indices.push_back(t2);
        indices.push_back(b1);

        indices.push_back(t2);
        indices.push_back(b2);
        indices.push_back(b1);
    }

    std::vector<Texture> textures;

    return Mesh(vertices, indices, textures);
}

Mesh createGemMesh(float size)
{
    std::vector<Vertex> vertices = {
        {{ 0,  size,  0}, { 0, 1, 0}, {0.5f,1.0f}},  // top
        {{-size, 0,  0}, {-1, 0, 0}, {0.0f,0.5f}},
        {{ 0, 0,  size}, { 0, 0, 1}, {0.5f,0.5f}},
        {{ size, 0,  0}, { 1, 0, 0}, {1.0f,0.5f}},
        {{ 0, 0, -size}, { 0, 0,-1}, {0.5f,0.0f}},
        {{ 0, -size, 0}, { 0,-1, 0}, {0.5f,0.0f}}   // bottom
    };

    std::vector<unsigned int> indices = {
        // Gornji piramidalni deo
        0,1,2,
        0,2,3,
        0,3,4,
        0,4,1,

        // Donji deo
        5,2,1,
        5,3,2,
        5,4,3,
        5,1,4
    };

    std::vector<Texture> textures;

    return Mesh(vertices, indices, textures);
}

float getSandHeightAt(const Mesh& sand, float x, float z, int rows, int cols, float width, float depth)
{
    float dx = width / (cols - 1);
    float dz = depth / (rows - 1);

    // Pretvori globalne koordinate u indeks u mreži
    float fx = (x + width / 2) / dx;
    float fz = (z + depth / 2) / dz;

    int ix = glm::clamp((int)fx, 0, cols - 2);
    int iz = glm::clamp((int)fz, 0, rows - 2);

    float tx = fx - ix;
    float tz = fz - iz;

    // Dobij gornje vrhove iz vertices
    auto getTopY = [&](int row, int col) {
        int idx = (row * cols + col) * 2; // top vertex je svaki drugi
        return sand.vertices[idx].Position.y;
        };

    // Bilinearna interpolacija između 4 susedna vrha
    float h00 = getTopY(iz, ix);
    float h10 = getTopY(iz, ix + 1);
    float h01 = getTopY(iz + 1, ix);
    float h11 = getTopY(iz + 1, ix + 1);

    float h0 = glm::mix(h00, h10, tx);
    float h1 = glm::mix(h01, h11, tx);

    return glm::mix(h0, h1, tz);
}

struct Overlay {
    unsigned int VAO, VBO, textureID;
    float width, height; // u pikselima

    Overlay(const std::string& texPath, float w, float h) : width(w), height(h) {
        float vertices[] = {
            // x, y, u, v
            0.0f, 0.0f, 0.0f, 0.0f,        // bottom-left
            w,    0.0f, 1.0f, 0.0f,        // bottom-right
            w,    h,    1.0f, 1.0f,        // top-right
            0.0f, h,    0.0f, 1.0f         // top-left
        };
        unsigned int indices[] = { 0,1,2, 2,3,0 };

        glGenVertexArrays(1, &VAO);
        glGenBuffers(1, &VBO);
        unsigned int EBO;
        glGenBuffers(1, &EBO);

        glBindVertexArray(VAO);

        glBindBuffer(GL_ARRAY_BUFFER, VBO);
        glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);

        glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0);
        glEnableVertexAttribArray(0);

        glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)(2 * sizeof(float)));
        glEnableVertexAttribArray(1);

        // Load texture
        textureID = loadTexture(texPath.c_str());
    }

    void Draw(Shader& shader, float screenWidth, float screenHeight, float posX = 10.0f, float posY = 10.0f) {
        shader.use();

        bool prevCull = cullFaceEnabled;
        bool prevDepth = depthTestEnabled;

        glDisable(GL_DEPTH_TEST);
        glDisable(GL_CULL_FACE);
        glDepthMask(GL_FALSE);

        glm::mat4 ortho = glm::ortho(0.0f, screenWidth, 0.0f, screenHeight);
        shader.setMat4("projection", ortho);
        shader.setInt("overlayTex", 0);

        glm::mat4 model = glm::mat4(1.0f);
        model = glm::translate(model, glm::vec3(posX, screenHeight - posY - height, 0.0f));
        shader.setMat4("model", model);

        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, textureID);
        glBindVertexArray(VAO);
        glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);

        glDepthMask(GL_TRUE);
        if (prevCull) glEnable(GL_CULL_FACE); else glDisable(GL_CULL_FACE);
        if (prevDepth) glEnable(GL_DEPTH_TEST); else glDisable(GL_DEPTH_TEST);
    }
};

struct AquariumBounds {
    float minX, maxX;
    float minY, maxY;
    float minZ, maxZ;
};

struct Bubble {
    glm::vec3 position;
    float speed;
    float radius;
    float driftPhase = 0.0f;
    float driftAmplitude = 0.1f;
};

struct FoodParticle {
    glm::vec3 position;
    float speed;
    float radius;
    float targetY;
    bool alive = true;
};

class AlgaeBush {
public:
    std::vector<Mesh> stems;
    std::vector<glm::vec3> basePositions;
    std::vector<float> swayOffsets;

    AlgaeBush(glm::vec3 center, int count = 20)
    {
        for (int i = 0; i < count; i++)
        {
            float offsetX = ((rand() % 100) / 100.0f - 0.5f) * 0.75f;
            float offsetZ = ((rand() % 100) / 100.0f - 0.5f) * 0.75f;

            float height = 1.75f + ((rand() % 100) / 100.0f) * 1.75f;  
            float width = 0.03f + ((rand() % 100) / 100.0f) * 0.02f; // 0.03 - 0.05

            stems.push_back(createCylinderMesh(height, width, 12));

            basePositions.push_back(glm::vec3(center.x + offsetX, 0.0f, center.z + offsetZ));

            swayOffsets.push_back(((rand() % 100) / 100.0f) * 3.14f * 10.0f); // random faza
        }
    }

    void Draw(Shader& shader, float time)
    {
        shader.use();
        shader.setVec4("uColor", glm::vec4(0.0f, 0.7f, 0.2f, 1.0f));

        for (int i = 0; i < stems.size(); i++)
        {
            glm::mat4 model = glm::mat4(1.0f);
            model = glm::translate(model, basePositions[i]);

            // Lagano njišenje stabljike
            float sway = sin(time + swayOffsets[i]) * 0.2f; // amplitude
            model = glm::rotate(model, sway, glm::vec3(0, 0, 1)); // njiši po Z osi

            shader.setMat4("model", model);
            stems[i].Draw(shader);
        }
    }
};

class Aquarium {
public:
    std::vector<Mesh> glass; // 0: front, 1: back, 2: left, 3: right
    std::vector<Mesh> frame;
    Mesh bottom;
    Mesh sand;
    AquariumBounds bounds;
    AlgaeBush algaeBush1;
    AlgaeBush algaeBush2;

    Aquarium() : bottom(createCubeMesh(glm::vec3(tankWidth, wallThickness, tankDepth), false)), sand(createSandMeshFilled(sandRows, sandCols, sandWidth, sandDepth, sandHeight)), algaeBush1(AlgaeBush(glm::vec3(-tankWidth / 4, sandHeight, -tankDepth / 4), 25)), algaeBush2(AlgaeBush(glm::vec3(tankWidth / 4, sandHeight, tankDepth / 5), 30)) {
        float gt = wallThickness;
        float gw = tankWidth;
        float gh = tankHeight;
        float gd = tankDepth;

        glass.push_back(createCubeMesh(glm::vec3(gw, gh, gt), false));
        glass.push_back(createCubeMesh(glm::vec3(gw, gh, gt), false));
        glass.push_back(createCubeMesh(glm::vec3(gt, gh, gd), false));
        glass.push_back(createCubeMesh(glm::vec3(gt, gh, gd), false));

        float bw = wallThickness;
        float bh = tankHeight;
        float bd = wallThickness;

        frame.push_back(createCubeMesh(glm::vec3(bw, bh, bd), false)); // front-left
        frame.push_back(createCubeMesh(glm::vec3(bw, bh, bd), false)); // front-right
        frame.push_back(createCubeMesh(glm::vec3(bw, bh, bd), false)); // back-left
        frame.push_back(createCubeMesh(glm::vec3(bw, bh, bd), false)); // back-right

        computeBounds();
    }

    void computeBounds()
    {
        float halfW = tankWidth / 2.0f;
        float halfD = tankDepth / 2.0f;

        bounds.minX = -halfW + wallThickness;
        bounds.maxX = halfW - wallThickness;

        bounds.minZ = -halfD + wallThickness;
        bounds.maxZ = halfD - wallThickness;

        bounds.minY = sandHeight;
        bounds.maxY = tankHeight - wallThickness;
    }

    const AquariumBounds& getBounds() const {
        return bounds;
    }

    void Draw(Shader& basicShader, Shader& sandShader, unsigned int sandTex, float time)
    {
        bool prevCull = cullFaceEnabled;
        bool prevDepth = depthTestEnabled;

        glm::mat4 model;

        basicShader.use();
        basicShader.setVec4("uColor", glm::vec4(0, 0, 0, 1));

        model = glm::translate(glm::mat4(1.0f),
            glm::vec3(0, -wallThickness / 2, 0));
        basicShader.setMat4("model", model);
        bottom.Draw(basicShader);

        algaeBush1.Draw(basicShader, time);
        algaeBush2.Draw(basicShader, time);

        sandShader.use();
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, sandTex);
        model = glm::translate(glm::mat4(1.0f),
            glm::vec3(0, 0.01f, 0));
        sandShader.setMat4("model", model);
        sand.Draw(sandShader);

        basicShader.use();
        basicShader.setVec4("uColor", glm::vec4(0, 0, 0, 1));

        float x = tankWidth / 2 - wallThickness / 2;
        float z = tankDepth / 2 - wallThickness / 2;
        float y = tankHeight / 2;

        //glDisable(GL_CULL_FACE);

        // Front-left
        model = glm::translate(glm::mat4(1.0f), glm::vec3(-x, y, -z));
        basicShader.setMat4("model", model);
        frame[0].Draw(basicShader);

        // Front-right
        model = glm::translate(glm::mat4(1.0f), glm::vec3(x, y, -z));
        basicShader.setMat4("model", model);
        frame[1].Draw(basicShader);

        // Back-left
        model = glm::translate(glm::mat4(1.0f), glm::vec3(-x, y, z));
        basicShader.setMat4("model", model);
        frame[2].Draw(basicShader);

        // Back-right
        model = glm::translate(glm::mat4(1.0f), glm::vec3(x, y, z));
        basicShader.setMat4("model", model);
        frame[3].Draw(basicShader);

        basicShader.use();
        basicShader.setVec4("uColor", glm::vec4(0.6f, 0.8f, 1.0f, 0.2f));

        //if (prevCull) glEnable(GL_CULL_FACE); else glDisable(GL_CULL_FACE);

        glDisable(GL_CULL_FACE);
        glDepthMask(GL_FALSE);

        // Back
        model = glm::translate(glm::mat4(1.0f),
            glm::vec3(0, tankHeight / 2, tankDepth / 2 - wallThickness / 2));
        basicShader.setMat4("model", model);
        glass[1].Draw(basicShader);

        // Left
        model = glm::translate(glm::mat4(1.0f),
            glm::vec3(-tankWidth / 2 + wallThickness / 2, tankHeight / 2, 0));
        basicShader.setMat4("model", model);
        glass[2].Draw(basicShader);

        // Right
        model = glm::translate(glm::mat4(1.0f),
            glm::vec3(tankWidth / 2 - wallThickness / 2, tankHeight / 2, 0));
        basicShader.setMat4("model", model);
        glass[3].Draw(basicShader);

        // Front
        model = glm::translate(glm::mat4(1.0f),
            glm::vec3(0, tankHeight / 2, -tankDepth / 2 + wallThickness / 2));
        basicShader.setMat4("model", model);
        glass[0].Draw(basicShader);

        glDepthMask(GL_TRUE);
        if (prevCull) glEnable(GL_CULL_FACE); else glDisable(GL_CULL_FACE);
        if (prevDepth) glEnable(GL_DEPTH_TEST); else glDisable(GL_DEPTH_TEST);
    }
};

class Chest {
public:
    std::vector<Mesh> sides;
    Mesh lid;
    Mesh coin;
    Mesh gem;

    float wallThickness = 0.1f;
    float width = 2.0f;
    float height = 0.6f;
    float depth = 1.0f;

    glm::vec3 position;
    float lidAngle = 0.0f; // 0 = zatvoren, >0 = otvoren
    bool opening = false;

    struct AABB {
        glm::vec3 min; // minimalna tačka
        glm::vec3 max; // maksimalna tačka
    };

    AABB getBodyAABB() const {
        glm::vec3 halfSize(width / 2.0f, height / 2.0f, depth / 2.0f);
        return { position - halfSize, position + halfSize };
    }

    AABB getLidAABB() const {
        glm::mat4 lidTransform = glm::mat4(1.0f);
        // Šarka pozadi kao u draw
        lidTransform = glm::translate(lidTransform, position + glm::vec3(0.0f, height, -depth / 2.0f));
        lidTransform = glm::rotate(lidTransform, -lidAngle, glm::vec3(1, 0, 0));
        lidTransform = glm::translate(lidTransform, glm::vec3(0.0f, 0.1f, 0.5f)); // offset poklopca

        glm::vec3 halfLidSize(1.0f, 0.1f, 0.5f);
        glm::vec3 minLocal = -halfLidSize;
        glm::vec3 maxLocal = halfLidSize;

        glm::vec3 corners[8] = {
            {minLocal.x, minLocal.y, minLocal.z}, {maxLocal.x, minLocal.y, minLocal.z},
            {minLocal.x, maxLocal.y, minLocal.z}, {maxLocal.x, maxLocal.y, minLocal.z},
            {minLocal.x, minLocal.y, maxLocal.z}, {maxLocal.x, minLocal.y, maxLocal.z},
            {minLocal.x, maxLocal.y, maxLocal.z}, {maxLocal.x, maxLocal.y, maxLocal.z}
        };

        glm::vec3 globalMin(FLT_MAX), globalMax(-FLT_MAX);
        for (int i = 0; i < 8; i++) {
            glm::vec3 transformed = glm::vec3(lidTransform * glm::vec4(corners[i], 1.0f));
            globalMin = glm::min(globalMin, transformed);
            globalMax = glm::max(globalMax, transformed);
        }

        return { globalMin, globalMax };
    }

    Chest(const std::string& bodyTex, const std::string& lidTex, glm::vec3 pos)
        : position(pos), lid(createCubeMesh(glm::vec3(2.0f, 0.2f, 1.0f), false, lidTex)), coin(createCoinMesh(0.15, 0.03f, 32)), gem(createGemMesh(0.18f))
    {
        sides.push_back(createCubeMesh(glm::vec3(width, height, wallThickness), false, bodyTex));
        sides.push_back(createCubeMesh(glm::vec3(width, height, wallThickness), false, bodyTex));
        sides.push_back(createCubeMesh(glm::vec3(wallThickness, height, depth), false, bodyTex));
        sides.push_back(createCubeMesh(glm::vec3(wallThickness, height, depth), false, bodyTex));
        sides.push_back(createCubeMesh(glm::vec3(width, wallThickness, depth), false, bodyTex));
    }

    void update(float deltaTime)
    {
        if (opening && lidAngle < glm::radians(110.0f))
            lidAngle += deltaTime * glm::radians(60.0f);
        else if (!opening && lidAngle > 0.0f)
            lidAngle -= deltaTime * glm::radians(60.0f);
    }

    void toggle()
    {
        opening = !opening;
    }

    void draw(Shader& textureShader, Shader& basicShader)
    {
        float innerBackZ = position.z - depth / 2 + wallThickness + 0.05f;
        float bottomY = position.y + wallThickness + 0.075f;

        // --- Treasure light aktivno za kovčeg ---
        glm::vec3 gemCenter = glm::vec3(position.x, bottomY + 0.2f, innerBackZ + 0.2f);
        glm::vec3 gemColor = glm::vec3(0.0f, 0.8f, 1.0f); 
        float gemIntensity = 0.05f;

        glm::vec3 coin1Center = glm::vec3(position.x - width * 0.3f, bottomY + 0.05f, innerBackZ + 0.075f);
        glm::vec3 coin2Center = glm::vec3(position.x + width * 0.25f, bottomY + 0.15f, innerBackZ + 0.05f);
        glm::vec3 coinColor = glm::vec3(1.0f, 0.84f, 0.0f);
        float coinIntensity = 0.05f;

        if (lidAngle > glm::radians(1.0f)) {
            basicShader.use();
            basicShader.setBool("uTreasureLightEnabled", true);
            basicShader.setVec3("uGemLightPos", gemCenter);
            basicShader.setVec3("uGemLightColor", gemColor);
            basicShader.setFloat("uGemLightIntensity", gemIntensity);

            basicShader.setVec3("uCoin1LightPos", coin1Center);
            basicShader.setVec3("uCoin2LightPos", coin2Center);
            basicShader.setVec3("uCoinLightColor", coinColor);
            basicShader.setFloat("uCoinLightIntensity", coinIntensity);

            // Coin 1
            glm::mat4 model = glm::mat4(1.0f);
            model = glm::translate(glm::mat4(1.0f), coin1Center);
            model = glm::rotate(model, glm::radians(60.0f), glm::vec3(1, 0, 0));
            model = glm::scale(model, glm::vec3(1.5f));
            basicShader.setMat4("model", model);
            basicShader.setVec4("uColor", glm::vec4(1.0f, 0.84f, 0.0f, 1.0f));
            coin.Draw(basicShader);

            // Coin 2
            model = glm::translate(glm::mat4(1.0f), coin2Center);
            model = glm::rotate(model, glm::radians(75.0f), glm::vec3(1, 0, 0));
            model = glm::scale(model, glm::vec3(1.5f));
            basicShader.setMat4("model", model);
            basicShader.setVec4("uColor", glm::vec4(1.0f, 0.84f, 0.0f, 1.0f));
            coin.Draw(basicShader);

            // Gem
            model = glm::translate(glm::mat4(1.0f), gemCenter);
            model = glm::rotate(model, glm::radians(78.0f), glm::vec3(1, 0, 0));
            model = glm::scale(model, glm::vec3(1.5f));
            basicShader.setMat4("model", model);
            basicShader.setVec4("uColor", glm::vec4(0.0f, 0.8f, 1.0f, 1.0f));
            gem.Draw(basicShader);

            // --- Isključi treasure light ---
            basicShader.setBool("uTreasureLightEnabled", false);

            textureShader.use();
            textureShader.setBool("uTreasureLightEnabled", true);
            textureShader.setVec3("uGemLightPos", gemCenter);
            textureShader.setVec3("uGemLightColor", gemColor);
            textureShader.setFloat("uGemLightIntensity", gemIntensity);

            textureShader.setVec3("uCoin1LightPos", coin1Center);
            textureShader.setVec3("uCoin2LightPos", coin2Center);
            textureShader.setVec3("uCoinLightColor", coinColor);
            textureShader.setFloat("uCoinLightIntensity", coinIntensity);
        }

        // --- Telo kovčega ---

        // Back
        textureShader.use();
        glm::mat4 model = glm::mat4(1.0f);
        model = glm::translate(glm::mat4(1.0f),
            position + glm::vec3(0, height / 2, depth / 2 - this->wallThickness / 2));
        textureShader.setMat4("model", model);
        sides[1].Draw(textureShader);

        // Left
        model = glm::translate(glm::mat4(1.0f),
            position + glm::vec3(-width / 2 + this->wallThickness / 2, height / 2, 0));
        textureShader.setMat4("model", model);
        sides[2].Draw(textureShader);

        // Right
        model = glm::translate(glm::mat4(1.0f),
            position + glm::vec3(width / 2 - this->wallThickness / 2, height / 2, 0));
        textureShader.setMat4("model", model);
        sides[3].Draw(textureShader);

        // Front
        model = glm::translate(glm::mat4(1.0f),
            position + glm::vec3(0, height / 2, -depth / 2 + this->wallThickness / 2));
        textureShader.setMat4("model", model);
        sides[0].Draw(textureShader);

        // Bottom
        model = glm::translate(glm::mat4(1.0f),
            position + glm::vec3(0, this->wallThickness / 2, 0));
        textureShader.setMat4("model", model);
        sides[4].Draw(textureShader);

        // --- Poklopac ---
        glm::mat4 lidModel = glm::mat4(1.0f);
        lidModel = glm::translate(lidModel, position + glm::vec3(0.0f, height, -depth / 2.0f)); // šarka pozadi
        lidModel = glm::rotate(lidModel, -lidAngle, glm::vec3(1, 0, 0));
        lidModel = glm::translate(lidModel, glm::vec3(0.0f, 0.1f, 0.5f)); // pomeraj da se poklopac lepo rotira

        textureShader.setMat4("model", lidModel);
        lid.Draw(textureShader);
        textureShader.setBool("uTreasureLightEnabled", false);
    }

    bool checkFishCollisionWithAABB(glm::vec3 fishPosition, float fishRadius, const Chest::AABB& box)
    {
        // Pronađi najbližu tačku u kutiji
        glm::vec3 closestPoint = glm::clamp(fishPosition, box.min, box.max);

        // Izračunaj udaljenost od centra ribe do najbliže tačke na kutiji
        float distance = glm::length(fishPosition - closestPoint);

        return distance < fishRadius;
    }
};

class Fish {
public:
    Model* model;
    glm::vec3 position;
    glm::vec3 direction;
    glm::vec3 baseRotation;
    float speed;
    float scale;
    glm::vec3 lastHorizontalDir = glm::vec3(0.0f, 0.0f, 1.0f);
    std::vector<Bubble> bubbles;

    Fish(Model* model, glm::vec3 startPos, glm::vec3 baseRotation, float speed = 3.0f, float scale = 1.0f) : model(model), position(startPos), baseRotation(baseRotation), speed(speed), scale(scale)
    {
        direction = glm::vec3(0.0f, 0.0f, 1.0f);
    }

    void emitBubbles()
    {
        glm::vec3 forward = glm::normalize(lastHorizontalDir);
        glm::vec3 up = glm::vec3(0.0f, 1.0f, 0.0f);
        glm::vec3 right = glm::normalize(glm::cross(up, forward));

        float fishLength = (model->maxBounds.z - model->minBounds.z) * scale;
        float mouthOffset = fishLength * 0.5f + 0.05f;
        float verticalOffset = 0.1f;
        float lateralSpread = 0.25f;
        float worldXSpread = 0.15f; // dodatni X spread za vizuelni efekat

        for (int i = 0; i < 3; i++) {
            Bubble b;

            float lateralOffset = ((rand() % 100) / 100.0f - 0.5f) * 2.0f * lateralSpread;
            float worldXOffset = ((rand() % 100) / 100.0f - 0.5f) * 2.0f * worldXSpread;

            b.position = position
                + forward * mouthOffset
                + right * lateralOffset
                + up * verticalOffset
                + glm::vec3(worldXOffset, 0.0f, 0.0f);

            b.speed = 0.8f + (rand() % 100) / 200.0f;

            float baseRadius = 0.075f;
            float variation = 0.02f;
            b.radius = baseRadius + ((rand() % 100) / 100.0f - 0.5f) * 2.0f * variation;

            bubbles.push_back(b);
        }
    }

    void update(float deltaTime, glm::vec3 inputDir, AquariumBounds bounds, Chest& chest)
    {
        if (glm::length(inputDir) > 0.001f)
        {
            if (abs(direction.x) > 0.001f || abs(direction.z) > 0.001f)
            {
                lastHorizontalDir = glm::normalize(glm::vec3(direction.x, 0.0f, direction.z));
            }
            direction = glm::normalize(inputDir);

            glm::vec3 proposedPos = position + direction * speed * deltaTime;

            // Dobijanje realnih min/max za ribu u svetu (scaled)
            glm::vec3 fishMin = proposedPos + model->minBounds * scale;
            glm::vec3 fishMax = proposedPos + model->maxBounds * scale;

            if (fishMin.x < bounds.minX) proposedPos.x += (bounds.minX - fishMin.x);
            if (fishMax.x > bounds.maxX) proposedPos.x -= (fishMax.x - bounds.maxX);

            if (fishMin.y < bounds.minY) proposedPos.y += (bounds.minY - fishMin.y);
            if (fishMax.y > bounds.maxY) proposedPos.y -= (fishMax.y - bounds.maxY);

            if (fishMin.z < bounds.minZ) proposedPos.z += (bounds.minZ - fishMin.z);
            if (fishMax.z > bounds.maxZ) proposedPos.z -= (fishMax.z - bounds.maxZ);

            // --- Kolizija sa kovčegom ---
            Chest::AABB bodyBox = chest.getBodyAABB();
            Chest::AABB lidBox = chest.getLidAABB();

            // Uzmemo centar ribe i njen "radius"
            glm::vec3 size = this->model->maxBounds - this->model->minBounds;
            float baseRadius = glm::length(size) * 0.5f;
            float fishRadius = baseRadius * this->scale;

            if (chest.checkFishCollisionWithAABB(proposedPos, fishRadius, bodyBox) ||
                chest.checkFishCollisionWithAABB(proposedPos, fishRadius, lidBox)) {
                proposedPos = position; // blokiraj kretanje
            }

            position = proposedPos;
        }
        else
        {
            direction = glm::vec3(0.0f);
        }

        for (int i = 0; i < bubbles.size(); i++) {
            bubbles[i].driftPhase += deltaTime * 2.0f;

            float drift = sin(bubbles[i].driftPhase) * bubbles[i].driftAmplitude * deltaTime;

            bubbles[i].position.y += bubbles[i].speed * deltaTime;
            bubbles[i].position.x += drift * 4.0f;

            if (bubbles[i].position.y > bounds.maxY) {
                bubbles.erase(bubbles.begin() + i);
                i--;
            }
        }
    }

    void draw(Shader& shader)
    {
        shader.use();
        glm::mat4 modelMat = glm::mat4(1.0f);
        modelMat = glm::translate(modelMat, position);

        float yawAngle = atan2(-lastHorizontalDir.z, lastHorizontalDir.x);
        modelMat = glm::rotate(modelMat, yawAngle, glm::vec3(0.0f, 1.0f, 0.0f));

        float pitchAngle = 0.0f;
        if (glm::length(direction) > 0.001f)
        {
            pitchAngle = direction.y * glm::radians(25.0f);
        }
        modelMat = glm::rotate(modelMat, pitchAngle, glm::vec3(0.0f, 0.0f, 1.0f));

        modelMat = glm::rotate(modelMat, glm::radians(baseRotation.x), glm::vec3(1, 0, 0));
        modelMat = glm::rotate(modelMat, glm::radians(baseRotation.y), glm::vec3(0, 1, 0));
        modelMat = glm::rotate(modelMat, glm::radians(baseRotation.z), glm::vec3(0, 0, 1));

        modelMat = glm::scale(modelMat, glm::vec3(scale));

        shader.setMat4("model", modelMat);
        model->Draw(shader);
    }

    void drawBubbles(Shader& shader, Mesh& bubbleMesh)
    {
        shader.use();
        shader.setVec4("uColor", glm::vec4(0.9f, 0.95f, 1.0f, 0.7f));

        for (auto& b : bubbles) {
            glm::mat4 model = glm::mat4(1.0f);
            model = glm::translate(model, b.position);
            model = glm::scale(model, glm::vec3(b.radius));
            shader.setMat4("model", model);
            bubbleMesh.Draw(shader);
        }
    }
};

class FoodSystem {
public:
    std::vector<FoodParticle> foods;
    Mesh* foodMesh;
    AquariumBounds bounds;
    float sandY;
    float targetY;

    FoodSystem(Mesh* mesh, AquariumBounds bounds, float sandY)
        : foodMesh(mesh), bounds(bounds), sandY(sandY), targetY(0.0f) {}

    void spawnFood(Aquarium& aquarium, int count = 5)
    {
        for (int i = 0; i < count; i++) {
            float x = bounds.minX + static_cast<float>(rand()) / RAND_MAX * (bounds.maxX - bounds.minX - 0.1f);
            float z = bounds.minZ + static_cast<float>(rand()) / RAND_MAX * (bounds.maxZ - bounds.minZ - 0.1f);

            float targetY = getSandHeightAt(aquarium.sand, x, z, sandRows, sandCols, sandWidth, sandDepth);

            FoodParticle f;
            f.position = glm::vec3(x, bounds.maxY + 0.5f, z);
            f.speed = 0.8f + ((rand() % 100) / 100.0f) * 0.5f;
            f.radius = 0.05f + ((rand() % 100) / 100.0f) * 0.05f;
            f.alive = true;
            f.targetY = targetY + f.radius;

            foods.push_back(f);
        }
    }

    bool checkFishEatsFood(const Fish& fish, const FoodParticle& food)
    {
        glm::vec3 size = fish.model->maxBounds - fish.model->minBounds;
        float baseRadius = glm::length(size) * 0.5f;
        float fishRadius = baseRadius * fish.scale;

        float foodRadius = food.radius;
        float dist = glm::length(fish.position - food.position);

        return dist <= (fishRadius + foodRadius);
    }

    void handleEating(Fish& fish)
    {
        for (auto& food : foods)
        {
            if (!food.alive) continue;

            if (checkFishEatsFood(fish, food))
            {
                food.alive = false;

                fish.scale *= 1.01f;
            }
        }
    }

    void update(float deltaTime)
    {
        for (auto& f : foods) {
            if (!f.alive) continue;

            f.position.y -= f.speed * deltaTime;

            if (f.position.y < f.targetY) {
                f.position.y = f.targetY;
            }
        }
    }

    void draw(Shader& shader)
    {
        shader.use();
        shader.setVec4("uColor", glm::vec4(0.7f, 0.5f, 0.2f, 1.0f)); 

        for (auto& f : foods) {
            if (!f.alive) continue;

            glm::mat4 model = glm::mat4(1.0f);
            model = glm::translate(model, f.position);
            model = glm::scale(model, glm::vec3(f.radius));
            shader.setMat4("model", model);
            foodMesh->Draw(shader);
        }
    }
};

void getMonitorResolution()
{
    GLFWmonitor* primaryMonitor = glfwGetPrimaryMonitor();
    const GLFWvidmode* videoMode = glfwGetVideoMode(primaryMonitor);
    screenWidth = videoMode->width;
    screenHeight = videoMode->height;
    aspect = (float)screenWidth / screenHeight;
}

int endProgram(std::string message) {
    std::cout << message << std::endl;
    glfwTerminate();
    return -1;
}

void processInput(Fish& goldfish, Fish& clownfish, FoodSystem& foodSystem, Aquarium& aquarium, Chest& chest)
{
    goldfishInput = glm::vec3(0.0f);
    clownfishInput = glm::vec3(0.0f);

    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS) {
        glfwSetWindowShouldClose(window, true);
    }

    if (glfwGetKey(window, GLFW_KEY_1) == GLFW_PRESS)
    {
        depthTestEnabled = true;
    }
    if (glfwGetKey(window, GLFW_KEY_2) == GLFW_PRESS)
    {
        depthTestEnabled = false;
    }

    if (glfwGetKey(window, GLFW_KEY_3) == GLFW_PRESS)
    {
        cullFaceEnabled = true;
    }
    if (glfwGetKey(window, GLFW_KEY_4) == GLFW_PRESS)
    {
        cullFaceEnabled = false;
    }

    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) goldfishInput.z -= 1.0f;
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) goldfishInput.z += 1.0f;
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS) goldfishInput.x -= 1.0f;
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS) goldfishInput.x += 1.0f;
    if (glfwGetKey(window, GLFW_KEY_Q) == GLFW_PRESS) goldfishInput.y += 1.0f;
    if (glfwGetKey(window, GLFW_KEY_E) == GLFW_PRESS) goldfishInput.y -= 1.0f;

    if (glfwGetKey(window, GLFW_KEY_UP) == GLFW_PRESS) clownfishInput.z -= 1.0f;
    if (glfwGetKey(window, GLFW_KEY_DOWN) == GLFW_PRESS) clownfishInput.z += 1.0f;
    if (glfwGetKey(window, GLFW_KEY_LEFT) == GLFW_PRESS) clownfishInput.x -= 1.0f;
    if (glfwGetKey(window, GLFW_KEY_RIGHT) == GLFW_PRESS) clownfishInput.x += 1.0f;
    if (glfwGetKey(window, GLFW_KEY_K) == GLFW_PRESS) clownfishInput.y += 1.0f;
    if (glfwGetKey(window, GLFW_KEY_L) == GLFW_PRESS) clownfishInput.y -= 1.0f;

    if (glfwGetKey(window, GLFW_KEY_Z) == GLFW_PRESS) {
        if (!isZPressed) {
            goldfish.emitBubbles();
            isZPressed = true;
        }
    }
    if (glfwGetKey(window, GLFW_KEY_Z) == GLFW_RELEASE) {
        isZPressed = false;
    }

    if (glfwGetKey(window, GLFW_KEY_X) == GLFW_PRESS) {
        if (!isXPressed) {
            clownfish.emitBubbles();
            isXPressed = true;
        }
    }
    if (glfwGetKey(window, GLFW_KEY_X) == GLFW_RELEASE) {
        isXPressed = false;
    }

    if (glfwGetKey(window, GLFW_KEY_F) == GLFW_PRESS) {
        if (!isFPressed) {
            foodSystem.spawnFood(aquarium, 8);
            isFPressed = true;
        }
    }
    if (glfwGetKey(window, GLFW_KEY_F) == GLFW_RELEASE) {
        isFPressed = false;
    }

    if (glfwGetKey(window, GLFW_KEY_C) == GLFW_PRESS) {
        if (!isCPressed) {
            chest.toggle();
            isCPressed = true;
        }
    }
    if (glfwGetKey(window, GLFW_KEY_C) == GLFW_RELEASE) {
        isCPressed = false;
    }
}

void applyGlobalGLState()
{
    if (depthTestEnabled) glEnable(GL_DEPTH_TEST);
    else glDisable(GL_DEPTH_TEST);

    if (cullFaceEnabled) glEnable(GL_CULL_FACE);
    else glDisable(GL_CULL_FACE);

    glCullFace(GL_BACK);
}

int main()
{
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    getMonitorResolution();
    window = glfwCreateWindow(screenWidth, screenHeight, "Akvarijum", glfwGetPrimaryMonitor(), NULL);
    if (window == NULL) return endProgram("Prozor nije uspeo da se kreira.");
    glfwMakeContextCurrent(window);

    if (glewInit() != GLEW_OK) return endProgram("GLEW nije uspeo da se inicijalizuje.");

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glViewport(0, 0, screenWidth, screenHeight);

    glEnable(GL_DEPTH_TEST);
    glDepthMask(GL_TRUE);
    glCullFace(GL_BACK);
    glFrontFace(GL_CCW);
    glEnable(GL_CULL_FACE);

    glfwSwapInterval(0);

    glm::vec3 lightPos(6.0f, 10.0f, 6.0f);
    glm::vec3 cameraPos(0.0f, 7.0f, 9.0f);
    projection = glm::perspective(glm::radians(60.0f), aspect, 0.1f, 100.0f);
    view = glm::lookAt(
        cameraPos, 
        glm::vec3(0.0f, 2.0f, 0.0f),   
        glm::vec3(0.0f, 1.0f, 0.0f)    
    );

    Shader basicShader("basic.vert", "basic.frag");
    Shader textureShader("texture.vert", "texture.frag");
    Shader fishShader("fish.vert", "fish.frag");
    Shader overlayShader("overlay.vert", "overlay.frag");

    unsigned int sandTex = loadTexture("sand.jpg");

    Aquarium aquarium;

    Model goldfishModel("res/goldfish.obj");
    Fish goldfish(&goldfishModel, glm::vec3(0.0f, 2.0f, 0.0f), glm::vec3(-90.0f, 0.0f, 0.0f), 3.0f, 0.1f);

    Model clownfishModel("res/clownfish.obj");
    Fish clownfish(&clownfishModel, glm::vec3(3.0f, 2.0f, 0.0f), glm::vec3(0.0f, -90.0f, 0.0f), 3.0f, 0.5f);

    Mesh bubbleMesh = createSphereMesh(1.0f, 12, 8);
    Mesh foodmesh = createSphereMesh(1.0f, 10, 6);

    FoodSystem foodSystem(&foodmesh, aquarium.bounds, sandHeight);

    Chest chest("wood.png", "wood.png", glm::vec3(-3.0f, 0.8f, 2.0f));

    Overlay signatureOverlay("potpis.png", 256, 64);

    glClearColor(0.12f, 0.5f, 0.88f, 1.0f);

    basicShader.use();
    basicShader.setMat4("projection", projection);
    basicShader.setMat4("view", view);
    basicShader.setVec3("uLightPos", lightPos);
    basicShader.setVec3("uViewPos", cameraPos);       
    basicShader.setVec3("uLightColor", glm::vec3(1.0f)); 

    textureShader.use();
    textureShader.setMat4("projection", projection);
    textureShader.setMat4("view", view);
    textureShader.setVec3("uLightPos", lightPos);
    textureShader.setVec3("uViewPos", cameraPos);
    textureShader.setVec3("uLightColor", glm::vec3(1.0f));
    textureShader.setInt("uTex", 0);

    fishShader.use();
    fishShader.setMat4("projection", projection);
    fishShader.setMat4("view", view);
    fishShader.setVec3("uLightPos", lightPos);
    fishShader.setVec3("uViewPos", cameraPos);
    fishShader.setVec3("uLightColor", glm::vec3(1.0f));
    fishShader.setInt("uDiffMap", 0); 

    auto previous = std::chrono::high_resolution_clock::now();

    while (!glfwWindowShouldClose(window))
    {
        auto now = std::chrono::high_resolution_clock::now();
        float deltaTime = std::chrono::duration<float>(now - previous).count();
        previous = now;

        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        processInput(goldfish, clownfish, foodSystem, aquarium, chest);

        applyGlobalGLState();

        aquarium.Draw(basicShader, textureShader, sandTex, deltaTime);

        goldfish.update(deltaTime, goldfishInput, aquarium.getBounds(), chest);
        clownfish.update(deltaTime, clownfishInput, aquarium.getBounds(), chest); 
        foodSystem.update(deltaTime);
        chest.update(deltaTime);

        foodSystem.handleEating(goldfish);
        foodSystem.handleEating(clownfish);

        goldfish.draw(fishShader);
        goldfish.drawBubbles(basicShader, bubbleMesh);
        clownfish.draw(fishShader);
        clownfish.drawBubbles(basicShader, bubbleMesh);
        foodSystem.draw(basicShader);
        chest.draw(textureShader, basicShader);

        signatureOverlay.Draw(overlayShader, screenWidth, screenHeight, 10.0f, 10.0f);

        glfwSwapBuffers(window); 
        glfwPollEvents(); 

        auto frameTime = std::chrono::high_resolution_clock::now() - now;
        if (frameTime < targetFrameDuration) {
            std::this_thread::sleep_for(targetFrameDuration - frameTime);
        }
    }

    glfwTerminate();
    return 0;
}