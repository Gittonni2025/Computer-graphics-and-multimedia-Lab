// ============================================
// Aqua World — Fish Game Edition
// Controls: W A S D / Arrow keys = move, R = restart, ESC = quit
// ============================================
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#include "glad.h"
#include "glfw3.h"

#include <iostream>
#include <cmath>
#include <cstdlib>
#include <cstdio>
#include <string>
#include <vector>

const unsigned int SCR_WIDTH = 1024;
const unsigned int SCR_HEIGHT = 680;

// ---------------- Shader ----------------
const char *vertexShaderSource = "#version 330 core\n"
    "layout (location = 0) in vec3 aPos;\n"
    "layout (location = 1) in vec2 aTexCoord;\n"
    "uniform vec2 uOffset;\n"
    "uniform float uScale;\n"
    "uniform float uRotate;\n"
    "out vec2 TexCoord;\n"
    "void main()\n"
    "{\n"
    "   vec3 p = aPos;\n"
    "   float c = cos(uRotate);\n"
    "   float s = sin(uRotate);\n"
    "   vec2 rotated = vec2(p.x * c - p.y * s, p.x * s + p.y * c);\n"
    "   vec2 final = rotated * uScale + uOffset;\n"
    "   gl_Position = vec4(final, 0.0, 1.0);\n"
    "   TexCoord = aTexCoord;\n"
    "}\0";

const char *fragmentShaderSource = "#version 330 core\n"
    "out vec4 FragColor;\n"
    "in vec2 TexCoord;\n"
    "uniform sampler2D ourTexture;\n"
    "uniform vec4 uColor;\n"
    "uniform int useTexture;\n"
    "void main()\n"
    "{\n"
    "   if (useTexture == 1) {\n"
    "       vec4 tex = texture(ourTexture, TexCoord);\n"
    "       FragColor = tex * uColor;\n"
    "   } else {\n"
    "       FragColor = uColor;\n"
    "   }\n"
    "}\n\0";

unsigned int shaderProgram;
unsigned int VAO, VBO;
float animTime = 0.0f;

unsigned int sandTexture;
unsigned int rockTexture;

// alpha for overlays (1.0 = solid). drawTriangles uses this.
float gAlpha = 1.0f;

GLFWwindow* gWindow = NULL;

// ============================================
//               GAME STATE
// ============================================
const int WIN_SCORE = 15;      // এতগুলো খাবার খেলে জিত
const int START_LIVES = 3;

float px = 0.0f, py = 0.30f;   // player fish position
int   pdir = 1;                // 1 = ডানে মুখ, -1 = বামে মুখ
int   score = 0;
int   lives = START_LIVES;
float invuln = 0.0f;           // ধাক্কা খাওয়ার পর কিছুক্ষণ সুরক্ষা
bool  gameOver = false;
bool  won = false;

struct Food {
    float x, y;
    float speed;
    float phase;
};
std::vector<Food> foods;
int foodTimer = 0;

float sharkX = 0.8f, sharkY = -0.10f;
int   sharkDir = -1;

float jx = 0.0f, jy = 0.5f;    // jellyfish position (hazard)

// ---------------- Texture Loader ----------------
unsigned int loadTexture(const char* path) {
    unsigned int textureID;
    glGenTextures(1, &textureID);

    int width, height, nrChannels;
    stbi_set_flip_vertically_on_load(true);
    unsigned char *data = stbi_load(path, &width, &height, &nrChannels, 0);

    if (data) {
        GLenum format = GL_RGB;
        if (nrChannels == 4) format = GL_RGBA;

        glBindTexture(GL_TEXTURE_2D, textureID);
        glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, data);
        glGenerateMipmap(GL_TEXTURE_2D);

        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

        std::cout << "Texture loaded: " << path << " (" << width << "x" << height << ")" << std::endl;
    } else {
        std::cout << "FAILED to load texture: " << path << std::endl;
    }
    stbi_image_free(data);
    return textureID;
}

// ---------------- Shader Helpers ----------------
unsigned int compileShader(unsigned int type, const char* src) {
    unsigned int s = glCreateShader(type);
    glShaderSource(s, 1, &src, NULL);
    glCompileShader(s);
    int ok;
    char log[512];
    glGetShaderiv(s, GL_COMPILE_STATUS, &ok);
    if (!ok) {
        glGetShaderInfoLog(s, 512, NULL, log);
        std::cout << "Shader error: " << log << std::endl;
    }
    return s;
}

void setColor(float r, float g, float b, float a = 1.0f) {
    glUniform4f(glGetUniformLocation(shaderProgram, "uColor"), r, g, b, a);
}

void setTransform(float ox, float oy, float scale, float rotate) {
    glUniform2f(glGetUniformLocation(shaderProgram, "uOffset"), ox, oy);
    glUniform1f(glGetUniformLocation(shaderProgram, "uScale"), scale);
    glUniform1f(glGetUniformLocation(shaderProgram, "uRotate"), rotate);
}

void setUseTexture(int use) {
    glUniform1i(glGetUniformLocation(shaderProgram, "useTexture"), use);
}

