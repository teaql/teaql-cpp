// Direct pinned runtime-root checker, required-field and ledger-preflight tests.
#include "root_fixture.hpp"
#include <teaql/runtime/i18n.hpp>
#include <teaql/runtime/detail/graph_fix_scope.hpp>

using namespace original_runtime_root;

static const Value* field(const EntityValues& values, std::string_view name) {
    auto found = values.fields.find(name);
    return found == values.fields.end() ? nullptr : &found->second;
}

static void put(EntityValues& values, std::string_view name, Value value) {
    values.fields.insert_or_assign(String(name, values.memory_resource()), std::move(value));
}

static void field_is(const EntityValues& values, std::string_view name, const Value& expected) {
    auto* actual = field(values, name);
    check(actual && *actual == expected, "original checker field retains exact Value variant");
}

static void field_is(const MutationValues& values, std::string_view name, const Value& expected) {
    auto found = values.fields.find(name);
    check(found != values.fields.end() && found->second == expected,
          "original prepared checker field is present with the exact Value variant");
}

struct OrderChecker final : Checker {
    std::string_view entity() const override {
        return "Order";
    }

    void check_and_fix(const UserContext&,
                       EntityValues& values,
                       const ObjectLocation& location,
                       CheckResults& results) const override {
        auto owner = values.memory_resource();
        auto status = check_object_status(values);
        if (status == CheckObjectStatus::Create) {
            required(values, "name", location, results);
            values.fields.try_emplace(String("version", owner), Value(owner, std::int64_t(1)));
        }
        if (status == CheckObjectStatus::Update && field(values, "name") &&
            *field(values, "name") == Value(owner, "graph-update")) {
            put(values, "name", Value(owner, "graph-update-checked"));
        }
        min_string_length(values, "name", 3, location, results);
    }
};

struct TypedOrderChecker final : TypedChecker<Order> {
    void check_and_fix_typed(const UserContext&,
                             Order& entity,
                             CheckObjectStatus status,
                             const ObjectLocation& location,
                             CheckResults& results) const override {
        auto owner = results.get_allocator().resource();
        if (status == CheckObjectStatus::Create && entity.name.empty()) {
            results.emplace_back(
                CheckResult::required(owner, detail::clone_in(owner, location).member("name")));
        }
        const auto length = std::ranges::count_if(entity.name, [](unsigned char byte) {
            return (byte & 0xc0) != 0x80;
        });
        if (length < 3) {
            results.emplace_back(CheckResult::min_str(
                owner, detail::clone_in(owner, location).member("name"), 3, entity.name));
        }
        if (entity.name == "fix") {
            entity.name = "fixed";
        }
    }
};

template <class C>
static void install_checker(UserContext& user) {
    auto& registry = user.make<InMemoryCheckerRegistry>();
    auto& checker = user.make_resource<C>();
    registry.with_checker(user, checker);
    user.with_checker_registry(user, registry);
}

template <class T, class C>
static void install_typed_checker(UserContext& user) {
    auto& registry = user.make<InMemoryCheckerRegistry>();
    auto& callback = user.make_resource<C>();
    auto& adapter = user.make<TypedEntityChecker<T, C>>(user, callback);
    registry.with_checker(user, adapter);
    user.with_checker_registry(user, registry);
}

struct PlainFixture {
    GlobalContext global;
    std::unique_ptr<UserContext> user = global.make_user_context();
    Memory owner = user->memory_resource();

    void metadata(EntityDescriptor descriptor) {
        auto& store = user->make<InMemoryMetadataStore>();
        store.with_entity(std::move(descriptor));
        user->with_metadata(*user, store);
    }
};

static EntityDescriptor school(Memory owner, std::string_view required, bool version) {
    EntityDescriptor result(std::allocator_arg, Allocator(owner), "School");
    result.property("id", DataType::U64).id().not_null();
    if (!required.empty()) {
        result.property(required, DataType::Text).not_null();
    }
    if (version) {
        result.property("version", DataType::I64).version().not_null();
    }
    return result;
}

static const CheckResults& check_error(const RuntimeHookResult& result) {
    check(!result && result.error().kind() == RuntimeError::Kind::Check,
          "original structured checker error");
    return result.error().check_results();
}

static void required_at(const CheckResults& results, std::string_view path, Memory owner) {
    check(results.size() == 1 && results[0].rule == CheckRule::Required &&
              results[0].location.to_string(owner) == path,
          "original one required error at the exact field location");
}

