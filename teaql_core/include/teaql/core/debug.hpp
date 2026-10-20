#pragma once
#include <teaql/core/list.hpp>
#include <teaql/core/detail/debug.hpp>
#include <teaql/core/decimal_format.hpp>
#include <array>
#include <charconv>
#include <limits>

namespace teaql {
enum class DebugNumbers { Decimal, LowerHex, UpperHex };
enum class DebugAlignment { Default, Left, Right, Center };

struct DebugOptions {
    bool pretty = false;
    DebugNumbers numbers = DebugNumbers::Decimal;
    std::size_t width = 0;
    std::optional<std::size_t> precision;
    DebugAlignment alignment = DebugAlignment::Default;
    char32_t fill = U' ';
    bool sign_plus = false;
    bool zero_pad = false;
};

// Borrowed formatting state. Output and all temporary strings use one explicit owner.
// Custom element formatters write through this state; they need neither Clone nor assignment.
class DebugWriter {
public:
    DebugWriter(String& output, DebugOptions options) : output_(output), options_(options) {
        if (options.fill > 0x10ffff || (options.fill >= 0xd800 && options.fill <= 0xdfff)) {
            throw std::invalid_argument("Debug fill must be a Unicode scalar");
        }
        if (options.precision && *options.precision > 65535) {
            throw std::invalid_argument("Debug precision exceeds the source formatter limit");
        }
    }

    Memory memory_resource() const noexcept {
        return output_.get_allocator().resource();
    }

    const DebugOptions& options() const noexcept {
        return options_;
    }

    void raw(std::string_view text) {
        for (char byte : text) {
            if (indent_next_ && byte != '\n') {
                if (depth_ > output_.max_size() / 4) {
                    throw std::length_error("Debug indentation overflow");
                }
                output_.append(depth_ * 4, ' ');
                indent_next_ = false;
            }
            output_ += byte;
            if (byte == '\n') {
                indent_next_ = true;
            }
        }
    }

    // Custom scalar formatters may pad complete text without quoting or truncation.
    void padded_text(std::string_view value,
                     DebugAlignment default_alignment = DebugAlignment::Left) {
        padded(value, default_alignment);
    }

    void text(std::string_view value) {
        raw(detail::debug_text(memory_resource(), value));
    }

    template <std::integral T>
        requires(!std::same_as<T, bool>)
    void integer(T value) {
        char buffer[32];
        String digits(memory_resource());
        bool negative = false;
        String prefix(memory_resource());
        if (options_.numbers == DebugNumbers::Decimal) {
            auto [end, error] = std::to_chars(buffer, buffer + sizeof(buffer), value);
            if (error != std::errc{}) {
                throw std::length_error("Debug integer formatting failed");
            }
            digits.assign(buffer, end);
            if (digits.starts_with('-')) {
                negative = true;
                digits.erase(0, 1);
            }
        } else {
            auto unsigned_value = static_cast<std::make_unsigned_t<T>>(value);
            auto [end, error] = std::to_chars(buffer, buffer + sizeof(buffer), unsigned_value, 16);
            if (error != std::errc{}) {
                throw std::length_error("Debug integer formatting failed");
            }
            digits.assign(buffer, end);
            if (options_.numbers == DebugNumbers::UpperHex) {
                for (auto& digit : digits) {
                    if (digit >= 'a' && digit <= 'f') {
                        digit -= 'a' - 'A';
                    }
                }
            }
            if (options_.pretty) {
                prefix = "0x";
            }
        }
        numeric(digits, negative, prefix);
    }

    void boolean(bool value) {
        auto text = std::string_view(value ? "true" : "false");
        if (options_.precision) {
            text = text.substr(0, *options_.precision);
        }
        padded(text, DebugAlignment::Left);
    }

    template <class Float>
        requires(std::same_as<Float, float> || std::same_as<Float, double>)
    void floating(Float value) {
        String digits(memory_resource());
        if (options_.precision) {
            digits.resize(*options_.precision + 326);
            auto [end, error] = std::to_chars(digits.data(),
                                              digits.data() + digits.size(),
                                              value,
                                              std::chars_format::fixed,
                                              int(*options_.precision));
            if (error != std::errc{}) {
                throw std::length_error("Debug floating formatting failed");
            }
            digits.resize(end - digits.data());
            if (digits == "nan" || digits == "-nan") {
                digits = "NaN";
            }
        } else if constexpr (std::same_as<Float, float>) {
            digits = detail::debug_float32(memory_resource(), value);
        } else {
            auto wrapped = detail::debug_value(memory_resource(), Value(memory_resource(), value));
            digits.assign(std::string_view(wrapped).substr(4, wrapped.size() - 5));
        }
        bool negative = digits.starts_with('-');
        if (negative) {
            digits.erase(0, 1);
        }
        numeric(digits, negative, {}, digits != "NaN");
    }

