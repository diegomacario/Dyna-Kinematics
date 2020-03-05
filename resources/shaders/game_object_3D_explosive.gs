#version 330 core

layout (triangles) in;
layout (triangle_strip, max_vertices = 3) out;

in VertexData
{
   vec3 worldPos;
   vec3 worldNormal;
   vec2 texCoords;
} i[];

out VertexData
{
   vec3 worldPos;
   vec3 worldNormal;
   vec2 texCoords;
} o;

uniform float distanceToMove;

vec3 calculateNormalOfTriangle();
vec4 explode(vec4 position, vec3 normal);

void main()
{
   vec3 normal = calculateNormalOfTriangle();

   gl_Position   = explode(gl_in[0].gl_Position, normal);
   o.worldPos    = i[0].worldPos;
   o.worldNormal = i[0].worldNormal;
   o.texCoords   = i[0].texCoords;
   EmitVertex();

   gl_Position = explode(gl_in[1].gl_Position, normal);
   o.worldPos    = i[1].worldPos;
   o.worldNormal = i[1].worldNormal;
   o.texCoords   = i[1].texCoords;
   EmitVertex();

   gl_Position = explode(gl_in[2].gl_Position, normal);
   o.worldPos    = i[2].worldPos;
   o.worldNormal = i[2].worldNormal;
   o.texCoords   = i[2].texCoords;
   EmitVertex();

   EndPrimitive();
}

vec3 calculateNormalOfTriangle()
{
   vec3 a = vec3(gl_in[0].gl_Position) - vec3(gl_in[1].gl_Position);
   vec3 b = vec3(gl_in[2].gl_Position) - vec3(gl_in[1].gl_Position);
   return normalize(cross(a, b));
}

vec4 explode(vec4 position, vec3 normal)
{
   vec3 direction = normal * distanceToMove;
   return (position + vec4(direction, 0.0));
}
