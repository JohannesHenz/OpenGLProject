#ifndef PADDLE_H
#define PADDLE_H

#include <GL/glew.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp> // Für Transformationen
#include <glm/gtc/type_ptr.hpp> // Für glm::value_ptr

class Paddle {
public:
    Paddle(float width, float height, glm::vec3 startPosition);

    void Update(float deltaTime, int direction); // Update die Paddle-Bewegung (0 = keine Bewegung, -1 = nach unten, 1 = nach oben)
    void Draw(GLuint shaderProgram); // Zeichne das Paddle
    void SetPosition(glm::vec3 newPosition); // Setze die Position des Paddles

    glm::vec3 position;
    float width, height;
private:
    GLuint VAO, VBO;
    GLuint textureID; // Paddle-Textur
};

#endif
