#include "arut_runtime.h"

namespace godot {

void ArutRuntime::_bind_methods() {
    ClassDB::bind_method(D_METHOD("get_build_info"), &ArutRuntime::get_build_info);
}

String ArutRuntime::get_build_info() const {
    return String("ARUT-HARDT Engine native layer | Godot 4.7 | C++");
}

} // namespace godot
