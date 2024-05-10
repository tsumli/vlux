#version 460
#extension GL_EXT_ray_tracing : enable
#extension GL_GOOGLE_include_directive : require
#extension GL_EXT_nonuniform_qualifier : require
#extension GL_EXT_buffer_reference2 : require
#extension GL_EXT_scalar_block_layout : require
#extension GL_EXT_shader_explicit_arithmetic_types_int64 : require

layout(location = 0) rayPayloadInEXT vec3 hit_value;
hitAttributeEXT vec2 attribs;

struct LightParams {
    vec4 pos;
    float range;
    vec4 color;
};
layout(set = 0, binding = 2) uniform ubo_light { LightParams light; };

struct TransformParams {
    mat4x4 world;
    mat4x4 view_proj;
    mat4x4 world_view_proj;
    mat4x4 proj_to_world;
};
layout(set = 0, binding = 3) uniform ubo_transform { TransformParams transform; };

layout(set = 1, binding = 1) uniform sampler2D base_colors[];
layout(set = 1, binding = 2) uniform sampler2D normals[];

#include "bufferreferences.glsl"
#include "geometry_node.glsl"
#include "geometrytypes.glsl"
#include "mode_push_constant.glsl"

const float PI = 3.14159265359f;
vec3 CookTorranceBRDF(in const vec3 N, in const vec3 V, in const vec3 L, in const vec3 albedo,
                      in const float roughness, in const float metalness) {
    vec3 H = normalize(V + L);
    float NdotL = max(dot(N, L), 0.0);
    float NdotV = max(dot(N, V), 0.0);
    float NdotH = max(dot(N, H), 0.0);
    float VdotH = max(dot(V, H), 0.0);

    // Fresnel term (Schlick approximation)
    float F0 = mix(0.04, 1.0, metalness);
    float F = F0 + (1.0 - F0) * pow(1.0 - VdotH, 5.0);

    // Distribution term (GGX/Trowbridge-Reitz)
    float alpha = roughness * roughness;
    float alpha2 = alpha * alpha;
    float denom = NdotH * NdotH * (alpha2 - 1.0) + 1.0;
    float D = alpha2 / (PI * denom * denom);

    // Geometric term (Smith's method)
    float k = alpha / 2.0;
    float G = NdotL / (NdotL * (1.0 - k) + k);
    G *= NdotV / (NdotV * (1.0 - k) + k);

    // Specular term
    vec3 Fc = vec3(F);
    vec3 Fs = D * Fc * G / (4.0 * NdotL * NdotV);

    // Combine specular and diffuse
    vec3 diffuse = (1.0 - Fc) * albedo / PI;
    return NdotL * (diffuse + Fs);
}

void main() {
    Triangle tri = UnpackTriangle(gl_PrimitiveID);
    GeometryNode geometry_node = geometry_nodes.nodes[gl_InstanceID];

    const vec3 base_color =
        texture(base_colors[nonuniformEXT(geometry_node.texture_index_base_color)], tri.uv).rgb;
    vec3 normal_ts =
        texture(normals[nonuniformEXT(geometry_node.texture_index_normal)], tri.uv).rgb;
    normal_ts = normalize(normal_ts * 2.0f - 1.0f);

    const vec3 normal_ws = normalize(mat3x3(transform.world) * tri.normal);
    const vec3 tangent_ws = normalize(mat3x3(transform.world) * normalize(tri.tangent.xyz));
    const vec3 bitangent_ws = normalize(cross(normal_ws, tangent_ws)) * tri.tangent.w;
    const mat3x3 tbn = mat3x3(tangent_ws, bitangent_ws, normal_ws);
    const vec3 normal = tbn * normal_ts;

    const float roughness = 0.3;
    const float metallic = 0.1;

    const float dist = length(light.pos.xyz - tri.pos.xyz);
    const float attenuation = 3.0 / (1.0 + 0.07 * dist + 0.017 * dist * dist) * light.range;

    // Calculate final color.
    const vec3 cook_torrance_brdf =
        CookTorranceBRDF(normal.xyz, normalize(gl_WorldRayOriginEXT - tri.pos.xyz),
                         normalize(light.pos.xyz - tri.pos.xyz), base_color.xyz, roughness,
                         metallic) *
        light.color.xyz * attenuation;

    const vec3 emissive = vec3(0.0, 0.0, 0.0);
    const vec3 final_color = clamp(cook_torrance_brdf, 0.0, 1.0) + emissive;

    switch (mode.mode) {
        case 0: {
            hit_value = final_color;
            break;
        }
        case 1: {
            // disable subpixel jittering
            hit_value = final_color;
            break;
        }
        case 2: {
            hit_value.xy = tri.uv;
            break;
        }
        case 3: {
            hit_value = normal;
            break;
        }
        case 4: {
            hit_value = tri.normal;
            break;
        }
        case 5: {
            hit_value = tri.pos;
            break;
        }
        case 6: {
            hit_value = base_color;
            break;
        }
        default: {
            hit_value = vec3(0.0, 0.0, 0.0);
        }
    }

    // // Shadow casting
    // float tmin = 0.001;
    // float tmax = 10000.0;
    // float epsilon = 0.001;
    // vec3 origin =
    //     gl_WorldRayOriginEXT + gl_WorldRayDirectionEXT * gl_HitTEXT + tri.normal * epsilon;
    // shadowed = true;
    // vec3 lightVector = vec3(-5.0, -2.5, -5.0);
    // // Trace shadow ray and offset indices to match shadow hit/miss shader group indices
    // //	traceRayEXT(topLevelAS, gl_RayFlagsTerminateOnFirstHitEXT | gl_RayFlagsOpaqueEXT |
    // //gl_RayFlagsSkipClosestHitShaderEXT, 0xFF, 0, 0, 1, origin, tmin, lightVector, tmax, 2);
    // if
    // //(shadowed) { 		hitValue *= 0.7;
    // //	}
}