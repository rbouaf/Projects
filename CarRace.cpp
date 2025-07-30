#include <iostream>
#include <list>
#include <algorithm>
#include <string>
#include <vector>

#define GLEW_STATIC 1
#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/common.hpp>

#define STB_IMAGE_IMPLEMENTATION
#include <stb/stb_image.h>

using namespace glm;
using namespace std;

struct TexturedColoredVertex
{
    TexturedColoredVertex(vec3 _position, vec3 _color, vec2 _uv, vec3 _normal)
    : position(_position), color(_color), uv(_uv), normal(_normal) {}

    vec3 position;
    vec3 color;
    vec2 uv;
    vec3 normal;
};

void addCube(vector<TexturedColoredVertex>& vertices, vec3 center, vec3 size, vec3 color, float rotationY = 0.0f, float rotationZ = 0.0f) {
    float x = size.x / 2.0f, y = size.y / 2.0f, z = size.z / 2.0f;

    float cosY = cos(radians(rotationY));
    float sinY = sin(radians(rotationY));
    float cosZ = cos(radians(rotationZ));
    float sinZ = sin(radians(rotationZ));

    auto rotatePoint = [cosY, sinY, cosZ, sinZ](vec3 point) -> vec3 {

        vec3 rotY = vec3(
            point.x * cosY - point.z * sinY,
            point.y,
            point.x * sinY + point.z * cosY
        );

        return vec3(
            rotY.x * cosZ - rotY.y * sinZ,
            rotY.x * sinZ + rotY.y * cosZ,
            rotY.z
        );
    };

    auto rotateNormal = [cosY, sinY, cosZ, sinZ](vec3 normal) -> vec3 {

        vec3 rotY = vec3(
            normal.x * cosY - normal.z * sinY,
            normal.y,
            normal.x * sinY + normal.z * cosY
        );

        return vec3(
            rotY.x * cosZ - rotY.y * sinZ,
            rotY.x * sinZ + rotY.y * cosZ,
            rotY.z
        );
    };

    vertices.push_back(TexturedColoredVertex(center + rotatePoint(vec3(-x, -y,  z)), color, vec2(0.0f, 0.0f), rotateNormal(vec3(0.0f, 0.0f, 1.0f))));
    vertices.push_back(TexturedColoredVertex(center + rotatePoint(vec3( x, -y,  z)), color, vec2(1.0f, 0.0f), rotateNormal(vec3(0.0f, 0.0f, 1.0f))));
    vertices.push_back(TexturedColoredVertex(center + rotatePoint(vec3( x,  y,  z)), color, vec2(1.0f, 1.0f), rotateNormal(vec3(0.0f, 0.0f, 1.0f))));
    vertices.push_back(TexturedColoredVertex(center + rotatePoint(vec3(-x, -y,  z)), color, vec2(0.0f, 0.0f), rotateNormal(vec3(0.0f, 0.0f, 1.0f))));
    vertices.push_back(TexturedColoredVertex(center + rotatePoint(vec3( x,  y,  z)), color, vec2(1.0f, 1.0f), rotateNormal(vec3(0.0f, 0.0f, 1.0f))));
    vertices.push_back(TexturedColoredVertex(center + rotatePoint(vec3(-x,  y,  z)), color, vec2(0.0f, 1.0f), rotateNormal(vec3(0.0f, 0.0f, 1.0f))));

    vertices.push_back(TexturedColoredVertex(center + rotatePoint(vec3(-x, -y, -z)), color, vec2(1.0f, 0.0f), rotateNormal(vec3(0.0f, 0.0f, -1.0f))));
    vertices.push_back(TexturedColoredVertex(center + rotatePoint(vec3( x,  y, -z)), color, vec2(0.0f, 1.0f), rotateNormal(vec3(0.0f, 0.0f, -1.0f))));
    vertices.push_back(TexturedColoredVertex(center + rotatePoint(vec3( x, -y, -z)), color, vec2(0.0f, 0.0f), rotateNormal(vec3(0.0f, 0.0f, -1.0f))));
    vertices.push_back(TexturedColoredVertex(center + rotatePoint(vec3(-x, -y, -z)), color, vec2(1.0f, 0.0f), rotateNormal(vec3(0.0f, 0.0f, -1.0f))));
    vertices.push_back(TexturedColoredVertex(center + rotatePoint(vec3(-x,  y, -z)), color, vec2(1.0f, 1.0f), rotateNormal(vec3(0.0f, 0.0f, -1.0f))));
    vertices.push_back(TexturedColoredVertex(center + rotatePoint(vec3( x,  y, -z)), color, vec2(0.0f, 1.0f), rotateNormal(vec3(0.0f, 0.0f, -1.0f))));

    vertices.push_back(TexturedColoredVertex(center + rotatePoint(vec3(-x, -y, -z)), color, vec2(0.0f, 0.0f), rotateNormal(vec3(-1.0f, 0.0f, 0.0f))));
    vertices.push_back(TexturedColoredVertex(center + rotatePoint(vec3(-x, -y,  z)), color, vec2(1.0f, 0.0f), rotateNormal(vec3(-1.0f, 0.0f, 0.0f))));
    vertices.push_back(TexturedColoredVertex(center + rotatePoint(vec3(-x,  y,  z)), color, vec2(1.0f, 1.0f), rotateNormal(vec3(-1.0f, 0.0f, 0.0f))));
    vertices.push_back(TexturedColoredVertex(center + rotatePoint(vec3(-x, -y, -z)), color, vec2(0.0f, 0.0f), rotateNormal(vec3(-1.0f, 0.0f, 0.0f))));
    vertices.push_back(TexturedColoredVertex(center + rotatePoint(vec3(-x,  y,  z)), color, vec2(1.0f, 1.0f), rotateNormal(vec3(-1.0f, 0.0f, 0.0f))));
    vertices.push_back(TexturedColoredVertex(center + rotatePoint(vec3(-x,  y, -z)), color, vec2(0.0f, 1.0f), rotateNormal(vec3(-1.0f, 0.0f, 0.0f))));

    vertices.push_back(TexturedColoredVertex(center + rotatePoint(vec3( x, -y, -z)), color, vec2(1.0f, 0.0f), rotateNormal(vec3(1.0f, 0.0f, 0.0f))));
    vertices.push_back(TexturedColoredVertex(center + rotatePoint(vec3( x,  y,  z)), color, vec2(0.0f, 1.0f), rotateNormal(vec3(1.0f, 0.0f, 0.0f))));
    vertices.push_back(TexturedColoredVertex(center + rotatePoint(vec3( x, -y,  z)), color, vec2(0.0f, 0.0f), rotateNormal(vec3(1.0f, 0.0f, 0.0f))));
    vertices.push_back(TexturedColoredVertex(center + rotatePoint(vec3( x, -y, -z)), color, vec2(1.0f, 0.0f), rotateNormal(vec3(1.0f, 0.0f, 0.0f))));
    vertices.push_back(TexturedColoredVertex(center + rotatePoint(vec3( x,  y, -z)), color, vec2(1.0f, 1.0f), rotateNormal(vec3(1.0f, 0.0f, 0.0f))));
    vertices.push_back(TexturedColoredVertex(center + rotatePoint(vec3( x,  y,  z)), color, vec2(0.0f, 1.0f), rotateNormal(vec3(1.0f, 0.0f, 0.0f))));

    vertices.push_back(TexturedColoredVertex(center + rotatePoint(vec3(-x, -y, -z)), color, vec2(0.0f, 1.0f), rotateNormal(vec3(0.0f, -1.0f, 0.0f))));
    vertices.push_back(TexturedColoredVertex(center + rotatePoint(vec3( x, -y, -z)), color, vec2(1.0f, 1.0f), rotateNormal(vec3(0.0f, -1.0f, 0.0f))));
    vertices.push_back(TexturedColoredVertex(center + rotatePoint(vec3( x, -y,  z)), color, vec2(1.0f, 0.0f), rotateNormal(vec3(0.0f, -1.0f, 0.0f))));
    vertices.push_back(TexturedColoredVertex(center + rotatePoint(vec3(-x, -y, -z)), color, vec2(0.0f, 1.0f), rotateNormal(vec3(0.0f, -1.0f, 0.0f))));
    vertices.push_back(TexturedColoredVertex(center + rotatePoint(vec3( x, -y,  z)), color, vec2(1.0f, 0.0f), rotateNormal(vec3(0.0f, -1.0f, 0.0f))));
    vertices.push_back(TexturedColoredVertex(center + rotatePoint(vec3(-x, -y,  z)), color, vec2(0.0f, 0.0f), rotateNormal(vec3(0.0f, -1.0f, 0.0f))));

    vertices.push_back(TexturedColoredVertex(center + rotatePoint(vec3(-x,  y, -z)), color, vec2(0.0f, 1.0f), rotateNormal(vec3(0.0f, 1.0f, 0.0f))));
    vertices.push_back(TexturedColoredVertex(center + rotatePoint(vec3( x,  y,  z)), color, vec2(1.0f, 0.0f), rotateNormal(vec3(0.0f, 1.0f, 0.0f))));
    vertices.push_back(TexturedColoredVertex(center + rotatePoint(vec3( x,  y, -z)), color, vec2(1.0f, 1.0f), rotateNormal(vec3(0.0f, 1.0f, 0.0f))));
    vertices.push_back(TexturedColoredVertex(center + rotatePoint(vec3(-x,  y, -z)), color, vec2(0.0f, 1.0f), rotateNormal(vec3(0.0f, 1.0f, 0.0f))));
    vertices.push_back(TexturedColoredVertex(center + rotatePoint(vec3(-x,  y,  z)), color, vec2(0.0f, 0.0f), rotateNormal(vec3(0.0f, 1.0f, 0.0f))));
    vertices.push_back(TexturedColoredVertex(center + rotatePoint(vec3( x,  y,  z)), color, vec2(1.0f, 0.0f), rotateNormal(vec3(0.0f, 1.0f, 0.0f))));
}

