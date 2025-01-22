/*********************************************************
 * 2D Normal-Mapped Pong Example (Fixed Camera, No Shake)
 *
 * Features:
 *  - Ten textured power-up objects (falling from top)
 *  - Paddle movement (keyboard)
 *  - Ball bouncing
 *  - Shaders (vertex & fragment)
 *  - Normal-mapped background
 *  - The ball is a moving 2D light source
 *
 * Libraries needed: GLEW, GLFW, GLM
 * Also uses stb_image.h (header-only)
 *********************************************************/

#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>  // glm::ortho, etc.
#include <glm/gtc/type_ptr.hpp>          // glm::value_ptr
#include <iostream>
#include <vector>
#include <cstdlib>   // rand()
#include <ctime>     // time()

 //------------------ STB Image ---------------------
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

// Screen size (fixed camera covers [0..SCR_WIDTH, 0..SCR_HEIGHT])
static const int SCR_WIDTH = 1400;
static const int SCR_HEIGHT = 800;

/*********************************************************
 * Helper: loadFile() to read .glsl files from disk
 *********************************************************/
#include <fstream>
#include <sstream>
std::string loadFile(const char* path)
{
    std::ifstream file(path, std::ios::in | std::ios::binary);
    if (!file) {
        std::cerr << "Could not open file: " << path << std::endl;
        return "";
    }
    std::stringstream buffer;
    buffer << file.rdbuf();
    return buffer.str();
}

/*********************************************************
 * Shader compilation/linking
 *********************************************************/
static void checkCompileErrors(GLuint shader, const char* stage)
{
    GLint success;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
    if (!success)
    {
        char infoLog[512];
        glGetShaderInfoLog(shader, 512, NULL, infoLog);
        std::cerr << "Shader " << stage << " compile error:\n"
            << infoLog << std::endl;
    }
}
static void checkLinkErrors(GLuint program)
{
    GLint success;
    glGetProgramiv(program, GL_LINK_STATUS, &success);
    if (!success)
    {
        char infoLog[512];
        glGetProgramInfoLog(program, 512, NULL, infoLog);
        std::cerr << "Program link error:\n" << infoLog << std::endl;
    }
}

GLuint createShaderProgram(const std::string& vsCode, const std::string& fsCode)
{
    // Vertex shader
    GLuint vs = glCreateShader(GL_VERTEX_SHADER);
    const char* vsrc = vsCode.c_str();
    glShaderSource(vs, 1, &vsrc, NULL);
    glCompileShader(vs);
    checkCompileErrors(vs, "VERTEX");

    // Fragment shader
    GLuint fs = glCreateShader(GL_FRAGMENT_SHADER);
    const char* fsrc = fsCode.c_str();
    glShaderSource(fs, 1, &fsrc, NULL);
    glCompileShader(fs);
    checkCompileErrors(fs, "FRAGMENT");

    // Link
    GLuint prog = glCreateProgram();
    glAttachShader(prog, vs);
    glAttachShader(prog, fs);
    glLinkProgram(prog);
    checkLinkErrors(prog);

    // Cleanup
    glDeleteShader(vs);
    glDeleteShader(fs);
    return prog;
}

/*********************************************************
 * loadTexture() - uses stb_image to load .png/.jpg
 *********************************************************/
GLuint loadTexture(const char* filePath, bool flipVert = true)
{
    stbi_set_flip_vertically_on_load(flipVert);

    int w, h, n;
    unsigned char* data = stbi_load(filePath, &w, &h, &n, 0);
    if (!data)
    {
        std::cerr << "Failed to load texture: " << filePath << std::endl;
        return 0;
    }

    GLuint texID;
    glGenTextures(1, &texID);
    glBindTexture(GL_TEXTURE_2D, texID);

    GLenum format = GL_RGB;
    if (n == 1) format = GL_RED;
    else if (n == 3) format = GL_RGB;
    else if (n == 4) format = GL_RGBA;

    glTexImage2D(GL_TEXTURE_2D, 0, format, w, h, 0, format, GL_UNSIGNED_BYTE, data);
    glGenerateMipmap(GL_TEXTURE_2D);

    // default texture parameters
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    stbi_image_free(data);
    return texID;
}

/*********************************************************
 * Simple 2D GameObject
 *********************************************************/
