#include "gltf.h"

#include "common/image.h"

namespace vlux {

namespace {

// TODO: This function is not working correctly
std::vector<glm::vec4> ComputeTangentFrame(const std::vector<Vertex>& vertices,
                                           const std::vector<Index>& indices) {
    auto tangents = std::vector<glm::vec3>(vertices.size(), glm::vec3(0.0f));
    auto bitangents = std::vector<glm::vec3>(vertices.size(), glm::vec3(0.0f));

    for (size_t i = 0; i < indices.size(); i += 3) {
        const auto idx_0 = indices[i];
        const auto idx_1 = indices[i + 1];
        const auto idx_2 = indices[i + 2];

        const auto& v0 = vertices[idx_0];
        const auto& v1 = vertices[idx_1];
        const auto& v2 = vertices[idx_2];

        const auto edge_1 = v1.pos - v0.pos;
        const auto edge_2 = v2.pos - v0.pos;
        const auto delta_uv_1 = v1.uv - v0.uv;
        const auto delta_uv_2 = v2.uv - v0.uv;

        const auto det = delta_uv_1.x * delta_uv_2.y - delta_uv_2.x * delta_uv_1.y;
        const auto f = 1.0f / det;
        auto tangent = f * (delta_uv_2.y * edge_1 - delta_uv_1.y * edge_2);
        auto bitangent = f * (-delta_uv_2.x * edge_1 + delta_uv_1.x * edge_2);
        tangent = glm::normalize(tangent);
        bitangent = glm::normalize(bitangent);

        tangents[idx_0] = tangent;
        tangents[idx_1] = tangent;
        tangents[idx_2] = tangent;

        bitangents[idx_0] = bitangent;
        bitangents[idx_1] = bitangent;
        bitangents[idx_2] = bitangent;
    }

    auto tangent_handedness = std::vector<glm::vec4>(vertices.size());
    // Average the tangents
    for (size_t i = 0; i < tangents.size(); ++i) {
        // tangents[i] = glm::normalize(tangents[i]);
        // bitangents[i] = glm::normalize(bitangents[i]);

        // Gram-Schmidt orthogonalize
        const auto orthogonal_tangent = glm::normalize(
            tangents[i] - vertices[i].normal * glm::dot(vertices[i].normal, tangents[i]));
        const auto orthogonal_bitangent =
            glm::normalize(glm::cross(vertices[i].normal, tangents[i]));

        tangents[i] = orthogonal_tangent;
        bitangents[i] = orthogonal_bitangent;

        const auto handedness =
            (glm::dot(glm::cross(vertices[i].normal, tangents[i]), bitangents[i]) < 0.0f) ? 1.0f
                                                                                          : -1.0f;

        tangent_handedness[i] = glm::vec4(tangents[i], handedness);
    }

    return tangent_handedness;
}
}  // namespace

GltfObject LoadGltfObjects(const tinygltf::Primitive& primitive, const tinygltf::Model& model,
                           const VkQueue graphics_queue, const VkCommandPool command_pool,
                           const VkPhysicalDevice physical_device, const VkDevice device,
                           const float scale, const glm::vec3& translation,
                           const glm::vec3& rotation) {
    auto indices = std::vector<Index>();
    [&]() {
        {
            const auto& attribute = primitive.indices;
            const auto& accessor = model.accessors[attribute];
            const auto& bufferView = model.bufferViews[accessor.bufferView];
            const auto& buffer = model.buffers[bufferView.buffer];
            const uint16_t* indices_tmp = reinterpret_cast<const uint16_t*>(
                &buffer.data[bufferView.byteOffset + accessor.byteOffset]);
            indices.resize(accessor.count);
            for (size_t i = 0; i < accessor.count; i++) {
                indices[i] = static_cast<uint16_t>(indices_tmp[i]);
            }
        }
    }();

    // Create rotation matrices
    glm::mat4 rot_yaw =
        glm::rotate(glm::mat4(1.0f), glm::radians(rotation.x), glm::vec3(0.0f, 1.0f, 0.0f));
    glm::mat4 rot_pitch =
        glm::rotate(glm::mat4(1.0f), glm::radians(rotation.y), glm::vec3(1.0f, 0.0f, 0.0f));
    glm::mat4 rot_roll =
        glm::rotate(glm::mat4(1.0f), glm::radians(rotation.z), glm::vec3(0.0f, 0.0f, 1.0f));
    const auto rotation_matrix = rot_yaw * rot_pitch * rot_roll;

    // Vertices
    auto vertices_size = 0uz;
    auto vertices = std::vector<Vertex>();
    [&]() {
        auto positions = std::vector<glm::vec3>();
        const auto& attribute = primitive.attributes.at("POSITION");
        const auto& accessor = model.accessors[attribute];
        const auto& bufferView = model.bufferViews[accessor.bufferView];
        const auto& buffer = model.buffers[bufferView.buffer];
        const float* pos = reinterpret_cast<const float*>(
            &buffer.data[bufferView.byteOffset + accessor.byteOffset]);
        vertices_size = accessor.count;
        vertices.resize(vertices_size);

        for (size_t i = 0; i < vertices_size; ++i) {
            vertices[i].pos = {pos[i * 3 + 0], pos[i * 3 + 1], pos[i * 3 + 2]};

            // Rotate
            vertices[i].pos = glm::vec3(rotation_matrix * glm::vec4(vertices[i].pos, 1.0f));

            // scale + translation
            vertices[i].pos = vertices[i].pos * scale + translation;
        }
    }();

    [&]() {
        const auto& attribute = primitive.attributes.at("TEXCOORD_0");
        const auto& accessor = model.accessors[attribute];
        const auto& bufferView = model.bufferViews[accessor.bufferView];
        const auto& buffer = model.buffers[bufferView.buffer];
        const float* uv = reinterpret_cast<const float*>(
            &buffer.data[bufferView.byteOffset + accessor.byteOffset]);
        assert(vertices_size == accessor.count && "count mismatched: `TEXCOORD_0`");
        for (size_t i = 0; i < vertices_size; ++i) {
            vertices[i].uv = {uv[i * 2 + 0], uv[i * 2 + 1]};
        }
    }();

    [&]() {
        const auto& attribute = primitive.attributes.at("NORMAL");
        const auto& accessor = model.accessors[attribute];
        const auto& bufferView = model.bufferViews[accessor.bufferView];
        const auto& buffer = model.buffers[bufferView.buffer];
        const float* normal = reinterpret_cast<const float*>(
            &buffer.data[bufferView.byteOffset + accessor.byteOffset]);
        assert(vertices_size == accessor.count && "count mismatched: `NORMAL`");
        for (size_t i = 0; i < vertices_size; ++i) {
            vertices[i].normal = {normal[i * 3 + 0], normal[i * 3 + 1], normal[i * 3 + 2]};
            vertices[i].normal = glm::vec3(rotation_matrix * glm::vec4(vertices[i].normal, 1.0f));
        }
    }();
    [&]() {
        if (primitive.attributes.contains("TANGENT")) {
            const auto& attribute = primitive.attributes.at("TANGENT");
            const auto& accessor = model.accessors[attribute];
            const auto& bufferView = model.bufferViews[accessor.bufferView];
            const auto& buffer = model.buffers[bufferView.buffer];
            const float* tangent = reinterpret_cast<const float*>(
                &buffer.data[bufferView.byteOffset + accessor.byteOffset]);
            assert(vertices_size == accessor.count && "count mismatched: `TANGENT`");
            for (size_t i = 0; i < vertices_size; ++i) {
                vertices[i].tangent = {tangent[i * 4 + 0], tangent[i * 4 + 1], tangent[i * 4 + 2],
                                       tangent[i * 4 + 3]};
            }

        } else {
            const auto tangents = ComputeTangentFrame(vertices, indices);
            for (size_t i = 0; i < vertices_size; i++) {
                vertices[i].tangent = {tangents[i].x, tangents[i].y, tangents[i].z, tangents[i].w};
            }
        }
    }();

    // Loading material
    const auto material = model.materials[primitive.material];

    // material
    const auto base_color_factor = glm::vec4(material.pbrMetallicRoughness.baseColorFactor[0],
                                             material.pbrMetallicRoughness.baseColorFactor[1],
                                             material.pbrMetallicRoughness.baseColorFactor[2],
                                             material.pbrMetallicRoughness.baseColorFactor[3]);
    const auto metallic_factor = static_cast<float>(material.pbrMetallicRoughness.metallicFactor);
    const auto roughness_factor = static_cast<float>(material.pbrMetallicRoughness.roughnessFactor);

    // texture
    const auto get_image_idx = [&](const auto texture_idx) {
        const auto& texture = model.textures[texture_idx];
        return texture.source;
    };

    const auto create_image = [&](const auto image_idx) {
        const auto& image = model.images[image_idx];
        return Image(image.image, image.width, image.height, image.component);
    };

    const auto create_texture = [&](const auto& image) {
        return std::make_shared<Texture<uint8_t>>(image, graphics_queue, command_pool, device,
                                                  physical_device);
    };

    auto base_color_texture = [&]() -> std::shared_ptr<Texture<uint8_t>> {
        const auto idx = material.pbrMetallicRoughness.baseColorTexture.index;
        if (idx == -1) {
            return nullptr;
        }
        const auto image = create_image(get_image_idx(idx));
        return create_texture(image);
    }();

    auto normal_texture = [&]() -> std::shared_ptr<Texture<uint8_t>> {
        const auto idx = material.normalTexture.index;
        if (idx == -1) {
            return nullptr;
        }
        auto image = create_image(get_image_idx(idx));
        return create_texture(image);
    }();

    auto occlusion_texture = [&]() -> std::shared_ptr<Texture<uint8_t>> {
        const auto idx = material.occlusionTexture.index;
        if (idx == -1) {
            return nullptr;
        }
        auto image = create_image(get_image_idx(idx));
        return create_texture(image);
    }();

    auto emmisive_texture = [&]() -> std::shared_ptr<Texture<uint8_t>> {
        const auto idx = material.emissiveTexture.index;
        if (idx == -1) {
            return nullptr;
        }
        auto image = create_image(get_image_idx(idx));
        return create_texture(image);
    }();

    auto metallic_roughness_texture = [&]() -> std::shared_ptr<Texture<uint8_t>> {
        const auto idx = material.pbrMetallicRoughness.metallicRoughnessTexture.index;
        if (idx == -1) {
            return nullptr;
        }
        auto image = create_image(get_image_idx(idx));
        return create_texture(image);
    }();

    return {
        .indices = indices,
        .vertices = vertices,
        .base_color_factor = base_color_factor,
        .metallic_factor = metallic_factor,
        .roughness_factor = roughness_factor,
        .base_color_texture = base_color_texture,
        .normal_texture = normal_texture,
        .occlusion_texture = occlusion_texture,
        .emissive_texture = emmisive_texture,
        .metallic_roughness_texture = metallic_roughness_texture,
    };
}

tinygltf::Model LoadTinyGltfModel(const std::filesystem::path& path) {
    if (!std::filesystem::exists(path)) {
        throw std::runtime_error(fmt::format("file not found: {}", path.string()));
    }
    tinygltf::Model model;
    std::string err;
    std::string warn;

    static tinygltf::TinyGLTF gltf;

    if (path.extension().string() == ".glb") {
        if (!gltf.LoadBinaryFromFile(&model, &err, &warn, path.string())) {
            spdlog::debug("Err: {}", err);
            spdlog::debug("Warn: {}", warn);
            throw std::runtime_error(fmt::format("failed to load: {}", path.string()));
        }
    } else if (path.extension().string() == ".gltf") {
        if (!gltf.LoadASCIIFromFile(&model, &err, &warn, path.string())) {
            spdlog::debug("Err: {}", err);
            spdlog::debug("Warn: {}", warn);
            throw std::runtime_error(fmt::format("failed to load: {}", path.string()));
        }
    } else {
        throw std::runtime_error(fmt::format("unsupported file format: {}", path.string()));
    }
    return model;
}
}  // namespace vlux