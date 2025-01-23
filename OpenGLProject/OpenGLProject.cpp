#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp> 
#include <glm/gtc/type_ptr.hpp>
#include <iostream>
#include <vector>
#include <string>
#include <ctime>
#include <cstdlib>
#include <fstream>
#include <sstream>

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

// Forward declaration
void updatePaddleSizeAndPosition();
void updateBallSize();
void updatePowerUpSize();

 // Window size (adjustable via callback)
static int gWindowWidth = 1400;
static int gWindowHeight = 800;

//Global Variable for Powerups
int gCurrentPowerUp = -1;

// Callback for window resizing
void framebuffer_size_callback(GLFWwindow* window, int width, int height)
{
    gWindowWidth = width;
    gWindowHeight = height;
    glViewport(0, 0, width, height);
    

    // Resize props
    updatePaddleSizeAndPosition();
    updateBallSize();
    updatePowerUpSize();
}

/*********************************************************
 * Helper: loadFile() to read .glsl files from disk
 *********************************************************/
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
    // Safeguard: if either is empty, bail out
    if (vsCode.empty()) {
        std::cerr << "ERROR: Vertex shader code is empty!\n";
        std::exit(1);
    }
    if (fsCode.empty()) {
        std::cerr << "ERROR: Fragment shader code is empty!\n";
        std::exit(1);
    }

    GLuint vs = glCreateShader(GL_VERTEX_SHADER);
    const char* vsrc = vsCode.c_str();
    glShaderSource(vs, 1, &vsrc, NULL);
    glCompileShader(vs);
    checkCompileErrors(vs, "VERTEX");

    GLuint fs = glCreateShader(GL_FRAGMENT_SHADER);
    const char* fsrc = fsCode.c_str();
    glShaderSource(fs, 1, &fsrc, NULL);
    glCompileShader(fs);
    checkCompileErrors(fs, "FRAGMENT");

    GLuint prog = glCreateProgram();
    glAttachShader(prog, vs);
    glAttachShader(prog, fs);
    glLinkProgram(prog);
    checkLinkErrors(prog);

    glDeleteShader(vs);
    glDeleteShader(fs);

    return prog;
}

/*********************************************************
 * Load Texture - uses stb_image to load images
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
    if (n == 1)      format = GL_RED;
    else if (n == 3) format = GL_RGB;
    else if (n == 4) format = GL_RGBA;

    glTexImage2D(GL_TEXTURE_2D, 0, format, w, h, 0, format, GL_UNSIGNED_BYTE, data);
    glGenerateMipmap(GL_TEXTURE_2D);

    // Default texture params
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    stbi_image_free(data);

    // Debug logging
    std::cout << "Loaded texture '" << filePath << "' -> TexID=" << texID
        << " (w=" << w << ",h=" << h << ",chan=" << n << ")\n";
    return texID;
}

/*********************************************************
 * Basic object pattern for all kinds of objects (paddle, ball, background, powerup)
 *********************************************************/
struct GameObject {
    float x, y, w, h;
    float vx, vy;
    GLuint texture;
    bool  useNormalMap;
    GLuint normalTex;

    bool  isPowerUp = false;
    int   powerUpType = 0; 
    // 0->speed up, 1->invert ball, 2->enlarge, 3->minimize, 4->decrease speed

    GameObject(float px, float py, float pw, float ph,
        GLuint tex, bool nm = false, GLuint ntex = 0)
        : x(px), y(py), w(pw), h(ph),
        vx(0.f), vy(0.f),
        texture(tex), useNormalMap(nm), normalTex(ntex)
    {}
};

// Score
int scoreLeft = 0, scoreRight = 0;

// Paddles
GameObject* gLeftPaddle = nullptr;
GameObject* gRightPaddle = nullptr;

// Ball
struct BallObject : public GameObject {
    int lastTouched; 
    // -1=none, 0=left, 1=right
    BallObject(float px, float py, float pw, float ph, GLuint tex)
        : GameObject(px, py, pw, ph, tex, false, 0),
        lastTouched(-1)
    {}
};
BallObject* gBall = nullptr;

// Powerups
std::vector<GameObject> gPowerUps; 
//3 in total -> adustable in function initGame()

