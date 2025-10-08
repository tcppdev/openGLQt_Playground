#include "webgl_renderer.h"
#include <iostream>
#include <fstream>
#include <sstream>
#include <cmath>
#include <emscripten.h>

// WebGL-compatible shader sources
const char* vertexShaderSource = R"(#version 300 es
precision mediump float;

layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aNormal;
layout (location = 2) in vec2 aTexCoord;
layout (location = 3) in vec3 aColor;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

out vec3 FragPos;
out vec3 Normal;
out vec2 TexCoord;
out vec3 Color;

void main() {
    FragPos = vec3(model * vec4(aPos, 1.0));
    Normal = mat3(transpose(inverse(model))) * aNormal;
    TexCoord = aTexCoord;
    Color = aColor;
    
    gl_Position = projection * view * vec4(FragPos, 1.0);
}
)";

const char* fragmentShaderSource = R"(#version 300 es
precision mediump float;

in vec3 FragPos;
in vec3 Normal;
in vec2 TexCoord;
in vec3 Color;

out vec4 FragColor;

uniform vec3 lightPos;
uniform vec3 lightColor;
uniform vec3 viewPos;
uniform float time;

void main() {
    // Ambient lighting
    float ambientStrength = 0.3;
    vec3 ambient = ambientStrength * lightColor;
    
    // Diffuse lighting
    vec3 norm = normalize(Normal);
    vec3 lightColor = vec3(1.0, 1.0, 1.0);
    vec3 lightDir = normalize(lightPos - FragPos);
    float diff = max(dot(norm, lightDir), 0.0);
    vec3 diffuse = diff * lightColor;
    
    // Specular lighting
    float specularStrength = 0.5;
    vec3 viewDir = normalize(viewPos - FragPos);
    vec3 reflectDir = reflect(-lightDir, norm);
    float spec = pow(max(dot(viewDir, reflectDir), 0.0), 32.0);
    vec3 specular = specularStrength * spec * lightColor;
    
    // Combine lighting with color
    vec3 result = (ambient + diffuse + specular) * Color;
    
    // Add some animation based on time
    result *= (0.9 + 0.1 * sin(time * 2.0));
    
    FragColor = vec4(result, 1.0);
}
)";

const char* lineVertexShaderSource = R"(#version 300 es
precision mediump float;

layout (location = 0) in vec3 aPos;
layout (location = 3) in vec3 aColor;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

out vec3 Color;

void main() {
    Color = aColor;
    gl_Position = projection * view * model * vec4(aPos, 1.0);
}
)";

const char* lineFragmentShaderSource = R"(#version 300 es
precision mediump float;

in vec3 Color;
out vec4 FragColor;

void main() {
    FragColor = vec4(Color, 1.0);
}
)";

const char* pointVertexShaderSource = R"(#version 300 es
precision mediump float;

layout (location = 0) in vec3 aPos;
layout (location = 3) in vec3 aColor;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

out vec3 Color;

void main() {
    Color = aColor;
    gl_Position = projection * view * model * vec4(aPos, 1.0);
    gl_PointSize = 5.0;
}
)";

WebGLRenderer::WebGLRenderer(int width, int height)
    : m_width(width), m_height(height), m_shaderProgram(0), m_lineShaderProgram(0), m_pointShaderProgram(0),
      m_VAO(0), m_VBO(0), m_EBO(0), m_lineVAO(0), m_lineVBO(0), m_pointVAO(0), m_pointVBO(0),
      m_cameraPos(0.0f, 0.0f, 5.0f), m_cameraTarget(0.0f, 0.0f, 0.0f), m_cameraUp(0.0f, 1.0f, 0.0f),
      m_cameraYaw(-90.0f), m_cameraPitch(0.0f), m_cameraDistance(5.0f), m_cameraMode(0),
      m_time(0.0f), m_animate(true) {
    updateCamera();
}

