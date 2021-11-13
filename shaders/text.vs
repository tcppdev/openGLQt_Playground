#version 330 core
layout (location = 0) in vec3 raw_vertex_pos; // <vec2 pos, vec2 tex>
layout (location = 1) in vec2 texcoords; //
out vec2 TexCoords;

uniform mat4 view;
uniform mat4 projection;

// Values that stay constant for the whole mesh.
uniform vec3 camera_right_worldspace;
uniform vec3 camera_up_worldspace;
uniform vec3 text_position; // Position of the center of the billboard
uniform bool fixed_size = false;

void main()
{
    // http://www.opengl-tutorial.org/intermediate-tutorials/billboards-particles/billboards/
    // https://github.com/andersonfreitas/opengl-tutorial-org/tree/master/tutorial18_billboards_and_particles

    vec3 particle_center_wordspace = text_position;
	vec3 vertex_position_worldspace = (text_position +
         camera_right_worldspace * raw_vertex_pos.x + camera_up_worldspace * raw_vertex_pos.y);

	// Output position of the vertex
	gl_Position = projection * view * vec4(vertex_position_worldspace, 1.0f);

	if (fixed_size) {   // make text fixed size wrt to zoom
        vertex_position_worldspace = particle_center_wordspace;
        gl_Position = projection * view * vec4(vertex_position_worldspace, 1.0f); // Get the screen-space position of the particle's center
        gl_Position /= gl_Position.w; // Here we have to do the perspective division ourselves.
        gl_Position.xy += raw_vertex_pos.xy; // Move the vertex in directly screen space. No need for CameraUp/Right_worlspace here.
    }

    TexCoords = texcoords.xy; //vertex.zw;
}