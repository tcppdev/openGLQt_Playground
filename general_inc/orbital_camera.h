#ifndef ORBITAL_CAMERA_H
#define ORBITAL_CAMERA_H

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <vector>
#include <cmath>

const float ORBITAL_MOUSE_SENSITIVITY =  0.01f; 
const float ORBITAL_ZOOM_SENSITIVITY =  0.01f;

// An abstract camera class that processes input and calculates the corresponding Euler Angles, Vectors and Matrices for use in OpenGL
class OrbitalCamera
{
public:

    OrbitalCamera(glm::vec3 target_origin = glm::vec3(0, 0, 0)) {
        target_ = target_origin;
    }

    void process_mouse_movements(float delta_x, float delta_y, bool constrain_pitch = true)
    {
        phi_ += ORBITAL_MOUSE_SENSITIVITY*delta_y;
        theta_ += ORBITAL_MOUSE_SENSITIVITY*delta_x;

        if (constrain_pitch) {
            if (phi_ > M_PI - 0.001) { phi_ = M_PI; }
            if (phi_ < 0.001) { phi_ = 0.001; }
        }

        update_camera_vectors();
    }

    void process_mouse_scroll(int offset)
    {
       //  std::cout << offset << std::endl;
        distance_to_origin_ -= offset*ORBITAL_ZOOM_SENSITIVITY;
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

private:
    // calculates the front vector from the Camera's (updated) Euler Angles
    void update_camera_vectors()
    {
        float x = distance_to_origin_ * sin(phi_) * cos(theta_);
        float y = distance_to_origin_ * cos(phi_);
        float z = distance_to_origin_ * sin(phi_) * sin(theta_);
        glm::vec3 camera_pos(x, y, z);

        view_ = glm::lookAt(camera_pos, target_, camera_up_);
    }

    float distance_to_origin_ = 6.0f;
    float phi_ = M_PI/2;
    float theta_ = M_PI/2;
    glm::vec3 camera_up_ = glm::vec3(0, 1, 0);   // camera up vector
    glm::vec3 target_ = glm::vec3(0, 0, 0);   // Target position
    glm::mat4 view_ = glm::mat4(1.0f);  // view matrix
};

#endif