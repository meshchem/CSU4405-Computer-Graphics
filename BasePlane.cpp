#include <glad/gl.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <render/shader.h>
#include <vector>
#include <iostream>
#include <string>
#include <tuple>
#include <unordered_set>
#define STB_IMAGE_IMPLEMENTATION
#include <stb/stb_image.h>
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include <stb/stb_image_write.h>
#define _USE_MATH_DEFINES
#include <math.h>

//user control functions
void handleCameraMovement(GLFWwindow* window);
void mouse_callback(GLFWwindow* window, double xpos, double ypos);
void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods);

static GLFWwindow *window;
static int windowWidth = 1024;
static int windowHeight = 768;

// Camera variables
static glm::vec3 cameraPos(0.0f, 50.0f, 3.0f);
static glm::vec3 cameraFront(0.0f, -1.0f, 0.0f);
static glm::vec3 cameraUp(0.0f, 1.0f, 0.0f);
static float yaw = -90.0f;
static float pitch = 0.0f;
static float lastX = windowWidth / 2.0f;
static float lastY = windowHeight / 2.0f;
static bool firstMouse = true;
static float cameraSpeed = 0.1f;

// Lighting control
const glm::vec3 wave500(0.0f, 255.0f, 146.0f);
const glm::vec3 wave600(255.0f, 190.0f, 0.0f);
const glm::vec3 wave700(205.0f, 0.0f, 0.0f);
static glm::vec3 lightIntensity = 5.0f * (8.0f * wave500 + 15.6f * wave600 + 18.4f * wave700);
//static glm::vec3 lightPosition(-275.0f, 500.0f, -275.0f);
static glm::vec3 lightPosition = glm::normalize(glm::vec3(-0.5f, -1.0f, -0.3f));


// OpenGL perspective parameters
static float FoV = 0.1f;
static float zNear = 600.0f;
static float zFar = 1500.0f;

static const float cellSize = 10.0f;
static const int renderDistance = 5;

static GLuint LoadTexture(const char *texture_file_path) {
    int w, h, channels;
    uint8_t* img = stbi_load(texture_file_path, &w, &h, &channels, 3);
    GLuint texture;
    glGenTextures(1, &texture);  
    glBindTexture(GL_TEXTURE_2D, texture);  

    // To tile textures on a box, we set wrapping to repeat
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);	
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

	glPixelStorei(GL_UNPACK_ALIGNMENT, 1);     // Set pixel storage mode to ensure correct alignment

    if (img) {
		std::cout << "Texture loaded: " << texture_file_path << std::endl;
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, w, h, 0, GL_RGB, GL_UNSIGNED_BYTE, img);
        glGenerateMipmap(GL_TEXTURE_2D);
    } else {
        std::cout << "Failed to load texture " << texture_file_path << std::endl;
    }
	
    stbi_image_free(img);

    return texture;
}

// Global variables
GLuint tileProgramID;
GLuint buildingProgramID;

struct Tile {
    float tileVertices[20] = {
        -0.5f, 0.0f, -0.5f,  0.0f, 0.0f,
         0.5f, 0.0f, -0.5f,  1.0f, 0.0f,
         0.5f, 0.0f,  0.5f,  1.0f, 1.0f,
        -0.5f, 0.0f,  0.5f,  0.0f, 1.0f
    };

    unsigned int tileIndices[6] = {
        0, 1, 2,
        0, 2, 3
    };

    GLuint vertexArrayID;
    GLuint vertexBufferID;
    GLuint indexBufferID;
    GLuint textureID;
    //GLuint tileProgramID;
    GLuint mvpMatrixID;

    glm::vec3 position;
    float scale;

    //constructor
    Tile(glm::vec3 pos, float size) : position(pos), scale(size) {}

