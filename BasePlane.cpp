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
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include <stb/stb_image.h>

#define _USE_MATH_DEFINES
#include <math.h>
#include <iomanip>
#include <sstream> // For std::stringstream

void handleCameraMovement(GLFWwindow* window);
void mouse_callback(GLFWwindow* window, double xpos, double ypos);
void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods);

static GLFWwindow *window;
static int windowWidth = 1024;
static int windowHeight = 768;

// Camera variables
static glm::vec3 cameraPos(0.0f, 5.0f, 4.0f);
static glm::vec3 cameraFront(0.0f, -1.0f, 0.0f);
static glm::vec3 cameraUp(0.0f, 1.0f, 0.0f);
static float yaw = -90.0f;
static float pitch = 0.0f;
static float lastX = windowWidth / 2.0f;
static float lastY = windowHeight / 2.0f;
static bool firstMouse = true;
static float cameraSpeed = 0.05f;

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
GLuint skyboxProgramID;

struct Skybox {
	glm::vec3 position;		// Position of the box 
	glm::vec3 scale;		// Size of the box in each axis
	
	GLfloat vertex_buffer_data[72] = {	// Vertex definition for a canonical box
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

	GLfloat color_buffer_data[72] = {
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

    
	// Modify the cube buffers so that the inner surfaces of the cube becomes front facing in counter-clockwise order.
	// to position viewer inside the cube need to render inner faces
    // reverse winding order, front face of each triangle is on inside of cube 
	GLuint index_buffer_data[36] = {		
        0, 3, 2,
		0, 2, 1,

		4, 7, 6,
		4, 6, 5,

		8, 11, 10,
		8, 10, 9,

		12, 15, 14,
		12, 14, 13,

		16, 19, 18,
		16, 18, 17,

		20, 23, 22,
		20, 22, 21,
 
	};

	GLfloat uv_buffer_data[48] = {
		// Pos Z
		0.5f, 0.666f,  	// Top right
		0.25f, 0.666f, 	// Top left
		0.25f, 0.333f, 	// Bottom left
		0.5f, 0.333f,  	// Bottom right

		// Neg Z
		1.0f, 0.666f,  	// Top right
		0.75f, 0.666f, 	// Top left
		0.75f, 0.333f, 	// Bottom left
		1.0f, 0.333f,  	// Bottom right

		// Pos X
		0.75f, 0.666f, 	// Top right
		0.5f, 0.666f,  	// Top left
		0.5f, 0.333f,  	// Bottom left
		0.75f, 0.333f, 	// Bottom right

		// Neg X
		0.25f, 0.666f, 	// Top right
		0.0f, 0.666f,  	// Top left
		0.0f, 0.333f,  	// Bottom left
		0.25f, 0.333f, 	// Bottom right

		// Neg Y
		0.5f, 0.333f,  	// Top right
		0.25f, 0.333f,  // Top left
		0.25f, 0.0f,   	// Bottom left
		0.5f, 0.0f,    	// Bottom right

        // Pos Y
		0.5f, 1.0f,    	// Top right
		0.25f, 1.0f,   	// Top left
		0.25f, 0.666f, 	// Bottom left
		0.5f, 0.666f  	// Bottom right

	};
    
	// OpenGL buffers
	GLuint vertexArrayID; 
	GLuint vertexBufferID; 
	GLuint indexBufferID; 
	GLuint colorBufferID;
	GLuint uvBufferID;
	GLuint textureID;

	// Shader variable IDs
	GLuint mvpMatrixID;
	GLuint textureSamplerID;
	

	void initialize(glm::vec3 position, glm::vec3 scale) {
		// Define scale of the skybox geometry
		this->position = position;
		this->scale = scale;

		// Create a vertex array object
		glGenVertexArrays(1, &vertexArrayID);
		glBindVertexArray(vertexArrayID);

		// Create a vertex buffer object to store the vertex data		
		glGenBuffers(1, &vertexBufferID);
		glBindBuffer(GL_ARRAY_BUFFER, vertexBufferID);
		glBufferData(GL_ARRAY_BUFFER, sizeof(vertex_buffer_data), vertex_buffer_data, GL_STATIC_DRAW);

		// Create a vertex buffer object to store the color data
        for (int i = 0; i < 72; ++i) {
        	color_buffer_data[i] = 1.0f; 
    	}
		glGenBuffers(1, &colorBufferID);
		glBindBuffer(GL_ARRAY_BUFFER, colorBufferID);		
		glBufferData(GL_ARRAY_BUFFER, sizeof(color_buffer_data), color_buffer_data, GL_STATIC_DRAW);


		// create a uv buffer object to store the uv data 
		glGenBuffers(1, &uvBufferID);
		glBindBuffer(GL_ARRAY_BUFFER, uvBufferID);
		glBufferData(GL_ARRAY_BUFFER, sizeof(uv_buffer_data), uv_buffer_data, GL_STATIC_DRAW);

		// Create an index buffer object to store the index data that defines triangle faces
		glGenBuffers(1, &indexBufferID);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, indexBufferID);
		glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(index_buffer_data), index_buffer_data, GL_STATIC_DRAW);

		// Create and compile our GLSL program from the shaders
		skyboxProgramID = LoadShadersFromFile("../FinalProject/skybox.vert", "../FinalProject/skybox.frag");
		if (skyboxProgramID == 0)
		{
			std::cerr << "Failed to load shaders." << std::endl;
		}

		// Get a handle for our "MVP" uniform
		mvpMatrixID = glGetUniformLocation(skyboxProgramID, "MVP");
		textureID = LoadTexture("../FinalProject/sky3.png");
		textureSamplerID = glGetUniformLocation(skyboxProgramID,"textureSampler");
	}

