#version 300 es
precision mediump float;

layout (location = 0) in vec3 aPos;      // Point center position
layout (location = 1) in vec2 aOffset;   // Shape offset in local 2D coordinates

uniform mat4 view;
uniform mat4 projection;
uniform vec3 camera_right_worldspace;
uniform vec3 camera_up_worldspace;
uniform float size;
uniform bool fixed_size;

void main()
{
    // Transform center to clip space
    vec4 center_clip = projection * view * vec4(aPos, 1.0);
    
    if (fixed_size) {
        // Screen-space size: work in NDC
        vec4 center_ndc = center_clip / center_clip.w;
        center_ndc.xy += aOffset * size;
        gl_Position = center_ndc * center_clip.w;
    } else {
        // World-space size: apply billboarding
        // Build billboard offset in world space using camera vectors
        vec3 offset_world = camera_right_worldspace * aOffset.x * size + 
                           camera_up_worldspace * aOffset.y * size;
        
        // Transform offset position
        gl_Position = projection * view * vec4(aPos + offset_world, 1.0);
    }
}
