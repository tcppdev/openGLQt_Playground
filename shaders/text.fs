#version 300 es
precision mediump float;

in vec2 TexCoords;
out vec4 color;

uniform sampler2D text;
uniform vec3 textColor;

void main()
{    
    // Texture data is stored in RGBA format with alpha channel containing glyph data
    vec4 sampled = texture(text, TexCoords);
    color = vec4(textColor, 1.0) * vec4(1.0, 1.0, 1.0, sampled.a);
}
