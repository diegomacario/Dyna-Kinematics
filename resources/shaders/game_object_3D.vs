#version 330 core

layout (location = 0) in vec3 inPos;
layout (location = 1) in vec3 inNormal;
layout (location = 2) in vec2 inTexCoords;

uniform mat4 model;
uniform mat4 projectionView;

out VertexData
{
   vec3 worldPos;
   vec3 worldNormal;
   vec2 texCoords;
} o;

void main()
{
   o.worldPos    = vec3(model * vec4(inPos, 1.0));
   o.worldNormal = normalize(mat3(model) * inNormal);
   o.texCoords   = inTexCoords;

   gl_Position = projectionView * vec4(o.worldPos, 1.0);
}
