#pragma once
#include <teaql/memory/types.hpp>
#include <charconv>
#include <cstdlib>
#include <thread>
#if defined(_WIN32)
#include <process.h>
#else
#include <unistd.h>
#endif
#if defined(__linux__)
#include <sys/syscall.h>
#endif
namespace teaql::detail {
// Environment identity is captured at Context construction, including empty
// values. Invalid UTF-8 has the same fallback behavior as Rust env::var.
inline bool identity_utf8(std::string_view s) {
    for (std::size_t i = 0; i < s.size();) {
        auto b = static_cast<unsigned char>(s[i]);
        if (b < 0x80) {
            ++i;
            continue;
        }
        unsigned n = b >= 0xc2 && b <= 0xdf   ? 2
                     : b >= 0xe0 && b <= 0xef ? 3
                     : b >= 0xf0 && b <= 0xf4 ? 4
                                              : 0;
        if (!n || n > s.size() - i) {
            return false;
        }
        for (unsigned k = 1; k < n; ++k) {
            auto c = static_cast<unsigned char>(s[i + k]);
            if ((c & 0xc0) != 0x80) {
                return false;
            }
            if (k == 1 && ((b == 0xe0 && c < 0xa0) || (b == 0xed && c >= 0xa0) ||
                           (b == 0xf0 && c < 0x90) || (b == 0xf4 && c >= 0x90))) {
                return false;
            }
        }
        i += n;
    }
    return true;
}

inline String default_user_identifier(Memory r) {
    auto valid = [](const char* key) -> const char* {
        auto* v = std::getenv(key);
        return v && identity_utf8(v) ? v : nullptr;
    };
    auto* user = valid("USER");
    if (!user) {
        user = valid("USERNAME");
    }
    String result(user ? user : "main", r);
    auto append = [&](auto n) {
        char b[32];
        auto end = std::to_chars(b, b + sizeof(b), n).ptr;
        result.append(b, end);
    };
    result += "@pid-";
#if defined(_WIN32)
    append(::_getpid());
#else
    append(::getpid());
#endif
    result += ".tid-";
#if defined(__linux__)
    append(::syscall(SYS_gettid));
#else
    append(std::hash<std::thread::id>{}(std::this_thread::get_id()));
#endif
    return result;
}
}
