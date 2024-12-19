#include <glad/gl.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <vector>
#include <iostream>
#include <string>
#include <sstream>
#include <tuple>
#include <unordered_set>

#define STB_PERLIN_IMPLEMENTATION
#include "stb_perlin.h"
#define BUFFER_OFFSET(i) ((char *)NULL + (i))

static GLFWwindow *window;
static int windowWidth = 1024;
static int windowHeight = 768;

//setting up user controls
static void key_callback(GLFWwindow *window, int key, int scancode, int action, int mode);
static void mouse_callback(GLFWwindow *window, double xpos, double ypos);
void handleCameraMovement(GLFWwindow* window);

// Camera variables
static glm::vec3 cameraPos(0.0f, 1.0f, 3.0f);
static glm::vec3 cameraFront(0.0f, 0.0f, -1.0f);
static glm::vec3 cameraUp(0.0f, 1.0f, 0.0f);
static float yaw = -90.0f;
static float pitch = 0.0f;
static float lastX = 1024.0f / 2.0;
static float lastY = 768.0f / 2.0;
static bool firstMouse = true;
static float cameraSpeed = 0.1f; 

// Shader sources
const char* vertexShaderSource = R"(
#version 330 core
layout(location = 0) in vec3 aPos;
layout(location = 1) in vec3 aColor;

uniform mat4 MVP;

out vec3 vertexColor;

void main() {
    gl_Position = MVP * vec4(aPos, 1.0);
    vertexColor = aColor;
}
)";

const char* fragmentShaderSource = R"(
#version 330 core
in vec3 vertexColor;
out vec4 FragColor;

void main() {
    FragColor = vec4(vertexColor, 1.0);
}
)";

static const float cellSize = 10.0f;
static const int renderDistance = 5; // Distance in grid cells

// Custom hash function for std::tuple<int, int>
struct TupleHash {
    template <typename T1, typename T2>
    std::size_t operator()(const std::tuple<T1, T2>& t) const {
        auto h1 = std::hash<T1>{}(std::get<0>(t));
        auto h2 = std::hash<T2>{}(std::get<1>(t));
        return h1 ^ (h2 << 1);
    }
};


// Add these functions outside the Tile struct, as they are global utilities
float lerp(float a, float b, float t) {
    return a + t * (b - a);
}

float fade(float t) {
    return t * t * t * (t * (t * 6 - 15) + 10);
}

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


struct Tile {

    float tileVertices[18] = {
        -0.5f, 0.0f, -0.5f, // Bottom-left
        0.5f, 0.0f, -0.5f,  // Bottom-right
        -0.5f, 0.0f,  0.5f, // Top-left

        0.5f, 0.0f, -0.5f,  // Bottom-right
        0.5f, 0.0f,  0.5f,  // Top-right
        -0.5f, 0.0f,  0.5f  // Top-left
    };

    float tileColors[18] = {
        0.1f, 0.3f, 0.2f, // Bottom-left
        0.1f, 0.3f, 0.2f, // Bottom-right
        0.1f, 0.3f, 0.2f, // Top-left

        0.1f, 0.3f, 0.2f, // Bottom-right
        0.1f, 0.3f, 0.2f, // Top-right
        0.1f, 0.3f, 0.2f  // Top-left
    };

    GLuint vertexArrayID;
    GLuint vertexBufferID;
    GLuint colorBufferID;
    GLuint programID;
    GLuint mvpMatrixID;

    glm::vec3 position; // Tile position
    float scale;        // Tile size scaling factor

    Tile(glm::vec3 pos, float size) : position(pos), scale(size) {}

    

    GLuint compileShader(GLenum type, const char* source) {
        GLuint shader = glCreateShader(type);
        glShaderSource(shader, 1, &source, NULL);
        glCompileShader(shader);

        int success;
        char infoLog[512];
        glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
        if (!success) {
            glGetShaderInfoLog(shader, 512, NULL, infoLog);
            std::cerr << "Shader Compilation Error: " << infoLog << std::endl;
        }

        return shader;
    }

    void initialize() {

        // Apply Perlin noise to modify Y-coordinates of the tile vertices
        /*for (int i = 0; i < 6; ++i) {
            float x = tileVertices[i * 3];     // X-coordinate
            float z = tileVertices[i * 3 + 2]; // Z-coordinate
            tileVertices[i * 3 + 1] = stb_perlin_noise3(x * 5.0f, z * 5.0f, 0.0f, 0, 0, 0) * 0.2f; // Adjust height using Perlin noise
        }*/

        // Calculate the middle point of the tile
        float midX = (tileVertices[0] + tileVertices[3] + tileVertices[6] + tileVertices[9]) / 4.0f;
        float midZ = (tileVertices[2] + tileVertices[5] + tileVertices[8] + tileVertices[11]) / 4.0f;
        float midY = stb_perlin_noise3(midX * 5.0f, midZ * 5.0f, 0.0f, 0, 0, 0) * 0.3f;

        // Adjust the middle Y value by Perlin noise
        for (int i = 0; i < 6; ++i) {
            float x = tileVertices[i * 3];     // X-coordinate
            float z = tileVertices[i * 3 + 2]; // Z-coordinate

            // Blend Perlin noise at the center with vertex-specific noise
            float vertexNoise = stb_perlin_noise3((position.x + x) * 5.0f, (position.z + z) * 5.0f, 0.0f, 0, 0, 0) * 0.2f;
            tileVertices[i * 3 + 1] = midY + vertexNoise * 0.5f; // Mix center noise with vertex noise
        }

        glGenVertexArrays(1, &vertexArrayID);
        glBindVertexArray(vertexArrayID);

        glGenBuffers(1, &vertexBufferID);
        glBindBuffer(GL_ARRAY_BUFFER, vertexBufferID);
        glBufferData(GL_ARRAY_BUFFER, sizeof(tileVertices), tileVertices, GL_STATIC_DRAW);

        glGenBuffers(1, &colorBufferID);
        glBindBuffer(GL_ARRAY_BUFFER, colorBufferID);
        glBufferData(GL_ARRAY_BUFFER, sizeof(tileColors), tileColors, GL_STATIC_DRAW);

        GLuint vertexShader = compileShader(GL_VERTEX_SHADER, vertexShaderSource);
        GLuint fragmentShader = compileShader(GL_FRAGMENT_SHADER, fragmentShaderSource);

        programID = glCreateProgram();
        glAttachShader(programID, vertexShader);
        glAttachShader(programID, fragmentShader);
        glLinkProgram(programID);

        int success;
        char infoLog[512];
        glGetProgramiv(programID, GL_LINK_STATUS, &success);
        if (!success) {
            glGetProgramInfoLog(programID, 512, NULL, infoLog);
            std::cerr << "Program Linking Error: " << infoLog << std::endl;
        }

        glDeleteShader(vertexShader);
        glDeleteShader(fragmentShader);

        mvpMatrixID = glGetUniformLocation(programID, "MVP");
    }

