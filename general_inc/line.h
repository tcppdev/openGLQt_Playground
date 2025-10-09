#ifndef _LINE_H_
#define _LINE_H_

#include <glm/glm.hpp>
#include <Eigen/Core>

#include <vector>
#include <stdexcept>

#include <QOpenGLContext> 
#ifdef __EMSCRIPTEN__
#include <QOpenGLFunctions>
#include <QOpenGLExtraFunctions>
#else
#include <QOpenGLFunctions_3_3_Core>
#endif

#include <shader.h>
#include <utilities.h>

#ifdef __EMSCRIPTEN__
// Extended vertex structure for WebAssembly lines (includes offset for vertex shader)
struct SimpleVertexWithOffset {
    glm::vec3 Position;
    glm::vec2 Offset;  // Perpendicular offset direction
};
#endif

#ifdef __EMSCRIPTEN__
class Line: protected QOpenGLExtraFunctions
#else
class Line: protected QOpenGLFunctions_3_3_Core
#endif
{
public:
    
    Line() = delete; // need to at least give some coordinates

    Line(std::vector<std::vector<Eigen::Vector3f>> lines, float linewidth = DEFAULT_LINE_WIDTH, Color linecolor = Color::GREEN)
    {
        linewidth_ = linewidth;
        linecolor_ = linecolor;
        lines_ = lines;
        lines_count_ = lines.size();
        
        // Line shader
        std::string vertex_path = LINE_VS.string();
        std::string fragment_path = LINE_FS.string();
#ifdef __EMSCRIPTEN__
        // WebAssembly: no geometry shader
        m_line_shader = new Shader(vertex_path.c_str(), fragment_path.c_str(), nullptr);
#else
        std::string geometry_path = LINE_GS.string();
        m_line_shader = new Shader(vertex_path.c_str(), fragment_path.c_str(), geometry_path.c_str());
#endif

        SimpleVertex vertex;

        unsigned int element_start_index = 0;
        unsigned int line_size = 0;

#ifdef __EMSCRIPTEN__
        // WebAssembly: Generate quad geometry for thick lines on CPU
        for(std::size_t i = 0; i < lines_count_; ++i) {
            const std::vector<Eigen::Vector3f>& line = lines[i];
            line_size = line.size();
            
            if (line_size < 2) continue; // Need at least 2 points for a line
            
            // For each line segment, we'll generate 6 vertices (2 triangles)
            GLsizei segment_count = line_size - 1;
            element_vertex_count_[i] = segment_count * 6;
            elements_start_indexes_[i] = (GLuint)element_start_index;
            
            // Store line points for geometry generation in setup()
            for (const Eigen::Vector3f& coordinate : line) {
                glm::vec3 vector(coordinate[0], coordinate[1], coordinate[2]);
                vertex.Position = vector;
                vertices_.push_back(vertex);
            }
            
            element_start_index += segment_count * 6;
        }
#else
        // Native: Store points directly (geometry shader will handle expansion)
        for(std::size_t i = 0; i < lines_count_; ++i) {
            const std::vector<Eigen::Vector3f>& line = lines[i];
            line_size = line.size();
            element_vertex_count_[i] = (GLsizei)line_size;
            elements_start_indexes_[i] = (GLuint)element_start_index;

            for (const Eigen::Vector3f& coordinate : line) {
                glm::vec3 vector(coordinate[0], coordinate[1], coordinate[2]);
                vertex.Position = vector;
                vertices_.push_back(vertex);
            }
            
            element_start_index += line_size;
        }
#endif

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

#ifdef __EMSCRIPTEN__
        // WebAssembly: Generate quad geometry with offset data from line points
        std::vector<SimpleVertexWithOffset> quad_vertices;
        generate_line_quads(quad_vertices);
        
        // load quad data into buffers
        glBindBuffer(GL_ARRAY_BUFFER, vbo_);
        glBufferData(GL_ARRAY_BUFFER, quad_vertices.size() * sizeof(SimpleVertexWithOffset), &quad_vertices[0], GL_STATIC_DRAW);
        
        // Set vertex attribute pointers for WebAssembly
        // Position (location = 0)
        glEnableVertexAttribArray(0);	
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(SimpleVertexWithOffset), (void*)0);
        
        // Offset (location = 1)
        glEnableVertexAttribArray(1);
        glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(SimpleVertexWithOffset), (void*)offsetof(SimpleVertexWithOffset, Offset));