WebGLRenderer::~WebGLRenderer() {
    // Cleanup OpenGL resources
    if (m_VAO) glDeleteVertexArrays(1, &m_VAO);
    if (m_VBO) glDeleteBuffers(1, &m_VBO);
    if (m_EBO) glDeleteBuffers(1, &m_EBO);
    if (m_lineVAO) glDeleteVertexArrays(1, &m_lineVAO);
    if (m_lineVBO) glDeleteBuffers(1, &m_lineVBO);
    if (m_pointVAO) glDeleteVertexArrays(1, &m_pointVAO);
    if (m_pointVBO) glDeleteBuffers(1, &m_pointVBO);
    if (m_shaderProgram) glDeleteProgram(m_shaderProgram);
    if (m_lineShaderProgram) glDeleteProgram(m_lineShaderProgram);
    if (m_pointShaderProgram) glDeleteProgram(m_pointShaderProgram);
}

bool WebGLRenderer::initialize() {
    std::cout << "Initializing WebGL renderer..." << std::endl;
    
    if (!loadShaders()) {
        std::cerr << "Failed to load shaders" << std::endl;
        return false;
    }
    
    if (!createGeometry()) {
        std::cerr << "Failed to create geometry" << std::endl;
        return false;
    }
    
    updateMatrices();
    
    std::cout << "WebGL renderer initialized successfully" << std::endl;
    return true;
}

void WebGLRenderer::render() {
    // Update time for animation
    m_time += 0.016f; // Approximate 60 FPS
    
    if (m_animate) {
        // Rotate model matrix for animation
        m_model = glm::rotate(glm::mat4(1.0f), m_time * 0.5f, glm::vec3(0.0f, 1.0f, 0.0f));
    }
    
    updateMatrices();
    
    // Render main geometry
    renderGeometry();
    
    // Render lines and points
    renderLines();
    renderPoints();
}

void WebGLRenderer::resize(int width, int height) {
    m_width = width;
    m_height = height;
    glViewport(0, 0, width, height);
    updateMatrices();
}

void WebGLRenderer::processMouse(float xoffset, float yoffset) {
    const float sensitivity = 0.1f;
    xoffset *= sensitivity;
    yoffset *= sensitivity;
    
    m_cameraYaw += xoffset;
    m_cameraPitch += yoffset;
    
    // Constrain pitch
    if (m_cameraPitch > 89.0f) m_cameraPitch = 89.0f;
    if (m_cameraPitch < -89.0f) m_cameraPitch = -89.0f;
    
    updateCamera();
}

void WebGLRenderer::processScroll(float yoffset) {
    m_cameraDistance -= yoffset * 0.5f;
    if (m_cameraDistance < 0.5f) m_cameraDistance = 0.5f;
    if (m_cameraDistance > 20.0f) m_cameraDistance = 20.0f;
    updateCamera();
}

void WebGLRenderer::processKeyboard(int key, int action) {
    if (action == 1) { // GLFW_PRESS
        switch (key) {
            case 32: // SPACE
                m_animate = !m_animate;
                break;
            case 82: // R key
                resetCamera();
                break;
            case 49: // 1 key
                setCameraMode(0);
                break;
            case 50: // 2 key
                setCameraMode(1);
                break;
        }
    }
}

void WebGLRenderer::setCameraMode(int mode) {
    m_cameraMode = mode;
    updateCamera();
}

void WebGLRenderer::resetCamera() {
    m_cameraYaw = -90.0f;
    m_cameraPitch = 0.0f;
    m_cameraDistance = 5.0f;
    m_cameraMode = 0;
    updateCamera();
}

