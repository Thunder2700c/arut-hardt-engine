#include "arut/core/cube_stress_scene.hpp"

#include <cmath>
#include <cstddef>
#include <cstdint>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <string_view>

namespace {
bool check(bool condition, std::ofstream& log, std::string_view label) {
    log << (condition ? "PASS " : "FAIL ") << label << '\n';
    std::cout << (condition ? "[PASS] " : "[FAIL] ") << label << '\n';
    return condition;
}
}

int main() {
    std::ofstream log("phase3-ecs-5000-cubes.log", std::ios::out | std::ios::trunc);
    if (!log) {
        std::cerr << "Cannot create phase3-ecs-5000-cubes.log\n";
        return 2;
    }

    log << "ARUT-HARDT Phase 3 ECS 5,000-cube stress test\n";
    log << "grid=20x10x25 target=5000\n";
    arut::core::World world;
    arut::core::CubeSpawnSystem spawn_system;
    arut::core::CubeRenderExtractionSystem extraction_system;
    bool passed = true;

    spawn_system.update(world);
    passed &= check(spawn_system.spawned_count() == arut::core::CubeSpawnSystem::target_count,
                    log, "CubeSpawnSystem creates exactly 5,000 entity handles");
    passed &= check(world.entity_count() == arut::core::CubeSpawnSystem::target_count,
                    log, "World owns exactly 5,000 live entities");

    spawn_system.update(world);
    passed &= check(world.entity_count() == arut::core::CubeSpawnSystem::target_count &&
                    spawn_system.spawned_count() == arut::core::CubeSpawnSystem::target_count,
                    log, "repeated system update is idempotent and does not duplicate cubes");

    std::size_t complete_entities = 0U;
    bool component_data_valid = true;
    bool handles_valid = true;
    for (const auto entity : spawn_system.entities()) {
        handles_valid &= world.alive(entity);
        if (!world.has<arut::core::CubeTransformComponent>(entity) ||
            !world.has<arut::core::CubeMeshComponent>(entity) ||
            !world.has<arut::core::CubeBoundsComponent>(entity)) {
            component_data_valid = false;
            continue;
        }
        const auto& transform = world.get<arut::core::CubeTransformComponent>(entity);
        const auto& mesh = world.get<arut::core::CubeMeshComponent>(entity);
        const auto& bounds = world.get<arut::core::CubeBoundsComponent>(entity);
        component_data_valid &= transform.scale >= 0.34F && transform.scale <= 0.49F;
        component_data_valid &= transform.yaw >= 0.0F && transform.yaw < 3.1415927F;
        component_data_valid &= mesh.mesh_id == 0U && mesh.material_id == 0U;
        component_data_valid &= std::abs(bounds.radius - transform.scale * 0.866025404F) < 1.0e-5F;
        ++complete_entities;
    }
    passed &= check(handles_valid, log, "all generated handles remain alive and generation-valid");
    passed &= check(complete_entities == 5000U && component_data_valid,
                    log, "all 5,000 entities contain valid transform, mesh and bounds components");

    const auto first = world.get<arut::core::CubeTransformComponent>(
        spawn_system.entities().front());
    const auto last = world.get<arut::core::CubeTransformComponent>(
        spawn_system.entities().back());
    passed &= check(std::abs(first.position[0] + 19.0F) < 1.0e-5F &&
                    std::abs(first.position[1] + 9.0F) < 1.0e-5F &&
                    std::abs(first.position[2] + 24.0F) < 1.0e-5F &&
                    std::abs(last.position[0] - 19.0F) < 1.0e-5F &&
                    std::abs(last.position[1] - 9.0F) < 1.0e-5F &&
                    std::abs(last.position[2] - 24.0F) < 1.0e-5F,
                    log, "grid transform boundaries match the 20x10x25 layout");

    arut::core::SceneGraphSystem scene_graph_system;
    scene_graph_system.update(world);
    const auto first_binding = world.get<arut::core::SceneGraphBindingComponent>(
        spawn_system.entities().front());
    const auto first_world_transform = scene_graph_system.graph().world_transform(first_binding.node);
    const bool scene_graph_valid =
        scene_graph_system.bound_entity_count() == 5000U &&
        scene_graph_system.graph().node_count() == 5000U &&
        first_world_transform.has_value() &&
        std::abs(first_world_transform->value[12] + 19.0F) < 1.0e-5F &&
        std::abs(first_world_transform->value[13] + 9.0F) < 1.0e-5F &&
        std::abs(first_world_transform->value[14] + 24.0F) < 1.0e-5F;
    passed &= check(scene_graph_valid, log,
                    "5,000 ECS entities synchronize to scene nodes with correct world transforms");

    extraction_system.update(world, &scene_graph_system);
    const auto& instances = extraction_system.instances();
    passed &= check(extraction_system.extracted_count() == 5000U &&
                    instances.size() == 5000U,
                    log, "render extraction emits exactly 5,000 ECS-derived instances");
    bool render_data_valid = instances.size() == 5000U;
    if (render_data_valid) {
        render_data_valid &= std::abs(instances.front().model[12] + 19.0F) < 1.0e-5F;
        render_data_valid &= std::abs(instances.front().model[13] + 9.0F) < 1.0e-5F;
        render_data_valid &= std::abs(instances.front().model[14] + 24.0F) < 1.0e-5F;
        render_data_valid &= std::abs(instances.back().model[12] - 19.0F) < 1.0e-5F;
        render_data_valid &= std::abs(instances.back().model[13] - 9.0F) < 1.0e-5F;
        render_data_valid &= std::abs(instances.back().model[14] - 24.0F) < 1.0e-5F;
        render_data_valid &= instances.front().bounds[3] > 0.0F;
        render_data_valid &= instances.front().model[15] == 1.0F;
    }
    passed &= check(render_data_valid, log,
                    "render hand-off contains ECS component transforms and valid bounds");

    log << "world_entities=" << world.entity_count() << '\n';
    log << "spawned_handles=" << spawn_system.spawned_count() << '\n';
    log << "complete_component_entities=" << complete_entities << '\n';
    log << "renderer_input_instances=" << instances.size() << '\n';
    log << "first_translation=-19,-9,-24\n";
    log << "last_translation=19,9,24\n";
    log << "cube_spawn_source=CubeSpawnSystem\n";
    log << "renderer_spawn_generation=removed\n";
    log << "RESULT=" << (passed ? "PASS" : "FAIL") << '\n';
    log.flush();
    std::cout << "RESULT=" << (passed ? "PASS" : "FAIL")
              << " log=phase3-ecs-5000-cubes.log\n";
    return passed ? 0 : 1;
}
