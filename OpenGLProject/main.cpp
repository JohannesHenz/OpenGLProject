#include <GL/glew.h>      // Für GLEW
#include <GLFW/glfw3.h>   // Für GLFW
#include <glm/glm.hpp>     // Für GLM (mathematische Operationen)
#include <glm/gtc/matrix_transform.hpp> // Für Matrixtransformationen
#include <glm/gtc/type_ptr.hpp>
#include <iostream>
#include "shader.h"
#include "texture_loader.h"

// Fenstergröße
const GLuint WIDTH = 800, HEIGHT = 600;

// Paddles
GLuint paddleWidth = 20, paddleHeight = 100;
GLfloat paddleSpeed = 0.5f;

// Ball
GLuint ballSize = 10;
GLfloat ballSpeed = 0.5f;
GLfloat ballDirectionX = 1.0f, ballDirectionY = 1.0f;

// Shader-Quellen (Vertex- und Fragment-Shader)
const char* vertexShaderSource = R"(
#version 330 core

layout(location = 0) in vec3 aPos; // Position des Vertex
layout(location = 1) in vec2 aTexCoord; // Texturkoordinaten

uniform mat4 model; // Model-Matrix
uniform mat4 projection; // Orthogonale Projektion

out vec2 TexCoord; // Weitergabe der Texturkoordinaten

void main() {
    gl_Position = projection * model * vec4(aPos, 1.0);
    TexCoord = aTexCoord;
}
)";

const char* fragmentShaderSource = R"(
#version 330 core

in vec2 TexCoord;

uniform sampler2D texture1;  // Basistexur
uniform sampler2D normalMap; // Normalmap

uniform vec3 lightPos; // Position der Lichtquelle
uniform vec3 lightColor; // Farbe des Lichts
uniform vec3 viewPos;  // Position des Betrachters (kann in 2D auf 0, 0, 1 gesetzt werden)

out vec4 FragColor;

void main() {
    // Basisfarben und Normalen
    vec3 color = texture(texture1, TexCoord).rgb;
    vec3 normal = texture(normalMap, TexCoord).rgb;
    normal = normalize(normal * 2.0 - 1.0); // Normalmap von [0,1] auf [-1,1] skalieren

    // Beleuchtungsvektoren
    vec3 lightDir = normalize(lightPos - vec3(TexCoord, 0.0));
    float diff = max(dot(normal, lightDir), 0.0);

    // Diffuse Beleuchtung
    vec3 diffuse = diff * lightColor;

    // Endfarbe
    vec3 result = (diffuse + vec3(0.1)) * color; // Ambient + Diffuse
    FragColor = vec4(result, 1.0);
}
)";

// Shader erstellen und kompilieren
GLuint CompileShader(const char* source, GLenum type) {
    GLuint shader = glCreateShader(type);
    glShaderSource(shader, 1, &source, nullptr);
    glCompileShader(shader);

    GLint success;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
    if (!success) {
        char infoLog[512];
        glGetShaderInfoLog(shader, 512, nullptr, infoLog);
        std::cerr << "Shader Compilation Failed\n" << infoLog << std::endl;
    }
    return shader;
}

// Shader-Programm erstellen
GLuint CreateShaderProgram() {
    GLuint vertexShader = CompileShader(vertexShaderSource, GL_VERTEX_SHADER);
    GLuint fragmentShader = CompileShader(fragmentShaderSource, GL_FRAGMENT_SHADER);

    GLuint shaderProgram = glCreateProgram();
    glAttachShader(shaderProgram, vertexShader);
    glAttachShader(shaderProgram, fragmentShader);
    glLinkProgram(shaderProgram);

    GLint success;
    glGetProgramiv(shaderProgram, GL_LINK_STATUS, &success);
    if (!success) {
        char infoLog[512];
        glGetProgramInfoLog(shaderProgram, 512, nullptr, infoLog);
        std::cerr << "Shader Program Linking Failed\n" << infoLog << std::endl;
    }

    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);

    return shaderProgram;
}

