#version 450

struct FragInput{
    vec3 color;
    vec2 uv;
};

layout(set = 0, binding = 1) uniform sampler2D color_sampler;
layout(set = 0, binding = 2) uniform sampler2D normal_sampler;

layout(location = 0) in FragInput frag_input;

layout(location = 0) out vec4 out_color;

void main() {
    out_color = texture(color_sampler, frag_input.uv);
}