#include <teaql/provider_sqlite/native_diagnostics.hpp>
#include <algorithm>

namespace teaql {
namespace {
template <class T>
void append_number(String& out, T value) {
    char buffer[32];
    auto result = std::to_chars(buffer, buffer + sizeof(buffer), value);
    if (result.ec != std::errc{}) {
        throw std::length_error("SQLite diagnostic number conversion failed");
    }
    out.append(buffer, result.ptr);
}
}

std::optional<SqliteUtf8Error> SqliteUtf8Error::validate(std::string_view bytes) noexcept {
    for (std::size_t position = 0; position < bytes.size();) {
        const auto first = static_cast<unsigned char>(bytes[position]);
        if (first < 128) {
            ++position;
            continue;
        }
        const unsigned width = first >= 0xc2 && first <= 0xdf   ? 2
                               : first >= 0xe0 && first <= 0xef ? 3
                               : first >= 0xf0 && first <= 0xf4 ? 4
                                                                : 0;
        unsigned length = 1;
        if (width) {
            for (; length < width; ++length) {
                if (length == bytes.size() - position) {
                    return SqliteUtf8Error(position, {});
                }
                const auto next = static_cast<unsigned char>(bytes[position + length]);
                if ((next & 0xc0) != 0x80 ||
                    (length == 1 &&
                     ((first == 0xe0 && next < 0xa0) || (first == 0xed && next >= 0xa0) ||
                      (first == 0xf0 && next < 0x90) || (first == 0xf4 && next >= 0x90)))) {
                    break;
                }
            }
            if (length == width) {
                position += width;
                continue;
            }
        }
        return SqliteUtf8Error(position, static_cast<std::uint8_t>(length));
    }
    return {};
}

String SqliteUtf8Error::to_string(Memory r) const {
    String text(r);
    if (length_) {
        text = "invalid utf-8 sequence of ";
        append_number(text, *length_);
        text += " bytes from index ";
    } else {
        text = "incomplete utf-8 byte sequence from index ";
    }
    append_number(text, position_);
    return text;
}

std::optional<SqliteNulError> SqliteNulError::from_bytes(Memory r,
                                                         std::span<const std::uint8_t> bytes) {
    auto first = std::find(bytes.begin(), bytes.end(), 0);
    if (first == bytes.end()) {
        return {};
    }
    return SqliteNulError(r, static_cast<std::size_t>(first - bytes.begin()), bytes);
}

SqliteNulError::SqliteNulError(std::allocator_arg_t, Allocator a, const SqliteNulError& other)
    : position_(other.position_), bytes_(other.bytes_, a.resource()) {}

String SqliteNulError::to_string(Memory r) const {
    String text("nul byte found in provided data at position: ", r);
    append_number(text, position_);
    return text;
}

void write_debug(DebugWriter& writer, const SqliteUtf8Error& error) {
    writer.structure("Utf8Error", [&](auto& fields) {
        fields.field("valid_up_to", [&](auto& out) {
            out.integer(error.valid_up_to());
        });
        fields.field("error_len", [&](auto& out) {
            if (auto length = error.error_len()) {
                out.tuple("Some", [&](auto& inner) {
                    inner.integer(static_cast<std::uint8_t>(*length));
                });
            } else {
                out.raw("None");
            }
        });
    });
}

void write_debug(DebugWriter& writer, const SqliteNulError& error) {
    writer.tuple("NulError", [&](auto& out) {
        out.integer(error.nul_position());
        out.raw(out.options().pretty ? ",\n" : ", ");
        out.sequence(error.bytes(), DefaultDebugElement{});
    });
}

String SqlitePath::to_string_lossy(Memory r) const {
    String result(r);
    std::string_view remaining = bytes_;
    while (auto invalid = SqliteUtf8Error::validate(remaining)) {
        result += remaining.substr(0, invalid->valid_up_to());
        result += "\xef\xbf\xbd";
        auto length = invalid->error_len().value_or(remaining.size() - invalid->valid_up_to());
        remaining.remove_prefix(invalid->valid_up_to() + length);
    }
    result += remaining;
    return result;
}

namespace {
class PathComponents {
public:
    explicit PathComponents(std::string_view bytes) : remaining_(bytes) {}

    std::optional<std::string_view> next() noexcept {
        if (first_) {
            first_ = false;
            if (remaining_.starts_with('/')) {
                skip_slashes();
                return "/";
            }
            if (remaining_ == "." || remaining_.starts_with("./")) {
                remaining_.remove_prefix(1);
                skip_slashes();
                return ".";
            }
        }
        while (!remaining_.empty()) {
            skip_slashes();
            if (remaining_.empty()) {
                return {};
            }
            auto separator = remaining_.find('/');
            auto part = remaining_.substr(0, separator);
            remaining_.remove_prefix(part.size());
            if (part != ".") {
                return part;
            }
        }
        return {};
    }

private:
    void skip_slashes() noexcept {
        while (remaining_.starts_with('/')) {
            remaining_.remove_prefix(1);
        }
    }

    std::string_view remaining_;
    bool first_ = true;
};
}

bool operator==(const SqlitePath& left, const SqlitePath& right) noexcept {
    PathComponents a(left.bytes_);
    PathComponents b(right.bytes_);
    while (true) {
        auto first = a.next();
        auto second = b.next();
        if (first != second) {
            return false;
        }
        if (!first) {
            return true;
        }
    }
}

void write_debug(DebugWriter& writer, const SqlitePath& path) {
    writer.raw("\"");
    auto remaining = path.as_encoded_bytes();
    constexpr std::string_view hex = "0123456789ABCDEF";
    while (!remaining.empty()) {
        auto invalid = SqliteUtf8Error::validate(remaining);
        auto valid_length = invalid ? invalid->valid_up_to() : remaining.size();
        auto escaped =
            detail::debug_text(writer.memory_resource(), remaining.substr(0, valid_length));
        writer.raw(std::string_view(escaped).substr(1, escaped.size() - 2));
        remaining.remove_prefix(valid_length);
        if (!invalid) {
            break;
        }
        auto length = invalid->error_len().value_or(remaining.size());
        for (std::size_t index = 0; index < length; ++index) {
            auto byte = static_cast<unsigned char>(remaining[index]);
            char escape[]{'\\', 'x', hex[byte >> 4], hex[byte & 15]};
            writer.raw(std::string_view(escape, sizeof(escape)));
        }
        remaining.remove_prefix(length);
    }
    writer.raw("\"");
}
}
