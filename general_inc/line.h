#ifndef _LINE_H_
#define _LINE_H_

#include <glm/glm.hpp>
#include <Eigen/Core>

#include <vector>
#include <stdexcept>

#include <QOpenGLContext> 
#include <QOpenGLFunctions_3_3_Core>

#include <general_inc/shader.h>
#include <general_inc/utilities.h>

class Line: protected QOpenGLFunctions_3_3_Core
{
public:
    
    Line() = delete; // need to at least give some coordinates

    Line(std::vector<Eigen::Vector3f> coordinates, float linewidth = DEFAULT_LINE_WIDTH, Color linecolor = Color::GREEN)
    {
        linewidth_ = linewidth;
        linecolor_ = linecolor;
        
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

    void draw(glm::mat4 view_matrix = glm::mat4(1.0f), glm::mat4 projection_matrix = glm::mat4(1.0f))
    {
        m_line_shader->use();  // Bind shader

        // Set the uniforms:
        glm::vec4 ourcolor = get_color(linecolor_);  // get the color
        m_line_shader->setVec4("ourColor", ourcolor); // Set uniform
        m_line_shader->setMat4("view", view_matrix);
        m_line_shader->setMat4("projection", projection_matrix);
        
        // Set linewidth uniform
         
        if (linewidth_ > MAX_LINE_WIDTH) {linewidth_ = MAX_LINE_WIDTH;}  // Clamping the value
        else if (linewidth_ < MIN_LINE_WIDTH) {linewidth_ = MIN_LINE_WIDTH;}
        m_line_shader->setFloat("thickness", linewidth_*LINEWIDTH_SCALING_FACTOR);

        // Draw line
        glEnable(GL_MULTISAMPLE);  
        glBindVertexArray(vao_);
        // glMultiDrawArrays(GL_LINE_STRIP, line_start_indexes_, line_vertex_count_, line_count_);
        glDrawArrays(GL_LINE_STRIP, 0, vertices_.size()); 
        glBindVertexArray(0);  // Unbind vao
    }

private:
    Color linecolor_ = Color::GREEN;
    float linewidth_ = DEFAULT_LINE_WIDTH;
    std::vector<SimpleVertex> vertices_;
    unsigned int vao_, vbo_;
    
    Shader* m_line_shader;
};

#endif