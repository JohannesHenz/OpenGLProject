#ifndef SHADER_H
#define SHADER_H

#include <GL/glew.h>
#include <string>
#include <glm/glm.hpp>
#include <gl/GL.h>


class Shader {
public:
    GLuint ID; // ID des Shader-Programms

    // Konstruktor: Lädt und kompiliert Shader
    Shader(const char* vertexPath, const char* fragmentPath);

    // Aktiviert den Shader
    void use();

    // Utility-Funktionen zum Setzen von Uniforms
    void setBool(const std::string& name, bool value) const;
    void setInt(const std::string& name, int value) const;
    void setFloat(const std::string& name, float value) const;
    void setVec2(const std::string& name, const glm::vec2& value) const;
    void setVec3(const std::string& name, const glm::vec3& value) const;
    void setMat4(const std::string& name, const glm::mat4& mat) const;
};

#endif
