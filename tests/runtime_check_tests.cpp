#include <teaql/runtime/context.hpp>
#include <teaql/runtime/error.hpp>
#include <teaql/runtime/error_debug.hpp>
#include <teaql/sql/error.hpp>
#include <iostream>
using namespace teaql;

template <class E>
concept HasDefaultErrorDebug =
    requires(DebugWriter& writer, const DataServiceError<E>& error) { write_debug(writer, error); };

struct DisplayOnlyError {
    const char* what() const {
        return "display only";
    }
};

static_assert(HasDefaultErrorDebug<String>);
static_assert(!HasDefaultErrorDebug<DisplayOnlyError>);

static void check(bool ok, const char* message) {
    if (!ok) {
        throw std::runtime_error(message);
    }
}

static void diagnostic_ownership() {
    GlobalContext global;
    Ref<RuntimeError> retained;
    {
        auto user = global.make_user_context();
        auto owner = user->memory_resource();
        CheckResults results(owner);
        results.emplace_back(CheckResult::required(owner, ObjectLocation::array_root(owner, 42)));
        results.back()
            .with_message(String(4096, 'm', owner))
            .with_input_value(Value(owner, "private input"))
            .with_source_instance_path("/raw/source");
        auto error = RuntimeError::check(owner, std::move(results));
        retained = user->make_global<RuntimeError>(error);
        auto text = debug_string(owner, error);
        check(text.get_allocator().resource() == owner && text.find("Check([CheckResult {") == 0 &&
                  text.find("Some(Text(\"private input\"))") != String::npos,
              "raw Debug retains full structured payload in the chosen allocation domain");

        struct Concrete {
            int code;
        };

        auto wrapped = DataServiceError<Concrete>::executor(owner, Concrete{42});
        String output(owner);
        DebugWriter writer(output, {true, DebugNumbers::UpperHex});
        write_debug(writer, wrapped, [](auto& out, const Concrete& value) {
            out.structure("Concrete", [&](auto& fields) {
                fields.field("code", [&](auto& inner) {
                    inner.integer(value.code);
                });
            });
        });
        check(
            output.find("Executor(\n    Concrete {") == 0 &&
                output.find("code: 0x2A") != String::npos,
            "borrowed concrete executor formatter retains its own type and inherited numeric mode");
    }
    check(global.live_user_bytes() == 0, "retained diagnostics leave no source User allocations");
    auto user = global.make_user_context();
    const auto& error = user->get(user->rebind_global(retained));
    String output(global.memory_resource());
    DebugWriter writer(output, {true});
    write_debug(writer, error);
    check(
        output.find(String(4096, 'm', global.memory_resource())) != String::npos &&
            output.find("/raw/source") != String::npos &&
            output.find("private input") != String::npos,
        "all diagnostic fields survive actual source User deletion through explicit Global copying");
}

