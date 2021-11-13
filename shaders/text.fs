#version 330 core
in vec2 TexCoords;
out vec4 color;

uniform sampler2D text;
uniform vec3 textColor;

void main()
{    
    vec4 sampled = vec4(1.0, 1.0, 1.0, texture(text, TexCoords).r);  // Because the texture's data is stored
    // in just its red component, we sample the r component of the texture as the sampled alpha value.
    //  By varying the output color's alpha value, the resulting pixel will be transparent for all the
    // glyph's background colors and non-transparent for the actual character pixels. 
    color = vec4(textColor, 1.0) * sampled;
}