// ---------------- Core Draw ----------------
void drawTriangles(const std::vector<float>& v, float ox, float oy, float scale, float rot, float r, float g, float b) {
    glBindVertexArray(VAO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, v.size() * sizeof(float), v.data(), GL_DYNAMIC_DRAW);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);

    setColor(r, g, b, gAlpha);
    setTransform(ox, oy, scale, rot);
    setUseTexture(0);
    glDrawArrays(GL_TRIANGLES, 0, (int)v.size() / 5);
}

void drawTrianglesTextured(const std::vector<float>& v, float ox, float oy, float scale, float rot,
                            float r, float g, float b, unsigned int textureID) {
    glBindVertexArray(VAO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, v.size() * sizeof(float), v.data(), GL_DYNAMIC_DRAW);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);

    setColor(r, g, b);
    setTransform(ox, oy, scale, rot);
    setUseTexture(1);

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, textureID);
    glUniform1i(glGetUniformLocation(shaderProgram, "ourTexture"), 0);

    glDrawArrays(GL_TRIANGLES, 0, (int)v.size() / 5);
}

// ---------------- Primitives ----------------
void drawRect(float cx, float cy, float w, float h, float r, float g, float b) {
    float hw = w / 2.0f, hh = h / 2.0f;
    std::vector<float> v = {
        -hw, -hh, 0,  0.0f, 0.0f,
         hw, -hh, 0,  1.0f, 0.0f,
        -hw,  hh, 0,  0.0f, 1.0f,
        -hw,  hh, 0,  0.0f, 1.0f,
         hw, -hh, 0,  1.0f, 0.0f,
         hw,  hh, 0,  1.0f, 1.0f,
    };
    drawTriangles(v, cx, cy, 1.0f, 0.0f, r, g, b);
}

void drawColoredTriangle(float x1, float y1, float x2, float y2, float x3, float y3,
                         float r, float g, float b) {
    std::vector<float> v = {
        x1, y1, 0,  0.0f, 0.0f,
        x2, y2, 0,  0.0f, 0.0f,
        x3, y3, 0,  0.0f, 0.0f,
    };
    drawTriangles(v, 0, 0, 1.0f, 0.0f, r, g, b);
}

void drawCircle(float cx, float cy, float rad, float r, float g, float b, int seg = 24) {
    std::vector<float> v;
    for (int i = 0; i < seg; i++) {
        float a1 = 2.0f * 3.1415926f * i / seg;
        float a2 = 2.0f * 3.1415926f * (i + 1) / seg;
        v.push_back(0); v.push_back(0); v.push_back(0);
        v.push_back(0.5f); v.push_back(0.5f);
        v.push_back(cosf(a1) * rad); v.push_back(sinf(a1) * rad); v.push_back(0);
        v.push_back(0.5f + cosf(a1) * 0.5f); v.push_back(0.5f + sinf(a1) * 0.5f);
        v.push_back(cosf(a2) * rad); v.push_back(sinf(a2) * rad); v.push_back(0);
        v.push_back(0.5f + cosf(a2) * 0.5f); v.push_back(0.5f + sinf(a2) * 0.5f);
    }
    drawTriangles(v, cx, cy, 1.0f, 0.0f, r, g, b);
}

// মোটা লাইন (দুইটা triangle দিয়ে) — X চিহ্নের জন্য
void drawLine(float x1, float y1, float x2, float y2, float thick, float r, float g, float b) {
    float dx = x2 - x1, dy = y2 - y1;
    float len = sqrtf(dx * dx + dy * dy);
    if (len < 0.0001f) return;
    float nx = -dy / len * thick * 0.5f;
    float ny =  dx / len * thick * 0.5f;
    drawColoredTriangle(x1 + nx, y1 + ny, x1 - nx, y1 - ny, x2 + nx, y2 + ny, r, g, b);
    drawColoredTriangle(x1 - nx, y1 - ny, x2 - nx, y2 - ny, x2 + nx, y2 + ny, r, g, b);
}

// ---------------- Background ----------------
void drawBackground() {
    int layers = 60;
    float yTop = 1.0f;
    float yBot = -0.35f;
    float layerH = (yTop - yBot) / layers;

    float rTop = 0.62f, gTop = 0.90f, bTop = 0.97f;
    float rBot = 0.05f, gBot = 0.28f, bBot = 0.62f;

    for (int i = 0; i < layers; i++) {
        float t = (float)i / (layers - 1);
        float r = rTop * (1.0f - t) + rBot * t;
        float g = gTop * (1.0f - t) + gBot * t;
        float b = bTop * (1.0f - t) + bBot * t;

        float y = yTop - layerH * (i + 0.5f);
        drawRect(0.0f, y, 2.0f, layerH + 0.005f, r, g, b);
    }
}

