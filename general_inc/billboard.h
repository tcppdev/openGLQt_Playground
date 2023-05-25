#pragma once

#include <Eigen/Core>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <QOpenGLContext> 
#include <QOpenGLFunctions_3_3_Core>

#include <general_inc/shader.h>
#include <general_inc/utilities.h> // colors

#include <ft2build.h>

class BillboardPolygon: protected QOpenGLFunctions_3_3_Core
{
public:
    
    BillboardPolygon(Eigen::Vector3f top_left, float size_x, float size_y, float width_margins = 0.01, float height_margins = 0.01, glm::vec4 color = {0.0, 0.0, 0.0, 0.5})
    {
        m_size_x = size_x + width_margins;  // in clip space
        m_size_y = size_y + height_margins; // in clip space
        m_color = color;

        width_margins_ = width_margins;
        height_margins_ = height_margins;
        
        initializeOpenGLFunctions();   // Initialise current context  (required)

        // Text shaders
        const char* vertex_billboard = BILLBOARD_VS.string().c_str();
        const char* fragment_billboard = BILLBOARD_FS.string().c_str();
        const char* geometry_billboard = BILLBOARD_GS.string().c_str();
        m_shader = new Shader(vertex_billboard, fragment_billboard, geometry_billboard);

        // positions
        m_top_left_pos = glm::vec3(top_left.x(), top_left.y(), top_left.z());

        setup();
    }

    ~BillboardPolygon() {
        delete m_shader;
    }

    void change_billboard(Eigen::Vector3f top_left, float size_x, float size_y)
    {
        m_top_left_pos = glm::vec3(top_left.x(), top_left.y(), top_left.z());

        // Update content of VBO memory
        glBindBuffer(GL_ARRAY_BUFFER, vbo_);
        glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(glm::vec3), &m_top_left_pos[0]);  // sets data within a specified region
        glBindBuffer(GL_ARRAY_BUFFER, 0);  // Release buffer

        m_size_x = size_x + width_margins_;  // in clip space
        m_size_y = size_y + height_margins_; // in clip space
    }

    void setup()
    {
        // Create the buffers and array:
        glGenVertexArrays(1, &vao_);
        glGenBuffers(1, &vbo_);

        glBindVertexArray(vao_);  

        // load data into buffers
        glBindBuffer(GL_ARRAY_BUFFER, vbo_);
        glBufferData(GL_ARRAY_BUFFER, 1 * sizeof(glm::vec3), &m_top_left_pos[0], GL_STATIC_DRAW);  

        // set the vertex attribute pointers:
        // vertex Positions
        glEnableVertexAttribArray(0);	
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(glm::vec3), (void*)0);
    }

    void draw(glm::mat4 view_matrix, 
              glm::mat4 projection_matrix,
              bool fixed_size = true)
    {
        // OpenGL state
        // ------------
        // glEnable(GL_CULL_FACE);
        // glEnable(GL_BLEND);  // enabling blending 
        // glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);  // setting blending function

        // activate corresponding render state	
        m_shader->use();

		// http://www.opengl-tutorial.org/intermediate-tutorials/billboards-particles/billboards/
		m_shader->setVec3("camera_right_worldspace", view_matrix[0][0], view_matrix[1][0], view_matrix[2][0]);
		m_shader->setVec3("camera_up_worldspace", view_matrix[0][1], view_matrix[1][1], view_matrix[2][1]);
        // m_shader->setBool("fixed_size", fixed_size);
        m_shader->setFloat("size_x", m_size_x);
        m_shader->setFloat("size_y", m_size_y);

        m_shader->setMat4("projection", projection_matrix);
        m_shader->setMat4("view", view_matrix);
        m_shader->setVec4("billboardColor", m_color); // Set uniform
        glBindVertexArray(vao_);

        // Draw point
        glBindVertexArray(vao_);
        glDrawArrays(GL_POINTS, 0, 1);  // Only one point required here
        glBindVertexArray(0);  // Unbind vao
    }

private:

    std::map<GLchar, Character> m_characters;
    Shader* m_shader;
    unsigned int vao_, vbo_;

    glm::vec3 m_top_left_pos;
    float m_size_x;
    float m_size_y;
    float width_margins_;
    float height_margins_;
    std::vector<SimpleVertex> vertices_;
    // float m_x;
    // float m_y;
    // float m_z;
    glm::vec4 m_color = {0.0, 0.0, 1.0, 1.0}; // blue
};