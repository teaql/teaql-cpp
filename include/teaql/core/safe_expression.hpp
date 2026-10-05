#pragma once
#include <teaql/core/entity.hpp>
#include <teaql/core/list.hpp>
#include <teaql/memory/function.hpp>
#include <functional>

namespace teaql {
inline bool teaql_is_empty(std::string_view value) {
    return value.empty();
}

inline bool teaql_is_empty(const String& value) {
    return value.empty();
}

template <class T>
bool teaql_is_empty(const Vector<T>& value) {
    return value.empty();
}

template <class T>
bool teaql_is_empty(const SmartList<T>& value) {
    return value.is_empty();
}

template <class T>
bool teaql_is_empty(const std::optional<T>& value) {
    return !value;
}

inline bool teaql_is_empty(const Value& value) {
    return std::visit(
        [](const auto& item) -> bool {
            using T = std::decay_t<decltype(item)>;
            if constexpr (std::same_as<T, std::monostate>)
                return true;
            else if constexpr (std::same_as<T, String> || std::same_as<T, Value::List> ||
                               std::same_as<T, Value::Object>)
                return item.empty();
            else if constexpr (std::same_as<T, Json>)
                return std::visit(
                    [](const auto& v) -> bool {
                        using J = std::decay_t<decltype(v)>;
                        if constexpr (std::same_as<J, std::monostate>)
                            return true;
                        else if constexpr (std::same_as<J, String> ||
                                           std::same_as<J, Json::Array> ||
                                           std::same_as<J, Json::Object>)
                            return v.empty();
                        else
                            return false;
                    },
                    item.data);
            else
                return false; // In particular TypedNull is NOT empty in Rust 5.0.5.
        },
        value.data);
}
template <class T>
concept TeaqlEmpty = requires(const T& value) {
    { teaql_is_empty(value) } -> std::same_as<bool>;
};

namespace detail {
template <class R>
struct ExpressionRoot {
    using allocator_type = Allocator;
    R value;

    ExpressionRoot(std::allocator_arg_t, Allocator a, const R& root)
        : value(clone_in(a.resource(), root)) {}
};

template <class F, class... Args>
decltype(auto) invoke_in(const F& function, Memory r, Args&&... args) {
    if constexpr (std::is_invocable_v<const F&, Args..., Memory>)
        return std::invoke(function, std::forward<Args>(args)..., r);
    else
        return std::invoke(function, std::forward<Args>(args)...);
}
template <class F, class... Args>
using InvokeResult =
    decltype(invoke_in(std::declval<const F&>(), std::declval<Memory>(), std::declval<Args>()...));
template <class T>
struct OptionalValue;

template <class T>
struct OptionalValue<std::optional<T>> {
    using type = T;
};
template <class T>
struct ListValue;

template <class T>
struct ListValue<SmartList<T>> {
    using type = T;
};

template <class R, class T, class F>
struct ExpressionEvaluator {
    using allocator_type = Allocator;
    Memory resource;
    F function;

    template <class G>
        requires std::same_as<std::remove_cvref_t<G>, F>
    ExpressionEvaluator(std::allocator_arg_t, Allocator a, G&& f)
        : resource(a.resource()),
          function(std::make_obj_using_allocator<F>(a, std::forward<G>(f))) {
        static_assert(std::uses_allocator_v<F, Allocator> || std::is_trivially_destructible_v<F>,
                      "Evaluator captures must propagate their allocator");
    }

    ExpressionEvaluator(std::allocator_arg_t, Allocator a, const ExpressionEvaluator& other)
        : resource(a.resource()),
          function(copy_callable(a.resource(), other.resource, other.function)) {}

    std::optional<T> operator()(const R& root, Memory r) const {
        return clone_in(r, invoke_in(function, r, root));
    }
};

template <class R, class T, class U, class F, bool Optional>
struct ExpressionMapper {
    using allocator_type = Allocator;
    ContextFunction<std::optional<T>(const R&, Memory)> previous;
    F mapper;