// ---------------- Mountain ----------------
void drawMountain() {
    std::vector<float> t1 = {
        -0.75f, -0.32f, 0,  0.0f,  0.20f,
         0.00f,  0.75f, 0,  0.5f,  0.90f,
         0.00f, -0.32f, 0,  0.5f,  0.20f,
    };
    drawTrianglesTextured(t1, 0, 0, 1.0f, 0.0f, 0.65f, 0.95f, 0.55f, rockTexture);

    std::vector<float> t2 = {
         0.00f,  0.75f, 0,  0.5f,  0.90f,
         0.75f, -0.32f, 0,  1.0f,  0.20f,
         0.00f, -0.32f, 0,  0.5f,  0.20f,
    };
    drawTrianglesTextured(t2, 0, 0, 1.0f, 0.0f, 0.60f, 0.90f, 0.50f, rockTexture);
}

// ---------------- Sand ----------------
void drawSand() {
    std::vector<float> sand = {
        -1.0f, -0.35f, 0,  0.0f, 0.0f,
         1.0f, -0.35f, 0,  4.0f, 0.0f,
         1.0f, -1.00f, 0,  4.0f, 4.0f,
        -1.0f, -0.35f, 0,  0.0f, 0.0f,
         1.0f, -1.00f, 0,  4.0f, 4.0f,
        -1.0f, -1.00f, 0,  0.0f, 4.0f,
    };
    drawTrianglesTextured(sand, 0, 0, 1.0f, 0.0f, 1.30f, 1.20f, 0.95f, sandTexture);
}

// ---------------- Coral ----------------
void drawCoral(float x, float y, float scale, float r, float g, float b) {
    drawColoredTriangle(x - 0.015f, y, x + 0.015f, y, x, y + 0.20f * scale, r * 0.7f, g * 0.7f, b * 0.7f);
    drawColoredTriangle(x, y + 0.12f * scale, x + 0.06f * scale, y + 0.06f * scale, x + 0.02f * scale, y + 0.25f * scale, r, g, b);
    drawColoredTriangle(x, y + 0.12f * scale, x - 0.06f * scale, y + 0.06f * scale, x - 0.02f * scale, y + 0.25f * scale, r, g, b);
    drawColoredTriangle(x, y + 0.16f * scale, x + 0.01f * scale, y + 0.20f * scale, x + 0.00f * scale, y + 0.30f * scale, r * 1.1f, g * 1.1f, b * 1.1f);
    drawColoredTriangle(x + 0.02f * scale, y + 0.22f * scale, x + 0.10f * scale, y + 0.20f * scale, x + 0.06f * scale, y + 0.30f * scale, r * 0.9f, g * 0.9f, b * 0.9f);
    drawColoredTriangle(x - 0.02f * scale, y + 0.22f * scale, x - 0.10f * scale, y + 0.20f * scale, x - 0.06f * scale, y + 0.30f * scale, r * 0.9f, g * 0.9f, b * 0.9f);
}

void drawAllCorals() {
    drawCoral(-0.70f, -0.45f, 1.0f, 0.75f, 0.35f, 0.70f);
    drawCoral(-0.62f, -0.42f, 0.8f, 0.80f, 0.40f, 0.75f);
    drawCoral( 0.55f, -0.42f, 1.0f, 0.95f, 0.45f, 0.70f);
    drawCoral( 0.63f, -0.45f, 0.85f, 0.90f, 0.40f, 0.65f);

    drawCoral(-0.55f, -0.60f, 0.55f, 0.95f, 0.55f, 0.30f);
    drawCoral(-0.10f, -0.65f, 0.45f, 0.90f, 0.30f, 0.45f);
    drawCoral( 0.10f, -0.63f, 0.50f, 0.95f, 0.45f, 0.70f);
    drawCoral( 0.60f, -0.68f, 0.55f, 0.80f, 0.40f, 0.75f);
    drawCoral(-0.80f, -0.70f, 0.45f, 0.95f, 0.55f, 0.30f);
}

// ---------------- Starfish ----------------
void drawStarfish(float x, float y, float size, float r, float g, float b) {
    float outer = size * 1.0f;
    float inner = size * 0.40f;

    float outerAngles[5];
    float innerAngles[5];

    for (int i = 0; i < 5; i++) {
        outerAngles[i] = (90.0f + i * 72.0f) * 3.1415926f / 180.0f;
        innerAngles[i] = (90.0f + 36.0f + i * 72.0f) * 3.1415926f / 180.0f;
    }

    float points[10][2];
    for (int i = 0; i < 5; i++) {
        points[i * 2][0]     = x + outer * cosf(outerAngles[i]);
        points[i * 2][1]     = y + outer * sinf(outerAngles[i]);
        points[i * 2 + 1][0] = x + inner * cosf(innerAngles[i]);
        points[i * 2 + 1][1] = y + inner * sinf(innerAngles[i]);
    }

    for (int i = 0; i < 10; i++) {
        int next = (i + 1) % 10;
        drawColoredTriangle(
            x, y,
            points[i][0], points[i][1],
            points[next][0], points[next][1],
            r, g, b
        );
    }
}

