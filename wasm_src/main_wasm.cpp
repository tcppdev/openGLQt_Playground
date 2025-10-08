#include <GLFW/glfw3.h>
#include <emscripten.h>
#include <emscripten/html5.h>
#ifdef __EMSCRIPTEN__
    #include <GLES3/gl3.h>
    #include <GLES2/gl2ext.h>
#else
    #include <GL/gl.h>
#endif
#include <iostream>
#include <memory>
#include <string>

// Include the existing headers
#include "webgl_renderer.h"
#include "utilities.h"

// Global variables
GLFWwindow* window = nullptr;
std::unique_ptr<WebGLRenderer> renderer;
int window_width = 800;
int window_height = 600;

// Mouse and keyboard state
double last_mouse_x = 400.0;
double last_mouse_y = 300.0;
bool first_mouse = true;
bool mouse_pressed = false;

// Camera controls
float camera_yaw = -90.0f;
float camera_pitch = 0.0f;
float camera_distance = 5.0f;

// Callback functions
void framebuffer_size_callback(GLFWwindow* window, int width, int height) {
    window_width = width;
    window_height = height;
    glViewport(0, 0, width, height);
    if (renderer) {
        renderer->resize(width, height);
    }
}

void mouse_callback(GLFWwindow* window, double xpos, double ypos) {
    if (first_mouse) {
        last_mouse_x = xpos;
        last_mouse_y = ypos;
        first_mouse = false;
    }

    float xoffset = xpos - last_mouse_x;
    float yoffset = last_mouse_y - ypos; // reversed since y-coordinates go from bottom to top

    last_mouse_x = xpos;
    last_mouse_y = ypos;

    if (mouse_pressed && renderer) {
        renderer->processMouse(xoffset, yoffset);
    }
}

void mouse_button_callback(GLFWwindow* window, int button, int action, int mods) {
    if (button == GLFW_MOUSE_BUTTON_LEFT) {
        mouse_pressed = (action == GLFW_PRESS);
    }
}

void scroll_callback(GLFWwindow* window, double xoffset, double yoffset) {
    if (renderer) {
        renderer->processScroll(yoffset);
    }
}

void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods) {
    if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS) {
        glfwSetWindowShouldClose(window, true);
    }
    
    if (renderer) {
        renderer->processKeyboard(key, action);
    }
}

// Main render loop
void main_loop() {
    // Process input
    glfwPollEvents();
    
    // Clear the screen
    glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    
    // Render
    if (renderer) {
        renderer->render();
    }
    
    // Swap buffers
    glfwSwapBuffers(window);
}

// Initialize OpenGL context
bool initialize_opengl() {
    // Initialize GLFW
    if (!glfwInit()) {
        std::cerr << "Failed to initialize GLFW" << std::endl;
        return false;
    }

    // Configure GLFW for WebGL
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_ANY_PROFILE);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);

    // Create window
    window = glfwCreateWindow(window_width, window_height, "OpenGL Qt Playground - WebAssembly", nullptr, nullptr);
    if (!window) {
        std::cerr << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return false;
    }

    // Make the window's context current
    glfwMakeContextCurrent(window);

    // Set callbacks
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
    glfwSetCursorPosCallback(window, mouse_callback);
    glfwSetMouseButtonCallback(window, mouse_button_callback);
    glfwSetScrollCallback(window, scroll_callback);
    glfwSetKeyCallback(window, key_callback);

    // Enable depth testing
    glEnable(GL_DEPTH_TEST);
    
    // Enable blending for transparency
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    // Set initial viewport
    glViewport(0, 0, window_width, window_height);

    return true;
}

// Main function
int main() {
    std::cout << "Initializing OpenGL Qt Playground WebAssembly version..." << std::endl;
    
    if (!initialize_opengl()) {
        std::cerr << "Failed to initialize OpenGL" << std::endl;
        return -1;
    }

    // Print OpenGL info
    std::cout << "OpenGL Version: " << glGetString(GL_VERSION) << std::endl;
    std::cout << "GLSL Version: " << glGetString(GL_SHADING_LANGUAGE_VERSION) << std::endl;
    std::cout << "Renderer: " << glGetString(GL_RENDERER) << std::endl;

    // Initialize renderer
    try {
        renderer = std::make_unique<WebGLRenderer>(window_width, window_height);
        if (!renderer->initialize()) {
            std::cerr << "Failed to initialize renderer" << std::endl;
            return -1;
        }
    } catch (const std::exception& e) {
        std::cerr << "Exception during renderer initialization: " << e.what() << std::endl;
        return -1;
    }

    std::cout << "Initialization complete. Starting render loop..." << std::endl;

    // Start the main loop (Emscripten will handle the loop)
    emscripten_set_main_loop(main_loop, 0, 1);

    // Cleanup (this won't be reached in Emscripten)
    renderer.reset();
    glfwDestroyWindow(window);
    glfwTerminate();

    return 0;
}

// Export functions for JavaScript
extern "C" {
    EMSCRIPTEN_KEEPALIVE
    void resize_canvas(int width, int height) {
        window_width = width;
        window_height = height;
        if (renderer) {
            renderer->resize(width, height);
        }
    }
    
    EMSCRIPTEN_KEEPALIVE
    void set_camera_mode(int mode) {
        if (renderer) {
            renderer->setCameraMode(mode);
        }
    }
    
    EMSCRIPTEN_KEEPALIVE
    void reset_camera() {
        if (renderer) {
            renderer->resetCamera();
        }
    }
}
