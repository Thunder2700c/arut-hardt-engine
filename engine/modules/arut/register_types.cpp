#include "register_types.h"

#include "core/object/class_db.h"

#include "arut_car_3d.h"

void initialize_arut_module(ModuleInitializationLevel p_level) {
    if (p_level != MODULE_INITIALIZATION_LEVEL_SCENE) {
        return;
    }

    ClassDB::register_class<ArutCar3D>();
}

void uninitialize_arut_module(ModuleInitializationLevel p_level) {
    if (p_level != MODULE_INITIALIZATION_LEVEL_SCENE) {
        return;
    }
}
