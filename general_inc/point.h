
#include <Eigen/Core>

#include <vector>
#include <string>
#include <stdexcept>

#include <QOpenGLContext> 
#include <QOpenGLFunctions_3_3_Core>

#include <general_inc/shader.h>

#include <general_inc/text.h>

struct VertexP {   // just want to make sure 
    // position
    glm::vec3 Position;
};

enum class Symbol { CIRCLE, SQUARE, TRIANGLE };

class Point: protected QOpenGLFunctions_3_3_Core
{
public:
    
    Point() = delete; // need to at least give some coordinates

    Point(std::vector<Eigen::Vector3f> coordinates, float size, Symbol symbol = Symbol::SQUARE,
         glm::vec4 color = glm::vec4(0.0, 1.0, 0.0, 1.0), std::string description = "Heks\\newstuff")
    {
        size_ = size;
        symbol_ = symbol;
        color_ = color;
        
        // Point shader
        const char* vertex_shader_path = "/home/t.clar/Repos/openGLQt/shaders/point.vs";
        const char* fragment_shader_path = "/home/t.clar/Repos/openGLQt/shaders/point.fs";
        const char* geometry_shader_path = "/home/t.clar/Repos/openGLQt/shaders/point.gs";
        m_point_shader = new Shader(vertex_shader_path, fragment_shader_path, geometry_shader_path);

        VertexP vertex;

        for (const Eigen::Vector3f& coordinate : coordinates) {// access by const reference  
            glm::vec3 vector; 
            // positions 
            vector.x = coordinate[0];
            vector.y = coordinate[1];
            vector.z = coordinate[2];
            vertex.Position = vector;

            vertices_.push_back(vertex);
        }

        // Billboard rectangle

        // Billboard text
        m_text = new Text3D(description, coordinates.back().x(), coordinates.back().y(), coordinates.back().z(), 1.0f/1200.0f, 0.05, 0.05);//1.0f/600.0f); 
        //
        // billboard = new BillboardPolygon(top_left, size_x, size_y, margin_left, margin_top);

        initializeOpenGLFunctions();   // Initialise current context  (required)
 
        // Setup opengl states
        setup(); 
    }

    ~Point() {
        delete m_point_shader;
    }

    void setup()
    {
        // Create the buffers and array:
        glGenVertexArrays(1, &vao_);
        glGenBuffers(1, &vbo_);

        glBindVertexArray(vao_);  

        // load data into buffers
        glBindBuffer(GL_ARRAY_BUFFER, vbo_);
        glBufferData(GL_ARRAY_BUFFER, vertices_.size() * sizeof(VertexP), &vertices_[0], GL_STATIC_DRAW);  

        // set the vertex attribute pointers:
        // vertex Positions
        glEnableVertexAttribArray(0);	
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(VertexP), (void*)0);

        // glBindVertexArray(0);  // Unbind vao
    }

    void draw(glm::mat4 view_matrix = glm::mat4(1.0f), glm::mat4 projection_matrix = glm::mat4(1.0f))
    {
        m_point_shader->use();  // Bind shader

        // Set the uniforms:
        m_point_shader->setVec4("ourColor", color_); // Set uniform
        m_point_shader->setMat4("view", view_matrix);
        m_point_shader->setMat4("projection", projection_matrix);
        m_point_shader->setFloat("size", size_);

        switch (symbol_) {
            case Symbol::SQUARE: {
                m_point_shader->setBool("square", true);
                break;
            }
            case Symbol::CIRCLE: {
                m_point_shader->setBool("circle", true);
                break;
            }
            case Symbol::TRIANGLE: { 
                m_point_shader->setBool("triangle", true);
                break;
            }
            default: { 
                m_point_shader->setBool("triangle", true);
                break; 
            }  // draw a triangle
        }

        // Draw line
        glBindVertexArray(vao_);
        glDrawArrays(GL_POINTS, 0, vertices_.size()); 
        glBindVertexArray(0);  // Unbind vao

        // Draw text
        m_text->draw(view_matrix, projection_matrix, true);

    }

private:
    glm::vec4 color_ = glm::vec4(1.0, 0.0, 0.0, 1.0);
    float size_ = 5;
    Symbol symbol_ = Symbol::SQUARE;
    std::vector<VertexP> vertices_;
    unsigned int vao_, vbo_;
    
    Shader* m_point_shader;

    // Billboard
    Text3D* m_text;
};