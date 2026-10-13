#include "../generated_types/include/cpp_types.hpp"
#include "../generated_relations/include/cpp_relations.hpp"
#include "../generated_optional_types/include/cpp_optional_types.hpp"
#include "fixture_value.hpp"
#include <iostream>

static bool boolean(const Json& input, std::string_view name) {
    return std::get<bool>(field(input, name).data);
}

template <class T, class F>
static Json state(Memory r, const eval::EvalResult<T>& result, F&& encode) {
    Json::Object output(r);
    if (result.is_value()) {
        output.emplace(String("state", r), Json(r, "Value"));
        output.emplace(String("value", r), encode(result.get()));
    } else if (result.is_null()) {
        output.emplace(String("state", r), Json(r, "Null"));
    } else {
        output.emplace(String("state", r), Json(r, "NotLoaded"));
        output.emplace(String("node", r), Json(r, result.missing().failed_node));
        output.emplace(String("path", r), Json(r, result.missing().attempted_path));
    }
    return Json(r, output);
}

static Json changed(Memory r, const std::optional<Value>& value) {
    Json::Object output(r);
    output.emplace(String("present", r), Json(r, bool(value)));
    output.emplace(String("value", r), value ? value->to_json_value(r) : Json(r));
    return Json(r, output);
}

static Json view(Memory r, const cpp_types::Person& entity) {
    Json::Object output(r);
    output.emplace(String("email", r), Json(r, entity.email()));
    output.emplace(String("nickname", r),
                   entity.nickname() ? Json(r, *entity.nickname()) : Json(r));
    output.emplace(String("active", r), Json(r, entity.active()));
    output.emplace(String("age", r), Json(r, entity.age()));
    output.emplace(String("eval_email", r), state(r, entity.evalEmail(), [&](const auto& v) {
                       return Json(r, v);
                   }));
    output.emplace(String("eval_nickname", r), state(r, entity.evalNickname(), [&](const auto& v) {
                       return v ? Json(r, *v) : Json(r);
                   }));
    output.emplace(String("eval_active", r), state(r, entity.evalActive(), [&](auto v) {
                       return Json(r, v);
                   }));
    output.emplace(String("eval_age", r), state(r, entity.evalAge(), [&](auto v) {
                       return Json(r, v);
                   }));
    output.emplace(String("changed_email", r), changed(r, entity.changedEmail()));
    output.emplace(String("changed_nickname", r), changed(r, entity.changedNickname()));
    output.emplace(String("changed_active", r), changed(r, entity.changedActive()));
    output.emplace(String("changed_age", r), changed(r, entity.changedAge()));
    return Json(r, output);
}

static Json scalar(UserContext& user_context, const Json& input) {
    auto r = user_context.memory_resource();
    Record row(r);
    row.emplace(String("id", r), Value(r, std::uint64_t(7)));
    row.emplace(String("version", r), Value(r, std::int64_t(3)));
    if (boolean(input, "loaded")) {
        row.emplace(String("email", r), Value(r, "initial"));
        row.emplace(String("nickname", r),
                    boolean(input, "initial_null") ? Value(r) : Value(r, "initial"));
        row.emplace(String("active", r), Value(r, false));
        row.emplace(String("age", r), Value(r, std::int64_t(0)));
    }
    auto decoded =
        cpp_types::Person::from_compact_row(r, CompactRow(std::allocator_arg, Allocator(r), row));
    auto& entity = user_context.make<cpp_types::Person>(decoded.value());
    auto& copy = user_context.make<cpp_types::Person>(entity);
    auto before = view(r, entity);
    auto text_value = text(field(input, "text"));
    const auto age = std::get<std::int64_t>(field(input, "age").data);
    if (text(field(input, "op")) == "update" || text(field(input, "op")) == "update_self") {
        entity.updateEmail(text_value);
        entity.updateNickname(boolean(input, "null")
                                  ? std::nullopt
                                  : std::optional<String>(std::in_place, text_value, r));
        entity.updateActive(boolean(input, "active"));
        entity.updateAge(age);
        if (text(field(input, "op")) == "update_self") {
            entity.updateNickname(entity.nickname());
            entity.updateEmail(entity.email());
        }
    } else if (text(field(input, "op")) == "overlay") {
        entity.root.set(entity.entity_key(), "email", Value(r, text_value));
        entity.root.set(entity.entity_key(),
                        "nickname",
                        boolean(input, "null") ? Value(r) : Value(r, text_value));
        entity.root.set(entity.entity_key(), "active", Value(r, boolean(input, "active")));
        entity.root.set(entity.entity_key(), "age", Value(r, age));
    }
    Json::Object output(r);
    output.emplace(String("before", r), std::move(before));
    output.emplace(String("after", r), view(r, entity));
    output.emplace(String("copy", r), view(r, copy));
    return Json(r, output);
}