#else
        // Native: Use original vertices (geometry shader handles expansion)
        glBindBuffer(GL_ARRAY_BUFFER, vbo_);
        glBufferData(GL_ARRAY_BUFFER, vertices_.size() * sizeof(SimpleVertex), &vertices_[0], GL_STATIC_DRAW);
        
        // Set vertex attribute pointers for native
        glEnableVertexAttribArray(0);	
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(SimpleVertex), (void*)0);
#endif

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

        // Draw lines
#ifndef __EMSCRIPTEN__
        glEnable(GL_MULTISAMPLE);  
#endif
        glBindVertexArray(vao_);
#ifdef __EMSCRIPTEN__
        // WebAssembly: Draw triangles (quads generated on CPU)
        for(GLuint i = 0; i < lines_count_; ++i) {
            glDrawArrays(GL_TRIANGLES, elements_start_indexes_[i], element_vertex_count_[i]);
        }
#else
        // Native: Draw line strips (geometry shader expands to quads)
        glMultiDrawArrays(GL_LINE_STRIP, elements_start_indexes_, element_vertex_count_, lines_count_);
#endif
        glBindVertexArray(0);  // Unbind vao
    }

private:

#ifdef __EMSCRIPTEN__
    // Helper method to generate quad geometry for thick lines in WebAssembly
    void generate_line_quads(std::vector<SimpleVertexWithOffset>& quad_vertices)
    {
        // Match geometry shader behavior exactly:
        // - Calculate 2D perpendicular direction in XY plane
        // - Store normalized perpendicular as offset (vertex shader applies thickness and w-scaling)
        
        size_t vertex_index = 0;
        for(std::size_t i = 0; i < lines_count_; ++i) {
            size_t line_point_count = 0;
            // Count vertices for this line
            for(size_t j = 0; j < vertices_.size(); ++j) {
                if (j >= vertex_index) {
                    line_point_count++;
                    if (line_point_count >= lines_[i].size()) break;
                }
            }
            
            // Generate quads for each segment
            for(size_t j = 0; j < line_point_count - 1; ++j) {
                glm::vec3 p1 = vertices_[vertex_index + j].Position;
                glm::vec3 p2 = vertices_[vertex_index + j + 1].Position;
                
                // Match geometry shader exactly:
                // vec2 dir = normalize(p2.xy - p1.xy);
                glm::vec2 dir = glm::normalize(glm::vec2(p2.x - p1.x, p2.y - p1.y));
                
                // vec2 normal = vec2(-dir.y, dir.x);
                glm::vec2 normal = glm::vec2(-dir.y, dir.x);
                
                // Store the normalized perpendicular direction (vertex shader will scale by thickness and w)
                SimpleVertexWithOffset v;
                
                // Triangle 1 vertices (matching geometry shader order)
                // p1 + offset
                v.Position = p1;
                v.Offset = normal;
                quad_vertices.push_back(v);
                
                // p1 - offset
                v.Position = p1;
                v.Offset = -normal;
                quad_vertices.push_back(v);
                
                // p2 + offset
                v.Position = p2;
                v.Offset = normal;
                quad_vertices.push_back(v);
                
                // Triangle 2 vertices
                // p2 + offset (reuse from triangle 1)
                v.Position = p2;
                v.Offset = normal;
                quad_vertices.push_back(v);
                
                // p1 - offset (reuse from triangle 1)
                v.Position = p1;
                v.Offset = -normal;
                quad_vertices.push_back(v);
                
                // p2 - offset
                v.Position = p2;
                v.Offset = -normal;
                quad_vertices.push_back(v);
            }
            
            vertex_index += line_point_count;
        }
    }
#endif

    Color linecolor_ = Color::GREEN;
    float linewidth_ = DEFAULT_LINE_WIDTH;
    std::vector<SimpleVertex> vertices_;
    unsigned int vao_, vbo_;
    Shader* m_line_shader;

    std::vector<std::vector<Eigen::Vector3f>> lines_;
    GLuint lines_count_ = 1;
    GLsizei elements_start_indexes_[MAX_FEATURES];
    GLsizei element_vertex_count_[MAX_FEATURES];
};

#endif
