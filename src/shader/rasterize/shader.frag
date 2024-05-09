#version 460

struct FragInput {
    vec4 position_cs;
    vec4 position_ws;
    vec3 normal_ws;
    vec2 texcoord;
    vec3 tangent_ws;
    vec3 bitangent_ws;
};

struct MaterialParams {
    vec4 base_color;
    vec4 metallic_roughness;
};

layout(set = 0, binding = 1) uniform sampler2D color_sampler;
layout(set = 0, binding = 2) uniform sampler2D normal_sampler;
layout(set = 0, binding = 3) uniform sampler2D emissive_sampler;
layout(set = 0, binding = 4) uniform sampler2D metallic_roughness_sampler;

layout(set = 1, binding = 0) uniform ubo_material { MaterialParams material; };

layout(location = 0) in FragInput frag_input;

layout(location = 0) out vec4 out_color;
layout(location = 1) out vec4 out_normal;
layout(location = 2) out vec4 out_position;
layout(location = 3) out vec4 out_emissive;
layout(location = 4) out vec4 out_base_color_factor;
layout(location = 5) out vec4 out_metallic_roughness_factor;
layout(location = 6) out vec4 out_metallic_roughness;

void main() {
    const vec2 texcoord = frag_input.texcoord;
    // Texture Loading
    vec3 normal_ts = texture(normal_sampler, texcoord).xyz;
    // invert y
    normal_ts = normalize(normal_ts * 2.0f - 1.0f);

    // TBN matrix
    mat3x3 tangent_frame_ws =
        mat3x3(normalize(frag_input.tangent_ws), normalize(frag_input.bitangent_ws),
               normalize(frag_input.normal_ws));
    vec3 normal_ws = tangent_frame_ws * normal_ts;

    out_color = texture(color_sampler, texcoord);
    out_normal = vec4(normal_ws, 0.0);
    out_position = frag_input.position_ws;
    out_emissive = texture(emissive_sampler, texcoord);
    out_base_color_factor = material.base_color;
    out_metallic_roughness_factor = material.metallic_roughness;
    out_metallic_roughness = texture(metallic_roughness_sampler, texcoord);
}