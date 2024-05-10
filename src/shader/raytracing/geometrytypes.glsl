struct Vertex {
    vec3 pos;
    vec3 normal;
    vec2 uv;
    vec4 tangent;
};

struct Triangle {
    Vertex vertices[3];
    vec3 normal;
    vec2 uv;
    vec4 tangent;
};

// This function will unpack our vertex buffer data into a single triangle and calculates uv
// coordinates
Triangle UnpackTriangle(const uint primitive_index) {
    Triangle tri;
    GeometryNode geometry_node = geometry_nodes.nodes[gl_InstanceID];
    Indices indices = Indices(geometry_node.index_buffer_device_address);
    Vertices vertices = Vertices(geometry_node.vertex_buffer_device_address);

    // Unpack vertices
    // Data is packed as vec4 so we can map to the glTF vertex structure from the host side
    const uint tri_index = primitive_index * 3;
    for (uint i = 0; i < 3; i++) {
        const uint vertex_offset =
            uint(indices.i[tri_index + i]) * 3;         // 12 bytes = 3 vec4 per Vertex
        const vec4 d0 = vertices.v[vertex_offset + 0];  // pos.xyz, normal.x
        const vec4 d1 = vertices.v[vertex_offset + 1];  // normal.yz, uv.xy
        const vec4 d2 = vertices.v[vertex_offset + 2];  // tangent.xyzw
        tri.vertices[i].pos = d0.xyz;
        tri.vertices[i].normal = vec3(d0.w, d1.xy);
        tri.vertices[i].uv = d1.zw;
        tri.vertices[i].tangent = d2;
    }
    // Calculate values at barycentric coordinates
    vec3 barycentric_coords = vec3(1.0f - attribs.x - attribs.y, attribs.x, attribs.y);
    tri.normal = tri.vertices[0].normal * barycentric_coords.x +
                 tri.vertices[1].normal * barycentric_coords.y +
                 tri.vertices[2].normal * barycentric_coords.z;
    tri.uv = tri.vertices[0].uv * barycentric_coords.x + tri.vertices[1].uv * barycentric_coords.y +
             tri.vertices[2].uv * barycentric_coords.z;
    tri.tangent = tri.vertices[0].tangent * barycentric_coords.x +
                  tri.vertices[1].tangent * barycentric_coords.y +
                  tri.vertices[2].tangent * barycentric_coords.z;
    return tri;
}