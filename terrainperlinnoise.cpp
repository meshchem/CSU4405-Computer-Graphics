#include <glad/gl.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <iostream>

#include <unordered_set>
#include <vector>
#include <tuple>
#include <cmath>
#include <random>
#include <unordered_map>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

#define STB_IMAGE_IMPLEMENTATION
#include <stb/stb_image.h>

static GLFWwindow* window;
static int windowWidth = 1024;
static int windowHeight = 768;

//setting up user controls
static void key_callback(GLFWwindow *window, int key, int scancode, int action, int mode);
static void mouse_callback(GLFWwindow *window, double xpos, double ypos);
void handleCameraMovement(GLFWwindow* window);

// Camera parameters
static glm::vec3 cameraPos(0.0f, 50.0f, 0.0f);  // High up
static glm::vec3 cameraFront(0.0f, -1.0f, 0.0f); // Look downward at an angle

static glm::vec3 cameraUp(0.0f, 1.0f, 0.0f);
static float cameraSpeed = 0.1f;
static float yaw = -90.0f;
static float pitch = -30.0f;
static float lastX = windowWidth / 2.0f;
static float lastY = windowHeight / 2.0f;
static bool firstMouse = true;

// Infinite terrain parameters
static const float cellSize = 10.0f;
static const int renderDistance = 3; // Distance in grid tiles
static const int gridResolution = 10; // Perlin grid resolution
static const float heightScale = 18.0f; // Scale Perlin noise height

// Custom hash function for std::tuple<int, int>
struct TupleHash {
    template <typename T1, typename T2>
    std::size_t operator()(const std::tuple<T1, T2>& t) const {
        auto h1 = std::hash<T1>{}(std::get<0>(t));
        auto h2 = std::hash<T2>{}(std::get<1>(t));
        return h1 ^ (h2 << 1);
    }
};

std::unordered_set<std::tuple<int, int>, TupleHash> renderedTiles;

// OpenGL shader program
GLuint programID;

// Infinite terrain parameters
static const int tileSize = 16; // Number of grid points per tile
static const float tileWorldSize = (tileSize - 1) * cellSize;

std::unordered_map<std::tuple<int, int>, GLuint, TupleHash> tileVAOs;

// Vertex and Fragment Shader source
const char* vertexShaderSource = R"(
#version 330 core
layout(location = 0) in vec3 aPos;

out vec3 FragPos;

uniform mat4 view;
uniform mat4 projection;
uniform mat4 model;

void main() {
    FragPos = vec3(model * vec4(aPos, 1.0));
    gl_Position = projection * view * model * vec4(aPos, 1.0);
}

)";

const char* fragmentShaderSource = R"(
#version 330 core
out vec4 FragColor;

in vec3 FragPos;

void main() {
    vec3 lowColor = vec3(0.047, 0.274, 0.208); // Dark green
    vec3 highColor = vec3(0.239, 0.647, 0.435); // Light green

    float height = FragPos.y;
    height = clamp(height / 10.0, 0.0, 1.0); // Normalize height between 0 and 1

    vec3 color = mix(lowColor, highColor, height);
    FragColor = vec4(color, 1.0);
}

)";

float lerp(float a, float b, float t) {
    return a + t * (b - a);
}

float fade(float t) {
    return t * t * t * (t * (t * 6 - 15) + 10);
}

// Use a global seed for consistent noise across tiles
static const unsigned int NOISE_SEED = 12345;

glm::vec2 randomGradient(int ix, int iy) {
    unsigned int seed = NOISE_SEED + 3251 * ix + 8741 * iy;
    seed = (seed << 13) ^ seed;
    unsigned int hashed = (seed * (seed * seed * 15731 + 789221) + 1376312589) & 0x7fffffff;

    float angle = hashed % 360 * (M_PI / 180.0f);
    return glm::vec2(cos(angle), sin(angle));
}

float dotGridGradient(int ix, int iy, float x, float y) {
    glm::vec2 gradient = randomGradient(ix, iy);
    glm::vec2 distance = glm::vec2(x - (float)ix, y - (float)iy);
    return glm::dot(gradient, distance);
}