static void models_and_domains() {
    GlobalContext global;
    auto user = global.make_user_context();
    auto r = user->memory_resource();
    auto g = global.memory_resource();
    auto& location = user->make<ObjectLocation>();
    location.member("")
        .member("long_field_name_for_allocation")
        .element(std::size_t(-1))
        .member("ßeta_ﬃle/~");
    check(location.to_string(r).starts_with(".long_") &&
              location.model_path(r).starts_with("long_"),
          "empty members preserve the distinct display and model paths");
    check(ObjectLocation::root(r).to_string(r) == "$" &&
              ObjectLocation::root(r).model_path(r).empty(),
          "root displays dollar but has empty model path");
    check(ObjectLocation::hash_root(r, "one_ßeta_ﬃle/~").instance_path(r) == "/oneSSetaFFIle~1~0",
          "Unicode expansion and JSON Pointer escaping");
    auto& result = user->make<CheckResult>(CheckRule::ContextRootMismatch, location);
    auto& object = user->make<Value>(Value::Object(r));
    auto& fields = std::get<Value::Object>(object.data);
    {
        Value::List list(r);
        list.emplace_back("nested text long enough to allocate a separate string buffer");
        fields.emplace(String("nested_long_field", r), Value(r, list));
    }
    result.with_input_value(object)
        .with_system_value(TypedNull{DataType::Text})
        .with_entity_type("LongEntityNameForAllocatorCheck")
        .with_source_instance_path("/original_field/0/long_path");
    auto& copied = global.make<CheckResult>(result);
    auto& wire = user->make<WireCheckResult>(result.to_wire(r, JsonFieldNamingProfile::PascalCase));
    auto& copied_wire = global.make<WireCheckResult>(wire);
    check(copied == result && copied_wire == wire,
          "allocator cloning preserves structural equality");
    CheckResults errors(r);
    errors.emplace_back(result);
    errors.emplace_back(CheckResult::required(r, ObjectLocation::root(r)));
    errors.back().with_message("");
    auto& error = user->make<RuntimeError>(RuntimeError::check(r, std::move(errors)));
    auto& global_error = global.make<RuntimeError>(error);
    auto& wrapped = global.make<DataServiceError<String>>(
        DataServiceError<String>::runtime(r, detail::clone_in(r, error)));
    auto& entity = global.make<DataServiceError<String>>(
        DataServiceError<String>::entity(r,
                                         EntityError(std::allocator_arg,
                                                     Allocator(r),
                                                     "EntityWithLongAllocatedName",
                                                     "a message long enough for allocation")));
    auto& executor = global.make<DataServiceError<String>>(DataServiceError<String>::executor(
        r, String("executor detail long enough for allocation", r)));
    auto& relation = global.make<RuntimeError>(RuntimeError::missing_relation(
        r, "LongEntityNameForAllocatorCheck", "long_relation_name_for_allocation"));
    auto& conflict = global.make<RuntimeError>(RuntimeError::optimistic_lock_conflict(
        r, "LongEntityNameForAllocatorCheck", "id long enough for allocation"));
    auto& sql = user->make<SqlCompileError>(SqlCompileError::Code::InvalidSchemaShape,
                                            "a long field name for allocation");
    sql.reason = "a long reason for schema shape failure";
    auto& compile =
        global.make<RuntimeError>(RuntimeError::sql_compile(r, detail::clone_in(r, sql)));
    auto& assigned = global.make<CheckResult>(CheckRule::Required, ObjectLocation::root(g));
    assigned = detail::clone_in(r, result);
    auto& assigned_wire = global.make<WireCheckResult>();
    assigned_wire = detail::clone_in(r, wire);
    auto& assigned_error = global.make<RuntimeError>(RuntimeError::behavior(g, "initial"));
    assigned_error = detail::clone_in(r, error);
    auto& assigned_service = global.make<DataServiceError<String>>(
        DataServiceError<String>::executor(g, String("initial", g)));
    assigned_service = detail::clone_in(r, wrapped);
    // An index-only segment also carries a domain, despite having no string payload.
    auto& index = user->make<LocationSegment>(std::size_t(42));
    auto& index_copy = global.make<LocationSegment>(index);
    user.reset();
    check(copied.memory_resource() == g && copied.input_value->memory_resource() == g &&
              copied.system_value->memory_resource() == g,
          "CheckResult copies recursively into the requested domain");
    const auto& copied_list = std::get<Value::List>(
        std::get<Value::Object>(copied.input_value->data).begin()->second.data);
    check(copied_list.front().memory_resource() == g &&
              copied_list.front().try_text()->starts_with("nested text"),
          "deep value graph survives UserContext destruction");
    check(copied_wire.memory_resource() == g && copied_wire.location[1].memory_resource() == g &&
              copied_wire.input_value->memory_resource() == g,
          "wire location and JSON own Global buffers");
    check(copied_wire.to_json(g) == assigned_wire.to_json(g) && copied == assigned,
          "cross-domain move assignment copies the complete payload");
    check(global_error.check_results().size() == 2 &&
              global_error.check_results()[0].memory_resource() == g &&
              global_error.to_string(g).ends_with("; "),
          "Runtime Check preserves payload and empty-message display");
    check(wrapped.kind() == DataServiceError<String>::Kind::Runtime &&
              wrapped.to_string(g) == global_error.to_string(g),
          "DataService runtime wrapper has no extra display prefix");
    check(assigned_error.to_string(g) == global_error.to_string(g) &&
              assigned_service.to_string(g) == wrapped.to_string(g),
          "error assignment preserves target lifetime");
    check(entity.entity_error().memory_resource() == g &&
              executor.executor_error().get_allocator().resource() == g,
          "entity and executor error payloads own Global memory");
    check(relation.missing_relation_error().relation == "long_relation_name_for_allocation" &&
              conflict.optimistic_lock_conflict_error().id == "id long enough for allocation",
          "structured relation and conflict data survive source close");
    check(compile.sql_compile_error().code == SqlCompileError::Code::InvalidSchemaShape &&
              compile.sql_compile_error().reason == "a long reason for schema shape failure" &&
              compile.sql_compile_error().memory_resource() == g,
          "SQL errors retain structured code and detail");
    check(index_copy.kind() == LocationSegment::Kind::Index && index_copy.memory_resource() == g,
          "index segment allocator domain is explicit");
}