vector<TexturedColoredVertex> generateCarBodyGeometry() {
    vector<TexturedColoredVertex> vertices;
    vec3 carColor(1.0f, 1.0f, 1.0f); 

    addCube(vertices, vec3(0.0f, 0.0f, 0.0f), vec3(1.4f, 0.3f, 0.6f), carColor);

    return vertices;
}

vector<TexturedColoredVertex> generateCarWindshieldGeometry() {
    vector<TexturedColoredVertex> vertices;
    vec3 windowColor(1.0f, 1.0f, 1.0f); 

    addCube(vertices, vec3(-0.1f, 0.25f, 0.0f), vec3(0.8f, 0.2f, 0.5f), windowColor);

    return vertices;
}

vector<TexturedColoredVertex> generateCarWheelsGeometry() {
    vector<TexturedColoredVertex> vertices;
    vec3 wheelColor(0.1f, 0.1f, 0.1f); 

    float wheelSize = 0.15f;
    float wheelWidth = 0.08f;
    addCube(vertices, vec3( 0.5f, -0.25f,  0.4f), vec3(wheelWidth, wheelSize, wheelSize), wheelColor, 0.0f, 90.0f); 
    addCube(vertices, vec3( 0.5f, -0.25f, -0.4f), vec3(wheelWidth, wheelSize, wheelSize), wheelColor, 0.0f, 90.0f); 
    addCube(vertices, vec3(-0.5f, -0.25f,  0.4f), vec3(wheelWidth, wheelSize, wheelSize), wheelColor, 0.0f, 90.0f); 
    addCube(vertices, vec3(-0.5f, -0.25f, -0.4f), vec3(wheelWidth, wheelSize, wheelSize), wheelColor, 0.0f, 90.0f); 

    return vertices;
}

vector<TexturedColoredVertex> generateGrassFieldGeometry() {
    vector<TexturedColoredVertex> vertices;
    vec3 grassColor(1.0f, 1.0f, 1.0f); 

    float size = 25.0f; 
    float tileRepeat = 25.0f; 

    vertices.push_back(TexturedColoredVertex(vec3(-size, 0.0f, -size), grassColor, vec2(0.0f, tileRepeat), vec3(0.0f, 1.0f, 0.0f)));
    vertices.push_back(TexturedColoredVertex(vec3( size, 0.0f,  size), grassColor, vec2(tileRepeat, 0.0f), vec3(0.0f, 1.0f, 0.0f)));
    vertices.push_back(TexturedColoredVertex(vec3( size, 0.0f, -size), grassColor, vec2(tileRepeat, tileRepeat), vec3(0.0f, 1.0f, 0.0f)));
    vertices.push_back(TexturedColoredVertex(vec3(-size, 0.0f, -size), grassColor, vec2(0.0f, tileRepeat), vec3(0.0f, 1.0f, 0.0f)));
    vertices.push_back(TexturedColoredVertex(vec3(-size, 0.0f,  size), grassColor, vec2(0.0f, 0.0f), vec3(0.0f, 1.0f, 0.0f)));
    vertices.push_back(TexturedColoredVertex(vec3( size, 0.0f,  size), grassColor, vec2(tileRepeat, 0.0f), vec3(0.0f, 1.0f, 0.0f)));

    return vertices;
}