float perlin(float x, float y) {
    int x0 = (int)floor(x);
    int x1 = x0 + 1;
    int y0 = (int)floor(y);
    int y1 = y0 + 1;

    float sx = fade(x - (float)x0);
    float sy = fade(y - (float)y0);

    float n0, n1, ix0, ix1;
    n0 = dotGridGradient(x0, y0, x, y);
    n1 = dotGridGradient(x1, y0, x, y);
    ix0 = lerp(n0, n1, sx);

    n0 = dotGridGradient(x0, y1, x, y);
    n1 = dotGridGradient(x1, y1, x, y);
    ix1 = lerp(n0, n1, sx);

    return lerp(ix0, ix1, sy);
}

std::vector<float> generateHeightMap(int gridResolution, float tileOffsetX, float tileOffsetZ, float cellSize) {
    int totalResolution = gridResolution + 1; // Standard grid size with shared edges
    std::vector<float> heightMap(totalResolution * totalResolution);

    for (int z = 0; z < totalResolution; ++z) {
        for (int x = 0; x < totalResolution; ++x) {
            float fx = tileOffsetX + x * cellSize;
            float fz = tileOffsetZ + z * cellSize;

            float noiseValue = perlin(fx, fz);
            noiseValue = (noiseValue + 1.0f) * 0.5f; // Normalize to [0, 1]

            heightMap[z * totalResolution + x] = noiseValue * heightScale;
        }
    }
    return heightMap;
}

void renderTiles() {
    for (const auto& tile : renderedTiles) {
        GLuint VAO = tileVAOs[tile];

        glm::mat4 model = glm::mat4(1.0f);
        model = glm::translate(model, glm::vec3(std::get<0>(tile) * tileWorldSize, 0.0f, std::get<1>(tile) * tileWorldSize));

        glUniformMatrix4fv(glGetUniformLocation(programID, "model"), 1, GL_FALSE, &model[0][0]);

        glBindVertexArray(VAO);
        glDrawElements(GL_TRIANGLES, (tileSize + 1) * (tileSize + 1) * 6, GL_UNSIGNED_INT, 0);
    }
}

void generateTerrainVAO(GLuint& VAO, GLuint& VBO, GLuint& EBO, const std::vector<float>& heightMap, int gridResolution, float cellSize) {
    std::vector<float> vertices;
    std::vector<unsigned int> indices;

    int totalResolution = gridResolution + 1;

    for (int z = 0; z < totalResolution; ++z) {
        for (int x = 0; x < totalResolution; ++x) {
            float height = heightMap[z * totalResolution + x];
            vertices.push_back(x * cellSize); // X position
            vertices.push_back(height);      // Y position
            vertices.push_back(z * cellSize); // Z position

            vertices.push_back(0.0f);
            vertices.push_back(1.0f);
            vertices.push_back(0.0f);
        }
    }

    for (int z = 0; z < gridResolution; ++z) {
        for (int x = 0; x < gridResolution; ++x) {
            int topLeft = z * totalResolution + x;
            int topRight = topLeft + 1;
            int bottomLeft = (z + 1) * totalResolution + x;
            int bottomRight = bottomLeft + 1;

            indices.push_back(topLeft);
            indices.push_back(bottomLeft);
            indices.push_back(topRight);

            indices.push_back(topRight);
            indices.push_back(bottomLeft);
            indices.push_back(bottomRight);
        }
    }

    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);
    glGenBuffers(1, &EBO);

    glBindVertexArray(VAO);

    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(float), vertices.data(), GL_STATIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(unsigned int), indices.data(), GL_STATIC_DRAW);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);

    glBindVertexArray(0);
}

void generateTile(int tileX, int tileZ) {
    float tileOffsetX = tileX * tileWorldSize;
    float tileOffsetZ = tileZ * tileWorldSize;

    std::vector<float> heightMap = generateHeightMap(tileSize, tileOffsetX, tileOffsetZ, tileWorldSize / tileSize);

    GLuint VAO, VBO, EBO;
    generateTerrainVAO(VAO, VBO, EBO, heightMap, tileSize, tileWorldSize / tileSize);

    tileVAOs[std::make_tuple(tileX, tileZ)] = VAO;
}