static void checker_registry_validates_and_fixes_insert_commands() {
    Fixture f;
    install_checker<OrderChecker>(*f.user);
    f.generator(77);
    auto& service = f.service();
    auto command = service.insert_command();
    command.value("name", "valid");
    auto prepared = service.prepare_insert_command(command);
    check(bool(prepared), "original valid checked insert");
    field_is(prepared.value().values, "id", Value(f.owner, std::uint64_t(77)));
    field_is(prepared.value().values, "version", Value(f.owner, std::int64_t(1)));
    check(!prepared.value().values.fields.contains(check_object_status_field),
          "original checker preparation removes its status marker");
    auto invalid = service.insert_command();
    invalid.value("name", "no");
    auto error = service.prepare_insert_command(invalid);
    check(!error && error.error().kind() == RuntimeError::Kind::Check,
          "original short name fails with checker results");
    const auto& results = error.error().check_results();
    check(results.size() == 1 && results[0].location.to_string(f.owner) == "name",
          "original short-name result count and location");
}

static void metadata_not_null_constraints_are_checked_without_a_custom_checker() {
    PlainFixture f;
    f.metadata(school(f.owner, "contact_phone", false));
    EntityValues values(std::allocator_arg, Allocator(f.owner));
    put(values, "id", Value(f.owner, std::uint64_t(1)));
    mark_entity_status(values, CheckObjectStatus::Create);
    auto result = f.user->check_and_fix_values("School", values);
    required_at(check_error(result), "contact_phone", f.owner);
}

static void sparse_ledger_create_payload_reports_required_field_before_sql() {
    PlainFixture f;
    f.metadata(school(f.owner, "address", true));
    EntityValues payload(std::allocator_arg, Allocator(f.owner));
    put(payload, "id", Value(f.owner, std::uint64_t(1)));
    put(payload, "version", Value(f.owner, std::int64_t(1)));
    auto result = detail::ContextFixAccess::validate_required_create_payload(
        *f.user, "School", payload, ObjectLocation::root(f.owner));
    required_at(check_error(result), "address", f.owner);
    auto nested = ObjectLocation::root(f.owner).member("school_list").element(2);
    auto failure = detail::ContextFixAccess::validate_required_create_payload(
        *f.user, "School", payload, nested);
    const auto& results = check_error(failure);
    check(results[0].location.model_path(f.owner) == "school_list[2].address" &&
              results[0].location.instance_path(f.owner) == "/schoolList/2/address",
          "original sparse payload model and JSON paths");
    put(payload, "address", Value(f.owner, "12 River Road"));
    check(bool(detail::ContextFixAccess::validate_required_create_payload(
              *f.user, "School", payload, ObjectLocation::root(f.owner))),
          "original corrected sparse payload succeeds");
}

using Locations = std::pmr::map<EntityKey, ObjectLocation>;

static auto& execute_ledger(PlainFixture& f,
                            std::string_view entity,
                            StubExecutor& executor,
                            const EntityRuntimeState& root,
                            const Locations& locations) {
    auto& service =
        EntityDataService<StubExecutor>::for_executor(*f.user, entity, *f.user, executor);
    return Access::execute_ledger_plan_internal(service, root, locations);
}

static const RuntimeError& ledger_error(const auto& result) {
    check(!result && result.error().kind() == DataServiceError<StubError>::Kind::Runtime,
          "original ledger failure retains the Runtime error variant");
    return result.error().runtime_error();
}

static EntityKey key(Memory owner, std::string_view entity, std::uint64_t id) {
    return EntityKey(std::allocator_arg, Allocator(owner), entity, Value(owner, id));
}

