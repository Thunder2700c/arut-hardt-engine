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

[[nodiscard]] inline std::array<float, 3> cross3(const std::array<float, 3>& a,
                                                  const std::array<float, 3>& b) noexcept {
    return {a[1] * b[2] - a[2] * b[1],
            a[2] * b[0] - a[0] * b[2],
            a[0] * b[1] - a[1] * b[0]};
}

[[nodiscard]] inline float dot3(const std::array<float, 3>& a,
                                const std::array<float, 3>& b) noexcept {
    return a[0] * b[0] + a[1] * b[1] + a[2] * b[2];
}

[[nodiscard]] inline std::array<float, 3> normalize3(const std::array<float, 3>& v) noexcept {
    const float length_squared = dot3(v, v);
    if (!std::isfinite(length_squared) || length_squared <= 1.0e-12F) return {0.0F, 0.0F, -1.0F};
    const float inverse_length = 1.0F / std::sqrt(length_squared);
    return {v[0] * inverse_length, v[1] * inverse_length, v[2] * inverse_length};
}

[[nodiscard]] inline Mat4 look_at_rh(const std::array<float, 3>& eye,
                                     const std::array<float, 3>& center,
                                     const std::array<float, 3>& up) noexcept {
    const std::array<float, 3> forward = normalize3({
        center[0] - eye[0], center[1] - eye[1], center[2] - eye[2]
    });
    const std::array<float, 3> side = normalize3(cross3(forward, up));
    const std::array<float, 3> corrected_up = cross3(side, forward);
    Mat4 result = Mat4::identity();
    result.value[0] = side[0];
    result.value[4] = side[1];
    result.value[8] = side[2];
    result.value[12] = -dot3(side, eye);
    result.value[1] = corrected_up[0];
    result.value[5] = corrected_up[1];
    result.value[9] = corrected_up[2];
    result.value[13] = -dot3(corrected_up, eye);
    result.value[2] = -forward[0];
    result.value[6] = -forward[1];
    result.value[10] = -forward[2];
    result.value[14] = dot3(forward, eye);
    return result;
}

[[nodiscard]] inline Mat4 multiply(const Mat4& a, const Mat4& b) noexcept {
    Mat4 result{};
    for (std::size_t column = 0; column < 4; ++column) {
        for (std::size_t row = 0; row < 4; ++row) {
            for (std::size_t k = 0; k < 4; ++k) {
                result.value[column * 4 + row] +=
                    a.value[k * 4 + row] * b.value[column * 4 + k];
            }
        }
    }
    return result;
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
