#ifndef _UTILITIES_H_
#define _UTILITIES_H_

#include <glm/glm.hpp>

constexpr float DEFAULT_LINE_WIDTH = 5;
constexpr float LINEWIDTH_SCALING_FACTOR = 0.0005; 
constexpr float MIN_LINE_WIDTH = 1;
constexpr float MAX_LINE_WIDTH = 20;

constexpr int MAX_FEATURES = 1000000; // Max number of features (lines/polygons) that can be draw in one call

/// Looks like colours to me
enum class Color { RED, GREEN, BLUE, BLACK, WHITE };
glm::vec4 get_color(Color color)   // use unordered map instead?
{
    switch(color) {
        case(Color::RED): { return glm::vec4(1.0, 0.0, 0.0, 1.0); }
        case(Color::GREEN): { return glm::vec4(0.0, 1.0, 0.0, 1.0); }
        case(Color::BLUE): { return glm::vec4(0.0, 0.0, 1.0, 1.0); }
        case(Color::BLACK): { return glm::vec4(0.0, 0.0, 0.0, 1.0); }
        case(Color::WHITE): { return glm::vec4(1.0, 1.0, 1.0, 1.0); }
        default: { throw std::invalid_argument("Unknown color"); }  // white
    }
}


struct SimpleVertex {   // just want to make sure 
    // position
    glm::vec3 Position;
};

#endif