	void render(glm::mat4 cameraMatrix) {
		glUseProgram(skyboxProgramID);

		glEnableVertexAttribArray(0);
		glBindBuffer(GL_ARRAY_BUFFER, vertexBufferID);
		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, 0);

		glEnableVertexAttribArray(1);
		glBindBuffer(GL_ARRAY_BUFFER, colorBufferID);
		glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 0, 0);

		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, indexBufferID);

		// Model transform 

    	glm::mat4 modelMatrix = glm::mat4();
    	modelMatrix = glm::scale(modelMatrix, scale);

		// Set model-view-projection matrix
		glm::mat4 mvp = cameraMatrix * modelMatrix;
		glUniformMatrix4fv(mvpMatrixID, 1, GL_FALSE, &mvp[0][0]);  //sends mvp matrix to vertex shader

		// Enable UV buffer and texture sampler
		glEnableVertexAttribArray(2);
 		glBindBuffer(GL_ARRAY_BUFFER, uvBufferID);
 		glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 0, 0);

		 // Set textureSampler to use texture unit 0
		glActiveTexture(GL_TEXTURE0);
 		glBindTexture(GL_TEXTURE_2D, textureID);
 		glUniform1i(textureSamplerID, 0); 
	
		// Draw the box
		glDrawElements(
			GL_TRIANGLES,      // mode
			36,    			   // number of indices
			GL_UNSIGNED_INT,   // type
			(void*)0           // element array buffer offset
		);

		glDisableVertexAttribArray(0);
		glDisableVertexAttribArray(1);
        //glDisableVertexAttribArray(2);
	}

	void cleanup() {
		glDeleteBuffers(1, &vertexBufferID);
		glDeleteBuffers(1, &colorBufferID);
		glDeleteBuffers(1, &indexBufferID);
		glDeleteVertexArrays(1, &vertexArrayID);
		glDeleteBuffers(1, &uvBufferID);
		glDeleteTextures(1, &textureID);
		glDeleteProgram(skyboxProgramID);
	}
};

struct Tile {
    float tileVertices[20] = {
        -0.5f, 0.0f, -0.5f,  0.0f, 0.0f,
         0.5f, 0.0f, -0.5f,  1.0f, 0.0f,
         0.5f, 0.0f,  0.5f,  1.0f, 1.0f,
        -0.5f, 0.0f,  0.5f,  0.0f, 1.0f
    };

    float tileNormals[12] = {
        0.0f, 1.0f, 0.0f,
        0.0f, 1.0f, 0.0f,
        0.0f, 1.0f, 0.0f,
        0.0f, 1.0f, 0.0f
    };

    unsigned int tileIndices[6] = {
        0, 1, 2,
        0, 2, 3
    };

    GLuint vertexArrayID;
    GLuint vertexBufferID;
    GLuint normalBufferID;
    GLuint indexBufferID;
    
