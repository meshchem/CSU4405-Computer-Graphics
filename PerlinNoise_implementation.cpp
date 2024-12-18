#include <glad/gl.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#define STB_PERLIN_IMPLEMENTATION
#include "stb_perlin.h"
#include <vector>
#include <iostream>
#include <string>
#include <sstream>

#define BUFFER_OFFSET(i) ((char *)NULL + (i))

static GLFWwindow *window;
static int windowWidth = 1024;
static int windowHeight = 768;

// Camera
static glm::vec3 cameraPos(0.0f, 10.0f, 0.0f);
static glm::vec3 cameraFront(0.0f, 0.0f, -1.0f);
static glm::vec3 cameraUp(0.0f, 1.0f, 0.0f);
static float yaw = -90.0f;
static float pitch = 0.0f;
static float cameraSpeed = 75.0f; // Adjust as necessary
static float deltaTime = 0.0f;
static float lastFrame = 0.0f;

// Frame rate
static float frameRate = 0.0f;
static float frameTime = 0.0f;

// Terrain grid parameters
static const int gridSize = 100;
static const float tileSize = 1.0f;

GLuint gridVAO, gridVBO;

static void key_callback(GLFWwindow *window, int key, int scancode, int action, int mode);
void generateGrid();
void renderGrid(const glm::mat4 &viewProjMatrix, GLuint programID);
void setupShaders(GLuint &programID);
void renderText(float frameRate);

int main(void) {
    // Initialize GLFW
    if (!glfwInit()) {
        std::cerr << "Failed to initialize GLFW." << std::endl;
        return -1;
    }

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE); // For MacOS
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    // Open a window and create its OpenGL context
    window = glfwCreateWindow(windowWidth, windowHeight, "Infinite Grid", NULL, NULL);
    if (window == NULL) {
        std::cerr << "Failed to open a GLFW window." << std::endl;
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);

    // Capture escape key
    glfwSetInputMode(window, GLFW_STICKY_KEYS, GL_TRUE);
    glfwSetKeyCallback(window, key_callback);

    // Load OpenGL functions, gladLoadGL returns the loaded version, 0 on error.
    int version = gladLoadGL(glfwGetProcAddress);
    if (version == 0) {
        std::cerr << "Failed to initialize OpenGL context." << std::endl;
        return -1;
    }

    // Setup OpenGL
    glEnable(GL_DEPTH_TEST);
    glClearColor(0.2f, 0.2f, 0.25f, 0.0f);

    // Generate grid
    generateGrid();

    // Setup shaders
    GLuint programID;
    setupShaders(programID);

    // Projection matrix
    glm::mat4 projection = glm::perspective(glm::radians(45.0f), (float)windowWidth / windowHeight, 0.1f, 100.0f);

    // Main loop
    while (!glfwWindowShouldClose(window)) {
        // Calculate frame time
        float currentFrame = glfwGetTime();
        deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;

        // Calculate frame rate
        frameRate = 1.0f / deltaTime;

        // Clear buffers
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // View matrix
        glm::mat4 view = glm::lookAt(cameraPos, cameraPos + cameraFront, cameraUp);
        glm::mat4 viewProj = projection * view;

        // Render the grid
        renderGrid(viewProj, programID);

        // Render frame rate
        renderText(frameRate);

        // Swap buffers and poll events
        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    // Cleanup
    glDeleteVertexArrays(1, &gridVAO);
    glDeleteBuffers(1, &gridVBO);
    glDeleteProgram(programID);

    glfwTerminate();
    return 0;
}

