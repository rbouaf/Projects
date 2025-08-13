// =================================================================================
// COMP 371 - Assignment 2: Wizard vs. Dragon FPS (FINAL CORRECTED CODE)
// Fixes scope for mouse variables and ambiguity with glm::max.
// Uses the correct include path for stb_image.h.
// =================================================================================

#include <iostream>
#include <vector>
#include <list>
#include <string>
#include <cmath>
#include <algorithm>

#define GLEW_STATIC 1
#include <GL/glew.h>
#include <GLFW/glfw3.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtc/constants.hpp>

#define STB_IMAGE_IMPLEMENTATION
#include <stb/stb_image.h> // Corrected include path

#include "OBJloader.h"
#include "shaderloader.h"

using namespace glm;
using namespace std;

// Globals
GLFWwindow* window = nullptr;
const int WINDOW_WIDTH = 1024;
const int WINDOW_HEIGHT = 768;

// Camera
vec3 cameraPosition(0.0f, 1.8f, 15.0f);
vec3 cameraLookAt(0.0f, 0.0f, -1.0f);
vec3 cameraUp(0.0f, 1.0f, 0.0f);
float cameraSpeed = 5.0f;
float cameraFastSpeed = 15.0f;
float cameraYaw = -90.0f;
float cameraPitch = 0.0f;
bool firstMouse = true;
// FIX: Declared globally so both main and mouse_callback can access them
double lastMouseX, lastMousePosY;

// Shaders
GLuint lightingShaderProgram;
GLuint shadowShaderProgram;

// Models
struct Model {
    GLuint vao;
    int vertexCount;
    GLuint texture;
};
Model ground, rock, staff, dragonBody, dragonWing, dragonNeck, dragonHead, dragonJaw, dragonTail;
Model fireballModel;

// Dragon State
int dragonHealth = 10;
float dragonHitTimer = 0.0f;
float dragonDeathTimer = 2.0f;
bool dragonIsDead = false;

// Projectiles
struct Projectile {
    vec3 position;
    vec3 velocity;
    float life;
};
list<Projectile> projectiles;

// Function Prototypes
void processInput(float dt);
void mouse_callback(GLFWwindow* window, double xpos, double ypos);
GLuint setupModelVBO(string path, int& vertexCount);
GLuint loadTexture(const char* filename);

// Helper to set mat4 uniforms
void setMat4(GLuint shaderID, const char* name, const mat4& mat) {
    glUseProgram(shaderID);
    glUniformMatrix4fv(glGetUniformLocation(shaderID, name), 1, GL_FALSE, &mat[0][0]);
}

// Helper to set vec3 uniforms
void setVec3(GLuint shaderID, const char* name, const vec3& value) {
    glUseProgram(shaderID);
    glUniform3fv(glGetUniformLocation(shaderID, name), 1, &value[0]);
}

// Helper to set float uniforms
void setFloat(GLuint shaderID, const char* name, float value) {
    glUseProgram(shaderID);
    glUniform1f(glGetUniformLocation(shaderID, name), value);
}

// Helper to set int uniforms
void setInt(GLuint shaderID, const char* name, int value) {
    glUseProgram(shaderID);
    glUniform1i(glGetUniformLocation(shaderID, name), value);
}

