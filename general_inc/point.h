
#include <Eigen/Core>

#include <vector>
#include <string>
#include <stdexcept>

#include <QOpenGLContext> 
#ifdef __EMSCRIPTEN__
#include <QOpenGLFunctions>
#include <QOpenGLExtraFunctions>
#else
#include <QOpenGLFunctions_3_3_Core>
#endif

#include <shader.h>

#include <text.h>
#include <billboard.h>

struct VertexP {   // just want to make sure 
    // position
    glm::vec3 Position;
};

#ifdef __EMSCRIPTEN__
// Extended vertex structure for WebAssembly points (includes offset for billboarding)
struct VertexPWithOffset {
    glm::vec3 Position;  // Point center
    glm::vec2 Offset;    // 2D offset in shape-local coordinates
};
#endif

struct GeoPoint {

    Eigen::Vector3f coordinate;
    std::string description;

    GeoPoint(Eigen::Vector3f coordinate_in, std::string description_in):
             coordinate(coordinate_in), description(description_in) {}
};

enum class Symbol { CIRCLE, SQUARE, TRIANGLE };

#ifdef __EMSCRIPTEN__
class Point: protected QOpenGLExtraFunctions
#else
class Point: protected QOpenGLFunctions_3_3_Core
#endif
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
        std::string vertex_path = POINT_VS.string();
        std::string fragment_path = POINT_FS.string();
#ifdef __EMSCRIPTEN__
        // WebAssembly: no geometry shader
        m_point_shader = new Shader(vertex_path.c_str(), fragment_path.c_str(), nullptr);
#else
        std::string geometry_path = POINT_GS.string();
        m_point_shader = new Shader(vertex_path.c_str(), fragment_path.c_str(), geometry_path.c_str());
#endif

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

#ifdef __EMSCRIPTEN__
        // WebAssembly: Generate shape geometry with billboarding offsets
        std::vector<VertexPWithOffset> shape_vertices;
        generate_point_shapes(shape_vertices);
        
        // load shape data into buffers
        glBindBuffer(GL_ARRAY_BUFFER, vbo_);
        glBufferData(GL_ARRAY_BUFFER, shape_vertices.size() * sizeof(VertexPWithOffset), &shape_vertices[0], GL_STATIC_DRAW);
        
        // Set vertex attribute pointers for WebAssembly
        // Position (location = 0)
        glEnableVertexAttribArray(0);	
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(VertexPWithOffset), (void*)0);
        
        // Offset (location = 1)
        glEnableVertexAttribArray(1);
        glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(VertexPWithOffset), (void*)offsetof(VertexPWithOffset, Offset));
#else
        // Native: Use point vertices (geometry shader handles expansion)
        glBindBuffer(GL_ARRAY_BUFFER, vbo_);
        glBufferData(GL_ARRAY_BUFFER, vertices_.size() * sizeof(VertexP), &vertices_[0], GL_STATIC_DRAW);
        
        // Set vertex attribute pointers for native
        glEnableVertexAttribArray(0);	
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(VertexP), (void*)0);
#endif

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

#ifdef __EMSCRIPTEN__
        // WebAssembly: Pass camera vectors for billboarding
        // Extract camera right and up vectors from view matrix
        m_point_shader->setVec3("camera_right_worldspace", view_matrix[0][0], view_matrix[1][0], view_matrix[2][0]);
        m_point_shader->setVec3("camera_up_worldspace", view_matrix[0][1], view_matrix[1][1], view_matrix[2][1]);
#endif

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

        // Draw shapes
        glBindVertexArray(vao_);
#ifdef __EMSCRIPTEN__
        // WebAssembly: Draw triangles (shapes generated on CPU)
        glDrawArrays(GL_TRIANGLES, 0, wasm_vertex_count_);
#else
        // Native: Draw points (geometry shader expands to shapes)
        glDrawArrays(GL_POINTS, 0, vertices_.size());
#endif
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

#ifdef __EMSCRIPTEN__
    // Helper method to generate shape geometry for points in WebAssembly
    // Generates billboarded shapes using 2D offsets that will be applied by vertex shader
    void generate_point_shapes(std::vector<VertexPWithOffset>& shape_vertices)
    {
        const int circle_segments = 20; // Number of segments for circle approximation
        
        for (const auto& vertex : vertices_) {
            glm::vec3 center = vertex.Position;
            
            switch (symbol_) {
                case Symbol::CIRCLE: {
                    // Generate circle as triangle fan with 2D offsets
                    float angle_step = 2.0f * M_PI / circle_segments;
                    for (int i = 0; i < circle_segments; ++i) {
                        float angle1 = i * angle_step;
                        float angle2 = (i + 1) * angle_step;
                        
                        // Triangle: center, point1, point2
                        VertexPWithOffset v;
                        
                        v.Position = center;
                        v.Offset = glm::vec2(0.0f, 0.0f);  // Center point
                        shape_vertices.push_back(v);
                        
                        v.Position = center;
                        v.Offset = glm::vec2(cos(angle1), sin(angle1));  // First edge point
                        shape_vertices.push_back(v);
                        
                        v.Position = center;
                        v.Offset = glm::vec2(cos(angle2), sin(angle2));  // Second edge point
                        shape_vertices.push_back(v);
                    }
                    break;
                }
                case Symbol::SQUARE: {
                    // Generate square as 2 triangles with 2D offsets
                    VertexPWithOffset v;
                    v.Position = center;
                    
                    // Triangle 1: bottom-left, bottom-right, top-left
                    v.Offset = glm::vec2(-0.5f, -0.5f);
                    shape_vertices.push_back(v);
                    v.Offset = glm::vec2(0.5f, -0.5f);
                    shape_vertices.push_back(v);
                    v.Offset = glm::vec2(-0.5f, 0.5f);
                    shape_vertices.push_back(v);
                    
                    // Triangle 2: bottom-right, top-right, top-left
                    v.Offset = glm::vec2(0.5f, -0.5f);
                    shape_vertices.push_back(v);
                    v.Offset = glm::vec2(0.5f, 0.5f);
                    shape_vertices.push_back(v);
                    v.Offset = glm::vec2(-0.5f, 0.5f);
                    shape_vertices.push_back(v);
                    break;
                }
                case Symbol::TRIANGLE: {
                    // Generate triangle with 2D offsets
                    VertexPWithOffset v;
                    v.Position = center;
                    
                    v.Offset = glm::vec2(-0.5f, -0.5f);
                    shape_vertices.push_back(v);
                    v.Offset = glm::vec2(0.5f, -0.5f);
                    shape_vertices.push_back(v);
                    v.Offset = glm::vec2(0.0f, 0.5f);
                    shape_vertices.push_back(v);
                    break;
                }
            }
        }
        
        wasm_vertex_count_ = shape_vertices.size();
    }
    
    size_t wasm_vertex_count_ = 0;
#endif

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