void drawAllStarfish() {
    drawStarfish(-0.45f, -0.55f, 0.10f, 0.90f, 0.20f, 0.15f);
    drawStarfish( 0.45f, -0.60f, 0.09f, 0.15f, 0.30f, 0.75f);
    drawStarfish(-0.05f, -0.75f, 0.075f, 1.0f, 0.55f, 0.20f);
}

// ---------------- Jellyfish (এখন নড়ে, আর বিপদজনক!) ----------------
void drawJellyfish(float x, float y, float size, float r, float g, float b) {
    drawCircle(x, y, size * 0.16f, r, g, b, 32);
    drawCircle(x, y + size * 0.04f, size * 0.155f, r, g, b, 32);
    drawCircle(x, y + size * 0.08f, size * 0.14f, r, g, b, 30);
    drawCircle(x, y + size * 0.12f, size * 0.12f, r, g, b, 28);
    drawCircle(x, y + size * 0.16f, size * 0.09f, r, g, b, 26);

    int n = 7;
    for (int i = 0; i < n; i++) {
        float tx = x - size * 0.12f + i * (size * 0.24f / (n - 1));
        float wave = 0.012f * sinf(animTime * 2.0f + i * 0.8f);

        drawColoredTriangle(
            tx - 0.004f, y - size * 0.02f,
            tx + 0.004f, y - size * 0.02f,
            tx + wave,   y - size * 0.15f,
            r * 0.9f, g * 0.9f, b * 0.9f
        );
        drawColoredTriangle(
            tx - 0.004f, y - size * 0.02f,
            tx + wave,   y - size * 0.15f,
            tx - 0.004f + wave, y - size * 0.15f,
            r * 0.9f, g * 0.9f, b * 0.9f
        );

        float wave2 = 0.018f * sinf(animTime * 2.0f + i * 0.8f + 1.0f);
        drawColoredTriangle(
            tx - 0.003f + wave, y - size * 0.15f,
            tx + 0.003f + wave, y - size * 0.15f,
            tx + wave2,         y - size * 0.28f,
            r * 0.85f, g * 0.85f, b * 0.85f
        );
        drawColoredTriangle(
            tx - 0.003f + wave, y - size * 0.15f,
            tx + wave2,         y - size * 0.28f,
            tx - 0.003f + wave2, y - size * 0.28f,
            r * 0.85f, g * 0.85f, b * 0.85f
        );
    }
}

void drawAllJellyfish() {
    drawJellyfish(jx, jy, 1.0f, 0.95f, 0.45f, 0.55f);
}

// ---------------- Plants ----------------
void drawPlant(float x, float baseY, float height, float r, float g, float b) {
    drawColoredTriangle(x - 0.018f, baseY, x + 0.018f, baseY, x, baseY + height, r * 0.7f, g * 0.7f, b * 0.7f);

    int n = 14;
    for (int i = 1; i <= n; i++) {
        float t = (float)i / (n + 1);
        float lx = x;
        float ly = baseY + height * t;
        float side = (i % 2 == 0) ? 1.0f : -1.0f;
        float curve = 0.03f * sinf(t * 6.28f);

        drawColoredTriangle(
            lx + curve, ly,
            lx + side * 0.10f + curve, ly + 0.03f,
            lx + side * 0.05f + curve, ly + 0.09f,
            r, g, b
        );
    }
}

void drawAllPlants() {
    drawPlant(-0.88f, -0.30f, 0.85f, 0.12f, 0.55f, 0.20f);
    drawPlant(-0.80f, -0.30f, 0.75f, 0.15f, 0.60f, 0.25f);
    drawPlant(-0.72f, -0.30f, 0.65f, 0.12f, 0.55f, 0.20f);

    drawPlant( 0.88f, -0.30f, 0.85f, 0.12f, 0.55f, 0.20f);
    drawPlant( 0.80f, -0.30f, 0.75f, 0.15f, 0.60f, 0.25f);
    drawPlant( 0.72f, -0.30f, 0.65f, 0.12f, 0.55f, 0.20f);
}

// ---------------- Rocks ----------------
void drawRocks() {
    drawCircle(-0.78f, -0.40f, 0.10f, 0.35f, 0.38f, 0.42f, 14);
    drawCircle(-0.70f, -0.45f, 0.07f, 0.30f, 0.33f, 0.38f, 12);
    drawCircle( 0.78f, -0.40f, 0.10f, 0.35f, 0.38f, 0.42f, 14);
    drawCircle( 0.70f, -0.45f, 0.07f, 0.30f, 0.33f, 0.38f, 12);
}

// ---------------- 3D Bowl ----------------
void drawBowl(float x, float y, float size, float r, float g, float b) {
    drawCircle(x, y - 0.02f * size, 0.14f * size, r * 0.55f, g * 0.55f, b * 0.55f, 30);
    drawCircle(x, y + 0.01f * size, 0.14f * size, r, g, b, 32);
    drawCircle(x, y + 0.02f * size, 0.10f * size, r * 0.30f, g * 0.30f, b * 0.30f, 30);
    drawCircle(x, y + 0.015f * size, 0.065f * size, r * 0.15f, g * 0.15f, b * 0.15f, 26);
    drawCircle(x, y + 0.03f * size, 0.12f * size, r * 1.2f, g * 1.2f, b * 1.2f, 32);
    drawCircle(x, y + 0.03f * size, 0.09f * size, r * 0.35f, g * 0.35f, b * 0.35f, 30);
}

