#pragma once
#include <teaql/core/debug.hpp>
#include <teaql/core/load_state.hpp>
#include <teaql/core/detail/model_json.hpp>
#include <teaql/memory/arena.hpp>
#include <algorithm>
#include <functional>
#include <span>
#include <variant>

namespace teaql::eval {
class LoadState {
public:
    using allocator_type = Allocator;
    enum class Kind { NotLoaded, Partial, PartialCompact, SharedColumns, Indexed, FullyLoaded };

    LoadState(std::allocator_arg_t, Allocator a) : fields_(a.resource()) {}

    LoadState(std::allocator_arg_t, Allocator a, const LoadState& other)
        : fields_(other.fields_, a.resource()), kind_(other.kind_) {
        if (other.indexed_) {
            if (a.resource() == other.memory_resource()) {
                indexed_ = other.indexed_;
            } else {
                auto* destination = dynamic_cast<Arena*>(a.resource());
                if (!destination) {
                    throw std::invalid_argument("Indexed load state requires a Context arena");
                }
                auto& copied = destination->make<LoadedSnapshot>(other.indexed_->get());
                indexed_.emplace(std::allocator_arg, a, copied);
            }
        }
        if (other.columns_) {
            if (a.resource() == other.memory_resource()) {
                columns_ = other.columns_;
            } else {
                columns_ = &arena(a.resource()).make_payload<ColumnLayout>(*other.columns_);
            }
        }
    }

    LoadState(std::allocator_arg_t, Allocator a, LoadState&& other)
        : LoadState(detail::move_to(a.resource(), std::move(other))) {}

    LoadState(const LoadState&) = delete;
    LoadState(LoadState&&) noexcept = default;

    LoadState& operator=(LoadState&& other) {
        return detail::assign_scoped(*this, std::move(other));
    }

    static LoadState not_loaded(Memory owner) {
        return LoadState(std::allocator_arg, Allocator(owner));
    }

    static LoadState fully_loaded(Memory owner) {
        auto result = not_loaded(owner);
        result.kind_ = Kind::FullyLoaded;
        return result;
    }

    static LoadState partial(Memory owner, std::span<const std::string_view> fields) {
        return named_fields(owner, Kind::Partial, fields);
    }

    static LoadState partial_compact(Memory owner, std::span<const std::string_view> fields) {
        return named_fields(owner, Kind::PartialCompact, fields);
    }

    static LoadState shared_columns(Memory owner, const ColumnLayout& columns) {
        auto& origin = arena(columns.memory_resource());
        if (!origin.can_lend_to(arena(owner))) {
            throw std::invalid_argument("Column layout cannot borrow across Context lifetimes");
        }
        auto result = not_loaded(owner);
        result.kind_ = Kind::SharedColumns;
        result.columns_ = &columns;
        return result;
    }

    static LoadState indexed(Memory owner, LoadedSnapshot::Borrow state) {
        if (state.memory_resource() != owner) {
            throw std::invalid_argument("Indexed state requires an explicit destination copy");
        }
        auto result = not_loaded(owner);
        result.kind_ = Kind::Indexed;
        result.indexed_ = state;
        return result;
    }

    const LoadedSnapshot::Borrow* indexed_snapshot() const noexcept {
        return indexed_ ? &*indexed_ : nullptr;
    }

    Result<LoadState, String> into_indexed(SnapshotPool& pool, const FieldLayout& layout) const {
        auto owner = pool.memory_resource();
        using Output = Result<LoadState, String>;
        if (indexed_) {
            if (&indexed_->get().layout() != &layout) {
                return Output::err(
                    owner, String("incompatible loaded-state type or layout revision", owner));
            }
            return Output::ok(owner, indexed(owner, *indexed_));
        }
        if (kind_ == Kind::FullyLoaded) {
            return Output::ok(
                owner,
                indexed(owner, LoadedSnapshot::fully_loaded(owner, layout).into_shared(pool)));
        }
        Vector<std::string_view> fields(owner);
        if (kind_ != Kind::NotLoaded) {
            for (const auto& field : names()) {
                fields.push_back(field);
            }
        }
        return Output::ok(
            owner,
            indexed(owner, LoadedSnapshot::projection(owner, layout, fields).into_shared(pool)));
    }