const TexturedColoredVertex cubeVertexArray[] = {
    TexturedColoredVertex(vec3(-0.5f,-0.5f,-0.5f), vec3(1.0f, 1.0f, 1.0f), vec2(0.0f, 0.0f), vec3(-1.0f, 0.0f, 0.0f)),
    TexturedColoredVertex(vec3(-0.5f,-0.5f, 0.5f), vec3(1.0f, 1.0f, 1.0f), vec2(0.0f, 1.0f), vec3(-1.0f, 0.0f, 0.0f)),
    TexturedColoredVertex(vec3(-0.5f, 0.5f, 0.5f), vec3(1.0f, 1.0f, 1.0f), vec2(1.0f, 1.0f), vec3(-1.0f, 0.0f, 0.0f)),
    TexturedColoredVertex(vec3(-0.5f,-0.5f,-0.5f), vec3(1.0f, 1.0f, 1.0f), vec2(0.0f, 0.0f), vec3(-1.0f, 0.0f, 0.0f)),
    TexturedColoredVertex(vec3(-0.5f, 0.5f, 0.5f), vec3(1.0f, 1.0f, 1.0f), vec2(1.0f, 1.0f), vec3(-1.0f, 0.0f, 0.0f)),
    TexturedColoredVertex(vec3(-0.5f, 0.5f,-0.5f), vec3(1.0f, 1.0f, 1.0f), vec2(1.0f, 0.0f), vec3(-1.0f, 0.0f, 0.0f)),

    TexturedColoredVertex(vec3( 0.5f, 0.5f,-0.5f), vec3(1.0f, 1.0f, 1.0f), vec2(1.0f, 1.0f), vec3(0.0f, 0.0f, -1.0f)),
    TexturedColoredVertex(vec3(-0.5f,-0.5f,-0.5f), vec3(1.0f, 1.0f, 1.0f), vec2(0.0f, 0.0f), vec3(0.0f, 0.0f, -1.0f)),
    TexturedColoredVertex(vec3(-0.5f, 0.5f,-0.5f), vec3(1.0f, 1.0f, 1.0f), vec2(0.0f, 1.0f), vec3(0.0f, 0.0f, -1.0f)),
    TexturedColoredVertex(vec3( 0.5f, 0.5f,-0.5f), vec3(1.0f, 1.0f, 1.0f), vec2(1.0f, 1.0f), vec3(0.0f, 0.0f, -1.0f)),
    TexturedColoredVertex(vec3( 0.5f,-0.5f,-0.5f), vec3(1.0f, 1.0f, 1.0f), vec2(1.0f, 0.0f), vec3(0.0f, 0.0f, -1.0f)),
    TexturedColoredVertex(vec3(-0.5f,-0.5f,-0.5f), vec3(1.0f, 1.0f, 1.0f), vec2(0.0f, 0.0f), vec3(0.0f, 0.0f, -1.0f)),

    TexturedColoredVertex(vec3( 0.5f,-0.5f, 0.5f), vec3(1.0f, 1.0f, 1.0f), vec2(1.0f, 1.0f), vec3(0.0f, -1.0f, 0.0f)),
    TexturedColoredVertex(vec3(-0.5f,-0.5f,-0.5f), vec3(1.0f, 1.0f, 1.0f), vec2(0.0f, 0.0f), vec3(0.0f, -1.0f, 0.0f)),
    TexturedColoredVertex(vec3( 0.5f,-0.5f,-0.5f), vec3(1.0f, 1.0f, 1.0f), vec2(1.0f, 0.0f), vec3(0.0f, -1.0f, 0.0f)),
    TexturedColoredVertex(vec3( 0.5f,-0.5f, 0.5f), vec3(1.0f, 1.0f, 1.0f), vec2(1.0f, 1.0f), vec3(0.0f, -1.0f, 0.0f)),
    TexturedColoredVertex(vec3(-0.5f,-0.5f, 0.5f), vec3(1.0f, 1.0f, 1.0f), vec2(0.0f, 1.0f), vec3(0.0f, -1.0f, 0.0f)),
    TexturedColoredVertex(vec3(-0.5f,-0.5f,-0.5f), vec3(1.0f, 1.0f, 1.0f), vec2(0.0f, 0.0f), vec3(0.0f, -1.0f, 0.0f)),

    TexturedColoredVertex(vec3(-0.5f, 0.5f, 0.5f), vec3(1.0f, 1.0f, 1.0f), vec2(0.0f, 1.0f), vec3(0.0f, 0.0f, 1.0f)),
    TexturedColoredVertex(vec3(-0.5f,-0.5f, 0.5f), vec3(1.0f, 1.0f, 1.0f), vec2(0.0f, 0.0f), vec3(0.0f, 0.0f, 1.0f)),
    TexturedColoredVertex(vec3( 0.5f,-0.5f, 0.5f), vec3(1.0f, 1.0f, 1.0f), vec2(1.0f, 0.0f), vec3(0.0f, 0.0f, 1.0f)),
    TexturedColoredVertex(vec3( 0.5f, 0.5f, 0.5f), vec3(1.0f, 1.0f, 1.0f), vec2(1.0f, 1.0f), vec3(0.0f, 0.0f, 1.0f)),
    TexturedColoredVertex(vec3(-0.5f, 0.5f, 0.5f), vec3(1.0f, 1.0f, 1.0f), vec2(0.0f, 1.0f), vec3(0.0f, 0.0f, 1.0f)),
    TexturedColoredVertex(vec3( 0.5f,-0.5f, 0.5f), vec3(1.0f, 1.0f, 1.0f), vec2(1.0f, 0.0f), vec3(0.0f, 0.0f, 1.0f)),

    TexturedColoredVertex(vec3( 0.5f, 0.5f, 0.5f), vec3(1.0f, 1.0f, 1.0f), vec2(1.0f, 1.0f), vec3(1.0f, 0.0f, 0.0f)),
    TexturedColoredVertex(vec3( 0.5f,-0.5f,-0.5f), vec3(1.0f, 1.0f, 1.0f), vec2(0.0f, 0.0f), vec3(1.0f, 0.0f, 0.0f)),
    TexturedColoredVertex(vec3( 0.5f, 0.5f,-0.5f), vec3(1.0f, 1.0f, 1.0f), vec2(1.0f, 0.0f), vec3(1.0f, 0.0f, 0.0f)),
    TexturedColoredVertex(vec3( 0.5f,-0.5f,-0.5f), vec3(1.0f, 1.0f, 1.0f), vec2(0.0f, 0.0f), vec3(1.0f, 0.0f, 0.0f)),
    TexturedColoredVertex(vec3( 0.5f, 0.5f, 0.5f), vec3(1.0f, 1.0f, 1.0f), vec2(1.0f, 1.0f), vec3(1.0f, 0.0f, 0.0f)),
    TexturedColoredVertex(vec3( 0.5f,-0.5f, 0.5f), vec3(1.0f, 1.0f, 1.0f), vec2(0.0f, 1.0f), vec3(1.0f, 0.0f, 0.0f)),

    TexturedColoredVertex(vec3( 0.5f, 0.5f, 0.5f), vec3(1.0f, 1.0f, 1.0f), vec2(1.0f, 1.0f), vec3(0.0f, 1.0f, 0.0f)),
    TexturedColoredVertex(vec3( 0.5f, 0.5f,-0.5f), vec3(1.0f, 1.0f, 1.0f), vec2(1.0f, 0.0f), vec3(0.0f, 1.0f, 0.0f)),
    TexturedColoredVertex(vec3(-0.5f, 0.5f,-0.5f), vec3(1.0f, 1.0f, 1.0f), vec2(0.0f, 0.0f), vec3(0.0f, 1.0f, 0.0f)),
    TexturedColoredVertex(vec3( 0.5f, 0.5f, 0.5f), vec3(1.0f, 1.0f, 1.0f), vec2(1.0f, 1.0f), vec3(0.0f, 1.0f, 0.0f)),
    TexturedColoredVertex(vec3(-0.5f, 0.5f,-0.5f), vec3(1.0f, 1.0f, 1.0f), vec2(0.0f, 0.0f), vec3(0.0f, 1.0f, 0.0f)),
    TexturedColoredVertex(vec3(-0.5f, 0.5f, 0.5f), vec3(1.0f, 1.0f, 1.0f), vec2(0.0f, 1.0f), vec3(0.0f, 1.0f, 0.0f))
};

