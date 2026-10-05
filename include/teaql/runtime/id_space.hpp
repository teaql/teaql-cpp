#pragma once
#include <teaql/memory/types.hpp>

namespace teaql {
// Rust runtime's canonical_id_space_entity: ASCII capitals each introduce a
// separator except at the start. Non-ASCII UTF-8 bytes are preserved verbatim.
inline String canonical_id_space_entity(Memory destination, std::string_view entity) {
    String result(destination);
    result.reserve(entity.size() + 4);
    for (std::size_t index = 0; index < entity.size(); ++index) {
        auto c = entity[index];
        if (c >= 'A' && c <= 'Z') {
            if (index)
                result += '_';
            result += char(c + ('a' - 'A'));
        } else
            result += c;
    }
    return result;
}
}
