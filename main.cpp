#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <ctime>
#include <iostream>
#include <vector>
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
GLuint interpolatedPointsBuffer;


float delta = 0;
float rotationAngle = 0;

GLuint numIterations = 15;

const int numDivisions = 1;

GLfloat g_vertex_buffer_data[17 * 3];
GLfloat g_color_buffer_data[100 * 3];
GLfloat g_division_buffer_data[4 * 3];




void interpolatePoints(GLuint divisionBuffer, int numDivisions, int numIterations) {
    // Bind the division buffer to read the data
    glBindBuffer(GL_ARRAY_BUFFER, divisionBuffer);

    // Map the buffer data to a pointer
    glm::vec3* divisionData = (glm::vec3*)glMapBuffer(GL_ARRAY_BUFFER, GL_READ_ONLY);
    if (!divisionData) {
        std::cerr << "Failed to map buffer data!" << std::endl;
        return;
    }

    const float t = 0.15f; // Define the value of t
    std::vector<glm::vec3> interpolatedPoints(4 * numDivisions); // Vector to hold the current interpolated points
    std::vector<glm::vec3> previousPoints(4 * numDivisions); // Vector to hold the previous interpolated points

    // Copy initial division data to previousPoints
    std::copy(divisionData, divisionData + 4 * numDivisions, previousPoints.begin());

    // Create a buffer large enough to store all points from all iterations
    GLuint totalPoints = 4 * numDivisions * numIterations;
    glGenBuffers(1, &interpolatedPointsBuffer);
    glBindBuffer(GL_ARRAY_BUFFER, interpolatedPointsBuffer);
    glBufferData(GL_ARRAY_BUFFER, totalPoints * sizeof(glm::vec3), nullptr, GL_DYNAMIC_DRAW); // Use GL_DYNAMIC_DRAW

    // Initialize the offset on zero (starting position in the buffer)
    GLintptr offset = 0;

    // Loop for the specified number of iterations
    for (int iter = 0; iter < numIterations; iter++) {
        for (int i = 0; i < numDivisions; i++) {
            // Indices of points P, Q, R, S
            glm::vec3 P = previousPoints[i * 4];     // P (1st point)
            glm::vec3 Q = previousPoints[i * 4 + 1]; // Q (2nd point)
            glm::vec3 R = previousPoints[i * 4 + 2]; // R (3rd point)
            glm::vec3 S = previousPoints[i * 4 + 3]; // S (4th point)

            // Calculate the interpolated points
            interpolatedPoints[i * 4] = (1 - t) * P + t * Q; // First interpolated point
            interpolatedPoints[i * 4 + 1] = (1 - t) * Q + t * R; // Second interpolated point
            interpolatedPoints[i * 4 + 2] = (1 - t) * R + t * S; // Third interpolated point
            interpolatedPoints[i * 4 + 3] = (1 - t) * S + t * P; // Fourth interpolated point
        }

        // Update the buffer with interpolated points at the current offset
        glBufferSubData(GL_ARRAY_BUFFER, offset, interpolatedPoints.size() * sizeof(glm::vec3), interpolatedPoints.data());

        // Print the new interpolated points 
        std::cout << "Iteration " << iter + 1 << ":\n";
        for (int i = 0; i < numDivisions; i++) {
            for (int j = 0; j < 4; j++) {
                glm::vec3 point = interpolatedPoints[i * 4 + j];
                std::cout << "Point " << (i * 4 + j) + 1 << ": ("
                    << point.x << ", "
                    << point.y << ", "
                    << point.z << ")\n";
            }
        }

        // Move the offset forward for the next set of points
        offset += interpolatedPoints.size() * sizeof(glm::vec3);

        // Prepare for the next iteration
        previousPoints = interpolatedPoints; // Update previousPoints with the interpolated points
    }

    // Unmap the buffer
    glUnmapBuffer(GL_ARRAY_BUFFER);

    // Map the final buffer to read its data and print all points stored
    glBindBuffer(GL_ARRAY_BUFFER, interpolatedPointsBuffer);
    glm::vec3* bufferData = (glm::vec3*)glMapBuffer(GL_ARRAY_BUFFER, GL_READ_ONLY);
    if (!bufferData) {
        std::cerr << "Failed to map final interpolated points buffer!" << std::endl;
        return;
    }

    // Print all points for all iterations saved on the buffer
    std::cout << "All Interpolated Points in Buffer:\n";
    for (size_t i = 0; i < totalPoints; ++i) {
        glm::vec3 point = bufferData[i];
        std::cout << "Point " << i + 1 << ": ("
            << point.x << ", "
            << point.y << ", "
            << point.z << ")\n";
    }

    // Unmap the buffer after reading
    glUnmapBuffer(GL_ARRAY_BUFFER);
}







