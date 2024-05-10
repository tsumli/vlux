struct GeometryNode {
    uint64_t vertex_buffer_device_address;
    uint64_t index_buffer_device_address;
    int texture_index_base_color;
    int dummy;
};
layout(set = 2, binding = 0) buffer GeometryNodes { GeometryNode nodes[]; }
geometry_nodes;