static void nested_sparse_ledger_insert_plan_rejects_required_field_at_child_path() {
    PlainFixture f;
    EntityDescriptor descriptor(std::allocator_arg, Allocator(f.owner), "OrderLine");
    descriptor.property("id", DataType::U64).id().not_null();
    descriptor.property("order_id", DataType::U64).not_null();
    descriptor.property("name", DataType::Text).not_null();
    f.metadata(std::move(descriptor));
    auto& executor = f.user->make<StubExecutor>();
    EntityRuntimeState root(std::allocator_arg, Allocator(f.owner));
    auto child = key(f.owner, "OrderLine", 9);
    root.mark_as_new(child);
    root.set(child, "order_id", Value(f.owner, std::uint64_t(7)));
    Locations locations(f.owner);
    locations.emplace(detail::clone_in(f.owner, child),
                      ObjectLocation::root(f.owner).member("line_items").element(0));
    const auto& error =
        ledger_error(execute_ledger(f, "OrderLine", executor, root, locations).wait());
    check(error.kind() == RuntimeError::Kind::Check, "original nested ledger check error");
    const auto& results = error.check_results();
    required_at(results, "line_items[0].name", f.owner);
    check(results[0].location.instance_path(f.owner) == "/lineItems/0/name",
          "original nested sparse child JSON path");
    check(executor.mutations == 0, "required-child failure precedes SQL mutation");
}

static void ledger_update_of_existing_versioned_row_requires_loaded_original_version() {
    PlainFixture f;
    f.metadata(school(f.owner, "name", true));
    auto& executor = f.user->make<StubExecutor>();
    Record row(f.owner);
    row.emplace("id", Value(f.owner, std::int64_t(1)));
    row.emplace("name", Value(f.owner, "Old School"));
    row.emplace("version", Value(f.owner, std::int64_t(3)));
    executor.rows.emplace_back(row);
    EntityRuntimeState root(std::allocator_arg, Allocator(f.owner));
    root.set(key(f.owner, "School", 1), "name", Value(f.owner, "New School"));
    Locations locations(f.owner);
    const auto& error = ledger_error(execute_ledger(f, "School", executor, root, locations).wait());
    check(error.kind() == RuntimeError::Kind::Graph &&
              error.message().find("cannot update School") != std::string_view::npos &&
              error.message().find("without its loaded original version") != std::string_view::npos,
          "original versionless update error fragments");
    check(executor.mutations == 0, "versionless update cannot reach SQL mutation");
}

static void generated_attachment_version_conflict_rejects_ledger_plan_before_sql() {
    PlainFixture f;
    f.metadata(school(f.owner, "name", true));
    auto& executor = f.user->make<StubExecutor>();
    EntityRuntimeState root(std::allocator_arg, Allocator(f.owner));
    EntityRuntimeState stale(std::allocator_arg, Allocator(f.owner));
    auto entity = key(f.owner, "School", 1);
    root.set_original_version(entity, 2);
    root.set(entity, "name", Value(f.owner, "Newer"));
    stale.set_original_version(entity, 1);
    stale.set(entity, "name", Value(f.owner, "Stale"));
    root.adopt_mutations_from(stale);
    Locations locations(f.owner);
    const auto& error = ledger_error(execute_ledger(f, "School", executor, root, locations).wait());
    check(error.kind() == RuntimeError::Kind::Graph,
          "original attachment conflict is a Graph error");
    for (auto fragment : {"generated entity graph attachment failed before save",
                          "School#1",
                          "version 2",
                          "version 1"}) {
        check(error.message().find(fragment) != std::string_view::npos,
              "every original attachment conflict fragment");
    }
    check(executor.queries == 0 && executor.mutations == 0, "attachment conflict precedes all SQL");
}

static void ledger_delete_of_existing_versioned_row_requires_loaded_original_version() {
    PlainFixture f;
    f.metadata(school(f.owner, "", true));
    auto& executor = f.user->make<StubExecutor>();
    EntityRuntimeState root(std::allocator_arg, Allocator(f.owner));
    root.mark_as_delete(key(f.owner, "School", 1));
    Locations locations(f.owner);
    const auto& error = ledger_error(execute_ledger(f, "School", executor, root, locations).wait());
    check(error.kind() == RuntimeError::Kind::Graph &&
              error.message().find("cannot delete School") != std::string_view::npos &&
              error.message().find("without its loaded original version") != std::string_view::npos,
          "original versionless delete error fragments");
    check(executor.mutations == 0, "versionless deletion precedes SQL mutation");
}

static void metadata_validation_does_not_require_runtime_managed_version_on_create() {
    PlainFixture f;
    auto descriptor = school(f.owner, "", true);
    descriptor.property("name", DataType::Text).not_null();
    f.metadata(std::move(descriptor));
    EntityValues values(std::allocator_arg, Allocator(f.owner));
    put(values, "id", Value(f.owner, std::uint64_t(1)));
    put(values, "name", Value(f.owner, "TeaQL School"));
    mark_entity_status(values, CheckObjectStatus::Create);
    check(bool(f.user->check_and_fix_values("School", values)) &&
              !values.fields.contains("version"),
          "original metadata validation accepts absent runtime-managed create version");
}

