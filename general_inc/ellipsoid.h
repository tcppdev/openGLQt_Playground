///////////////////////////////////////////////////////////////////////////////
// Ellipsoid.h
// ==========
// Ellipsoid for OpenGL with (radius, sectors, stacks)
// The min number of sectors is 3 and the min number of stacks are 2.
//

#include <iostream>
#include <iomanip>
#include <cmath>
#include <vector>

#include <QOpenGLContext> 
#include <QOpenGLFunctions_3_3_Core>

// constants //////////////////////////////////////////////////////////////////
const int MIN_SECTOR_COUNT = 3;
const int MIN_STACK_COUNT  = 2;

class Ellipsoid: protected QOpenGLFunctions_3_3_Core
{
 public:

    Ellipsoid(glm::vec3 radius_abc, int sectors, int stacks, Color fill_color = Color::BLUE)
    {
        // Ellipsoid shader
        const char* ellipsoid_vertex_shader_path = "/home/t.clar/Repos/openGLQt/shaders/ellipsoid.vs";
        const char* ellipsoid_fragment_shader_path = "/home/t.clar/Repos/openGLQt/shaders/ellipsoid.fs";
        ellipsoid_shader_ = new Shader(ellipsoid_vertex_shader_path, ellipsoid_fragment_shader_path);

        // First and last element of contour may be 
        fill_color_ = fill_color;
        radius_abc_ = radius_abc;
        sector_count_ = sectors;
        if(sectors < MIN_SECTOR_COUNT)
            sector_count_ = MIN_SECTOR_COUNT;
        stack_count_ = stacks;
        if(sectors < MIN_STACK_COUNT)
            sector_count_ = MIN_STACK_COUNT;

        build_vertices_smooth();  // Build vertices and index array

        initializeOpenGLFunctions();   // Initialise current context  (required)
        setup();  // Setup buffer data for openGL
    }