    void initialize(const std::string& textureFilePath) {
        glGenVertexArrays(1, &vertexArrayID);
        glBindVertexArray(vertexArrayID);

        glGenBuffers(1, &vertexBufferID);
        glBindBuffer(GL_ARRAY_BUFFER, vertexBufferID);
        glBufferData(GL_ARRAY_BUFFER, sizeof(tileVertices), tileVertices, GL_STATIC_DRAW);

        glGenBuffers(1, &indexBufferID);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, indexBufferID);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(tileIndices), tileIndices, GL_STATIC_DRAW);

        tileProgramID = LoadShadersFromFile("../FinalProject/map.vert", "../FinalProject/map.frag");
		if (tileProgramID == 0)
		{
			std::cerr << "Failed to load shaders." << std::endl;
		}

        textureID = LoadTexture(textureFilePath.c_str());

        mvpMatrixID = glGetUniformLocation(tileProgramID, "MVP");
    }

    void render(glm::mat4 viewProjection) {
        glUseProgram(tileProgramID);

        glBindVertexArray(vertexArrayID);

        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0);

        glEnableVertexAttribArray(1);
        glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(3 * sizeof(float)));

        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, textureID);
        glUniform1i(glGetUniformLocation(tileProgramID, "texture1"), 0);

        glm::mat4 model = glm::translate(glm::mat4(1.0f), position);
        model = glm::scale(model, glm::vec3(scale));
        glm::mat4 mvp = viewProjection * model;
        glUniformMatrix4fv(mvpMatrixID, 1, GL_FALSE, &mvp[0][0]);

        glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);

        glBindVertexArray(0);
    }

    void cleanup() {
        glDeleteBuffers(1, &vertexBufferID);
        glDeleteBuffers(1, &indexBufferID);
        glDeleteVertexArrays(1, &vertexArrayID);
        glDeleteTextures(1, &textureID);
        glDeleteProgram(tileProgramID);
    }
};

struct Building {
	glm::vec3 position;		// Position of the box 
	glm::vec3 scale;		// Size of the box in each axis
	
	GLfloat vertexData[72] = {	// Vertex definition for a canonical box
		// Front face
		-1.0f, -1.0f, 1.0f, 
		1.0f, -1.0f, 1.0f, 
		1.0f, 1.0f, 1.0f, 
		-1.0f, 1.0f, 1.0f, 
		
		// Back face 
		1.0f, -1.0f, -1.0f, 
		-1.0f, -1.0f, -1.0f, 
		-1.0f, 1.0f, -1.0f, 
		1.0f, 1.0f, -1.0f,
		
		// Left face
		-1.0f, -1.0f, -1.0f, 
		-1.0f, -1.0f, 1.0f, 
		-1.0f, 1.0f, 1.0f, 
		-1.0f, 1.0f, -1.0f, 

		// Right face 
		1.0f, -1.0f, 1.0f, 
		1.0f, -1.0f, -1.0f, 
		1.0f, 1.0f, -1.0f, 
		1.0f, 1.0f, 1.0f,

		// Top face
		-1.0f, 1.0f, 1.0f, 
		1.0f, 1.0f, 1.0f, 
		1.0f, 1.0f, -1.0f, 
		-1.0f, 1.0f, -1.0f, 

		// Bottom face
		-1.0f, -1.0f, -1.0f, 
		1.0f, -1.0f, -1.0f, 
		1.0f, -1.0f, 1.0f, 
		-1.0f, -1.0f, 1.0f, 
	};