bool WebGLRenderer::loadShaders() {
    // Create main shader program
    m_shaderProgram = createShaderProgram(vertexShaderSource, fragmentShaderSource);
    if (!m_shaderProgram) {
        std::cerr << "Failed to create main shader program" << std::endl;
        return false;
    }
    
    // Create line shader program
    m_lineShaderProgram = createShaderProgram(lineVertexShaderSource, lineFragmentShaderSource);
    if (!m_lineShaderProgram) {
        std::cerr << "Failed to create line shader program" << std::endl;
        return false;
    }
    
    // Create point shader program
    m_pointShaderProgram = createShaderProgram(pointVertexShaderSource, lineFragmentShaderSource);
    if (!m_pointShaderProgram) {
        std::cerr << "Failed to create point shader program" << std::endl;
        return false;
    }
    
    return true;
}

bool WebGLRenderer::createGeometry() {
    // Create main geometry (cube and sphere)
    createCube();
    createSphere();
    createGrid();
    
    // Try to create coastline if data is available
    createCoastline();
    
    // Setup main VAO
    glGenVertexArrays(1, &m_VAO);
    glGenBuffers(1, &m_VBO);
    glGenBuffers(1, &m_EBO);
    
    glBindVertexArray(m_VAO);
    
    glBindBuffer(GL_ARRAY_BUFFER, m_VBO);
    glBufferData(GL_ARRAY_BUFFER, m_vertices.size() * sizeof(Vertex), m_vertices.data(), GL_STATIC_DRAW);
    
    if (!m_indices.empty()) {
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_EBO);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, m_indices.size() * sizeof(unsigned int), m_indices.data(), GL_STATIC_DRAW);
    }
    
    // Position attribute
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)0);
    glEnableVertexAttribArray(0);
    
    // Normal attribute
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, normal));
    glEnableVertexAttribArray(1);
    
    // Texture coordinate attribute
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, texCoords));
    glEnableVertexAttribArray(2);
    
    // Color attribute
    glVertexAttribPointer(3, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, color));
    glEnableVertexAttribArray(3);
    
    // Setup line VAO
    if (!m_lineVertices.empty()) {
        glGenVertexArrays(1, &m_lineVAO);
        glGenBuffers(1, &m_lineVBO);
        
        glBindVertexArray(m_lineVAO);
        glBindBuffer(GL_ARRAY_BUFFER, m_lineVBO);
        glBufferData(GL_ARRAY_BUFFER, m_lineVertices.size() * sizeof(Vertex), m_lineVertices.data(), GL_STATIC_DRAW);
        
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)0);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(3, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, color));
        glEnableVertexAttribArray(3);
    }
    
    // Setup point VAO
    if (!m_pointVertices.empty()) {
        glGenVertexArrays(1, &m_pointVAO);
        glGenBuffers(1, &m_pointVBO);
        
        glBindVertexArray(m_pointVAO);
        glBindBuffer(GL_ARRAY_BUFFER, m_pointVBO);
        glBufferData(GL_ARRAY_BUFFER, m_pointVertices.size() * sizeof(Vertex), m_pointVertices.data(), GL_STATIC_DRAW);
        
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)0);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(3, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, color));
        glEnableVertexAttribArray(3);
    }
    
    glBindVertexArray(0);
    return true;
}

void WebGLRenderer::updateCamera() {
    // Calculate camera position based on spherical coordinates
    glm::vec3 front;
    front.x = cos(glm::radians(m_cameraYaw)) * cos(glm::radians(m_cameraPitch));
    front.y = sin(glm::radians(m_cameraPitch));
    front.z = sin(glm::radians(m_cameraYaw)) * cos(glm::radians(m_cameraPitch));
    
    m_cameraPos = m_cameraTarget - glm::normalize(front) * m_cameraDistance;
    m_view = glm::lookAt(m_cameraPos, m_cameraTarget, m_cameraUp);
}

void WebGLRenderer::updateMatrices() {
    m_view = glm::lookAt(m_cameraPos, m_cameraTarget, m_cameraUp);
    m_projection = glm::perspective(glm::radians(45.0f), (float)m_width / (float)m_height, 0.1f, 100.0f);
}

