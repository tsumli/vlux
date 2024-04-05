#version 450

layout (binding = 1) uniform samplerCube sampler_cubemap;

layout (location = 0) in vec3 in_uvw;

layout (location = 0) out vec4 out_color;

void main() 
{
	out_color = texture(sampler_cubemap, in_uvw);
}