    void setup()
    {
        // Create the buffers and array:
        glGenVertexArrays(1, &vao_);
        glGenBuffers(1, &vbo_);
        glGenBuffers(1, &ebo_);

        glBindVertexArray(vao_);  

        // load vertex data into buffer
        glBindBuffer(GL_ARRAY_BUFFER, vbo_);
        glBufferData(GL_ARRAY_BUFFER, vertices_.size() * sizeof(SimpleVertex), &vertices_[0], GL_STATIC_DRAW);  

        // load index data into element buffer
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo_);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices_.size()*sizeof(unsigned int), &indices_[0], GL_STATIC_DRAW);

        // set the vertex attribute pointers:
        // vertex Positions
        glEnableVertexAttribArray(0);	
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(SimpleVertex), (void*)0);
    }

    ///////////////////////////////////////////////////////////////////////////////
    // draw a sphere in VertexArray mode
    // OpenGL RC must be set before calling it
    ///////////////////////////////////////////////////////////////////////////////
    void draw(glm::mat4 view_matrix = glm::mat4(1.0f), glm::mat4 projection_matrix = glm::mat4(1.0f), glm::mat4 model_matrix = glm::mat4(1.0f))
    {
        ellipsoid_shader_->use();  // Bind shader

        // Set the uniforms:
        glm::vec4 ourcolor = get_color(fill_color_);  // get the color
        ellipsoid_shader_->setVec4("ourColor", ourcolor); // Set uniform
        ellipsoid_shader_->setMat4("view", view_matrix);
        ellipsoid_shader_->setMat4("projection", projection_matrix);
        ellipsoid_shader_->setMat4("model", model_matrix);
    
        // Draw triangles
        //glEnable(GL_MULTISAMPLE);  // Antialiasing
        glBindVertexArray(vao_);

        // Draw ellipsoid
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices_.size()*sizeof(unsigned int), &indices_[0], GL_STATIC_DRAW);

        glEnable(GL_POLYGON_OFFSET_FILL);
        glPolygonOffset(1.0, 1.0f); // move polygon backward
        glDrawElements(GL_TRIANGLES, (unsigned int)indices_.size(), GL_UNSIGNED_INT, 0);  // Set element buffer for triangle faces
        glDisable(GL_POLYGON_OFFSET_FILL);

        // Draw lines
        ellipsoid_shader_->setVec4("ourColor", glm::vec4(1.0f)); // Set uniform
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, line_indices_.size()*sizeof(unsigned int), &line_indices_[0], GL_STATIC_DRAW);  // Update element buffer
        // for lines
        glDrawElements(GL_LINES, (unsigned int)line_indices_.size(), GL_UNSIGNED_INT, 0);

        glBindVertexArray(0);  // Unbind vao
    }

    // ray_ndc: incoming ray in normalised device coordinates
    bool test_ray_tracing(
        glm::vec3 ray_world_origin,        // Ray origin, in world space
        glm::vec3 ray_world_direction     // Ray direction (NOT target position!), in world space. Must be normalize()'d.
    ){
        glm::vec3 ray_origin = ray_world_origin; //  - ellipsoid_origin;
        float a = ((ray_world_direction.x*ray_world_direction.x)/(radius_abc_.x*radius_abc_.x))
                + ((ray_world_direction.y*ray_world_direction.y)/(radius_abc_.y*radius_abc_.y))
                + ((ray_world_direction.z*ray_world_direction.z)/(radius_abc_.z*radius_abc_.z));
        float b = ((2*ray_origin.x*ray_world_direction.x)/(radius_abc_.x*radius_abc_.x))
                + ((2*ray_origin.y*ray_world_direction.y)/(radius_abc_.y*radius_abc_.y))
                + ((2*ray_origin.z*ray_world_direction.z)/(radius_abc_.z*radius_abc_.z));
        float c = ((ray_origin.x*ray_origin.x)/(radius_abc_.x*radius_abc_.x))
                + ((ray_origin.y*ray_origin.y)/(radius_abc_.y*radius_abc_.y))
                + ((ray_origin.z*ray_origin.z)/(radius_abc_.z*radius_abc_.z))
                - 1;

        float d = ((b*b)-(4*a*c));
        if ( d < 0 ) { return false; }
        else { d = std::pow(d, 0.5); }
        float hit = (-b + d)/(2*a);
        float hitsecond = (-b - d)/(2*a);

        //DEBUG
        //std::cout << "Hit1: " << hit << " Hit2: " << hitsecond << std::endl;

        return true;
        // if( hit < hitsecond) { return hit; }
        // else { return hitsecond; }

    }

 private: 
    ///////////////////////////////////////////////////////////////////////////////
    // dealloc vectors
    ///////////////////////////////////////////////////////////////////////////////
    void clear_arrays()
    {
        std::vector<SimpleVertex>().swap(vertices_);
        std::vector<unsigned int>().swap(indices_);
        std::vector<unsigned int>().swap(line_indices_);
    }

    ///////////////////////////////////////////////////////////////////////////////
    // build vertices of an ellipsoid with smooth shading using parametric equations
    // x = a * cos(u) * cos(v)
    // y = b * cos(u) * sin(v)
    // z = c * sin(u)
    // where u: stack(latitude) angle (-90 <= u <= 90)
    //       v: sector(longitude) angle (0 <= v <= 360)
    // Adapted from http://www.songho.ca/opengl/gl_sphere.html
    ///////////////////////////////////////////////////////////////////////////////
    void build_vertices_smooth()
    {
        const float PI = acos(-1);

        // clear memory of prev arrays
        clear_arrays();

        float x, y, z;                              // vertex position

        float sector_step = 2 * PI / sector_count_;
        float stack_step = PI / stack_count_;
        float sector_angle, stack_angle;

        for(int i = 0; i <= stack_count_; ++i)
        {
            stack_angle = PI / 2 - i * stack_step;        // starting from pi/2 to -pi/2
            // xy = radius_abc_ * cosf(stack_angle);             // r * cos(u)
            z = radius_abc_.z * sinf(stack_angle);              // r * sin(u)

            // add (sector_count_+1) vertices per stack
            // the first and last vertices have same position and normal
            // 
            SimpleVertex vertex;

            for(int j = 0; j <= sector_count_; ++j)
            {
                sector_angle = j * sector_step;           // starting from 0 to 2pi

                // vertex position
                x = radius_abc_.x * cosf(stack_angle) * cosf(sector_angle);             // r * cos(u) * cos(v)
                y = radius_abc_.y * cosf(stack_angle) * sinf(sector_angle);             // r * cos(u) * sin(v)
                
                // positions
                vertex.Position = glm::vec3(x, y, z);
                vertices_.push_back(vertex);
            }
        }

        // indices
        //  k1--k1+1
        //  |  / |
        //  | /  |
        //  k2--k2+1
        unsigned int k1, k2;
        for(int i = 0; i < stack_count_; ++i)
        {
            k1 = i * (sector_count_ + 1);     // beginning of current stack
            k2 = k1 + sector_count_ + 1;      // beginning of next stack

            for(int j = 0; j < sector_count_; ++j, ++k1, ++k2)
            {
                // 2 triangles per sector excluding 1st and last stacks
                if(i != 0)
                {
                    add_indices(k1, k2, k1+1);   // k1---k2---k1+1
                }

                if(i != (stack_count_-1))
                {
                    add_indices(k1+1, k2, k2+1); // k1+1---k2---k2+1
                }

                // vertical lines for all stacks
                line_indices_.push_back(k1);
                line_indices_.push_back(k2);
                if(i != 0)  // horizontal lines except 1st stack
                {
                    line_indices_.push_back(k1);
                    line_indices_.push_back(k1 + 1);
                }
            }
        }
    }


    ///////////////////////////////////////////////////////////////////////////////
    // add 3 indices to array
    ///////////////////////////////////////////////////////////////////////////////
    void add_indices(unsigned int i1, unsigned int i2, unsigned int i3)
    {
        indices_.push_back(i1);
        indices_.push_back(i2);
        indices_.push_back(i3);
    }


    // member vars
    unsigned int vao_, vbo_, ebo_;
    glm::vec3 radius_abc_;
    int sector_count_;                        // longitude, # of slices
    int stack_count_;                         // latitude, # of stacks
    std::vector<SimpleVertex> vertices_;
    std::vector<unsigned int> indices_;
    std::vector<unsigned int> line_indices_;
    
    Shader* ellipsoid_shader_;
    Color fill_color_ = Color::BLUE;
};