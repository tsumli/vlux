#version 460

struct FragInput{
    vec3 color;
    vec2 uv;
};

layout(set = 0, binding = 1) uniform sampler2D color_sampler;
layout(set = 0, binding = 2) uniform sampler2D normal_sampler;

layout(location = 0) in FragInput frag_input;

layout(location = 0) out vec4 out_color;
layout(location = 1) out vec4 out_normal;

void main() {
    out_color = texture(color_sampler, frag_input.uv);
    out_normal = texture(normal_sampler, frag_input.uv);
}