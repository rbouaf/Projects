#include <vector>
#include <random>
#include <iostream>
#include <fstream>
#include <sstream>
#include <list>

#define GLEW_STATIC 1
#include <GL/glew.h>
#include <GLFW/glfw3.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#define STB_IMAGE_IMPLEMENTATION
#include <stb/stb_image.h>

#include "OBJloader.h"
#include "OBJloaderV2.h"

using namespace glm;
using namespace std;

class Projectile {
public:
    Projectile(vec3 position, vec3 velocity) : mPosition(position), mVelocity(velocity) {}
    
    void Update(float dt) {
        mPosition += mVelocity * dt;
    }
    
    void Draw(GLuint shaderProgram, mat4 view, mat4 projection) {
        glUseProgram(shaderProgram);
        
        mat4 worldMatrix = translate(mat4(1.0f), mPosition) * scale(mat4(1.0f), vec3(0.3f, 0.3f, 0.3f));
        glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "model"), 1, GL_FALSE, &worldMatrix[0][0]);
        glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "view"), 1, GL_FALSE, &view[0][0]);
        glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "projection"), 1, GL_FALSE, &projection[0][0]);
        
        // Set light uniforms (basic lighting)
        glUniform3fv(glGetUniformLocation(shaderProgram, "lightPos"), 1, &vec3(0.0f, 10.0f, 0.0f)[0]);
        glUniform3fv(glGetUniformLocation(shaderProgram, "lightColor"), 1, &vec3(1.0f, 0.8f, 0.2f)[0]);
        glUniform3fv(glGetUniformLocation(shaderProgram, "viewPos"), 1, &vec3(0.0f)[0]);
        
        // Bind a simple orange/red texture color (no texture, just color)
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, 0); // No texture
        glUniform1i(glGetUniformLocation(shaderProgram, "texture_diffuse1"), 0);
        
        // Render as a proper cube
        extern GLuint cubeVAO;
        glBindVertexArray(cubeVAO);
        glDrawArrays(GL_TRIANGLES, 0, 36); // 36 vertices = 6 faces * 2 triangles * 3 vertices
        glBindVertexArray(0);
    }
    
    vec3 getPosition() const { return mPosition; }
    
private:
    vec3 mPosition;
    vec3 mVelocity;
};

const GLuint WIDTH = 1200, HEIGHT = 800;

// Camera variables
vec3 cameraPos = vec3(0.0f, 3.5f, 5.0f);
vec3 cameraFront = vec3(0.0f, 0.0f, -1.0f);
vec3 cameraUp = vec3(0.0f, 1.0f, 0.0f);

float cameraYaw = -90.0f;
float cameraPitch = 0.0f;
float lastX = WIDTH / 2.0f;
float lastY = HEIGHT / 2.0f;
bool firstMouse = true;

// Movement
bool keys[1024];
float deltaTime = 0.0f;
float lastFrame = 0.0f;

// Physics
bool flightMode = false;
vec3 cameraVelocity = vec3(0.0f);
float gravity = -9.8f;
float groundLevel = 2.0f;  // Height above ground plane (raised with camera)

// Shooting mechanics
bool leftMousePressed = false;
float lastShotTime = 0.0f;
float shootCooldown = 0.5f;               // Time between shots
float staffShakeTimer = 0.0f;             // Current shake timer
list<Projectile> projectileList;

// Hit effects
bool snakeHit = false;
float hitFlashTimer = 0.0f;
float hitFlashDuration = 0.3f;            // How long the red flash lasts
// Lighting
vec3 lightPos = vec3(10.0f, 10.0f, 10.0f);
vec3 lightColor = vec3(1.0f, 1.0f, 0.9f);

// Shader programs
GLuint sceneShaderProgram;
GLuint shadowShaderProgram;

// Shadow mapping
GLuint depthMapFBO;
GLuint depthMap;
const unsigned int SHADOW_WIDTH = 2048, SHADOW_HEIGHT = 2048;

// Ground
GLuint groundVAO, groundVBO;
GLuint groundTexture;

// Cube for projectiles
GLuint cubeVAO, cubeVBO;

// Dragon models
struct DragonModel {
    GLuint VAO, VBO;
    vector<vec3> vertices;
    vector<vec3> normals;
    vector<vec2> uvs;
    GLuint texture;
};

DragonModel dragonHead;
DragonModel fishBody;
DragonModel bearPaw;
DragonModel eagleWing;
DragonModel staff;

// Snake neck chain (15 fish segments - longer snake)
const int SNAKE_NECK_SEGMENTS = 15;
struct SnakeNeckSegment {
    vec3 position;
    vec3 rotation;
    float animationPhase;
};

vector<SnakeNeckSegment> snakeNeckSegments(SNAKE_NECK_SEGMENTS);
vec3 snakeBasePos = vec3(0.0f, 0.0f, -15.0f);  // Starts on ground, further away
float snakeAnimationTime = 0.0f;

// ========== SNAKE CREATURE ADJUSTMENT VARIABLES ==========
// EDIT THESE VALUES TO ADJUST SIZE AND POSITION

// Snake Head (dragon head model)
vec3 headScale = vec3(0.1f, 0.1f, 0.1f);         // Size: increase to make bigger
vec3 headRotation = vec3(0.0f, 0.0f, 0.0f);      // Base rotation

// Snake Neck (fish chain)
vec3 neckScale = vec3(0.3f, 0.2f, 0.3f);         // Size of each neck segment (much smaller)
float neckLength = 10.0f;                         // Total length of neck (longer)
float oscillationStrength = 1.5f;                // How much the neck sways
float oscillationSpeed = 2.0f;                   // How fast the neck moves

// Camera following
float headFollowStrength = 0.3f;                 // How much head follows camera (0-1)
float maxHeadDistance = 3.0f;                    // Maximum distance head can be from neck

// FPS Staff
vec3 staffPosition = vec3(0.5f, -0.3f, 0.8f);   // Position relative to camera (right, down, forward)
vec3 staffRotation = vec3(0.0f, 0.0f, 0.0f);     // Base rotation
vec3 staffScale = vec3(0.3f, 0.3f, 0.3f);        // Much smaller scale for FPS weapon
float staffShakeAmount = 0.05f;                   // How much shake when shooting
float staffShakeDuration = 0.2f;                  // How long shake lasts

// ========== END ADJUSTMENT VARIABLES ==========