// Background
GameObject* gBackground = nullptr;

// VAO for rendering
GLuint gVAO = 0, gVBO = 0;

// Shader + uniforms
GLuint gShaderProgram;
GLint  gUniMVP, gUniDiffuse, gUniNormal, gUniUseNormalMap;
GLint  gUniLightPos, gUniResolution;

/*********************************************************
 * Score & Message Rendering
 *********************************************************/
GLuint gDigitTextures[10];
GLuint gMsgTexSpeed = 0;
GLuint gMsgTexSpawn = 0;
GLuint gMsgTexEnlarge = 0;
GLuint gMsgTexMinimize = 0;
GLuint gMsgTexSpeedDecrease = 0;

// Active messages on screen
struct ActiveMessage {
    GLuint texture;
    float timer;   
    // how long to remain
};

std::vector<ActiveMessage> gMessages;

/*********************************************************
 * DrawQuadTexture - For text or messages
 *********************************************************/
void drawQuadTexture(GLuint tex, float x, float y, float w, float h, const glm::mat4& pv)
{
    // Check if the texture is initialized
    if (tex == 0) {
        std::cerr << "WARNING: drawQuadTexture called with tex=0!\n";
        return;
    }

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, tex);

    // For text, no normal map
    glUniform1i(gUniUseNormalMap, 0);

    glm::mat4 model = glm::translate(glm::mat4(1.0f), glm::vec3(x, y, 0.f));
    model = glm::scale(model, glm::vec3(w, h, 1.f));
    glm::mat4 mvp = pv * model;
    glUniformMatrix4fv(gUniMVP, 1, GL_FALSE, glm::value_ptr(mvp));

    glBindVertexArray(gVAO);
    glDrawArrays(GL_TRIANGLES, 0, 6);
    glBindVertexArray(0);
}

/*********************************************************
 * Draw Numbers 
 *********************************************************/
void drawNumber(int number, float x, float y, float digitW, float digitH, const glm::mat4& pv)
{
    if (number < 0) number = 0;
    std::string s = std::to_string(number);

    float offset = 0.f;
    for (char c : s)
    {
        int d = c - '0';
        if (d < 0 || d>9) {
            // if out of range, default to 0
            d = 0;
        }
        // check the texture
        GLuint tex = gDigitTextures[d];
        if (!tex) {
            std::cerr << "WARNING: digit texture for '" << d << "' is 0!\n";
            continue;
        }
        drawQuadTexture(tex, x + offset, y, digitW, digitH, pv);
        offset += digitW + 5.f;
    }
}

GLuint getPowerUpIcon(int type) {
	if (type == 0) return gMsgTexSpeed;
	else if (type == 1) return gMsgTexSpawn;
	else if (type == 2) return gMsgTexEnlarge;
	else if (type == 3) return gMsgTexMinimize;
	else if (type == 4) return gMsgTexSpeedDecrease;
	return 0;
}

/*********************************************************
 * ShowPowerUpMessage
 *********************************************************/
void showPowerUpMessage(int powerType, int side)
{
    GLuint tex = 0;
    if (powerType == 0)      tex = gMsgTexSpeed;
    else if (powerType == 1) tex = gMsgTexSpawn;
    else if (powerType == 2) tex = gMsgTexEnlarge;
    else if (powerType == 3) tex = gMsgTexMinimize;
    else if (powerType == 4) tex = gMsgTexSpeedDecrease;

    if (tex != 0)
    {
        ActiveMessage msg{ tex, 2.0f };
        gMessages.push_back(msg);
    }
    else {
        std::cerr << "WARNING: No message texture for powerType=" << powerType << "\n";
    }
}

/*********************************************************
 * Create Quad
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
 * Draw Objects - for paddles, ball, background, powerUP
 *********************************************************/