static Json field_view(Memory r, const cpp_types::Person& entity, std::string_view name) {
    Json::Object output(r);
    auto inspect = [&](const auto& getter, const auto& evaluation, const auto& mutation) {
        output.emplace(String("getter", r),
                       cpp_types::generated_detail::encode(r, getter).to_json_value(r));
        output.emplace(String("eval", r), state(r, evaluation, [&](const auto& v) {
                           return cpp_types::generated_detail::encode(r, v).to_json_value(r);
                       }));
        output.emplace(String("changed", r), changed(r, mutation));
    };
    if (name == "balance") {
        inspect(entity.balance(), entity.evalBalance(), entity.changedBalance());
    } else if (name == "height") {
        inspect(entity.height(), entity.evalHeight(), entity.changedHeight());
    } else if (name == "ratio") {
        inspect(entity.ratio(), entity.evalRatio(), entity.changedRatio());
    } else if (name == "birthday") {
        inspect(entity.birthday(), entity.evalBirthday(), entity.changedBirthday());
    } else if (name == "updated_at") {
        inspect(entity.updatedAt(), entity.evalUpdatedAt(), entity.changedUpdatedAt());
    } else if (name == "id") {
        inspect(entity.id(), entity.evalId(), entity.changedId());
    } else if (name == "version") {
        inspect(entity.version(), entity.evalVersion(), entity.changedVersion());
    } else if (name == "payload") {
        inspect(entity.payload(), entity.evalPayload(), entity.changedPayload());
    } else {
        throw std::runtime_error("invalid field name");
    }
    std::optional<Value> original;
    if (auto snapshot = entity.root.original_snapshot()) {
        auto found = snapshot->fields.find(name);
        if (found != snapshot->fields.end()) {
            original.emplace(detail::clone_in(r, found->second));
        }
    }
    output.emplace(String("original", r), changed(r, original));
    output.emplace(String("metadata_version", r), Json(r, entity.version_value()));
    return Json(r, output);
}

static Json field_probe(UserContext& user_context, const Json& input) {
    auto r = user_context.memory_resource();
    auto name = text(field(input, "field"));
    Record row(r);
    row.emplace(String("id", r), Value(r, std::uint64_t(7)));
    row.emplace(String("version", r), Value(r, std::int64_t(3)));
    if (auto found = row.find(name); found != row.end()) {
        row.erase(found);
    }
    if (boolean(input, "loaded")) {
        row.emplace(String(name, r), value(r, field(input, "initial")));
    }
    auto decoded =
        cpp_types::Person::from_compact_row(r, CompactRow(std::allocator_arg, Allocator(r), row));
    if (!decoded) {
        throw std::runtime_error("invalid field row");
    }
    auto& entity = user_context.make<cpp_types::Person>(decoded.value());
    auto& copy = user_context.make<cpp_types::Person>(entity);
    auto before = field_view(r, entity, name);
    auto operand = value(r, field(input, "value"));
    auto op = text(field(input, "op"));
    if (op == "update") {
        if (name == "balance") {
            entity.updateBalance(*operand.try_decimal());
        } else if (name == "height") {
            entity.updateHeight(*operand.try_f64());
        } else if (name == "ratio") {
            entity.updateRatio(static_cast<float>(*operand.try_f64()));
        } else if (name == "birthday") {
            entity.updateBirthday(*operand.try_date());
        } else if (name == "updated_at") {
            entity.updateUpdatedAt(*operand.try_timestamp());
        } else if (name == "id") {
            entity.updateId(*operand.try_u64());
        } else if (name == "version") {
            entity.updateVersion(*operand.try_i64());
        } else if (name == "payload") {
            std::optional<Json> payload;
            if (!operand.is_null()) {
                payload.emplace(operand.to_json_value(r));
            }
            entity.updatePayload(payload);
        }
    } else if (op == "overlay") {
        entity.root.set(entity.entity_key(), name, operand);
    }
    Json::Object output(r);
    output.emplace(String("before", r), std::move(before));
    output.emplace(String("after", r), field_view(r, entity, name));
    output.emplace(String("copy", r), field_view(r, copy, name));
    return Json(r, output);
}