void transferDataToGPUMemory(void)
{
    // VAO
    glGenVertexArrays(1, &VertexArrayID);
    glBindVertexArray(VertexArrayID);

    // Create and compile shaders
    programID = LoadShaders(
        "./SimpleVertexShader.vertexshader",
        "./SimpleFragmentShader.fragmentshader");

    // Initialize vertices for the octagon (8 vertices + 8 midpoints)
    float pi = glm::pi<float>();
    GLfloat radius = 1.0f;
    GLfloat xpos = 0.0f;
    GLfloat ypos = 0.0f;


    // Octagon vertices and midpoints
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
    GLfloat g_color_buffer_data[100 * 3];
    for (int i = 0; i < 100; i++)
    {
        g_color_buffer_data[i * 3] = 0.9f; // R
        g_color_buffer_data[i * 3 + 1] = 0.7f; // G
        g_color_buffer_data[i * 3 + 2] = 0.0f; // B
    }

    // Vertex buffer
    glGenBuffers(1, &vertexbuffer);
    glBindBuffer(GL_ARRAY_BUFFER, vertexbuffer);
    glBufferData(GL_ARRAY_BUFFER, sizeof(g_vertex_buffer_data), g_vertex_buffer_data, GL_STATIC_DRAW);

    // Color buffer
    glGenBuffers(1, &colorbuffer);
    glBindBuffer(GL_ARRAY_BUFFER, colorbuffer);
    glBufferData(GL_ARRAY_BUFFER, sizeof(g_color_buffer_data), g_color_buffer_data, GL_STATIC_DRAW);

    // Center Octágon
    glm::vec3 center(xpos, ypos, 0.0f); // Center


    //divisoes

    for (int i = 0; i < numDivisions; i++)
    {
        // Índice
        int vertexIndex2 = 1 + (i % 8);             // Vértice vai avançando em cada iteração
        int midpointIndex1 = 8 + (i % 8);           // Primeiro ponto médio, também avançando em cada iteração
        int midpointIndex2 = 8 + ((i + 1) % 8);     // Próximo ponto médio, avançando 1 posição a mais
        int centerIndex = 0;                         // Centro do octágono (fixo)

        // Point 1 = 1st Middle Point
        g_division_buffer_data[i * 12] = g_vertex_buffer_data[midpointIndex1 * 3];        // X
        g_division_buffer_data[i * 12 + 1] = g_vertex_buffer_data[midpointIndex1 * 3 + 1]; // Y
        g_division_buffer_data[i * 12 + 2] = g_vertex_buffer_data[midpointIndex1 * 3 + 2]; // Z

        // Point 2 = vértice
        g_division_buffer_data[i * 12 + 3] = g_vertex_buffer_data[vertexIndex2 * 3];       // X
        g_division_buffer_data[i * 12 + 4] = g_vertex_buffer_data[vertexIndex2 * 3 + 1];   // Y
        g_division_buffer_data[i * 12 + 5] = g_vertex_buffer_data[vertexIndex2 * 3 + 2];   // Z

        // Point 3 = 2nd Middle Point
        g_division_buffer_data[i * 12 + 6] = g_vertex_buffer_data[midpointIndex2 * 3];       // X
        g_division_buffer_data[i * 12 + 7] = g_vertex_buffer_data[midpointIndex2 * 3 + 1];   // Y
        g_division_buffer_data[i * 12 + 8] = g_vertex_buffer_data[midpointIndex2 * 3 + 2];   // Z

        // Point 4 = center 
        g_division_buffer_data[i * 12 + 9] = center.x;   //  X
        g_division_buffer_data[i * 12 + 10] = center.y;  // Y
        g_division_buffer_data[i * 12 + 11] = center.z;  // Z

        // Print the 4 points from Divisao
        printf("Divisão %d:\n", i + 1);
        printf("Ponto 1: X = %f, Y = %f, Z = %f\n", g_division_buffer_data[i * 12], g_division_buffer_data[i * 12 + 1], g_division_buffer_data[i * 12 + 2]);
        printf("Ponto 2: X = %f, Y = %f, Z = %f\n", g_division_buffer_data[i * 12 + 3], g_division_buffer_data[i * 12 + 4], g_division_buffer_data[i * 12 + 5]);
        printf("Ponto 3: X = %f, Y = %f, Z = %f\n", g_division_buffer_data[i * 12 + 6], g_division_buffer_data[i * 12 + 7], g_division_buffer_data[i * 12 + 8]);
        printf("Centro: X = %f, Y = %f, Z = %f\n", g_division_buffer_data[i * 12 + 9], g_division_buffer_data[i * 12 + 10], g_division_buffer_data[i * 12 + 11]);

    }

    // Create a Buffer to save the points of the divison
    GLuint divisionBuffer;
    glGenBuffers(1, &divisionBuffer);
    glBindBuffer(GL_ARRAY_BUFFER, divisionBuffer);
    glBufferData(GL_ARRAY_BUFFER, sizeof(g_division_buffer_data), g_division_buffer_data, GL_STATIC_DRAW);


    // Call function to calculate the Interpolated Points
    interpolatePoints(divisionBuffer, 1, numIterations); 


}




