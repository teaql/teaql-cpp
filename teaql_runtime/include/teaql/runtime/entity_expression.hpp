#pragma once
#include <teaql/runtime/entity_runtime.hpp>
#include <teaql/runtime/context.hpp>
#include <teaql/core/eval.hpp>
#include <teaql/core/debug.hpp>
#include <teaql/memory/object_borrow.hpp>
#include <charconv>

namespace teaql {
template <class T>
class EntityBorrow {
public:
    explicit EntityBorrow(const T& value)
        : value_(&value), lifetime_(value.root.borrow_lifetime()) {}

    EntityBorrow(T&&) = delete;
    EntityBorrow(const T&&) = delete;

    const T& get() const {
        lifetime_.check();
        return *value_;
    }

    Memory memory_resource() const {
        lifetime_.check();
        return lifetime_.scope.memory_resource();
    }

    const detail::EntityLifetimeAnchor::Borrow& lifetime() const noexcept {
        return lifetime_;
    }

private:
    const T* value_;
    detail::EntityLifetimeAnchor::Borrow lifetime_;
};

template <class T>
class EntityListBorrow {
public:
    template <class Owner>
    EntityListBorrow(const EntityBorrow<Owner>& owner, const SmartList<T>& list)
        : lifetime_(owner.lifetime()), list_(&list), data_(list.data.data()), size_(list.len()),
          loaded_(list.is_loaded) {
        (void)owner.get();
        if (owner.memory_resource() != list.memory_resource()) {
            throw std::invalid_argument("Relation list must share its owner's allocation domain");
        }
    }

    const SmartList<T>& get() const {
        lifetime_.check();
        if (list_->data.data() != data_ || list_->len() != size_ || list_->is_loaded != loaded_) {
            throw ReferenceError(ReferenceError::Kind::Dead, 0, std::source_location::current());
        }
        return *list_;
    }

    Memory memory_resource() const {
        (void)get();
        return lifetime_.scope.memory_resource();
    }

private:
    detail::EntityLifetimeAnchor::Borrow lifetime_;
    const SmartList<T>* list_;
    const T* data_;
    std::size_t size_;
    bool loaded_;
};

// Model relation helpers and E adapters share these loaded/null/borrow rules.
class GeneratedRelationAccess {
public:
    struct FieldLookup {
        const Value* value = nullptr;
        bool alias_present = false;
        bool object_alias = false;
    };

    static Result<FieldLookup, EntityError>
    lookupField(Memory r,
                const CompactRow& row,
                std::string_view entity,
                std::string_view field,
                std::string_view column,
                bool forward_relation,
                std::optional<ArenaBorrow<FieldLayout>> field_layout = std::nullopt) {
        using Output = Result<FieldLookup, EntityError>;
        const FieldLayout* aliases = nullptr;
        std::optional<std::size_t> field_index;
        if (field_layout) {
            aliases = &field_layout->get();
            if (field_layout->memory_resource() != r) {
                throw std::invalid_argument("Field lookup requires metadata bound to its Context");
            }
            field_index = aliases->index(field);
            if (aliases->entity() != entity || !field_index ||
                aliases->index(column) != field_index) {
                return Output::err(r,
                                   EntityError(std::allocator_arg,
                                               Allocator(r),
                                               entity,
                                               "incompatible generated field layout"));
            }
        }
        FieldLookup result;
        for (const auto& [name, value] : row.iter()) {
            if (name == field) {
                result.value = &value;
            } else if (name == column) {
                if (forward_relation) {
                    result.alias_present = true;
                    const auto* object = value.as_object();
                    result.object_alias = object != nullptr;
                    if (object) {
                        auto id = object->find("id");
                        if (id == object->end()) {
                            String message("missing reference id: ", r);
                            message += column;
                            return Output::err(
                                r, EntityError(std::allocator_arg, Allocator(r), entity, message));
                        }
                        result.value = &id->second;
                    } else {
                        result.value = &value;
                    }
                } else {
                    result.value = &value;
                }
            } else if (aliases && aliases->index(name) == field_index) {
                // Native member aliases are scalar slots. Only the physical
                // forward-relation spelling above unwraps a reference object.
                result.value = &value;
            }
        }
        return Output::ok(r, result);
    }