// Function declarations
void processInput(GLFWwindow *window);
void mouse_callback(GLFWwindow* window, double xpos, double ypos);
void key_callback(GLFWwindow* window, int key, int scancode, int action, int mode);
GLuint loadTexture(const char* path);
GLuint createShaderProgram(const char* vertexPath, const char* fragmentPath);
void setupShadowMapping();
void setupGround();
void renderGround(GLuint shaderProgram, mat4 model, mat4 view, mat4 projection);
void setupCube();
bool loadDragonModel(DragonModel& model, const char* objPath, const char* texturePath);
void setupSnakeModels();
void updateSnakeAnimation(float deltaTime, vec3 cameraPos);
bool checkCollision(vec3 projectilePos, vec3 segmentPos, float radius);
void playHitSound();
void renderSnake(GLuint shaderProgram, mat4 view, mat4 projection, mat4 lightSpaceMatrix);
void renderStaff(GLuint shaderProgram, mat4 view, mat4 projection, mat4 lightSpaceMatrix);

string loadShaderSource(const char* filename) {
    ifstream file(filename);
    if (!file.is_open()) {
        cout << "ERROR: Unable to read shader file " << filename << endl;
        return "";
    }
    
    stringstream ss;
    ss << file.rdbuf();
    return ss.str();
}

GLuint compileShader(GLenum type, const string& source) {
    GLuint shader = glCreateShader(type);
    const char* src = source.c_str();
    glShaderSource(shader, 1, &src, NULL);
    glCompileShader(shader);
    
    GLint success;
    GLchar infoLog[512];
    glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
    if (!success) {
        glGetShaderInfoLog(shader, 512, NULL, infoLog);
        cout << "ERROR: Shader compilation failed\n" << infoLog << endl;
    }
    
    return shader;
}

GLuint createShaderProgram(const char* vertexPath, const char* fragmentPath) {
    string vertexSource = loadShaderSource(vertexPath);
    string fragmentSource = loadShaderSource(fragmentPath);
    
    GLuint vertexShader = compileShader(GL_VERTEX_SHADER, vertexSource);
    GLuint fragmentShader = compileShader(GL_FRAGMENT_SHADER, fragmentSource);
    
    GLuint program = glCreateProgram();
    glAttachShader(program, vertexShader);
    glAttachShader(program, fragmentShader);
    glLinkProgram(program);
    
    GLint success;
    GLchar infoLog[512];
    glGetProgramiv(program, GL_LINK_STATUS, &success);
    if (!success) {
        glGetProgramInfoLog(program, 512, NULL, infoLog);
        cout << "ERROR: Shader program linking failed\n" << infoLog << endl;
    }
    
    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);
    
    return program;
}

GLuint loadTexture(const char* path) {
    GLuint textureID;
    glGenTextures(1, &textureID);
    
    int width, height, nrChannels;
    cout << "Loading texture: " << path << endl;
    unsigned char *data = stbi_load(path, &width, &height, &nrChannels, 0);
    if (data) {
        GLenum format;
        if (nrChannels == 1)
            format = GL_RED;
        else if (nrChannels == 3)
            format = GL_RGB;
        else if (nrChannels == 4)
            format = GL_RGBA;
        
        cout << "Texture loaded successfully: " << width << "x" << height << " channels: " << nrChannels << endl;
        
        glBindTexture(GL_TEXTURE_2D, textureID);
        glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, data);
        glGenerateMipmap(GL_TEXTURE_2D);
        
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        
        stbi_image_free(data);
    } else {
        cout << "FAILED to load texture: " << path << endl;
        cout << "STB Image error: " << stbi_failure_reason() << endl;
        
        // Create a simple fallback texture (bright white)
        glBindTexture(GL_TEXTURE_2D, textureID);
        unsigned char fallbackData[] = {255, 255, 255};
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, 1, 1, 0, GL_RGB, GL_UNSIGNED_BYTE, fallbackData);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        
        stbi_image_free(data);
    }
    
    return textureID;
}

void setupShadowMapping() {
    glGenFramebuffers(1, &depthMapFBO);
    
    glGenTextures(1, &depthMap);
    glBindTexture(GL_TEXTURE_2D, depthMap);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT, SHADOW_WIDTH, SHADOW_HEIGHT, 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
    float borderColor[] = { 1.0f, 1.0f, 1.0f, 1.0f };
    glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, borderColor);
    
    glBindFramebuffer(GL_FRAMEBUFFER, depthMapFBO);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, depthMap, 0);
    glDrawBuffer(GL_NONE);
    glReadBuffer(GL_NONE);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void setupGround() {
    float groundVertices[] = {
        // positions        // normals         // texture coords
        -50.0f, 0.0f, -50.0f,  0.0f, 1.0f, 0.0f,  0.0f, 50.0f,
         50.0f, 0.0f, -50.0f,  0.0f, 1.0f, 0.0f,  50.0f, 50.0f,
         50.0f, 0.0f,  50.0f,  0.0f, 1.0f, 0.0f,  50.0f, 0.0f,
         50.0f, 0.0f,  50.0f,  0.0f, 1.0f, 0.0f,  50.0f, 0.0f,
        -50.0f, 0.0f,  50.0f,  0.0f, 1.0f, 0.0f,  0.0f, 0.0f,
        -50.0f, 0.0f, -50.0f,  0.0f, 1.0f, 0.0f,  0.0f, 50.0f,
    };
    
    glGenVertexArrays(1, &groundVAO);
    glGenBuffers(1, &groundVBO);
    
    glBindVertexArray(groundVAO);
    glBindBuffer(GL_ARRAY_BUFFER, groundVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(groundVertices), groundVertices, GL_STATIC_DRAW);
    
    // Position attribute
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    // Normal attribute
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);
    // Texture coordinate attribute
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(6 * sizeof(float)));
    glEnableVertexAttribArray(2);
    
    glBindVertexArray(0);
    
    groundTexture = loadTexture("Textures/grass.jpg");
}