void WebGLRenderer::renderGeometry() {
    if (m_vertices.empty()) return;
    
    glUseProgram(m_shaderProgram);
    
    // Set uniforms
    GLint modelLoc = glGetUniformLocation(m_shaderProgram, "model");
    GLint viewLoc = glGetUniformLocation(m_shaderProgram, "view");
    GLint projLoc = glGetUniformLocation(m_shaderProgram, "projection");
    GLint timeLoc = glGetUniformLocation(m_shaderProgram, "time");
    GLint lightPosLoc = glGetUniformLocation(m_shaderProgram, "lightPos");
    GLint lightColorLoc = glGetUniformLocation(m_shaderProgram, "lightColor");
    GLint viewPosLoc = glGetUniformLocation(m_shaderProgram, "viewPos");
    
    glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(m_model));
    glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(m_view));
    glUniformMatrix4fv(projLoc, 1, GL_FALSE, glm::value_ptr(m_projection));
    glUniform1f(timeLoc, m_time);
    glUniform3f(lightPosLoc, 2.0f, 2.0f, 2.0f);
    glUniform3f(lightColorLoc, 1.0f, 1.0f, 1.0f);
    glUniform3fv(viewPosLoc, 1, glm::value_ptr(m_cameraPos));
    
    glBindVertexArray(m_VAO);
    
    if (!m_indices.empty()) {
        glDrawElements(GL_TRIANGLES, m_indices.size(), GL_UNSIGNED_INT, 0);
    } else {
        glDrawArrays(GL_TRIANGLES, 0, m_vertices.size());
    }
    
    glBindVertexArray(0);
}

void WebGLRenderer::renderLines() {
    if (m_lineVertices.empty()) return;
    
    glUseProgram(m_lineShaderProgram);
    
    GLint modelLoc = glGetUniformLocation(m_lineShaderProgram, "model");
    GLint viewLoc = glGetUniformLocation(m_lineShaderProgram, "view");
    GLint projLoc = glGetUniformLocation(m_lineShaderProgram, "projection");
    
    glm::mat4 lineModel = glm::mat4(1.0f);
    glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(lineModel));
    glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(m_view));
    glUniformMatrix4fv(projLoc, 1, GL_FALSE, glm::value_ptr(m_projection));
    
    glBindVertexArray(m_lineVAO);
    glDrawArrays(GL_LINES, 0, m_lineVertices.size());
    glBindVertexArray(0);
}

void WebGLRenderer::renderPoints() {
    if (m_pointVertices.empty()) return;
    
    glUseProgram(m_pointShaderProgram);
    
    GLint modelLoc = glGetUniformLocation(m_pointShaderProgram, "model");
    GLint viewLoc = glGetUniformLocation(m_pointShaderProgram, "view");
    GLint projLoc = glGetUniformLocation(m_pointShaderProgram, "projection");
    
    glm::mat4 pointModel = glm::mat4(1.0f);
    glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(pointModel));
    glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(m_view));
    glUniformMatrix4fv(projLoc, 1, GL_FALSE, glm::value_ptr(m_projection));
    
    glBindVertexArray(m_pointVAO);
    glDrawArrays(GL_POINTS, 0, m_pointVertices.size());
    glBindVertexArray(0);
}

GLuint WebGLRenderer::compileShader(const char* source, GLenum type) {
    GLuint shader = glCreateShader(type);
    glShaderSource(shader, 1, &source, nullptr);
    glCompileShader(shader);
    
    checkShaderCompilation(shader, type == GL_VERTEX_SHADER ? "VERTEX" : "FRAGMENT");
    
    return shader;
}