void drawObject(const GameObject& obj, const glm::mat4& pv)
{
    // check texture
    if (obj.texture == 0) {
        std::cerr << "WARNING: object texture=0, skipping draw...\n";
        return;
    }

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, obj.texture);

    bool useNM = (obj.useNormalMap && obj.normalTex != 0);
    if (useNM)
    {
        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, obj.normalTex);
    }
    glUniform1i(gUniUseNormalMap, (useNM ? 1 : 0));

    glm::mat4 model = glm::translate(glm::mat4(1.0f), glm::vec3(obj.x, obj.y, 0.f));
    model = glm::scale(model, glm::vec3(obj.w, obj.h, 1.f));
    glm::mat4 mvp = pv * model;

    glUniformMatrix4fv(gUniMVP, 1, GL_FALSE, glm::value_ptr(mvp));

    glBindVertexArray(gVAO);
    glDrawArrays(GL_TRIANGLES, 0, 6);
    glBindVertexArray(0);
}

/*********************************************************
 * Draw Active PowerUp Indicator
 *********************************************************/
void drawActivePowerUpIndicator(const glm::mat4& pv)
{
    if (gCurrentPowerUp < 0) return; // No current power‐up
    GLuint iconTex = getPowerUpIcon(gCurrentPowerUp);
    if (!iconTex) return; // no icon for this type

    // Placement
    float iconW = 150.0f;
    float iconH = 150.0f;

    float x = (gWindowWidth * 0.5f) - (iconW * 0.5f);
    float y = gWindowHeight - iconH - 10.0f; // 10 px down from top

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, iconTex);
    glUniform1i(gUniUseNormalMap, 0);

    // Matrix calculation
    glm::mat4 model = glm::translate(glm::mat4(1.0f), glm::vec3(x, y, 0.f));
    model = glm::scale(model, glm::vec3(iconW, iconH, 1.f));
    glm::mat4 mvp = pv * model;
    glUniformMatrix4fv(gUniMVP, 1, GL_FALSE, glm::value_ptr(mvp));

    glBindVertexArray(gVAO);
    glDrawArrays(GL_TRIANGLES, 0, 6);
    glBindVertexArray(0);
}


/*********************************************************
 * Dynamic Speeds (Resizing)
 *********************************************************/

float paddleModificator = 0.5f;
float ballModificatorX = 0.3f;
float ballModificatorY = 0.24f;

static float PADDLE_SPEED = gWindowHeight * paddleModificator;
static float BALL_SPEED_X = gWindowWidth * ballModificatorX;
static float BALL_SPEED_Y = gWindowWidth * ballModificatorY;

/*********************************************************
 * InitGame
 *********************************************************/