void setupCube() {
    float cubeVertices[] = {
        // Front face
        -0.5f, -0.5f,  0.5f,  0.0f, 0.0f, 1.0f,  0.0f, 0.0f,
         0.5f, -0.5f,  0.5f,  0.0f, 0.0f, 1.0f,  1.0f, 0.0f,
         0.5f,  0.5f,  0.5f,  0.0f, 0.0f, 1.0f,  1.0f, 1.0f,
         0.5f,  0.5f,  0.5f,  0.0f, 0.0f, 1.0f,  1.0f, 1.0f,
        -0.5f,  0.5f,  0.5f,  0.0f, 0.0f, 1.0f,  0.0f, 1.0f,
        -0.5f, -0.5f,  0.5f,  0.0f, 0.0f, 1.0f,  0.0f, 0.0f,
        
        // Back face
        -0.5f, -0.5f, -0.5f,  0.0f, 0.0f, -1.0f, 0.0f, 0.0f,
         0.5f, -0.5f, -0.5f,  0.0f, 0.0f, -1.0f, 1.0f, 0.0f,
         0.5f,  0.5f, -0.5f,  0.0f, 0.0f, -1.0f, 1.0f, 1.0f,
         0.5f,  0.5f, -0.5f,  0.0f, 0.0f, -1.0f, 1.0f, 1.0f,
        -0.5f,  0.5f, -0.5f,  0.0f, 0.0f, -1.0f, 0.0f, 1.0f,
        -0.5f, -0.5f, -0.5f,  0.0f, 0.0f, -1.0f, 0.0f, 0.0f,
        
        // Left face
        -0.5f,  0.5f,  0.5f,  -1.0f, 0.0f, 0.0f, 1.0f, 0.0f,
        -0.5f,  0.5f, -0.5f,  -1.0f, 0.0f, 0.0f, 1.0f, 1.0f,
        -0.5f, -0.5f, -0.5f,  -1.0f, 0.0f, 0.0f, 0.0f, 1.0f,
        -0.5f, -0.5f, -0.5f,  -1.0f, 0.0f, 0.0f, 0.0f, 1.0f,
        -0.5f, -0.5f,  0.5f,  -1.0f, 0.0f, 0.0f, 0.0f, 0.0f,
        -0.5f,  0.5f,  0.5f,  -1.0f, 0.0f, 0.0f, 1.0f, 0.0f,
        
        // Right face
         0.5f,  0.5f,  0.5f,  1.0f, 0.0f, 0.0f,  1.0f, 0.0f,
         0.5f,  0.5f, -0.5f,  1.0f, 0.0f, 0.0f,  1.0f, 1.0f,
         0.5f, -0.5f, -0.5f,  1.0f, 0.0f, 0.0f,  0.0f, 1.0f,
         0.5f, -0.5f, -0.5f,  1.0f, 0.0f, 0.0f,  0.0f, 1.0f,
         0.5f, -0.5f,  0.5f,  1.0f, 0.0f, 0.0f,  0.0f, 0.0f,
         0.5f,  0.5f,  0.5f,  1.0f, 0.0f, 0.0f,  1.0f, 0.0f,
         
        // Bottom face
        -0.5f, -0.5f, -0.5f,  0.0f, -1.0f, 0.0f, 0.0f, 1.0f,
         0.5f, -0.5f, -0.5f,  0.0f, -1.0f, 0.0f, 1.0f, 1.0f,
         0.5f, -0.5f,  0.5f,  0.0f, -1.0f, 0.0f, 1.0f, 0.0f,
         0.5f, -0.5f,  0.5f,  0.0f, -1.0f, 0.0f, 1.0f, 0.0f,
        -0.5f, -0.5f,  0.5f,  0.0f, -1.0f, 0.0f, 0.0f, 0.0f,
        -0.5f, -0.5f, -0.5f,  0.0f, -1.0f, 0.0f, 0.0f, 1.0f,
        
        // Top face
        -0.5f,  0.5f, -0.5f,  0.0f, 1.0f, 0.0f,  0.0f, 1.0f,
         0.5f,  0.5f, -0.5f,  0.0f, 1.0f, 0.0f,  1.0f, 1.0f,
         0.5f,  0.5f,  0.5f,  0.0f, 1.0f, 0.0f,  1.0f, 0.0f,
         0.5f,  0.5f,  0.5f,  0.0f, 1.0f, 0.0f,  1.0f, 0.0f,
        -0.5f,  0.5f,  0.5f,  0.0f, 1.0f, 0.0f,  0.0f, 0.0f,
        -0.5f,  0.5f, -0.5f,  0.0f, 1.0f, 0.0f,  0.0f, 1.0f
    };
    
    glGenVertexArrays(1, &cubeVAO);
    glGenBuffers(1, &cubeVBO);
    
    glBindVertexArray(cubeVAO);
    glBindBuffer(GL_ARRAY_BUFFER, cubeVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(cubeVertices), cubeVertices, GL_STATIC_DRAW);
    
    // Position attribute
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    
    // Normal attribute
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);
    
    // Texture coordinate attribute
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(6 * sizeof(float)));
    glEnableVertexAttribArray(2);
    
    glBindVertexArray(0);
}

void renderGround(GLuint shaderProgram, mat4 model, mat4 view, mat4 projection) {
    glUseProgram(shaderProgram);
    
    glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "model"), 1, GL_FALSE, &model[0][0]);
    glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "view"), 1, GL_FALSE, &view[0][0]);
    glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "projection"), 1, GL_FALSE, &projection[0][0]);
    
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, groundTexture);
    glUniform1i(glGetUniformLocation(shaderProgram, "texture_diffuse1"), 0);
    
    glBindVertexArray(groundVAO);
    glDrawArrays(GL_TRIANGLES, 0, 6);
    glBindVertexArray(0);
}