// ---------------- Shark ----------------
void drawShark(float x, float y, float dir, float scale) {
    drawColoredTriangle(x - 0.15f * dir, y, x + 0.00f * dir, y + 0.05f, x + 0.15f * dir, y + 0.02f, 0.35f, 0.45f, 0.60f);
    drawColoredTriangle(x - 0.15f * dir, y, x + 0.15f * dir, y + 0.02f, x + 0.13f * dir, y - 0.04f, 0.35f, 0.45f, 0.60f);
    drawColoredTriangle(x - 0.15f * dir, y, x + 0.13f * dir, y - 0.04f, x + 0.00f * dir, y - 0.05f, 0.35f, 0.45f, 0.60f);
    drawColoredTriangle(x - 0.15f * dir, y, x + 0.00f * dir, y - 0.05f, x - 0.12f * dir, y - 0.02f, 0.35f, 0.45f, 0.60f);

    drawColoredTriangle(x - 0.15f * dir, y, x - 0.25f * dir, y + 0.07f, x - 0.24f * dir, y - 0.07f, 0.30f, 0.40f, 0.55f);
    drawColoredTriangle(x - 0.04f * dir, y + 0.04f, x + 0.01f * dir, y + 0.14f, x + 0.05f * dir, y + 0.04f, 0.30f, 0.40f, 0.55f);

    drawCircle(x + 0.10f * dir, y + 0.01f, 0.013f, 1.0f, 1.0f, 1.0f, 10);
    drawCircle(x + 0.105f * dir, y + 0.01f, 0.007f, 0.0f, 0.0f, 0.0f, 10);
}

// ---------------- Fish ----------------
void drawCartoonFish(float x, float y, float scale, float dir, float r, float g, float b) {
    drawColoredTriangle(x - 0.10f * dir * scale, y, x + 0.02f * dir * scale, y + 0.10f * scale, x + 0.12f * dir * scale, y, r, g, b);
    drawColoredTriangle(x - 0.10f * dir * scale, y, x + 0.12f * dir * scale, y, x + 0.02f * dir * scale, y - 0.10f * scale, r, g, b);

    drawColoredTriangle(x - 0.10f * dir * scale, y, x - 0.20f * dir * scale, y + 0.10f * scale, x - 0.20f * dir * scale, y - 0.10f * scale, 1.0f, 0.85f, 0.15f);
    drawColoredTriangle(x - 0.02f * dir * scale, y + 0.08f * scale, x + 0.04f * dir * scale, y + 0.16f * scale, x + 0.08f * dir * scale, y + 0.07f * scale, 1.0f, 0.85f, 0.15f);
    drawColoredTriangle(x + 0.00f * dir * scale, y - 0.08f * scale, x - 0.02f * dir * scale, y - 0.15f * scale, x + 0.06f * dir * scale, y - 0.08f * scale, 1.0f, 0.85f, 0.15f);

    drawCircle(x + 0.05f * dir * scale, y + 0.02f * scale, 0.035f * scale, 1.0f, 1.0f, 1.0f, 16);
    drawCircle(x + 0.06f * dir * scale, y + 0.02f * scale, 0.02f * scale, 0.1f, 0.05f, 0.05f, 16);
    drawCircle(x + 0.07f * dir * scale, y + 0.03f * scale, 0.008f * scale, 1.0f, 1.0f, 1.0f, 8);

    drawRect(x + 0.10f * dir * scale, y - 0.03f * scale, 0.02f * scale, 0.005f * scale, 0.3f, 0.05f, 0.05f);
}

// ---------------- Background Fish (শুধু সাজানোর জন্য, ক্ষতি করে না) ----------------
float f2X =  0.6f,  f2Y =  0.15f, f2Speed = -0.005f;
float f3X = -0.7f,  f3Y = -0.05f, f3Speed = 0.004f;
float f4X =  0.2f,  f4Y = -0.20f, f4Speed = -0.0045f;

void updateFishes() {
    f2X += f2Speed;
    if (f2X >  0.90f) f2Speed = -f2Speed;
    if (f2X < -0.90f) f2Speed = -f2Speed;

    f3X += f3Speed;
    if (f3X >  0.90f) f3Speed = -f3Speed;
    if (f3X < -0.90f) f3Speed = -f3Speed;

    f4X += f4Speed;
    if (f4X >  0.90f) f4Speed = -f4Speed;
    if (f4X < -0.90f) f4Speed = -f4Speed;
}

// ---------------- Bubbles ----------------
struct Bubble {
    float x, y;
    float size;
    float speed;
};

std::vector<Bubble> bubbles;

void spawnBubble() {
    Bubble b;
    b.x = -0.95f + (rand() % 190) / 100.0f;
    b.y = -0.15f;
    b.size = 0.004f + (rand() % 10) / 1000.0f;
    b.speed = 0.003f + (rand() % 10) / 4000.0f;
    bubbles.push_back(b);
}