void initGame()
{
    std::cout << "Init Game Started.\n";

    // Load textures
    GLuint bgDiffuse = loadTexture("../resources/background_diffuse.png");
    GLuint bgNormal = loadTexture("../resources/background_normal.png");
    GLuint paddleTex = loadTexture("../resources/paddle_diffuse.png");
    GLuint ballTex = loadTexture("../resources/ball_diffuse.png");
    GLuint powerTex = loadTexture("../resources/powerup.png");

    // If any critical ones are 0, exit game
    if (!bgDiffuse || !bgNormal || !paddleTex || !ballTex || !powerTex) {
        std::cerr << "ERROR: A critical texture didn't load. Exiting.\n";
        std::exit(1);
    }

    // Digit textures
    for (int i = 0; i < 10; i++)
    {
        std::string path = "../resources/digit" + std::to_string(i) + ".png";
        gDigitTextures[i] = loadTexture(path.c_str());
        if (!gDigitTextures[i]) {
            std::cerr << "WARNING: digit" << i << " missing or 0.\n";
        }
    }

    // Message textures
    std::cout << "Loading Speed Powerup:\n";
    gMsgTexSpeed = loadTexture("../resources/msg_speed.png");
    std::cout << "Speed Powerup Texture loaded.\n";
    std::cout << "Loading Spawn Powerup.\n";
    gMsgTexSpawn = loadTexture("../resources/msg_spawn.png");
    std::cout << "Ball Spawn Texture loaded.\n";
    std::cout << "Loading Enlarge Powerup\n";
    gMsgTexEnlarge = loadTexture("../resources/msg_enlarge.png");
    std::cout << "Enlarge Texture loaded.\n";
    std::cout << "Loading Minimize Powerup\n";
    gMsgTexMinimize = loadTexture("../resources/msg_minimize.png");
    std::cout << "Minimize Texture loaded.\n";
    std::cout << "Loading SpeedDecrease Powerup\n";
    gMsgTexSpeedDecrease = loadTexture("../resources/msg_slowdown.png");
    std::cout << "SpeedDecrease Texture loaded.\n";

    if (!gMsgTexSpeed)   std::cerr << "WARNING: msg_speed.png missing.\n";
    if (!gMsgTexSpawn)   std::cerr << "WARNING: msg_spawn.png missing.\n";
    if (!gMsgTexEnlarge) std::cerr << "WARNING: msg_enlarge.png missing.\n";
    if (!gMsgTexMinimize) std::cerr << "WARNING: msg_minimize.png missing.\n";
    if (!gMsgTexSpeedDecrease) std::cerr << "WARNING: msg_speedDecrease.png missing.\n";

    std::cout << "All textures loaded successfully.\n";

    // Background
    gBackground = new GameObject(0, 0, (float)gWindowWidth, (float)gWindowHeight,
        bgDiffuse, true, bgNormal);
    if (!gBackground) {
        std::cerr << "ERROR: gBackground new failed.\n";
        std::exit(1);
    }
    std::cout << "Background initialized.\n";

    // Paddles
    gLeftPaddle = new GameObject(50, (gWindowHeight / 2 - 50), 20, 120, paddleTex);
    if (!gLeftPaddle) {
        std::cerr << "ERROR: gLeftPaddle new failed.\n";
        std::exit(1);
    }
    std::cout << "Left Paddle initialized.\n";

    gRightPaddle = new GameObject(gWindowWidth - 70, (gWindowHeight / 2 - 50),
        20, 120, paddleTex);
    if (!gRightPaddle) {
        std::cerr << "ERROR: gRightPaddle new failed.\n";
        std::exit(1);
    }
    std::cout << "Right Paddle initialized.\n";

    // Ball
    gBall = new BallObject((gWindowWidth / 2 - 15), (gWindowHeight / 2 - 15),
        30, 30, ballTex);
    if (!gBall) {
        std::cerr << "ERROR: gBall new failed.\n";
        std::exit(1);
    }
    std::cout << "Ball initialized.\n";
    gBall->vx = BALL_SPEED_X;
    gBall->vy = BALL_SPEED_Y;
    gBall->lastTouched = -1;
    std::cout << "Ball Parameters set.\n";

    // Powerups => 3
    for (int i = 0; i < 3; i++)
    {
        float px = 100 + (rand() % (gWindowWidth - 100));
        float py = (float)gWindowHeight + i * 100.f;
        GameObject p(px, py, 32, 32, powerTex);
        p.vy = -250.f - (rand() % 100);
        p.isPowerUp = true;
        p.powerUpType = rand() % 5;
        gPowerUps.push_back(p);
    }
    std::cout << "Powerups initialized.\n";

    // Final pointer checks
    bool foundNull = false;
    if (gBackground == nullptr)
    {
        std::cerr << "Error: gBackground is not initialized.\n";
        foundNull = true;
    }
    if (gLeftPaddle == nullptr)
    {
        std::cerr << "Error: gLeftPaddle is not initialized.\n";
        foundNull = true;
    }
    if (gRightPaddle == nullptr)
    {
        std::cerr << "Error: gRightPaddle is not initialized.\n";
        foundNull = true;
    }
    if (gBall == nullptr)
    {
        std::cerr << "Error: gBall is not initialized.\n";
        foundNull = true;
    }
    if (foundNull)
    {
        std::exit(1);
    }

    std::cout << "initGame() finished successfully.\n";
}


/*********************************************************
 * Messages for power-ups
 *********************************************************/
void updateMessages(float dt)
{
    for (auto& msg : gMessages)
    {
        msg.timer -= dt;
    }
    gMessages.erase(
        std::remove_if(gMessages.begin(), gMessages.end(),
            [](const ActiveMessage& m) {return m.timer <= 0.f; }),
        gMessages.end()
    );
}

void drawMessages(const glm::mat4& pv)
{
    float startY = (float)gWindowHeight - 60.f;
    float x = (gWindowWidth * 0.5f) - 80.f;
    float y = startY;

    for (auto& m : gMessages)
    {
        drawQuadTexture(m.texture, x, y, 160.f, 40.f, pv);
        y -= 50.f;
    }
}