void processInput(GLFWwindow *window) {
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);
    
    // Toggle flight mode with F key
    static bool fKeyPressed = false;
    if (glfwGetKey(window, GLFW_KEY_F) == GLFW_PRESS && !fKeyPressed) {
        flightMode = !flightMode;
        if (flightMode) {
            cout << "Flight mode ON" << endl;
            cameraVelocity.y = 0.0f; // Stop falling when entering flight mode
        } else {
            cout << "Flight mode OFF - gravity enabled" << endl;
        }
        fKeyPressed = true;
    }
    if (glfwGetKey(window, GLFW_KEY_F) == GLFW_RELEASE) {
        fKeyPressed = false;
    }
    
    float cameraSpeed = 5.0f * deltaTime;
    vec3 forward = cameraFront;
    vec3 right = normalize(cross(cameraFront, cameraUp));
    
    // Movement input
    vec3 inputVelocity = vec3(0.0f);
    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
        inputVelocity += forward * cameraSpeed;
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
        inputVelocity -= forward * cameraSpeed;
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
        inputVelocity -= right * cameraSpeed;
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
        inputVelocity += right * cameraSpeed;
    
    if (flightMode) {
        // In flight mode: WASD moves in all directions, including Y
        if (glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS)
            inputVelocity += cameraUp * cameraSpeed;
        if (glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS)
            inputVelocity -= cameraUp * cameraSpeed;
        
        // Direct position update (no physics)
        cameraPos += inputVelocity;
    } else {
        // Ground mode: apply gravity and collision
        // Apply gravity
        cameraVelocity.y += gravity * deltaTime;
        
        // Horizontal movement (ignore Y component of forward vector)
        vec3 groundForward = normalize(vec3(forward.x, 0.0f, forward.z));
        vec3 groundRight = normalize(vec3(right.x, 0.0f, right.z));
        
        vec3 horizontalInput = vec3(0.0f);
        if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
            horizontalInput += groundForward * cameraSpeed;
        if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
            horizontalInput -= groundForward * cameraSpeed;
        if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
            horizontalInput -= groundRight * cameraSpeed;
        if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
            horizontalInput += groundRight * cameraSpeed;
        
        // Jump
        if (glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS && cameraPos.y <= groundLevel + 0.1f) {
            cameraVelocity.y = 8.0f; // Jump velocity
        }
        
        // Apply velocity
        cameraPos += horizontalInput;
        cameraPos.y += cameraVelocity.y * deltaTime;
        
        // Ground collision
        if (cameraPos.y < groundLevel) {
            cameraPos.y = groundLevel;
            cameraVelocity.y = 0.0f;
        }
    }
    
    // Shooting mechanics
    bool currentLeftMouse = glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS;
    if (currentLeftMouse && !leftMousePressed) {
        // Just pressed left mouse
        cout << "Left mouse clicked!" << endl;
        float currentTime = glfwGetTime();
        if (currentTime - lastShotTime >= shootCooldown) {
            // Fire!
            cout << "FIREBALL SHOT! Time: " << currentTime << endl;
            
            // Create projectile
            vec3 projectileStart = cameraPos + cameraFront * 1.0f; // Start in front of camera
            vec3 projectileVelocity = cameraFront * 20.0f; // High speed forward
            projectileList.push_back(Projectile(projectileStart, projectileVelocity));
            
            lastShotTime = currentTime;
            staffShakeTimer = staffShakeDuration; // Start shake animation
        } else {
            cout << "Shot on cooldown. Time until next shot: " << (shootCooldown - (currentTime - lastShotTime)) << endl;
        }
    }
    leftMousePressed = currentLeftMouse;
    
    // Update staff shake
    if (staffShakeTimer > 0.0f) {
        staffShakeTimer -= deltaTime;
        if (staffShakeTimer < 0.0f) {
            staffShakeTimer = 0.0f;
        }
    }
    
    // Update projectiles
    for (auto it = projectileList.begin(); it != projectileList.end();) {
        it->Update(deltaTime);
        vec3 projPos = it->getPosition();
        
        // Check collision with snake segments
        bool hitSnake = false;
        for (int i = 0; i < SNAKE_NECK_SEGMENTS; i++) {
            if (checkCollision(projPos, snakeNeckSegments[i].position, 1.0f)) {
                hitSnake = true;
                break;
            }
        }
        
        // Check collision with dragon head
        if (!hitSnake && SNAKE_NECK_SEGMENTS > 0) {
            vec3 headPos = snakeNeckSegments[SNAKE_NECK_SEGMENTS-1].position;
            if (checkCollision(projPos, headPos, 1.5f)) {
                hitSnake = true;
            }
        }
        
        if (hitSnake) {
            // Hit detected!
            snakeHit = true;
            hitFlashTimer = hitFlashDuration;
            playHitSound();
            it = projectileList.erase(it); // Remove projectile
        }
        // Remove projectiles that are too far away
        else if (length(projPos - cameraPos) > 50.0f) {
            it = projectileList.erase(it);
        } else {
            ++it;
        }
    }
    
    // Update hit flash timer
    if (hitFlashTimer > 0.0f) {
        hitFlashTimer -= deltaTime;
        if (hitFlashTimer <= 0.0f) {
            hitFlashTimer = 0.0f;
            snakeHit = false;
        }
    }
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
    
    cameraYaw += xoffset;
    cameraPitch += yoffset;
    
    if (cameraPitch > 89.0f)
        cameraPitch = 89.0f;
    if (cameraPitch < -89.0f)
        cameraPitch = -89.0f;
    
    vec3 front;
    front.x = cos(radians(cameraYaw)) * cos(radians(cameraPitch));
    front.y = sin(radians(cameraPitch));
    front.z = sin(radians(cameraYaw)) * cos(radians(cameraPitch));
    cameraFront = normalize(front);
}

void key_callback(GLFWwindow* window, int key, int scancode, int action, int mode) {
    if (key >= 0 && key < 1024) {
        if (action == GLFW_PRESS)
            keys[key] = true;
        else if (action == GLFW_RELEASE)
            keys[key] = false;
    }
}

