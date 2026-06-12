#include "kem/AST.hpp"

namespace kem {

std::string TypeAnnotation::toString() const {
    std::string base;
    switch (kind) {
        case TypeKind::ENTERO:   base = "entero";   break;
        case TypeKind::DECIMAL:  base = "decimal";  break;
        case TypeKind::TEXTO:    base = "texto";    break;
        case TypeKind::BOOLEANO: base = "booleano"; break;
        case TypeKind::VOID:     base = "vacio";    break;
        default:                 base = "?";        break;
    }
    if (is_array) {
        base += "[" + std::to_string(array_size) + "]";
    }
    return base;
}

} // namespace kem