/*********************************************************
 * Paddles
 *********************************************************/
void updatePaddles(GLFWwindow* w, float dt)
{
    if (!gLeftPaddle || !gRightPaddle) return;


    // left paddle -> W/S
    if (glfwGetKey(w, GLFW_KEY_W) == GLFW_PRESS)
        gLeftPaddle->y += PADDLE_SPEED * dt;
    if (glfwGetKey(w, GLFW_KEY_S) == GLFW_PRESS)
        gLeftPaddle->y -= PADDLE_SPEED * dt;


    // right paddle -> up/down arrow
    if (glfwGetKey(w, GLFW_KEY_UP) == GLFW_PRESS)
        gRightPaddle->y += PADDLE_SPEED * dt;
    if (glfwGetKey(w, GLFW_KEY_DOWN) == GLFW_PRESS)
        gRightPaddle->y -= PADDLE_SPEED * dt;


    // clamp
    if (gLeftPaddle->y < 0) gLeftPaddle->y = 0;
    if (gLeftPaddle->y + gLeftPaddle->h > gWindowHeight)
        gLeftPaddle->y = gWindowHeight - gLeftPaddle->h;

    if (gRightPaddle->y < 0) gRightPaddle->y = 0;
    if (gRightPaddle->y + gRightPaddle->h > gWindowHeight)
        gRightPaddle->y = gWindowHeight - gRightPaddle->h;
}

void updatePaddleSizeAndPosition()
{
    // dynamic resizing of paddles
    float paddleWidth = gWindowWidth * 0.02f;  // 2% of window width
    float paddleHeight = gWindowHeight * 0.2f; // 20% of window height

    // Left Paddle
    gLeftPaddle->w = paddleWidth;
    gLeftPaddle->h = paddleHeight;
    gLeftPaddle->x = gWindowWidth * 0.05f;

    // Right Paddle
    gRightPaddle->w = paddleWidth;
    gRightPaddle->h = paddleHeight;
    gRightPaddle->x = gWindowWidth - gRightPaddle->w - (gWindowWidth * 0.05f); 

    // Update paddle speed
    PADDLE_SPEED = gWindowHeight * paddleModificator;
}


/*********************************************************
 * Ball
 *********************************************************/
//forward declaration
void resetItems();

void updateBall(float dt)
{
    if (!gBall) return;

    // Dynamic resizing of paddles
    gBall->w = gWindowWidth * 0.02f;  // 2% of window width
    gBall->h = gWindowWidth * 0.02f; // 20% of window height

    gBall->x += gBall->vx * dt;
    gBall->y += gBall->vy * dt;

    // top/bottom
    if (gBall->y < 0)
    {
        gBall->y = 0;
        gBall->vy *= -1;
    }
    if (gBall->y + gBall->h > gWindowHeight)
    {
        gBall->y = gWindowHeight - gBall->h;
        gBall->vy *= -1;
    }

    // left => right scores
    if (gBall->x < 0)
    {
        scoreRight++;
		gCurrentPowerUp = -1;
        resetItems();
        // reset
        gBall->x = (gWindowWidth / 2 - 15);
        gBall->y = (gWindowHeight / 2 - 15);
        gBall->vx = BALL_SPEED_X;
        gBall->vy = BALL_SPEED_Y;
        gBall->lastTouched = -1;
    }
    // right => left scores
    if (gBall->x + gBall->w > gWindowWidth)
    {
        scoreLeft++;
        gCurrentPowerUp = -1;

        resetItems();
        // reset
        gBall->x = (gWindowWidth / 2 - 15);
        gBall->y = (gWindowHeight / 2 - 15);
        gBall->vx = -BALL_SPEED_X;
        gBall->vy = BALL_SPEED_Y;
        gBall->lastTouched = -1;
    }

    // Collide with paddles
    if (gLeftPaddle)
    {
        if (gBall->x < gLeftPaddle->x + gLeftPaddle->w &&
            gBall->x + gBall->w> gLeftPaddle->x &&
            gBall->y < gLeftPaddle->y + gLeftPaddle->h &&
            gBall->y + gBall->h> gLeftPaddle->y)
        {
            gBall->x = gLeftPaddle->x + gLeftPaddle->w;
            gBall->vx *= -1;
            gBall->lastTouched = 0;
        }
    }
    if (gRightPaddle)
    {
        if (gBall->x + gBall->w > gRightPaddle->x &&
            gBall->x < gRightPaddle->x + gRightPaddle->w &&
            gBall->y < gRightPaddle->y + gRightPaddle->h &&
            gBall->y + gBall->h> gRightPaddle->y)
        {
            gBall->x = gRightPaddle->x - gBall->w;
            gBall->vx *= -1;
            gBall->lastTouched = 1;
        }
    }
}