bool loadDragonModel(DragonModel& model, const char* objPath, const char* texturePath) {
    cout << "Loading OBJ model: " << objPath << endl;
    
    // Clear any existing data
    model.vertices.clear();
    model.normals.clear();
    model.uvs.clear();
    
    if (!loadOBJ(objPath, model.vertices, model.normals, model.uvs)) {
        cout << "FAILED to load OBJ: " << objPath << endl;
        return false;
    }
    cout << "OBJ loaded successfully: " << model.vertices.size() << " vertices, " 
         << model.normals.size() << " normals, " << model.uvs.size() << " UVs" << endl;
    
    
    // Check for mismatched array sizes
    if (model.vertices.size() != model.normals.size()) {
        cout << "WARNING: Vertex count doesn't match normal count!" << endl;
    }
    if (model.vertices.size() != model.uvs.size()) {
        cout << "WARNING: Vertex count doesn't match UV count!" << endl;
    }
    
    // The OBJ loader already gives us triangulated data, so we can use it directly
    glGenVertexArrays(1, &model.VAO);
    
    // Create separate VBOs for vertices, normals, and UVs
    GLuint vertexVBO, normalVBO, uvVBO;
    glGenBuffers(1, &vertexVBO);
    glGenBuffers(1, &normalVBO);
    glGenBuffers(1, &uvVBO);
    
    glBindVertexArray(model.VAO);
    
    // Vertex positions
    glBindBuffer(GL_ARRAY_BUFFER, vertexVBO);
    glBufferData(GL_ARRAY_BUFFER, model.vertices.size() * sizeof(vec3), &model.vertices[0], GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, (void*)0);
    glEnableVertexAttribArray(0);
    
    // Normals
    if (!model.normals.empty() && model.normals.size() == model.vertices.size()) {
        glBindBuffer(GL_ARRAY_BUFFER, normalVBO);
        glBufferData(GL_ARRAY_BUFFER, model.normals.size() * sizeof(vec3), &model.normals[0], GL_STATIC_DRAW);
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 0, (void*)0);
        glEnableVertexAttribArray(1);
    } else {
        cout << "Using default normals (all pointing up)" << endl;
        // Create default normals pointing up
        vector<vec3> defaultNormals(model.vertices.size(), vec3(0.0f, 1.0f, 0.0f));
        glBindBuffer(GL_ARRAY_BUFFER, normalVBO);
        glBufferData(GL_ARRAY_BUFFER, defaultNormals.size() * sizeof(vec3), &defaultNormals[0], GL_STATIC_DRAW);
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 0, (void*)0);
        glEnableVertexAttribArray(1);
    }
    
    // UVs
    if (!model.uvs.empty() && model.uvs.size() == model.vertices.size()) {
        glBindBuffer(GL_ARRAY_BUFFER, uvVBO);
        glBufferData(GL_ARRAY_BUFFER, model.uvs.size() * sizeof(vec2), &model.uvs[0], GL_STATIC_DRAW);
        glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 0, (void*)0);
        glEnableVertexAttribArray(2);
    } else {
        cout << "Using default UVs (all 0,0)" << endl;
        // Create default UVs
        vector<vec2> defaultUVs(model.vertices.size(), vec2(0.0f, 0.0f));
        glBindBuffer(GL_ARRAY_BUFFER, uvVBO);
        glBufferData(GL_ARRAY_BUFFER, defaultUVs.size() * sizeof(vec2), &defaultUVs[0], GL_STATIC_DRAW);
        glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 0, (void*)0);
        glEnableVertexAttribArray(2);
    }
    
    glBindVertexArray(0);
    
    // Store the main VBO for cleanup (just use the vertex VBO)
    model.VBO = vertexVBO;
    
    if (texturePath) {
        model.texture = loadTexture(texturePath);
    } else {
        // Create default white texture if no path provided
        glGenTextures(1, &model.texture);
        glBindTexture(GL_TEXTURE_2D, model.texture);
        unsigned char whitePixel[] = {255, 255, 255};
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, 1, 1, 0, GL_RGB, GL_UNSIGNED_BYTE, whitePixel);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    }
    cout << "Texture ID: " << model.texture << endl;
    
    return true;
}

void setupSnakeModels() {
    cout << "Setting up snake creature models..." << endl;
    
    cout << "Loading dragon head for snake..." << endl;
    if (!loadDragonModel(dragonHead, "Models/dragon_head.obj", "Textures/dragon_texture.jpg")) {
        cout << "ERROR: Failed to load dragon head!" << endl;
    }
    
    cout << "Loading fish for snake neck..." << endl;
    if (!loadDragonModel(fishBody, "Models/fish.obj", "Textures/fish_texture.jpg")) {
        cout << "ERROR: Failed to load fish!" << endl;
    }
    
    cout << "Loading staff..." << endl;
    if (!loadDragonModel(staff, "Models/staff.obj", "Textures/brick.jpg")) {
        cout << "ERROR: Failed to load staff!" << endl;
    } else {
        cout << "Staff loaded successfully: " << staff.vertices.size() << " vertices" << endl;
    }
    
    cout << "Snake models setup complete!" << endl;
    
    // Initialize snake neck segments
    for (int i = 0; i < SNAKE_NECK_SEGMENTS; i++) {
        snakeNeckSegments[i].animationPhase = (float)i / SNAKE_NECK_SEGMENTS * 2.0f * 3.14159f;
        snakeNeckSegments[i].position = vec3(0.0f);
        snakeNeckSegments[i].rotation = vec3(0.0f);
    }
}

void updateSnakeAnimation(float deltaTime, vec3 cameraPos) {
    snakeAnimationTime += deltaTime;
    
    // Snake slithers towards camera
    vec3 directionToCamera = normalize(cameraPos - snakeBasePos);
    float slitherSpeed = 0.8f; // Units per second (slower)
    
    // Move snake base position towards camera (but keep it on ground)
    vec3 cameraGroundPos = vec3(cameraPos.x, 0.0f, cameraPos.z); // Project camera to ground
    vec3 directionToGroundCamera = normalize(cameraGroundPos - snakeBasePos);
    snakeBasePos += directionToGroundCamera * slitherSpeed * deltaTime;
    snakeBasePos.y = 0.0f; // Keep snake base on the ground
    
    // Keep snake at a reasonable distance (don't get too close)
    float distanceToCamera = length(cameraGroundPos - snakeBasePos);
    if (distanceToCamera < 8.0f) {
        // Keep minimum distance (increased from 5 to 8)
        vec3 pushBack = normalize(snakeBasePos - cameraGroundPos) * (8.0f - distanceToCamera);
        snakeBasePos += pushBack;
        snakeBasePos.y = 0.0f; // Ensure it stays on ground
    }
    
    // Calculate snake neck segments following the base
    float segmentSpacing = neckLength / SNAKE_NECK_SEGMENTS;
    
    for (int i = 0; i < SNAKE_NECK_SEGMENTS; i++) {
        float progress = (float)i / SNAKE_NECK_SEGMENTS;  // 0.0 at base, 1.0 at head
        
        // Height varies along the snake body (slithering motion)
        // Back half affected by gravity, front half rises up
        float baseGroundHeight = 0.1f; // Very close to ground
        float maxRiseHeight = 1.8f;    // How high the front can rise
        
        // Only the front 60% of the snake can rise significantly
        float riseStartPoint = 0.4f; // Start rising at 40% progress
        float heightProgress = 0.0f;
        if (progress > riseStartPoint) {
            // Smooth rise only for front segments
            float adjustedProgress = (progress - riseStartPoint) / (1.0f - riseStartPoint);
            heightProgress = adjustedProgress * adjustedProgress; // Quadratic
        }
        
        // Back segments have minimal wave, front segments have more dramatic motion
        float waveIntensity = progress < 0.5f ? 0.05f : 0.15f; // Less wave for back half
        float slitherWave = sin(snakeAnimationTime * 2.0f + progress * 6.28f) * waveIntensity;
        
        // Apply "gravity" effect to back segments
        float gravityEffect = 1.0f - (progress * 0.3f); // Back segments pulled down more
        
        float height = (baseGroundHeight * gravityEffect) + (heightProgress * maxRiseHeight) + slitherWave;
        
        // Forward direction with slithering side motion (use ground-projected direction)
        vec3 forwardOffset = directionToGroundCamera * (progress * neckLength);
        
        // Oscillating side motion for slithering effect
        vec3 rightVector = normalize(cross(directionToGroundCamera, vec3(0.0f, 1.0f, 0.0f)));
        float sideMotion = sin(snakeAnimationTime * oscillationSpeed + progress * 3.14159f) * oscillationStrength;
        vec3 sideOffset = rightVector * sideMotion;
        
        snakeNeckSegments[i].position = vec3(
            snakeBasePos.x + forwardOffset.x + sideOffset.x,
            height,
            snakeBasePos.z + forwardOffset.z + sideOffset.z
        );
        
        // Rotation to follow the movement direction
        if (i > 0) {
            vec3 direction = snakeNeckSegments[i].position - snakeNeckSegments[i-1].position;
            if (length(direction) > 0.001f) {
                direction = normalize(direction);
                snakeNeckSegments[i].rotation.y = atan2(direction.x, direction.z);
                snakeNeckSegments[i].rotation.x = atan2(direction.y, sqrt(direction.x*direction.x + direction.z*direction.z));
            }
        }
    }
}

