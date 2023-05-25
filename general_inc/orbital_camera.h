#ifndef ORBITAL_CAMERA_H
#define ORBITAL_CAMERA_H

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
// #include <glm/gtc/quaternion.hpp>

#include <vector>
#include <cmath>
#include "general_inc/utilities.h"

const float ORBITAL_MOUSE_SENSITIVITY_START =  0.01f; 
const float ORBITAL_MOUSE_SENSITIVITY_END =  0.0001f;

const float ORBITAL_ZOOM_SENSITIVITY_START = 10000.0f;
const float ORBITAL_ZOOM_SENSITIVITY_END = 1000.0f;

float lerp(float x0, float x1, float y0, float y1, float xp)
{
    return y0 + ((y1-y0)/(x1-x0))*(xp - x0);
}

float exponential(float x0, float x1, float y0, float y1, float xp)
{
    float b = std::pow((y1/y0), 1/(x1-x0));
    float a = y0/std::pow(b, x0);

    if (xp > x0) {
        // std::cout << "more" << std::endl;
        return y0;
    }
    else if (xp < x1) {
        // std::cout << "less" << std::endl;
        return y1;
    }
    
    return a*std::pow(b, xp);
}

// An abstract camera class that processes input and calculates the corresponding Euler Angles, Vectors and Matrices for use in OpenGL
class OrbitalCamera
{
public:

    OrbitalCamera(glm::vec3 target_origin = glm::vec3(0, 0, 0), 
        float start_distance_to_origin = 8, float min_distance_to_origin = 2.6) {

        distance_to_origin_ = start_distance_to_origin;
        start_distance_to_origin_ = start_distance_to_origin;
        min_distance_to_origin_ = min_distance_to_origin;

        target_ = target_origin;
    }

    void process_mouse_movements(float delta_x, float delta_y, bool constrain_pitch = true)
    {
        float mouse_sensitivity = exponential(start_distance_to_origin_, min_distance_to_origin_,
                                            ORBITAL_MOUSE_SENSITIVITY_START, ORBITAL_MOUSE_SENSITIVITY_END,
                                            distance_to_origin_);
        phi_ += mouse_sensitivity*delta_y;
        theta_ -= mouse_sensitivity*delta_x;

        if (constrain_pitch) {
            if (phi_ > M_PI - 0.001) { phi_ = M_PI - 0.001; }
            if (phi_ < 0.001) { phi_ = 0.001; }
        }

        update_camera_vectors();
    }

    void process_mouse_scroll(int offset)
    {
       //  std::cout << offset << std::endl;
        float zoom_sensitivity = exponential(start_distance_to_origin_, min_distance_to_origin_,
                                            ORBITAL_ZOOM_SENSITIVITY_START, ORBITAL_ZOOM_SENSITIVITY_END,
                                            distance_to_origin_);
        
        distance_to_origin_ -= offset*zoom_sensitivity;

        if (distance_to_origin_ < min_distance_to_origin_)
        {
            distance_to_origin_ = min_distance_to_origin_;
        }
        
        update_camera_vectors();
    }


    glm::mat4 get_view_matrix()
    {
        return view_;
    }

    float get_camera_distance_to_origin()
    {
        return distance_to_origin_;
    }

    glm::vec3 get_camera_current_world_position() {
        return camera_pos_;
    }

    void set_camera_target(glm::vec3 const& new_target) 
    {
        target_ = new_target;
    }

private:
    // calculates the front vector from the Camera's (updated) Euler Angles
    void update_camera_vectors()
    {

        float x = distance_to_origin_ * sin(phi_) * cos(theta_);
        float y = distance_to_origin_ * sin(phi_) * sin(theta_);
        float z = distance_to_origin_ * cos(phi_);
        camera_pos_ = glm::vec3(x, y, z);
        
        // theta_: Rotation around z-axis
        // phi_ : Rotation around x-y plane (0 - 180 degrees)
        // std::cout << x << ", " <<  y << ", " << z << std::endl;
        // std::cout << theta_*180/M_PI << ", " <<  phi_*180/M_PI << std::endl;

        view_ = glm::lookAt(camera_pos_, target_, camera_up_);

        // glm::mat4 look_mat = glm::lookAt(camera_pos_, target_, camera_up_);
        // glm::quat rotation = glm::quat_cast(look_mat);
        // view_ = glm::mat4_cast(rotation);
    }

    float distance_to_origin_ = 8.0f;
    float phi_ = M_PI/2;
    float theta_ = 0; //M_PI/2;
    glm::vec3 camera_up_ = glm::vec3(0, 0, 1);   // camera up vector
    glm::vec3 camera_pos_ = glm::vec3(0, 0, 0);   // camera world position 
    glm::vec3 target_ = glm::vec3(0, 0, 0);   // Target position
    glm::mat4 view_ = glm::mat4(1.0f);  // view matrix

    float start_distance_to_origin_ = 0;  // Start fistance of camera to the origin
    float min_distance_to_origin_ = 0; // Minum distance to origin allowed by camera (zoom)
};

#endif