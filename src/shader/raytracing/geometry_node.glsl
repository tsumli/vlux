struct GeometryNode {
    uint64_t vertex_buffer_device_address;
    uint64_t index_buffer_device_address;
    int texture_index_base_color;
    int texture_index_normal;
    int texture_index_emissive;
    int texture_index_occlusion_roughness_metallic;
};
layout(set = 2, binding = 0) buffer GeometryNodes { GeometryNode nodes[]; }
geometry_nodes;