#include "gltf.h"

#include "common/image.h"

namespace vlux {

namespace {

bool ComputeTangentFrame(std::vector<Vertex>& vertices, const std::vector<Index>& indices) {
    if (vertices.size() % 3 != 0) {
        return false;
    }
    for (size_t i = 0; i < vertices.size(); i += 3) {
        auto& v0 = vertices[indices[i]];
        auto& v1 = vertices[indices[i + 1]];
        auto& v2 = vertices[indices[i + 2]];

        auto edge_1 = v1.pos - v0.pos;
        auto edge_2 = v2.pos - v0.pos;
        auto delta_uv_1 = v1.uv - v0.uv;
        auto delta_uv_2 = v2.uv - v0.uv;

        float f = 1.0f / (delta_uv_1.x * delta_uv_2.y - delta_uv_2.x * delta_uv_1.y);

        glm::vec3 tangent;
        tangent.x = f * (delta_uv_2.y * edge_1.x - delta_uv_1.y * edge_2.x);
        tangent.y = f * (delta_uv_2.y * edge_1.y - delta_uv_1.y * edge_2.y);
        tangent.z = f * (delta_uv_2.y * edge_1.z - delta_uv_1.y * edge_2.z);

        glm::vec3 bitangent;
        bitangent.x = f * (-delta_uv_2.x * edge_1.x + delta_uv_1.x * edge_2.x);
        bitangent.y = f * (-delta_uv_2.x * edge_1.y + delta_uv_1.x * edge_2.y);
        bitangent.z = f * (-delta_uv_2.x * edge_1.z + delta_uv_1.x * edge_2.z);

        // Normalize the tangent
        tangent = glm::normalize(tangent);

        // Compute the handedness (whether the bitangent needs to be flipped)
        float handedness =
            (glm::dot(glm::cross(v0.normal, tangent), bitangent) < 0.0f) ? -1.0f : 1.0f;

        // Store the tangent and its handedness in the vertex
        v0.tangent = glm::vec4(tangent, handedness);
        // Repeat storing the tangent and handedness for v1 and v2 if averaging is not applied
    }
    return true;
}
}  // namespace

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

GltfObject LoadGltfObjects(const tinygltf::Primitive& primitive, const tinygltf::Model& model,
                           const VkQueue graphics_queue, const VkCommandPool command_pool,
                           const VkPhysicalDevice physical_device, const VkDevice device,
                           const float scale, const glm::vec3 translation) {
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
        std::random_device rd{};
        std::mt19937 gen(rd());
        std::uniform_real_distribution<float> dis(0.0, 1.0);
        for (size_t i = 0; i < vertices_size; ++i) {
            vertices[i].pos = {pos[i * 3 + 0] * scale + translation.x,
                               pos[i * 3 + 1] * scale + translation.y,
                               pos[i * 3 + 2] * scale + translation.z};
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
            auto positions = std::vector<glm::vec3>(vertices_size);
            auto normals = std::vector<glm::vec3>(vertices_size);
            auto uvs = std::vector<glm::vec2>(vertices_size);
            for (size_t i = 0; i < vertices_size; i++) {
                positions[i] = {vertices[i].pos.x, vertices[i].pos.y, vertices[i].pos.z};
                normals[i] = vertices[i].normal;
                uvs[i] = vertices[i].uv;
            }

            auto tangents = std::vector<glm::vec4>(vertices_size);
            if (!ComputeTangentFrame(vertices, indices)) {
                spdlog::warn("failed to compute tangent frame");
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
        return std::make_shared<Texture>(image, graphics_queue, command_pool, device,
                                         physical_device);
    };

    auto base_color_texture = [&]() -> std::shared_ptr<Texture> {
        const auto idx = material.pbrMetallicRoughness.baseColorTexture.index;
        if (idx == -1) {
            spdlog::debug("base color texture not found");
            return nullptr;
        }
        const auto image = create_image(get_image_idx(idx));
        return create_texture(image);
    }();

    auto normal_texture = [&]() -> std::shared_ptr<Texture> {
        const auto idx = material.normalTexture.index;
        if (idx == -1) {
            spdlog::debug("normal texture not found");
            return nullptr;
        }
        auto image = create_image(get_image_idx(idx));
        return create_texture(image);
    }();

    auto occlusion_texture = [&]() -> std::shared_ptr<Texture> {
        const auto idx = material.occlusionTexture.index;
        if (idx == -1) {
            spdlog::debug("occlusion texture not found");
            return nullptr;
        }
        auto image = create_image(get_image_idx(idx));
        return create_texture(image);
    }();

    auto emmisive_texture = [&]() -> std::shared_ptr<Texture> {
        const auto idx = material.emissiveTexture.index;
        if (idx == -1) {
            spdlog::debug("emmisive texture not found");
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
    };
}

}  // namespace vlux