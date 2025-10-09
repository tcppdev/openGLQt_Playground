#pragma once

#include <Eigen/Core>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <QOpenGLContext> 
#ifdef __EMSCRIPTEN__
#include <QOpenGLFunctions>
#include <QOpenGLExtraFunctions>
#else
#include <QOpenGLFunctions_3_3_Core>
#endif

#include <shader.h>
#include <utilities.h> // colors

#ifdef __EMSCRIPTEN__
class BillboardPolygon: protected QOpenGLExtraFunctions
#else
class BillboardPolygon: protected QOpenGLFunctions_3_3_Core
#endif
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
        std::string vertex_path = BILLBOARD_VS.string();
        std::string fragment_path = BILLBOARD_FS.string();
#ifdef __EMSCRIPTEN__
        // WebAssembly: no geometry shader, use null pointer
        m_shader = new Shader(vertex_path.c_str(), fragment_path.c_str(), nullptr);
#else
        std::string geometry_path = BILLBOARD_GS.string();
        m_shader = new Shader(vertex_path.c_str(), fragment_path.c_str(), geometry_path.c_str());
#endif

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
        m_size_x = size_x + width_margins_;  // in clip space
        m_size_y = size_y + height_margins_; // in clip space

#ifdef __EMSCRIPTEN__
        // Update quad vertices for WebAssembly
        update_quad_vertices();
#else
        // Update single point for native (geometry shader will expand it)
        glBindBuffer(GL_ARRAY_BUFFER, vbo_);
        glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(glm::vec3), &m_top_left_pos[0]);
        glBindBuffer(GL_ARRAY_BUFFER, 0);
#endif
    }

    void setup()
    {
        // Create the buffers and array:
        glGenVertexArrays(1, &vao_);
        glGenBuffers(1, &vbo_);

        glBindVertexArray(vao_);  

#ifdef __EMSCRIPTEN__
        // WebAssembly: Generate quad geometry (6 vertices for 2 triangles)
        update_quad_vertices();
        glBindBuffer(GL_ARRAY_BUFFER, vbo_);
        glBufferData(GL_ARRAY_BUFFER, 6 * sizeof(glm::vec3), quad_vertices_, GL_DYNAMIC_DRAW);
#else
        // Native: Single point (geometry shader will expand it)
        glBindBuffer(GL_ARRAY_BUFFER, vbo_);
        glBufferData(GL_ARRAY_BUFFER, 1 * sizeof(glm::vec3), &m_top_left_pos[0], GL_STATIC_DRAW);
#endif

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

#ifndef __EMSCRIPTEN__
		// http://www.opengl-tutorial.org/intermediate-tutorials/billboards-particles/billboards/
		m_shader->setVec3("camera_right_worldspace", view_matrix[0][0], view_matrix[1][0], view_matrix[2][0]);
		m_shader->setVec3("camera_up_worldspace", view_matrix[0][1], view_matrix[1][1], view_matrix[2][1]);
        // m_shader->setBool("fixed_size", fixed_size);
        m_shader->setFloat("size_x", m_size_x);
        m_shader->setFloat("size_y", m_size_y);
#endif

        m_shader->setMat4("projection", projection_matrix);
        m_shader->setMat4("view", view_matrix);
        m_shader->setVec4("billboardColor", m_color); // Set uniform
        glBindVertexArray(vao_);

#ifdef __EMSCRIPTEN__
        // WebAssembly: Draw triangles (6 vertices = 2 triangles forming a quad)
        glDrawArrays(GL_TRIANGLES, 0, 6);
#else
        // Native: Draw point (geometry shader will expand it)
        glDrawArrays(GL_POINTS, 0, 1);
#endif
        glBindVertexArray(0);  // Unbind vao
    }

private:

#ifdef __EMSCRIPTEN__
    // Helper method to generate quad vertices for WebAssembly
    void update_quad_vertices()
    {
        // Generate 6 vertices for 2 triangles forming a quad
        // Triangle 1: top-left, top-right, bottom-left
        // Triangle 2: bottom-left, top-right, bottom-right
        
        // Top-left
        quad_vertices_[0] = m_top_left_pos;
        
        // Top-right
        quad_vertices_[1] = glm::vec3(m_top_left_pos.x + m_size_x, m_top_left_pos.y, m_top_left_pos.z);
        
        // Bottom-left
        quad_vertices_[2] = glm::vec3(m_top_left_pos.x, m_top_left_pos.y + m_size_y, m_top_left_pos.z);
        
        // Bottom-left (triangle 2)
        quad_vertices_[3] = glm::vec3(m_top_left_pos.x, m_top_left_pos.y + m_size_y, m_top_left_pos.z);
        
        // Top-right (triangle 2)
        quad_vertices_[4] = glm::vec3(m_top_left_pos.x + m_size_x, m_top_left_pos.y, m_top_left_pos.z);
        
        // Bottom-right
        quad_vertices_[5] = glm::vec3(m_top_left_pos.x + m_size_x, m_top_left_pos.y + m_size_y, m_top_left_pos.z);
        
        // Update VBO
        glBindBuffer(GL_ARRAY_BUFFER, vbo_);
        glBufferSubData(GL_ARRAY_BUFFER, 0, 6 * sizeof(glm::vec3), quad_vertices_);
        glBindBuffer(GL_ARRAY_BUFFER, 0);
    }
    
    glm::vec3 quad_vertices_[6];  // 6 vertices for 2 triangles
#endif

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