static Json
optional_view(Memory r, const cpp_optional_types::Reading& entity, std::string_view name) {
    Json::Object output(r);
    auto inspect = [&](const auto& getter, const auto& evaluation, const auto& mutation) {
        output.emplace(String("getter", r),
                       cpp_optional_types::generated_detail::encode(r, getter).to_json_value(r));
        output.emplace(String("eval", r), state(r, evaluation, [&](const auto& v) {
                           return cpp_optional_types::generated_detail::encode(r, v).to_json_value(
                               r);
                       }));
        output.emplace(String("changed", r), changed(r, mutation));
    };
    if (name == "text") {
        inspect(entity.text(), entity.evalText(), entity.changedText());
    } else if (name == "active") {
        inspect(entity.active(), entity.evalActive(), entity.changedActive());
    } else if (name == "balance") {
        inspect(entity.balance(), entity.evalBalance(), entity.changedBalance());
    } else if (name == "height") {
        inspect(entity.height(), entity.evalHeight(), entity.changedHeight());
    } else if (name == "ratio") {
        inspect(entity.ratio(), entity.evalRatio(), entity.changedRatio());
    } else if (name == "count") {
        inspect(entity.count(), entity.evalCount(), entity.changedCount());
    } else if (name == "day") {
        inspect(entity.day(), entity.evalDay(), entity.changedDay());
    } else if (name == "recorded_at") {
        inspect(entity.recordedAt(), entity.evalRecordedAt(), entity.changedRecordedAt());
    } else if (name == "payload") {
        inspect(entity.payload(), entity.evalPayload(), entity.changedPayload());
    } else {
        throw std::runtime_error("invalid optional field");
    }
    std::optional<Value> original;
    if (auto snapshot = entity.root.original_snapshot()) {
        if (auto found = snapshot->fields.find(name); found != snapshot->fields.end()) {
            original.emplace(detail::clone_in(r, found->second));
        }
    }
    output.emplace(String("original", r), changed(r, original));
    return Json(r, output);
}

struct OptionalMarker {
    bool allow, fix_text;
};

class OptionalLogic final : public cpp_optional_types::ReadingCheckerLogic {
public:
    using ReadingCheckerLogic::ReadingCheckerLogic;
    mutable std::size_t calls = 0, before_logic = 0;

    void checkAndFixReading(const UserContext& user_context,
                            cpp_optional_types::Reading& entity,
                            CheckObjectStatus,
                            const ObjectLocation& location,
                            CheckResults& results) const override {
        ++calls;
        before_logic = results.size();
        const auto& marker = user_context.require_resource<OptionalMarker>().value().get();
        required(marker.allow, "custom_field", location, results);
        if (marker.fix_text) {
            auto r = user_context.memory_resource();
            entity.updateText(std::optional<String>(std::in_place, "custom fixed", r));
        }
    }
};