vector<TexturedColoredVertex> generateCircularTrack(float innerRadius, float outerRadius, int segments)
{
    vector<TexturedColoredVertex> vertices;
    vec3 trackColor(0.3f, 0.3f, 0.3f); 

    for(int i = 0; i < segments; i++)
    {
        float angle1 = 2.0f * 3.14159f * i / segments;
        float angle2 = 2.0f * 3.14159f * (i + 1) / segments;

        vec3 inner1(innerRadius * cos(angle1), 0.0f, innerRadius * sin(angle1));
        vec3 outer1(outerRadius * cos(angle1), 0.0f, outerRadius * sin(angle1));
        vec3 inner2(innerRadius * cos(angle2), 0.0f, innerRadius * sin(angle2));
        vec3 outer2(outerRadius * cos(angle2), 0.0f, outerRadius * sin(angle2));

        vec3 normal(0.0f, 1.0f, 0.0f); 

        vertices.push_back(TexturedColoredVertex(inner1, trackColor, vec2(0.0f, 0.0f), normal));
        vertices.push_back(TexturedColoredVertex(outer1, trackColor, vec2(1.0f, 0.0f), normal));
        vertices.push_back(TexturedColoredVertex(inner2, trackColor, vec2(0.0f, 1.0f), normal));

        vertices.push_back(TexturedColoredVertex(outer1, trackColor, vec2(1.0f, 0.0f), normal));
        vertices.push_back(TexturedColoredVertex(outer2, trackColor, vec2(1.0f, 1.0f), normal));
        vertices.push_back(TexturedColoredVertex(inner2, trackColor, vec2(0.0f, 1.0f), normal));
    }

    return vertices;
}

GLuint loadTexture(const char *filename);
const char* getLightingVertexShaderSource();
const char* getLightingFragmentShaderSource();
int compileAndLinkShaders(const char* vertexShaderSource, const char* fragmentShaderSource);

void setProjectionMatrix(int shaderProgram, mat4 projectionMatrix)
{
    glUseProgram(shaderProgram);
    GLuint projectionMatrixLocation = glGetUniformLocation(shaderProgram, "projectionMatrix");
    glUniformMatrix4fv(projectionMatrixLocation, 1, GL_FALSE, &projectionMatrix[0][0]);
}

void setViewMatrix(int shaderProgram, mat4 viewMatrix)
{
    glUseProgram(shaderProgram);
    GLuint viewMatrixLocation = glGetUniformLocation(shaderProgram, "viewMatrix");
    glUniformMatrix4fv(viewMatrixLocation, 1, GL_FALSE, &viewMatrix[0][0]);
}

void setWorldMatrix(int shaderProgram, mat4 worldMatrix)
{
    glUseProgram(shaderProgram);
    GLuint worldMatrixLocation = glGetUniformLocation(shaderProgram, "worldMatrix");
    glUniformMatrix4fv(worldMatrixLocation, 1, GL_FALSE, &worldMatrix[0][0]);
}

int createVertexArrayObject(const TexturedColoredVertex* vertexArray, int arraySize)
{
    GLuint vertexArrayObject;
    glGenVertexArrays(1, &vertexArrayObject);
    glBindVertexArray(vertexArrayObject);

    GLuint vertexBufferObject;
    glGenBuffers(1, &vertexBufferObject);
    glBindBuffer(GL_ARRAY_BUFFER, vertexBufferObject);
    glBufferData(GL_ARRAY_BUFFER, arraySize, vertexArray, GL_STATIC_DRAW);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(TexturedColoredVertex), (void*)0);
    glEnableVertexAttribArray(0);

    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(TexturedColoredVertex), (void*)sizeof(vec3));
    glEnableVertexAttribArray(1);

    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(TexturedColoredVertex), (void*)(2*sizeof(vec3)));
    glEnableVertexAttribArray(2);

    glVertexAttribPointer(3, 3, GL_FLOAT, GL_FALSE, sizeof(TexturedColoredVertex), (void*)(2*sizeof(vec3) + sizeof(vec2)));
    glEnableVertexAttribArray(3);

    return vertexArrayObject;
}

