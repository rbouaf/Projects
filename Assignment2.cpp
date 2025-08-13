// -----------------------------
// Wizard Shooter (single-pass dynamic fireball lights)
// -----------------------------

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

#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

#define M_PI 3.14159265358979323846

using namespace glm;
using namespace std;

// ------------------------------------
// Config
// ------------------------------------
const GLuint WIDTH = 1200, HEIGHT = 800;
const int MAX_FIREBALLS = 32;

// ------------------------------------
// Globals
// ------------------------------------

// Camera
vec3 cameraPos = vec3(2.0f, 2.0f, -5.0f); // spawn inside dome next to snake
vec3 cameraFront = vec3(0.0f, 0.0f, -1.0f);
vec3 cameraUp = vec3(0.0f, 1.0f, 0.0f);

float cameraYaw = -90.0f;
float cameraPitch = 0.0f;
float lastX = WIDTH / 2.0f;
float lastY = HEIGHT / 2.0f;
bool firstMouse = true;

// Movement
bool keys[1024]{};
float deltaTime = 0.0f;
float lastFrame = 0.0f;

// Physics
bool flightMode = false;
vec3 cameraVelocity = vec3(0.0f);
float gravity = -9.8f;
float groundLevel = 2.0f;

// Shooting
bool leftMousePressed = false;
float lastShotTime = 0.0f;
float shootCooldown = 0.5f;
float staffShakeTimer = 0.0f;
list<struct Projectile> projectileList;

// Hit effects
bool snakeHit = false;
float hitFlashTimer = 0.0f;
float hitFlashDuration = 0.3f;

// Lighting (sun / directional-ish) - very dim for cave
vec3 lightPos = vec3(10.0f, 10.0f, 10.0f);
vec3 lightColor = vec3(0.15f, 0.15f, 0.15f); // very dim sun for cave atmosphere

// Shader programs
GLuint sceneShaderProgram;
GLuint shadowShaderProgram;

// Shadow mapping
GLuint depthMapFBO;
GLuint depthMap;
const unsigned int SHADOW_WIDTH = 2048, SHADOW_HEIGHT = 2048;

// Ground
GLuint groundVAO = 0, groundVBO = 0;

// Cube (kept for testing)
GLuint cubeVAO = 0, cubeVBO = 0;

// Geodesic dome (hemisphere)
GLuint domeVAO = 0, domeVBO = 0, domeEBO = 0;
int domeIndexCount = 0;
GLuint domeTexture = 0;

// Fixed world position (don't center on camera) - lowered dome to ground level
glm::vec3 domeCenter = glm::vec3(0.0f, -11.0f, 0.0f);

// Sphere for fireballs
GLuint sphereVAO = 0, sphereVBO = 0;
int sphereVertexCount = 0;

// Dragon models
struct DragonModel
{
    GLuint VAO = 0, VBO = 0;
    vector<vec3> vertices;
    vector<vec3> normals;
    vector<vec2> uvs;
    GLuint texture = 0;
};

DragonModel dragonHead;
DragonModel fishBody;
DragonModel bearPaw;   // (unused in this file but kept for parity)
DragonModel eagleWing; // (unused)
DragonModel staff;

// Snake neck chain
const int SNAKE_NECK_SEGMENTS = 20;
struct SnakeNeckSegment
{
    vec3 position;
    vec3 rotation;
    float animationPhase;
};
vector<SnakeNeckSegment> snakeNeckSegments(SNAKE_NECK_SEGMENTS);
vec3 snakeBasePos = vec3(0.0f, 0.0f, -8.0f); // spawn snake inside dome
float snakeAnimationTime = 0.0f;

// Snake adjustables
vec3 headScale = vec3(0.4f); // bigger dragon head
vec3 headRotation = vec3(0.0f);

vec3 neckScale = vec3(0.2f, 0.2f, 0.2f);
float neckLength = 10.0f; // shorter neck for closer segments
float oscillationStrength = 1.5f;
float oscillationSpeed = 2.0f;

float headFollowStrength = 0.3f;
float maxHeadDistance = 3.0f;


// FPS staff
vec3 staffPosition = vec3(0.5f, -0.3f, 0.8f);
vec3 staffRotation = vec3(0.0f);
vec3 staffScale = vec3(0.3f);
float staffShakeAmount = 0.05f;
float staffShakeDuration = 0.2f;

// ------------------------------------
// Forward decls
// ------------------------------------
void processInput(GLFWwindow *window);
void mouse_callback(GLFWwindow *window, double xpos, double ypos);
void key_callback(GLFWwindow *window, int key, int scancode, int action, int mode);

GLuint loadTexture(const char *path);
GLuint createShaderProgram(const char *vertexPath, const char *fragmentPath);
void setupShadowMapping();
void setupGround();
void renderGround(GLuint shaderProgram, mat4 model, mat4 view, mat4 projection);
void setupCube();
void setupSphere();
bool loadDragonModel(DragonModel &model, const char *objPath, const char *texturePath);
void setupSnakeModels();
void updateSnakeAnimation(float deltaTime, vec3 cameraPos);
bool checkCollision(vec3 projectilePos, vec3 segmentPos, float radius);
void playHitSound();
void renderDragonModel(DragonModel &model, GLuint shaderProgram, mat4 modelMatrix, mat4 view, mat4 projection, mat4 lightSpaceMatrix);
void renderSnake(GLuint shaderProgram, mat4 view, mat4 projection, mat4 lightSpaceMatrix);
void renderStaff(GLuint shaderProgram, mat4 view, mat4 projection, mat4 lightSpaceMatrix);
void setupDome();
void renderDome();
void setupDomeGeodesic();
void renderDomeGeodesic();

string loadShaderSource(const char *filename)
{
    ifstream file(filename);
    if (!file.is_open())
    {
        cout << "ERROR: Unable to read shader file " << filename << endl;
        return "";
    }
    stringstream ss;
    ss << file.rdbuf();
    return ss.str();
}

GLuint compileShader(GLenum type, const string &source)
{
    GLuint shader = glCreateShader(type);
    const char *src = source.c_str();
    glShaderSource(shader, 1, &src, NULL);
    glCompileShader(shader);
    GLint success;
    GLchar infoLog[512];
    glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
    if (!success)
    {
        glGetShaderInfoLog(shader, 512, NULL, infoLog);
        cout << "ERROR: Shader compilation failed\n"
             << infoLog << endl;
    }
    return shader;
}

GLuint createShaderProgram(const char *vertexPath, const char *fragmentPath)
{
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
    if (!success)
    {
        glGetProgramInfoLog(program, 512, NULL, infoLog);
        cout << "ERROR: Shader program linking failed\n"
             << infoLog << endl;
    }
    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);
    return program;
}

// ------------------------------------
// Projectile
// ------------------------------------
class Projectile
{
public:
    Projectile(vec3 position, vec3 velocity) : mPosition(position), mVelocity(velocity) {}
    void Update(float dt) { mPosition += mVelocity * dt; }

