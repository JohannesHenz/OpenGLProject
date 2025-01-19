#include <GL/glew.h>   // GLEW
#include <GLFW/glfw3.h> // GLFW
#include <iostream>

// Forward declaration for any callback functions you might want to use, e.g. keyboard, mouse callbacks

// -----------------------------------------------------------------------------
//   Helper function to draw a white cube of the given size in immediate mode
// -----------------------------------------------------------------------------
void drawCube(float size)
{
    float half = size * 0.5f;

    // We'll just set the color to white (no lighting in this example).
    

    // Each face is a quad (4 vertices). We define 6 faces in total.
    // Note: This is old-style immediate mode. Not supported in core profile.
    glBegin(GL_QUADS);
    // Front face
    glColor3f(1.0f, 0.0f, 0.0f); //red
    glVertex3f(-half, -half, half);
    glVertex3f(half, -half, half);
    glVertex3f(half, half, half);
    glVertex3f(-half, half, half);

    // Back face
    glColor3f(0.0f, 1.0f, 0.0f); //green
    glVertex3f(half, -half, -half);
    glVertex3f(-half, -half, -half);
    glVertex3f(-half, half, -half);
    glVertex3f(half, half, -half);

    // Left face
    glColor3f(0.0f, 0.0f, 1.0f); //blue
    glVertex3f(-half, -half, -half);
    glVertex3f(-half, -half, half);
    glVertex3f(-half, half, half);
    glVertex3f(-half, half, -half);

    // Right face
    glColor3f(1.0f, 1.0f, 0.0f); //yellow
    glVertex3f(half, -half, half);
    glVertex3f(half, -half, -half);
    glVertex3f(half, half, -half);
    glVertex3f(half, half, half);

    // Top face
    glColor3f(1.0f, 0.0f, 1.0f); // magenta
    glVertex3f(-half, half, half);
    glVertex3f(half, half, half);
    glVertex3f(half, half, -half);
    glVertex3f(-half, half, -half);

    // Bottom face
    glColor3f(0.0f, 1.0f, 1.0f); //cyan
    glVertex3f(-half, -half, -half);
    glVertex3f(half, -half, -half);
    glVertex3f(half, -half, half);
    glVertex3f(-half, -half, half);
    glEnd();
}



int main()
{
    // 1. Initialize GLFW
    if (!glfwInit())
    {
        std::cerr << "Failed to initialize GLFW." << std::endl;
        return -1;
    }

    // 2. Create a window with an OpenGL context
    GLFWwindow* window = glfwCreateWindow(800, 600, "My OpenGL Game", nullptr, nullptr);
    if (!window)
    {
        std::cerr << "Failed to create GLFW window." << std::endl;
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);

    // 3. Initialize GLEW (extension loader)
    GLenum err = glewInit();
    if (err != GLEW_OK)
    {
        std::cerr << "Failed to initialize GLEW: "
            << glewGetErrorString(err) << std::endl;
        glfwTerminate();
        return -1;
    }

    // 4. Set up any OpenGL options you want (viewport, etc.)
    glViewport(0, 0, 800, 600);
    glEnable(GL_DEPTH_TEST);

    /*
    // 5. Main Loop
    while (!glfwWindowShouldClose(window))
    {
        // -- Input handling, e.g., keyboard, mouse, camera movement --
        // -- Update animations --
        // -- Rendering calls go here --

        // Clear screen
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // ... Render objects, bind textures, use shaders, etc. ...

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    // 6. Clean up
    glfwDestroyWindow(window);
    glfwTerminate();
    return 0;
    }
    */



    // -------------------------------------------------------------------------
   //   PROJECTION SETUP (Orthographic) - using the fixed-function pipeline
   //   This is "legacy" style but quick for demonstration. 
   // -------------------------------------------------------------------------
   // Switch to the Projection matrix, set up an orthographic view.
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    // Left, Right, Bottom, Top, Near, Far
    // This is a 2D "box" from -4 to +4 horizontally, -3 to +3 vertically,
    // and we allow from -10 to +10 in depth.
    glOrtho(-4.0, 4.0, -3.0, 3.0, -10.0, 10.0);

    // -------------------------------------------------------------------------
    //   Variables for our bouncing animation
    // -------------------------------------------------------------------------
    float xPos = 0.0f;        // Current horizontal position of the cube
    float speed = 0.03f;      // How fast the cube moves per frame
    float leftLimit = -3.5f;  // Left boundary for bounce
    float rightLimit = 3.5f;  // Right boundary for bounce

    // 5. Main Loop
    while (!glfwWindowShouldClose(window))
    {
        // -- Input handling could go here, e.g., keyboard, mouse, etc. --
        //    For now, we just do the bouncing logic.

        // Update the position
        xPos += speed;

        // Bounce if we hit the limits
        if (xPos > rightLimit)
        {
            xPos = rightLimit;
            speed = -speed;
        }
        else if (xPos < leftLimit)
        {
            xPos = leftLimit;
            speed = -speed;
        }

        // ---------------------------------------------------------------------
        //   RENDERING
        // ---------------------------------------------------------------------
        // Clear the color and depth buffers
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // Switch to the ModelView matrix for drawing the cube
        glMatrixMode(GL_MODELVIEW);
        glLoadIdentity();

        // Move (translate) to the current x-position. Keep z at -5.0 so cube is visible
        // (in orthographic mode, if you keep z at 0, it's still visible, but let's
        // push it "into" the screen a bit).
        glTranslatef(xPos, 0.0f, -5.0f);

        // Let's rotate around each axis at different speeds
        float timeSec = (float)glfwGetTime();

        // Rotate around the X-axis
        glRotatef(timeSec * 30.0f, 1.0f, 0.0f, 0.0f);

        // Rotate around the Y-axis
        glRotatef(timeSec * 50.0f, 0.0f, 1.0f, 0.0f);

        // Rotate around the Z-axis
        glRotatef(timeSec * 70.0f, 0.0f, 0.0f, 1.0f);

        // Draw a small white cube
        drawCube(1.0f);

        // Swap front and back buffers
        glfwSwapBuffers(window);

        // Poll for and process events
        glfwPollEvents();
    }

    // 6. Clean up
    glfwDestroyWindow(window);
    glfwTerminate();
    return 0;
}