struct GameObject {
    float x, y, w, h;   // position & size
    float vx, vy;       // velocity
    GLuint texture;     // diffuse texture
    bool  useNormalMap;
    GLuint normalTex;

    GameObject(float px, float py, float pw, float ph,
        GLuint tex, bool nm = false, GLuint ntex = 0)
        : x(px), y(py), w(pw), h(ph),
        vx(0.f), vy(0.f),
        texture(tex), useNormalMap(nm), normalTex(ntex)
    {}
};

// We'll have a background, paddles, ball, and 10 powerups
GameObject* gBackground = nullptr;
GameObject* gLeftPaddle = nullptr;
GameObject* gRightPaddle = nullptr;
GameObject* gBall = nullptr;
std::vector<GameObject> gPowerUps;

// Basic quad VAO for drawing
GLuint gVAO = 0, gVBO = 0;

// The shader program & uniform locs
GLuint gShaderProgram;
GLint  gUniMVP, gUniDiffuse, gUniNormal, gUniUseNormalMap;
GLint  gUniLightPos, gUniResolution;

/*********************************************************
 * createQuad() - a 1x1 quad from (0,0)->(1,1)
 *********************************************************/
void createQuad()
{
    float vertices[] = {
        // aPos     aTexCoord
        0.0f,0.0f,  0.0f,0.0f,
        1.0f,0.0f,  1.0f,0.0f,
        1.0f,1.0f,  1.0f,1.0f,

        0.0f,0.0f,  0.0f,0.0f,
        1.0f,1.0f,  1.0f,1.0f,
        0.0f,1.0f,  0.0f,1.0f
    };

    glGenVertexArrays(1, &gVAO);
    glGenBuffers(1, &gVBO);

    glBindVertexArray(gVAO);
    glBindBuffer(GL_ARRAY_BUFFER, gVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

    // aPos -> location=0
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    // aTexCoord -> location=1
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)(2 * sizeof(float)));
    glEnableVertexAttribArray(1);

    glBindVertexArray(0);
}

/*********************************************************
 * drawObject() - sets up MVP, binds textures, draws quad
 *********************************************************/
void drawObject(const GameObject& obj, const glm::mat4& pv)
{
    // Bind diffuse texture
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, obj.texture);

    bool useNM = obj.useNormalMap && obj.normalTex != 0;
    // If using normal map
    if (useNM)
    {
        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, obj.normalTex);
    }
    glUniform1i(gUniUseNormalMap, (useNM ? 1 : 0));

    // Build model matrix from (x,y)->(x+w,y+h)
    glm::mat4 model = glm::translate(glm::mat4(1.0f),
        glm::vec3(obj.x, obj.y, 0.f));
    model = glm::scale(model, glm::vec3(obj.w, obj.h, 1.0f));

    // Multiply with projection*view
    glm::mat4 mvp = pv * model;
    glUniformMatrix4fv(gUniMVP, 1, GL_FALSE, glm::value_ptr(mvp));

    // Draw
    glBindVertexArray(gVAO);
    glDrawArrays(GL_TRIANGLES, 0, 6);
    glBindVertexArray(0);
}

/*********************************************************
 * initGame() - load textures, create objects
 *********************************************************/
void initGame()
{
    // 1) Load textures for background (with normal map),
    //    ball, paddle, powerup
    GLuint bgDiffuse = loadTexture("../resources/background_diffuse.png");
    GLuint bgNormal = loadTexture("../resources/background_normal.png");

    GLuint paddleTex = loadTexture("../resources/paddle_diffuse.png");
    GLuint ballTex = loadTexture("../resources/ball_diffuse.png");
    GLuint powerTex = loadTexture("../resources/powerup.png");

    // 2) Create the background
    gBackground = new GameObject(
        0, 0,
        (float)SCR_WIDTH, (float)SCR_HEIGHT,
        bgDiffuse, true, bgNormal);

    // 3) Create left + right paddles
    gLeftPaddle = new GameObject(
        50.0f, (SCR_HEIGHT / 2 - 50),
        20.0f, 150.0f,
        paddleTex);
    gRightPaddle = new GameObject(
        (SCR_WIDTH - 70.0f), (SCR_HEIGHT / 2 - 50),
        20.0f, 150.0f,
        paddleTex);

    // 4) Create the ball
    gBall = new GameObject(
        (SCR_WIDTH / 2 - 15), (SCR_HEIGHT / 2 - 15),
        30.0f, 30.0f,
        ballTex);
    gBall->vx = 900.0f;
    gBall->vy = 700.0f;

    // 5) Create 1 falling power-ups
    float px = 50 + rand() % (SCR_WIDTH - 100);
    float py = SCR_HEIGHT +  50.0f;
    GameObject p(px, py, 32.0f, 32.0f, powerTex);
    p.vy = -0.001f - (rand() % 3); // fall speed
    gPowerUps.push_back(p);
}

