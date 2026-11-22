#include "arut/core/world_systems.hpp"

#include <cassert>
#include <cmath>
#include <cstdint>
#include <stdexcept>

int main() {
    using namespace arut::core;

    const auto terrain = TerrainSystem::build(256.0F, 32U);
    assert(terrain.valid());
    assert(terrain.vertices.size() == 33U * 33U);
    assert(terrain.indices.size() == 32U * 32U * 6U);
    for (const auto& vertex : terrain.vertices) {
        assert(std::isfinite(vertex.y));
        assert(std::isfinite(vertex.nx) && std::isfinite(vertex.ny) &&
               std::isfinite(vertex.nz));
        const float normal_length = std::sqrt(vertex.nx * vertex.nx +
            vertex.ny * vertex.ny + vertex.nz * vertex.nz);
        assert(std::abs(normal_length - 1.0F) < 0.001F);
    }
    bool invalid_terrain_rejected = false;
    try { (void)TerrainSystem::build(0.0F, 32U); }
    catch (const std::invalid_argument&) { invalid_terrain_rejected = true; }
    assert(invalid_terrain_rejected);

    LodSystem lod;
    assert(lod.select(7U, 10.0F) == TerrainLod::High);
    assert(lod.select(7U, 60.0F) == TerrainLod::Medium);
    assert(lod.select(7U, 52.0F) == TerrainLod::Medium); // hysteresis
    assert(lod.select(7U, 120.0F) == TerrainLod::Low);
    assert(lod.select(7U, 250.0F) == TerrainLod::Impostor);
    assert(lod.select(7U, 215.0F) == TerrainLod::Impostor); // hysteresis
    assert(lod.select(7U, 180.0F) == TerrainLod::Low);
    lod.forget(7U);
    assert(lod.tracked_count() == 0U);

    WorldPartition world(128.0F);
    assert((world.cell_at(0.0F, 0.0F) == WorldCell{0, 0}));
    assert((world.cell_at(-0.1F, -128.1F) == WorldCell{-1, -2}));
    const auto initial = world.update_streaming(0.0F, 0.0F, 1U);
    assert(initial.size() == 9U);
    assert(world.active_count() == 9U);
    assert(world.update_streaming(0.0F, 0.0F, 1U).empty());
    const auto shifted = world.update_streaming(128.0F, 0.0F, 1U);
    assert(shifted.size() == 3U);
    assert(world.active_count() == 9U);
    assert(world.active({1, 0}));
    assert(!world.active({-1, 0}));

    WorldPartition streamed_partition(128.0F);
    WorldCellStreamer streamer;
    std::size_t unloaded_cells = 0U;
    const auto loader = [](WorldCell cell, WorldCellContent& content) {
        content.cell = cell;
        content.terrain = TerrainSystem::build(128.0F, 8U);
        const float offset_x = static_cast<float>(cell.x) * 128.0F;
        const float offset_z = static_cast<float>(cell.z) * 128.0F;
        for (auto& vertex : content.terrain.vertices) {
            vertex.x += offset_x;
            vertex.z += offset_z;
        }
        return true;
    };
    const auto unloader = [&unloaded_cells](WorldCell, WorldCellContent&) {
        ++unloaded_cells;
    };
    const auto first_stream = streamer.update(streamed_partition, 0.0F, 0.0F, 1U, loader, unloader);
    assert(first_stream.loaded == 9U && first_stream.unloaded == 0U);
    assert(first_stream.failed == 0U && first_stream.resident == 9U);
    assert(streamer.find({0, 0}) != nullptr);
    const auto second_stream = streamer.update(streamed_partition, 128.0F, 0.0F, 1U, loader, unloader);
    assert(second_stream.loaded == 3U && second_stream.unloaded == 3U);
    assert(second_stream.resident == 9U && unloaded_cells == 3U);
    assert(streamer.find({-1, 0}) == nullptr);
    assert(streamer.find({2, 0}) != nullptr);

    bool invalid_radius_rejected = false;
    try { (void)world.update_streaming(0.0F, 0.0F, 1000U); }
    catch (const std::length_error&) { invalid_radius_rejected = true; }
    assert(invalid_radius_rejected);
}
