
#include <Eigen/Core>

#include <vector>
#include <string>
#include <stdexcept>

#include <QOpenGLContext> 
#include <QOpenGLFunctions_3_3_Core>

#include <general_inc/shader.h>

#include <general_inc/text.h>
#include <general_inc/billboard.h>

struct VertexP {   // just want to make sure 
    // position
    glm::vec3 Position;
};

struct GeoPoint {

    Eigen::Vector3f coordinate;
    std::string description;

    GeoPoint(Eigen::Vector3f coordinate_in, std::string description_in):
             coordinate(coordinate_in), description(description_in) {}
};

enum class Symbol { CIRCLE, SQUARE, TRIANGLE };

class Point: protected QOpenGLFunctions_3_3_Core
{
public:
    
    Point() = delete; // need to at least give some coordinates

    Point(std::vector<GeoPoint> geopoints, float size, 
         Symbol symbol = Symbol::SQUARE,  bool fixed_size = false,
         glm::vec4 color = glm::vec4(0.0, 1.0, 0.0, 1.0))
    {
        geopoints_ = geopoints;
        size_ = size;
        symbol_ = symbol;
        color_ = color;
        fixed_size_ = fixed_size;
        
        // Point shader
        const char* vertex_shader_path = "/home/tclar/Repos/openGLQt/shaders/point.vs";
        const char* fragment_shader_path = "/home/tclar/Repos/openGLQt/shaders/point.fs";
        const char* geometry_shader_path = "/home/tclar/Repos/openGLQt/shaders/point.gs";
        m_point_shader = new Shader(vertex_shader_path, fragment_shader_path, geometry_shader_path);

        VertexP vertex;

        for (auto const& geopoint : geopoints) {// access by const reference  
            glm::vec3 vector; 
            // positions 
            vector.x = geopoint.coordinate[0];
            vector.y = geopoint.coordinate[1];
            vector.z = geopoint.coordinate[2];
            vertex.Position = vector;

            vertices_.push_back(vertex);
        }

        // Billboard rectangle

        // Billboard text (set ramdon initial positions/text)
        m_text = new Text3D("hecls\noshfosei\ndfca", 0.0, 0.0, 0.0, 1.0f/2000.0f, 
                            {1, 0, 0}, 0.0, 0.0);//1.0f/600.0f); 

        m_billboard = new BillboardPolygon(geopoints.back().coordinate, m_text->get_text_screen_size().first, 
                                           m_text->get_text_screen_size().second, 0, 0, {1.0, 1.0, 1.0, 0.5});
        // m_billboard = new BillboardPolygon(Eigen::Vector3f({0, 0, 0}), 0.4, 
        //                                    0.5, 0, 0, {1.0, 1.0, 1.0, 0.5});

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

    // Args:
    // ray_ndc: incoming ray in normalised device coordinates
    bool test_ray_tracing(glm::mat4 view_matrix = glm::mat4(1.0f), glm::mat4 projection_matrix = glm::mat4(1.0f), glm::vec3 ray_ndc = glm::vec3(0.0f)) {
        
        // Test ray tracing intersection 
        draw_description_ = false;

        if (fixed_size_)  // Test intersection in ndc
        {
            // for (std::size_t i = 0; i < vertices_.size(); i++)
            // {
                // VertexP vertex = vertices_[i];
                // glm::vec4 vertex_center_clip = projection_matrix * view_matrix * glm::vec4(vertex.Position, 1.0);
                // glm::vec4 vertex_center_clip_norm = glm::normalize(vertex_center_clip);
                // glm::vec4 vertex_center_ndc = vertex_center_clip_norm;

                // glm::vec4 size_circle_clip = glm::normalize(vertex_center_clip + glm::vec4(size_, 0.0, 0.0, 0.0));
                // glm::vec4 size_circle_ndc = size_circle_clip;

                // float radius_ndc = abs(size_circle_ndc.x - vertex_center_ndc.x);
                // float radius_ray = std::pow(std::pow(ray_ndc.x-vertex_center_ndc.x, 2) + std::pow(ray_ndc.y-vertex_center_ndc.y, 2), 0.5); 

                // if (radius_ray <= radius_ndc)
                // {
                //     draw_description_ = true;
                //     description_index = i;
                // }
            // }
        }
        else {  // Intersection in model coordinates

            // std::cout << ray_clip.x << " " << ray_clip.y << " " <<ray_clip.z << " " << ray_clip.w << std::endl;

            for (std::size_t i = 0; i < vertices_.size(); i++)
            {
                VertexP vertex = vertices_[i];
                glm::vec4 vertex_center_clip = projection_matrix * view_matrix * glm::vec4(vertex.Position, 1.0);
                glm::vec4 vertex_center_clip_norm = glm::normalize(vertex_center_clip);
                glm::vec4 vertex_center_ndc = vertex_center_clip_norm/vertex_center_clip_norm.w;

                glm::vec4 size_circle_clip = glm::normalize(vertex_center_clip + glm::vec4(size_, 0.0, 0.0, 0.0));
                glm::vec4 size_circle_ndc = size_circle_clip/size_circle_clip.w;

                float radius_ndc = abs(size_circle_ndc.x - vertex_center_ndc.x);
                float radius_ray = std::pow(std::pow(ray_ndc.x-vertex_center_ndc.x, 2) + std::pow(ray_ndc.y-vertex_center_ndc.y, 2), 0.5); 

                if (radius_ray <= radius_ndc)
                {
                    draw_description_ = true;
                    description_index = i;
                }
            }
        }

        if (draw_description_)
        {
            GeoPoint geopoint = geopoints_[description_index];
            m_text->change_text(geopoint.description, geopoint.coordinate.x(), geopoint.coordinate.y(), geopoint.coordinate.z());
            m_billboard->change_billboard(geopoint.coordinate, m_text->get_text_screen_size().first, m_text->get_text_screen_size().second);   
        }

        return false;
    }
    
    void draw(glm::mat4 view_matrix = glm::mat4(1.0f), glm::mat4 projection_matrix = glm::mat4(1.0f))
    {
        m_point_shader->use();  // Bind shader

        // Set the uniforms:
        m_point_shader->setVec4("ourColor", color_); // Set uniform
        m_point_shader->setMat4("view", view_matrix);
        m_point_shader->setMat4("projection", projection_matrix);
        m_point_shader->setBool("fixed_size", fixed_size_);
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
        // m_billboard->draw(view_matrix, projection_matrix);
        
        if (draw_description_) 
        {
            m_billboard->draw(view_matrix, projection_matrix);
            m_text->draw(view_matrix, projection_matrix, true);
        }
    }

private:
    glm::vec4 color_ = glm::vec4(1.0, 0.0, 0.0, 1.0);
    float size_ = 5;
    bool fixed_size_ = false;
    Symbol symbol_ = Symbol::SQUARE;
    std::vector<VertexP> vertices_;
    unsigned int vao_, vbo_;
    
    Shader* m_point_shader;

    // Billboard
    Text3D* m_text;
    BillboardPolygon* m_billboard;

    std::vector<GeoPoint> geopoints_;
    bool draw_description_ = false;
    std::size_t description_index = 0;
};