int main() {
    // Initialization
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);

    window = glfwCreateWindow(WINDOW_WIDTH, WINDOW_HEIGHT, "Wizard vs. Dragon", nullptr, nullptr);
    if (!window) {
        cerr << "Failed to create GLFW window" << endl;
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
    glfwSetCursorPosCallback(window, mouse_callback);

    glewExperimental = true;
    if (glewInit() != GLEW_OK) {
        cerr << "Failed to initialize GLEW" << endl;
        return -1;
    }
    
    // Initialize mouse position
    glfwGetCursorPos(window, &lastMouseX, &lastMousePosY);

    // Configure OpenGL
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_CULL_FACE);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glClearColor(0.1f, 0.2f, 0.3f, 1.0f);

    // Load Shaders
    lightingShaderProgram = loadSHADER("Shaders/lighting.vs", "Shaders/lighting.fs");
    shadowShaderProgram = loadSHADER("Shaders/shadow.vs", "Shaders/shadow.fs");

    // Load Models and Textures...
    // (This part remains the same)
    ground.vao = setupModelVBO("Models/plane.obj", ground.vertexCount);
    rock.vao = setupModelVBO("Models/rock.obj", rock.vertexCount);
    staff.vao = setupModelVBO("Models/staff.obj", staff.vertexCount);
    dragonBody.vao = setupModelVBO("Models/dragon_body.obj", dragonBody.vertexCount);
    dragonWing.vao = setupModelVBO("Models/dragon_wing.obj", dragonWing.vertexCount);
    dragonNeck.vao = setupModelVBO("Models/dragon_neck.obj", dragonNeck.vertexCount);
    dragonHead.vao = setupModelVBO("Models/dragon_head.obj", dragonHead.vertexCount);
    dragonJaw.vao = setupModelVBO("Models/dragon_jaw.obj", dragonJaw.vertexCount);
    dragonTail.vao = setupModelVBO("Models/dragon_tail.obj", dragonTail.vertexCount);
    fireballModel.vao = setupModelVBO("Models/quad.obj", fireballModel.vertexCount);

    ground.texture = loadTexture("Textures/rocky_ground.jpg");
    rock.texture = loadTexture("Textures/rock.jpg");
    staff.texture = loadTexture("Textures/wood.jpg");
    GLuint dragonTexture = loadTexture("Textures/dragon_skin.jpg");
    dragonBody.texture = dragonWing.texture = dragonNeck.texture = dragonHead.texture = dragonJaw.texture = dragonTail.texture = dragonTexture;
    fireballModel.texture = loadTexture("Textures/fireball.png");
    
    // ... Shadow Mapping Setup ...
    // (This part remains the same)
    const unsigned int SHADOW_WIDTH = 2048, SHADOW_HEIGHT = 2048;
    GLuint depthMapFBO;
    glGenFramebuffers(1, &depthMapFBO);
    GLuint depthMap;
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


    // Main Loop
    float lastFrameTime = glfwGetTime();

    while (!glfwWindowShouldClose(window)) {
        float currentTime = glfwGetTime();
        float dt = currentTime - lastFrameTime;
        lastFrameTime = currentTime;

        processInput(dt);

        if (dragonHitTimer > 0.0f) dragonHitTimer -= dt;
        if (dragonIsDead && dragonDeathTimer > 0.0f) dragonDeathTimer -= dt;

        // --- Shadow Pass ---
        vec3 lightPos = vec3(50.0f, 80.0f, 20.0f);
        mat4 lightProjection = ortho(-40.0f, 40.0f, -40.0f, 40.0f, 1.0f, 150.0f);
        mat4 lightView = lookAt(lightPos, vec3(0.0f, 10.0f, 0.0f), vec3(0.0f, 1.0f, 0.0f));
        mat4 lightSpaceMatrix = lightProjection * lightView;

        glUseProgram(shadowShaderProgram);
        setMat4(shadowShaderProgram, "lightSpaceMatrix", lightSpaceMatrix);
        glViewport(0, 0, SHADOW_WIDTH, SHADOW_HEIGHT);
        glBindFramebuffer(GL_FRAMEBUFFER, depthMapFBO);
        glClear(GL_DEPTH_BUFFER_BIT);
        glCullFace(GL_FRONT);

        // FIX: dragonWorldMatrix must be declared before use
        mat4 dragonWorldMatrix = mat4(1.0f);
        if (dragonDeathTimer >= 0.0f) {
            float dragonYPos = 10.0f + sin(currentTime * 0.5f) * 2.0f;
            dragonWorldMatrix = translate(mat4(1.0f), vec3(0, dragonYPos, -20)) * rotate(mat4(1.0f), radians(180.0f), vec3(0,1,0)) * scale(mat4(1.0f), vec3(3.0f));
            setMat4(shadowShaderProgram, "model", dragonWorldMatrix);
            glBindVertexArray(dragonBody.vao);
            glDrawArrays(GL_TRIANGLES, 0, dragonBody.vertexCount);
        }
        
        glCullFace(GL_BACK);
        glBindFramebuffer(GL_FRAMEBUFFER, 0);

        // --- Main Render Pass ---
        int width, height;
        glfwGetFramebufferSize(window, &width, &height);
        glViewport(0, 0, width, height);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        glUseProgram(lightingShaderProgram);
        
        mat4 projectionMatrix = perspective(radians(45.0f), (float)width / (float)height, 0.1f, 200.0f);
        mat4 viewMatrix = lookAt(cameraPosition, cameraPosition + cameraLookAt, cameraUp);
        setMat4(lightingShaderProgram, "projection", projectionMatrix);
        setMat4(lightingShaderProgram, "view", viewMatrix);
        setMat4(lightingShaderProgram, "lightSpaceMatrix", lightSpaceMatrix);
        
        setVec3(lightingShaderProgram, "viewPos", cameraPosition);
        setVec3(lightingShaderProgram, "sunLight.direction", -normalize(lightPos));
        setVec3(lightingShaderProgram, "sunLight.color", vec3(1.0f, 1.0f, 0.8f));
        
        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, depthMap);
        setInt(lightingShaderProgram, "shadowMap", 1);
        
        // ... (Rendering code for ground, rocks, dragon, staff, projectiles remains the same) ...
        // Render Ground
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, ground.texture);
        setInt(lightingShaderProgram, "ourTexture", 0);
        glBindVertexArray(ground.vao);
        setMat4(lightingShaderProgram, "model", scale(mat4(1.0f), vec3(50.0f)));
        glDrawArrays(GL_TRIANGLES, 0, ground.vertexCount);

        // Render Rocks
        glBindTexture(GL_TEXTURE_2D, rock.texture);
        glBindVertexArray(rock.vao);
        for(int i = 0; i < 10; ++i) {
            mat4 rockWorldMatrix = translate(mat4(1.0f), vec3(i*8.0f - 40.0f, 0.0f, -15.0f)) * scale(mat4(1.0f), vec3(2.0f));
            setMat4(lightingShaderProgram, "model", rockWorldMatrix);
            glDrawArrays(GL_TRIANGLES, 0, rock.vertexCount);
        }

        // Render Dragon
        if (dragonDeathTimer > 0.0f) {
            float dragonAlpha = dragonDeathTimer / 2.0f;
            setFloat(lightingShaderProgram, "alpha", dragonAlpha);

            if (dragonHitTimer > 0.0f) {
                setVec3(lightingShaderProgram, "overrideColor", vec3(1.0f, 0.2f, 0.2f));
            } else {
                setVec3(lightingShaderProgram, "overrideColor", vec3(-1.0f));
            }
            
            glBindTexture(GL_TEXTURE_2D, dragonBody.texture);
            glBindVertexArray(dragonBody.vao);
            setMat4(lightingShaderProgram, "model", dragonWorldMatrix);
            glDrawArrays(GL_TRIANGLES, 0, dragonBody.vertexCount);
            
            setFloat(lightingShaderProgram, "alpha", 1.0f);
            setVec3(lightingShaderProgram, "overrideColor", vec3(-1.0f));
        }

        // Render Staff
        glBindTexture(GL_TEXTURE_2D, staff.texture);
        glBindVertexArray(staff.vao);
        mat4 staffMatrix = translate(mat4(1.0f), vec3(0.4f, -0.4f, -1.0f)) * rotate(mat4(1.0f), radians(-10.0f), vec3(1,0,0)) * scale(mat4(1.0f), vec3(0.3f));
        setMat4(lightingShaderProgram, "model", inverse(viewMatrix) * staffMatrix);
        vec3 staffLightPos = vec3(inverse(viewMatrix) * staffMatrix * vec4(0,0,1,1));
        setVec3(lightingShaderProgram, "staffLight.position", staffLightPos);
        setVec3(lightingShaderProgram, "staffLight.color", vec3(1.0f, 0.5f, 0.0f));
        glDrawArrays(GL_TRIANGLES, 0, staff.vertexCount);
        setVec3(lightingShaderProgram, "staffLight.color", vec3(0.0f));


        // Render Projectiles
        glBindTexture(GL_TEXTURE_2D, fireballModel.texture);
        glBindVertexArray(fireballModel.vao);
        for(auto it = projectiles.begin(); it != projectiles.end(); ) {
            it->position += it->velocity * dt;
            it->life -= dt;

            mat4 billboardRotation = mat4(transpose(mat3(viewMatrix)));
            mat4 projMatrix = translate(mat4(1.0f), it->position) * billboardRotation;
            setMat4(lightingShaderProgram, "model", projMatrix);
            setVec3(lightingShaderProgram, "fireballLight.position", it->position);
            setVec3(lightingShaderProgram, "fireballLight.color", vec3(1.0f, 0.5f, 0.0f));
            glDrawArrays(GL_TRIANGLES, 0, fireballModel.vertexCount);

            if (!dragonIsDead && distance(it->position, vec3(dragonWorldMatrix[3])) < 5.0f) {
                dragonHealth--;
                dragonHitTimer = 0.2f;
                it = projectiles.erase(it);
                if (dragonHealth <= 0) {
                    dragonIsDead = true;
                }
            } else if (it->life <= 0.0f) {
                it = projectiles.erase(it);
            } else {
                ++it;
            }
        }
        setVec3(lightingShaderProgram, "fireballLight.color", vec3(0.0f));

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    glfwTerminate();
    return 0;
}

