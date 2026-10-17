#include "register_types.h"

#include "core/object/class_db.h"

#include "arut_car_3d.h"
#include "arut_character_3d.h"
#include "arut_ragdoll_dummy_3d.h"
#include "arut_render_profile_3d.h"

void initialize_arut_module(ModuleInitializationLevel p_level) {
    if (p_level != MODULE_INITIALIZATION_LEVEL_SCENE) {
        return;
    }

    ClassDB::register_class<ArutCar3D>();
    ClassDB::register_class<ArutCharacter3D>();
    ClassDB::register_class<ArutRagdollDummy3D>();
    ClassDB::register_class<ArutRenderProfile3D>();
}

void uninitialize_arut_module(ModuleInitializationLevel p_level) {
    if (p_level != MODULE_INITIALIZATION_LEVEL_SCENE) {
        return;
    }
}
