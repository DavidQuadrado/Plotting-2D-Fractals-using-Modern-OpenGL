
//
//  main.cpp
//  
//
//  Created by David Quadrado on 12/10/2024.
//  Copyright © 2024 David Quadrado. All rights reserved.
//

// Includes	

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <ctime>
#include <iostream>

#include <glm/gtc/constants.hpp>
#include <cstdlib>


#include <GL/glew.h>	     // Include GLEW
#include <GLFW/glfw3.h>      // Include GLFW
GLFWwindow* window;

#include <glm/glm.hpp>	     // Include GLM
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
using namespace glm;

#include <common/shader.hpp> // shaders header file


#define SCREEN_WIDTH 900
#define SCREEN_HEIGHT 900


// Define your variables
GLuint VertexArrayID;   // Vertex array object (VAO)
GLuint vertexbuffer;    // Vertex buffer object (VBO)
GLuint colorbuffer;     // Color buffer object (CBO)
GLuint programID;       // GLSL program from the shaders
GLuint linebuffer;      // Line buffer object for connecting midpoints to center
GLuint midpointbuffer;
GLfloat g_vertex_buffer_data[17 * 3];



void transferDataToGPUMemory(void)
{
    // VAO
    glGenVertexArrays(1, &VertexArrayID);
    glBindVertexArray(VertexArrayID);

    // Create and compile shaders
    programID = LoadShaders(
        "./SimpleVertexShader.vertexshader",
        "./SimpleFragmentShader.fragmentshader");

    // Initialize vertices for the octagon (8 vertices + 8 midpoints + 1 for closing)
    float pi = glm::pi<float>();
    GLfloat radius = 30.0f;
    GLfloat xpos = 0.0f;
    GLfloat ypos = 0.0f;

    // Calculate the center of the octagon
    GLfloat center[3] = { xpos, ypos, 0.0f };

    for (int i = 0; i < 8; i++)
    {
        float angle = (2 * pi * i / 8);
        g_vertex_buffer_data[i * 3] = xpos + radius * cos(angle); // X
        g_vertex_buffer_data[i * 3 + 1] = ypos + radius * sin(angle); // Y
        g_vertex_buffer_data[i * 3 + 2] = 0.0f; // Z

        // Midpoint calculation
        float nextAngle = (2 * pi * ((i + 1) % 8) / 8);
        g_vertex_buffer_data[(i + 8) * 3] = (g_vertex_buffer_data[i * 3] + (xpos + radius * cos(nextAngle))) / 2; // Midpoint X
        g_vertex_buffer_data[(i + 8) * 3 + 1] = (g_vertex_buffer_data[i * 3 + 1] + (ypos + radius * sin(nextAngle))) / 2; // Midpoint Y
        g_vertex_buffer_data[(i + 8) * 3 + 2] = 0.0f; // Z
    }

    // Close the shape
    g_vertex_buffer_data[16 * 3] = g_vertex_buffer_data[0]; // Closing the shape

    // Color buffer setup (for both vertices and midpoints)
    GLfloat g_color_buffer_data[17 * 3];
    for (int i = 0; i < 17; i++)
    {
        g_color_buffer_data[i * 3] = 0.9f; // R
        g_color_buffer_data[i * 3 + 1] = 0.7f; // G
        g_color_buffer_data[i * 3 + 2] = 0.0f; // B
    }

    // Store quad points
    GLfloat g_quad_buffer_data[8 * 4 * 3]; // 8 quadrados, 4 pontos cada, 3 coordenadas por ponto
    int quadIndex = 0;
    for (int i = 0; i < 8; i++)
    {
        // Cálculo dos índices
        int currentMidpointIndex = i + 8;
        int nextVertex = (i + 1) % 8;
        int nextMidpointIndex = nextVertex + 8;

        // Ponto médio atual
        GLfloat midpointX = g_vertex_buffer_data[currentMidpointIndex * 3];
        GLfloat midpointY = g_vertex_buffer_data[currentMidpointIndex * 3 + 1];

        // Armazenando os pontos
        g_quad_buffer_data[quadIndex * 12] = midpointX;           // Ponto médio
        g_quad_buffer_data[quadIndex * 12 + 1] = midpointY;
        g_quad_buffer_data[quadIndex * 12 + 2] = 0.0f;

        g_quad_buffer_data[quadIndex * 12 + 3] = g_vertex_buffer_data[nextVertex * 3];      // Próximo vértice
        g_quad_buffer_data[quadIndex * 12 + 4] = g_vertex_buffer_data[nextVertex * 3 + 1];
        g_quad_buffer_data[quadIndex * 12 + 5] = 0.0f;

        g_quad_buffer_data[quadIndex * 12 + 6] = g_vertex_buffer_data[nextMidpointIndex * 3]; // Próximo ponto médio
        g_quad_buffer_data[quadIndex * 12 + 7] = g_vertex_buffer_data[nextMidpointIndex * 3 + 1];
        g_quad_buffer_data[quadIndex * 12 + 8] = 0.0f;

        g_quad_buffer_data[quadIndex * 12 + 9] = center[0];  // Ponto central
        g_quad_buffer_data[quadIndex * 12 + 10] = center[1];
        g_quad_buffer_data[quadIndex * 12 + 11] = center[2];

        quadIndex++;
    }

    // Vertex buffer
    glGenBuffers(1, &vertexbuffer);
    glBindBuffer(GL_ARRAY_BUFFER, vertexbuffer);
    glBufferData(GL_ARRAY_BUFFER, sizeof(g_vertex_buffer_data), g_vertex_buffer_data, GL_STATIC_DRAW);

    // Color buffer
    glGenBuffers(1, &colorbuffer);
    glBindBuffer(GL_ARRAY_BUFFER, colorbuffer);
    glBufferData(GL_ARRAY_BUFFER, sizeof(g_color_buffer_data), g_color_buffer_data, GL_STATIC_DRAW);

    // Quad buffer
    GLuint quadBuffer;
    glGenBuffers(1, &quadBuffer);
    glBindBuffer(GL_ARRAY_BUFFER, quadBuffer);
    glBufferData(GL_ARRAY_BUFFER, sizeof(g_quad_buffer_data), g_quad_buffer_data, GL_STATIC_DRAW);
}

