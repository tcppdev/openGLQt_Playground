//
// Object bounding box drawing and ray intersection

// 
#include <glm/glm.hpp>

#include <vector>

#include <QOpenGLContext> 
#include <QOpenGLFunctions_3_3_Core>

#include <general_inc/shader.h>
#include <general_inc/utilities.h> // colors


class OBB: protected QOpenGLFunctions_3_3_Core
{
public:
    
    OBB() = delete; // need to at least give some coordinates

    // aabb_min is the box bottom left corner for a right hand cartesian system
    // aabb_max is the box top right corner for a right hand cartesian system
    OBB(glm::vec3 aabb_min, glm::vec3 aabb_max, Color fill_color = Color::GREEN, Color linecolor = Color::BLACK)
    {
        fill_color_ = fill_color;
        linecolor_ = linecolor;
        aabb_min_ = aabb_min;
        aabb_max_ = aabb_max;

        // OBB shader
        const char* obb_vertex_shader_path = "/home/tclar/Repos/openGLQt/shaders/obb.vs";
        const char* obb_fragment_shader_path = "/home/tclar/Repos/openGLQt/shaders/obb.fs";
        obb_shader_ = new Shader(obb_vertex_shader_path, obb_fragment_shader_path);

        // Lets create the vertex position array
        vertices_.emplace_back(glm::vec3(aabb_min_.x, aabb_min_.y, aabb_min_.z));
        vertices_.emplace_back(glm::vec3(aabb_max_.x, aabb_min_.y, aabb_min_.z));
        vertices_.emplace_back(glm::vec3(aabb_max_.x, aabb_min_.y, aabb_max_.z));
        vertices_.emplace_back(glm::vec3(aabb_min_.x, aabb_min_.y, aabb_max_.z));
        vertices_.emplace_back(glm::vec3(aabb_min_.x, aabb_max_.y, aabb_min_.z));
        vertices_.emplace_back(glm::vec3(aabb_max_.x, aabb_max_.y, aabb_min_.z));
        vertices_.emplace_back(glm::vec3(aabb_max_.x, aabb_max_.y, aabb_max_.z));
        vertices_.emplace_back(glm::vec3(aabb_min_.x, aabb_max_.y, aabb_max_.z));
        
        // Now the vertex index array
        indices_ = {0, 1, 2,
                    0, 2, 3,
                    1, 5, 6,
                    1, 6, 2,
                    0, 4, 7,
                    0, 7, 3,
                    0, 1, 5,
                    0, 5, 4,
                    3, 2, 6,
                    3, 6, 7,
                    4, 5, 6,
                    4, 6, 7};

        line_indices_ = {0, 1,
                         1, 2,
                         2, 3,
                         3, 0,
                         4, 5,
                         5, 6,
                         6, 7,
                         7, 4,
                         2, 6,
                         1, 5,
                         3, 7,
                         0, 4};

        initializeOpenGLFunctions();   // Initialise current context  (required)
 
        // Setup opengl states
        setup(); 
    }

    ~OBB() {
        // delete m_outline_lines;
        delete obb_shader_;
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

    void draw(glm::mat4 view_matrix = glm::mat4(1.0f), glm::mat4 projection_matrix = glm::mat4(1.0f), glm::mat4 model_matrix = glm::mat4(1.0f))
    {
        obb_shader_->use();  // Bind shader

        // Set the uniforms:
        glm::vec4 ourcolor = get_color(fill_color_);  // get the color
        obb_shader_->setVec4("ourColor", ourcolor); // Set uniform
        obb_shader_->setMat4("view", view_matrix);
        obb_shader_->setMat4("projection", projection_matrix);
        obb_shader_->setMat4("model", model_matrix);
    
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
        ourcolor = get_color(linecolor_);
        obb_shader_->setVec4("ourColor", ourcolor); // Set uniform
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, line_indices_.size()*sizeof(unsigned int), &line_indices_[0], GL_STATIC_DRAW);  // Update element buffer
        // for lines
        glDrawElements(GL_LINES, (unsigned int)line_indices_.size(), GL_UNSIGNED_INT, 0);

        glBindVertexArray(0);  // Unbind vao
    }

