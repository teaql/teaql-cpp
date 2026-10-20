#define main constant_variant_baseline_main
#include "generated_constant_variants_probe.cpp"
#undef main
#include <teaql/core/mutation_debug.hpp>
#include <teaql/core/query_debug.hpp>
#include <teaql/runtime/checker_model_debug.hpp>

struct BindingSettings {
    using allocator_type = Allocator;
    String tag;
    Vector<String> payload;
    std::int64_t delta;
    std::uint64_t& destroyed;

    BindingSettings(std::allocator_arg_t,
                    Allocator a,
                    std::string_view tag,
                    std::int64_t delta,
                    std::uint64_t& destroyed)
        : tag(tag, a.resource()), payload(a.resource()), delta(delta), destroyed(destroyed) {
        for (auto suffix : {"-first", "-second"}) {
            String value(this->tag, a.resource());
            value += suffix;
            payload.emplace_back(std::move(value));
        }
    }

    ~BindingSettings() {
        ++destroyed;
    }
};

struct OtherBindingSettings {
    using allocator_type = Allocator;
    std::int64_t value;
    std::uint64_t& destroyed;

    OtherBindingSettings(std::allocator_arg_t,
                         Allocator,
                         std::int64_t value,
                         std::uint64_t& destroyed)
        : value(value), destroyed(destroyed) {}

    ~OtherBindingSettings() {
        ++destroyed;
    }
};

static Json settings_view(Memory r, const BindingSettings* settings) {
    if (!settings) {
        return Json(r);
    }
    Json::Object out(r);
    out.emplace("tag", Json(r, settings->tag));
    out.emplace("delta", Json(r, settings->delta));
    Json::Array payload(r);
    for (const auto& item : settings->payload) {
        payload.emplace_back(item);
    }
    out.emplace("payload", Json(r, payload));
    return Json(r, out);
}

struct BindingLogic final : cpp_constants_numeric::PriorityCheckerLogic {
    using allocator_type = Allocator;
    bool named;
    String key;
    Json::Array& hooks;

    BindingLogic(
        std::allocator_arg_t, Allocator a, bool named, std::string_view key, Json::Array& hooks)
        : PriorityCheckerLogic(std::allocator_arg, a), named(named), key(key, a.resource()),
          hooks(hooks) {}

    void checkAndFixPriority(const UserContext& user_context,
                             cpp_constants_numeric::Priority& entity,
                             CheckObjectStatus status,
                             const ObjectLocation& location,
                             CheckResults& results) const override {
        auto required = named ? user_context.require_named_resource<BindingSettings>(key)
                              : user_context.require_resource<BindingSettings>();
        auto r = hooks.get_allocator().resource();
        Json::Object hook(r);
        hook.emplace("status", Json(r, debug_string(r, status)));
        hook.emplace("before", Json(r, entity.rank()));
        hook.emplace("resource", settings_view(r, required ? &required.value().get() : nullptr));
        if (required) {
            entity.updateRank(entity.rank() + required.value().get().delta);
        } else {
            ObjectLocation path(
                std::allocator_arg, Allocator(results.get_allocator().resource()), location);
            results.push_back(CheckResult::required(results.get_allocator().resource(),
                                                    std::move(path).member("rank")));
        }
        hook.emplace("after", Json(r, entity.rank()));
        hooks.emplace_back(hook);
    }
};