    void Draw(GLuint shaderProgram, mat4 view, mat4 projection)
    {
        glUseProgram(shaderProgram);

        static float fireballTime = 0.0f;
        fireballTime += 0.016f;
        mat4 worldMatrix = translate(mat4(1.0f), mPosition) * rotate(mat4(1.0f), fireballTime * 3.0f, vec3(0.5f, 1.0f, 0.3f)) * scale(mat4(1.0f), vec3(0.2f));

        glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "model"), 1, GL_FALSE, value_ptr(worldMatrix));
        glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "view"), 1, GL_FALSE, value_ptr(view));
        glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "projection"), 1, GL_FALSE, value_ptr(projection));

        // Emissive fireball toggle
        glUniform1f(glGetUniformLocation(shaderProgram, "isFireball"), 1.0f);
        glUniform1f(glGetUniformLocation(shaderProgram, "hitFlashStrength"), 0.0f);

        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, 0);
        glUniform1i(glGetUniformLocation(shaderProgram, "texture_diffuse1"), 0);

        extern GLuint sphereVAO;
        extern int sphereVertexCount;
        glBindVertexArray(sphereVAO);
        glDrawElements(GL_TRIANGLES, sphereVertexCount, GL_UNSIGNED_INT, 0);
        glBindVertexArray(0);
    }

    vec3 getPosition() const { return mPosition; }

private:
    vec3 mPosition;
    vec3 mVelocity;
};

// ------------------------------------
// Assets setup
// ------------------------------------
GLuint loadTexture(const char *path)
{
    GLuint textureID;
    glGenTextures(1, &textureID);
    int width, height, nrChannels;
    cout << "Loading texture: " << path << endl;
    unsigned char *data = stbi_load(path, &width, &height, &nrChannels, 0);
    if (data)
    {
        GLenum format = (nrChannels == 1 ? GL_RED : (nrChannels == 3 ? GL_RGB : GL_RGBA));
        cout << "Texture loaded: " << width << "x" << height << " ch=" << nrChannels << endl;
        glBindTexture(GL_TEXTURE_2D, textureID);
        glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, data);
        glGenerateMipmap(GL_TEXTURE_2D);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        stbi_image_free(data);
    }
    else
    {
        cout << "FAILED to load texture: " << path << " reason: " << stbi_failure_reason() << endl;
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

void setupShadowMapping()
{
    glGenFramebuffers(1, &depthMapFBO);
    glGenTextures(1, &depthMap);
    glBindTexture(GL_TEXTURE_2D, depthMap);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT, SHADOW_WIDTH, SHADOW_HEIGHT, 0,
                 GL_DEPTH_COMPONENT, GL_FLOAT, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
    float borderColor[] = {1, 1, 1, 1};
    glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, borderColor);

    glBindFramebuffer(GL_FRAMEBUFFER, depthMapFBO);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, depthMap, 0);
    glDrawBuffer(GL_NONE);
    glReadBuffer(GL_NONE);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void setupGround()
{
    float groundVertices[] = {
        // positions             // normals        // uvs
        -50,
        0,
        -50,
        0,
        1,
        0,
        0,
        50,
        50,
        0,
        -50,
        0,
        1,
        0,
        50,
        50,
        50,
        0,
        50,
        0,
        1,
        0,
        50,
        0,
        50,
        0,
        50,
        0,
        1,
        0,
        50,
        0,
        -50,
        0,
        50,
        0,
        1,
        0,
        0,
        0,
        -50,
        0,
        -50,
        0,
        1,
        0,
        0,
        50,
    };

    glGenVertexArrays(1, &groundVAO);
    glGenBuffers(1, &groundVBO);

    glBindVertexArray(groundVAO);
    glBindBuffer(GL_ARRAY_BUFFER, groundVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(groundVertices), groundVertices, GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void *)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void *)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void *)(6 * sizeof(float)));
    glEnableVertexAttribArray(2);
    glBindVertexArray(0);
}

void setupCube()
{
    float cubeVertices[] = {
        // pos                  // n               // uv
        -0.5, -0.5, 0.5, 0, 0, 1, 0, 0,
        0.5, -0.5, 0.5, 0, 0, 1, 1, 0,
        0.5, 0.5, 0.5, 0, 0, 1, 1, 1,
        0.5, 0.5, 0.5, 0, 0, 1, 1, 1,
        -0.5, 0.5, 0.5, 0, 0, 1, 0, 1,
        -0.5, -0.5, 0.5, 0, 0, 1, 0, 0,

        -0.5, -0.5, -0.5, 0, 0, -1, 0, 0,
        0.5, -0.5, -0.5, 0, 0, -1, 1, 0,
        0.5, 0.5, -0.5, 0, 0, -1, 1, 1,
        0.5, 0.5, -0.5, 0, 0, -1, 1, 1,
        -0.5, 0.5, -0.5, 0, 0, -1, 0, 1,
        -0.5, -0.5, -0.5, 0, 0, -1, 0, 0,

        -0.5, 0.5, 0.5, -1, 0, 0, 1, 0,
        -0.5, 0.5, -0.5, -1, 0, 0, 1, 1,
        -0.5, -0.5, -0.5, -1, 0, 0, 0, 1,
        -0.5, -0.5, -0.5, -1, 0, 0, 0, 1,
        -0.5, -0.5, 0.5, -1, 0, 0, 0, 0,
        -0.5, 0.5, 0.5, -1, 0, 0, 1, 0,

        0.5, 0.5, 0.5, 1, 0, 0, 1, 0,
        0.5, 0.5, -0.5, 1, 0, 0, 1, 1,
        0.5, -0.5, -0.5, 1, 0, 0, 0, 1,
        0.5, -0.5, -0.5, 1, 0, 0, 0, 1,
        0.5, -0.5, 0.5, 1, 0, 0, 0, 0,
        0.5, 0.5, 0.5, 1, 0, 0, 1, 0,

        -0.5, -0.5, -0.5, 0, -1, 0, 0, 1,
        0.5, -0.5, -0.5, 0, -1, 0, 1, 1,
        0.5, -0.5, 0.5, 0, -1, 0, 1, 0,
        0.5, -0.5, 0.5, 0, -1, 0, 1, 0,
        -0.5, -0.5, 0.5, 0, -1, 0, 0, 0,
        -0.5, -0.5, -0.5, 0, -1, 0, 0, 1,

        -0.5, 0.5, -0.5, 0, 1, 0, 0, 1,
        0.5, 0.5, -0.5, 0, 1, 0, 1, 1,
        0.5, 0.5, 0.5, 0, 1, 0, 1, 0,
        0.5, 0.5, 0.5, 0, 1, 0, 1, 0,
        -0.5, 0.5, 0.5, 0, 1, 0, 0, 0,
        -0.5, 0.5, -0.5, 0, 1, 0, 0, 1};

    glGenVertexArrays(1, &cubeVAO);
    glGenBuffers(1, &cubeVBO);
    glBindVertexArray(cubeVAO);
    glBindBuffer(GL_ARRAY_BUFFER, cubeVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(cubeVertices), cubeVertices, GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void *)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void *)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void *)(6 * sizeof(float)));
    glEnableVertexAttribArray(2);
    glBindVertexArray(0);
}