    ExpressionMapper(std::allocator_arg_t,
                     Allocator a,
                     const decltype(previous)& previous,
                     F mapper)
        : previous(std::allocator_arg, a, previous),
          mapper(std::make_obj_using_allocator<F>(a, std::move(mapper))) {
        static_assert(std::uses_allocator_v<F, Allocator> || std::is_trivially_destructible_v<F>,
                      "Mapper captures must propagate their allocator");
    }

    ExpressionMapper(std::allocator_arg_t, Allocator a, const ExpressionMapper& other)
        : previous(std::allocator_arg, a, other.previous),
          mapper(copy_callable(a.resource(), other.previous.memory_resource(), other.mapper)) {}

    std::optional<U> operator()(const R& root, Memory r) const {
        auto input = previous(root, r);
        if (!input)
            return {};
        if constexpr (Optional)
            return clone_in(r, invoke_in(mapper, r, std::move(*input)));
        else
            return clone_in(r, invoke_in(mapper, r, std::move(*input)));
    }
};

template <class T>
struct EntityIdMapper {
    using allocator_type = Allocator;
    std::uint64_t id;

    EntityIdMapper(std::allocator_arg_t, Allocator, std::uint64_t id) : id(id) {}

    EntityIdMapper(std::allocator_arg_t, Allocator, const EntityIdMapper& other) : id(other.id) {}

    T operator()(T entity) const {
        entity.set_id(id);
        return entity;
    }
};

template <class T>
struct ListGetMapper {
    using allocator_type = Allocator;
    std::size_t index;

    ListGetMapper(std::allocator_arg_t, Allocator, std::size_t index) : index(index) {}

    ListGetMapper(std::allocator_arg_t, Allocator, const ListGetMapper& other)
        : index(other.index) {}

    std::optional<typename ListValue<T>::type> operator()(T list, Memory r) const {
        if (const auto* value = list.get(index))
            return clone_in(r, *value);
        return {};
    }
};
}

// Root, evaluator and every mapping step are Context-owned. The expression itself
// is a borrow, just like other Context results. Same-domain clones share the graph;
// explicit cross-domain copies deep-copy root and allocator-aware callback state.
template <class R, class T>
class SafeExpression {
public:
    using allocator_type = Allocator;

    template <class F>
    SafeExpression(std::allocator_arg_t, Allocator a, const R& root, F evaluator)
        : resource_(a.resource()),
          root_(&detail::context_arena(resource_).make<detail::ExpressionRoot<R>>(root).value),
          evaluator_(
              std::allocator_arg,
              a,
              detail::ExpressionEvaluator<R, T, F>(std::allocator_arg, a, std::move(evaluator))) {}

    SafeExpression(std::allocator_arg_t, Allocator a, const SafeExpression& other)
        : resource_(a.resource()), root_(a.resource() == other.resource_
                                             ? other.root_
                                             : &detail::context_arena(resource_)
                                                    .make<detail::ExpressionRoot<R>>(*other.root_)
                                                    .value),
          evaluator_(std::allocator_arg, a, other.evaluator_) {}

    SafeExpression(std::allocator_arg_t, Allocator a, SafeExpression&& other)
        : SafeExpression(std::allocator_arg, a, std::as_const(other)) {}

    SafeExpression(const SafeExpression&) =
        default; // Same-domain borrow, like cloning Rust's Arc handles.
    SafeExpression& operator=(const SafeExpression&) = delete;

    Memory memory_resource() const {
        return resource_;
    }

    static SafeExpression value(Memory r, const R& root)
        requires std::same_as<R, T>
    {
        return SafeExpression(std::allocator_arg,
                              Allocator(r),
                              root,
                              [](const R& input, Memory target) -> std::optional<R> {
                                  return detail::clone_in(target, input);
                              });
    }

    const R& root() const
        requires std::same_as<R, T>
    {
        return *root_;
    }

    std::optional<T> eval() const {
        return evaluator_(*root_, resource_);
    }

    std::optional<T> eval_with(const R& root) const {
        return evaluator_(root, resource_);
    }

