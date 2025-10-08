#version 300 es
precision mediump float;

out vec4 FragColor;

in vec2 TexCoords;

uniform sampler2D texture_diffuse1;
uniform bool hasTexture;
uniform vec3 materialColor;

void main()
{    
    if (hasTexture) {
        FragColor = texture(texture_diffuse1, TexCoords);
    } else {
        FragColor = vec4(materialColor, 1.0);
    }
}
