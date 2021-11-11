#version 330

layout (lines) in;                              
layout (triangle_strip, max_vertices = 4) out; 

uniform float thickness;

void main()
{
    vec4 p1 = gl_in[0].gl_Position;   // position in clip space 
    vec4 p2 = gl_in[1].gl_Position;

    vec2 dir    = normalize(p2.xy - p1.xy);  // Line direction vector in clip space
    vec2 normal = vec2(-dir.y, dir.x);   // Normal vector in clip space
    vec2 offset = normal * thickness;

    gl_Position = p1 + vec4(offset.xy * p1.w, 0.0, 0.0);   // Scale offset by w to account for perspective
    EmitVertex();
    gl_Position = p1 - vec4(offset.xy * p1.w, 0.0, 0.0);
    EmitVertex();
    gl_Position = p2 + vec4(offset.xy * p2.w, 0.0, 0.0);
    EmitVertex();
    gl_Position = p2 - vec4(offset.xy * p2.w, 0.0, 0.0);
    EmitVertex();

    EndPrimitive();
}