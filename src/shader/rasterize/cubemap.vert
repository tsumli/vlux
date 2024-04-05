#version 450

layout (location = 0) in vec3 in_pos;

layout (binding = 0) uniform UBO
{
	mat4 proj;
	mat4 view;
} ubo;

layout (location = 0) out vec3 out_uvw;

void main() 
{
	out_uvw = in_pos;
	// Convert cubemap coordinates into Vulkan coordinate space
	out_uvw.xy *= -1.0;
	// Remove translation from view matrix
	mat4 view_mat = mat4(mat3(ubo.view));
	gl_Position = ubo.proj * view_mat * vec4(in_pos.xyz, 1.0);
}