#version 450

struct TransformParams {
    mat4x4 world;
    mat4x4 view_proj;
    mat4x4 world_view_proj;
    mat4x4 proj_to_world;
};

layout(set = 0, binding = 0) uniform ubo {
    TransformParams transform;
};

struct FragInput{
    vec3 color;
    vec2 uv;
};

layout(location = 0) in vec3 in_position;
layout(location = 1) in vec3 in_normal;
layout(location = 2) in vec2 in_uv;
layout(location = 3) in vec4 in_tangent;

layout(location = 0) out FragInput frag_input;

void main() {
    gl_Position = transform.world_view_proj * vec4(in_position, 1.0);
    frag_input.color = in_normal;
    frag_input.uv = in_uv;
}