void updateBallSize()
{
    // Dynamic resizing ball
    gBall->w = gWindowWidth * 0.02f;  // 2% of window width
    gBall->h = gWindowWidth * 0.02f; // 2% of window height

    // Update ball speed
    BALL_SPEED_X = gWindowWidth * ballModificatorX;
    BALL_SPEED_Y = gWindowWidth * ballModificatorY;
    gBall->vx = BALL_SPEED_X;
    gBall->vy = BALL_SPEED_Y;
}

/*********************************************************
 * Powerups
 *********************************************************/
void applyPowerUp(int powerType, int side)
{
    if (!gBall) return;

    // Record which power‐up is now active
    gCurrentPowerUp = powerType;

    // side=0 => left, side=1 => right
    if (powerType == 0) {
        // Speed up the ball
        BALL_SPEED_X *= 1.3f;
        BALL_SPEED_Y *= 1.3f;
        gBall->vx = BALL_SPEED_X;
        gBall->vy = BALL_SPEED_Y;
    }
    else if (powerType == 1) {
        // invert direction
        gBall->vx *= -1.f;
    }
    else if (powerType == 2) {
        // enlarge paddle
        if (side == 0 && gLeftPaddle) {
            gLeftPaddle->h *= 1.3f;
        }
        else if (side == 1 && gRightPaddle) {
            gRightPaddle->h *= 1.3f;
        }
    }
    else if (powerType == 3) {
        // minimize the opponent paddle
        if (side == 1 && gLeftPaddle) {
            gLeftPaddle->h *= 0.8f;
        }
        else if (side == 0 && gRightPaddle) {
            gRightPaddle->h *= 0.8f;
        }
    }
    else if (powerType == 4) {
        // slow down the ball
        BALL_SPEED_X *= 0.85f;
        BALL_SPEED_Y *= 0.85f;
        gBall->vx = BALL_SPEED_X;
        gBall->vy = BALL_SPEED_Y;
    }

    // The existing message system
    showPowerUpMessage(powerType, side);
}


void updatePowerups(float dt)
{
    if (!gBall) return;

    for (auto& p : gPowerUps)
    {
        if (!p.isPowerUp) continue;
        p.x += p.vx * dt;
        p.y += p.vy * dt;

        // Off bottom => reset
        if (p.y + p.h < 0)
        {
            p.x = 100 + (rand() % (gWindowWidth - 100));
            p.y = (float)gWindowHeight + 50.f;
            p.powerUpType = rand() % 5;
        }

        // Collision with ball
        if (gBall->x< p.x + p.w &&
            gBall->x + gBall->w> p.x &&
            gBall->y< p.y + p.h &&
            gBall->y + gBall->h> p.y)
        {
            if (gBall->lastTouched >= 0)
                applyPowerUp(p.powerUpType, gBall->lastTouched);

            // reset
            p.x = 100 + (rand() % (gWindowWidth - 100));
            p.y = (float)gWindowHeight + 50.f;
            p.powerUpType = rand() % 5;
        }
    }
}

void updatePowerUpSize()
{
    // Dynamic resizing powerUPs
    for (auto& p : gPowerUps)
    {
        p.w = gWindowWidth * 0.02f;  // 2% of window width
        p.h = gWindowWidth * 0.02f; // 2% of window height

        // Update powerup speed
        p.vy = -gWindowHeight * 0.3f;
    }

    
}


/*********************************************************
 * Reset item effects after a point was made
 *********************************************************/