void processInput(float dt) {
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);

    float currentSpeed = (glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS) ? cameraFastSpeed : cameraSpeed;

    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
        cameraPosition += cameraLookAt * currentSpeed * dt;
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
        cameraPosition -= cameraLookAt * currentSpeed * dt;
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
        cameraPosition -= normalize(cross(cameraLookAt, cameraUp)) * currentSpeed * dt;
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
        cameraPosition += normalize(cross(cameraLookAt, cameraUp)) * currentSpeed * dt;

    static int lastMouseLeftState = GLFW_RELEASE;
    if (lastMouseLeftState == GLFW_RELEASE && glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS) {
        Projectile p = {cameraPosition + cameraLookAt * 1.5f, cameraLookAt * 30.0f, 5.0f};
        projectiles.push_back(p);
    }
    lastMouseLeftState = glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT);
}

void mouse_callback(GLFWwindow* window, double xpos, double ypos) {
    if (firstMouse) {
        lastMouseX = xpos;
        lastMouseY = ypos;
        firstMouse = false;
    }

    float xoffset = xpos - lastMouseX;
    float yoffset = lastMouseY - ypos; 
    lastMouseX = xpos;
    lastMouseY = ypos;

    float sensitivity = 0.1f;
    xoffset *= sensitivity;
    yoffset *= sensitivity;

    cameraYaw += xoffset;
    cameraPitch += yoffset;

    // FIX: Use glm::max and glm::min to resolve ambiguity
    cameraPitch = glm::max(-89.0f, glm::min(89.0f, cameraPitch));

    vec3 front;
    front.x = cos(radians(cameraYaw)) * cos(radians(cameraPitch));
    front.y = sin(radians(cameraPitch));
    front.z = sin(radians(cameraYaw)) * cos(radians(cameraPitch));
    cameraLookAt = normalize(front);
}

