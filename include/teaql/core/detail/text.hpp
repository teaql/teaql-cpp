#pragma once
#include <initializer_list>
#include <string_view>

namespace teaql::detail {
// Rust Unicode White_Space, independent of the process locale.
inline void trim_start(std::string_view& value) {
    while (!value.empty()) {
        auto c = static_cast<unsigned char>(value.front());
        if (c == ' ' || (c >= 9 && c <= 13)) {
            value.remove_prefix(1);
            continue;
        }
        bool matched = false;
        for (std::string_view space : {"\xc2\x85",
                                       "\xc2\xa0",
                                       "\xe1\x9a\x80",
                                       "\xe2\x80\x80",
                                       "\xe2\x80\x81",
                                       "\xe2\x80\x82",
                                       "\xe2\x80\x83",
                                       "\xe2\x80\x84",
                                       "\xe2\x80\x85",
                                       "\xe2\x80\x86",
                                       "\xe2\x80\x87",
                                       "\xe2\x80\x88",
                                       "\xe2\x80\x89",
                                       "\xe2\x80\x8a",
                                       "\xe2\x80\xa8",
                                       "\xe2\x80\xa9",
                                       "\xe2\x80\xaf",
                                       "\xe2\x81\x9f",
                                       "\xe3\x80\x80"}) {
            if (value.starts_with(space)) {
                value.remove_prefix(space.size());
                matched = true;
                break;
            }
        }
        if (!matched) {
            return;
        }
    }
}

inline void trim(std::string_view& value) {
    trim_start(value);
    while (!value.empty()) {
        auto start = value.size() - 1;
        while (start && (static_cast<unsigned char>(value[start]) & 0xc0) == 0x80) {
            --start;
        }
        auto tail = value.substr(start);
        trim_start(tail);
        if (!tail.empty()) {
            break;
        }
        value = value.substr(0, start);
    }
}
}