GLuint WebGLRenderer::createShaderProgram(const char* vertexSource, const char* fragmentSource, const char* geometrySource) {
    GLuint vertexShader = compileShader(vertexSource, GL_VERTEX_SHADER);
    GLuint fragmentShader = compileShader(fragmentSource, GL_FRAGMENT_SHADER);
    
    GLuint program = glCreateProgram();
    glAttachShader(program, vertexShader);
    glAttachShader(program, fragmentShader);
    
    // WebGL doesn't support geometry shaders
#ifndef __EMSCRIPTEN__
    if (geometrySource) {
        GLuint geometryShader = compileShader(geometrySource, GL_GEOMETRY_SHADER);
        glAttachShader(program, geometryShader);
        glDeleteShader(geometryShader);
    }
#endif
    
    glLinkProgram(program);
    checkProgramLinking(program);
    
    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);
    
    return program;
}

void WebGLRenderer::checkShaderCompilation(GLuint shader, const std::string& type) {
    GLint success;
    GLchar infoLog[512];
    glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
    if (!success) {
        glGetShaderInfoLog(shader, 512, nullptr, infoLog);
        std::cerr << "ERROR::SHADER::" << type << "::COMPILATION_FAILED\n" << infoLog << std::endl;
    }
}

void WebGLRenderer::checkProgramLinking(GLuint program) {
    GLint success;
    GLchar infoLog[512];
    glGetProgramiv(program, GL_LINK_STATUS, &success);
    if (!success) {
        glGetProgramInfoLog(program, 512, nullptr, infoLog);
        std::cerr << "ERROR::SHADER::PROGRAM::LINKING_FAILED\n" << infoLog << std::endl;
    }
}

void WebGLRenderer::createCube() {
    // Simple cube vertices
    std::vector<Vertex> cubeVertices = {
        // Front face
        {{-0.5f, -0.5f,  0.5f}, {0.0f,  0.0f,  1.0f}, {0.0f, 0.0f}, {1.0f, 0.0f, 0.0f}},
        {{ 0.5f, -0.5f,  0.5f}, {0.0f,  0.0f,  1.0f}, {1.0f, 0.0f}, {1.0f, 0.0f, 0.0f}},
        {{ 0.5f,  0.5f,  0.5f}, {0.0f,  0.0f,  1.0f}, {1.0f, 1.0f}, {1.0f, 0.0f, 0.0f}},
        {{-0.5f,  0.5f,  0.5f}, {0.0f,  0.0f,  1.0f}, {0.0f, 1.0f}, {1.0f, 0.0f, 0.0f}},
        
        // Back face
        {{-0.5f, -0.5f, -0.5f}, {0.0f,  0.0f, -1.0f}, {1.0f, 0.0f}, {0.0f, 1.0f, 0.0f}},
        {{ 0.5f, -0.5f, -0.5f}, {0.0f,  0.0f, -1.0f}, {0.0f, 0.0f}, {0.0f, 1.0f, 0.0f}},
        {{ 0.5f,  0.5f, -0.5f}, {0.0f,  0.0f, -1.0f}, {0.0f, 1.0f}, {0.0f, 1.0f, 0.0f}},
        {{-0.5f,  0.5f, -0.5f}, {0.0f,  0.0f, -1.0f}, {1.0f, 1.0f}, {0.0f, 1.0f, 0.0f}},
    };
    
    std::vector<unsigned int> cubeIndices = {
        0, 1, 2, 2, 3, 0,   // Front
        4, 5, 6, 6, 7, 4,   // Back
        7, 3, 0, 0, 4, 7,   // Left
        1, 5, 6, 6, 2, 1,   // Right
        3, 2, 6, 6, 7, 3,   // Top
        0, 1, 5, 5, 4, 0    // Bottom
    };
    
    m_vertices.insert(m_vertices.end(), cubeVertices.begin(), cubeVertices.end());
    m_indices.insert(m_indices.end(), cubeIndices.begin(), cubeIndices.end());
}

