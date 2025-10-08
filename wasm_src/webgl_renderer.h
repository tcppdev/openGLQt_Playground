#ifndef WEBGL_RENDERER_H
#define WEBGL_RENDERER_H

#ifdef __EMSCRIPTEN__
    #include <GLES3/gl3.h>
    #include <GLES2/gl2ext.h>
#else
    #include <GL/gl.h>
#endif
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <vector>
#include <string>
#include <memory>

// Forward declarations
struct Vertex {
    glm::vec3 position;
    glm::vec3 normal;
    glm::vec2 texCoords;
    glm::vec3 color;
};

class WebGLRenderer {
public:
    WebGLRenderer(int width, int height);
    ~WebGLRenderer();

    bool initialize();
    void render();
    void resize(int width, int height);
    
    // Input handling
    void processMouse(float xoffset, float yoffset);
    void processScroll(float yoffset);
    void processKeyboard(int key, int action);
    
    // Camera controls
    void setCameraMode(int mode);
    void resetCamera();

private:
    // Window dimensions
    int m_width;
    int m_height;
    
    // Shader programs
    GLuint m_shaderProgram;
    GLuint m_lineShaderProgram;
    GLuint m_pointShaderProgram;
    
    // Vertex data
    std::vector<Vertex> m_vertices;
    std::vector<unsigned int> m_indices;
    std::vector<Vertex> m_lineVertices;
    std::vector<Vertex> m_pointVertices;
    
    // OpenGL objects
    GLuint m_VAO, m_VBO, m_EBO;
    GLuint m_lineVAO, m_lineVBO;
    GLuint m_pointVAO, m_pointVBO;
    
    // Camera
    glm::vec3 m_cameraPos;
    glm::vec3 m_cameraTarget;
    glm::vec3 m_cameraUp;
    float m_cameraYaw;
    float m_cameraPitch;
    float m_cameraDistance;
    int m_cameraMode;
    
    // Matrices
    glm::mat4 m_view;
    glm::mat4 m_projection;
    glm::mat4 m_model;
    
    // Animation
    float m_time;
    bool m_animate;
    
    // Private methods
    bool loadShaders();
    bool createGeometry();
    void updateCamera();
    void updateMatrices();
    void renderGeometry();
    void renderLines();
    void renderPoints();
    
    // Shader utilities
    GLuint compileShader(const char* source, GLenum type);
    GLuint createShaderProgram(const char* vertexSource, const char* fragmentSource, const char* geometrySource = nullptr);
    void checkShaderCompilation(GLuint shader, const std::string& type);
    void checkProgramLinking(GLuint program);
    
    // Geometry creation
    void createCube();
    void createSphere(int segments = 32);
    void createCoastline();
    void createGrid();
};

#endif // WEBGL_RENDERER_H