    void decimal(const Decimal& value) {
        DecimalFormat format;
        format.notation = DecimalNotation::Debug;
        format.width = options_.width;
        format.precision = options_.precision;
        format.fill = options_.fill;
        format.sign_plus = options_.sign_plus;
        format.zero_pad = options_.zero_pad;
        format.alignment = options_.alignment == DebugAlignment::Left     ? DecimalAlignment::Left
                           : options_.alignment == DebugAlignment::Center ? DecimalAlignment::Center
                                                                          : DecimalAlignment::Right;
        raw(value.format(memory_resource(), format));
    }

    template <class Element>
    void tuple(std::string_view name, Element&& element) {
        raw(name);
        raw("(");
        if (options_.pretty) {
            ++depth_;
            raw("\n");
        }
        std::invoke(element, *this);
        if (options_.pretty) {
            raw(",\n");
            --depth_;
        } else if (name.empty()) {
            raw(",");
        }
        raw(")");
    }

    class Fields {
    public:
        Fields(DebugWriter& writer) : writer_(writer) {}

        template <class Element>
        void field(std::string_view name, Element&& element) {
            if (count_++ == 0) {
                writer_.raw(writer_.options_.pretty ? " {\n" : " { ");
                if (writer_.options_.pretty) {
                    ++writer_.depth_;
                }
            } else if (!writer_.options_.pretty) {
                writer_.raw(", ");
            }
            writer_.raw(name);
            writer_.raw(": ");
            std::invoke(element, writer_);
            if (writer_.options_.pretty) {
                writer_.raw(",\n");
            }
        }

    private:
        friend class DebugWriter;
        DebugWriter& writer_;
        std::size_t count_ = 0;
    };

    template <class Body>
    void structure(std::string_view name, Body&& body, bool non_exhaustive = false) {
        raw(name);
        Fields fields(*this);
        std::invoke(body, fields);
        if (non_exhaustive) {
            if (fields.count_++ == 0) {
                raw(options_.pretty ? " {\n" : " { ");
                if (options_.pretty) {
                    ++depth_;
                }
            } else if (!options_.pretty) {
                raw(", ");
            }
            raw("..");
            if (options_.pretty) {
                raw("\n");
            }
        }
        if (fields.count_) {
            if (options_.pretty) {
                --depth_;
                raw("}");
            } else {
                raw(" }");
            }
        }
    }

    template <std::ranges::input_range Range, class Element>
    void sequence(Range&& items,
                  Element&& element,
                  std::string_view opening = "[",
                  std::string_view closing = "]") {
        raw(opening);
        bool first = true;
        for (auto&& item : items) {
            if (first) {
                first = false;
                if (options_.pretty) {
                    ++depth_;
                    raw("\n");
                }
            } else if (!options_.pretty) {
                raw(", ");
            }
            std::invoke(element, *this, item);
            if (options_.pretty) {
                raw(",\n");
            }
        }
        if (!first && options_.pretty) {
            --depth_;
        }
        raw(closing);
    }

private:
    void fill(std::size_t count, char32_t code) {
        String encoded(memory_resource());
        if (code < 0x80) {
            encoded += char(code);
        } else if (code < 0x800) {
            encoded += char(0xc0 | (code >> 6));
            encoded += char(0x80 | (code & 63));
        } else if (code < 0x10000) {
            encoded += char(0xe0 | (code >> 12));
            encoded += char(0x80 | ((code >> 6) & 63));
            encoded += char(0x80 | (code & 63));
        } else {
            encoded += char(0xf0 | (code >> 18));
            encoded += char(0x80 | ((code >> 12) & 63));
            encoded += char(0x80 | ((code >> 6) & 63));
            encoded += char(0x80 | (code & 63));
        }
        if (count > output_.max_size() / encoded.size()) {
            throw std::length_error("Debug padding overflow");
        }
        for (std::size_t index = 0; index < count; ++index) {
            raw(encoded);
        }
    }