void draw(void)
{
    // Clear the screen
    glClear(GL_COLOR_BUFFER_BIT);

    // Use our shader
    glUseProgram(programID);

    // Set the MVP matrix
    glm::mat4 mvp = glm::ortho(-40.0f, 40.0f, -40.0f, 40.0f);
    unsigned int matrix = glGetUniformLocation(programID, "mvp");
    glUniformMatrix4fv(matrix, 1, GL_FALSE, &mvp[0][0]);

    // Enable vertex attribute arrays
    glEnableVertexAttribArray(0);
    glEnableVertexAttribArray(1);

    // Bind the vertex buffer
    glBindBuffer(GL_ARRAY_BUFFER, vertexbuffer);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, (void*)0);

    // Bind the color buffer
    glBindBuffer(GL_ARRAY_BUFFER, colorbuffer);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 0, (void*)0);

    // Draw the octagon
    glDrawArrays(GL_LINE_LOOP, 0, 8); // Draw the octagon

    // Draw the midpoints as points
    glEnable(GL_PROGRAM_POINT_SIZE);
    glDrawArrays(GL_POINTS, 8, 8); // Draw midpoints

    // Draw lines connecting every point (vertices + midpoints)
    const int totalPoints = 17; // 8 vertices + 8 midpoints + 1 closing point
    for (int i = 0; i < totalPoints; ++i)
    {
        for (int j = i + 1; j < totalPoints; ++j)
        {
            // Set up a buffer for the two points
            GLfloat lineVertices[6] = {
                g_vertex_buffer_data[i * 3], g_vertex_buffer_data[i * 3 + 1], 0.0f, // Point i
                g_vertex_buffer_data[j * 3], g_vertex_buffer_data[j * 3 + 1], 0.0f  // Point j
            };

            // Create a buffer for the line
            GLuint lineBuffer;
            glGenBuffers(1, &lineBuffer);
            glBindBuffer(GL_ARRAY_BUFFER, lineBuffer);
            glBufferData(GL_ARRAY_BUFFER, sizeof(lineVertices), lineVertices, GL_STATIC_DRAW);

            // Draw the line
            glBindBuffer(GL_ARRAY_BUFFER, lineBuffer);
            glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, (void*)0);
            glDrawArrays(GL_LINES, 0, 2); // Draw line between points i and j

            // Clean up
            glDeleteBuffers(1, &lineBuffer); // Delete the temporary buffer
        }
    }

    // Clean up the vertex and color buffers
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glDisableVertexAttribArray(0);
    glDisableVertexAttribArray(1);
}







void cleanupDataFromGPU()
{
    glDeleteBuffers(1, &vertexbuffer);
    glDeleteBuffers(1, &colorbuffer);
    glDeleteVertexArrays(1, &VertexArrayID);
    glDeleteProgram(programID);
}

int main(void)
{
    GLFWwindow* window;

    // Initialize the library
    if (!glfwInit())
    {
        return -1;
    }

    glfwWindowHint(GLFW_SAMPLES, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE); // To make MacOS happy; should not be needed
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);



    // Create a windowed mode window and its OpenGL context
    window = glfwCreateWindow(SCREEN_WIDTH, SCREEN_HEIGHT, "Trabalho 1 - Octagon fractal ", NULL, NULL);

    if (!window)
    {
        glfwTerminate();
        return -1;
    }

    // Make the window's context current
    glfwMakeContextCurrent(window);

    glewExperimental = true;
    // Needed for core profile
    if (glewInit() != GLEW_OK) {
        fprintf(stderr, "Failed to initialize GLEW\n");
        getchar();
        glfwTerminate();
        return -1;
    }

    glfwSetInputMode(window, GLFW_STICKY_KEYS, GL_TRUE);

    // Põe a cor do ecrã a preto
    glClearColor(0.0f, 0.0f, 0.0f, 0.0f);


    // transfer my data (vertices, colors, and shaders) to GPU side
    transferDataToGPUMemory();


    // Loop until the user closes the window
    while (!glfwWindowShouldClose(window))
    {

        draw();
        


        // Swap front and back buffers
        glfwSwapBuffers(window);

        // Poll for and process events
        glfwPollEvents();
    }

    // Cleanup VAO, VBOs, and shaders from GPU
    cleanupDataFromGPU();

    glfwTerminate();

    return 0;
}