struct TimestampedEntity : example::Model<TimestampedEntity> {
    static constexpr std::string_view ENTITY_NAME = "TimestampedEntity";
    std::int64_t revision = 0;
    Timestamp happened_at;

    TimestampedEntity(std::allocator_arg_t, Allocator a) : Model(std::allocator_arg, a) {}

    TimestampedEntity(std::allocator_arg_t, Allocator a, const TimestampedEntity& other)
        : Model(std::allocator_arg, a, other), revision(other.revision),
          happened_at(other.happened_at) {}

    TimestampedEntity(const TimestampedEntity&) = delete;
    TimestampedEntity(TimestampedEntity&&) noexcept = default;

    static EntityDescriptor entity_descriptor(Memory owner) {
        EntityDescriptor descriptor(std::allocator_arg, Allocator(owner), ENTITY_NAME);
        descriptor.table("timestamped_entity");
        descriptor.property("id", DataType::U64).id().not_null();
        descriptor.property("version", DataType::I64).version().not_null();
        descriptor.property("happened_at", DataType::Timestamp).not_null();
        return descriptor;
    }

    static Result<TimestampedEntity, EntityError> from_compact_row(Memory owner, CompactRow row) {
        using Output = Result<TimestampedEntity, EntityError>;
        auto id = example::decode_field<std::uint64_t>(owner, row, ENTITY_NAME, "id");
        auto version = example::decode_field<std::int64_t>(owner, row, ENTITY_NAME, "version");
        if (!id) {
            return Output::err(owner, std::move(id).error());
        }
        if (!version) {
            return Output::err(owner, std::move(version).error());
        }
        TimestampedEntity result(std::allocator_arg, Allocator(owner));
        result.id = id.value();
        result.revision = version.value();
        auto* value = row.get("happened_at");
        if (value && !std::holds_alternative<std::monostate>(value->data)) {
            if (auto* timestamp = std::get_if<Timestamp>(&value->data)) {
                result.happened_at = *timestamp;
            } else if (auto* number = std::get_if<std::int64_t>(&value->data)) {
                result.happened_at = Timestamp(*number);
            } else if (auto* number = std::get_if<std::uint64_t>(&value->data)) {
                result.happened_at = Timestamp(std::bit_cast<std::int64_t>(*number));
            } else {
                String message("invalid field happened_at: ", owner);
                message += detail::debug_value(owner, *value);
                return Output::err(
                    owner, EntityError(std::allocator_arg, Allocator(owner), ENTITY_NAME, message));
            }
        }
        return Output::ok(owner, std::move(result));
    }

    MutationValues into_values() && {
        MutationValues result(memory_resource());
        result.fields.emplace("id", Value(memory_resource(), id));
        result.fields.emplace("version", Value(memory_resource(), revision));
        result.fields.emplace("happened_at", Value(memory_resource(), happened_at));
        return result;
    }
};

struct NoopTimestampedChecker final : TypedChecker<TimestampedEntity> {
    void check_and_fix_typed(const UserContext&,
                             TimestampedEntity&,
                             CheckObjectStatus,
                             const ObjectLocation&,
                             CheckResults&) const override {}
};

static void typed_checker_preserves_values_and_reports_timestamp_type_error() {
    PlainFixture f;
    f.metadata(TimestampedEntity::entity_descriptor(f.owner));
    install_typed_checker<TimestampedEntity, NoopTimestampedChecker>(*f.user);
    EntityValues values(std::allocator_arg, Allocator(f.owner));
    put(values, "id", Value(f.owner, std::uint64_t(7)));
    put(values, "version", Value(f.owner, std::int64_t(1)));
    put(values, "happened_at", Value(f.owner, "2026-08-25"));
    mark_entity_status(values, CheckObjectStatus::Update);
    auto failure = f.user->check_and_fix_values("TimestampedEntity", values);
    field_is(values, "happened_at", Value(f.owner, "2026-08-25"));
    const auto& results = check_error(failure);
    check(results.size() == 1 && results[0].rule == CheckRule::InvalidType && results[0].message &&
              results[0].message->find("happened_at") != String::npos &&
              results[0].message->find("2026-08-25") != String::npos,
          "original timestamp materialization error preserves input and diagnostic fragments");
}