static void constructors_and_optional_presence() {
    GlobalContext global;
    auto user = global.make_user_context();
    auto r = user->memory_resource();
    auto absent = CheckResult::required(r, ObjectLocation::root(r));
    auto present = CheckResult::required(r, ObjectLocation::root(r));
    present.with_input_value(Value(r)).with_message("");
    auto a = absent.to_wire(r, JsonFieldNamingProfile::CamelCase);
    auto b = present.to_wire(r, JsonFieldNamingProfile::CamelCase);
    check(
        !a.input_value && b.input_value && present.to_string(r).empty(),
        "Some(null) and absent remain distinct in the model; Some(empty message) overrides display");
    check(std::get<Json::Object>(a.to_json(r).data).size() == 8,
          "wire includes every nullable key");
    auto min =
        CheckResult::min(r, ObjectLocation::hash_root(r, "amount"), Value(r, 2), Value(r, 1));
    auto max = CheckResult::max(r, ObjectLocation::array_root(r, 3), Value(r, 7), Value(r, 9));
    auto min_str = CheckResult::min_str(r, ObjectLocation::root(r), UINT64_MAX, Value(r, "abc"));
    auto max_str = CheckResult::max_str(r, ObjectLocation::root(r), 0, Value(r, ""));
    check(min.rule == CheckRule::Min && min.system_value->try_i64() == 2 &&
              min.input_value->try_i64() == 1 && max.system_value->try_i64() == 7 &&
              max.input_value->try_i64() == 9,
          "numeric constructors preserve bound/current direction");
    check(min_str.rule == CheckRule::MinStringLength &&
              std::get<std::uint64_t>(min_str.system_value->data) == UINT64_MAX &&
              std::get<std::uint64_t>(max_str.system_value->data) == 0,
          "string limits preserve full u64 domain");
    enum class NativeFailure { Failed };
    auto error = DataServiceError<NativeFailure>::executor(r, NativeFailure::Failed);
    auto formatter = [&](NativeFailure) -> String {
        return String("native failure", r);
    };
    check(error.to_string(r, formatter) == "native failure",
          "typed executor errors accept an explicit formatter");
    auto duplicate =
        DataServiceError<RuntimeError>::executor(r, RuntimeError::schema(r, "native schema"));
    auto clone = detail::clone_in(r, duplicate);
    check(clone.kind() == DataServiceError<RuntimeError>::Kind::Executor &&
              clone.executor_error().kind() == RuntimeError::Kind::Schema,
          "variant alternatives remain distinct even for identical C++ payload types");
}

int main() {
    try {
        auto* previous = std::pmr::set_default_resource(std::pmr::null_memory_resource());
        models_and_domains();
        constructors_and_optional_presence();
        diagnostic_ownership();
        std::pmr::set_default_resource(previous);
        std::cout << "Runtime check/error contracts passed\n";
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