    template <Entity T>
    static Result<std::optional<T>, EntityError> decodeObject(Memory r, const Value* value) {
        using Output = Result<std::optional<T>, EntityError>;
        if (const auto* object = value ? value->as_object() : nullptr) {
            auto decoded =
                T::from_compact_row(r, CompactRow(std::allocator_arg, Allocator(r), *object));
            if (!decoded) {
                return Output::err(r, std::move(decoded).error());
            }
            return Output::ok(r, std::optional<T>(std::move(decoded).value()));
        }
        return Output::ok(r, std::optional<T>{});
    }

    template <class T>
    static eval::EvalResult<EntityBorrow<T>>
    object(Memory resource, std::string_view node, const T* value, bool known_null = false) {
        using Result = eval::EvalResult<EntityBorrow<T>>;
        if (value) {
            if (value->memory_resource() != resource) {
                throw std::invalid_argument("Relation values require an explicit destination copy");
            }
            return Result::value(resource, EntityBorrow<T>(*value));
        }
        if (known_null) {
            return Result::null(resource);
        }
        return Result::not_loaded(resource, node, node);
    }

    template <class Owner, class T>
    static eval::EvalResult<EntityListBorrow<T>>
    list(const Owner& owner, const SmartList<T>& value, std::string_view node) {
        using Result = eval::EvalResult<EntityListBorrow<T>>;
        auto resource = owner.memory_resource();
        bool loaded = value.is_loaded;
        if (!loaded) {
            if constexpr (requires {
                              Owner::ENTITY_NAME;
                              owner.root.template relation_list<T>(
                                  Owner::ENTITY_NAME, owner.id(), node);
                          }) {
                // An explicitly installed graph view determines relation availability.
                // Its stored list flag remains part of the value, not the selection state.
                auto view =
                    owner.root.template relation_list<T>(Owner::ENTITY_NAME, owner.id(), node);
                loaded = view.value() == &value;
            }
        }
        if (!loaded) {
            return Result::not_loaded(resource, node, node);
        }
        return Result::value(resource, EntityListBorrow<T>(EntityBorrow<Owner>(owner), value));
    }
};

// A diagnostic belongs to the expression's Context, including every nested string.
struct ExpressionDiagnostic {
    using allocator_type = Allocator;
    String root, failed_node, attempted_path, original_expr_with_broken_point;
    Vector<String> access_path, missing_preload;
    String suggested_fix, human_message;
    std::optional<String> nested_fix;
    StaticString entry_name;
    static constexpr std::string_view severity = "error";

    static String method(Memory r, std::string_view prefix, std::string_view name) {
        String result(prefix, r);
        bool uppercase = !prefix.empty();
        for (char c : name) {
            if (c == '_') {
                uppercase = true;
            } else {
                result += uppercase && c >= 'a' && c <= 'z' ? char(c - 'a' + 'A') : c;
                uppercase = false;
            }
        }
        return result;
    }

    ExpressionDiagnostic(std::allocator_arg_t,
                         Allocator a,
                         std::string_view root_desc,
                         std::string_view node,
                         std::string_view path,
                         StaticString native_entry = "")
        : root(root_desc, a.resource()), failed_node(node, a.resource()),
          attempted_path(path, a.resource()), original_expr_with_broken_point(a.resource()),
          access_path(a.resource()), missing_preload(a.resource()), suggested_fix(a.resource()),
          human_message(a.resource()), entry_name(native_entry) {
        auto r = a.resource();
        std::size_t begin = 0;
        while (true) {
            auto end = path.find('.', begin);
            access_path.emplace_back(
                path.substr(begin, end == path.npos ? path.npos : end - begin));
            if (end == path.npos) {
                break;
            }
            begin = end + 1;
        }
        auto found = std::find(access_path.begin(), access_path.end(), node);
        auto break_index =
            found == access_path.end() ? std::size_t(0) : std::size_t(found - access_path.begin());
        suggested_fix = method(r, "select", node);
        suggested_fix += "()";
        missing_preload.emplace_back(node);
        if (break_index + 1 < access_path.size()) {
            nested_fix.emplace(method(r, "select", node));
            *nested_fix += "With(";
            std::size_t closes = 1;
            for (std::size_t i = break_index + 1; i < access_path.size(); ++i) {
                *nested_fix += "<generated Q entry for ";
                *nested_fix += access_path[i - 1];
                *nested_fix += ">.";
                *nested_fix += method(r, "select", access_path[i]);
                if (i + 1 == access_path.size()) {
                    *nested_fix += "()";
                } else {
                    *nested_fix += "With(";
                    ++closes;
                }
            }
            nested_fix->append(closes, ')');
        }
        auto separator = root_desc.find('(');
        auto root_name = root_desc.substr(0, separator);
        auto explicit_entry = entry_name.view();
        auto entry = explicit_entry.empty() ? method(r, "", root_name) : String(explicit_entry, r);
        // Source entry spelling is adapted to the generated C++ lowerCamelCase API.
        if (explicit_entry.empty() && !entry.empty() && entry.front() >= 'A' &&
            entry.front() <= 'Z') {
            entry.front() = char(entry.front() - 'A' + 'a');
        }
        original_expr_with_broken_point = "E::";
        original_expr_with_broken_point += entry;
        original_expr_with_broken_point += '(';
        if (separator == root_desc.npos) {
            original_expr_with_broken_point += ')';
        } else {
            auto arguments = root_desc.substr(separator + 1);
            // The original renderer takes split('(').nth(1), not the entire tail.
            original_expr_with_broken_point += arguments.substr(0, arguments.find('('));
        }
        for (const auto& part : access_path) {
            original_expr_with_broken_point += '.';
            original_expr_with_broken_point += method(r, "get", part);
            original_expr_with_broken_point += "()";
            if (part == node) {
                original_expr_with_broken_point += "<broken>";
            }
        }
        human_message = "Access to ";
        human_message += root_name;
        human_message += '.';
        human_message += path;
        human_message += " requires preloading. Add ";
        human_message += suggested_fix;
        if (nested_fix) {
            human_message += " or the complete nested selection ";
            human_message += *nested_fix;
        }
        human_message += " to the query.";
    }