    template <class F>
    auto apply(F mapper) const {
        using U = std::remove_cvref_t<detail::InvokeResult<F, T>>;
        return map<U, F, false>(std::move(mapper));
    }

    template <class F>
    auto apply_optional(F mapper) const {
        using U =
            typename detail::OptionalValue<std::remove_cvref_t<detail::InvokeResult<F, T>>>::type;
        return map<U, F, true>(std::move(mapper));
    }

    T or_else(T fallback) const {
        auto result = eval();
        return result ? std::move(*result) : detail::clone_in(resource_, fallback);
    }

    template <class F>
    T or_else_with(F&& fallback) const {
        auto result = eval();
        return result ? std::move(*result)
                      : detail::clone_in(resource_, std::invoke(std::forward<F>(fallback)));
    }

    template <class F>
    auto or_else_throw(F&& error) const {
        using E = std::remove_cvref_t<std::invoke_result_t<F>>;
        using Outcome = Result<T, E>;
        auto result = eval();
        if (result)
            return Outcome::ok(resource_, std::move(*result));
        return Outcome::err(resource_, std::invoke(std::forward<F>(error)));
    }

    bool is_null() const {
        return !eval();
    }

    bool is_not_null() const {
        return bool(eval());
    }

    bool is_empty() const
        requires TeaqlEmpty<T>
    {
        auto result = eval();
        return !result || teaql_is_empty(*result);
    }

    bool is_not_empty() const
        requires TeaqlEmpty<T>
    {
        return !is_empty();
    }

    template <class F>
    void when_is_null(F&& function) const {
        if (is_null())
            std::invoke(std::forward<F>(function));
    }

    template <class F>
    void when_is_not_null(F&& consumer) const {
        if (auto v = eval())
            std::invoke(std::forward<F>(consumer), std::move(*v));
    }

    template <class F>
    void when_is_empty(F&& function) const
        requires TeaqlEmpty<T>
    {
        if (is_empty())
            std::invoke(std::forward<F>(function));
    }

    template <class F>
    void when_not_empty(F&& consumer) const
        requires TeaqlEmpty<T>
    {
        if (auto v = eval(); v && !teaql_is_empty(*v))
            std::invoke(std::forward<F>(consumer), std::move(*v));
    }

    auto entity_id() const
        requires BaseEntity<T>
    {
        return apply([](T entity) {
            return entity.id();
        });
    }

    auto entity_version() const
        requires BaseEntity<T>
    {
        return apply([](T entity) {
            return entity.version_value();
        });
    }

    auto update_entity_id(std::uint64_t id) const
        requires BaseEntity<T>
    {
        return apply(detail::EntityIdMapper<T>(std::allocator_arg, Allocator(resource_), id));
    }

    auto size() const
        requires requires { typename detail::ListValue<T>::type; }
    {
        return apply([](T list) {
            return list.len();
        });
    }

    auto first() const
        requires requires { typename detail::ListValue<T>::type; }
    {
        return get(0);
    }

    auto get(std::size_t index) const
        requires requires { typename detail::ListValue<T>::type; }
    {
        return apply_optional(
            detail::ListGetMapper<T>(std::allocator_arg, Allocator(resource_), index));
    }

private:
    template <class, class>
    friend class SafeExpression;

    template <class U, class F, bool Optional>
    auto map(F mapper) const {
        using State = detail::ExpressionMapper<R, T, U, F, Optional>;
        auto a = Allocator(resource_);
        ContextFunction<std::optional<U>(const R&, Memory)> next(
            std::allocator_arg, a, State(std::allocator_arg, a, evaluator_, std::move(mapper)));
        return SafeExpression<R, U>(resource_, root_, next);
    }

    SafeExpression(Memory r,
                   const R* root,
                   ContextFunction<std::optional<T>(const R&, Memory)> evaluator)
        : resource_(r), root_(root), evaluator_(evaluator) {}

    Memory resource_;
    const R* root_;
    ContextFunction<std::optional<T>(const R&, Memory)> evaluator_;
};
}