    void padded(std::string_view text, DebugAlignment default_alignment) {
        std::size_t length = 0;
        for (unsigned char byte : text) {
            if ((byte & 0xc0) != 0x80) {
                ++length;
            }
        }
        auto padding = options_.width > length ? options_.width - length : 0;
        auto alignment =
            options_.alignment == DebugAlignment::Default ? default_alignment : options_.alignment;
        auto left = alignment == DebugAlignment::Right    ? padding
                    : alignment == DebugAlignment::Center ? padding / 2
                                                          : 0;
        fill(left, options_.fill);
        raw(text);
        fill(padding - left, options_.fill);
    }

    void numeric(std::string_view digits,
                 bool negative,
                 std::string_view prefix,
                 bool allow_plus = true) {
        String sign(negative                           ? "-"
                    : options_.sign_plus && allow_plus ? "+"
                                                       : "",
                    memory_resource());
        auto length = sign.size() + prefix.size() + digits.size();
        if (options_.zero_pad) {
            raw(sign);
            raw(prefix);
            fill(options_.width > length ? options_.width - length : 0, U'0');
            raw(digits);
        } else {
            String whole(sign, memory_resource());
            whole += prefix;
            whole += digits;
            padded(whole, DebugAlignment::Right);
        }
    }

    String& output_;
    DebugOptions options_;
    std::size_t depth_ = 0;
    bool indent_next_ = false;
};

inline void write_debug(DebugWriter& writer, const Value& value);

inline void write_debug(DebugWriter& writer, DataType type) {
    constexpr std::array<std::string_view, 10> names{
        "Bool", "I64", "U64", "F64", "Decimal", "Text", "LargeText", "Json", "Date", "Timestamp"};
    auto index = static_cast<std::size_t>(type);
    if (index >= names.size()) {
        throw std::invalid_argument("Unknown DataType diagnostic variant");
    }
    writer.raw(names[index]);
}

inline void write_debug(DebugWriter& writer, const Json& value) {
    if (const auto* items = std::get_if<Json::Array>(&value.data)) {
        writer.raw("Array ");
        writer.sequence(*items, [](auto& out, const auto& item) {
            write_debug(out, item);
        });
    } else if (const auto* items = std::get_if<Json::Object>(&value.data)) {
        writer.raw("Object ");
        writer.sequence(
            *items,
            [](auto& out, const auto& pair) {
                out.text(pair.first);
                out.raw(": ");
                write_debug(out, pair.second);
            },
            "{",
            "}");
    } else {
        auto wrapped =
            detail::debug_value(writer.memory_resource(), Value(writer.memory_resource(), value));
        writer.raw(std::string_view(wrapped).substr(5, wrapped.size() - 6));
    }
}

inline void write_debug(DebugWriter& writer, const Value& value) {
    auto owner = writer.memory_resource();
    std::visit(
        [&](const auto& item) {
            using T = std::decay_t<decltype(item)>;
            if constexpr (std::same_as<T, std::monostate>) {
                writer.raw("Null");
            } else {
                constexpr auto name = [] {
                    if constexpr (std::same_as<T, bool>) {
                        return "Bool";
                    } else if constexpr (std::same_as<T, std::int64_t>) {
                        return "I64";
                    } else if constexpr (std::same_as<T, std::uint64_t>) {
                        return "U64";
                    } else if constexpr (std::same_as<T, double>) {
                        return "F64";
                    } else if constexpr (std::same_as<T, String>) {
                        return "Text";
                    } else if constexpr (std::same_as<T, Decimal>) {
                        return "Decimal";
                    } else if constexpr (std::same_as<T, Json>) {
                        return "Json";
                    } else if constexpr (std::same_as<T, Date>) {
                        return "Date";
                    } else if constexpr (std::same_as<T, Timestamp>) {
                        return "Timestamp";
                    } else if constexpr (std::same_as<T, TypedNull>) {
                        return "TypedNull";
                    } else if constexpr (std::same_as<T, Value::List>) {
                        return "List";
                    } else {
                        return "Object";
                    }
                }();
                writer.tuple(name, [&](auto& out) {
                    if constexpr (std::same_as<T, bool>) {
                        out.boolean(item);
                    } else if constexpr (std::integral<T>) {
                        out.integer(item);
                    } else if constexpr (std::same_as<T, double>) {
                        out.floating(item);
                    } else if constexpr (std::same_as<T, String>) {
                        out.text(item);
                    } else if constexpr (std::same_as<T, Decimal>) {
                        out.decimal(item);
                    } else if constexpr (std::same_as<T, Json>) {
                        write_debug(out, item);
                    } else if constexpr (std::same_as<T, Date>) {
                        out.raw(item.to_string(owner));
                    } else if constexpr (std::same_as<T, Timestamp>) {
                        out.tuple("Timestamp", [&](auto& inner) {
                            inner.integer(item.as_millis());
                        });
                    } else if constexpr (std::same_as<T, TypedNull>) {
                        write_debug(out, item.type);
                    } else if constexpr (std::same_as<T, Value::List>) {
                        out.sequence(item, [](auto& inner, const auto& child) {
                            write_debug(inner, child);
                        });
                    } else {
                        out.sequence(
                            item,
                            [](auto& inner, const auto& pair) {
                                inner.text(pair.first);
                                inner.raw(": ");
                                write_debug(inner, pair.second);
                            },
                            "{",
                            "}");
                    }
                });
            }
        },
        value.data);
}

inline void write_debug(DebugWriter& writer, const Record& record) {
    writer.sequence(
        record,
        [](auto& out, const auto& pair) {
            out.text(pair.first);
            out.raw(": ");
            write_debug(out, pair.second);
        },
        "{",
        "}");
}

void write_debug(DebugWriter&, const FieldLayout&);
void write_debug(DebugWriter&, const LoadedSnapshot&);
void write_debug(DebugWriter&, const DynamicPropertyDefinitions&);
void write_debug(DebugWriter&, const ColumnLayout&);
void write_debug(DebugWriter&, const CompactRow&);

inline void write_debug(DebugWriter& writer, bool value) {
    writer.boolean(value);
}

template <std::integral T>
    requires(!std::same_as<T, bool>)
void write_debug(DebugWriter& writer, T value) {
    writer.integer(value);
}

inline void write_debug(DebugWriter& writer, double value) {
    writer.floating(value);
}

inline void write_debug(DebugWriter& writer, float value) {
    writer.floating(value);
}

inline void write_debug(DebugWriter& writer, std::string_view value) {
    writer.text(value);
}

inline void write_debug(DebugWriter& writer, const String& value) {
    writer.text(value);
}

inline void write_debug(DebugWriter& writer, const char* value) {
    writer.text(value);
}

inline void write_debug(DebugWriter& writer, const Decimal& value) {
    writer.decimal(value);
}

inline void write_debug(DebugWriter& writer, Timestamp value) {
    writer.tuple("Timestamp", [&](auto& out) {
        out.integer(value.as_millis());
    });
}

inline void write_debug(DebugWriter& writer, Date value) {
    writer.raw(value.to_string(writer.memory_resource()));
}

struct DefaultDebugElement {
    template <class T>
    void operator()(DebugWriter& writer, const T& item) const {
        write_debug(writer, item);
    }
};

template <class T, class Element>
void write_debug(DebugWriter& writer, const SmartList<T>& list, Element&& element) {
    writer.structure("SmartList", [&](auto& fields) {
        fields.field("data", [&](auto& out) {
            out.sequence(list.data, element);
        });
        fields.field("total_count", [&](auto& out) {
            if (list.total_count) {
                out.tuple("Some", [&](auto& inner) {
                    inner.integer(*list.total_count);
                });
            } else {
                out.raw("None");
            }
        });
        auto record = [](auto& out, const Record& items) {
            out.sequence(
                items,
                [](auto& inner, const auto& pair) {
                    inner.text(pair.first);
                    inner.raw(": ");
                    write_debug(inner, pair.second);
                },
                "{",
                "}");
        };
        fields.field("aggregations", [&](auto& out) {
            record(out, list.aggregations);
        });
        fields.field("summary", [&](auto& out) {
            record(out, list.summary);
        });
        fields.field("facets", [&](auto& out) {
            out.sequence(
                list.facets,
                [](auto& inner, const auto& pair) {
                    inner.text(pair.first);
                    inner.raw(": ");
                    write_debug(inner, pair.second, DefaultDebugElement{});
                },
                "{",
                "}");
        });
        fields.field("is_loaded", [&](auto& out) {
            out.boolean(list.is_loaded);
        });
    });
}

template <class T>
String debug_string(Memory owner, const T& value, DebugOptions options = {}) {
    String result(owner);
    DebugWriter writer(result, options);
    if constexpr (requires { write_debug(writer, value); }) {
        write_debug(writer, value);
    } else {
        write_debug(writer, value, DefaultDebugElement{});
    }
    return result;
}

template <class T, class Element>
    requires std::invocable<Element&, DebugWriter&, const T&>
String
debug_string(Memory owner, const SmartList<T>& list, Element&& element, DebugOptions options = {}) {
    String result(owner);
    DebugWriter writer(result, options);
    write_debug(writer, list, std::forward<Element>(element));
    return result;
}
}