int createVertexArrayObjectFromVector(const vector<TexturedColoredVertex>& vertices)
{
    GLuint vertexArrayObject;
    glGenVertexArrays(1, &vertexArrayObject);
    glBindVertexArray(vertexArrayObject);

    GLuint vertexBufferObject;
    glGenBuffers(1, &vertexBufferObject);
    glBindBuffer(GL_ARRAY_BUFFER, vertexBufferObject);
    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(TexturedColoredVertex), vertices.data(), GL_STATIC_DRAW);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(TexturedColoredVertex), (void*)0);
    glEnableVertexAttribArray(0);

    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(TexturedColoredVertex), (void*)sizeof(vec3));
    glEnableVertexAttribArray(1);

    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(TexturedColoredVertex), (void*)(2*sizeof(vec3)));
    glEnableVertexAttribArray(2);

    glVertexAttribPointer(3, 3, GL_FLOAT, GL_FALSE, sizeof(TexturedColoredVertex), (void*)(2*sizeof(vec3) + sizeof(vec2)));
    glEnableVertexAttribArray(3);

    return vertexArrayObject;
}

int main(int argc, char* argv[])
{
    glfwInit();

#if defined(PLATFORM_OSX)
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 2);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#else
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 2);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 1);
#endif

    GLFWwindow* window = glfwCreateWindow(1200, 800, "Car Race with Dynamic Lighting", NULL, NULL);
    if (window == NULL) {
        cerr << "Failed to create GLFW window" << endl;
        glfwTerminate();
        return -1;
    }
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
    glfwMakeContextCurrent(window);

    glewExperimental = true;
    if (glewInit() != GLEW_OK) {
        cerr << "Failed to create GLEW" << endl;
        glfwTerminate();
        return -1;
    }

    GLuint carTextureID = loadTexture("Textures/car.jpg");
    GLuint carGreenTextureID = loadTexture("Textures/cargreen.jpg");
    GLuint grassTextureID = loadTexture("Textures/grass.jpg");
    GLuint asphaltTextureID = loadTexture("Textures/asphalt.jpg");
    GLuint windshieldTextureID = loadTexture("Textures/windshield.jpg");
    GLuint bleacherTextureID = loadTexture("Textures/bleacher.jpg");
    GLuint lampTextureID = loadTexture("Textures/lamp.jpg");

    GLuint lightSurfaceTextureID = loadTexture("Textures/light_surface.jpg");

    glClearColor(0.1f, 0.1f, 0.2f, 1.0f); 

    int shaderProgram = compileAndLinkShaders(getLightingVertexShaderSource(), getLightingFragmentShaderSource());

    vec3 cameraPosition(0.0f, 5.0f, 15.0f);
    vec3 cameraLookAt(0.0f, 0.0f, -1.0f);
    vec3 cameraUp(0.0f, 1.0f, 0.0f);

    float cameraSpeed = 5.0f;
    float cameraFastSpeed = 10.0f;
    float cameraHorizontalAngle = 90.0f;
    float cameraVerticalAngle = -20.0f;

    mat4 projectionMatrix = perspective(70.0f, 1200.0f / 800.0f, 0.01f, 100.0f);

    setProjectionMatrix(shaderProgram, projectionMatrix);

    int cubeVAO = createVertexArrayObject(cubeVertexArray, sizeof(cubeVertexArray));

    vector<TexturedColoredVertex> grassFieldVertices = generateGrassFieldGeometry();
    int grassFieldVAO = createVertexArrayObjectFromVector(grassFieldVertices);

    vector<TexturedColoredVertex> carBodyVertices = generateCarBodyGeometry();
    vector<TexturedColoredVertex> carWindshieldVertices = generateCarWindshieldGeometry();
    vector<TexturedColoredVertex> carWheelsVertices = generateCarWheelsGeometry();

    int carBodyVAO = createVertexArrayObjectFromVector(carBodyVertices);
    int carWindshieldVAO = createVertexArrayObjectFromVector(carWindshieldVertices);
    int carWheelsVAO = createVertexArrayObjectFromVector(carWheelsVertices);

    float lastFrameTime = glfwGetTime();
    double lastMousePosX, lastMousePosY;
    glfwGetCursorPos(window, &lastMousePosX, &lastMousePosY);

    float carAngle = 0.0f;
    float carSpeed = 60.0f; 
    float trackRadius = 8.0f;

    float car2Angle = -20.0f; 
    float car2Speed = 58.0f;  
    float car2Radius = 8.6f; 

    vector<TexturedColoredVertex> trackVertices = generateCircularTrack(trackRadius - 1.0f, trackRadius + 1.0f, 64);
    int trackVAO = createVertexArrayObjectFromVector(trackVertices);

    cout << "Generated track with " << trackVertices.size() << " vertices" << endl;

    glEnable(GL_CULL_FACE);
    glEnable(GL_DEPTH_TEST);

    while (!glfwWindowShouldClose(window)) {
        float dt = glfwGetTime() - lastFrameTime;
        lastFrameTime += dt;

        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        glUseProgram(shaderProgram);

        double mousePosX, mousePosY;
        glfwGetCursorPos(window, &mousePosX, &mousePosY);
        double dx = mousePosX - lastMousePosX;
        double dy = mousePosY - lastMousePosY;
        lastMousePosX = mousePosX;
        lastMousePosY = mousePosY;

        const float cameraAngularSpeed = 60.0f;
        cameraHorizontalAngle -= dx * cameraAngularSpeed * dt;
        cameraVerticalAngle -= dy * cameraAngularSpeed * dt;
        cameraVerticalAngle = std::max(-85.0f, std::min(85.0f, cameraVerticalAngle));

        float theta = radians(cameraHorizontalAngle);
        float phi = radians(cameraVerticalAngle);
        cameraLookAt = vec3(cosf(phi)*cosf(theta), sinf(phi), -cosf(phi)*sinf(theta));
        vec3 cameraSideVector = normalize(cross(cameraLookAt, vec3(0.0f, 1.0f, 0.0f)));

        bool fastCam = glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS;
        float currentCameraSpeed = fastCam ? cameraFastSpeed : cameraSpeed;

        if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
            cameraPosition += cameraLookAt * dt * currentCameraSpeed;
        if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
            cameraPosition -= cameraLookAt * dt * currentCameraSpeed;
        if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
            cameraPosition += cameraSideVector * dt * currentCameraSpeed;
        if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
            cameraPosition -= cameraSideVector * dt * currentCameraSpeed;
        if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
            glfwSetWindowShouldClose(window, true);

        mat4 viewMatrix = lookAt(cameraPosition, cameraPosition + cameraLookAt, cameraUp);
        setViewMatrix(shaderProgram, viewMatrix);

        carAngle += carSpeed * dt;
        vec3 carPosition = vec3(trackRadius * cos(radians(carAngle)), 0.5f, trackRadius * sin(radians(carAngle)));

        vec3 carDirection = normalize(vec3(-sin(radians(carAngle)), 0.0f, cos(radians(carAngle))));

        car2Angle += car2Speed * dt;
        vec3 car2Position = vec3(car2Radius * cos(radians(car2Angle)), 0.5f, car2Radius * sin(radians(car2Angle)));
        vec3 car2Direction = normalize(vec3(-sin(radians(car2Angle)), 0.0f, cos(radians(car2Angle))));

        GLuint viewPosLocation = glGetUniformLocation(shaderProgram, "viewPos");
        glUniform3fv(viewPosLocation, 1, &cameraPosition[0]);

        GLuint numLightsLocation = glGetUniformLocation(shaderProgram, "numLights");

        glUniform1i(numLightsLocation, 10);

        for (int i = 0; i < 6; i++) {
            float lightAngle = i * 60.0f; 
            vec3 lightPos = vec3(10.0f * cos(radians(lightAngle)), 4.0f, 10.0f * sin(radians(lightAngle)));

            string lightPosUniform = "lights[" + to_string(i) + "].position";
            string lightColorUniform = "lights[" + to_string(i) + "].color";
            string lightIntensityUniform = "lights[" + to_string(i) + "].intensity";

            GLuint lightPosLoc = glGetUniformLocation(shaderProgram, lightPosUniform.c_str());
            GLuint lightColorLoc = glGetUniformLocation(shaderProgram, lightColorUniform.c_str());
            GLuint lightIntensityLoc = glGetUniformLocation(shaderProgram, lightIntensityUniform.c_str());

            glUniform3fv(lightPosLoc, 1, &lightPos[0]);
            glUniform3f(lightColorLoc, 1.0f, 1.0f, 0.8f); 
            glUniform1f(lightIntensityLoc, 2.0f);
        }

        vec3 headlight1Pos = carPosition + carDirection * 0.6f + vec3(0, 0.1f, 0);
        vec3 headlight2Pos = carPosition + carDirection * 0.6f + vec3(0, 0.1f, 0);

        GLuint light6PosLoc = glGetUniformLocation(shaderProgram, "lights[6].position");
        GLuint light6ColorLoc = glGetUniformLocation(shaderProgram, "lights[6].color");
        GLuint light6IntensityLoc = glGetUniformLocation(shaderProgram, "lights[6].intensity");
        glUniform3fv(light6PosLoc, 1, &headlight1Pos[0]);
        glUniform3f(light6ColorLoc, 1.0f, 1.0f, 1.0f); 
        glUniform1f(light6IntensityLoc, 1.5f);

        GLuint light7PosLoc = glGetUniformLocation(shaderProgram, "lights[7].position");
        GLuint light7ColorLoc = glGetUniformLocation(shaderProgram, "lights[7].color");
        GLuint light7IntensityLoc = glGetUniformLocation(shaderProgram, "lights[7].intensity");
        glUniform3fv(light7PosLoc, 1, &headlight2Pos[0]);
        glUniform3f(light7ColorLoc, 1.0f, 1.0f, 1.0f); 
        glUniform1f(light7IntensityLoc, 1.5f);

        vec3 headlight3Pos = car2Position + car2Direction * 0.6f + vec3(0, 0.1f, 0);
        vec3 headlight4Pos = car2Position + car2Direction * 0.6f + vec3(0, 0.1f, 0);

        GLuint light8PosLoc = glGetUniformLocation(shaderProgram, "lights[8].position");
        GLuint light8ColorLoc = glGetUniformLocation(shaderProgram, "lights[8].color");
        GLuint light8IntensityLoc = glGetUniformLocation(shaderProgram, "lights[8].intensity");
        glUniform3fv(light8PosLoc, 1, &headlight3Pos[0]);
        glUniform3f(light8ColorLoc, 1.0f, 1.0f, 1.0f); 
        glUniform1f(light8IntensityLoc, 1.5f);

        GLuint light9PosLoc = glGetUniformLocation(shaderProgram, "lights[9].position");
        GLuint light9ColorLoc = glGetUniformLocation(shaderProgram, "lights[9].color");
        GLuint light9IntensityLoc = glGetUniformLocation(shaderProgram, "lights[9].intensity");
        glUniform3fv(light9PosLoc, 1, &headlight4Pos[0]);
        glUniform3f(light9ColorLoc, 1.0f, 1.0f, 1.0f); 
        glUniform1f(light9IntensityLoc, 1.5f);

        GLuint textureLocation = glGetUniformLocation(shaderProgram, "textureSampler");

        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, grassTextureID);
        glUniform1i(textureLocation, 0);

        glBindVertexArray(grassFieldVAO);

        setWorldMatrix(shaderProgram, mat4(1.0f));
        glDrawArrays(GL_TRIANGLES, 0, grassFieldVertices.size());

        glBindTexture(GL_TEXTURE_2D, asphaltTextureID);
        glUniform1i(textureLocation, 0);
        glBindVertexArray(cubeVAO);

        for (int i = 0; i < 32; i++) {
            float angle = i * 11.25f; 
            vec3 trackPos = vec3(trackRadius * cos(radians(angle)), 0.01f, trackRadius * sin(radians(angle)));

            mat4 trackMatrix = translate(mat4(1.0f), trackPos) *
                              rotate(mat4(1.0f), radians(-1 * (angle + 90.0f)), vec3(0.0f, 1.0f, 0.0f)) *
                              scale(mat4(1.0f), vec3(4.4f, 0.1f, 2.0f)); 
            setWorldMatrix(shaderProgram, trackMatrix);
            glDrawArrays(GL_TRIANGLES, 0, 36);
        }

        glBindTexture(GL_TEXTURE_2D, bleacherTextureID);
        glUniform1i(textureLocation, 0);
        glBindVertexArray(cubeVAO);

        mat4 shearMatrix(1.0f);
        shearMatrix[1][2] = -0.5f;

        for (int i = 0; i < 4; i++) {
            float bleacherAngle = i * 90.0f;
            vec3 bleacherPos = vec3(15.0f * cos(radians(bleacherAngle)), 2.0f, 15.0f * sin(radians(bleacherAngle)));

            mat4 bleacherMatrix = translate(mat4(1.0f), bleacherPos)
                                * rotate(mat4(1.0f), radians(-1 * (bleacherAngle + 90.0f)), vec3(0.0f, 1.0f, 0.0f))
                                * scale(mat4(1.0f), vec3(8.0f, 4.0f, 2.0f))
                                * shearMatrix;

            setWorldMatrix(shaderProgram, bleacherMatrix);
            glDrawArrays(GL_TRIANGLES, 0, 36);
        }

        glBindVertexArray(cubeVAO);

        for (int i = 0; i < 6; i++) {
            float lightAngle = i * 60.0f;
            vec3 lampCenterPos = vec3(10.0f * cos(radians(lightAngle)), 0.0f, 10.0f * sin(radians(lightAngle)));

            mat4 lampRotationMatrix = rotate(mat4(1.0f), radians(-1 * (lightAngle - 90.0f)), vec3(0.0f, 1.0f, 0.0f));
            mat4 lampTranslationMatrix = translate(mat4(1.0f), lampCenterPos);
            mat4 lampGroupMatrix = lampTranslationMatrix * lampRotationMatrix;

            glBindTexture(GL_TEXTURE_2D, lampTextureID);
            glUniform1i(textureLocation, 0);
            float poleHeight = 4.0f;

            mat4 poleMatrix = translate(mat4(1.0f), vec3(0.0f, poleHeight / 2.0f, 0.0f)) * scale(mat4(1.0f), vec3(0.15f, poleHeight, 0.15f));
            setWorldMatrix(shaderProgram, lampGroupMatrix * poleMatrix);
            glDrawArrays(GL_TRIANGLES, 0, 36);

            float fixtureAngleDown = 30.0f;
            vec3 fixtureSize = vec3(1.5f, 0.15f, 0.8f);
            float sideOffset = 0.6f; 
            float verticalOffset = poleHeight;
            mat4 tiltRotation = rotate(mat4(1.0f), radians(fixtureAngleDown), vec3(1.0f, 0.0f, 0.0f));

            float forwardOffset = -fixtureSize.z / 2.0f;

            mat4 fixture1Root = translate(mat4(1.0f), vec3(sideOffset, verticalOffset, forwardOffset));

            glBindTexture(GL_TEXTURE_2D, lampTextureID);
            glUniform1i(textureLocation, 0);
            mat4 fixture1CasingMatrix = lampGroupMatrix * fixture1Root * tiltRotation * scale(mat4(1.0f), fixtureSize);
            setWorldMatrix(shaderProgram, fixture1CasingMatrix);
            glDrawArrays(GL_TRIANGLES, 0, 36);

            glBindTexture(GL_TEXTURE_2D, lightSurfaceTextureID);
            glUniform1i(textureLocation, 0);
            mat4 offsetMatrix = translate(mat4(1.0f), vec3(0.0f, -0.1f, 0.0f)); 
            mat4 fixture1LightMatrix = lampGroupMatrix * fixture1Root * offsetMatrix * tiltRotation * scale(mat4(1.0f), vec3(fixtureSize.x, 0.1f, fixtureSize.z)); 
            setWorldMatrix(shaderProgram, fixture1LightMatrix);
            glDrawArrays(GL_TRIANGLES, 0, 36);

            mat4 fixture2Root = translate(mat4(1.0f), vec3(-sideOffset, verticalOffset, forwardOffset));

            glBindTexture(GL_TEXTURE_2D, lampTextureID);
            glUniform1i(textureLocation, 0);
            mat4 fixture2CasingMatrix = lampGroupMatrix * fixture2Root * tiltRotation * scale(mat4(1.0f), fixtureSize);
            setWorldMatrix(shaderProgram, fixture2CasingMatrix);
            glDrawArrays(GL_TRIANGLES, 0, 36);

            glBindTexture(GL_TEXTURE_2D, lightSurfaceTextureID);
            glUniform1i(textureLocation, 0);
            mat4 fixture2LightMatrix = lampGroupMatrix * fixture2Root * offsetMatrix * tiltRotation * scale(mat4(1.0f), vec3(fixtureSize.x, 0.1f, fixtureSize.z)); 
            setWorldMatrix(shaderProgram, fixture2LightMatrix);
            glDrawArrays(GL_TRIANGLES, 0, 36);
        }

        mat4 carMatrix = translate(mat4(1.0f), carPosition) *
                        rotate(mat4(1.0f), radians(-1*(carAngle + 90.0f)), vec3(0.0f, 1.0f, 0.0f)) *
                        scale(mat4(1.0f), vec3(1.0f, 1.0f, 1.0f));

        glBindTexture(GL_TEXTURE_2D, carTextureID);
        glUniform1i(textureLocation, 0);
        glBindVertexArray(carBodyVAO);
        setWorldMatrix(shaderProgram, carMatrix);
        glDrawArrays(GL_TRIANGLES, 0, carBodyVertices.size());

        glBindTexture(GL_TEXTURE_2D, windshieldTextureID);
        glUniform1i(textureLocation, 0);
        glBindVertexArray(carWindshieldVAO);
        setWorldMatrix(shaderProgram, carMatrix);
        glDrawArrays(GL_TRIANGLES, 0, carWindshieldVertices.size());

        glBindVertexArray(carWheelsVAO);
        setWorldMatrix(shaderProgram, carMatrix);
        glDrawArrays(GL_TRIANGLES, 0, carWheelsVertices.size());

        mat4 car2Matrix = translate(mat4(1.0f), car2Position) *
                         rotate(mat4(1.0f), radians(-1 * (car2Angle + 90.0f)), vec3(0.0f, 1.0f, 0.0f)) *
                         scale(mat4(1.0f), vec3(1.0f, 1.0f, 1.0f));

        glBindTexture(GL_TEXTURE_2D, carGreenTextureID);
        glUniform1i(textureLocation, 0);
        glBindVertexArray(carBodyVAO);
        setWorldMatrix(shaderProgram, car2Matrix);
        glDrawArrays(GL_TRIANGLES, 0, carBodyVertices.size());

        glBindTexture(GL_TEXTURE_2D, windshieldTextureID);
        glUniform1i(textureLocation, 0);
        glBindVertexArray(carWindshieldVAO);
        setWorldMatrix(shaderProgram, car2Matrix);
        glDrawArrays(GL_TRIANGLES, 0, carWindshieldVertices.size());

        glBindVertexArray(carWheelsVAO);
        setWorldMatrix(shaderProgram, car2Matrix);
        glDrawArrays(GL_TRIANGLES, 0, carWheelsVertices.size());

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    glfwTerminate();
    return 0;
}