void setupSphere()
{
    const int stacks = 20, slices = 20;
    const float radius = 1.0f;
    vector<float> vertices;
    for (int i = 0; i <= stacks; ++i)
    {
        float stackAngle = M_PI / 2 - i * M_PI / stacks;
        float xy = radius * cosf(stackAngle);
        float z = radius * sinf(stackAngle);
        for (int j = 0; j <= slices; ++j)
        {
            float sectorAngle = j * 2 * M_PI / slices;
            float x = xy * cosf(sectorAngle);
            float y = xy * sinf(sectorAngle);
            vertices.push_back(x);
            vertices.push_back(y);
            vertices.push_back(z);
            vertices.push_back(x);
            vertices.push_back(y);
            vertices.push_back(z);
            vertices.push_back((float)j / slices);
            vertices.push_back((float)i / stacks);
        }
    }
    vector<unsigned int> indices;
    for (int i = 0; i < stacks; ++i)
    {
        int k1 = i * (slices + 1);
        int k2 = k1 + slices + 1;
        for (int j = 0; j < slices; ++j, ++k1, ++k2)
        {
            if (i != 0)
            {
                indices.push_back(k1);
                indices.push_back(k2);
                indices.push_back(k1 + 1);
            }
            if (i != (stacks - 1))
            {
                indices.push_back(k1 + 1);
                indices.push_back(k2);
                indices.push_back(k2 + 1);
            }
        }
    }
    sphereVertexCount = (int)indices.size();

    GLuint sphereEBO;
    glGenVertexArrays(1, &sphereVAO);
    glGenBuffers(1, &sphereVBO);
    glGenBuffers(1, &sphereEBO);
    glBindVertexArray(sphereVAO);
    glBindBuffer(GL_ARRAY_BUFFER, sphereVBO);
    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(float), vertices.data(), GL_STATIC_DRAW);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, sphereEBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(unsigned int), indices.data(), GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void *)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void *)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void *)(6 * sizeof(float)));
    glEnableVertexAttribArray(2);
    glBindVertexArray(0);
}

void renderGround(GLuint shaderProgram, mat4 model, mat4 view, mat4 projection)
{
    glUseProgram(shaderProgram);
    glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "model"), 1, GL_FALSE, value_ptr(model));
    glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "view"), 1, GL_FALSE, value_ptr(view));
    glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "projection"), 1, GL_FALSE, value_ptr(projection));

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, domeTexture);
    glUniform1i(glGetUniformLocation(shaderProgram, "texture_diffuse1"), 0);

    // ensure not emissive
    glUniform1f(glGetUniformLocation(shaderProgram, "isFireball"), 0.0f);

    glBindVertexArray(groundVAO);
    glDrawArrays(GL_TRIANGLES, 0, 6);
    glBindVertexArray(0);
}

