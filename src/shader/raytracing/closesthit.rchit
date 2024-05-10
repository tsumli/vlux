#version 460
#extension GL_EXT_ray_tracing : enable
#extension GL_GOOGLE_include_directive : require
#extension GL_EXT_nonuniform_qualifier : require
#extension GL_EXT_buffer_reference2 : require
#extension GL_EXT_scalar_block_layout : require
#extension GL_EXT_shader_explicit_arithmetic_types_int64 : require

layout(location = 0) rayPayloadInEXT vec3 hit_value;
hitAttributeEXT vec2 attribs;

layout(set = 1, binding = 1) uniform sampler2D base_colors[];

struct ModePushConstants {
    uint mode;
};
layout(push_constant) uniform push_mode { ModePushConstants mode; };

#include "bufferreferences.glsl"
#include "geometry_node.glsl"
#include "geometrytypes.glsl"

void main() {
    // 16-bit
    Triangle tri = UnpackTriangle(gl_PrimitiveID);
    GeometryNode geometry_node = geometry_nodes.nodes[gl_InstanceID];
    switch (mode.mode) {
        case 0: {
            const vec3 base_color =
                texture(base_colors[nonuniformEXT(geometry_node.texture_index_base_color)], tri.uv)
                    .rgb;
            hit_value = base_color;
            break;
        }
        case 1: {
            hit_value = tri.normal;
            break;
        }
        case 2: {
            hit_value.xy = tri.uv;
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