static void metadata_not_null_constraints_allow_omitted_fields_on_partial_update() {
    PlainFixture f;
    f.metadata(school(f.owner, "contact_phone", false));
    EntityValues values(std::allocator_arg, Allocator(f.owner));
    put(values, "id", Value(f.owner, std::uint64_t(1)));
    mark_entity_status(values, CheckObjectStatus::Update);
    check(bool(f.user->check_and_fix_values("School", values)),
          "original partial update allows omission");
    put(values, "contact_phone", Value(f.owner));
    auto error = f.user->check_and_fix_values("School", values);
    check_error(error);
}

static void typed_checker_validates_and_fixes_derived_entities_without_record_access() {
    Fixture f;
    f.store.with_entity(Order::entity_descriptor(f.owner));
    install_typed_checker<Order, TypedOrderChecker>(*f.user);
    f.generator(79);
    auto& service = f.service();
    auto command = service.insert_command();
    command.value("name", "fix");
    auto prepared = service.prepare_insert_command(command);
    check(bool(prepared), "original typed checker insert fixes the model");
    field_is(prepared.value().values, "name", Value(f.owner, "fixed"));
    field_is(prepared.value().values, "id", Value(f.owner, std::uint64_t(79)));
    field_is(prepared.value().values, "version", Value(f.owner, std::int64_t(1)));
    check(!prepared.value().values.fields.contains(check_object_status_field),
          "original typed checker preparation removes its status marker");
    auto absent = service.insert_command();
    absent.value("version", std::int64_t(1));
    auto failure = service.prepare_insert_command(absent);
    check(!failure && failure.error().kind() == RuntimeError::Kind::Check,
          "original typed checker absent name error");
    check(std::ranges::any_of(failure.error().check_results(),
                              [&](const auto& result) {
                                  return result.rule == CheckRule::Required &&
                                         result.location.to_string(f.owner) == "name";
                              }),
          "original required-name result remains present");
}

static void typed_checker_preserves_sparse_update_boundary() {
    PlainFixture f;
    f.metadata(Order::entity_descriptor(f.owner));
    install_typed_checker<Order, TypedOrderChecker>(*f.user);
    EntityValues values(std::allocator_arg, Allocator(f.owner));
    put(values, "id", Value(f.owner, std::uint64_t(7)));
    put(values, "name", Value(f.owner, "valid"));
    mark_entity_status(values, CheckObjectStatus::Update);
    check(bool(f.user->check_and_fix_values("Order", values)),
          "original sparse typed update succeeds");
    field_is(values, "id", Value(f.owner, std::uint64_t(7)));
    field_is(values, "name", Value(f.owner, "valid"));
    check(!values.fields.contains("version"), "typed default cannot become update intent");
    put(values, "name", Value(f.owner, "fix"));
    check(bool(f.user->check_and_fix_values("Order", values)), "original sparse fix succeeds");
    field_is(values, "name", Value(f.owner, "fixed"));
    check(!values.fields.contains("version"), "checker fix cannot expand sparse update");
}

static void checker_registry_reports_nested_create_locations_and_fixes_records() {
    PlainFixture f;
    install_checker<OrderChecker>(*f.user);
    EntityValues child(std::allocator_arg, Allocator(f.owner));
    put(child, "id", Value(f.owner, std::uint64_t(10)));
    mark_entity_status(child, CheckObjectStatus::Create);
    auto location = ObjectLocation::hash_root(f.owner, "lines").element(0);
    auto result = f.user->check_and_fix_values_at("Order", child, location);
    field_is(child, "version", Value(f.owner, std::int64_t(1)));
    required_at(check_error(result), "lines[0].name", f.owner);
    put(child, "name", Value(f.owner, "valid child"));
    check(bool(f.user->check_and_fix_values_at("Order", child, location)),
          "original nested corrected child succeeds");
}