bool loadDragonModel(DragonModel &model, const char *objPath, const char *texturePath)
{
    cout << "Loading model with Assimp: " << objPath << endl;

    Assimp::Importer importer;
    // Only use essential flags for speed
    const aiScene *scene = importer.ReadFile(objPath, aiProcess_Triangulate);

    if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode)
    {
        cout << "ERROR: Assimp failed to load " << objPath << ": " << importer.GetErrorString() << endl;
        return false;
    }

    model.vertices.clear();
    model.normals.clear();
    model.uvs.clear();

    // Process only the first mesh for speed
    if (scene->mNumMeshes > 0)
    {
        aiMesh *mesh = scene->mMeshes[0];

        // Reserve space for better performance
        model.vertices.reserve(mesh->mNumFaces * 3);
        model.normals.reserve(mesh->mNumFaces * 3);
        model.uvs.reserve(mesh->mNumFaces * 3);

        // Process faces directly - convert to individual vertices for glDrawArrays
        for (unsigned int i = 0; i < mesh->mNumFaces; i++)
        {
            aiFace face = mesh->mFaces[i];
            for (unsigned int j = 0; j < face.mNumIndices; j++)
            {
                unsigned int index = face.mIndices[j];

                // Vertices
                model.vertices.push_back(vec3(
                    mesh->mVertices[index].x,
                    mesh->mVertices[index].y,
                    mesh->mVertices[index].z));

                // Normals
                if (mesh->HasNormals())
                {
                    model.normals.push_back(vec3(
                        mesh->mNormals[index].x,
                        mesh->mNormals[index].y,
                        mesh->mNormals[index].z));
                }
                else
                {
                    model.normals.push_back(vec3(0.0f, 1.0f, 0.0f));
                }

                // UVs
                if (mesh->mTextureCoords[0])
                {
                    model.uvs.push_back(vec2(
                        mesh->mTextureCoords[0][index].x,
                        mesh->mTextureCoords[0][index].y));
                }
                else
                {
                    model.uvs.push_back(vec2(0.0f, 0.0f));
                }
            }
        }
    }

    cout << "Loaded " << model.vertices.size() << " vertices" << endl;

    // Create OpenGL buffers
    glGenVertexArrays(1, &model.VAO);
    GLuint vertexVBO, normalVBO, uvVBO;
    glGenBuffers(1, &vertexVBO);
    glGenBuffers(1, &normalVBO);
    glGenBuffers(1, &uvVBO);

    glBindVertexArray(model.VAO);

    // Vertices
    glBindBuffer(GL_ARRAY_BUFFER, vertexVBO);
    glBufferData(GL_ARRAY_BUFFER, model.vertices.size() * sizeof(vec3), model.vertices.data(), GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, (void *)0);
    glEnableVertexAttribArray(0);

    // Normals
    glBindBuffer(GL_ARRAY_BUFFER, normalVBO);
    glBufferData(GL_ARRAY_BUFFER, model.normals.size() * sizeof(vec3), model.normals.data(), GL_STATIC_DRAW);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 0, (void *)0);
    glEnableVertexAttribArray(1);

    // UVs
    glBindBuffer(GL_ARRAY_BUFFER, uvVBO);
    glBufferData(GL_ARRAY_BUFFER, model.uvs.size() * sizeof(vec2), model.uvs.data(), GL_STATIC_DRAW);
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 0, (void *)0);
    glEnableVertexAttribArray(2);

    glBindVertexArray(0);
    model.VBO = vertexVBO;

    // Load texture
    if (texturePath)
        model.texture = loadTexture(texturePath);
    else
    {
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

void setupSnakeModels()
{
    cout << "Setting up snake creature models..." << endl;
    if (!loadDragonModel(dragonHead, "Models/dragon_head.obj", "Textures/dragon_texture.jpg"))
        cout << "ERROR: Failed to load dragon head!\n";
    if (!loadDragonModel(fishBody, "Models/fish.obj", "Textures/fish_texture.jpg"))
        cout << "ERROR: Failed to load fish!\n";
    if (!loadDragonModel(staff, "Models/staff.obj", "Textures/light_surface.jpg"))
        cout << "ERROR: Failed to load staff!\n";
    for (int i = 0; i < SNAKE_NECK_SEGMENTS; i++)
    {
        snakeNeckSegments[i].animationPhase = (float)i / SNAKE_NECK_SEGMENTS * 2.0f * 3.14159f;
        snakeNeckSegments[i].position = vec3(0);
        snakeNeckSegments[i].rotation = vec3(0);
    }
}

// ------------------------------------
// Polygonal Dome (hemisphere) generator
// ------------------------------------
// ---- Geodesic half-icosphere with flat shading ----
struct Tri
{
    glm::vec3 a, b, c;
};

// base icosahedron (unit)
static std::vector<glm::vec3> icosaVerts()
{
    const float t = (1.0f + sqrtf(5.0f)) * 0.5f;
    std::vector<glm::vec3> v = {
        {-1, t, 0}, {1, t, 0}, {-1, -t, 0}, {1, -t, 0}, {0, -1, t}, {0, 1, t}, {0, -1, -t}, {0, 1, -t}, {t, 0, -1}, {t, 0, 1}, {-t, 0, -1}, {-t, 0, 1}};
    for (auto &p : v)
        p = glm::normalize(p);
    return v;
}
static std::vector<glm::ivec3> icosaFaces()
{
    return {
        {0, 11, 5}, {0, 5, 1}, {0, 1, 7}, {0, 7, 10}, {0, 10, 11}, {1, 5, 9}, {5, 11, 4}, {11, 10, 2}, {10, 7, 6}, {7, 1, 8}, {3, 9, 4}, {3, 4, 2}, {3, 2, 6}, {3, 6, 8}, {3, 8, 9}, {4, 9, 5}, {2, 4, 11}, {6, 2, 10}, {8, 6, 7}, {9, 8, 1}};
}

// subdivide a triangle (frequency f), projecting to unit sphere
static void subdivide(const Tri &t, int f, std::vector<Tri> &out)
{
    if (f == 0)
    {
        out.push_back(t);
        return;
    }
    auto mid = [](const glm::vec3 &a, const glm::vec3 &b)
    {
        return glm::normalize(0.5f * (a + b));
    };
    glm::vec3 a = t.a, b = t.b, c = t.c;
    glm::vec3 ab = mid(a, b), bc = mid(b, c), ca = mid(c, a);
    std::vector<Tri> sub = {
        {a, ab, ca},
        {ab, b, bc},
        {ca, bc, c},
        {ab, bc, ca}};
    for (auto &s : sub)
        subdivide(s, f - 1, out);
}

// build hemisphere from subdivided icosahedron; duplicate verts per face for flat shading
void setupDomeGeodesic(int subdivLevel = 2, float radius = 10.0f, float tile = 4.0f)
{
    std::vector<glm::vec3> V = icosaVerts();
    auto F = icosaFaces();

    // collect subdivided triangles on unit sphere
    std::vector<Tri> tris;
    tris.reserve(10 * (int)pow(4, subdivLevel)); // rough
    for (auto f : F)
    {
        Tri t{V[f.x], V[f.y], V[f.z]};
        subdivide(t, subdivLevel, tris);
    }

    // keep only hemisphere (y >= 0), and project to radius
    std::vector<float> interleaved; // pos(3) normal(3) uv(2) per-vertex, but flat shaded per-tri
    std::vector<unsigned int> indices;
    interleaved.reserve(tris.size() * 3 * 8);
    indices.reserve(tris.size() * 3);

    auto uv_proj = [tile](const glm::vec3 &p) -> glm::vec2
    {
        // Fixed spherical UVs that don't depend on camera
        float u = (atan2f(p.z, p.x) / (2.0f * (float)M_PI)) + 0.5f;
        float v = (asinf(p.y) / (float)M_PI) + 0.5f; // proper spherical V coordinate
        return glm::vec2(u * tile, v * tile);
    };

    unsigned int idx = 0;
    for (auto &t : tris)
    {
        // hemisphere cut: only triangles fully on y>=0 to avoid cracks
        if (t.a.y < 0 || t.b.y < 0 || t.c.y < 0)
            continue;

        // scale to radius
        glm::vec3 A = glm::normalize(t.a) * radius;
        glm::vec3 B = glm::normalize(t.b) * radius;
        glm::vec3 C = glm::normalize(t.c) * radius;

        // flat normal (inward)
        glm::vec3 N = -glm::normalize(glm::cross(B - A, C - A));

        // per-face UVs
        glm::vec2 uA = uv_proj(glm::normalize(t.a));
        glm::vec2 uB = uv_proj(glm::normalize(t.b));
        glm::vec2 uC = uv_proj(glm::normalize(t.c));

        auto push = [&](const glm::vec3 &P, const glm::vec2 &UV)
        {
            interleaved.push_back(P.x);
            interleaved.push_back(P.y);
            interleaved.push_back(P.z);
            interleaved.push_back(N.x);
            interleaved.push_back(N.y);
            interleaved.push_back(N.z);
            interleaved.push_back(UV.x);
            interleaved.push_back(UV.y);
            indices.push_back(idx++);
        };
        push(A, uA);
        push(B, uB);
        push(C, uC);
    }

    domeIndexCount = (int)indices.size();
    if (domeVAO == 0)
    {
        glGenVertexArrays(1, &domeVAO);
        glGenBuffers(1, &domeVBO);
        glGenBuffers(1, &domeEBO);
    }
    glBindVertexArray(domeVAO);
    glBindBuffer(GL_ARRAY_BUFFER, domeVBO);
    glBufferData(GL_ARRAY_BUFFER, interleaved.size() * sizeof(float), interleaved.data(), GL_STATIC_DRAW);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, domeEBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(unsigned int), indices.data(), GL_STATIC_DRAW);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void *)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void *)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void *)(6 * sizeof(float)));
    glEnableVertexAttribArray(2);
    glBindVertexArray(0);

    domeTexture = loadTexture("Textures/cave.jpg");
}
void renderDomeGeodesic(GLuint shaderProgram, const glm::mat4 &view, const glm::mat4 &projection)
{
    if (domeVAO == 0 || domeIndexCount == 0)
        return;

    glUseProgram(shaderProgram);

    // don’t block scene: write no depth, draw inside faces
    glDepthMask(GL_FALSE);
    glCullFace(GL_FRONT);

    glm::mat4 model(1.0f);
    model = glm::translate(model, domeCenter); // fixed world pos

    glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "model"), 1, GL_FALSE, glm::value_ptr(model));
    glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "view"), 1, GL_FALSE, glm::value_ptr(view));
    glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "projection"), 1, GL_FALSE, glm::value_ptr(projection));

    glUniform1f(glGetUniformLocation(shaderProgram, "isFireball"), 0.0f);
    glUniform1f(glGetUniformLocation(shaderProgram, "hitFlashStrength"), 0.0f);

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, domeTexture);
    glUniform1i(glGetUniformLocation(shaderProgram, "texture_diffuse1"), 0);

    glBindVertexArray(domeVAO);
    glDrawElements(GL_TRIANGLES, domeIndexCount, GL_UNSIGNED_INT, 0);
    glBindVertexArray(0);

    glCullFace(GL_BACK);
    glDepthMask(GL_TRUE);
}

