#include <glad/gl.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
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
void renderGrid(const glm::mat4 &viewProjMatrix);
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
        renderGrid(viewProj);

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
    std::vector<glm::vec3> vertices;

    for (int z = -gridSize; z <= gridSize; ++z) {
        for (int x = -gridSize; x <= gridSize; ++x) {
            vertices.emplace_back(x * tileSize, 0.0f, z * tileSize);
        }
    }

    glGenVertexArrays(1, &gridVAO);
    glGenBuffers(1, &gridVBO);

    glBindVertexArray(gridVAO);

    glBindBuffer(GL_ARRAY_BUFFER, gridVBO);
    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(glm::vec3), vertices.data(), GL_STATIC_DRAW);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(glm::vec3), (void*)0);
    glEnableVertexAttribArray(0);

    glBindVertexArray(0);
}

void renderGrid(const glm::mat4 &viewProjMatrix) {
    glBindVertexArray(gridVAO);

    GLuint matrixID = glGetUniformLocation(gridVAO, "viewProj");
    glUniformMatrix4fv(matrixID, 1, GL_FALSE, &viewProjMatrix[0][0]);

    glDrawArrays(GL_POINTS, 0, (gridSize * 2 + 1) * (gridSize * 2 + 1));

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
        uniform mat4 viewProj;
        void main() {
            gl_Position = viewProj * vec4(aPos, 1.0);
        }
    )";

    const char *fragmentShaderSource = R"(
        #version 330 core
        out vec4 FragColor;
        void main() {
            FragColor = vec4(0.0, 1.0, 0.0, 1.0); // Green color
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

static void key_callback(GLFWwindow *window, int key, int scancode, int action, int mode) {
    float velocity = cameraSpeed * deltaTime;
    if (key == GLFW_KEY_UP) pitch += velocity; // Tilt up
    if (key == GLFW_KEY_DOWN) pitch -= velocity; // Tilt down
    if (key == GLFW_KEY_LEFT) yaw -= velocity; // Pan left
    if (key == GLFW_KEY_RIGHT) yaw += velocity; // Pan right

    // Constrain pitch to avoid flipping
    if (pitch > 89.0f) pitch = 89.0f;
    if (pitch < -89.0f) pitch = -89.0f;

    // Update camera front based on yaw and pitch
    glm::vec3 direction;
    direction.x = cos(glm::radians(yaw)) * cos(glm::radians(pitch));
    direction.y = sin(glm::radians(pitch));
    direction.z = sin(glm::radians(yaw)) * cos(glm::radians(pitch));
    cameraFront = glm::normalize(direction);

    if (key == GLFW_KEY_W) cameraPos += glm::vec3(cameraFront.x, 0.0f, cameraFront.z) * velocity;  // Move forward
    if (key == GLFW_KEY_S) cameraPos -= glm::vec3(cameraFront.x, 0.0f, cameraFront.z) * velocity;  // Move backward
    if (key == GLFW_KEY_A) cameraPos -= glm::normalize(glm::cross(cameraFront, cameraUp)) * velocity; // Move left
    if (key == GLFW_KEY_D) cameraPos += glm::normalize(glm::cross(cameraFront, cameraUp)) * velocity; // Move right
    if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS) glfwSetWindowShouldClose(window, GL_TRUE);
}
