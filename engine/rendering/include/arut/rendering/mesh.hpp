#pragma once

#include <array>
#include <cmath>
#include <cstdint>
#include <vector>

namespace arut::rendering {

struct MeshVertex final {
    std::array<float, 3> position{};
    std::array<float, 3> normal{0.0F, 0.0F, 1.0F};
    std::array<float, 4> tangent{1.0F, 0.0F, 0.0F, 1.0F};
    std::array<float, 2> uv{};
};

struct MeshData final {
    std::vector<MeshVertex> vertices;
    std::vector<std::uint32_t> indices;
};

struct Mat4 final {
    // Column-major, matching GLSL's default matrix layout.
    std::array<float, 16> value{};

    [[nodiscard]] static constexpr Mat4 identity() noexcept {
        Mat4 result{};
        result.value[0] = result.value[5] = result.value[10] = result.value[15] = 1.0F;
        return result;
    }
};

[[nodiscard]] inline MeshData make_unit_cube() {
    MeshData mesh{};
    mesh.vertices.reserve(24);
    mesh.indices.reserve(36);

    struct Face final {
        std::array<float, 3> normal;
        std::array<float, 4> tangent;
        std::array<std::array<float, 3>, 4> corners;
    };
    constexpr std::array<Face, 6> faces{{
        {{{ 0, 0, 1}}, {{1, 0, 0, 1}}, {{{-0.5F,-0.5F, 0.5F},{0.5F,-0.5F, 0.5F},{0.5F,0.5F,0.5F},{-0.5F,0.5F,0.5F}}}},
        {{{ 0, 0,-1}}, {{-1,0,0,1}}, {{{0.5F,-0.5F,-0.5F},{-0.5F,-0.5F,-0.5F},{-0.5F,0.5F,-0.5F},{0.5F,0.5F,-0.5F}}}},
        {{{ 1, 0, 0}}, {{0,0,-1,1}}, {{{0.5F,-0.5F,0.5F},{0.5F,-0.5F,-0.5F},{0.5F,0.5F,-0.5F},{0.5F,0.5F,0.5F}}}},
        {{{-1, 0, 0}}, {{0,0,1,1}}, {{{-0.5F,-0.5F,-0.5F},{-0.5F,-0.5F,0.5F},{-0.5F,0.5F,0.5F},{-0.5F,0.5F,-0.5F}}}},
        {{{ 0, 1, 0}}, {{1,0,0,1}}, {{{-0.5F,0.5F,0.5F},{0.5F,0.5F,0.5F},{0.5F,0.5F,-0.5F},{-0.5F,0.5F,-0.5F}}}},
        {{{ 0,-1, 0}}, {{1,0,0,1}}, {{{-0.5F,-0.5F,-0.5F},{0.5F,-0.5F,-0.5F},{0.5F,-0.5F,0.5F},{-0.5F,-0.5F,0.5F}}}}
    }};
    constexpr std::array<std::array<float, 2>, 4> uvs{{
        {{0.0F, 0.0F}}, {{1.0F, 0.0F}}, {{1.0F, 1.0F}}, {{0.0F, 1.0F}}
    }};

    for (const Face& face : faces) {
        const auto base = static_cast<std::uint32_t>(mesh.vertices.size());
        for (std::size_t i = 0; i < 4; ++i) {
            mesh.vertices.push_back({face.corners[i], face.normal, face.tangent, uvs[i]});
        }
        mesh.indices.insert(mesh.indices.end(), {
            base, base + 1, base + 2, base + 2, base + 3, base
        });
    }
    return mesh;
}

[[nodiscard]] inline Mat4 perspective_rh_zo(float vertical_fov_radians, float aspect,
                                             float near_plane, float far_plane) noexcept {
    Mat4 result{};
    if (!(aspect > 0.0F) || !(near_plane > 0.0F) || !(far_plane > near_plane) ||
        !(vertical_fov_radians > 0.0F && vertical_fov_radians < 3.14159265F)) return result;
    const float focal = 1.0F / std::tan(vertical_fov_radians * 0.5F);
    result.value[0] = focal / aspect;
    result.value[5] = -focal; // Vulkan framebuffer Y points down.
    result.value[10] = far_plane / (near_plane - far_plane);
    result.value[11] = -1.0F;
    result.value[14] = (far_plane * near_plane) / (near_plane - far_plane);
    return result;
}

} // namespace arut::rendering
