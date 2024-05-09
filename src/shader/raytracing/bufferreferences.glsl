layout(push_constant) uniform BufferReferences {
    uint64_t vertices;
    uint64_t indices;
}
bufferReferences;

layout(buffer_reference, scalar) buffer Vertices { vec4 v[]; };
layout(buffer_reference, scalar) buffer Indices { uint i[]; };