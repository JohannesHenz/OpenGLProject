#include <GL/glew.h>
#include <glm/glm.hpp>
#include <glm/gtc/constants.hpp>
#include "paddle.h"
#include "ball.h"
#include "helpers.h"

// Kreis zeichnen
void drawCircle(Shader& shader, float radius) {
    const int segments = 36;
    float angleStep = glm::two_pi<float>() / segments;

    // Erstellt VAO und VBO für den Kreis
    GLuint VAO, VBO;
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);

    glBindVertexArray(VAO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);

    GLfloat* vertices = new GLfloat[segments * 2]; // 2 für x, y Koordinaten

    for (int i = 0; i < segments; ++i) {
        float angle = i * angleStep;
        vertices[i * 2] = radius * cos(angle);
        vertices[i * 2 + 1] = radius * sin(angle);
    }

    glBufferData(GL_ARRAY_BUFFER, sizeof(GLfloat) * segments * 2, vertices, GL_STATIC_DRAW);

    // Definiere die Vertex-Attribute
    shader.use();
    GLint posAttrib = glGetAttribLocation(shader.ID, "position");
    glVertexAttribPointer(posAttrib, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(GLfloat), (void*)0);
    glEnableVertexAttribArray(posAttrib);

    // Zeichne den Kreis
    glDrawArrays(GL_TRIANGLE_FAN, 0, segments);

    // Bereinigung
    delete[] vertices;
    glDeleteBuffers(1, &VBO);
    glDeleteVertexArrays(1, &VAO);
}

// Rechteck zeichnen
void drawRectangle(Shader& shader, glm::vec2 size) {
    GLfloat vertices[] = {
        -size.x, -size.y, 0.0f,  // linke untere Ecke
         size.x, -size.y, 0.0f,  // rechte untere Ecke
         size.x,  size.y, 0.0f,  // rechte obere Ecke
        -size.x,  size.y, 0.0f   // linke obere Ecke
    };

    GLuint VAO, VBO;
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);

    glBindVertexArray(VAO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

    shader.use();
    GLint posAttrib = glGetAttribLocation(shader.ID, "position");
    glVertexAttribPointer(posAttrib, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(GLfloat), (void*)0);
    glEnableVertexAttribArray(posAttrib);

    // Rechteck zeichnen
    glDrawArrays(GL_QUADS, 0, 4);

    glDeleteBuffers(1, &VBO);
    glDeleteVertexArrays(1, &VAO);
}

// Kollisionsprüfung zwischen Ball und Paddle
bool checkCollision(const Ball& ball, const Paddle& paddle) {
    bool collisionX = ball.position.x + ball.radius >= paddle.position.x &&
        ball.position.x - ball.radius <= paddle.position.x + paddle.width;

    bool collisionY = ball.position.y + ball.radius >= paddle.position.y &&
        ball.position.y - ball.radius <= paddle.position.y + paddle.height;

    return collisionX && collisionY;
}