	GLfloat colorData[72] = {
		// Front, red
		1.0f, 0.0f, 0.0f,
		1.0f, 0.0f, 0.0f,
		1.0f, 0.0f, 0.0f,
		1.0f, 0.0f, 0.0f,

		// Back, yellow
		1.0f, 1.0f, 0.0f,
		1.0f, 1.0f, 0.0f,
		1.0f, 1.0f, 0.0f,
		1.0f, 1.0f, 0.0f,

		// Left, green
		0.0f, 1.0f, 0.0f, 
		0.0f, 1.0f, 0.0f,
		0.0f, 1.0f, 0.0f,
		0.0f, 1.0f, 0.0f,

		// Right, cyan
		0.0f, 1.0f, 1.0f, 
		0.0f, 1.0f, 1.0f, 
		0.0f, 1.0f, 1.0f, 
		0.0f, 1.0f, 1.0f, 

		// Top, blue
		0.0f, 0.0f, 1.0f, 
		0.0f, 0.0f, 1.0f,
		0.0f, 0.0f, 1.0f,
		0.0f, 0.0f, 1.0f,

		// Bottom, magenta
		1.0f, 0.0f, 1.0f,
		1.0f, 0.0f, 1.0f, 
		1.0f, 0.0f, 1.0f, 
		1.0f, 0.0f, 1.0f,  
	};

	GLuint indexData[36] = {		// 12 triangle faces of a box
		0, 1, 2, 	
		0, 2, 3, 
		4, 5, 6, 
		4, 6, 7, 
		8, 9, 10, 
		8, 10, 11, 
		12, 13, 14, 
		12, 14, 15, 
		16, 17, 18, 
		16, 18, 19, 
		20, 21, 22, 
		20, 22, 23, 
	};

	GLfloat uvData[48] = {
		// Front
		0.0f, 1.0f,
		1.0f, 1.0f,
		1.0f, 0.0f,
		0.0f, 0.0f,
		// Back
		0.0f, 1.0f,
		1.0f, 1.0f,
		1.0f, 0.0f,
		0.0f, 0.0f,
		// Left
		0.0f, 1.0f,
		1.0f, 1.0f,
		1.0f, 0.0f,
		0.0f, 0.0f,
		// Right
		0.0f, 1.0f,
		1.0f, 1.0f,
		1.0f, 0.0f,
		0.0f, 0.0f,
		// Top - we do not want texture the top
		0.0f, 0.0f,
		0.0f, 0.0f,
		0.0f, 0.0f,
		0.0f, 0.0f,
		// Bottom - we do not want texture the bottom
		0.0f, 0.0f,
		0.0f, 0.0f,
		0.0f, 0.0f,
		0.0f, 0.0f,
 	};
    
    GLfloat normalData[72] = {
        // Front face
        0.0f, 0.0f, 1.0f, 
        0.0f, 0.0f, 1.0f, 
        0.0f, 0.0f, 1.0f, 
        0.0f, 0.0f, 1.0f,

        // Back face
        0.0f, 0.0f, -1.0f, 
        0.0f, 0.0f, -1.0f, 
        0.0f, 0.0f, -1.0f, 
        0.0f, 0.0f, -1.0f,

        // Left face
        -1.0f, 0.0f, 0.0f, 
        -1.0f, 0.0f, 0.0f, 
        -1.0f, 0.0f, 0.0f, 
        -1.0f, 0.0f, 0.0f,

        // Right face
        1.0f, 0.0f, 0.0f, 
        1.0f, 0.0f, 0.0f, 
        1.0f, 0.0f, 0.0f, 
        1.0f, 0.0f, 0.0f,

        // Top face
        0.0f, 1.0f, 0.0f, 
        0.0f, 1.0f, 0.0f, 
        0.0f, 1.0f, 0.0f, 
        0.0f, 1.0f, 0.0f,

        // Bottom face
        0.0f, -1.0f, 0.0f, 
        0.0f, -1.0f, 0.0f, 
        0.0f, -1.0f, 0.0f, 
        0.0f, -1.0f, 0.0f,
    };

    // OpenGL buffers
    GLuint vaoID; // Vertex Array Object
    GLuint vboVerticesID; // Vertex Buffer Object for vertices
    GLuint vboColorsID; // Vertex Buffer Object for colors
    GLuint vboUVsID; // Vertex Buffer Object for UV coordinates
    GLuint eboIndicesID; // Element Buffer Object for indices
    GLuint textureObjID; // Texture Object ID
    GLuint NormalBufferID; // vertex buffer object for normals