    Result<std::monostate, String> mark_loaded(std::string_view field,
                                               SnapshotPool* pool = nullptr) {
        auto owner = memory_resource();
        using Output = Result<std::monostate, String>;
        if (indexed_) {
            if (!pool || pool->memory_resource() != owner) {
                throw std::invalid_argument("Indexed mutation requires its Context snapshot pool");
            }
            auto result = LoadedSnapshot::with_loaded(*pool, *indexed_, field, true);
            if (!result) {
                return Output::err(owner, std::move(result).error());
            }
            indexed_ = result.value();
        } else if (kind_ != Kind::FullyLoaded && !is_loaded(field)) {
            if (columns_) {
                fields_.assign(names().begin(), names().end());
                columns_ = nullptr;
            }
            fields_.emplace_back(field);
            if (kind_ == Kind::NotLoaded) {
                kind_ = Kind::PartialCompact;
            }
            if (kind_ == Kind::Partial) {
                std::sort(fields_.begin(), fields_.end());
            }
        }
        return Output::ok(owner, {});
    }

    Result<std::monostate, String> mark_unloaded(std::string_view field,
                                                 SnapshotPool* pool = nullptr) {
        auto owner = memory_resource();
        using Output = Result<std::monostate, String>;
        if (indexed_) {
            if (!pool || pool->memory_resource() != owner) {
                throw std::invalid_argument("Indexed mutation requires its Context snapshot pool");
            }
            auto result = LoadedSnapshot::with_loaded(*pool, *indexed_, field, false);
            if (!result) {
                return Output::err(owner, std::move(result).error());
            }
            indexed_ = result.value();
        } else if (kind_ == Kind::FullyLoaded) {
            return Output::err(owner,
                               String("clearing availability requires a generated layout", owner));
        } else {
            if (columns_) {
                fields_.assign(names().begin(), names().end());
                columns_ = nullptr;
            }
            std::erase_if(fields_, [&](const auto& name) {
                return name == field;
            });
        }
        return Output::ok(owner, {});
    }

    bool is_loaded(std::string_view field) const {
        if (indexed_) {
            return indexed_->get().is_loaded(field);
        }
        if (kind_ == Kind::NotLoaded) {
            return false;
        }
        if (kind_ == Kind::FullyLoaded) {
            return true;
        }
        const auto& fields = names();
        return std::find(fields.begin(), fields.end(), field) != fields.end();
    }

    Kind kind() const noexcept {
        return kind_;
    }

    std::span<const String> names() const noexcept {
        return columns_ ? columns_->names() : std::span<const String>(fields_);
    }

    const ColumnLayout* column_layout() const noexcept {
        return columns_;
    }

    Memory memory_resource() const noexcept {
        return fields_.get_allocator().resource();
    }

    bool operator==(const LoadState& other) const {
        if (indexed_ || other.indexed_) {
            return indexed_ && other.indexed_ && indexed_->get() == other.indexed_->get();
        }
        return kind_ == other.kind_ &&
               std::equal(
                   names().begin(), names().end(), other.names().begin(), other.names().end());
    }

    Json to_json(Memory destination) const {
        if (indexed_) {
            throw std::logic_error("the enum variant LoadState::Indexed cannot be serialized");
        }
        if (kind_ == Kind::NotLoaded || kind_ == Kind::FullyLoaded) {
            return Json(destination, kind_ == Kind::NotLoaded ? "NotLoaded" : "FullyLoaded");
        }
        Json::Array values(destination);
        for (const auto& name : names()) {
            values.emplace_back(name);
        }
        auto result = detail::model_json(destination);
        detail::model_put(result, variant_name(), std::move(values));
        return result;
    }

    static ModelResult<LoadState> from_json(Memory owner, const Json& input) {
        return detail::decode_model<LoadState>(owner, [&] {
            if (auto* unit = std::get_if<String>(&input.data)) {
                if (*unit == "NotLoaded") {
                    return not_loaded(owner);
                }
                if (*unit == "FullyLoaded") {
                    return fully_loaded(owner);
                }
            }
            const auto& object = detail::model_object(input, "LoadState");
            if (object.size() != 1) {
                throw ModelDecodeError{ModelDecodeError::Code::ExpectedObject, "LoadState"};
            }
            const auto& [name, value] = *object.begin();
            if ((name == "NotLoaded" || name == "FullyLoaded") &&
                std::holds_alternative<std::monostate>(value.data)) {
                return name == "NotLoaded" ? not_loaded(owner) : fully_loaded(owner);
            }
            Kind kind;
            if (name == "Partial") {
                kind = Kind::Partial;
            } else if (name == "PartialCompact") {
                kind = Kind::PartialCompact;
            } else if (name == "SharedColumns") {
                kind = Kind::SharedColumns;
            } else {
                throw ModelDecodeError{ModelDecodeError::Code::ExpectedStruct, "LoadState"};
            }
            Vector<std::string_view> names(owner);
            for (const auto& field : detail::model_array(value, "LoadState")) {
                names.push_back(detail::model_string(field, "LoadState"));
            }
            if (kind == Kind::SharedColumns) {
                Vector<String> columns(owner);
                for (auto name : names) {
                    columns.emplace_back(name);
                }
                auto& layout = arena(owner).make_payload<ColumnLayout>(std::move(columns));
                return shared_columns(owner, layout);
            }
            return named_fields(owner, kind, names);
        });
    }