void updateAndDrawBubbles() {
    for (auto& b : bubbles) b.y += b.speed;
    for (int i = (int)bubbles.size() - 1; i >= 0; i--) {
        if (bubbles[i].y > 0.85f) bubbles.erase(bubbles.begin() + i);
    }
    for (const auto& b : bubbles) {
        drawCircle(b.x, b.y, b.size, 0.95f, 0.98f, 1.0f, 10);
        drawCircle(b.x, b.y, b.size * 0.6f, 1.0f, 1.0f, 1.0f, 10);
    }
}

// ============================================
//          HUD: seven-segment number
// ============================================
// প্রতিটা digit এর জন্য a b c d e f g segment অন (1) / অফ (0)
const int SEG[10][7] = {
    {1,1,1,1,1,1,0}, // 0
    {0,1,1,0,0,0,0}, // 1
    {1,1,0,1,1,0,1}, // 2
    {1,1,1,1,0,0,1}, // 3
    {0,1,1,0,0,1,1}, // 4
    {1,0,1,1,0,1,1}, // 5
    {1,0,1,1,1,1,1}, // 6
    {1,1,1,0,0,0,0}, // 7
    {1,1,1,1,1,1,1}, // 8
    {1,1,1,1,0,1,1}  // 9
};

void drawDigit(int d, float x, float y, float w, float h, float r, float g, float b) {
    if (d < 0 || d > 9) return;
    float t = w * 0.20f;
    const int* s = SEG[d];
    if (s[0]) drawRect(x, y + h / 2 - t / 2, w, t, r, g, b);                    // a (top)
    if (s[1]) drawRect(x + w / 2 - t / 2, y + h / 4, t, h / 2, r, g, b);        // b
    if (s[2]) drawRect(x + w / 2 - t / 2, y - h / 4, t, h / 2, r, g, b);        // c
    if (s[3]) drawRect(x, y - h / 2 + t / 2, w, t, r, g, b);                    // d (bottom)
    if (s[4]) drawRect(x - w / 2 + t / 2, y - h / 4, t, h / 2, r, g, b);        // e
    if (s[5]) drawRect(x - w / 2 + t / 2, y + h / 4, t, h / 2, r, g, b);        // f
    if (s[6]) drawRect(x, y, w, t, r, g, b);                                    // g (middle)
}

// number কে cx কে কেন্দ্র ধরে মাঝখানে বসায়
void drawNumber(int n, float cx, float cy, float w, float h, float r, float g, float b) {
    std::string s = std::to_string(n);
    float spacing = w * 1.5f;
    float total = (s.size() - 1) * spacing;
    float startX = cx - total / 2.0f;
    for (size_t i = 0; i < s.size(); i++) {
        drawDigit(s[i] - '0', startX + i * spacing, cy, w, h, r, g, b);
    }
}

void drawHUD() {
    // ---- Score (বাম উপরে) ----
    gAlpha = 0.35f;
    drawRect(-0.795f, 0.90f, 0.35f, 0.13f, 0.0f, 0.1f, 0.25f);
    gAlpha = 1.0f;
    drawStarfish(-0.93f, 0.90f, 0.04f, 1.0f, 0.85f, 0.20f);
    drawNumber(score, -0.77f, 0.90f, 0.035f, 0.09f, 1.0f, 1.0f, 1.0f);

    // ---- Lives (ডান উপরে) ----
    gAlpha = 0.35f;
    drawRect(0.745f, 0.90f, 0.45f, 0.13f, 0.0f, 0.1f, 0.25f);
    gAlpha = 1.0f;
    for (int i = 0; i < START_LIVES; i++) {
        float lx = 0.62f + i * 0.13f;
        if (i < lives) drawCartoonFish(lx, 0.90f, 0.35f, 1.0f, 0.92f, 0.20f, 0.12f);
        else           drawCartoonFish(lx, 0.90f, 0.35f, 1.0f, 0.35f, 0.35f, 0.40f);
    }
}

void drawEndScreen() {
    if (gameOver) {
        gAlpha = 0.55f;
        drawRect(0.0f, 0.0f, 2.0f, 2.0f, 0.45f, 0.0f, 0.0f);
        gAlpha = 1.0f;
        // বড় X চিহ্ন
        drawLine(-0.18f, 0.55f,  0.18f, 0.15f, 0.07f, 1.0f, 1.0f, 1.0f);
        drawLine(-0.18f, 0.15f,  0.18f, 0.55f, 0.07f, 1.0f, 1.0f, 1.0f);
        drawNumber(score, 0.0f, -0.15f, 0.09f, 0.22f, 1.0f, 1.0f, 1.0f);
    } else if (won) {
        gAlpha = 0.45f;
        drawRect(0.0f, 0.0f, 2.0f, 2.0f, 0.0f, 0.35f, 0.10f);
        gAlpha = 1.0f;
        drawStarfish(0.0f, 0.35f, 0.25f, 1.0f, 0.85f, 0.15f);
        drawNumber(score, 0.0f, -0.15f, 0.09f, 0.22f, 1.0f, 1.0f, 1.0f);
    }
}