void ProcessInput(GLFWwindow* window) {
    // Steuerung für Spieler 1 (links)
    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) {
        paddleHeight += paddleSpeed;
    }
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) {
        paddleHeight -= paddleSpeed;
    }

    // Steuerung für Spieler 2 (rechts)
    if (glfwGetKey(window, GLFW_KEY_UP) == GLFW_PRESS) {
        paddleHeight += paddleSpeed;
    }
    if (glfwGetKey(window, GLFW_KEY_DOWN) == GLFW_PRESS) {
        paddleHeight -= paddleSpeed;
    }
}

// Ballbewegung
void UpdateBall() {
    // Ballbewegung
    if (ballDirectionY > 0 && ballSize >= HEIGHT) {
        ballDirectionY = -ballDirectionY;
    }
    if (ballDirectionY < 0 && ballSize <= 0) {
        ballDirectionY = -ballDirectionY;
    }

    if (ballDirectionX < 0 && ballSize <= 0) {
        ballDirectionX = -ballDirectionX;
    }
    if (ballDirectionX > 0 && ballSize >= WIDTH) {
        ballDirectionX = -ballDirectionX;
    }
}

int main() {
    // Initialisiere GLFW
    if (!glfwInit()) {
        std::cerr << "GLFW konnte nicht initialisiert werden!" << std::endl;
        return -1;
    }

    // Fenster erstellen
    GLFWwindow* window = glfwCreateWindow(WIDTH, HEIGHT, "Pong Game", nullptr, nullptr);
    if (!window) {
        std::cerr << "Fenster konnte nicht erstellt werden!" << std::endl;
        glfwTerminate();
        return -1;
    }

    glfwMakeContextCurrent(window);

    // GLEW initialisieren
    if (glewInit() != GLEW_OK) {
        std::cerr << "GLEW konnte nicht initialisiert werden!" << std::endl;
        return -1;
    }

    GLuint shaderProgram = CreateShaderProgram();
    GLuint VAO, VBO;

    // Vertices für das Paddle und den Ball
    GLfloat paddleVertices[] = {
        -0.5f, -0.5f, 0.0f,
         0.5f, -0.5f, 0.0f,
         0.5f,  0.5f, 0.0f,
        -0.5f,  0.5f, 0.0f
    };

    GLfloat ballVertices[] = {
        -0.5f, -0.5f, 0.0f,
         0.5f, -0.5f, 0.0f,
         0.5f,  0.5f, 0.0f,
        -0.5f,  0.5f, 0.0f
    };

    // VAO und VBO für Paddle und Ball
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);

    glBindVertexArray(VAO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(paddleVertices), paddleVertices, GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(GLfloat), (GLvoid*)0);
    glEnableVertexAttribArray(0);

    glUseProgram(shaderProgram);

    // Render-Loop
    while (!glfwWindowShouldClose(window)) {
        // Eingabe verarbeiten
        ProcessInput(window);

        // Bildschirm löschen
        glClear(GL_COLOR_BUFFER_BIT);

        // Modellmatrix für Paddle und Ball erstellen
        glm::mat4 model = glm::mat4(1.0f);
        model = glm::translate(model, glm::vec3(paddleWidth, paddleHeight, 0.0f));

        GLuint modelLoc = glGetUniformLocation(shaderProgram, "model");
        glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));

        glBindVertexArray(VAO);
        glDrawArrays(GL_QUADS, 0, 4); // Paddle zeichnen

        model = glm::mat4(1.0f);
        model = glm::translate(model, glm::vec3(ballSize, ballSize, 0.0f));

        // Update Ball
        UpdateBall();

        glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));

        glDrawArrays(GL_QUADS, 0, 4); // Ball zeichnen

        // Puffer tauschen
        glfwSwapBuffers(window);

        // Ereignisse verarbeiten
        glfwPollEvents();
    }

    // Aufräumen
    glfwTerminate();
    return 0;
}
