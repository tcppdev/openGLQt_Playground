#version 300 es
precision mediump float;

layout (location = 0) in vec3 aPos;
layout (location = 1) in vec2 aOffset;  // Perpendicular offset from line center

uniform mat4 view;
uniform mat4 projection;
uniform float thickness;

void main()
{
    // Transform to clip space first
    vec4 clipPos = projection * view * vec4(aPos, 1.0);
    
    // Apply offset in clip space, scaled by w component (matching geometry shader)
    clipPos.xy += aOffset * clipPos.w * thickness;
    
    gl_Position = clipPos;
}
