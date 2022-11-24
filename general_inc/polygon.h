#include <glm/glm.hpp>
#include <Eigen/Core>

#include <vector>
#include <stdexcept>

#include <QOpenGLContext> 
#include <QOpenGLFunctions_3_3_Core>

#include <general_inc/shader.h>
#include <general_inc/line.h>
#include <general_inc/utilities.h> // colors

constexpr int MAX_POLYGONS = 1000000; // Max number of polygons that can be draw in one call

/// Class for drawin convex shaped polygons 
// Note: for concave polygon we'd need to use polygon triangulation or stencil buffers hacks
// https://stackoverflow.com/questions/25463015/black-out-everything-outside-a-polygon/25463682#25463682
class Polygon: protected QOpenGLFunctions_3_3_Core
{
public:
    
    Polygon() = delete; // need to at least give some coordinates

    Polygon(std::vector<std::vector<Eigen::Vector3f>> polygons, Color fill_color = Color::GREEN,
            float linewidth = DEFAULT_LINE_WIDTH, Color linecolor = Color::GREEN)
    {
        linewidth_ = linewidth;
        linecolor_ = linecolor;
        polygons_ = polygons;
        polygons_count_ = polygons.size();

        // Polygon shader
        const char* polygon_vertex_shader_path = "/home/t.clar/Repos/openGLQt/shaders/polygon.vs";
        const char* polygon_fragment_shader_path = "/home/t.clar/Repos/openGLQt/shaders/polygon.fs";
        m_polygon_shader = new Shader(polygon_vertex_shader_path, polygon_fragment_shader_path);

        // // Create the outline lines
        // m_outline_lines = new Line(the_coordinates, linewidth); 

        SimpleVertex vertex;
        
        unsigned int element_start_index = 0;
        unsigned int polygon_size = 0;

        for(std::size_t i = 0; i < polygons_count_; ++i) {

            const std::vector<Eigen::Vector3f>& polygon = polygons[i];
            polygon_size = polygon.size();
            element_vertex_count_[i] = (GLsizei)polygon_size;
            elements_start_indexes_[i] = (GLuint)element_start_index;

            for (const Eigen::Vector3f& coordinate : polygon) {
                glm::vec3 vector; 
                // positions
                vector.x = coordinate[0];
                vector.y = coordinate[1];
                vector.z = coordinate[2];
                vertex.Position = vector;

                vertices_.push_back(vertex);
            }

            element_start_index += polygon_size;
        }

        initializeOpenGLFunctions();   // Initialise current context  (required)
 
        // Setup opengl states
        setup(); 
    }

    ~Polygon() {
        // delete m_outline_lines;
        delete m_polygon_shader;
    }

    void setup()
    {
        // Create the buffers and array:
        glGenVertexArrays(1, &vao_);
        glGenBuffers(1, &vbo_);

        glStencilMask(0x00);

        glStencilFunc(GL_ALWAYS, 1, 0xFF);
        glStencilMask(0xFF);
        
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
        m_polygon_shader->use();  // Bind shader
        // Set the uniforms:
        glm::vec4 ourcolor = get_color(linecolor_);  // get the color
        m_polygon_shader->setVec4("ourColor", ourcolor); // Set uniform
        m_polygon_shader->setMat4("view", view_matrix);
        m_polygon_shader->setMat4("projection", projection_matrix);
        
        // // Set linewidth uniform
         
        // if (linewidth_ > MAX_LINE_WIDTH) {linewidth_ = MAX_LINE_WIDTH;}  // Clamping the value
        // else if (linewidth_ < MIN_LINE_WIDTH) {linewidth_ = MIN_LINE_WIDTH;}
        // m_line_shader->setFloat("thickness", linewidth_*LINEWIDTH_SCALING_FACTOR);

        // // Draw line
        glEnable(GL_MULTISAMPLE);  // Antialiasing
        glBindVertexArray(vao_);
        
        glMultiDrawArrays(GL_TRIANGLE_FAN, elements_start_indexes_, element_vertex_count_, polygons_count_); //
        glBindVertexArray(0);  // Unbind vao
    }

private:
    Color polygoncolor_ = Color::GREEN;
    Color linecolor_ = Color::BLUE;
    float linewidth_ = DEFAULT_LINE_WIDTH;
    std::vector<std::vector<Eigen::Vector3f>> polygons_;
    GLuint polygons_count_ = 1;
    GLsizei elements_start_indexes_[MAX_POLYGONS];
    GLsizei element_vertex_count_[MAX_POLYGONS];
    std::vector<SimpleVertex> vertices_;
    unsigned int vao_, vbo_;
    
    Shader* m_polygon_shader;
};