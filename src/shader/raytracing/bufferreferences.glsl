#extension GL_EXT_shader_16bit_storage : require
#extension GL_EXT_shader_explicit_arithmetic_types_int64 : require

layout(push_constant) uniform BufferReferences {
    uint64_t vertices;
    uint64_t indices;
}
buffer_references;

layout(buffer_reference, scalar) buffer Vertices { vec4 v[]; };
layout(buffer_reference, scalar) buffer Indices { uint16_t i[]; };