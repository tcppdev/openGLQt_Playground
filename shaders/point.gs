#version 330

layout (points) in;                              
layout (triangle_strip, max_vertices = 50) out; 

uniform float size;

int delta_angle = 20; // Change in angle between circle triangles
uniform bool circle = false;
uniform bool square = false;
uniform bool fixed_size = false;

// Note 1: Maybe batch rendering will be noticeably faster if we have loads 
// of entities (circle, triangle, squares) to draw (eg: debris).
// Difficulty is that we'll then need a different batch index and vertex buffers
// structure for each entity type (circle, triangle, squares) whereas with geometry
// shaders this is easier to implement as shown here

// Note 2: If we're ok with only rendering squares for the points we could also use 
// gl_PointSize = gl_Position.z; in the vertex shader and not have to use a geometry
// shader at all (see https://learnopengl.com/Advanced-OpenGL/Advanced-GLSL)

void main()
{
    vec4 center_pos = gl_in[0].gl_Position;   // position in clip space (obtain from vertex shader)

    if (fixed_size) {  // Set size relative to screen space
        center_pos /= center_pos.w;
    } 

    vec4 previous_pos = vec4(center_pos.x + size, center_pos.yzw);

    if (circle) {
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

            //previous_pos = new_pos; // reset position
        }
    }
    else if (square) {  // square
        gl_Position = vec4(center_pos.x - size/2, center_pos.y - size/2, center_pos.z, center_pos.w);
        EmitVertex();
        gl_Position = vec4(center_pos.x + size/2, center_pos.y - size/2, center_pos.z, center_pos.w);
        EmitVertex();
        gl_Position = vec4(center_pos.x - size/2, center_pos.y + size/2, center_pos.z, center_pos.w);
        EmitVertex();
        gl_Position = vec4(center_pos.x + size/2, center_pos.y + size/2, center_pos.z, center_pos.w);
        EmitVertex();
    }
    else {  // draw a triangle
        gl_Position = vec4(center_pos.x - size/2, center_pos.y - size/2, center_pos.z, center_pos.w);
        EmitVertex();
        gl_Position = vec4(center_pos.x + size/2, center_pos.y - size/2, center_pos.z, center_pos.w);
        EmitVertex();
        gl_Position = vec4(center_pos.x, center_pos.y + size/2, center_pos.z, center_pos.w);
        EmitVertex();
    }

    EndPrimitive();
}