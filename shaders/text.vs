#version 330 core
layout (location = 0) in vec4 vertex; // <vec2 pos, vec2 tex>
out vec2 TexCoords;

uniform mat4 view;
uniform float distance_from_camera;
uniform mat4 projection;

void main()
{
    vec4 position_prespective = projection * vec4(vertex.xy, distance_from_camera, 1.0);
    gl_Position = position_prespective;
    // gl_Position = vec4(position_prespective.xyz, 1.0)/position_prespective.w;
    TexCoords = vertex.zw;
}