static Json optional_probe(UserContext& user_context, const Json& input) {
    using cpp_optional_types::Reading;
    auto r = user_context.memory_resource();
    auto name = text(field(input, "field"));
    Record row(r);
    row.emplace(String("id", r), Value(r, std::uint64_t(7)));
    row.emplace(String("version", r), Value(r, std::int64_t(3)));
    if (boolean(input, "loaded")) {
        row.emplace(String(name, r), value(r, field(input, "initial")));
    }
    auto decoded = Reading::from_compact_row(r, CompactRow(std::allocator_arg, Allocator(r), row));
    if (!decoded) {
        throw std::runtime_error("invalid optional row");
    }
    auto& entity = user_context.make<Reading>(decoded.value());
    if (boolean(input, "full")) {
        entity.load_state =
            eval::LoadState::fully_loaded(r)
                .into_indexed(detail::ContextArena::from_memory(r).loaded_snapshots(),
                              Reading::field_layout(r))
                .value();
    }
    auto& copy = user_context.make<Reading>(entity);
    auto before = optional_view(r, entity, name);
    auto operand = value(r, field(input, "value"));
    auto op = text(field(input, "op"));
    const bool null = std::holds_alternative<std::monostate>(operand.data);
    if (op == "update") {
        if (name == "text") {
            entity.updateText(null ? std::nullopt
                                   : std::optional<String>(std::in_place, *operand.try_text(), r));
        } else if (name == "active") {
            entity.updateActive(null ? std::nullopt : operand.try_bool());
        } else if (name == "balance") {
            entity.updateBalance(null ? std::nullopt : operand.try_decimal());
        } else if (name == "height") {
            entity.updateHeight(null ? std::nullopt : operand.try_f64());
        } else if (name == "ratio") {
            entity.updateRatio(null ? std::nullopt
                                    : std::optional<float>(static_cast<float>(*operand.try_f64())));
        } else if (name == "count") {
            entity.updateCount(null ? std::nullopt : operand.try_i64());
        } else if (name == "day") {
            entity.updateDay(null ? std::nullopt : operand.try_date());
        } else if (name == "recorded_at") {
            entity.updateRecordedAt(null ? std::nullopt : operand.try_timestamp());
        } else if (name == "payload") {
            std::optional<Json> payload;
            if (!null) {
                payload.emplace(operand.to_json_value(r));
            }
            entity.updatePayload(payload);
        }
    } else if (op == "overlay") {
        entity.root.set(entity.entity_key(), name, operand);
    }
    auto& marker =
        user_context.make<OptionalMarker>(boolean(input, "allow"), boolean(input, "fix"));
    user_context.insert_resource(user_context, marker);
    auto& logic = user_context.make<OptionalLogic>();
    auto& checker = user_context.make<cpp_optional_types::ReadingChecker>(logic);
    auto status_name = text(field(input, "status"));
    auto status = status_name == "update"   ? CheckObjectStatus::Update
                  : status_name == "create" ? CheckObjectStatus::Create
                                            : CheckObjectStatus::Unknown;
    auto location = ObjectLocation::array_root(r, 2);
    location.member("reading_info");
    CheckResults results(r);
    checker.check_and_fix_typed(user_context, entity, status, location, results);
    Json::Array diagnostics(r);
    for (auto profile : {JsonFieldNamingProfile::CamelCase,
                         JsonFieldNamingProfile::SnakeCase,
                         JsonFieldNamingProfile::PascalCase}) {
        Json::Array errors(r);
        for (const auto& result : results) {
            errors.emplace_back(result.to_wire(r, profile).to_json(r));
        }
        diagnostics.emplace_back(std::move(errors));
    }
    Json::Object output(r);
    output.emplace(String("before", r), std::move(before));
    output.emplace(String("after", r), optional_view(r, entity, name));
    output.emplace(String("copy", r), optional_view(r, copy, name));
    output.emplace(String("diagnostics", r), Json(r, diagnostics));
    output.emplace(String("calls", r), Json(r, static_cast<std::uint64_t>(logic.calls)));
    output.emplace(String("before_logic", r),
                   Json(r, static_cast<std::uint64_t>(logic.before_logic)));
    output.emplace(String("text", r), entity.text() ? Json(r, *entity.text()) : Json(r));
    return Json(r, output);
}

