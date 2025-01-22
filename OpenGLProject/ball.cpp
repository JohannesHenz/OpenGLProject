#include "ball.h"
#include <GLFW/glfw3.h>
#include <iostream>

Ball::Ball(float radius, glm::vec3 startPosition, glm::vec3 startVelocity)
    : radius(radius), position(startPosition), velocity(startVelocity) {
    // Ball-Textur laden
    glGenTextures(1, &textureID);
    glBindTexture(GL_TEXTURE_2D, textureID);

    // Hier könntest du deine Textur laden (z.B. Ball-Textur)
    // Zum Beispiel: glTexImage2D(...) und andere Texturparameter setzen

    GLfloat vertices[] = {
        -radius, -radius, 0.0f,
         radius, -radius, 0.0f,
         radius,  radius, 0.0f,
        -radius,  radius, 0.0f
    };

    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);

    glBindVertexArray(VAO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(GLfloat), (GLvoid*)0);
    glEnableVertexAttribArray(0);
}

void Ball::Update(float deltaTime) {
    position += velocity * deltaTime;

    // Kollisionsbehandlung mit den Wänden
    if (position.x - radius <= -1.0f || position.x + radius >= 1.0f) {
        velocity.x = -velocity.x; // Ball prallt von der Wand ab
    }

    if (position.y - radius <= -1.0f || position.y + radius >= 1.0f) {
        velocity.y = -velocity.y; // Ball prallt von der Wand ab
    }
}

void Ball::Draw(GLuint shaderProgram) {
    glUseProgram(shaderProgram);

    // Modellmatrix für die Position des Balls erstellen
    glm::mat4 model = glm::mat4(1.0f);
    model = glm::translate(model, position);

    GLuint modelLoc = glGetUniformLocation(shaderProgram, "model");
    glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));

    glBindTexture(GL_TEXTURE_2D, textureID);

    glBindVertexArray(VAO);
    glDrawArrays(GL_QUADS, 0, 4); // Ball zeichnen
    glBindVertexArray(0);
}

void Ball::Reset() {
    // Setze die Ballposition und Geschwindigkeit zurück
    position = glm::vec3(0.0f, 0.0f, 0.0f);
    velocity = glm::vec3(0.01f, 0.01f, 0.0f); // Beispiel für eine Startgeschwindigkeit
}