    void render(glm::mat4 viewProjection) {
        glUseProgram(programID);

        glBindVertexArray(vertexArrayID);

        glEnableVertexAttribArray(0);
        glBindBuffer(GL_ARRAY_BUFFER, vertexBufferID);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, 0);

        glEnableVertexAttribArray(1);
        glBindBuffer(GL_ARRAY_BUFFER, colorBufferID);
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 0, 0);

        glm::mat4 model = glm::mat4(1.0f);
        model = glm::translate(model, position);
        model = glm::scale(model, glm::vec3(scale));
        glm::mat4 mvp = viewProjection * model;

        glUniformMatrix4fv(mvpMatrixID, 1, GL_FALSE, &mvp[0][0]);

        glDrawArrays(GL_TRIANGLES, 0, 6);

        glDisableVertexAttribArray(0);
        glDisableVertexAttribArray(1);

        glBindVertexArray(0);
    }

    void cleanup() {
        glDeleteBuffers(1, &vertexBufferID);
        glDeleteBuffers(1, &colorBufferID);
        glDeleteVertexArrays(1, &vertexArrayID);
        glDeleteProgram(programID);
    }
};

std::vector<Tile> tiles;

void generateTiles(glm::vec3 position) {
    int centerX = static_cast<int>(std::floor(position.x / cellSize));
    int centerZ = static_cast<int>(std::floor(position.z / cellSize));

    for (int x = centerX - renderDistance; x <= centerX + renderDistance; ++x) {
        for (int z = centerZ - renderDistance; z <= centerZ + renderDistance; ++z) {
            auto chunk = std::make_tuple(x, z);
            if (std::find_if(tiles.begin(), tiles.end(), [&](const Tile& t) {
                return t.position == glm::vec3(x * cellSize, 0.0f, z * cellSize);
            }) == tiles.end()) {
                glm::vec3 tilePosition = glm::vec3(x * cellSize, 0.0f, z * cellSize);
                Tile newTile(tilePosition, cellSize);
                newTile.initialize();
                tiles.push_back(newTile);
            }
        }
    }
}

int main() {

    // Initialise GLFW
	if (!glfwInit())
	{
		std::cerr << "Failed to initialize GLFW." << std::endl;
		return -1;
	}

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    window = glfwCreateWindow(windowWidth, windowHeight, "Perlin Noise Infinite Scene with gaps", NULL, NULL);
    if (window == NULL) {
        std::cerr << "Failed to open a GLFW window." << std::endl;
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);

    glfwSetCursorPosCallback(window, mouse_callback);
    glfwSetKeyCallback(window, key_callback);
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

    // Initialize GLAD
    if (!gladLoadGL(glfwGetProcAddress)) {
        std::cerr << "Failed to initialize GLAD" << std::endl;
        return -1;
    }

    glClearColor(0.53f, 0.81f, 0.92f, 1.0f);
    glEnable(GL_DEPTH_TEST);

    // Compile and link shaders
    GLuint vertexShader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertexShader, 1, &vertexShaderSource, NULL);
    glCompileShader(vertexShader);

    GLuint fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragmentShader, 1, &fragmentShaderSource, NULL);
    glCompileShader(fragmentShader);

    GLuint programID = glCreateProgram();
    glAttachShader(programID, vertexShader);
    glAttachShader(programID, fragmentShader);
    glLinkProgram(programID);

    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);

    //generateTileGrid(cameraPos);
    generateTiles(cameraPos);

    while (!glfwWindowShouldClose(window)) {
        handleCameraMovement(window);

        static glm::vec3 lastCameraPos = cameraPos;
        if (glm::distance(lastCameraPos, cameraPos) > 1.0f) {
            generateTiles(cameraPos);
            lastCameraPos = cameraPos;
        }

        glm::mat4 view = glm::lookAt(cameraPos, cameraPos + cameraFront, cameraUp);
        glm::mat4 projection = glm::perspective(glm::radians(45.0f), 1024.0f / 768.0f, 0.1f, 100.0f);
        glm::mat4 vp = projection * view;

        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        for (auto& tile : tiles) {
            tile.render(vp);
        }

        glfwSwapBuffers(window);
        glfwPollEvents();
    }   

    // Cleanup resources
    for (auto& tile : tiles) {
        tile.cleanup();
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
