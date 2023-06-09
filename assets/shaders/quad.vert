#version 430 core
layout (location = 0) in vec3 inPos;
layout (location = 1) in vec2 inTexCoord;

out vec2 texCoord;

uniform vec2 scale;

void main()
{
	gl_Position = vec4(inPos.xy * scale, inPos.z, 1.0);
	texCoord = inTexCoord;
}