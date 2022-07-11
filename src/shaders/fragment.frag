#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(location = 0) in vec3 fragColor;
layout(location = 1) in vec2 fragTexCoord;
layout(binding = 1) uniform sampler2D textureSampler;

layout(location = 0) out vec4 outColor;

void main() 
{
	vec3 color = texture(textureSampler, fragTexCoord).rgb;
	outColor = vec4(color, 1.0);
}