static Json binding_observe(GlobalContext& global, const Json& input) {
    namespace domain = cpp_constants_numeric;
    auto r = global.memory_resource();
    auto& destroyed = global.make<std::uint64_t>(0);
    auto& hooks = global.make<Json::Array>();
    Json::Array semantic(r), ownership(r);
    std::uint64_t created_total = 0;
    auto key = text(field(input, "key"));
    bool named = text(field(input, "lookup")) == "named";
    for (std::uint64_t round = 0; round < std::get<std::uint64_t>(field(input, "rounds").data);
         ++round) {
        auto user_context_owner = global.make_user_context();
        auto& user_context = *user_context_owner;
        auto mode = text(field(input, "module"));
        auto& module = mode == "plain"      ? domain::module(user_context)
                       : mode == "checker"  ? domain::moduleWithCheckers(user_context)
                       : mode == "behavior" ? domain::moduleWithBehaviors(user_context)
                                            : domain::moduleWithBehaviorsAndCheckers(user_context);
        auto& logic = user_context.make<BindingLogic>(named, key, hooks);
        auto& checker = user_context.make<domain::PriorityChecker>(logic);
        auto& adapter =
            user_context.make<TypedEntityChecker<domain::Priority, domain::PriorityChecker>>(
                user_context, checker);
        module.checker(user_context, adapter);
        module.apply_to(user_context);
        user_context.with_active_root("Platform", 1);
        std::optional<ResourceBorrow<BindingSettings>> escaped;
        Json::Array steps(r), drops(r);
        std::uint64_t sequence = 0;
        for (const auto& action : std::get<Json::Array>(field(input, "actions").data)) {
            auto name = text(action);
            if (name == "typed" || name == "named") {
                String tag(text(field(input, "tag")), r);
                tag += "-";
                tag += std::to_string(sequence);
                auto delta =
                    static_cast<std::int64_t>(std::get<std::uint64_t>(field(input, "delta").data)) *
                    static_cast<std::int64_t>(sequence + 1);
                auto& settings = user_context.make<BindingSettings>(tag, delta, destroyed);
                ++created_total;
                if (name == "typed") {
                    user_context.insert_resource(user_context, settings);
                    if (!escaped) {
                        escaped.emplace(
                            std::move(user_context.require_resource<BindingSettings>()).value());
                    }
                } else {
                    user_context.insert_named_resource(key, user_context, settings);
                }
            } else if (name == "other") {
                auto& other = user_context.make<OtherBindingSettings>(
                    static_cast<std::int64_t>(sequence), destroyed);
                ++created_total;
                user_context.insert_named_resource(key, user_context, other);
            } else if (name != "check") {
                throw std::runtime_error("Unknown resource binding action");
            }
            Json::Object step(r);
            step.emplace("action", Json(r, name));
            step.emplace("typed", settings_view(r, user_context.get_resource<BindingSettings>()));
            step.emplace("named",
                         settings_view(r, user_context.get_named_resource<BindingSettings>(key)));
            auto* other = user_context.get_named_resource<OtherBindingSettings>(key);
            step.emplace("other", other ? Json(r, other->value) : Json(r));
            auto required_named = user_context.require_named_resource<BindingSettings>(key);
            step.emplace("named_error",
                         required_named ? Json(r) : Json(r, required_named.error().to_string(r)));
            auto required_typed = user_context.require_resource<BindingSettings>();
            step.emplace("typed_missing", Json(r, !required_typed));
            if (!required_typed) {
                std::cerr << required_typed.error().to_string(r) << '\n';
            }
            EntityValues values(std::allocator_arg, Allocator(user_context.memory_resource()));
            auto add = [&](std::string_view name, auto&& value) {
                values.fields.emplace(
                    String(name, user_context.memory_resource()),
                    Value(user_context.memory_resource(), std::forward<decltype(value)>(value)));
            };
            add("id", std::uint64_t(7));
            add("version", std::int64_t(0));
            add("platform_id", std::uint64_t(1));
            add("name", "Fixture");
            add("code", "FIXTURE");
            add("rank", std::int64_t(10));
            add("weight", 1.0);
            add("ratio", 1.0f);
            add("cost", Decimal::from_i64(0));
            add("day", Date{});
            add("note", Value(user_context.memory_resource()));
            add("optional_rank", Value(user_context.memory_resource()));
            mark_entity_status(values, CheckObjectStatus::Create);
            auto checked = user_context.check_and_fix_values("Priority", values);
            step.emplace("checker_error",
                         checked ? Json(r) : Json(r, checked.error().to_string(r)));
            step.emplace("rank", Json(r, std::get<std::int64_t>(values.fields.at("rank").data)));
            steps.emplace_back(step);
            drops.emplace_back(destroyed);
            ++sequence;
        }
        bool old_borrow_live = escaped && !escaped->get().tag.empty();
        auto captured = bool(escaped);
        user_context_owner.reset();
        bool expired_rejected = !captured, expired_copy_rejected = !captured;
        if (escaped) {
            try {
                (void)escaped->get();
            } catch (const ReferenceError& error) {
                expired_rejected = error.kind() == ReferenceError::Kind::Closed;
            }
            auto next_user_context = global.make_user_context();
            try {
                ResourceBorrow<BindingSettings> copy(
                    std::allocator_arg, Allocator(next_user_context->memory_resource()), *escaped);
                (void)copy;
            } catch (const ReferenceError& error) {
                expired_copy_rejected = error.kind() == ReferenceError::Kind::Closed;
            }
        }
        Json::Object outcome(r);
        outcome.emplace("steps", Json(r, steps));
        outcome.emplace("created", Json(r, created_total));
        outcome.emplace("destroyed_after_close", Json(r, destroyed));
        semantic.emplace_back(outcome);
        Json::Object lifetime(r);
        lifetime.emplace("during", Json(r, drops));
        lifetime.emplace("captured", Json(r, captured));
        lifetime.emplace("old_borrow_live_before_close", Json(r, old_borrow_live));
        lifetime.emplace("expired_rejected", Json(r, expired_rejected));
        lifetime.emplace("expired_copy_rejected", Json(r, expired_copy_rejected));
        ownership.emplace_back(lifetime);
    }
    Json::Object out(r), semantic_out(r);
    semantic_out.emplace("rounds", Json(r, semantic));
    semantic_out.emplace("hooks", Json(r, hooks));
    out.emplace("semantic", Json(r, semantic_out));
    out.emplace("ownership", Json(r, ownership));
    return Json(r, out);
}

int main() {
    auto* previous = std::pmr::set_default_resource(std::pmr::null_memory_resource());
    try {
        for (std::string line; std::getline(std::cin, line);) {
            GlobalContext global;
            auto input = Json::parse(global.memory_resource(), line);
            if (!input) {
                throw std::runtime_error("Invalid resource binding input");
            }
            std::cout << binding_observe(global, *input).dump(global.memory_resource()) << '\n';
        }
    } catch (const std::exception& error) {
        std::pmr::set_default_resource(previous);
        std::cerr << error.what() << '\n';
        return 1;
    }
    std::pmr::set_default_resource(previous);
}