bool checkCollision(vec3 projectilePos, vec3 segmentPos, float radius) {
    return length(projectilePos - segmentPos) < radius;
}

void playHitSound() {
    // Simple sound playback using Windows API
    #ifdef _WIN32
    // You can use PlaySoundA for simple sound playback
    // PlaySoundA("pain.ogg", NULL, SND_FILENAME | SND_ASYNC);
    cout << "SNAKE HIT! *pain sound*" << endl; // For now, just console output
    #else
    cout << "SNAKE HIT! *pain sound*" << endl;
    #endif
}

void renderTestCube(GLuint shaderProgram, mat4 modelMatrix, mat4 view, mat4 projection, mat4 lightSpaceMatrix) {
    // Simple test cube with known good geometry
    static GLuint testVAO = 0;
    if (testVAO == 0) {
        float vertices[] = {
            // Front face
            -1.0f, -1.0f,  1.0f,  0.0f, 0.0f, 1.0f,  0.0f, 0.0f,
             1.0f, -1.0f,  1.0f,  0.0f, 0.0f, 1.0f,  1.0f, 0.0f,
             1.0f,  1.0f,  1.0f,  0.0f, 0.0f, 1.0f,  1.0f, 1.0f,
             1.0f,  1.0f,  1.0f,  0.0f, 0.0f, 1.0f,  1.0f, 1.0f,
            -1.0f,  1.0f,  1.0f,  0.0f, 0.0f, 1.0f,  0.0f, 1.0f,
            -1.0f, -1.0f,  1.0f,  0.0f, 0.0f, 1.0f,  0.0f, 0.0f,
            
            // Back face
            -1.0f, -1.0f, -1.0f,  0.0f, 0.0f, -1.0f,  1.0f, 0.0f,
            -1.0f,  1.0f, -1.0f,  0.0f, 0.0f, -1.0f,  1.0f, 1.0f,
             1.0f,  1.0f, -1.0f,  0.0f, 0.0f, -1.0f,  0.0f, 1.0f,
             1.0f,  1.0f, -1.0f,  0.0f, 0.0f, -1.0f,  0.0f, 1.0f,
             1.0f, -1.0f, -1.0f,  0.0f, 0.0f, -1.0f,  0.0f, 0.0f,
            -1.0f, -1.0f, -1.0f,  0.0f, 0.0f, -1.0f,  1.0f, 0.0f
        };
        
        GLuint testVBO;
        glGenVertexArrays(1, &testVAO);
        glGenBuffers(1, &testVBO);
        
        glBindVertexArray(testVAO);
        glBindBuffer(GL_ARRAY_BUFFER, testVBO);
        glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
        
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)0);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(3 * sizeof(float)));
        glEnableVertexAttribArray(1);
        glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(6 * sizeof(float)));
        glEnableVertexAttribArray(2);
        
        glBindVertexArray(0);
    }
    
    glUseProgram(shaderProgram);
    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
    glDisable(GL_CULL_FACE);
    
    glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "model"), 1, GL_FALSE, &modelMatrix[0][0]);
    glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "view"), 1, GL_FALSE, &view[0][0]);
    glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "projection"), 1, GL_FALSE, &projection[0][0]);
    glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "lightSpaceMatrix"), 1, GL_FALSE, &lightSpaceMatrix[0][0]);
    
    glBindVertexArray(testVAO);
    glDrawArrays(GL_TRIANGLES, 0, 12);  // 12 vertices = 4 triangles (2 faces)
    glBindVertexArray(0);
}

void renderDragonModel(DragonModel& model, GLuint shaderProgram, mat4 modelMatrix, mat4 view, mat4 projection, mat4 lightSpaceMatrix) {
    // Debug output removed to reduce spam
    
    // Use fixed tutorial-style rendering
    glUseProgram(shaderProgram);
    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
    glDisable(GL_CULL_FACE);
    
    
    glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "model"), 1, GL_FALSE, &modelMatrix[0][0]);
    glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "view"), 1, GL_FALSE, &view[0][0]);
    glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "projection"), 1, GL_FALSE, &projection[0][0]);
    glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "lightSpaceMatrix"), 1, GL_FALSE, &lightSpaceMatrix[0][0]);
    
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, model.texture);
    glUniform1i(glGetUniformLocation(shaderProgram, "texture_diffuse1"), 0);
    
    if (model.VAO == 0) {
        cout << "ERROR: Model VAO is 0!" << endl;
        return;
    }
    
    glBindVertexArray(model.VAO);
    // Use glDrawArrays with correct vertex count
    glDrawArrays(GL_TRIANGLES, 0, model.vertices.size());
    glBindVertexArray(0);
    return;
    
    // Original broken code below
    glUseProgram(shaderProgram);
    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
    glDisable(GL_CULL_FACE);
    
    glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "model"), 1, GL_FALSE, &modelMatrix[0][0]);
    glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "view"), 1, GL_FALSE, &view[0][0]);
    glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "projection"), 1, GL_FALSE, &projection[0][0]);
    glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "lightSpaceMatrix"), 1, GL_FALSE, &lightSpaceMatrix[0][0]);
    
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, model.texture);
    glUniform1i(glGetUniformLocation(shaderProgram, "texture_diffuse1"), 0);
    
    glBindVertexArray(model.VAO);
    glDrawArrays(GL_TRIANGLES, 0, model.vertices.size());
    glBindVertexArray(0);
}