/*********************************************************
 * Update logic for paddles, ball, power-ups
 *********************************************************/
void updatePaddles(GLFWwindow* w, float dt)
{
    // left paddle -> up/down
    if (glfwGetKey(w, GLFW_KEY_W) == GLFW_PRESS)
        gLeftPaddle->y += 800 * dt;
    if (glfwGetKey(w, GLFW_KEY_S) == GLFW_PRESS)
        gLeftPaddle->y -= 800 * dt;

    // right paddle -> W/S
    if (glfwGetKey(w, GLFW_KEY_UP) == GLFW_PRESS)
        gRightPaddle->y += 800 * dt;
    if (glfwGetKey(w, GLFW_KEY_DOWN) == GLFW_PRESS)
        gRightPaddle->y -= 800 * dt;

    // clamp
    if (gLeftPaddle->y < 0) gLeftPaddle->y = 0;
    if (gLeftPaddle->y + gLeftPaddle->h > SCR_HEIGHT)
        gLeftPaddle->y = SCR_HEIGHT - gLeftPaddle->h;

    if (gRightPaddle->y < 0) gRightPaddle->y = 0;
    if (gRightPaddle->y + gRightPaddle->h > SCR_HEIGHT)
        gRightPaddle->y = SCR_HEIGHT - gRightPaddle->h;
}

void updateBall(float dt)
{
    // move
    gBall->x += gBall->vx * dt;
    gBall->y += gBall->vy * dt;

    // top/bottom
    if (gBall->y < 0) {
        gBall->y = 0;
        gBall->vy *= -1;
    }
    if (gBall->y + gBall->h > SCR_HEIGHT) {
        gBall->y = SCR_HEIGHT - gBall->h;
        gBall->vy *= -1;
    }

    // left/right
    if (gBall->x < 0) {
        gBall->x = SCR_WIDTH / 2 - 15;
        gBall->y = SCR_HEIGHT / 2 - 15;
        gBall->vx = 900.0f;
        gBall->vy = 700.0f;
    }
    if (gBall->x + gBall->w > SCR_WIDTH) {
        gBall->x = SCR_WIDTH / 2 - 15;
        gBall->y = SCR_HEIGHT / 2 - 15;
        gBall->vx = -900.0f;
        gBall->vy = 700.0f;
    }

    // collisions with paddles
    // left
    if (gBall->x < gLeftPaddle->x + gLeftPaddle->w &&
        gBall->x + gBall->w > gLeftPaddle->x &&
        gBall->y < gLeftPaddle->y + gLeftPaddle->h &&
        gBall->y + gBall->h > gLeftPaddle->y)
    {
        gBall->x = gLeftPaddle->x + gLeftPaddle->w;
        gBall->vx *= -1;
    }
    // right
    if (gBall->x + gBall->w > gRightPaddle->x &&
        gBall->x < gRightPaddle->x + gRightPaddle->w &&
        gBall->y < gRightPaddle->y + gRightPaddle->h &&
        gBall->y + gBall->h > gRightPaddle->y)
    {
        gBall->x = gRightPaddle->x - gBall->w;
        gBall->vx *= -1;
    }
}

void updatePowerUps(float dt)
{
    for (auto& p : gPowerUps)
    {
        p.x += p.vx;
        p.y += p.vy;

        // off bottom -> reset to top
        if (p.y + p.h < 0) {
            p.x = 50 + rand() % (SCR_WIDTH - 100);
            p.y = SCR_HEIGHT + 50.0f;
        }

        // collision with ball -> we won't do camera shake, just reset
        if (gBall->x < p.x + p.w &&
            gBall->x + gBall->w > p.x &&
            gBall->y < p.y + p.h &&
            gBall->y + gBall->h > p.y)
        {
            // Just reset the power-up
            p.x = 50 + rand() % (SCR_WIDTH - 100);
            p.y = SCR_HEIGHT + 50.0f;
        }
    }
}