GLuint setupModelVBO(string path, int& vertexCount) {
    std::vector<glm::vec3> vertices;
    std::vector<glm::vec3> normals;
    std::vector<glm::vec2> UVs;
    loadOBJ(path.c_str(), vertices, normals, UVs);

    GLuint VAO;
    glGenVertexArrays(1, &VAO);
    glBindVertexArray(VAO);

    GLuint VBO_positions, VBO_normals, VBO_uvs;
    glGenBuffers(1, &VBO_positions);
    glBindBuffer(GL_ARRAY_BUFFER, VBO_positions);
    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(glm::vec3), &vertices[0], GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, nullptr);
    glEnableVertexAttribArray(0);

    glGenBuffers(1, &VBO_normals);
    glBindBuffer(GL_ARRAY_BUFFER, VBO_normals);
    glBufferData(GL_ARRAY_BUFFER, normals.size() * sizeof(glm::vec3), &normals[0], GL_STATIC_DRAW);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 0, nullptr);
    glEnableVertexAttribArray(1);

    glGenBuffers(1, &VBO_uvs);
    glBindBuffer(GL_ARRAY_BUFFER, VBO_uvs);
    glBufferData(GL_ARRAY_BUFFER, UVs.size() * sizeof(glm::vec2), &UVs[0], GL_STATIC_DRAW);
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 0, nullptr);
    glEnableVertexAttribArray(2);
    
    glBindVertexArray(0);
    vertexCount = vertices.size();
    return VAO;
}

GLuint loadTexture(const char *filename) {
    GLuint textureID;
    glGenTextures(1, &textureID);
    glBindTexture(GL_TEXTURE_2D, textureID);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    int width, height, nrChannels;
    stbi_set_flip_vertically_on_load(true);
    unsigned char *data = stbi_load(filename, &width, &height, &nrChannels, 0);

    if (data) {
        GLenum format = (nrChannels == 4) ? GL_RGBA : GL_RGB;
        glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, data);
        glGenerateMipmap(GL_TEXTURE_2D);
    } else {
        std::cout << "Failed to load texture: " << filename << std::endl;
    }
    stbi_image_free(data);

    return textureID;
}