void renderStaff(GLuint shaderProgram, mat4 view, mat4 projection, mat4 lightSpaceMatrix) {
    // Debug: Check if staff has vertices
    if (staff.vertices.empty()) {
        cout << "Staff has no vertices!" << endl;
        return;
    }
    
    // Calculate staff position relative to camera (FPS weapon style)
    vec3 rightVector = normalize(cross(cameraFront, cameraUp));
    vec3 staffWorldPos = cameraPos + 
                        cameraFront * 3.0f +        // Further away from camera
                        rightVector * 1.2f +        // More to the right
                        cameraUp * (-0.8f);         // Further down
                        
    // Debug position output removed
    
    // Add shake effect when shooting
    vec3 shakeOffset = vec3(0.0f);
    if (staffShakeTimer > 0.0f) {
        float shakeIntensity = staffShakeTimer / staffShakeDuration;  // 1.0 to 0.0
        shakeOffset.x = (sin(staffShakeTimer * 50.0f) * staffShakeAmount * shakeIntensity);
        shakeOffset.y = (cos(staffShakeTimer * 60.0f) * staffShakeAmount * shakeIntensity);
    }
    
    mat4 staffModel = mat4(1.0f);
    staffModel = translate(staffModel, staffWorldPos + shakeOffset);
    
    // Rotate staff to match camera orientation
    float cameraYawRad = radians(cameraYaw);
    float cameraPitchRad = radians(cameraPitch);
    staffModel = rotate(staffModel, cameraYawRad, vec3(0.0f, 1.0f, 0.0f));
    staffModel = rotate(staffModel, cameraPitchRad, vec3(1.0f, 0.0f, 0.0f));
    
    // Apply base rotation and scale
    staffModel = rotate(staffModel, staffRotation.x, vec3(1.0f, 0.0f, 0.0f));
    staffModel = rotate(staffModel, staffRotation.y, vec3(0.0f, 1.0f, 0.0f));
    staffModel = rotate(staffModel, staffRotation.z, vec3(0.0f, 0.0f, 1.0f));
    staffModel = scale(staffModel, staffScale);
    
    renderDragonModel(staff, shaderProgram, staffModel, view, projection, lightSpaceMatrix);
}

void renderSnake(GLuint shaderProgram, mat4 view, mat4 projection, mat4 lightSpaceMatrix) {
    // Render snake neck segments (fish chain)
    for (int i = 0; i < SNAKE_NECK_SEGMENTS; i++) {
        mat4 neckModel = mat4(1.0f);
        neckModel = translate(neckModel, snakeNeckSegments[i].position);
        neckModel = rotate(neckModel, snakeNeckSegments[i].rotation.x, vec3(1.0f, 0.0f, 0.0f));
        neckModel = rotate(neckModel, snakeNeckSegments[i].rotation.y, vec3(0.0f, 1.0f, 0.0f));
        neckModel = rotate(neckModel, snakeNeckSegments[i].rotation.z, vec3(0.0f, 0.0f, 1.0f));
        neckModel = scale(neckModel, neckScale);
        // Set hit flash uniforms before rendering this segment
        if (snakeHit) {
            glUniform3f(glGetUniformLocation(shaderProgram, "hitFlashColor"), 1.0f, 0.0f, 0.0f);
            glUniform1f(glGetUniformLocation(shaderProgram, "hitFlashStrength"), hitFlashTimer / hitFlashDuration);
        } else {
            glUniform1f(glGetUniformLocation(shaderProgram, "hitFlashStrength"), 0.0f);
        }
        renderDragonModel(fishBody, shaderProgram, neckModel, view, projection, lightSpaceMatrix);
    }
    
    // Render snake head at the end of neck (faces camera)
    if (SNAKE_NECK_SEGMENTS > 0) {
        vec3 headPos = snakeNeckSegments[SNAKE_NECK_SEGMENTS-1].position;
        
        // Calculate direction from head to camera
        vec3 headToCam = normalize(cameraPos - headPos);
        
        // Calculate yaw (rotation around Y axis) to face camera
        float headYaw = atan2(headToCam.x, headToCam.z);
        
        // Calculate pitch (rotation around X axis) to look up/down at camera
        float horizontalDistance = sqrt(headToCam.x * headToCam.x + headToCam.z * headToCam.z);
        float headPitch = atan2(headToCam.y, horizontalDistance);
        
        mat4 headModel = mat4(1.0f);
        headModel = translate(headModel, headPos);
        headModel = rotate(headModel, headYaw, vec3(0.0f, 1.0f, 0.0f));      // Yaw (left/right)
        headModel = rotate(headModel, -headPitch, vec3(1.0f, 0.0f, 0.0f));   // Pitch (up/down)
        headModel = scale(headModel, headScale);
        // Set hit flash uniforms before rendering dragon head
        if (snakeHit) {
            glUniform3f(glGetUniformLocation(shaderProgram, "hitFlashColor"), 1.0f, 0.0f, 0.0f);
            glUniform1f(glGetUniformLocation(shaderProgram, "hitFlashStrength"), hitFlashTimer / hitFlashDuration);
        } else {
            glUniform1f(glGetUniformLocation(shaderProgram, "hitFlashStrength"), 0.0f);
        }
        renderDragonModel(dragonHead, shaderProgram, headModel, view, projection, lightSpaceMatrix);
    }
}

