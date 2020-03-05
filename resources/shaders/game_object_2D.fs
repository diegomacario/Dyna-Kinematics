#version 330 core

in VertexData
{
   vec2 texCoords;
} i;

uniform sampler2D image;

out vec4 fragColor;

void main()
{
   fragColor = texture(image, i.texCoords);
}
