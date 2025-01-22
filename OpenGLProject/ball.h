#ifndef BALL_H
#define BALL_H

#include <GL/glew.h>
#include <glm/glm.hpp>    // Für die Position und Bewegungsrichtung des Balls
#include <glm/gtc/matrix_transform.hpp> // Für die Transformationen
#include <glm/gtc/type_ptr.hpp> // Für glm::value_ptr

class Ball {
public:
    Ball(float radius, glm::vec3 startPosition, glm::vec3 startVelocity);

    void Update(float deltaTime);  // Update die Ballbewegung
    void Draw(GLuint shaderProgram); // Zeichne den Ball
    void Reset(); // Setze den Ball zurück, wenn er das Spielfeld verlässt

    glm::vec3 position;
    glm::vec3 velocity;
    float radius;
private:
    GLuint VAO, VBO;
    GLuint textureID; // Ball-Textur
};

#endif
