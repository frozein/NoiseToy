#version 430 core

in vec2 texCoord;
out vec4 outColor;

uniform sampler2D tex2D;
uniform sampler3D tex3D;

uniform int channel;
uniform bool grayscale;

uniform bool is3D;
uniform float slice;

void main()
{
	vec4 texCol;
	if(is3D)
		texCol = texture(tex3D, vec3(texCoord, slice));
	else
		texCol = texture(tex2D, texCoord);
	
	vec4 finalCol;
	switch(channel)
	{
	case 0:
		finalCol = vec4(texCol.r, 0.0, 0.0, 1.0);
		break;
	case 1:
		finalCol = vec4(0.0, texCol.g, 0.0, 1.0);
		break;
	case 2:
		finalCol = vec4(0.0, 0.0, texCol.b, 1.0);
		break;
	case 3:
		finalCol = vec4(vec3(texCol.a), 1.0);
		break;
	default:
		finalCol = texCol;
		break;
	}

	if(grayscale)
		finalCol = vec4(vec3(max(max(finalCol.r, finalCol.g), finalCol.b)), 1.0);

	outColor = finalCol;
} 