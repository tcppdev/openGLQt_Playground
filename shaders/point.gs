#version 330

layout (points) in;                              
layout (triangle_strip, max_vertices = 50) out; 

uniform float size;

int delta_angle = 20; // Change in angle between triangles

void main()
{
    vec4 center_pos = gl_in[0].gl_Position;   // position in clip space (obtain from vertex shader)

    vec4 previous_pos = vec4(center_pos.x + size, center_pos.yzw);

    gl_Position = previous_pos;
    EmitVertex();

    for (int i = delta_angle; i <= 360; i+=delta_angle) 
    {   // draw circle
        gl_Position = center_pos;  // bit of heck but works without needing to implement a batching method
        EmitVertex();

        float x = center_pos.x + size*cos(radians(i));
        float y = center_pos.y + size*sin(radians(i));
        vec4 new_pos = vec4(x, y, center_pos.z, center_pos.w);
        gl_Position = new_pos;
        EmitVertex();

        previous_pos = new_pos; // reset position
        
    }

    EndPrimitive();
}