void updateVisibleTiles(glm::vec3 position) {
    int currentTileX = (int)floor(position.x / tileWorldSize);
    int currentTileZ = (int)floor(position.z / tileWorldSize);

    std::unordered_set<std::tuple<int, int>, TupleHash> newTiles;

    for (int x = -renderDistance; x <= renderDistance; ++x) {
        for (int z = -renderDistance; z <= renderDistance; ++z) {
            int tileX = currentTileX + x;
            int tileZ = currentTileZ + z;

            auto tile = std::make_tuple(tileX, tileZ);
            newTiles.insert(tile);

            if (renderedTiles.find(tile) == renderedTiles.end()) {
                generateTile(tileX, tileZ);
                renderedTiles.insert(tile);
            }
        }
    }

    for (auto it = renderedTiles.begin(); it != renderedTiles.end();) {
        if (newTiles.find(*it) == newTiles.end()) {
            GLuint VAO = tileVAOs[*it];
            glDeleteVertexArrays(1, &VAO);
            tileVAOs.erase(*it);
            it = renderedTiles.erase(it);
        } else {
            ++it;
        }
    }
}

int main() {
    if (!glfwInit()) {
        std::cerr << "Failed to initialize GLFW" << std::endl;
        return -1;
    }

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    window = glfwCreateWindow(windowWidth, windowHeight, "Infinite Terrain with Perlin Noise", NULL, NULL);
    if (!window) {
        std::cerr << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);
    
    glfwSetCursorPosCallback(window, mouse_callback);
    glfwSetKeyCallback(window, key_callback);
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

    if (!gladLoadGL(glfwGetProcAddress)) {
        std::cerr << "Failed to initialize GLAD" << std::endl;
        return -1;
    }

    glEnable(GL_DEPTH_TEST);

    GLuint vertexShader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertexShader, 1, &vertexShaderSource, NULL);
    glCompileShader(vertexShader);

    GLuint fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragmentShader, 1, &fragmentShaderSource, NULL);
    glCompileShader(fragmentShader);

    programID = glCreateProgram();
    glAttachShader(programID, vertexShader);
    glAttachShader(programID, fragmentShader);
    glLinkProgram(programID);

    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);

    glClearColor(0.5f, 0.7f, 1.0f, 1.0f);

    while (!glfwWindowShouldClose(window)) {
        handleCameraMovement(window);
        updateVisibleTiles(cameraPos);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        glUseProgram(programID);

        glm::mat4 view = glm::lookAt(cameraPos, cameraPos + cameraFront, cameraUp);
        glm::mat4 projection = glm::perspective(glm::radians(45.0f), (float)windowWidth / windowHeight, 0.1f, 1000.0f);

        glUniformMatrix4fv(glGetUniformLocation(programID, "view"), 1, GL_FALSE, &view[0][0]);
        glUniformMatrix4fv(glGetUniformLocation(programID, "projection"), 1, GL_FALSE, &projection[0][0]);

        renderTiles();

        GLenum err;
        while ((err = glGetError()) != GL_NO_ERROR) {
            std::cerr << "OpenGL Error: " << err << std::endl;
        }

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    glfwTerminate();
    return 0;
}

void mouse_callback(GLFWwindow* window, double xpos, double ypos) {
    if (firstMouse) {
        lastX = xpos;
        lastY = ypos;
        firstMouse = false;
    }

    float xoffset = xpos - lastX;
    float yoffset = lastY - ypos; // Reversed since y-coordinates range from bottom to top
    lastX = xpos;
    lastY = ypos;

    float sensitivity = 0.1f;
    xoffset *= sensitivity;
    yoffset *= sensitivity;

    yaw += xoffset;
    pitch += yoffset;

    if (pitch > 89.0f)
        pitch = 89.0f;
    if (pitch < -89.0f)
        pitch = -89.0f;

    glm::vec3 front;
    front.x = cos(glm::radians(yaw)) * cos(glm::radians(pitch));
    front.y = sin(glm::radians(pitch));
    front.z = sin(glm::radians(yaw)) * cos(glm::radians(pitch));
    cameraFront = glm::normalize(front);
}

void handleCameraMovement(GLFWwindow* window) {
    glm::vec3 direction(0.0f);

    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
        direction += cameraFront;
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
        direction -= cameraFront;
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
        direction -= glm::normalize(glm::cross(cameraFront, cameraUp));
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
        direction += glm::normalize(glm::cross(cameraFront, cameraUp));

    if (glm::length(direction) > 0.0f)
        cameraPos += glm::normalize(direction) * cameraSpeed;
}


void key_callback(GLFWwindow *window, int key, int scancode, int action, int mods) {
    if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
		glfwSetWindowShouldClose(window, GL_TRUE);
}