    // shader buffers
    GLuint textureID;
    GLuint mvpMatrixID;
    GLuint modelMatrixID; 
    GLuint lightPosID;
    GLuint lightColorID;

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

        glGenBuffers(1, &normalBufferID);
        glBindBuffer(GL_ARRAY_BUFFER, normalBufferID);
        glBufferData(GL_ARRAY_BUFFER, sizeof(tileNormals), tileNormals, GL_STATIC_DRAW);

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
        modelMatrixID = glGetUniformLocation(tileProgramID, "model");
        lightPosID = glGetUniformLocation(tileProgramID, "lightPos");
        lightColorID = glGetUniformLocation(tileProgramID, "lightColor");
    }

    void render(glm::mat4 viewProjection) {
        glUseProgram(tileProgramID);

        glBindVertexArray(vertexArrayID);

        glm::mat4 model = glm::translate(glm::mat4(1.0f), position);
        model = glm::scale(model, glm::vec3(scale));
        glUniformMatrix4fv(modelMatrixID, 1, GL_FALSE, &model[0][0]);
        
        glm::mat4 mvp = viewProjection * model;
        glUniformMatrix4fv(mvpMatrixID, 1, GL_FALSE, &mvp[0][0]);

        glm::vec3 lightPosition = glm::vec3(50.0f, 80.0f, 50.0f);
        glm::vec3 lightColor = glm::vec3(1.0f, 1.0f, 0.8f); // light yellow light
        glUniform3fv(lightPosID, 1, &lightPosition[0]);
        glUniform3fv(lightColorID, 1, &lightColor[0]);

        // Positions
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0);

        // UVs
        glEnableVertexAttribArray(1);
        glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(3 * sizeof(float)));

        // Normals
        glEnableVertexAttribArray(2); 
        glBindBuffer(GL_ARRAY_BUFFER, normalBufferID);
        glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, 0, 0);
       
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, textureID);
        glUniform1i(glGetUniformLocation(tileProgramID, "texture1"), 0);


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

    // Check if a point is inside the building's AABB
    bool isPointInside(const glm::vec3& point, float margin = 0.5f) const {
        return (point.x >= position.x - scale.x - margin && point.x <= position.x + scale.x + margin) &&
            (point.y >= position.y - scale.y - margin && point.y <= position.y + scale.y + margin) &&
            (point.z >= position.z - scale.z - margin && point.z <= position.z + scale.z + margin);
    }   
	
	GLfloat vertexData[72] = {	// Vertex definition for a canonical box
		// Front face
		-1.0f, -1.0f, 1.0f, 1.0f, -1.0f, 1.0f, 1.0f, 1.0f, 1.0f, -1.0f, 1.0f, 1.0f, 
		
		// Back face 
		1.0f, -1.0f, -1.0f, -1.0f, -1.0f, -1.0f, -1.0f, 1.0f, -1.0f, 1.0f, 1.0f, -1.0f,
		
		// Left face
        -1.0f, -1.0f, -1.0f, -1.0f, -1.0f, 1.0f, -1.0f, 1.0f, 1.0f, -1.0f, 1.0f, -1.0f, 

		// Right face 
		1.0f, -1.0f, 1.0f, 1.0f, -1.0f, -1.0f, 1.0f, 1.0f, -1.0f, 1.0f, 1.0f, 1.0f,

		// Top face
		-1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, -1.0f, -1.0f, 1.0f, -1.0f, 

		// Bottom face
        -1.0f, -1.0f, -1.0f, 1.0f, -1.0f, -1.0f, 1.0f, -1.0f, 1.0f, -1.0f, -1.0f, 1.0f, 
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
        // Front face normals
        0.0f, 0.0f, 1.0f,  0.0f, 0.0f, 1.0f,  0.0f, 0.0f, 1.0f,  0.0f, 0.0f, 1.0f,
        // Back face normals
        0.0f, 0.0f, -1.0f,  0.0f, 0.0f, -1.0f,  0.0f, 0.0f, -1.0f,  0.0f, 0.0f, -1.0f,
        // Left face normals
        -1.0f, 0.0f, 0.0f,  -1.0f, 0.0f, 0.0f,  -1.0f, 0.0f, 0.0f,  -1.0f, 0.0f, 0.0f,
        // Right face normals
        1.0f, 0.0f, 0.0f,  1.0f, 0.0f, 0.0f,  1.0f, 0.0f, 0.0f,  1.0f, 0.0f, 0.0f,
        // Top face normals
        0.0f, 1.0f, 0.0f,  0.0f, 1.0f, 0.0f,  0.0f, 1.0f, 0.0f,  0.0f, 1.0f, 0.0f,
        // Bottom face normals
        0.0f, -1.0f, 0.0f,  0.0f, -1.0f, 0.0f,  0.0f, -1.0f, 0.0f,  0.0f, -1.0f, 0.0f,

    };

    // OpenGL Buffers
    GLuint vaoID;           // Vertex Array Object
    GLuint vboVerticesID;   // Vertex Buffer Object for vertices
    GLuint vboColorsID;     // Vertex Buffer Object for colors
    GLuint vboUVsID;        // Vertex Buffer Object for UV coordinates
    GLuint vboNormalsID;    // Vertex Buffer Object for normals
    GLuint eboIndicesID;    // Element Buffer Object for indices

    // Shader and Texture IDs
    GLuint buildingProgramID;          // Shader program ID
    GLuint mvpUniformID;         // Uniform ID for MVP matrix
    GLuint modelUniformID;               // Uniform ID for model matrix
    GLuint textureUniformID;    // Uniform ID for texture sampler
    GLuint textureObjID;               // Texture object ID
    GLuint lightPosID;
    GLuint lightColorID;

    
    // Constructor
    Building(glm::vec3 position, glm::vec3 scale)
        : position(position), scale(scale) {}

        

    // Buffer setup function
    void setupBuffers() {
        // Generate and bind VAO
        glGenVertexArrays(1, &vaoID);
        glBindVertexArray(vaoID);

        // Vertex buffer
        glGenBuffers(1, &vboVerticesID);
        glBindBuffer(GL_ARRAY_BUFFER, vboVerticesID);
        glBufferData(GL_ARRAY_BUFFER, sizeof(vertexData), vertexData, GL_STATIC_DRAW);

        // Color buffer
        glGenBuffers(1, &vboColorsID);
        glBindBuffer(GL_ARRAY_BUFFER, vboColorsID);
        for (int i = 0; i < 72; ++i) {
            colorData[i] = 1.0f; // Set each color component to 1
        }
        glBufferData(GL_ARRAY_BUFFER, sizeof(colorData), colorData, GL_STATIC_DRAW);
        for (int i = 0; i < 24; ++i) uvData[2 * i + 1] *= 5;

        // UV buffer
        glGenBuffers(1, &vboUVsID);
        glBindBuffer(GL_ARRAY_BUFFER, vboUVsID);
        glBufferData(GL_ARRAY_BUFFER, sizeof(uvData), uvData, GL_STATIC_DRAW);

        // Normal buffer
        glGenBuffers(1, &vboNormalsID);
        glBindBuffer(GL_ARRAY_BUFFER, vboNormalsID);
        glBufferData(GL_ARRAY_BUFFER, sizeof(normalData), normalData, GL_STATIC_DRAW);

        // Element buffer
        glGenBuffers(1, &eboIndicesID);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, eboIndicesID);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indexData), indexData, GL_STATIC_DRAW);
        
    }

    // Initialization function
    void initialize(const glm::vec3& position, const glm::vec3& scale, const std::string& textureFilePath) {
        this->position = position;
        this->scale = glm::vec3(scale.x * 0.5f, scale.y, scale.z * 0.5f);
        //this->scale = scale * 0.5f;

        // Setup OpenGL buffers
        setupBuffers();

        // Load shaders and set up uniforms
        buildingProgramID = LoadShadersFromFile("../FinalProject/box.vert", "../FinalProject/box.frag");
        if (buildingProgramID == 0) {
            std::cerr << "Failed to load shaders." << std::endl;
        }

        mvpUniformID = glGetUniformLocation(buildingProgramID, "MVP");
        modelUniformID = glGetUniformLocation(buildingProgramID, "model");
        textureUniformID = glGetUniformLocation(buildingProgramID, "textureSampler");
        textureObjID = LoadTexture(textureFilePath.c_str());

        lightPosID = glGetUniformLocation(buildingProgramID, "lightPos");
        lightColorID = glGetUniformLocation(buildingProgramID, "lightColor");

    }

    void render(glm::mat4 cameraMatrix) {
        glUseProgram(buildingProgramID);

        // Calculate and send the model matrix
        glm::mat4 modelMatrix = glm::mat4(1.0f);
        modelMatrix = glm::translate(modelMatrix, position);
        modelMatrix = glm::scale(modelMatrix, scale);
        glUniformMatrix4fv(modelUniformID, 1, GL_FALSE, &modelMatrix[0][0]);

        // Calculate and send the MVP matrix
        glm::mat4 mvp = cameraMatrix * modelMatrix;
        glUniformMatrix4fv(mvpUniformID, 1, GL_FALSE, &mvp[0][0]);

        glm::vec3 lightPosition = glm::vec3(50.0f, 80.0f, 50.0f);
        glm::vec3 lightColor = glm::vec3(1.0f, 1.0f, 0.8f); // sunset light
        glUniform3fv(lightPosID, 1, &lightPosition[0]);
        glUniform3fv(lightColorID, 1, &lightColor[0]);

        // Enable vertex attributes and bind buffers
        glEnableVertexAttribArray(0); // Position
        glBindBuffer(GL_ARRAY_BUFFER, vboVerticesID);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, 0);

        glEnableVertexAttribArray(1); //Colors
        glBindBuffer(GL_ARRAY_BUFFER, vboColorsID);
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 0, 0);

        glEnableVertexAttribArray(2); //UVs
        glBindBuffer(GL_ARRAY_BUFFER, vboUVsID);
        glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 0, 0);

        glEnableVertexAttribArray(3); // Normals
        glBindBuffer(GL_ARRAY_BUFFER, vboNormalsID);
        glVertexAttribPointer(3, 3, GL_FLOAT, GL_FALSE, 0, 0);

        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, eboIndicesID); 

        // Bind Texture 
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, textureObjID);
        glUniform1i(textureUniformID, 0);

        // Draw the building
        glDrawElements(GL_TRIANGLES, 36, GL_UNSIGNED_INT,(void*)0);

        glDisableVertexAttribArray(0);
        glDisableVertexAttribArray(1);
        glDisableVertexAttribArray(2);
        glDisableVertexAttribArray(3);
    }

    void cleanup() {
        glDeleteBuffers(1, &vboVerticesID);
        glDeleteBuffers(1, &vboColorsID);
        glDeleteBuffers(1, &eboIndicesID);
        glDeleteBuffers(1, &vboNormalsID);
        glDeleteVertexArrays(1, &vaoID);
        glDeleteBuffers(1, &vboUVsID);
        glDeleteTextures(1, &textureObjID);
        glDeleteProgram(buildingProgramID);
    }

}; 