static Json relations(UserContext& user_context, const Json& input) {
    using namespace cpp_relations;
    auto r = user_context.memory_resource();
    EntityRuntimeState root(std::allocator_arg, Allocator(r));
    auto& device = user_context.make<Device>();
    device.root = detail::clone_in(r, root);
    device.updateId(7);
    const auto& foreign = field(input, "foreign_id");
    std::optional<std::uint64_t> id;
    if (!std::holds_alternative<std::monostate>(foreign.data)) {
        id = std::holds_alternative<std::uint64_t>(foreign.data)
                 ? std::get<std::uint64_t>(foreign.data)
                 : static_cast<std::uint64_t>(std::get<std::int64_t>(foreign.data));
    }
    if (boolean(input, "fk_loaded")) {
        device.updatePlatformId(id);
    }
    Vector<std::string_view> fields(r);
    fields.emplace_back("id");
    if (boolean(input, "fk_loaded")) {
        fields.emplace_back("platform_id");
    }
    if (boolean(input, "selected")) {
        fields.emplace_back("platform");
    }
    device.load_state = eval::LoadState::partial(r, fields);
    auto& owner = user_context.make<Platform>();
    owner.root = detail::clone_in(r, root);
    owner.updateId(9);
    auto& target = user_context.make<Platform>();
    target.updateId(9);
    auto& profile = user_context.make<Profile>();
    profile.updateId(11);
    EntityGraphBuilder graph(std::allocator_arg, Allocator(r));
    if (boolean(input, "cached")) {
        graph.install(9, detail::clone_in(r, target));
    }
    const auto forward = text(field(input, "forward"));
    if (forward == "present") {
        graph.install_relation_option(
            "Device",
            7,
            "platform",
            std::optional<Platform>(std::in_place, std::allocator_arg, Allocator(r), target));
    } else if (forward == "empty") {
        graph.install_relation_option<Platform>("Device", 7, "platform", std::nullopt);
    } else if (forward == "wrong_type") {
        graph.install_relation_option("Device", 7, "platform", std::optional<std::uint64_t>(99));
    }
    const auto reverse = text(field(input, "reverse"));
    if (reverse == "present") {
        graph.install_relation_option(
            "Platform",
            9,
            "profile",
            std::optional<Profile>(std::in_place, std::allocator_arg, Allocator(r), profile));
    } else if (reverse == "empty") {
        graph.install_relation_option<Profile>("Platform", 9, "profile", std::nullopt);
    } else if (reverse == "wrong_type") {
        graph.install_relation_option("Platform", 9, "profile", std::optional<std::uint64_t>(99));
    }
    root.freeze_graph(std::move(graph)).value();
    auto handle = owner.profileHandle();
    Json::Object output(r);
    output.emplace(String("forward", r), state(r, device.evalPlatform(), [&](const auto& v) {
                       return Json(r, v.get().id());
                   }));
    output.emplace(String("foreign", r), state(r, device.evalPlatformId(), [&](const auto& v) {
                       return v ? Json(r, *v) : Json(r);
                   }));
    output.emplace(String("reverse", r), state(r, owner.evalProfile(), [&](const auto& v) {
                       return Json(r, v.get().id());
                   }));
    output.emplace(String("handle", r),
                   Json(r,
                        handle.is_empty()    ? "Empty"
                        : handle.is_loaded() ? "Loaded"
                                             : "NotLoaded"));
    output.emplace(String("empty", r), Json(r, handle.is_empty()));
    output.emplace(String("loaded", r), Json(r, handle.is_loaded()));
    return Json(r, output);
}

int main() {
    try {
        for (std::string line; std::getline(std::cin, line);) {
            GlobalContext global;
            auto user_context = global.make_user_context();
            auto input = Json::parse(user_context->memory_resource(), line);
            if (!input) {
                throw std::runtime_error("invalid entity fixture");
            }
            auto output = [&] {
                try {
                    auto kind = text(field(*input, "kind"));
                    if (kind == "scalar") {
                        return scalar(*user_context, *input);
                    }
                    if (kind == "optional") {
                        return optional_probe(*user_context, *input);
                    }
                    return kind == "field" ? field_probe(*user_context, *input)
                                           : relations(*user_context, *input);
                } catch (const std::logic_error& error) {
                    auto r = user_context->memory_resource();
                    Json::Object result(r);
                    result.emplace(String("error", r), Json(r, error.what()));
                    return Json(r, result);
                }
            }();
            std::cout << output.dump(user_context->memory_resource()) << '\n';
        }
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