static void built_in_language_translators_cover_fifteen_languages() {
    PlainFixture f;
    check(all_languages.size() == 15, "original language count");
    CheckResults results(f.owner);
    results.emplace_back(
        CheckResult::required(f.owner, ObjectLocation::hash_root(f.owner, "name")));
    results.emplace_back(CheckResult::min(
        f.owner, ObjectLocation::hash_root(f.owner, "age"), std::int64_t(18), std::int64_t(12)));
    results.emplace_back(CheckResult::max(
        f.owner, ObjectLocation::hash_root(f.owner, "age"), std::int64_t(65), std::int64_t(70)));
    results.emplace_back(
        CheckResult::min_str(f.owner, ObjectLocation::hash_root(f.owner, "name"), 2, "x"));
    results.emplace_back(CheckResult::max_str(
        f.owner, ObjectLocation::hash_root(f.owner, "name"), 8, "too long name"));
    Vector<String> messages(f.owner);
    for (auto language : all_languages) {
        for (const auto& result : results) {
            messages.emplace_back(translate_check_result(f.owner, language, result));
        }
    }
    check(messages.size() == 75 && std::ranges::all_of(messages,
                                                       [](const auto& message) {
                                                           return !message.empty() &&
                                                                  message.find('{') == String::npos;
                                                       }),
          "all 75 original translations are complete and nonempty");
    for (auto word : {"required", "必填", "obligatoire"}) {
        check(std::ranges::any_of(messages,
                                  [&](const auto& message) {
                                      return message.find(word) != String::npos;
                                  }),
              "original English, Chinese and French translation witnesses");
    }
    check(language_from_code("zh-CN") == Language::Chinese &&
              language_from_code("zh-TW") == Language::TraditionalChinese,
          "original Chinese locale aliases");
}

static void user_context_language_switch_translates_checker_errors() {
    Fixture f;
    install_checker<OrderChecker>(*f.user);
    f.generator(77);
    f.user->with_language(Language::Chinese);
    auto& service = f.service();
    auto result = service.prepare_insert_command(service.insert_command());
    check(!result && result.error().kind() == RuntimeError::Kind::Check,
          "original translated missing-name error");
    const auto& errors = result.error().check_results();
    check(errors.size() == 1 && errors[0].message &&
              errors[0].message->find("必填") != String::npos,
          "original Chinese required message");
    PlainFixture switched;
    switched.user->with_language(Language::English);
    check(bool(switched.user->set_language_code("es")) &&
              switched.user->language() == Language::Spanish,
          "original successful language switch");
    check(!switched.user->set_locale_code("invalid-code") &&
              switched.user->language() == Language::Spanish,
          "failed locale switch preserves previous language");
    auto catalog = I18nCatalog::from_json(switched.owner, R"({
        "schema":"teaql.i18n/v1","defaultLocale":"en","locales":{
        "en":{"messages":{"checker.required":"EN {location}"},"vocabulary":{}},
        "es":{"messages":{"checker.required":"ES {location}"},"vocabulary":{}}}})");
    check(bool(catalog), "original bilingual custom catalog parses");
    auto& installed = switched.user->make<I18nCatalog>(std::move(catalog).value());
    switched.user->set_i18n_catalog(*switched.user, installed);
    CheckResults translated(switched.owner);
    translated.emplace_back(
        CheckResult::required(switched.owner, ObjectLocation::hash_root(switched.owner, "name")));
    switched.user->translate_check_results(translated);
    check(translated[0].message && *translated[0].message == "ES Name",
          "original exact custom catalog result");
}

int main() {
    auto* previous = std::pmr::set_default_resource(std::pmr::null_memory_resource());

    struct Restore {
        Memory previous;

        ~Restore() {
            std::pmr::set_default_resource(previous);
        }
    } restore{previous};

    try {
        checker_registry_validates_and_fixes_insert_commands();
        metadata_not_null_constraints_are_checked_without_a_custom_checker();
        sparse_ledger_create_payload_reports_required_field_before_sql();
        nested_sparse_ledger_insert_plan_rejects_required_field_at_child_path();
        ledger_update_of_existing_versioned_row_requires_loaded_original_version();
        generated_attachment_version_conflict_rejects_ledger_plan_before_sql();
        ledger_delete_of_existing_versioned_row_requires_loaded_original_version();
        metadata_validation_does_not_require_runtime_managed_version_on_create();
        typed_checker_preserves_values_and_reports_timestamp_type_error();
        metadata_not_null_constraints_allow_omitted_fields_on_partial_update();
        typed_checker_validates_and_fixes_derived_entities_without_record_access();
        typed_checker_preserves_sparse_update_boundary();
        checker_registry_reports_nested_create_locations_and_fixes_records();
        built_in_language_translators_cover_fifteen_languages();
        user_context_language_switch_translates_checker_errors();
        std::cout << "Fifteen original runtime-root validation contracts passed\n";
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
