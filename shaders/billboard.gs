#version 330

layout (points) in;                              
layout (triangle_strip, max_vertices = 50) out; 

uniform float size_x;
uniform float size_y;

void main()
{
    vec4 center_pos = gl_in[0].gl_Position;   // position in clip space (obtain from vertex shader)
    center_pos /= center_pos.w; // Screen space
    center_pos.z -= 0.000001;  // Makes sure billboard is slightly offset from origin point
    
    gl_Position = center_pos;
    EmitVertex();
    gl_Position = vec4(center_pos.x + size_x, center_pos.y, center_pos.z, center_pos.w);
    EmitVertex();
    gl_Position = vec4(center_pos.x + size_x, center_pos.y + size_y, center_pos.z, center_pos.w);
    EmitVertex();
    gl_Position = vec4(center_pos.x, center_pos.y + size_y, center_pos.z, center_pos.w);
    EmitVertex();
    gl_Position = center_pos;
    EmitVertex();
    
    EndPrimitive();
}