void WebGLRenderer::createSphere(int segments) {
    const float PI = 3.14159265359f;
    
    // Generate sphere vertices
    for (int i = 0; i <= segments; ++i) {
        float lat = PI * (-0.5f + (float)i / segments);
        float y = sin(lat);
        float r = cos(lat);
        
        for (int j = 0; j <= segments; ++j) {
            float lon = 2 * PI * (float)j / segments;
            float x = r * cos(lon);
            float z = r * sin(lon);
            
            Vertex vertex;
            vertex.position = glm::vec3(x + 2.0f, y, z); // Offset to avoid overlap with cube
            vertex.normal = glm::vec3(x, y, z);
            vertex.texCoords = glm::vec2((float)j / segments, (float)i / segments);
            vertex.color = glm::vec3(0.0f, 0.0f, 1.0f); // Blue sphere
            
            m_vertices.push_back(vertex);
        }
    }
    
    // Generate sphere indices
    int offset = 8; // After cube vertices
    for (int i = 0; i < segments; ++i) {
        for (int j = 0; j < segments; ++j) {
            int first = offset + i * (segments + 1) + j;
            int second = first + segments + 1;
            
            m_indices.push_back(first);
            m_indices.push_back(second);
            m_indices.push_back(first + 1);
            
            m_indices.push_back(second);
            m_indices.push_back(second + 1);
            m_indices.push_back(first + 1);
        }
    }
}

void WebGLRenderer::createGrid() {
    // Create a simple grid for reference
    const int gridSize = 10;
    const float spacing = 0.5f;
    const float halfGrid = gridSize * spacing * 0.5f;
    
    // Horizontal lines
    for (int i = 0; i <= gridSize; ++i) {
        float z = -halfGrid + i * spacing;
        Vertex v1, v2;
        v1.position = glm::vec3(-halfGrid, -1.0f, z);
        v1.color = glm::vec3(0.3f, 0.3f, 0.3f);
        v2.position = glm::vec3(halfGrid, -1.0f, z);
        v2.color = glm::vec3(0.3f, 0.3f, 0.3f);
        
        m_lineVertices.push_back(v1);
        m_lineVertices.push_back(v2);
    }
    
    // Vertical lines
    for (int i = 0; i <= gridSize; ++i) {
        float x = -halfGrid + i * spacing;
        Vertex v1, v2;
        v1.position = glm::vec3(x, -1.0f, -halfGrid);
        v1.color = glm::vec3(0.3f, 0.3f, 0.3f);
        v2.position = glm::vec3(x, -1.0f, halfGrid);
        v2.color = glm::vec3(0.3f, 0.3f, 0.3f);
        
        m_lineVertices.push_back(v1);
        m_lineVertices.push_back(v2);
    }
}

void WebGLRenderer::createCoastline() {
    // Try to load coastline data from the preloaded file
    // This is a simplified version - in a real application you'd parse the CSV
    
    // Create some sample points for demonstration
    std::vector<glm::vec3> coastPoints = {
        {-2.0f, 0.0f, -2.0f},
        {-1.5f, 0.0f, -1.8f},
        {-1.0f, 0.0f, -1.5f},
        {-0.5f, 0.0f, -1.2f},
        { 0.0f, 0.0f, -1.0f},
        { 0.5f, 0.0f, -0.8f},
        { 1.0f, 0.0f, -0.5f},
        { 1.5f, 0.0f, -0.2f},
        { 2.0f, 0.0f,  0.0f},
    };
    
    // Add coastline as points
    for (const auto& point : coastPoints) {
        Vertex vertex;
        vertex.position = point;
        vertex.color = glm::vec3(1.0f, 1.0f, 0.0f); // Yellow points
        m_pointVertices.push_back(vertex);
    }
    
    // Add coastline as lines
    for (size_t i = 0; i < coastPoints.size() - 1; ++i) {
        Vertex v1, v2;
        v1.position = coastPoints[i];
        v1.color = glm::vec3(0.0f, 1.0f, 1.0f); // Cyan lines
        v2.position = coastPoints[i + 1];
        v2.color = glm::vec3(0.0f, 1.0f, 1.0f);
        
        m_lineVertices.push_back(v1);
        m_lineVertices.push_back(v2);
    }
}