std::vector<Tile> tiles;
std::vector<Building> buildings;
std::vector<std::string> BuildingFacades;
Skybox skybox;

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
                newTile.initialize("../FinalProject/tile4.jpg");
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


/*
void generateBuildings(glm::vec3 position) {
    int centerX = static_cast<int>(std::floor(position.x / cellSize));
    int centerZ = static_cast<int>(std::floor(position.z / cellSize));

    // Define radius for circular patterns and cluster distance
    float clusterRadius = cellSize * 2.0f; // Adjust as needed
    int buildingsPerCluster = 8; // Number of buildings in a circular cluster

    for (int x = centerX - renderDistance; x <= centerX + renderDistance; ++x) {
        for (int z = centerZ - renderDistance; z <= centerZ + renderDistance; ++z) {
            glm::vec3 clusterCenter = glm::vec3(x * cellSize, 0.0f, z * cellSize);

            // Check if this position already has a cluster
            if (rand() % 2 == 0 && std::none_of(buildings.begin(), buildings.end(), [&](const Building& b) {
                return glm::distance(glm::vec2(b.position.x, b.position.z), glm::vec2(clusterCenter.x, clusterCenter.z)) < clusterRadius;
            })) {
                // Generate buildings in a circular cluster
                for (int i = 0; i < buildingsPerCluster; ++i) {
                    float angle = i * (360.0f / buildingsPerCluster); // Angle for each building
                    float radians = glm::radians(angle);
                    glm::vec3 offset = glm::vec3(std::cos(radians), 0.0f, std::sin(radians)) * (clusterRadius / 2.0f);

                    glm::vec3 buildingPosition = clusterCenter + offset;

                    // Ensure no overlapping buildings
                    if (std::none_of(buildings.begin(), buildings.end(), [&](const Building& b) {
                        return glm::distance(glm::vec2(b.position.x, b.position.z), glm::vec2(buildingPosition.x, buildingPosition.z)) < (cellSize / 2.0f);
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
    }
}*/