void setupDome(int stacks = 12, int slices = 24, float radius = 20.0f, float tile = 6.0f)
{
    // stacks: vertical bands from top (y=+r) down to equator (y=0)
    // slices: horizontal segments around Y
    // tile: texture tiling factor
    std::vector<glm::vec3> positions;
    std::vector<glm::vec3> normals;
    std::vector<glm::vec2> uvs;
    std::vector<unsigned int> indices;

    // generate hemisphere vertices
    for (int i = 0; i <= stacks; ++i)
    {
        float v = (float)i / (float)stacks;     // 0..1
        float phi = (M_PI * 0.5f) * (1.0f - v); // from +pi/2 (top) down to 0 (equator)
        float y = radius * sinf(phi);
        float r = radius * cosf(phi); // ring radius at this stack

        for (int j = 0; j <= slices; ++j)
        {
            float u = (float)j / (float)slices; // 0..1
            float theta = u * 2.0f * (float)M_PI;

            float x = r * cosf(theta);
            float z = r * sinf(theta);

            glm::vec3 p(x, y, z);
            positions.push_back(p);

            // inward normals so the inside is lit by interior lights
            glm::vec3 n = -glm::normalize(p); // point inward
            normals.push_back(n);

            // spherical-ish UVs (tile factor)
            uvs.push_back(glm::vec2(u * tile, (1.0f - v) * tile));
        }
    }

    // indices (triangles)
    int ring = slices + 1;
    for (int i = 0; i < stacks; ++i)
    {
        for (int j = 0; j < slices; ++j)
        {
            int k1 = i * ring + j;
            int k2 = k1 + ring;

            // two tris per quad
            indices.push_back(k1);
            indices.push_back(k2);
            indices.push_back(k1 + 1);

            indices.push_back(k1 + 1);
            indices.push_back(k2);
            indices.push_back(k2 + 1);
        }
    }

    domeIndexCount = (int)indices.size();

    // interleave: pos(3) normal(3) uv(2)
    std::vector<float> interleaved;
    interleaved.reserve(positions.size() * 8);
    for (size_t i = 0; i < positions.size(); ++i)
    {
        interleaved.push_back(positions[i].x);
        interleaved.push_back(positions[i].y);
        interleaved.push_back(positions[i].z);
        interleaved.push_back(normals[i].x);
        interleaved.push_back(normals[i].y);
        interleaved.push_back(normals[i].z);
        interleaved.push_back(uvs[i].x);
        interleaved.push_back(uvs[i].y);
    }

    glGenVertexArrays(1, &domeVAO);
    glGenBuffers(1, &domeVBO);
    glGenBuffers(1, &domeEBO);

    glBindVertexArray(domeVAO);
    glBindBuffer(GL_ARRAY_BUFFER, domeVBO);
    glBufferData(GL_ARRAY_BUFFER, interleaved.size() * sizeof(float), interleaved.data(), GL_STATIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, domeEBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(unsigned int), indices.data(), GL_STATIC_DRAW);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void *)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void *)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void *)(6 * sizeof(float)));
    glEnableVertexAttribArray(2);

    glBindVertexArray(0);

    domeTexture = loadTexture("Textures/cave.jpg"); // put your rocky/cave texture there
}

void renderDome(GLuint shaderProgram, const glm::mat4 &view, const glm::mat4 &projection)
{
    if (domeVAO == 0 || domeIndexCount == 0)
        return;

    glUseProgram(shaderProgram);

    // Keep depth writes OFF so the dome never blocks scene geometry
    glDepthMask(GL_FALSE);

    // Cull FRONT faces so we see the inside of the dome
    glCullFace(GL_FRONT);

    glm::mat4 model(1.0f);
    model = glm::translate(model, domeCenter);
    // (radius baked into vertices; no scale here)

    glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "model"), 1, GL_FALSE, glm::value_ptr(model));
    glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "view"), 1, GL_FALSE, glm::value_ptr(view));
    glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "projection"), 1, GL_FALSE, glm::value_ptr(projection));

    // No emissive, no hit flash
    glUniform1f(glGetUniformLocation(shaderProgram, "isFireball"), 0.0f);
    glUniform1f(glGetUniformLocation(shaderProgram, "hitFlashStrength"), 0.0f);

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, domeTexture);
    glUniform1i(glGetUniformLocation(shaderProgram, "texture_diffuse1"), 0);

    glBindVertexArray(domeVAO);
    glDrawElements(GL_TRIANGLES, domeIndexCount, GL_UNSIGNED_INT, 0);
    glBindVertexArray(0);

    // Restore state
    glCullFace(GL_BACK);
    glDepthMask(GL_TRUE);
}

void updateSnakeAnimation(float dt, vec3 camPos)
{
    snakeAnimationTime += dt;

    vec3 cameraGroundPos = vec3(camPos.x, 0.0f, camPos.z);
    vec3 directionToGroundCamera = normalize(cameraGroundPos - snakeBasePos);
    float slitherSpeed = 0.3f; // slower snake movement
    snakeBasePos += directionToGroundCamera * slitherSpeed * dt;
    snakeBasePos.y = 0.0f;

    float distanceToCamera = length(cameraGroundPos - snakeBasePos);
    if (distanceToCamera < 8.0f)
    {
        vec3 pushBack = normalize(snakeBasePos - cameraGroundPos) * (8.0f - distanceToCamera);
        snakeBasePos += pushBack;
        snakeBasePos.y = 0.0f;
    }

    float segmentSpacing = neckLength / SNAKE_NECK_SEGMENTS;
    for (int i = 0; i < SNAKE_NECK_SEGMENTS; i++)
    {
        float progress = (float)i / SNAKE_NECK_SEGMENTS;
        float baseGroundHeight = 0.1f;
        float maxRiseHeight = 1.8f;
        float riseStartPoint = 0.4f;
        float heightProgress = 0.0f;
        if (progress > riseStartPoint)
        {
            float adjusted = (progress - riseStartPoint) / (1.0f - riseStartPoint);
            heightProgress = adjusted * adjusted;
        }
        float waveIntensity = progress < 0.5f ? 0.05f : 0.15f;
        float slitherWave = sin(snakeAnimationTime * 2.0f + progress * 6.28f) * waveIntensity;
        float gravityEffect = 1.0f - (progress * 0.3f);
        float height = (baseGroundHeight * gravityEffect) + (heightProgress * maxRiseHeight) + slitherWave;

        vec3 forwardOffset = directionToGroundCamera * (progress * neckLength);
        vec3 rightVector = normalize(cross(directionToGroundCamera, vec3(0, 1, 0)));
        float sideMotion = sin(snakeAnimationTime * oscillationSpeed + progress * 3.14159f) * oscillationStrength;
        vec3 sideOffset = rightVector * sideMotion;

        snakeNeckSegments[i].position = vec3(
            snakeBasePos.x + forwardOffset.x + sideOffset.x,
            height,
            snakeBasePos.z + forwardOffset.z + sideOffset.z);
        if (i > 0)
        {
            vec3 dir = snakeNeckSegments[i].position - snakeNeckSegments[i - 1].position;
            if (length(dir) > 0.001f)
            {
                dir = normalize(dir);
                snakeNeckSegments[i].rotation.y = atan2(dir.x, dir.z);
                snakeNeckSegments[i].rotation.x = atan2(dir.y, sqrt(dir.x * dir.x + dir.z * dir.z));
            }
        }
    }
}