int main() {
    if (!glfwInit()) {
        cout << "Failed to initialize GLFW" << endl;
        return -1;
    }
    
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    
    GLFWwindow* window = glfwCreateWindow(WIDTH, HEIGHT, "Wizard Shooter", NULL, NULL);
    if (!window) {
        cout << "Failed to create GLFW window" << endl;
        glfwTerminate();
        return -1;
    }
    
    glfwMakeContextCurrent(window);
    glfwSetCursorPosCallback(window, mouse_callback);
    glfwSetKeyCallback(window, key_callback);
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
    
    glewExperimental = GL_TRUE;
    if (glewInit() != GLEW_OK) {
        cout << "Failed to initialize GLEW" << endl;
        return -1;
    }
    
    glEnable(GL_DEPTH_TEST);
    // Re-enable face culling with correct settings
    glEnable(GL_CULL_FACE);
    glCullFace(GL_BACK);
    glFrontFace(GL_CCW);
    
    // Make sure we're rendering filled polygons, not wireframes
    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
    
    sceneShaderProgram = createShaderProgram("Shaders/scene_vertex_textured.glsl", "Shaders/scene_fragment_textured.glsl");
    shadowShaderProgram = createShaderProgram("Shaders/shadow_vertex.glsl", "Shaders/shadow_fragment.glsl");
    
    setupShadowMapping();
    setupGround();
    setupCube();
    setupSnakeModels();
    
    glClearColor(0.5f, 0.7f, 1.0f, 1.0f);
    
    while (!glfwWindowShouldClose(window)) {
        float currentFrame = glfwGetTime();
        deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;
        
        processInput(window);
        updateSnakeAnimation(deltaTime, cameraPos);
        
        // Update light position (sun moving across sky)
        lightPos.x = 15.0f * cos(currentFrame * 0.5f);
        lightPos.z = 15.0f * sin(currentFrame * 0.5f);
        
        // Shadow mapping pass
        glViewport(0, 0, SHADOW_WIDTH, SHADOW_HEIGHT);
        glBindFramebuffer(GL_FRAMEBUFFER, depthMapFBO);
        glClear(GL_DEPTH_BUFFER_BIT);
        
        mat4 lightProjection = ortho(-20.0f, 20.0f, -20.0f, 20.0f, 1.0f, 50.0f);
        mat4 lightView = lookAt(lightPos, vec3(0.0f), vec3(0.0f, 1.0f, 0.0f));
        mat4 lightSpaceMatrix = lightProjection * lightView;
        
        glUseProgram(shadowShaderProgram);
        glUniformMatrix4fv(glGetUniformLocation(shadowShaderProgram, "lightSpaceMatrix"), 1, GL_FALSE, &lightSpaceMatrix[0][0]);
        
        mat4 groundModel = mat4(1.0f);
        renderGround(shadowShaderProgram, groundModel, lightView, lightProjection);
        
        // Render snake in shadow pass (simplified)
        for (int i = 0; i < SNAKE_NECK_SEGMENTS; i++) {
            mat4 bodyModel = mat4(1.0f);
            bodyModel = translate(bodyModel, snakeNeckSegments[i].position);
            bodyModel = scale(bodyModel, neckScale);
            glUniformMatrix4fv(glGetUniformLocation(shadowShaderProgram, "model"), 1, GL_FALSE, &bodyModel[0][0]);
            glBindVertexArray(fishBody.VAO);
            glDrawArrays(GL_TRIANGLES, 0, fishBody.vertices.size());
        }
        
        // Render snake head in shadow pass
        if (SNAKE_NECK_SEGMENTS > 0) {
            mat4 headModel = mat4(1.0f);
            headModel = translate(headModel, snakeNeckSegments[SNAKE_NECK_SEGMENTS-1].position);
            headModel = scale(headModel, headScale);
            glUniformMatrix4fv(glGetUniformLocation(shadowShaderProgram, "model"), 1, GL_FALSE, &headModel[0][0]);
            glBindVertexArray(dragonHead.VAO);
            glDrawArrays(GL_TRIANGLES, 0, dragonHead.vertices.size());
        }
        
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        
        // Scene rendering pass
        glViewport(0, 0, WIDTH, HEIGHT);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        
        mat4 projection = perspective(radians(45.0f), (float)WIDTH / (float)HEIGHT, 0.1f, 100.0f);
        mat4 view = lookAt(cameraPos, cameraPos + cameraFront, cameraUp);
        
        glUseProgram(sceneShaderProgram);
        glUniformMatrix4fv(glGetUniformLocation(sceneShaderProgram, "projection"), 1, GL_FALSE, &projection[0][0]);
        glUniformMatrix4fv(glGetUniformLocation(sceneShaderProgram, "view"), 1, GL_FALSE, &view[0][0]);
        glUniformMatrix4fv(glGetUniformLocation(sceneShaderProgram, "lightSpaceMatrix"), 1, GL_FALSE, &lightSpaceMatrix[0][0]);
        
        glUniform3fv(glGetUniformLocation(sceneShaderProgram, "lightPos"), 1, &lightPos[0]);
        glUniform3fv(glGetUniformLocation(sceneShaderProgram, "lightColor"), 1, &lightColor[0]);
        glUniform3fv(glGetUniformLocation(sceneShaderProgram, "viewPos"), 1, &cameraPos[0]);
        
        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, depthMap);
        glUniform1i(glGetUniformLocation(sceneShaderProgram, "shadowMap"), 1);
        
        // Ensure hit flash is disabled for ground
        glUseProgram(sceneShaderProgram);
        glUniform1f(glGetUniformLocation(sceneShaderProgram, "hitFlashStrength"), 0.0f);
        
        renderGround(sceneShaderProgram, groundModel, view, projection);
        renderSnake(sceneShaderProgram, view, projection, lightSpaceMatrix);
        
        // Reset hit flash after snake rendering to prevent affecting other objects
        glUseProgram(sceneShaderProgram);
        glUniform1f(glGetUniformLocation(sceneShaderProgram, "hitFlashStrength"), 0.0f);
        
        renderStaff(sceneShaderProgram, view, projection, lightSpaceMatrix);
        
        // Render projectiles
        for (auto& projectile : projectileList) {
            projectile.Draw(sceneShaderProgram, view, projection);
        }
        
        glfwSwapBuffers(window);
        glfwPollEvents();
    }
    
    glDeleteVertexArrays(1, &groundVAO);
    glDeleteBuffers(1, &groundVBO);
    glDeleteProgram(sceneShaderProgram);
    glDeleteProgram(shadowShaderProgram);
    glDeleteFramebuffers(1, &depthMapFBO);
    glDeleteTextures(1, &depthMap);
    
    glfwTerminate();
    return 0;
}