const char* getLightingVertexShaderSource()
{
    return
        "#version 330 core\n"
        "layout (location = 0) in vec3 aPos;"
        "layout (location = 1) in vec3 aColor;"
        "layout (location = 2) in vec2 aUV;"
        "layout (location = 3) in vec3 aNormal;"

        "uniform mat4 worldMatrix;"
        "uniform mat4 viewMatrix = mat4(1.0);"
        "uniform mat4 projectionMatrix = mat4(1.0);"

        "out vec3 FragPos;"
        "out vec3 Normal;"
        "out vec2 TexCoord;"
        "out vec3 vertexColor;"

        "void main()"
        "{"
        "   FragPos = vec3(worldMatrix * vec4(aPos, 1.0));"
        "   Normal = mat3(transpose(inverse(worldMatrix))) * aNormal;"
        "   TexCoord = aUV;"
        "   vertexColor = aColor;"
        "   gl_Position = projectionMatrix * viewMatrix * vec4(FragPos, 1.0);"
        "}";
}

const char* getLightingFragmentShaderSource()
{

    return
        "#version 330 core\n"
        "struct Light {"
        "   vec3 position;"
        "   vec3 color;"
        "   float intensity;"
        "};"

        "uniform Light lights[10];"
        "uniform int numLights;"
        "uniform vec3 viewPos;"
        "uniform sampler2D textureSampler;"

        "in vec3 FragPos;"
        "in vec3 Normal;"
        "in vec2 TexCoord;"
        "in vec3 vertexColor;"

        "out vec4 FragColor;"

        "void main()"
        "{"
        "   vec3 color = texture(textureSampler, TexCoord).rgb * vertexColor;"
        "   vec3 ambient = 0.15 * color;"
        "   vec3 lighting = ambient;"
        "   vec3 normal = normalize(Normal);"
        "   "
        "   for(int i = 0; i < numLights; i++) {"
        "       vec3 lightDir = normalize(lights[i].position - FragPos);"
        "       float distance = length(lights[i].position - FragPos);"
        "       float attenuation = lights[i].intensity / (1.0 + 0.1 * distance + 0.02 * distance * distance);"
        "       "
        "       float diff = max(dot(normal, lightDir), 0.0);"
        "       vec3 diffuse = diff * lights[i].color * attenuation;"
        "       "
        "       vec3 viewDir = normalize(viewPos - FragPos);"
        "       vec3 reflectDir = reflect(-lightDir, normal);"
        "       float spec = pow(max(dot(viewDir, reflectDir), 0.0), 64);"
        "       vec3 specular = spec * lights[i].color * attenuation * 0.5;"
        "       "
        "       lighting += (diffuse + specular) * color;"
        "   }"
        "   "
        "   FragColor = vec4(lighting, 1.0);"
        "}";
}