    ExpressionDiagnostic(std::allocator_arg_t, Allocator a, const ExpressionDiagnostic& other)
        : ExpressionDiagnostic(std::allocator_arg,
                               a,
                               other.root,
                               other.failed_node,
                               other.attempted_path,
                               other.entry_name) {}

    Memory memory_resource() const {
        return root.get_allocator().resource();
    }

    Json to_json(Memory destination) const {
        auto result = detail::model_json(destination);
        detail::model_put(result, "root", root);
        detail::model_put(
            result, "original_expr_with_broken_point", original_expr_with_broken_point);
        detail::model_put(result, "break_point", failed_node);
        Json::Array path(destination), preload(destination);
        for (const auto& part : access_path) {
            path.emplace_back(part);
        }
        for (const auto& part : missing_preload) {
            preload.emplace_back(part);
        }
        detail::model_put(result, "access_path", path);
        detail::model_put(result, "missing_preload", preload);
        detail::model_put(result, "suggested_fix", suggested_fix);
        if (nested_fix) {
            detail::model_put(result, "nested_fix", *nested_fix);
        } else {
            detail::model_put(result, "nested_fix", Json(destination));
        }
        detail::model_put(result, "severity", severity);
        detail::model_put(result, "human_message", human_message);
        return result;
    }
};

class ExpressionNotLoadedError final : public std::exception {
public:
    using allocator_type = Allocator;

    ExpressionNotLoadedError(Memory r,
                             std::string_view root,
                             const eval::MissingPath& missing,
                             StaticString entry_name = "")
        : diagnostic_(std::allocator_arg,
                      Allocator(r),
                      detail::resource_arena(r).make<ExpressionDiagnostic>(
                          root, missing.failed_node, missing.attempted_path, entry_name)) {}

    ExpressionNotLoadedError(std::allocator_arg_t,
                             Allocator a,
                             const ExpressionNotLoadedError& other)
        : diagnostic_(
              std::allocator_arg,
              a,
              detail::resource_arena(a.resource()).make<ExpressionDiagnostic>(other.diagnostic())) {
    }

    const char* what() const noexcept override {
        return "Expression requires preloading";
    }

    const ExpressionDiagnostic& diagnostic() const {
        return diagnostic_.get();
    }

private:
    ArenaBorrow<ExpressionDiagnostic> diagnostic_;
};

namespace detail {
struct ExpressionRootProvenance {
    using allocator_type = Allocator;
    String description;
    StaticString entry_name;

    ExpressionRootProvenance(std::allocator_arg_t,
                             Allocator a,
                             std::string_view root,
                             StaticString native_entry = "")
        : description(root, a.resource()), entry_name(native_entry) {}

    Memory memory_resource() const {
        return description.get_allocator().resource();
    }
};

template <class T>
struct ExpressionState {
    using allocator_type = Allocator;
    eval::EvalResult<T> result;

