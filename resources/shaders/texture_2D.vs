#version 330 core

layout (location = 0) in vec4 inPosAndTexCoords; // The first two elements correspond to the position, while the second two elements correspond to the texture coordinates

uniform mat4 model;
uniform mat4 projection;

out VertexData
{
   vec2 texCoords;
} o;

void main()
{
   o.texCoords = inPosAndTexCoords.zw;
   gl_Position = projection * model * vec4(inPosAndTexCoords.xy, 0.0, 1.0);
}