// ============================================
//               GAME LOGIC
// ============================================
void resetGame() {
    px = 0.0f; py = 0.30f; pdir = 1;
    score = 0;
    lives = START_LIVES;
    invuln = 0.0f;
    gameOver = false;
    won = false;
    foods.clear();
    foodTimer = 0;
    sharkX = 0.8f; sharkY = -0.10f; sharkDir = -1;
}

void spawnFood() {
    Food f;
    f.x = -0.85f + (rand() % 170) / 100.0f;
    f.y = 0.90f;
    f.speed = 0.0025f + (rand() % 25) / 10000.0f;
    f.phase = (rand() % 628) / 100.0f;
    foods.push_back(f);
}

void hitPlayer() {
    if (invuln > 0.0f) return;
    lives--;
    invuln = 100.0f;                       // ~১.৬ সেকেন্ড সুরক্ষা
    px = 0.0f; py = 0.45f;                 // player কে মাঝখানে ফিরিয়ে আনো
    sharkX = (px > 0.0f) ? -0.8f : 0.8f;   // shark কে দূরে পাঠাও
    sharkY = -0.10f;
    if (lives <= 0) gameOver = true;
}

void updateGame() {
    if (gameOver || won) return;

    if (invuln > 0.0f) invuln -= 1.0f;

    // ---- খাবার তৈরি ----
    foodTimer--;
    if (foodTimer <= 0 && foods.size() < 7) {
        spawnFood();
        foodTimer = 35 + rand() % 35;
    }

    // ---- খাবার পড়া ----
    for (auto& f : foods) {
        f.y -= f.speed;
        f.x += 0.0008f * sinf(animTime * 2.0f + f.phase);
    }
    for (int i = (int)foods.size() - 1; i >= 0; i--) {
        if (foods[i].y < -0.30f) foods.erase(foods.begin() + i);
    }

    // ---- খাবার খাওয়া ----
    for (int i = (int)foods.size() - 1; i >= 0; i--) {
        float dx = foods[i].x - (px + 0.06f * pdir);
        float dy = foods[i].y - py;
        if (fabsf(dx) < 0.13f && fabsf(dy) < 0.10f) {
            foods.erase(foods.begin() + i);
            score++;
            if (score >= WIN_SCORE) { won = true; return; }
        }
    }

    // ---- Shark player কে তাড়া করে (স্কোর বাড়লে দ্রুত হয়) ----
    float sSpeed = 0.0032f + score * 0.00025f;
    if (sSpeed > 0.0075f) sSpeed = 0.0075f;
    float sdx = px - sharkX;
    float sdy = py - sharkY;
    if (fabsf(sdx) > 0.02f) {
        sharkDir = (sdx > 0) ? 1 : -1;
        sharkX += sharkDir * sSpeed;
    }
    if (fabsf(sdy) > 0.02f) {
        sharkY += ((sdy > 0) ? 1.0f : -1.0f) * sSpeed * 0.6f;
    }
    if (sharkY < -0.28f) sharkY = -0.28f;
    if (sharkY >  0.80f) sharkY =  0.80f;

    // ---- Shark এর সাথে ধাক্কা ----
    float shx = px - (sharkX - 0.05f * sharkDir);
    float shy = py - sharkY;
    if (fabsf(shx) < 0.20f && fabsf(shy) < 0.09f) hitPlayer();

    // ---- Jellyfish এর সাথে ধাক্কা ----
    float jdx = px - jx;
    float jdy = py - jy;
    if (fabsf(jdx) < 0.16f && jdy > -0.24f && jdy < 0.22f) hitPlayer();
}

void updateJelly() {
    jx = 0.60f * sinf(animTime * 0.10f);
    jy = 0.50f + 0.22f * sinf(animTime * 0.16f + 1.0f);
}

// ---------------- Window title (score/lives দেখানোর জন্য) ----------------
void refreshTitle() {
    static int lastScore = -1, lastLives = -1, lastState = -1;
    int state = gameOver ? 1 : (won ? 2 : 0);
    if (score == lastScore && lives == lastLives && state == lastState) return;
    lastScore = score; lastLives = lives; lastState = state;

    char buf[200];
    if (state == 1)
        snprintf(buf, sizeof(buf), "Aqua World | GAME OVER | Score: %d | Press R to restart", score);
    else if (state == 2)
        snprintf(buf, sizeof(buf), "Aqua World | YOU WIN! | Score: %d | Press R to play again", score);
    else
        snprintf(buf, sizeof(buf), "Aqua World | Score: %d / %d | Lives: %d | WASD/Arrows = move, R = restart", score, WIN_SCORE, lives);
    glfwSetWindowTitle(gWindow, buf);
}

