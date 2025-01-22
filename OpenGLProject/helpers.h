#ifndef HELPERS_H
#define HELPERS_H

#include <glm/glm.hpp>
#include "shader.h"

// Kreis zeichnen
void drawCircle(Shader& shader, float radius);

// Rechteck zeichnen
void drawRectangle(Shader& shader, glm::vec2 size);

// Kollisionsprüfung zwischen Ball und Paddle
bool checkCollision(const Ball& ball, const Paddle& paddle);

#endif
