#include <teaql/provider_sqlite/detail/schema.hpp>
#include <teaql/runtime/context.hpp>
#include <iostream>
using namespace teaql;

static void append(String& out, std::uint32_t c) {
    if (c < 128)
        out += char(c);
    else if (c < 0x800) {
        out += char(0xc0 | (c >> 6));
        out += char(0x80 | (c & 63));
    } else if (c < 0x10000) {
        out += char(0xe0 | (c >> 12));
        out += char(0x80 | ((c >> 6) & 63));
        out += char(0x80 | (c & 63));
    } else {
        out += char(0xf0 | (c >> 18));
        out += char(0x80 | ((c >> 12) & 63));
        out += char(0x80 | ((c >> 6) & 63));
        out += char(0x80 | (c & 63));
    }
}

static void hash(std::uint64_t& value, std::string_view text) {
    for (unsigned char c : text)
        value = (value ^ c) * 1099511628211ull;
    value = (value ^ 255) * 1099511628211ull;
}

int main() {
    try {
        GlobalContext global;
        auto r = global.memory_resource();
        auto* old = std::pmr::set_default_resource(std::pmr::null_memory_resource());
        std::array<std::uint64_t, 5> hashes;
        hashes.fill(14695981039346656037ull);
        for (std::uint32_t c = 0; c <= 0x10ffff; ++c) {
            if (c >= 0xd800 && c <= 0xdfff)
                continue;
            String scalar(r);
            append(scalar, c);
            std::array<String, 5> cases{String(scalar, r),
                                        String("AΣ", r),
                                        String("AΣ", r),
                                        String(scalar, r),
                                        String(scalar, r)};
            cases[1] += scalar;
            cases[2] += scalar;
            cases[2] += 'A';
            cases[3] += "Σ";
            cases[4] += "ΣA";
            for (std::size_t i = 0; i < cases.size(); ++i)
                hash(hashes[i], detail::sqlite_identifier_lower(r, cases[i]));
        }
        // Exhaustive Unicode 17 observations from the public Rust oracle. Each hash
        // includes all Unicode scalars in order and a separator after each result.
        constexpr std::array<std::uint64_t, 5> expected{5417383564357655604ull,
                                                        8810379091495780097ull,
                                                        13049804147059838974ull,
                                                        18199685264230103116ull,
                                                        12654908739667017640ull};
        if (hashes != expected)
            throw std::runtime_error("Unicode lowercase differs from exhaustive Rust observations");
        std::pmr::set_default_resource(old);
        std::cout << "All Unicode scalar lowercase and four sigma contexts match Rust\n";
    } catch (const std::exception& e) {
        std::cerr << e.what() << '\n';
        return 1;
    }
}