    std::string_view variant_name() const noexcept {
        switch (kind_) {
        case Kind::NotLoaded:
            return "NotLoaded";
        case Kind::Partial:
            return "Partial";
        case Kind::PartialCompact:
            return "PartialCompact";
        case Kind::SharedColumns:
            return "SharedColumns";
        case Kind::Indexed:
            return "Indexed";
        case Kind::FullyLoaded:
            return "FullyLoaded";
        }
        std::terminate();
    }

private:
    static Arena& arena(Memory resource) {
        auto* result = dynamic_cast<Arena*>(resource);
        if (!result) {
            throw std::invalid_argument("Shared column layouts require a Context arena");
        }
        return *result;
    }

    static LoadState
    named_fields(Memory owner, Kind kind, std::span<const std::string_view> fields) {
        auto result = not_loaded(owner);
        result.kind_ = kind;
        for (auto name : fields) {
            result.fields_.emplace_back(name);
        }
        if (kind == Kind::Partial) {
            std::sort(result.fields_.begin(), result.fields_.end());
            result.fields_.erase(std::unique(result.fields_.begin(), result.fields_.end()),
                                 result.fields_.end());
        }
        return result;
    }

    Vector<String> fields_;
    Kind kind_ = Kind::NotLoaded;
    const ColumnLayout* columns_ = nullptr;
    std::optional<LoadedSnapshot::Borrow> indexed_;
};

struct MissingPath {
    using allocator_type = Allocator;
    String failed_node, attempted_path;

    MissingPath(std::allocator_arg_t, Allocator a, std::string_view node, std::string_view path)
        : failed_node(node, a.resource()), attempted_path(path, a.resource()) {}

    MissingPath(std::allocator_arg_t, Allocator a, const MissingPath& other)
        : MissingPath(std::allocator_arg, a, other.failed_node, other.attempted_path) {}

    MissingPath(const MissingPath&) = delete;
    MissingPath(MissingPath&&) noexcept = default;
};

template <class T>
class EvalResult {
public:
    using allocator_type = Allocator;
    using payload_type = T;

    static EvalResult value(Memory owner, T value) {
        return EvalResult(owner, std::in_place_index<0>, detail::move_in(owner, std::move(value)));
    }

    static EvalResult null(Memory owner) {
        return EvalResult(owner, std::in_place_index<1>, std::monostate{});
    }

    static EvalResult not_loaded(Memory owner, std::string_view node, std::string_view path) {
        return EvalResult(owner,
                          std::in_place_index<2>,
                          MissingPath(std::allocator_arg, Allocator(owner), node, path));
    }

    EvalResult(std::allocator_arg_t, Allocator a, const EvalResult& other)
        requires(detail::domain_copyable<T>)
        : EvalResult(other.is_value()
                         ? value(a.resource(), detail::clone_in(a.resource(), other.get()))
                     : other.is_null() ? null(a.resource())
                                       : not_loaded(a.resource(),
                                                    other.missing().failed_node,
                                                    other.missing().attempted_path)) {}

    EvalResult(std::allocator_arg_t, Allocator a, EvalResult&& other)
        : EvalResult(detail::move_to(a.resource(), std::move(other))) {}

    EvalResult(const EvalResult&) = delete;
    EvalResult(EvalResult&&) noexcept(std::is_nothrow_move_constructible_v<T>) = default;

    bool is_value() const noexcept {
        return data_.index() == 0;
    }

    bool is_null() const noexcept {
        return data_.index() == 1;
    }

    bool is_not_loaded() const noexcept {
        return data_.index() == 2;
    }

    T& get() {
        return std::get<0>(data_);
    }

    const T& get() const {
        return std::get<0>(data_);
    }

    const MissingPath& missing() const {
        return std::get<2>(data_);
    }

    Memory memory_resource() const noexcept {
        return owner_;
    }

    template <class F>
    auto map(F&& transform) && -> EvalResult<std::remove_cvref_t<std::invoke_result_t<F, T>>> {
        using Output = EvalResult<std::remove_cvref_t<std::invoke_result_t<F, T>>>;
        if (is_value()) {
            return Output::value(owner_, std::invoke(std::forward<F>(transform), std::move(get())));
        }
        if (is_null()) {
            return Output::null(owner_);
        }
        return Output::not_loaded(owner_, missing().failed_node, missing().attempted_path);
    }

