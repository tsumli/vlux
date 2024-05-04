#version 460

struct TransformParams {
    mat4x4 world;
    mat4x4 view_proj;
    mat4x4 world_view_proj;
    mat4x4 proj_to_world;
};

layout(set = 0, binding = 0) uniform ubo { TransformParams transform; };

struct FragInput {
    vec4 position_cs;
    vec4 position_ws;
    vec3 normal_ws;
    vec2 texcoord;
    vec3 tangent_ws;
    vec3 bitangent_ws;
};

layout(location = 0) in vec3 in_position;
layout(location = 1) in vec3 in_normal;
layout(location = 2) in vec2 in_uv;
layout(location = 3) in vec4 in_tangent;

layout(location = 0) out FragInput frag_input;

void main() {
    // Convert normal
    frag_input.normal_ws = normalize(mat3x3(transform.world) * in_normal);

    // Reconstruct the rest of the tangent frame
    frag_input.tangent_ws = normalize(mat3x3(transform.world) * in_tangent.xyz);
    frag_input.bitangent_ws =
        normalize(cross(frag_input.normal_ws, frag_input.tangent_ws)) * in_tangent.w;

    // Calculate the clip-space position
    frag_input.position_cs = transform.world_view_proj * vec4(in_position, 1.0);
    frag_input.position_ws = transform.world * vec4(in_position, 1.0);

    // Pass through the rest of the data
    frag_input.texcoord = in_uv;

    // Output the clip-space position
    gl_Position = frag_input.position_cs;
}