void draw(void)
{
    // Clear the screen
    //glClear(GL_COLOR_BUFFER_BIT);

    // Use our shader
    glUseProgram(programID);

    // Set the MVP matrix
    glm::mat4 mvp = glm::ortho(-2.0f, 2.0f, -2.0f, 2.0f);
    unsigned int matrix = glGetUniformLocation(programID, "mvp");
    glUniformMatrix4fv(matrix, 1, GL_FALSE, &mvp[0][0]);

    // Set of the Translate func

    glm::mat4 trans;
    trans = glm::translate(glm::mat4(1.0), glm::vec3(delta, delta, 0.0f));
    trans = glm::rotate(trans, rotationAngle, glm::vec3(0.0f, 0.0f, 1.0f)); // Rotate on z coordenate
    unsigned int m = glGetUniformLocation(programID, "trans");
    glUniformMatrix4fv(m, 1, GL_FALSE, &trans[0][0]);


    // 1st attribute buffer: vertices
    glEnableVertexAttribArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, vertexbuffer);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, (void*)0);

    // 2nd attribute buffer: colors
    glEnableVertexAttribArray(1);
    glBindBuffer(GL_ARRAY_BUFFER, colorbuffer);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 0, (void*)0);

    // Draw the octagon
    glDrawArrays(GL_LINE_LOOP, 0, 8); // Draw the octagon linhas de fora

    // Draw the midpoints 
    glEnable(GL_PROGRAM_POINT_SIZE);
    glDrawArrays(GL_POINTS, 8, 8); // Draw midpoints

    // Def on Center
    GLfloat centerX = 0.0f;
    GLfloat centerY = 0.0f;

    // Create a buffer for the center lines
    GLfloat centerLines[16 * 3]; // 16 vertices for 8 lines (midpoint to center)
    for (int i = 0; i < 8; i++)
    {
        // Midpoint
        centerLines[i * 6] = g_vertex_buffer_data[(i + 8) * 3];     // Midpoint X
        centerLines[i * 6 + 1] = g_vertex_buffer_data[(i + 8) * 3 + 1]; // Midpoint Y
        centerLines[i * 6 + 2] = 0.0f; // Z

        // Center
        centerLines[i * 6 + 3] = centerX; // Center X
        centerLines[i * 6 + 4] = centerY; // Center Y
        centerLines[i * 6 + 5] = 0.0f; // Z
    }

    // Buffer for center lines
    GLuint centerLinesBuffer;
    glGenBuffers(1, &centerLinesBuffer);
    glBindBuffer(GL_ARRAY_BUFFER, centerLinesBuffer);
    glBufferData(GL_ARRAY_BUFFER, sizeof(centerLines), centerLines, GL_STATIC_DRAW);

    // Draw the lines from midpoints to center
    glBindBuffer(GL_ARRAY_BUFFER, centerLinesBuffer);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, (void*)0);
    glDrawArrays(GL_LINES, 0, 16); // Draw lines from midpoints to center

    // Clean up
    glDeleteBuffers(1, &centerLinesBuffer); // Delete temporary buffer

    int vezes = numIterations * 4;

    // Draw lines between InterpolatedPoints
    glBindBuffer(GL_ARRAY_BUFFER, interpolatedPointsBuffer); // Use buffer 
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, (void*)0);
    glEnableVertexAttribArray(0); // attribute on
    glDrawArrays(GL_LINE_STRIP, 0, vezes); // Draw Lines // strip  = loop menos o ultimo

    // Clean up
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
    // Initialise GLFW
    glfwInit();

    glfwWindowHint(GLFW_SAMPLES, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE); // To make MacOS happy; should not be needed
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    // Create a windowed mode window and its OpenGL context
    window = glfwCreateWindow(SCREEN_WIDTH, SCREEN_HEIGHT, "Trabalho 1 - Octagon fractal ", NULL, NULL);

    // Create window context
    glfwMakeContextCurrent(window);

    // Initialize GLEW
    glewExperimental = true; // Needed for core profile
    glewInit();

    // Ensure we can capture the escape key being pressed below
    glfwSetInputMode(window, GLFW_STICKY_KEYS, GL_TRUE);

    // White background
    glClearColor(0.0f, 0.0f, 0.0f, 0.0f);

    // transfer my data (vertices, colors, and shaders) to GPU side
    transferDataToGPUMemory();

    // render scene for each frame
    do
    {
        // drawing callback
        draw();


        // Make the Rotation of the Division

        float pi = glm::pi<float>();

        rotationAngle += glm::radians((float(360) / 8));

        if (rotationAngle >= 2 * pi) {
            rotationAngle = 0.0f;
        }

        // Swap buffers
        glfwSwapBuffers(window);

        // looking for events
        glfwPollEvents();

    } // Check if the ESC key was pressed or the window was closed
    while (glfwGetKey(window, GLFW_KEY_ESCAPE) != GLFW_PRESS &&
        glfwWindowShouldClose(window) == 0);

    // Cleanup VAO, VBOs, and shaders from GPU
    cleanupDataFromGPU();

    // Close OpenGL window and terminate GLFW
    glfwTerminate();

    return 0;
}