    // Shader variable IDs
    GLuint mvpMatrixUniformID; // Uniform ID for MVP matrix
    GLuint textureSamplerUniformID; // Uniform ID for texture sampler
    //GLuint lightPositionID;
    GLuint lightPositionID;
    GLuint lightIntensityID;
    

    // Constructor
    Building(glm::vec3 position, glm::vec3 scale)
        : position(position), scale(scale) {}

    void initialize(glm::vec3 position, glm::vec3 scale, const std::string& textureFilePath) {
        this->position = position;
        this->scale = glm::vec3(scale.x * 0.5f, scale.y, scale.z * 0.5f);

        // Create a Vertex Array Object
        glGenVertexArrays(1, &vaoID);
        glBindVertexArray(vaoID);

        // Vertex buffer for vertex positions
        glGenBuffers(1, &vboVerticesID);
        glBindBuffer(GL_ARRAY_BUFFER, vboVerticesID);
        glBufferData(GL_ARRAY_BUFFER, sizeof(vertexData), vertexData, GL_STATIC_DRAW);

        // Vertex buffer for colors
        glGenBuffers(1, &vboColorsID);
        glBindBuffer(GL_ARRAY_BUFFER, vboColorsID);
        for (int i = 0; i < 72; ++i) {
            colorData[i] = 1.0f; // Set each color component to 1
        }
        glBufferData(GL_ARRAY_BUFFER, sizeof(colorData), colorData, GL_STATIC_DRAW);
        for (int i = 0; i < 24; ++i) uvData[2 * i + 1] *= 5;

        // Vertex buffer for UV coordinates
        glGenBuffers(1, &vboUVsID);
        glBindBuffer(GL_ARRAY_BUFFER, vboUVsID);
        glBufferData(GL_ARRAY_BUFFER, sizeof(uvData), uvData, GL_STATIC_DRAW);

        // Setup normal buffer
        glGenBuffers(1, &NormalBufferID);
        glBindBuffer(GL_ARRAY_BUFFER, NormalBufferID);
        glBufferData(GL_ARRAY_BUFFER, sizeof(normalData), normalData, GL_STATIC_DRAW);

        // Element buffer for triangle indices
        glGenBuffers(1, &eboIndicesID);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, eboIndicesID);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indexData), indexData, GL_STATIC_DRAW);

        // Load shaders and compile the shader program
        buildingProgramID = LoadShadersFromFile("../FinalProject/box.vert", "../FinalProject/box.frag");
        if (buildingProgramID == 0) {
            std::cerr << "Failed to load shaders." << std::endl;
        }

        //uniforms
        mvpMatrixUniformID = glGetUniformLocation(buildingProgramID, "MVP");
        textureObjID = LoadTexture(textureFilePath.c_str());
        textureSamplerUniformID = glGetUniformLocation(buildingProgramID, "textureSampler");
        lightPositionID = glGetUniformLocation(buildingProgramID, "lightPosition");
        lightIntensityID = glGetUniformLocation(buildingProgramID, "lightIntensity");
        
    }

    void render(glm::mat4 cameraMatrix, glm::vec3 cameraPosition, glm::vec3 lightPosition, glm::vec3 lightIntensity ) {
        glUseProgram(buildingProgramID);

        // Set the MVP matrix
        glm::mat4 modelMatrix = glm::mat4(1.0f);
        modelMatrix = glm::translate(modelMatrix, position);
        modelMatrix = glm::scale(modelMatrix, scale);

        glm::mat4 mvp = glm::mat4(1.0f);
        mvp = cameraMatrix * modelMatrix;
        
        glUniformMatrix4fv(mvpMatrixUniformID, 1, GL_FALSE, &mvp[0][0]);

        // Pass lighting uniforms
        glUniform3fv(lightPositionID, 1, &lightPosition[0]);
        glUniform3fv(lightIntensityID, 1, &lightIntensity[0]);
        glUniform3fv(glGetUniformLocation(buildingProgramID, "viewPosition"), 1, &cameraPosition[0]);

        glUniform1f(glGetUniformLocation(buildingProgramID, "constant"), 1.0f);  // Base light level
        glUniform1f(glGetUniformLocation(buildingProgramID, "linear"), 0.09f);  // Linear decay
        glUniform1f(glGetUniformLocation(buildingProgramID, "quadratic"), 0.032f); // Quadratic decay


        glEnableVertexAttribArray(0);
        glBindBuffer(GL_ARRAY_BUFFER, vboVerticesID);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, 0);

        glEnableVertexAttribArray(1);
        glBindBuffer(GL_ARRAY_BUFFER, vboColorsID);
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 0, 0);

        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, eboIndicesID);