int main() {
    if (!glfwInit()) {
        std::cerr << "Failed to initialize GLFW." << std::endl;
        return -1;
    }

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    window = glfwCreateWindow(windowWidth, windowHeight, "SkyBox Implementation", NULL, NULL);
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

    skybox.initialize(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(500.0f, 500.0f, 500.0f)); // Adjust scale if needed

    srand(static_cast<unsigned int>(time(nullptr)));
    initializeBuildingFacades();

    // Time and frame rate tracking
	static double lastTime = glfwGetTime();
	float time = 0.0f;			// Animation time 
	float fTime = 0.0f;			// Time for measuring fps
	unsigned long frames = 0;

    while (!glfwWindowShouldClose(window)) {
        // Handle camera movement
        handleCameraMovement(window);

        // Update states for animation
        double currentTime = glfwGetTime();
        float deltaTime = float(currentTime - lastTime);
		lastTime = currentTime;

        // Clear the screen (only once per frame)
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // Render the Skybox
        glDepthMask(GL_FALSE); // Disable depth writing
        glUseProgram(skyboxProgramID);

        glm::mat4 skyboxViewMatrix = glm::mat4(glm::mat3(glm::lookAt(cameraPos, cameraPos + cameraFront, cameraUp)));
        glm::mat4 skyboxProjection = glm::perspective(glm::radians(45.0f), (float)windowWidth / windowHeight, 0.1f, 1000.0f);

        skybox.render(skyboxProjection * skyboxViewMatrix);

        glDepthMask(GL_TRUE); // Re-enable depth writing for other objects
        glUseProgram(0);

        // Generate tiles and buildings dynamically based on the camera position
        generateTiles(cameraPos);
        generateBuildings(cameraPos);

        // Calculate view-projection matrix for tiles and buildings
        glm::mat4 view = glm::lookAt(cameraPos, cameraPos + cameraFront, cameraUp);
        glm::mat4 projection = glm::perspective(glm::radians(45.0f), (float)windowWidth / windowHeight, 0.1f, 100.0f);
        glm::mat4 vp = projection * view;

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
        for (auto& building : buildings) {
            glBindVertexArray(building.vaoID);
            glBindBuffer(GL_ARRAY_BUFFER, building.vboVerticesID);
            glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, building.eboIndicesID);

            // Render the building
            building.render(vp);
        }
        glUseProgram(0); // Unbind the building shader program

        // FPS tracking 
		// Count number of frames over a few seconds and take average
		frames++;
		fTime += deltaTime;
		if (fTime > 2.0f) {		
			float fps = frames / fTime;
			frames = 0;
			fTime = 0;
			
			std::stringstream stream;
			stream << std::fixed << std::setprecision(2) << "Futuristic Emerald Isle | Frames per second (FPS): " << fps;
			glfwSetWindowTitle(window, stream.str().c_str());
		}

        // Swap buffers and poll events
        glfwSwapBuffers(window);
        glfwPollEvents();
    }

        skybox.cleanup(); 

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

    // Update Skybox position to follow the camera
    skybox.position = cameraPos;
}


/*
void handleCameraMovement(GLFWwindow* window) {
    glm::vec3 direction(0.0f);

    // Handle keyboard input for movement
    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
        direction += cameraFront;
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
        direction -= cameraFront;
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
        direction -= glm::normalize(glm::cross(cameraFront, cameraUp));
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
        direction += glm::normalize(glm::cross(cameraFront, cameraUp));

    if (glm::length(direction) > 0.0f) {
        glm::vec3 proposedPosition = cameraPos + glm::normalize(direction) * cameraSpeed;

        // Preserve the camera's fixed y-coordinate
        proposedPosition.y = cameraPos.y;

        // Check for collisions with buildings
        bool collision = false;
        for (const auto& building : buildings) {
            if (building.isPointInside(proposedPosition)) {
                collision = true;
                break;
            }
        }

        // Update camera position only if no collision
        if (!collision) {
            cameraPos = proposedPosition;

            // Update Skybox position to follow the camera
            skybox.position = cameraPos;
        }
    }
}
*/

void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods) {
    if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
        glfwSetWindowShouldClose(window, GL_TRUE);
}
