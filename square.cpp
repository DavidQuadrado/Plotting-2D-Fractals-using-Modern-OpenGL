#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <iostream>
#include <vector>
#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include "shader.hpp" // Header file for loading shaders

GLFWwindow * window;
GLuint VertexArrayID;
GLuint vertexbuffer;
GLuint colorbuffer;
GLuint programID;

#define SCREEN_WIDTH 1000 
#define SCREEN_HEIGHT 1000 
#define RADIUS 2.0f 
#define M_PI 3.14159265358979323846 

// Function to generate vertices for a quadrilateral (e.g., square)
std::vector<GLfloat> generateQuadrilateralVertices() {
    const int sides = 4; // 4 sides for a quadrilateral
    const float angleIncrement = M_PI / 2; // 90 degrees
    std::vector<GLfloat> vertices(sides * 3);

    for (int i = 0; i < sides; ++i) {
        float angle = i * angleIncrement;
        vertices[i * 3] = RADIUS * cosf(angle);
        vertices[i * 3 + 1] = RADIUS * sinf(angle);
        vertices[i * 3 + 2] = 0.0f; // Z coordinate
    }

    return vertices;
}

// Function to calculate a point using the formula P(t) = (1 - t)A + tB
glm::vec3 calculatePoint(const glm::vec3& A, const glm::vec3& B, float t) {
    return (1 - t) * A + t * B;
}

// Recursive function to generate points for fractals based on sides of a polygon
void drawFractal(std::vector<GLfloat>& fractal_points, const std::vector<glm::vec3>& vertices, int depth) {
    if (depth <= 0) return;

    std::vector<glm::vec3> newPoints;

    for (size_t i = 0; i < vertices.size(); ++i) {
        glm::vec3 A = vertices[i];
        glm::vec3 B = vertices[(i + 1) % vertices.size()]; // Next point (wraps around)

        // Calculate the new point P(0.25)
        glm::vec3 P = calculatePoint(A, B, 0.01f);
        newPoints.push_back(P); // Store the created point
    }

    // Connect the new points
    for (size_t i = 0; i < newPoints.size(); ++i) {
        fractal_points.push_back(newPoints[i].x);
        fractal_points.push_back(newPoints[i].y);
        fractal_points.push_back(newPoints[i].z);

        // Draw a line to the next point
        glm::vec3 nextPoint = newPoints[(i + 1) % newPoints.size()]; // Next point (wraps around)
        fractal_points.push_back(nextPoint.x);
        fractal_points.push_back(nextPoint.y);
        fractal_points.push_back(nextPoint.z);
    }

    // Recursive call for the next depth level using the new points
    drawFractal(fractal_points, newPoints, depth - 1);
}

// Transfer data to GPU memory
void transferDataToGPUMemory() {
    glGenVertexArrays(1, &VertexArrayID);
    glBindVertexArray(VertexArrayID);
    programID = LoadShaders("SimpleVertexShader.vertexshader", "SimpleFragmentShader.fragmentshader");

    std::vector<GLfloat> vertex_data = generateQuadrilateralVertices();

    // Create the fractal points vector
    std::vector<GLfloat> fractal_points;

    // Start the fractal drawing
    drawFractal(fractal_points,
        { glm::vec3(vertex_data[0], vertex_data[1], vertex_data[2]),
         glm::vec3(vertex_data[3], vertex_data[4], vertex_data[5]),
         glm::vec3(vertex_data[6], vertex_data[7], vertex_data[8]),
         glm::vec3(vertex_data[9], vertex_data[10], vertex_data[11]) },
        4); // Change depth as needed

    // Setup the vertex buffer
    glGenBuffers(1, &vertexbuffer);
    glBindBuffer(GL_ARRAY_BUFFER, vertexbuffer);
    glBufferData(GL_ARRAY_BUFFER, fractal_points.size() * sizeof(GLfloat), fractal_points.data(), GL_STATIC_DRAW);
}

// Clean up GPU resources
void cleanupDataFromGPU() {
    glDeleteBuffers(1, &vertexbuffer);
    glDeleteVertexArrays(1, &VertexArrayID);
    glDeleteProgram(programID);
}

// Draw function
void draw(void) {
    glClear(GL_COLOR_BUFFER_BIT); // Clear the screen
    glUseProgram(programID); // Use the compiled shader program

    // Set up the Model-View-Projection (MVP) matrix using an orthographic projection
    glm::mat4 mvp = glm::ortho(-3.0f, 3.0f, -3.0f, 3.0f);
    unsigned int matrix = glGetUniformLocation(programID, "mvp");
    glUniformMatrix4fv(matrix, 1, GL_FALSE, &mvp[0][0]);

    glEnableVertexAttribArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, vertexbuffer);

    // Correctly retrieve the size of the vertex buffer
    GLsizei vertexCount;
    glGetBufferParameteriv(GL_ARRAY_BUFFER, GL_BUFFER_SIZE, &vertexCount);
    vertexCount /= sizeof(GLfloat); // Convert from bytes to number of vertices (GLfloat)

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, (void*)0);

    // Draw the fractal lines
    glDrawArrays(GL_LINES, 0, vertexCount);

    glDisableVertexAttribArray(0);
}

// Main function
int main(void) {
    if (!glfwInit()) {
        return -1;
    }

    glfwWindowHint(GLFW_SAMPLES, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    window = glfwCreateWindow(SCREEN_WIDTH, SCREEN_HEIGHT, "Quadrilateral Fractal", NULL, NULL);
    if (!window) {
        glfwTerminate();
        return -1;
    }

    glfwMakeContextCurrent(window);
    glewExperimental = true;
    if (glewInit() != GLEW_OK) {
        fprintf(stderr, "Failed to initialize GLEW\n");
        getchar();
        glfwTerminate();
        return -1;
    }

    glfwSetInputMode(window, GLFW_STICKY_KEYS, GL_TRUE);
    glClearColor(1.0f, 1.0f, 1.0f, 0.0f);
    transferDataToGPUMemory();

    while (!glfwWindowShouldClose(window)) {
        draw();
        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    cleanupDataFromGPU();
    glfwTerminate();
    return 0;
}