// ---------------- Display ----------------
void display() {
    glClearColor(0.42f, 0.78f, 0.92f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);
    glUseProgram(shaderProgram);

    gAlpha = 1.0f;

    drawBackground();
    drawMountain();
    drawSand();
    drawAllCorals();
    drawAllJellyfish();
    drawAllPlants();
    drawRocks();
    drawAllStarfish();

    drawBowl(-0.30f, -0.72f, 1.0f, 0.55f, 0.40f, 0.22f);
    drawBowl( 0.40f, -0.77f, 0.85f, 0.75f, 0.70f, 0.60f);

    // ---- খাবার (সোনালি গোল দানা) ----
    for (const auto& f : foods) {
        float pulse = 1.0f + 0.15f * sinf(animTime * 4.0f + f.phase);
        drawCircle(f.x, f.y, 0.028f * pulse, 1.0f, 0.75f, 0.10f, 14);
        drawCircle(f.x, f.y, 0.014f * pulse, 1.0f, 1.0f, 0.70f, 10);
    }

    // ---- Shark ----
    drawShark(sharkX, sharkY, (float)sharkDir, 1.0f);

    // ---- Background fish ----
    drawCartoonFish(f2X, f2Y, 0.9f, f2Speed > 0 ? 1.0f : -1.0f, 0.98f, 0.55f, 0.15f);
    drawCartoonFish(f3X, f3Y, 1.0f, f3Speed > 0 ? 1.0f : -1.0f, 0.20f, 0.75f, 0.92f);
    drawCartoonFish(f4X, f4Y, 0.9f, f4Speed > 0 ? 1.0f : -1.0f, 0.70f, 0.30f, 0.85f);

    // ---- Player fish (ধাক্কা খেলে ঝিকমিক করে) ----
    bool visible = (invuln <= 0.0f) || (((int)(invuln / 6.0f)) % 2 == 0);
    if (visible) drawCartoonFish(px, py, 1.2f, (float)pdir, 0.92f, 0.20f, 0.12f);

    updateAndDrawBubbles();

    drawHUD();
    drawEndScreen();
}

// ---------------- Update ----------------
void update() {
    animTime += 0.03f;
    updateFishes();
    updateJelly();
    if (rand() % 10 == 0) spawnBubble();
    if (rand() % 15 == 0) spawnBubble();
    if (rand() % 20 == 0) spawnBubble();
    updateGame();
    refreshTitle();
}

// ---------------- Callbacks ----------------
void framebuffer_size_callback(GLFWwindow* window, int width, int height) {
    glViewport(0, 0, width, height);
}

void processInput(GLFWwindow *window) {
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);

    if (glfwGetKey(window, GLFW_KEY_R) == GLFW_PRESS)
        resetGame();

    if (gameOver || won) return;

    const float speed = 0.012f;
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS || glfwGetKey(window, GLFW_KEY_LEFT) == GLFW_PRESS) {
        px -= speed; pdir = -1;
    }
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS || glfwGetKey(window, GLFW_KEY_RIGHT) == GLFW_PRESS) {
        px += speed; pdir = 1;
    }
    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS || glfwGetKey(window, GLFW_KEY_UP) == GLFW_PRESS)
        py += speed;
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS || glfwGetKey(window, GLFW_KEY_DOWN) == GLFW_PRESS)
        py -= speed;

    // সীমানা: পানির ভেতরেই থাকবে
    if (px >  0.88f) px =  0.88f;
    if (px < -0.88f) px = -0.88f;
    if (py >  0.78f) py =  0.78f;
    if (py < -0.22f) py = -0.22f;
}

// ---------------- Main ----------------
int main() {
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

#ifdef __APPLE__
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif

    GLFWwindow* window = glfwCreateWindow(SCR_WIDTH, SCR_HEIGHT, "Aqua World", NULL, NULL);
    if (window == NULL) {
        std::cout << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return -1;
    }
    gWindow = window;
    glfwMakeContextCurrent(window);
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);

    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
        std::cout << "Failed to initialize GLAD" << std::endl;
        return -1;
    }

    // alpha blending চালু (HUD panel ও game over overlay এর জন্য)
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    unsigned int vertexShader = compileShader(GL_VERTEX_SHADER, vertexShaderSource);
    unsigned int fragmentShader = compileShader(GL_FRAGMENT_SHADER, fragmentShaderSource);

    shaderProgram = glCreateProgram();
    glAttachShader(shaderProgram, vertexShader);
    glAttachShader(shaderProgram, fragmentShader);
    glLinkProgram(shaderProgram);

    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);

    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);

    sandTexture = loadTexture("sand.jpg");
    rockTexture = loadTexture("rock.jpg");

    resetGame();

    double lastTime = glfwGetTime();
    while (!glfwWindowShouldClose(window)) {
        double currentTime = glfwGetTime();
        if (currentTime - lastTime >= 0.016) {
            processInput(window);
            update();
            display();
            glfwSwapBuffers(window);
            glfwPollEvents();
            lastTime = currentTime;
        }
    }

    glDeleteVertexArrays(1, &VAO);
    glDeleteBuffers(1, &VBO);
    glDeleteProgram(shaderProgram);

    glfwTerminate();
    return 0;
}
