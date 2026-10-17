#ifndef ARUT_RUNTIME_H
#define ARUT_RUNTIME_H

#include <godot_cpp/classes/node.hpp>
#include <godot_cpp/variant/string.hpp>

namespace godot {

class ArutRuntime : public Node {
    GDCLASS(ArutRuntime, Node)

protected:
    static void _bind_methods();

public:
    String get_build_info() const;
};

} // namespace godot

#endif // ARUT_RUNTIME_H