    ExpressionState(std::allocator_arg_t, Allocator a, eval::EvalResult<T> value)
        : result(std::allocator_arg, a, std::move(value)) {}
};
}

// Immutable snapshots/checked borrows live in Context state. The ordinary C++
// wrapper has no owned allocator-aware payload and is safe to destroy after User.
template <class T>
    requires(!std::is_pointer_v<T>)
class RuntimeExpression {
public:
    using payload_type = T;

    static RuntimeExpression value(Memory resource, T value) {
        if constexpr (requires { value.memory_resource(); }) {
            if (value.memory_resource() != resource) {
                throw std::invalid_argument(
                    "Expression values require an explicit destination copy");
            }
        }
        return from_result(eval::EvalResult<T>::value(resource, std::move(value)));
    }

    static RuntimeExpression null(Memory resource) {
        return from_result(eval::EvalResult<T>::null(resource));
    }

    static RuntimeExpression
    not_loaded(Memory resource, std::string_view node, std::string_view path) {
        return from_result(eval::EvalResult<T>::not_loaded(resource, node, path));
    }

    static RuntimeExpression from_result(eval::EvalResult<T> value,
                                         std::string_view root_desc = "Unknown()") {
        return from_result(std::move(value), root_desc, StaticString{""});
    }

    static RuntimeExpression
    from_result(eval::EvalResult<T> value, std::string_view root_desc, StaticString entry_name) {
        ExpressionScope scope(value.memory_resource());
        if constexpr (requires { value.get().memory_resource(); }) {
            if (value.is_value() && value.get().memory_resource() != value.memory_resource()) {
                throw std::invalid_argument("Expression payload cannot borrow another domain");
            }
        }
        auto state =
            scope.get().template make_identified<detail::ExpressionState<T>>(std::move(value));
        auto& root =
            scope.get().template make<detail::ExpressionRootProvenance>(root_desc, entry_name);
        return RuntimeExpression(scope,
                                 state.slot,
                                 ArenaBorrow<detail::ExpressionRootProvenance>(
                                     std::allocator_arg, Allocator(scope.memory_resource()), root));
    }

    Memory memory_resource() const {
        (void)result();
        return scope_.memory_resource();
    }

    RuntimeExpression copyTo(UserContext& destination) const
        requires(!requires(const T& value) { value.get(); })
    {
        return from_result(detail::clone_in(destination.memory_resource(), result()),
                           rootDescription(),
                           root_.get().entry_name);
    }

    std::optional<T> eval() const {
        const auto& current = result();
        require_loaded(current);
        if (current.is_null()) {
            return {};
        }
        check_payload(current.get());
        return detail::clone_in(memory_resource(), current.get());
    }

    T unwrap() const {
        auto current = eval();
        if (!current) {
            throw std::logic_error("Expression value is legitimately null");
        }
        return std::move(*current);
    }

    bool isNull() const {
        const auto& current = result();
        require_loaded(current);
        if (current.is_value()) {
            check_payload(current.get());
        }
        return current.is_null();
    }

    bool isNotNull() const {
        return !isNull();
    }

    T orIfNull(T fallback) const {
        auto current = eval();
        return current ? std::move(*current)
                       : detail::move_in(memory_resource(), std::move(fallback));
    }

    template <class F>
    T orElseIfNull(F&& fallback) const {
        auto current = eval();
        return current ? std::move(*current)
                       : detail::move_in(memory_resource(), std::invoke(std::forward<F>(fallback)));
    }

    T orDefaultIfNull() const
        requires(std::default_initializable<T> ||
                 std::is_constructible_v<T, std::allocator_arg_t, Allocator> ||
                 std::is_constructible_v<T, Allocator>)
    {
        if constexpr (std::uses_allocator_v<T, Allocator>) {
            return orElseIfNull([&] {
                return std::make_obj_using_allocator<T>(Allocator(memory_resource()));
            });
        } else {
            return orElseIfNull([] {
                return T{};
            });
        }
    }

    template <class F>
    auto
    map(F&& function) const -> RuntimeExpression<std::remove_cvref_t<std::invoke_result_t<F, T>>> {
        using U = std::remove_cvref_t<std::invoke_result_t<F, T>>;
        const auto& current = result();
        auto r = memory_resource();
        if (current.is_null()) {
            return RuntimeExpression<U>::from_result(
                eval::EvalResult<U>::null(r), rootDescription(), root_.get().entry_name);
        }
        if (current.is_not_loaded()) {
            return RuntimeExpression<U>::from_result(
                eval::EvalResult<U>::not_loaded(
                    r, current.missing().failed_node, current.missing().attempted_path),
                rootDescription(),
                root_.get().entry_name);
        }
        check_payload(current.get());
        return RuntimeExpression<U>::from_result(
            eval::EvalResult<U>::value(
                r, std::invoke(std::forward<F>(function), detail::clone_in(r, current.get()))),
            rootDescription(),
            root_.get().entry_name);
    }