GLuint loadTexture(const char *filename)
{
    cout << "Attempting to load texture: " << filename << endl; 

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
        cout << "Successfully loaded '" << filename << "' with dimensions " << width << "x" << height << " and " << nrChannels << " channels." << endl;

        if (width <= 0 || height <= 0 || width > 16384 || height > 16384) {
            cout << "ERROR: Image '" << filename << "' has invalid dimensions (" << width << "x" << height << ")." << endl;
            stbi_image_free(data); 
            data = nullptr; 
        }
    }

    if (data) {
        GLenum format = (nrChannels == 3) ? GL_RGB : GL_RGBA;
        glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, data);
        glGenerateMipmap(GL_TEXTURE_2D);
    } else {

        cout << "Failed to load texture: " << filename << endl;
        cout << "STB Image Error: " << stbi_failure_reason() << endl; 

        unsigned char redPixel[] = {255, 0, 0, 255};
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 1, 1, 0, GL_RGBA, GL_UNSIGNED_BYTE, redPixel);
    }

    stbi_image_free(data);

    return textureID;
}

int compileAndLinkShaders(const char* vertexShaderSource, const char* fragmentShaderSource)
{
    int vertexShader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertexShader, 1, &vertexShaderSource, NULL);
    glCompileShader(vertexShader);

    int success;
    char infoLog[512];
    glGetShaderiv(vertexShader, GL_COMPILE_STATUS, &success);
    if (!success) {
        glGetShaderInfoLog(vertexShader, 512, NULL, infoLog);
        cerr << "ERROR::SHADER::VERTEX::COMPILATION_FAILED\n" << infoLog << endl;
    }

    int fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragmentShader, 1, &fragmentShaderSource, NULL);
    glCompileShader(fragmentShader);

    glGetShaderiv(fragmentShader, GL_COMPILE_STATUS, &success);
    if (!success) {
        glGetShaderInfoLog(fragmentShader, 512, NULL, infoLog);
        cerr << "ERROR::SHADER::FRAGMENT::COMPILATION_FAILED\n" << infoLog << endl;
    }

    int shaderProgram = glCreateProgram();
    glAttachShader(shaderProgram, vertexShader);
    glAttachShader(shaderProgram, fragmentShader);
    glLinkProgram(shaderProgram);

    glGetProgramiv(shaderProgram, GL_LINK_STATUS, &success);
    if (!success) {
        glGetProgramInfoLog(shaderProgram, 512, NULL, infoLog);
        cerr << "ERROR::SHADER::PROGRAM::LINKING_FAILED\n" << infoLog << endl;
    }

    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);

    return shaderProgram;
}