bool checkCollision(vec3 projectilePos, vec3 segmentPos, float radius)
{
    return length(projectilePos - segmentPos) < radius;
}

void playHitSound()
{
#ifdef _WIN32
    cout << "SNAKE HIT! *pain sound*" << endl;
#else
    cout << "SNAKE HIT! *pain sound*" << endl;
#endif
}

void renderDragonModel(DragonModel &model, GLuint shaderProgram, mat4 modelMatrix, mat4 view, mat4 projection, mat4 lightSpaceMatrix)
{
    if (model.VAO == 0)
    {
        cout << "ERROR: Model VAO is 0!\n";
        return;
    }
    glUseProgram(shaderProgram);
    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
    glDisable(GL_CULL_FACE);

    glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "model"), 1, GL_FALSE, value_ptr(modelMatrix));
    glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "view"), 1, GL_FALSE, value_ptr(view));
    glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "projection"), 1, GL_FALSE, value_ptr(projection));
    glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "lightSpaceMatrix"), 1, GL_FALSE, value_ptr(lightSpaceMatrix));

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, model.texture);
    glUniform1i(glGetUniformLocation(shaderProgram, "texture_diffuse1"), 0);

    glBindVertexArray(model.VAO);
    glDrawArrays(GL_TRIANGLES, 0, (GLsizei)model.vertices.size());
    glBindVertexArray(0);
}

void renderStaff(GLuint shaderProgram, mat4 view, mat4 projection, mat4 lightSpaceMatrix)
{
    if (staff.vertices.empty())
    {
        cout << "Staff has no vertices!\n";
        return;
    }

    vec3 rightVector = normalize(cross(cameraFront, cameraUp));
    vec3 staffWorldPos = cameraPos + cameraFront * 3.0f + rightVector * 1.2f + cameraUp * (-0.8f);

    vec3 shakeOffset(0.0f);
    if (staffShakeTimer > 0.0f)
    {
        float t = staffShakeTimer / staffShakeDuration;
        shakeOffset.x = (sin(staffShakeTimer * 50.0f) * staffShakeAmount * t);
        shakeOffset.y = (cos(staffShakeTimer * 60.0f) * staffShakeAmount * t);
    }

    mat4 m = translate(mat4(1.0f), staffWorldPos + shakeOffset);
    m = rotate(m, radians(cameraYaw), vec3(0, 1, 0));
    m = rotate(m, radians(cameraPitch), vec3(1, 0, 0));
    m = rotate(m, staffRotation.x, vec3(1, 0, 0));
    m = rotate(m, staffRotation.y, vec3(0, 1, 0));
    m = rotate(m, staffRotation.z, vec3(0, 0, 1));
    m = scale(m, staffScale);

    // make staff bioluminescent blue (bright but not full emissive)
    glUniform1f(glGetUniformLocation(sceneShaderProgram, "isFireball"), 0.7f); // stronger glow for bioluminescence
    renderDragonModel(staff, shaderProgram, m, view, projection, lightSpaceMatrix);
}

void renderSnake(GLuint shaderProgram, mat4 view, mat4 projection, mat4 lightSpaceMatrix)
{
    // Neck
    for (int i = 0; i < SNAKE_NECK_SEGMENTS; i++)
    {
        mat4 m(1.0f);
        m = translate(m, snakeNeckSegments[i].position);
        m = rotate(m, radians(45.0f), vec3(1, 0, 0)); // rotate fish to be horizontal
        m = rotate(m, snakeNeckSegments[i].rotation.x, vec3(1, 0, 0));
        m = rotate(m, snakeNeckSegments[i].rotation.y, vec3(0, 1, 0));
        m = rotate(m, snakeNeckSegments[i].rotation.z, vec3(0, 0, 1));
        m = scale(m, neckScale);

        // hit flash uniforms
        if (snakeHit)
        {
            glUniform3f(glGetUniformLocation(sceneShaderProgram, "hitFlashColor"), 1.0f, 0.0f, 0.0f);
            glUniform1f(glGetUniformLocation(sceneShaderProgram, "hitFlashStrength"), hitFlashTimer / hitFlashDuration);
        }
        else
        {
            glUniform1f(glGetUniformLocation(sceneShaderProgram, "hitFlashStrength"), 0.0f);
        }
        // ensure not emissive
        glUniform1f(glGetUniformLocation(sceneShaderProgram, "isFireball"), 0.0f);

        renderDragonModel(fishBody, shaderProgram, m, view, projection, lightSpaceMatrix);
    }

    // Head facing camera (direction snake is moving)
    if (SNAKE_NECK_SEGMENTS > 0)
    {
        vec3 lastSegmentPos = snakeNeckSegments[SNAKE_NECK_SEGMENTS - 1].position;
        // Calculate snake's forward direction (same as body)
        vec3 cameraGroundPos = vec3(cameraPos.x, 0.0f, cameraPos.z);
        vec3 snakeForward = normalize(cameraGroundPos - snakeBasePos);

        vec3 headToCam = normalize(vec3(cameraPos.x - lastSegmentPos.x, 0.0f, cameraPos.z - lastSegmentPos.z));
        vec3 headPos = lastSegmentPos + headToCam * 1.0f;

        // Use same approach as Test Head 2: simple camera direction calculation
        vec3 dirToCamera = normalize(cameraPos - headPos);
        float cameraYaw = atan2(dirToCamera.x, dirToCamera.z);
        float snakeYaw = atan2(snakeForward.x, snakeForward.z);
        float relativeYaw = cameraYaw - snakeYaw;

        // Normalize angle to [-π, π] range
        while (relativeYaw > M_PI)
            relativeYaw -= 2.0f * M_PI;
        while (relativeYaw < -M_PI)
            relativeYaw += 2.0f * M_PI;

        // Limit head rotation to ±10 degrees from snake's forward direction
        relativeYaw = glm::clamp(relativeYaw, radians(-10.0f), radians(10.0f));

        // Debug output
        static int debugCounter = 0;
        if (debugCounter++ % 60 == 0)
        { // print every 60 frames
            cout << "snakeYaw: " << degrees(snakeYaw) << " cameraYaw: " << degrees(cameraYaw)
                 << " relativeYaw: " << degrees(relativeYaw) << endl;
        }

        mat4 m(1.0f);
        m = translate(m, headPos);
        m = rotate(m, radians(90.0f), vec3(1, 0, 0));  // base orientation
        m = rotate(m, radians(180.0f), vec3(0, 1, 0)); // base orientation
        m = rotate(m, radians(190.0f), vec3(0, 0, 1));  // base orientation
        m = rotate(m, snakeYaw, vec3(0, 0, 1));        // first orient with snake body direction
        m = rotate(m, relativeYaw, vec3(0, 0, 1));     // then add limited head turn toward camera
        m = scale(m, headScale);

        if (snakeHit)
        {
            glUniform3f(glGetUniformLocation(sceneShaderProgram, "hitFlashColor"), 1.0f, 0.0f, 0.0f);
            glUniform1f(glGetUniformLocation(sceneShaderProgram, "hitFlashStrength"), hitFlashTimer / hitFlashDuration);
        }
        else
        {
            glUniform1f(glGetUniformLocation(sceneShaderProgram, "hitFlashStrength"), 0.0f);
        }
        glUniform1f(glGetUniformLocation(sceneShaderProgram, "isFireball"), 0.0f);

        renderDragonModel(dragonHead, shaderProgram, m, view, projection, lightSpaceMatrix);
    }

}

