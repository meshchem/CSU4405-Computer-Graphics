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
static float cameraSpeed = 0.05f; // Adjust as needed

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

/*
std::vector<Tile> tiles;

void generateTileGrid(glm::vec3 cameraPos) {
    tiles.clear();
    int gridSize = 10; // Number of tiles in each direction
    float tileSize = 0.5f;

    for (int x = -gridSize; x <= gridSize; ++x) {
        for (int z = -gridSize; z <= gridSize; ++z) {
            glm::vec3 tilePos = glm::vec3(x * tileSize * 2.0f, 0.0f, z * tileSize * 2.0f);
            if (glm::distance(tilePos, glm::vec3(cameraPos.x, 0.0f, cameraPos.z)) < gridSize * tileSize * 2.0f) {
                tiles.emplace_back(tilePos, tileSize);
                tiles.back().initialize();
            }
        }
    }
}
*/

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

    window = glfwCreateWindow(windowWidth, windowHeight, "Flat Green Infinite Scene", NULL, NULL);
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