    template <class F>
    auto and_then(std::string_view field,
                  F&& transform) && -> std::remove_cvref_t<std::invoke_result_t<F, T>> {
        using Output = std::remove_cvref_t<std::invoke_result_t<F, T>>;
        if (is_null()) {
            return Output::null(owner_);
        }
        if (is_not_loaded()) {
            return Output::not_loaded(owner_, missing().failed_node, missing().attempted_path);
        }
        auto result =
            detail::move_in(owner_, std::invoke(std::forward<F>(transform), std::move(get())));
        if (!result.is_not_loaded()) {
            return result;
        }
        const auto& missing = result.missing();
        if (missing.attempted_path == field) {
            return result;
        }
        String path(field, owner_);
        if (!missing.attempted_path.empty()) {
            path += '.';
            path += missing.attempted_path;
        }
        return Output::not_loaded(owner_, missing.failed_node, path);
    }

    template <class Encode>
    Json to_json(Memory destination, Encode&& encode) const {
        if (is_null()) {
            return Json(destination, "Null");
        }
        auto result = detail::model_json(destination);
        if (is_value()) {
            detail::model_put(result, "Value", std::invoke(std::forward<Encode>(encode), get()));
        } else {
            auto path = detail::model_json(destination);
            detail::model_put(path, "failed_node", missing().failed_node);
            detail::model_put(path, "attempted_path", missing().attempted_path);
            detail::model_put(result, "NotLoaded", std::move(path));
        }
        return result;
    }

    template <class Decode>
    static ModelResult<EvalResult> from_json(Memory owner, const Json& input, Decode&& decode) {
        return detail::decode_model<EvalResult>(owner, [&] {
            if (auto* unit = std::get_if<String>(&input.data); unit && *unit == "Null") {
                return null(owner);
            }
            const auto& object = detail::model_object(input, "EvalResult");
            if (object.size() != 1) {
                throw ModelDecodeError{ModelDecodeError::Code::ExpectedObject, "EvalResult"};
            }
            const auto& [name, payload] = *object.begin();
            if (name == "Null" && std::holds_alternative<std::monostate>(payload.data)) {
                return null(owner);
            }
            if (name == "Value") {
                return value(owner, std::invoke(std::forward<Decode>(decode), payload));
            }
            if (name != "NotLoaded") {
                throw ModelDecodeError{ModelDecodeError::Code::ExpectedStruct, "EvalResult"};
            }
            // serde_json's externally tagged struct variants require an object.
            (void)detail::model_object(payload, "NotLoaded");
            detail::ModelReader reader(payload, 2);
            return not_loaded(
                owner,
                detail::model_string(*reader.field("failed_node", 0), "failed_node"),
                detail::model_string(*reader.field("attempted_path", 1), "attempted_path"));
        });
    }

private:
    template <std::size_t Index, class Payload>
    EvalResult(Memory owner, std::in_place_index_t<Index> index, Payload&& payload)
        : owner_(owner), data_(index, std::forward<Payload>(payload)) {}

    Memory owner_;
    std::variant<T, std::monostate, MissingPath> data_;
};

inline void write_debug(DebugWriter& writer, const LoadState& state) {
    if (state.kind() == LoadState::Kind::NotLoaded ||
        state.kind() == LoadState::Kind::FullyLoaded) {
        writer.raw(state.variant_name());
        return;
    }
    writer.tuple(state.variant_name(), [&](auto& out) {
        const bool set = state.kind() == LoadState::Kind::Partial;
        out.sequence(
            state.names(),
            [](auto& inner, const auto& name) {
                inner.text(name);
            },
            set ? "{" : "[",
            set ? "}" : "]");
    });
}

template <class T, class Element>
void write_debug(DebugWriter& writer, const EvalResult<T>& result, Element&& element) {
    if (result.is_value()) {
        writer.tuple("Value", [&](auto& out) {
            std::invoke(element, out, result.get());
        });
    } else if (result.is_null()) {
        writer.raw("Null");
    } else {
        writer.structure("NotLoaded", [&](auto& fields) {
            fields.field("failed_node", [&](auto& out) {
                out.text(result.missing().failed_node);
            });
            fields.field("attempted_path", [&](auto& out) {
                out.text(result.missing().attempted_path);
            });
        });
    }
}

template <class T>
void write_debug(DebugWriter& writer, const EvalResult<T>& result) {
    write_debug(writer, result, [](auto& out, const auto& value) {
        using ::teaql::write_debug;
        write_debug(out, value);
    });
}
}