// ------------------------------------
// Input / callbacks
// ------------------------------------
void processInput(GLFWwindow *window)
{
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);

    static bool fKeyPressed = false;
    if (glfwGetKey(window, GLFW_KEY_F) == GLFW_PRESS && !fKeyPressed)
    {
        flightMode = !flightMode;
        if (flightMode)
        {
            cout << "Flight mode ON\n";
            cameraVelocity.y = 0.0f;
        }
        else
        {
            cout << "Flight mode OFF - gravity enabled\n";
        }
        fKeyPressed = true;
    }
    if (glfwGetKey(window, GLFW_KEY_F) == GLFW_RELEASE)
        fKeyPressed = false;

    float cameraSpeed = 5.0f * deltaTime;
    vec3 forward = cameraFront;
    vec3 right = normalize(cross(cameraFront, cameraUp));

    vec3 inputVelocity(0.0f);
    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
        inputVelocity += forward * cameraSpeed;
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
        inputVelocity -= forward * cameraSpeed;
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
        inputVelocity -= right * cameraSpeed;
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
        inputVelocity += right * cameraSpeed;

    if (flightMode)
    {
        if (glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS)
            inputVelocity += cameraUp * cameraSpeed;
        if (glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS)
            inputVelocity -= cameraUp * cameraSpeed;
        cameraPos += inputVelocity;
    }
    else
    {
        cameraVelocity.y += gravity * deltaTime;
        vec3 groundForward = normalize(vec3(forward.x, 0.0f, forward.z));
        vec3 groundRight = normalize(vec3(right.x, 0.0f, right.z));
        vec3 horizontalInput(0.0f);
        if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
            horizontalInput += groundForward * cameraSpeed;
        if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
            horizontalInput -= groundForward * cameraSpeed;
        if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
            horizontalInput -= groundRight * cameraSpeed;
        if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
            horizontalInput += groundRight * cameraSpeed;
        if (glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS && cameraPos.y <= groundLevel + 0.1f)
            cameraVelocity.y = 8.0f;
        cameraPos += horizontalInput;
        cameraPos.y += cameraVelocity.y * deltaTime;
        if (cameraPos.y < groundLevel)
        {
            cameraPos.y = groundLevel;
            cameraVelocity.y = 0.0f;
        }
    }

    // Shooting
    bool currentLeftMouse = glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS;
    if (currentLeftMouse && !leftMousePressed)
    {
        float currentTime = glfwGetTime();
        if (currentTime - lastShotTime >= shootCooldown)
        {
            vec3 projectileStart = cameraPos + cameraFront * 1.0f;
            vec3 projectileVelocity = cameraFront * 20.0f;
            projectileList.push_back(Projectile(projectileStart, projectileVelocity));
            lastShotTime = currentTime;
            staffShakeTimer = staffShakeDuration;
        }
    }
    leftMousePressed = currentLeftMouse;

    if (staffShakeTimer > 0.0f)
    {
        staffShakeTimer -= deltaTime;
        if (staffShakeTimer < 0.0f)
            staffShakeTimer = 0.0f;
    }

    // Update projectiles and collisions
    for (auto it = projectileList.begin(); it != projectileList.end();)
    {
        it->Update(deltaTime);
        vec3 projPos = it->getPosition();

        bool hitSnakeSeg = false;
        for (int i = 0; i < SNAKE_NECK_SEGMENTS; i++)
        {
            if (checkCollision(projPos, snakeNeckSegments[i].position, 1.0f))
            {
                hitSnakeSeg = true;
                break;
            }
        }
        if (!hitSnakeSeg && SNAKE_NECK_SEGMENTS > 0)
        {
            vec3 headPos = snakeNeckSegments[SNAKE_NECK_SEGMENTS - 1].position;
            if (checkCollision(projPos, headPos, 1.5f))
                hitSnakeSeg = true;
        }

        if (hitSnakeSeg)
        {
            snakeHit = true;
            hitFlashTimer = hitFlashDuration;
            playHitSound();
            it = projectileList.erase(it);
        }
        else if (length(projPos - cameraPos) > 50.0f)
        {
            it = projectileList.erase(it);
        }
        else
            ++it;
    }

    if (hitFlashTimer > 0.0f)
    {
        hitFlashTimer -= deltaTime;
        if (hitFlashTimer <= 0.0f)
        {
            hitFlashTimer = 0.0f;
            snakeHit = false;
        }
    }
}