;
        // Enable UV buffer and texture sampler
        glEnableVertexAttribArray(2);
        glBindBuffer(GL_ARRAY_BUFFER, vboUVsID);
        glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 0, 0);

        // Set the texture sampler to use texture unit 0
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, textureObjID);
        glUniform1i(textureSamplerUniformID, 0);

        // Enable Normal Buffer 
        glEnableVertexAttribArray(3);
        glBindBuffer(GL_ARRAY_BUFFER, NormalBufferID);
        glVertexAttribPointer(3, 3, GL_FLOAT, GL_FALSE, 0, 0);

        // Draw the building
        glDrawElements(GL_TRIANGLES, 36, GL_UNSIGNED_INT, (void*)0);

        glDisableVertexAttribArray(0);
        glDisableVertexAttribArray(1);
        glDisableVertexAttribArray(2);
        glDisableVertexAttribArray(3);
        
    }

    void cleanup() {
        glDeleteBuffers(1, &vboVerticesID);
        glDeleteBuffers(1, &vboColorsID);
        glDeleteBuffers(1, &eboIndicesID);
        glDeleteBuffers(1, &NormalBufferID);
        glDeleteVertexArrays(1, &vaoID);
        glDeleteBuffers(1, &vboUVsID);
        glDeleteTextures(1, &textureObjID);
        glDeleteProgram(buildingProgramID);
    }

}; 

std::vector<Tile> tiles;
std::vector<Building> buildings;
std::vector<std::string> BuildingFacades;

void generateTiles(glm::vec3 position) {
    int centerX = static_cast<int>(std::floor(position.x / cellSize));
    int centerZ = static_cast<int>(std::floor(position.z / cellSize));

    for (int x = centerX - renderDistance; x <= centerX + renderDistance; ++x) {
        for (int z = centerZ - renderDistance; z <= centerZ + renderDistance; ++z) {
            glm::vec3 tilePosition = glm::vec3(x * cellSize, 0.0f, z * cellSize);
            if (std::none_of(tiles.begin(), tiles.end(), [&](const Tile& t) {
                return t.position == tilePosition;
            })) {
                Tile newTile(tilePosition, cellSize);
                newTile.initialize("../FinalProject/floor_texture.jpg");
                tiles.push_back(newTile);
            }
        }
    }
}

// Function to initialize the BuildingFacades vector
void initializeBuildingFacades() {
    BuildingFacades.push_back("../FinalProject/facade0.jpg");
   // BuildingFacades.push_back("../FinalProject/facade1.jpg");
    BuildingFacades.push_back("../FinalProject/facade2.jpg");
    BuildingFacades.push_back("../FinalProject/facade3.jpg");
    BuildingFacades.push_back("../FinalProject/facade4.jpg");
}

// Function to get a random facade texture
std::string getRandomFacade() {
    int randomIndex = rand() % BuildingFacades.size();
    return BuildingFacades[randomIndex];
}