void resetItems() {
    updatePaddleSizeAndPosition();
    updateBallSize();
}



/*********************************************************
 * updateAll
 *********************************************************/
void updateAll(GLFWwindow* window, float dt)
{
    updateMessages(dt);

    updatePaddles(window, dt);
    updateBall(dt);
    updatePowerups(dt);
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

    GLFWwindow* window = glfwCreateWindow(800, 600,
        "Normal-Mapped Pong with Score & Fewer Powerups", NULL, NULL);
    if (!window)
    {
        std::cerr << "Failed to create window\n";
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);

    glewInit();

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    // Load .glsl
    std::string vsCode = loadFile("../shaders/vertexShader.glsl");
    std::string fsCode = loadFile("../shaders/fragmentShader.glsl");
    gShaderProgram = createShaderProgram(vsCode, fsCode);
    glUseProgram(gShaderProgram);

    gUniMVP = glGetUniformLocation(gShaderProgram, "uMVP");
    gUniDiffuse = glGetUniformLocation(gShaderProgram, "uDiffuse");
    gUniNormal = glGetUniformLocation(gShaderProgram, "uNormal");
    gUniUseNormalMap = glGetUniformLocation(gShaderProgram, "uUseNormalMap");
    gUniLightPos = glGetUniformLocation(gShaderProgram, "uLightPos");
    gUniResolution = glGetUniformLocation(gShaderProgram, "uResolution");

    glUniform1i(gUniDiffuse, 0);
    glUniform1i(gUniNormal, 1);

    // Pass initial resolution
    glUniform2f(gUniResolution, (float)gWindowWidth, (float)gWindowHeight);

    createQuad();
	std::cout << "create quad done\n";
    initGame();
	std::cout << "init game done\n";

    // main loop
    float lastTime = (float)glfwGetTime();
    while (!glfwWindowShouldClose(window))
    {
        float now = (float)glfwGetTime();
        float dt = now - lastTime;
        lastTime = now;

        glfwPollEvents();
        if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
            glfwSetWindowShouldClose(window, true);

        updateAll(window, dt);

        // Clear
        glClearColor(0.1f, 0.1f, 0.15f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);

        // Light at ball center
        if (gBall) {
            float ballCenterX = gBall->x + gBall->w * 0.5f;
            float ballCenterY = gBall->y + gBall->h * 0.5f;
            glUseProgram(gShaderProgram);
            glUniform2f(gUniLightPos, ballCenterX, ballCenterY);
        }

        // If the window was resized, update the resolution uniform
        glUniform2f(gUniResolution, (float)gWindowWidth, (float)gWindowHeight);

        // build an ortho for the current window size
        glm::mat4 proj = glm::ortho(
            0.0f, (float)gWindowWidth,
            0.0f, (float)gWindowHeight,
            -1.0f, 1.0f
        );
        glm::mat4 view = glm::mat4(1.0f);
        glm::mat4 pv = proj * view;

        // background
        if (gBackground) {
            gBackground->w = (float)gWindowWidth;
            gBackground->h = (float)gWindowHeight;
            drawObject(*gBackground, pv);
        }

        // paddles
        if (gLeftPaddle) {
            drawObject(*gLeftPaddle, pv);
        }
		else {
			std::cerr << "Error: gLeftPaddle is null." << std::endl;
			return 1;
		}
        if (gRightPaddle) {
            drawObject(*gRightPaddle, pv);
		}
		else { std::cerr << "Error: gRightPaddle is null." << std::endl; return 1; }    

        // ball
       if (gBall) drawObject(*gBall, pv);

        // powerups
        for (auto& p : gPowerUps) {
            drawObject(p, pv);
        }

        // draw score
        drawNumber(scoreLeft, 50.f, (float)gWindowHeight - 60.f, 40.f, 40.f, pv);
        drawNumber(scoreRight, gWindowWidth - 90.f, (float)gWindowHeight - 60.f, 40.f, 40.f, pv);

        // messages
        drawMessages(pv);

        // Draw big icon for the current power-up
        drawActivePowerUpIndicator(pv);

        glfwSwapBuffers(window);
    }

    glfwDestroyWindow(window);
    glfwTerminate();
    return 0;
}