void mouse_callback(GLFWwindow *window, double xpos, double ypos)
{
    if (firstMouse)
    {
        lastX = (float)xpos;
        lastY = (float)ypos;
        firstMouse = false;
    }
    float xoffset = (float)xpos - lastX;
    float yoffset = lastY - (float)ypos;
    lastX = (float)xpos;
    lastY = (float)ypos;
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

void key_callback(GLFWwindow *window, int key, int scancode, int action, int mode)
{
    if (key >= 0 && key < 1024)
    {
        if (action == GLFW_PRESS)
            keys[key] = true;
        else if (action == GLFW_RELEASE)
            keys[key] = false;
    }
}

// ------------------------------------
// Main
// ------------------------------------
int main()
{
    if (!glfwInit())
    {
        cout << "Failed to initialize GLFW\n";
        return -1;
    }
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    GLFWwindow *window = glfwCreateWindow(WIDTH, HEIGHT, "Wizard Shooter", NULL, NULL);
    if (!window)
    {
        cout << "Failed to create GLFW window\n";
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);
    glfwSetCursorPosCallback(window, mouse_callback);
    glfwSetKeyCallback(window, key_callback);
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

    glewExperimental = GL_TRUE;
    if (glewInit() != GLEW_OK)
    {
        cout << "Failed to initialize GLEW\n";
        return -1;
    }

    glEnable(GL_DEPTH_TEST);
    glEnable(GL_CULL_FACE);
    glCullFace(GL_BACK);
    glFrontFace(GL_CCW);
    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

    sceneShaderProgram = createShaderProgram("Shaders/scene_vertex_textured.glsl", "Shaders/scene_fragment_textured.glsl");
    shadowShaderProgram = createShaderProgram("Shaders/shadow_vertex.glsl", "Shaders/shadow_fragment.glsl");

    setupShadowMapping();
    setupGround();
    setupCube();
    setupSphere();
    setupSnakeModels();
    setupDomeGeodesic(/*subdivLevel=*/1, /*radius=*/32.0f, /*tile=*/2.0f);
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f); // pure black for cave atmosphere

    while (!glfwWindowShouldClose(window))
    {
        float currentFrame = (float)glfwGetTime();
        deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;

        processInput(window);
        updateSnakeAnimation(deltaTime, cameraPos);

        // animate sun around origin
        lightPos.x = 15.0f * cos(currentFrame * 0.5f);
        lightPos.z = 15.0f * sin(currentFrame * 0.5f);

        // ---------- Shadow pass ----------
        glViewport(0, 0, SHADOW_WIDTH, SHADOW_HEIGHT);
        glBindFramebuffer(GL_FRAMEBUFFER, depthMapFBO);
        glClear(GL_DEPTH_BUFFER_BIT);

        mat4 lightProjection = ortho(-20.0f, 20.0f, -20.0f, 20.0f, 1.0f, 50.0f);
        mat4 lightView = lookAt(lightPos, vec3(0.0f), vec3(0, 1, 0));
        mat4 lightSpaceMatrix = lightProjection * lightView;

        glUseProgram(shadowShaderProgram);
        glUniformMatrix4fv(glGetUniformLocation(shadowShaderProgram, "lightSpaceMatrix"), 1, GL_FALSE, value_ptr(lightSpaceMatrix));

        mat4 groundModel(1.0f);
        // simple shadow pass for ground: reuse ground VAO with identity model
        glUniformMatrix4fv(glGetUniformLocation(shadowShaderProgram, "model"), 1, GL_FALSE, value_ptr(groundModel));
        glBindVertexArray(groundVAO);
        glDrawArrays(GL_TRIANGLES, 0, 6);

        // snake body
        for (int i = 0; i < SNAKE_NECK_SEGMENTS; i++)
        {
            mat4 bodyModel(1.0f);
            bodyModel = translate(bodyModel, snakeNeckSegments[i].position);
            bodyModel = scale(bodyModel, neckScale);
            glUniformMatrix4fv(glGetUniformLocation(shadowShaderProgram, "model"), 1, GL_FALSE, value_ptr(bodyModel));
            glBindVertexArray(fishBody.VAO);
            glDrawArrays(GL_TRIANGLES, 0, (GLsizei)fishBody.vertices.size());
        }
        // head
        if (SNAKE_NECK_SEGMENTS > 0)
        {
            mat4 headModel(1.0f);
            headModel = translate(headModel, snakeNeckSegments[SNAKE_NECK_SEGMENTS - 1].position);
            headModel = scale(headModel, headScale);
            glUniformMatrix4fv(glGetUniformLocation(shadowShaderProgram, "model"), 1, GL_FALSE, value_ptr(headModel));
            glBindVertexArray(dragonHead.VAO);
            glDrawArrays(GL_TRIANGLES, 0, (GLsizei)dragonHead.vertices.size());
        }

        glBindFramebuffer(GL_FRAMEBUFFER, 0);

        // ---------- Scene pass ----------
        glViewport(0, 0, WIDTH, HEIGHT);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        mat4 projection = perspective(radians(45.0f), (float)WIDTH / (float)HEIGHT, 0.1f, 100.0f);
        mat4 view = lookAt(cameraPos, cameraPos + cameraFront, cameraUp);

        glUseProgram(sceneShaderProgram);
        glUniformMatrix4fv(glGetUniformLocation(sceneShaderProgram, "projection"), 1, GL_FALSE, value_ptr(projection));
        glUniformMatrix4fv(glGetUniformLocation(sceneShaderProgram, "view"), 1, GL_FALSE, value_ptr(view));
        glUniformMatrix4fv(glGetUniformLocation(sceneShaderProgram, "lightSpaceMatrix"), 1, GL_FALSE, value_ptr(lightSpaceMatrix));

        glUniform3fv(glGetUniformLocation(sceneShaderProgram, "lightPos"), 1, value_ptr(lightPos));
        glUniform3fv(glGetUniformLocation(sceneShaderProgram, "lightColor"), 1, value_ptr(lightColor));
        glUniform3fv(glGetUniformLocation(sceneShaderProgram, "viewPos"), 1, value_ptr(cameraPos));

        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, depthMap);
        glUniform1i(glGetUniformLocation(sceneShaderProgram, "shadowMap"), 1);
        // Dark cave atmosphere - some ambient light for visibility
        glUniform1f(glGetUniformLocation(sceneShaderProgram, "uAmbient"), 0.3f); // more ambient for cave
        // (If you didn’t add uAmbient earlier, either add it as in my prior message or skip. Cave still works without it.)

        // ---- upload fireball point lights (single pass) ----
        {
            vector<vec3> fbPos;
            fbPos.reserve(MAX_FIREBALLS);
            vector<vec3> fbCol;
            fbCol.reserve(MAX_FIREBALLS);

            // Add staff blue light with reduced intensity for smaller effective radius
            vec3 rightVector = normalize(cross(cameraFront, cameraUp));
            vec3 staffWorldPos = cameraPos + cameraFront * 3.0f + rightVector * 1.2f + cameraUp * (-0.8f);
            fbPos.push_back(staffWorldPos);
            fbCol.push_back(vec3(0.4f, 0.8f, 1.5f)); // dimmer blue for smaller effective range

            // Add fireball lights
            for (auto &p : projectileList)
            {
                fbPos.push_back(p.getPosition());
                fbCol.push_back(vec3(4.5f, 2.2f, 1.2f)); // bright orange
                if ((int)fbPos.size() == MAX_FIREBALLS)
                    break;
            }
            glUniform1i(glGetUniformLocation(sceneShaderProgram, "uFireballCount"), (int)fbPos.size());
            if (!fbPos.empty())
            {
                glUniform3fv(glGetUniformLocation(sceneShaderProgram, "uFireballPos"), (GLsizei)fbPos.size(), &fbPos[0].x);
                glUniform3fv(glGetUniformLocation(sceneShaderProgram, "uFireballColor"), (GLsizei)fbCol.size(), &fbCol[0].x);
            }
            glUniform1f(glGetUniformLocation(sceneShaderProgram, "uFireballRadius"), 3.5f); // back to single radius
        }

        // Ensure default flags for non-fireball draws
        glUniform1f(glGetUniformLocation(sceneShaderProgram, "hitFlashStrength"), 0.0f);
        glUniform1f(glGetUniformLocation(sceneShaderProgram, "isFireball"), 0.0f);

        // Draw world
        renderGround(sceneShaderProgram, groundModel, view, projection);
        renderSnake(sceneShaderProgram, view, projection, lightSpaceMatrix);
        // reset flash for other objects
        glUniform1f(glGetUniformLocation(sceneShaderProgram, "hitFlashStrength"), 0.0f);
        glUniform1f(glGetUniformLocation(sceneShaderProgram, "isFireball"), 0.0f);
        renderStaff(sceneShaderProgram, view, projection, lightSpaceMatrix);

        // Draw fireball meshes (emissive spheres)
        for (auto &projectile : projectileList)
        {
            projectile.Draw(sceneShaderProgram, view, projection);
        }

        // Draw dome LAST so it appears behind everything (depth disabled)
        renderDomeGeodesic(sceneShaderProgram, view, projection);

        // NOTE: removed the old additive re-render pass entirely (not needed)

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    // Cleanup
    glDeleteVertexArrays(1, &groundVAO);
    glDeleteBuffers(1, &groundVBO);
    glDeleteProgram(sceneShaderProgram);
    glDeleteProgram(shadowShaderProgram);
    glDeleteFramebuffers(1, &depthMapFBO);
    glDeleteTextures(1, &depthMap);

    glfwTerminate();
    return 0;
}