void generateBuildings(glm::vec3 position) {
    int centerX = static_cast<int>(std::floor(position.x / cellSize));
    int centerZ = static_cast<int>(std::floor(position.z / cellSize));

    for (int x = centerX - renderDistance; x <= centerX + renderDistance; ++x) {
        for (int z = centerZ - renderDistance; z <= centerZ + renderDistance; ++z) {
            glm::vec3 buildingPosition = glm::vec3(x * cellSize, 0.0f, z * cellSize);
            if (rand() % 2 == 0 && std::none_of(buildings.begin(), buildings.end(), [&](const Building& b) {
                return b.position.x == buildingPosition.x && b.position.z == buildingPosition.z;
            })) {
                float buildingHeight = 5.0f + static_cast<float>(rand() % 15); // Random height
                glm::vec3 buildingScale(5.0f, buildingHeight, 5.0f);

                // Ensure the building starts on top of the tile
                glm::vec3 adjustedBuildingPosition = buildingPosition + glm::vec3(0.0f, buildingHeight / 2.0f, 0.0f);

                // Create and initialize the building
                Building newBuilding(adjustedBuildingPosition, buildingScale);
                newBuilding.initialize(buildingPosition, buildingScale, getRandomFacade());
                buildings.push_back(newBuilding);

            }
        }
    }
}



int main() {
    if (!glfwInit()) {
        std::cerr << "Failed to initialize GLFW." << std::endl;
        return -1;
    }

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    window = glfwCreateWindow(windowWidth, windowHeight, "Infinite Scene with Buildings", NULL, NULL);
    if (window == NULL) {
        std::cerr << "Failed to open a GLFW window." << std::endl;
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

    glClearColor(0.05f, 0.05f, 0.2f, 1.0f);
    glEnable(GL_DEPTH_TEST);

    srand(static_cast<unsigned int>(time(nullptr)));
    initializeBuildingFacades();

    while (!glfwWindowShouldClose(window)) {
    // Handle camera movement
        handleCameraMovement(window);

        // Generate tiles and buildings dynamically based on the camera position
        generateTiles(cameraPos);
        generateBuildings(cameraPos);

        // Calculate view-projection matrix
        glm::mat4 view = glm::lookAt(cameraPos, cameraPos + cameraFront, cameraUp);
        glm::mat4 projection = glm::perspective(glm::radians(45.0f), (float)windowWidth / windowHeight, 0.1f, 100.0f);
        glm::mat4 vp = projection * view;

        // Clear the screen and enable depth testing
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        glEnable(GL_DEPTH_TEST);

        // Render Tiles
        glUseProgram(tileProgramID);
        for (auto& tile : tiles) {
            glBindVertexArray(tile.vertexArrayID);
            glBindBuffer(GL_ARRAY_BUFFER, tile.vertexBufferID);
            glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, tile.indexBufferID);

            // Render the tile
            tile.render(vp);
        }
        glUseProgram(0); // Unbind the tile shader program

        // Render Buildings
        glUseProgram(buildingProgramID);
        glUniform3fv(glGetUniformLocation(buildingProgramID, "lightPosition"), 1, &lightPosition[0]);

        for (auto& building : buildings) {
            glBindVertexArray(building.vaoID);
            glBindBuffer(GL_ARRAY_BUFFER, building.vboVerticesID);
            glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, building.eboIndicesID);

            // Pass lighting uniforms
            glUniform3fv(building.lightPositionID, 1, &lightPosition[0]);
            glUniform3fv(building.lightIntensityID, 1, &lightIntensity[0]);
            glUniform3fv(glGetUniformLocation(buildingProgramID, "viewPosition"), 1, &cameraPos[0]);

            // Render the building
            building.render(vp, cameraPos, lightPosition, lightIntensity);
        }
        glUseProgram(0); // Unbind the building shader program

        // Swap buffers and poll events
        glfwSwapBuffers(window);
        glfwPollEvents();
    }

        for (auto& tile : tiles) {
            tile.cleanup();
        }

        for (auto& building : buildings) {
            building.cleanup();
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
    float yoffset = lastY - ypos;
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

void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods) {
    if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
        glfwSetWindowShouldClose(window, GL_TRUE);
}
