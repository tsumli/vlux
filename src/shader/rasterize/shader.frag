#version 460

struct FragInput{
    vec4 position_cs;
    vec3 normal_ws;
    vec2 texcoord;
    vec3 tangent_ws ;
    vec3 bitangent_ws;
};

layout(set = 0, binding = 1) uniform sampler2D color_sampler;
layout(set = 0, binding = 2) uniform sampler2D normal_sampler;

layout(location = 0) in FragInput frag_input;

layout(location = 0) out vec4 out_color;
layout(location = 1) out vec4 out_normal;

void main() {
    // Texture Loading
    vec3 normal_ts = texture(normal_sampler, frag_input.texcoord).xyz;
    normal_ts = normalize(normal_ts * 2.0f - 1.0f);

    // TBN matrix
    mat3x3 tangent_frame_ws = mat3x3(
        normalize(frag_input.tangent_ws),
        normalize(frag_input.bitangent_ws),
        normalize(frag_input.normal_ws));
    vec3 normal_ws = tangent_frame_ws * normal_ts;

    out_color = texture(color_sampler, frag_input.texcoord);
    out_normal = vec4(normal_ts, 0.0);
}