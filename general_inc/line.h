#include <glm/glm.hpp>
#include <Eigen/Core>

#include <vector>
#include <stdexcept>

#include <QOpenGLContext> 
#include <QOpenGLFunctions_3_3_Core>

#include <general_inc/shader.h>

constexpr float DEFAULT_LINE_WIDTH = 5;
constexpr float LINEWIDTH_SCALING_FACTOR = 0.0005; 
constexpr float MIN_LINE_WIDTH = 1;
constexpr float MAX_LINE_WIDTH = 20;

struct SimpleVertex {   // just want to make sure 
    // position
    glm::vec3 Position;
};

/// Looks like colours to me
enum class Color { RED, GREEN, BLUE, BLACK, WHITE };
glm::vec4 get_color(Color color)   // use unordered map instead?
{
    switch(color) {
        case(Color::RED): { return glm::vec4(1.0, 0.0, 0.0, 1.0); }
        case(Color::GREEN): { return glm::vec4(0.0, 1.0, 0.0, 1.0); }
        case(Color::BLUE): { return glm::vec4(0.0, 0.0, 1.0, 1.0); }
        case(Color::BLACK): { return glm::vec4(0.0, 0.0, 0.0, 1.0); }
        case(Color::WHITE): { return glm::vec4(1.0, 1.0, 1.0, 1.0); }
        default: { throw std::invalid_argument("Unknown color"); }  // white
    }
}

class Line: protected QOpenGLFunctions_3_3_Core
{
public:
    
    Line() = delete; // need to at least give some coordinates

    Line(std::vector<Eigen::Vector3f> coordinates, glm::mat4 view_matrix = glm::mat4(1.0f), 
        glm::mat4 projection_matrix = glm::mat4(1.0f), float linewidth = DEFAULT_LINE_WIDTH, Color linecolor = Color::RED)
    {
        linewidth_ = linewidth;
        linecolor_ = linecolor;
        view_matrix_ = view_matrix;
        projection_matrix_ = projection_matrix;

        // Line shader
        const char* vertex_line_path = "/home/t.clar/Repos/openGLQt/shaders/line_shader.vs";
        const char* fragment_line_path = "/home/t.clar/Repos/openGLQt/shaders/line_shader.fs";
        const char* geometry_line_path = "/home/t.clar/Repos/openGLQt/shaders/line_shader.gs";
        m_line_shader = new Shader(vertex_line_path, fragment_line_path, geometry_line_path);

        SimpleVertex vertex;

        for (const Eigen::Vector3f& coordinate : coordinates) {// access by const reference  
            glm::vec3 vector; 
            // positions
            vector.x = coordinate[0];
            vector.y = coordinate[1];
            vector.z = coordinate[2];
            vertex.Position = vector;

            vertices_.push_back(vertex);
        }

        initializeOpenGLFunctions();   // Initialise current context  (required)
 
        // Setup opengl states
        setup(); 
    }

    ~Line() {
        delete m_line_shader;
    }

    void setup()
    {
        // Create the buffers and array:
        glGenVertexArrays(1, &vao_);
        glGenBuffers(1, &vbo_);

        glBindVertexArray(vao_);  

        // load data into buffers
        glBindBuffer(GL_ARRAY_BUFFER, vbo_);
        glBufferData(GL_ARRAY_BUFFER, vertices_.size() * sizeof(SimpleVertex), &vertices_[0], GL_STATIC_DRAW);  

        // set the vertex attribute pointers:
        // vertex Positions
        glEnableVertexAttribArray(0);	
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(SimpleVertex), (void*)0);

        // glBindVertexArray(0);  // Unbind vao
    }

    void draw()
    {
        m_line_shader->use();  // Bind shader

        // Set the uniforms:
        glm::vec4 ourcolor = get_color(linecolor_);  // get the color
        m_line_shader->setVec4("ourColor", ourcolor); // Set uniform
        m_line_shader->setMat4("view", view_matrix_);
        m_line_shader->setMat4("projection", projection_matrix_);
        
        // Set linewidth uniform
         
        if (linewidth_ > MAX_LINE_WIDTH) {linewidth_ = MAX_LINE_WIDTH;}  // Clamping the value
        else if (linewidth_ < MIN_LINE_WIDTH) {linewidth_ = MIN_LINE_WIDTH;}
        m_line_shader->setFloat("thickness", linewidth_*LINEWIDTH_SCALING_FACTOR);

        // Draw line
        glEnable(GL_MULTISAMPLE);  
        glBindVertexArray(vao_);
        glDrawArrays(GL_LINE_STRIP, 0, vertices_.size()); 
        glBindVertexArray(0);  // Unbind vao
    }

private:
    Color linecolor_ = Color::RED;
    float linewidth_ = DEFAULT_LINE_WIDTH;
    std::vector<SimpleVertex> vertices_;
    glm::mat4 view_matrix_ = glm::mat4(1.0f);
    glm::mat4 projection_matrix_ = glm::mat4(1.0f);
    unsigned int vao_, vbo_;
    
    Shader* m_line_shader;
};