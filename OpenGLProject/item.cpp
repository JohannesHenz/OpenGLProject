#include "item.h"
#include <iostream>

// Konstruktor
Item::Item(glm::vec3 position, GLfloat size, GLuint textureID)
    : position(position), size(size), textureID(textureID) {
}

// Methode zum Zeichnen des Items
void Item::Draw(GLuint shaderProgram) {
    // Bereite die Vertices für das Item (Rechteck)
    GLfloat vertices[] = {
        position.x - size / 2, position.y - size / 2, 0.0f,   // unten links
        position.x + size / 2, position.y - size / 2, 0.0f,   // unten rechts
        position.x + size / 2, position.y + size / 2, 0.0f,   // oben rechts
        position.x - size / 2, position.y + size / 2, 0.0f    // oben links
    };

    GLuint VAO, VBO;
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);

    glBindVertexArray(VAO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(GLfloat), (GLvoid*)0);
    glEnableVertexAttribArray(0);

    glUseProgram(shaderProgram);

    // Modellmatrix für die Position
    glm::mat4 model = glm::mat4(1.0f);
    model = glm::translate(model, position);

    // Übergebe die Modellmatrix an den Shader
    GLuint modelLoc = glGetUniformLocation(shaderProgram, "model");
    glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));

    // Setze Textur
    glBindTexture(GL_TEXTURE_2D, textureID);

    // Zeichne das Item
    glDrawArrays(GL_QUADS, 0, 4);

    glBindVertexArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
}

// Methode zur Aktualisierung der Position des Items
void Item::UpdatePosition(GLfloat deltaTime) {
    // Hier könnte man z.B. das Item nach unten fallen lassen
    position.y -= 50.0f * deltaTime;  // Geschwindigkeit in Y-Richtung anpassen
}
