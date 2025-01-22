#include "paddle.h"
#include <GLFW/glfw3.h>
#include <iostream>

Paddle::Paddle(float width, float height, glm::vec3 startPosition)
    : width(width), height(height), position(startPosition) {
    // Paddle-Textur laden
    glGenTextures(1, &textureID);
    glBindTexture(GL_TEXTURE_2D, textureID);

    // Hier kannst du deine Paddle-Textur laden (z.B. ein einfaches Rechteck als Textur)
    // Zum Beispiel: glTexImage2D(...) und andere Texturparameter setzen

    GLfloat vertices[] = {
        -width / 2, -height / 2, 0.0f,   // unten links
         width / 2, -height / 2, 0.0f,   // unten rechts
         width / 2,  height / 2, 0.0f,   // oben rechts
        -width / 2,  height / 2, 0.0f    // oben links
    };

    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);

    glBindVertexArray(VAO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(GLfloat), (GLvoid*)0);
    glEnableVertexAttribArray(0);
}

void Paddle::Update(float deltaTime, int direction) {
    // Wenn der Spieler nach oben oder unten steuert
    float moveSpeed = 1.0f; // Geschwindigkeit des Paddles

    if (direction == 1) {
        position.y += moveSpeed * deltaTime; // Paddle nach oben bewegen
    }
    else if (direction == -1) {
        position.y -= moveSpeed * deltaTime; // Paddle nach unten bewegen
    }

    // Verhindere, dass das Paddle das Spielfeld verlässt
    if (position.y - height / 2 < -1.0f) {
        position.y = -1.0f + height / 2; // Begrenze das Paddle an der unteren Spielfeldgrenze
    }
    if (position.y + height / 2 > 1.0f) {
        position.y = 1.0f - height / 2; // Begrenze das Paddle an der oberen Spielfeldgrenze
    }
}

void Paddle::Draw(GLuint shaderProgram) {
    glUseProgram(shaderProgram);

    // Modellmatrix für die Position des Paddles
    glm::mat4 model = glm::mat4(1.0f);
    model = glm::translate(model, position);

    GLuint modelLoc = glGetUniformLocation(shaderProgram, "model");
    glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));

    glBindTexture(GL_TEXTURE_2D, textureID); // Textur für das Paddle anwenden

    glBindVertexArray(VAO);
    glDrawArrays(GL_QUADS, 0, 4); // Zeichne das Paddle
    glBindVertexArray(0);
}

void Paddle::SetPosition(glm::vec3 newPosition) {
    position = newPosition;
}