void updateAll(GLFWwindow* w, float dt)
{
    updatePaddles(w, dt);
    updateBall(dt);
    updatePowerUps(dt);
}

/*********************************************************
 * main
 *********************************************************/
int main()
{
    srand((unsigned)time(NULL));

    if (!glfwInit())
    {
        std::cerr << "Failed to init GLFW\n";
        return -1;
    }
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    GLFWwindow* window = glfwCreateWindow(SCR_WIDTH, SCR_HEIGHT,
        "Normal-Mapped Pong (Fixed Camera)",
        NULL, NULL);
    if (!window)
    {
        std::cerr << "Failed to create window\n";
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);

    // init GLEW
    GLenum err = glewInit();
    if (err != GLEW_OK)
    {
        std::cerr << "GLEW init error: "
            << glewGetErrorString(err) << std::endl;
        return -1;
    }

    // alpha blending
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    // 1) Load the .glsl files for vertex/fragment
    std::string vsCode = loadFile("../shaders/vertexShader.glsl");
    std::string fsCode = loadFile("../shaders/fragmentShader.glsl");
    gShaderProgram = createShaderProgram(vsCode, fsCode);
    glUseProgram(gShaderProgram);

    // get uniform locations
    gUniMVP = glGetUniformLocation(gShaderProgram, "uMVP");
    gUniDiffuse = glGetUniformLocation(gShaderProgram, "uDiffuse");
    gUniNormal = glGetUniformLocation(gShaderProgram, "uNormal");
    gUniUseNormalMap = glGetUniformLocation(gShaderProgram, "uUseNormalMap");
    gUniLightPos = glGetUniformLocation(gShaderProgram, "uLightPos");
    gUniResolution = glGetUniformLocation(gShaderProgram, "uResolution");

    // set the sampler2D indices
    glUniform1i(gUniDiffuse, 0); // Diffuse = texture unit 0
    glUniform1i(gUniNormal, 1); // Normal  = texture unit 1

    // pass resolution (for the FS lighting)
    glUniform2f(gUniResolution, (float)SCR_WIDTH, (float)SCR_HEIGHT);

    // 2) Create a 1x1 quad
    createQuad();

    // 3) Initialize the game objects (load textures, etc.)
    initGame();

    // main loop
    float lastTime = (float)glfwGetTime();
    while (!glfwWindowShouldClose(window))
    {
        float now = (float)glfwGetTime();
        float dt = now - lastTime;
        lastTime = now;

        // Verlangsamen des Spiels, indem dt mit einem Faktor multipliziert wird
        // Du kannst den Faktor anpassen, um die Geschwindigkeit zu verändern
        dt *= 0.5f; // Faktor, um das Spiel langsamer zu machen (Halbierung der Geschwindigkeit)

        glfwPollEvents();
        if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
            glfwSetWindowShouldClose(window, true);

        // update logic
        updateAll(window, dt);

        // Clear
        glClearColor(0.1f, 0.1f, 0.15f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);

        // 4) The ball is our light source
        float ballCenterX = gBall->x + gBall->w * 0.5f;
        float ballCenterY = gBall->y + gBall->h * 0.5f;
        glUseProgram(gShaderProgram);
        glUniform2f(gUniLightPos, ballCenterX, ballCenterY);

        // 5) Build a 2D orthographic projection from (0..SCR_WIDTH, 0..SCR_HEIGHT)
        glm::mat4 proj = glm::ortho(
            0.0f, (float)SCR_WIDTH,
            0.0f, (float)SCR_HEIGHT,
            -1.0f, 1.0f
        );
        glm::mat4 view = glm::mat4(1.0f);
        glm::mat4 pv = proj * view;

        // 6) Draw background
        drawObject(*gBackground, pv);

        // 7) Draw paddles
        drawObject(*gLeftPaddle, pv);
        drawObject(*gRightPaddle, pv);

        // 8) Draw ball
        drawObject(*gBall, pv);

        // 9) Draw power-ups
        for (auto& p : gPowerUps)
            drawObject(p, pv);

        glfwSwapBuffers(window);
    }

    // cleanup
    glfwDestroyWindow(window);
    glfwTerminate();
    return 0;
}