    // ray_ndc: incoming ray in normalised device coordinates
    bool test_ray_tracing(
        glm::vec3 ray_world_origin,        // Ray origin, in world space
        glm::vec3 ray_world_direction,     // Ray direction (NOT target position!), in world space. Must be normalize()'d.
        glm::mat4 model_matrix       // Transformation applied to the mesh (which will thus be also applied to its bounding box),
    ){
        // Compute local axes (wrt world axes) R matrix
        glm::vec3 x_axis(model_matrix[0].x, model_matrix[0].y, model_matrix[0].z);
        glm::vec3 y_axis(model_matrix[1].x, model_matrix[1].y, model_matrix[1].z);
        glm::vec3 z_axis(model_matrix[2].x, model_matrix[2].y, model_matrix[2].z);

        // Compute scaling matrix
        // glm::mat3 S = glm::mat3(1.0f);
        // S[0].x = glm::length(x_axis);
        // S[1].y = glm::length(y_axis);
        // S[2].z = glm::length(z_axis);

        float x_scaling = glm::length(x_axis);
        float y_scaling = glm::length(y_axis);
        float z_scaling = glm::length(z_axis);

        // Compute scaled bounding box points
        glm::vec3 aabb_min_scaled; // = S * aabb_min_;
        glm::vec3 aabb_max_scaled; // = S * aabb_max_;

        aabb_min_scaled.x = aabb_min_.x * x_scaling;
        aabb_min_scaled.y = aabb_min_.y * y_scaling;
        aabb_min_scaled.z = aabb_min_.z * z_scaling;

        aabb_max_scaled.x = aabb_max_.x * x_scaling;
        aabb_max_scaled.y = aabb_max_.y * y_scaling;
        aabb_max_scaled.z = aabb_max_.z * z_scaling;

        // Intersection method from Real-Time Rendering and Essential Mathematics for Games
        
        float tMin = 0.0f;
        float tMax = 100000000.0f;

        glm::vec3 OBBposition_worldspace(model_matrix[3].x, model_matrix[3].y, model_matrix[3].z);

        glm::vec3 delta = OBBposition_worldspace - ray_world_origin;

        // Compute scaling from model matrix

        // Test intersection with the 2 planes perpendicular to the OBB's X axis
        {
            glm::vec3 xaxis(model_matrix[0].x, model_matrix[0].y, model_matrix[0].z);
            float e = glm::dot(xaxis, delta);
            float f = glm::dot(ray_world_direction, xaxis);

            if ( fabs(f) > 0.001f ){ // Standard case

                float t1 = (e+aabb_min_scaled.x)/f; // Intersection with the "left" plane
                float t2 = (e+aabb_max_scaled.x)/f; // Intersection with the "right" plane
                // t1 and t2 now contain distances betwen ray origin and ray-plane intersections

                // We want t1 to represent the nearest intersection, 
                // so if it's not the case, invert t1 and t2
                if (t1>t2){
                    float w=t1;t1=t2;t2=w; // swap t1 and t2
                }

                // tMax is the nearest "far" intersection (amongst the X,Y and Z planes pairs)
                if ( t2 < tMax )
                    tMax = t2;
                // tMin is the farthest "near" intersection (amongst the X,Y and Z planes pairs)
                if ( t1 > tMin )
                    tMin = t1;

                // And here's the trick :
                // If "far" is closer than "near", then there is NO intersection.
                // See the images in the tutorials for the visual explanation.
                if (tMax < tMin )
                    return false;

            }else{ // Rare case : the ray is almost parallel to the planes, so they don't have any "intersection"
                if(-e+aabb_min_scaled.x > 0.0f || -e+aabb_max_scaled.x < 0.0f)
                    return false;
            }
        }


        // Test intersection with the 2 planes perpendicular to the OBB's Y axis
        // Exactly the same thing than above.
        {
            glm::vec3 yaxis(model_matrix[1].x, model_matrix[1].y, model_matrix[1].z);
            float e = glm::dot(yaxis, delta);
            float f = glm::dot(ray_world_direction, yaxis);

            if ( fabs(f) > 0.001f ){

                float t1 = (e+aabb_min_scaled.y)/f;
                float t2 = (e+aabb_max_scaled.y)/f;

                if (t1>t2){float w=t1;t1=t2;t2=w;}

                if ( t2 < tMax )
                    tMax = t2;
                if ( t1 > tMin )
                    tMin = t1;
                if (tMin > tMax)
                    return false;

            }else{
                if(-e+aabb_min_scaled.y > 0.0f || -e+aabb_max_scaled.y < 0.0f)
                    return false;
            }
        }


        // Test intersection with the 2 planes perpendicular to the OBB's Z axis
        // Exactly the same thing than above.
        {
            glm::vec3 zaxis(model_matrix[2].x, model_matrix[2].y, model_matrix[2].z);
            float e = glm::dot(zaxis, delta);
            float f = glm::dot(ray_world_direction, zaxis);

            if ( fabs(f) > 0.001f ){

                float t1 = (e+aabb_min_scaled.z)/f;
                float t2 = (e+aabb_max_scaled.z)/f;

                if (t1>t2){float w=t1;t1=t2;t2=w;}

                if ( t2 < tMax )
                    tMax = t2;
                if ( t1 > tMin )
                    tMin = t1;
                if (tMin > tMax)
                    return false;

            }else{
                if(-e+aabb_min_scaled.z > 0.0f || -e+aabb_max_scaled.z < 0.0f)
                    return false;
            }
        }

        float intersection_distance = tMin;  // Distance between ray_world_origin and the intersection with the OBB
        return true;

    }

private:
    Color fill_color_ = Color::GREEN;
    Color linecolor_ = Color::BLUE;

    std::vector<SimpleVertex> vertices_;
    std::vector<unsigned int> indices_;
    std::vector<unsigned int> line_indices_;

    unsigned int vao_, vbo_, ebo_;
    Shader* obb_shader_;

    glm::vec3 aabb_min_;  // is the box bottom left corner for a right hand cartesian system
    glm::vec3 aabb_max_;  // is the box top right corner for a right hand cartesian system
};