void generateGrid() {
    std::vector<float> vertices;
    std::vector<unsigned int> indices;

    for (int z = -gridSize; z <= gridSize; ++z) {
        for (int x = -gridSize; x <= gridSize; ++x) {
            float xPos = x * tileSize;
            float zPos = z * tileSize;

            vertices.push_back(xPos);
            float yPos = stb_perlin_noise3(x * 0.1f, 0.0f, z * 0.1f, 0, 0, 0) * 2.0f; // Adjust scaling and amplitude
            vertices.push_back(yPos);
            vertices.push_back(zPos);

            // Add texture coordinates
            vertices.push_back((float)(x + gridSize) / (gridSize * 2));
            vertices.push_back((float)(z + gridSize) / (gridSize * 2));
        }
    }

    for (int z = 0; z < gridSize * 2; ++z) {
        for (int x = 0; x < gridSize * 2; ++x) {
            unsigned int topLeft = z * (gridSize * 2 + 1) + x;
            unsigned int topRight = topLeft + 1;
            unsigned int bottomLeft = (z + 1) * (gridSize * 2 + 1) + x;
            unsigned int bottomRight = bottomLeft + 1;

            indices.push_back(topLeft);
            indices.push_back(bottomLeft);
            indices.push_back(topRight);

            indices.push_back(topRight);
            indices.push_back(bottomLeft);
            indices.push_back(bottomRight);
        }
    }

    glGenVertexArrays(1, &gridVAO);
    glGenBuffers(1, &gridVBO);

    glBindVertexArray(gridVAO);

    GLuint gridEBO;
    glGenBuffers(1, &gridEBO);

    glBindBuffer(GL_ARRAY_BUFFER, gridVBO);
    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(float), vertices.data(), GL_STATIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, gridEBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(unsigned int), indices.data(), GL_STATIC_DRAW);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);

    glBindVertexArray(0);
}

void renderGrid(const glm::mat4 &viewProjMatrix, GLuint programID) {
    glUseProgram(programID);

    GLuint matrixID = glGetUniformLocation(programID, "viewProj");
    glUniformMatrix4fv(matrixID, 1, GL_FALSE, &viewProjMatrix[0][0]);

    glBindVertexArray(gridVAO);
    glDrawElements(GL_TRIANGLES, (gridSize * 2) * (gridSize * 2) * 6, GL_UNSIGNED_INT, 0);
    glBindVertexArray(0);
}

void renderText(float frameRate) {
    std::ostringstream ss;
    ss << "FPS: " << frameRate;
    std::string text = ss.str();

    // For simplicity, we use the GLFW window title to display FPS
    glfwSetWindowTitle(window, text.c_str());
}

void setupShaders(GLuint &programID) {
    const char *vertexShaderSource = R"(
        #version 330 core
        layout(location = 0) in vec3 aPos;
        layout(location = 1) in vec2 aTexCoords;
        uniform mat4 viewProj;
        out vec2 TexCoords;
        void main() {
            gl_Position = viewProj * vec4(aPos, 1.0);
            TexCoords = aTexCoords;
        }
    )";

    const char *fragmentShaderSource = R"(
        #version 330 core
        in vec2 TexCoords;
        out vec4 FragColor;
        uniform sampler2D ourTexture;
        void main() {
            //FragColor = texture(ourTexture, TexCoords);
            FragColor = vec4(0.0, 0.8, 0.2, 1.0);
        }
    )";

    // Compile shaders and link program
    programID = glCreateProgram();
    GLuint vertexShader = glCreateShader(GL_VERTEX_SHADER);
    GLuint fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);

    glShaderSource(vertexShader, 1, &vertexShaderSource, NULL);
    glCompileShader(vertexShader);

    glShaderSource(fragmentShader, 1, &fragmentShaderSource, NULL);
    glCompileShader(fragmentShader);

    glAttachShader(programID, vertexShader);
    glAttachShader(programID, fragmentShader);
    glLinkProgram(programID);

    glUseProgram(programID);
    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);
}

void key_callback(GLFWwindow* window, int key, int scancode, int action, int mode) {
    // Exit application on pressing ESC
    if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);

    // Camera controls
    float cameraSpeedDelta = cameraSpeed * deltaTime;
    if (key == GLFW_KEY_W && (action == GLFW_PRESS || action == GLFW_REPEAT))
        cameraPos += cameraSpeedDelta * cameraFront;
    if (key == GLFW_KEY_S && (action == GLFW_PRESS || action == GLFW_REPEAT))
        cameraPos -= cameraSpeedDelta * cameraFront;
    if (key == GLFW_KEY_A && (action == GLFW_PRESS || action == GLFW_REPEAT))
        cameraPos -= glm::normalize(glm::cross(cameraFront, cameraUp)) * cameraSpeedDelta;
    if (key == GLFW_KEY_D && (action == GLFW_PRESS || action == GLFW_REPEAT))
        cameraPos += glm::normalize(glm::cross(cameraFront, cameraUp)) * cameraSpeedDelta;
}

