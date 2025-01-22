#ifndef ITEM_H
#define ITEM_H

#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>  // Für Vektor und Matrizenoperationen
#include <glm/gtc/matrix_transform.hpp> // Für Transformationen
#include <glm/gtc/type_ptr.hpp>


class Item {
public:
    glm::vec3 position;  // Position des Items im 3D Raum
    GLfloat size;        // Größe des Items
    GLuint textureID;    // ID der Textur des Items

    Item(glm::vec3 position, GLfloat size, GLuint textureID);

    void Draw(GLuint shaderProgram);
    void UpdatePosition(GLfloat deltaTime);
};

#endif