    template <class F>
    auto andThen(std::string_view field,
                 F&& function) const -> std::remove_cvref_t<std::invoke_result_t<F, T>> {
        using Output = std::remove_cvref_t<std::invoke_result_t<F, T>>;
        const auto& current = result();
        auto r = memory_resource();
        if (current.is_null()) {
            return Output::from_result(eval::EvalResult<typename Output::payload_type>::null(r),
                                       rootDescription(),
                                       root_.get().entry_name);
        }
        if (current.is_not_loaded()) {
            return Output::from_result(
                eval::EvalResult<typename Output::payload_type>::not_loaded(
                    r, current.missing().failed_node, current.missing().attempted_path),
                rootDescription(),
                root_.get().entry_name);
        }
        check_payload(current.get());
        auto output = std::invoke(std::forward<F>(function), detail::clone_in(r, current.get()));
        scope_.require_same(output.scope());
        const auto& next = output.result();
        if (!next.is_not_loaded() || next.missing().attempted_path == field) {
            return output.with_root(root_);
        }
        String path(field, r);
        if (!next.missing().attempted_path.empty()) {
            path += '.';
            path += next.missing().attempted_path;
        }
        return Output::from_result(eval::EvalResult<typename Output::payload_type>::not_loaded(
                                       r, next.missing().failed_node, path),
                                   rootDescription(),
                                   root_.get().entry_name);
    }

    const ExpressionScope& scope() const noexcept {
        return scope_;
    }

    std::string_view rootDescription() const {
        (void)result();
        return root_.get().description;
    }

    const eval::EvalResult<T>& result() const {
        return state().result;
    }

private:
    const detail::ExpressionState<T>& state() const {
        auto& arena = scope_.get();
        auto identity = arena.allocation_identity(slot_);
        auto& state = *static_cast<detail::ExpressionState<T>*>(
            arena.resolve(slot_, typeid(detail::ExpressionState<T>), identity));
        return state;
    }

    template <class U>
        requires(!std::is_pointer_v<U>)
    friend class RuntimeExpression;

    RuntimeExpression(ExpressionScope scope,
                      std::uint32_t slot,
                      ArenaBorrow<detail::ExpressionRootProvenance> root)
        : scope_(scope), slot_(slot), root_(root) {}

    RuntimeExpression with_root(ArenaBorrow<detail::ExpressionRootProvenance> root) const {
        ExpressionScope(root.memory_resource()).require_same(scope_);
        return RuntimeExpression(scope_, slot_, root);
    }

    static void check_payload(const T& value) {
        if constexpr (requires { value.get(); }) {
            (void)value.get();
        }
    }

    void require_loaded(const eval::EvalResult<T>& value) const {
        if (value.is_not_loaded()) {
            throw ExpressionNotLoadedError(value.memory_resource(),
                                           rootDescription(),
                                           value.missing(),
                                           root_.get().entry_name);
        }
    }

    ExpressionScope scope_;
    std::uint32_t slot_;
    ArenaBorrow<detail::ExpressionRootProvenance> root_;
};

template <class T>
RuntimeExpression<EntityBorrow<T>> entity_expression(const T& value, StaticString entry_name) {
    auto r = value.memory_resource();
    String root("Unknown()", r);
    if constexpr (requires {
                      T::ENTITY_NAME;
                      value.id();
                  }) {
        root = T::ENTITY_NAME;
        root += "(id=";
        char digits[20];
        auto converted = std::to_chars(std::begin(digits), std::end(digits), value.id());
        root.append(digits, converted.ptr);
        root += ')';
    }
    return RuntimeExpression<EntityBorrow<T>>::from_result(
        eval::EvalResult<EntityBorrow<T>>::value(r, EntityBorrow<T>(value)), root, entry_name);
}

template <class T>
RuntimeExpression<EntityBorrow<T>> entity_expression(const T& value) {
    return entity_expression(value, StaticString{""});
}

template <class T>
    requires(!std::is_lvalue_reference_v<T>)
auto entity_expression(T&&) = delete;

template <class T>
    requires(!std::is_lvalue_reference_v<T>)
auto entity_expression(T&&, StaticString) = delete;
}
