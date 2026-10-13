#include "../generated_inheritance/include/cpp_inheritance.hpp"
#include <iostream>

using namespace teaql;
using namespace cpp_inheritance;

static void check(bool value, const char* message) {
    if (!value) {
        throw std::runtime_error(message);
    }
}

template <class F>
static void rejects(F&& action, const char* message) {
    bool rejected = false;
    try {
        action();
    } catch (const ReferenceError&) {
        rejected = true;
    } catch (const std::invalid_argument&) {
        rejected = true;
    }
    check(rejected, message);
}

static void check_domain(const SelectQuery& query, Memory resource) {
    check(query.memory_resource() == resource &&
              query.entity.get_allocator().resource() == resource &&
              query.projection.get_allocator().resource() == resource,
          "every descendant query and nested container uses its destination Context");
    for (const auto& child : query.child_enhancements) {
        check_domain(child, resource);
    }
}

static void construction_and_retention() {
    GlobalContext global;
    auto user_context = global.make_user_context();
    auto Q = user_context->queries<query_factory>();
    auto request = Q.schoolsWithChildren();
    const auto& child = request.query().child_enhancements.at(0);
    check(!request.query().predicate && !request.query().slice && child.entity == "Academy" &&
              !child.predicate && child.child_enhancements.at(0).entity == "ResearchAcademy" &&
              !child.child_enhancements.at(0).predicate,
          "with-children recursively builds unfiltered, unsliced descendant selections");
    check(Q.schools().query().predicate.has_value(), "normal entry keeps live-row filtering");
    auto own_fields = Q.researchAcademiesMinimal().selectSelfWithoutParent();
    check(std::ranges::find(own_fields.query().projection, "name") !=
                  own_fields.query().projection.end() &&
              std::ranges::find(own_fields.query().projection, "budget") !=
                  own_fields.query().projection.end(),
          "the source without-parent alias still selects ancestor and descendant fields");
    auto all_relations = Q.schoolsMinimal().selectAny();
    check(all_relations.query().relations.size() == 1 &&
              all_relations.query().relations[0].name == "platform" &&
              all_relations.query().child_enhancements.empty(),
          "relation shortcuts remain separate from polymorphic child enhancement");
    auto independent = all_relations;
    all_relations.selectAll();
    check(
        independent.query().relations.size() == 1 && all_relations.query().relations.size() == 2,
        "repeated relation shortcuts preserve source append order and independent request copies");
    auto copy = request;
    request.limit(5).withTypeGroup().isAcademy();
    check(!copy.query().predicate && !copy.query().slice &&
              copy.query().projection.size() + 1 == request.query().projection.size(),
          "query value copies retain independent filter, projection and slice state");
    auto processed = [&] {
        auto local_child = Q.academiesMinimal().selectCampusCode();
        auto parent = Q.schoolsMinimal().enhanceChild(local_child);
        local_child.selectRank();
        return parent;
    }();
    check(processed.query().child_enhancements.at(0).projection.back() == "campus_code",
          "explicit child enhancement owns a snapshot after helper-local request destruction");
    auto next_user_context = global.make_user_context();
    auto next_Q = next_user_context->queries<query_factory>();
    rejects(
        [&] {
            request.enhanceChild(next_Q.academies());
        },
        "cross-Context enhancement requires an explicit destination copy");
    auto compatible_child = next_Q.academies().copyTo(*user_context);
    request.enhanceChild(compatible_child);
    auto destination_copy = request.copyTo(*next_user_context);
    check_domain(destination_copy.query(), next_user_context->memory_resource());
    auto retained = request.copyToGlobal(*user_context);
    auto& retained_model = [&]() -> School& {
        auto r = user_context->memory_resource();
        Record row(r);
        row.emplace(String("id", r), Value(r, std::uint64_t(7)));
        row.emplace(String("version", r), Value(r, std::int64_t(3)));
        row.emplace(String("campus_code", r), Value(r, String(1024, 'x', r)));
        row.emplace(String("budget", r), Value(r, *Decimal::parse("12.50")));
        auto decoded =
            School::from_compact_row(r, CompactRow(std::allocator_arg, Allocator(r), row));
        auto& model = user_context->make<School>(decoded.value());
        check(model.dynamic.size() == 2 && model.dynamic.contains("campus_code") &&
                  model.evalName().is_not_loaded(),
              "ancestor decode retains descendant columns separately from unloaded native fields");
        return global.make<School>(model);
    }();
    auto expired = request;
    user_context.reset();
    rejects(
        [&] {
            (void)expired.query();
        },
        "unexecuted query expires after Context deletion");
    auto rebound = retained.rebindGlobal(*next_user_context);
    check_domain(rebound.query(), global.memory_resource());
    check(rebound.query().child_enhancements.size() == 2 &&
              rebound.query().child_enhancements.at(0).child_enhancements.size() == 1,
          "explicit Global retention preserves the recursive enhancement tree");
    rejects(
        [&] {
            rebound.enhanceChild(next_Q.academies());
        },
        "Global query mutation cannot retain a UserContext child implicitly");
    rebound.enhanceChild(next_Q.academies().copyToGlobal(*next_user_context));
    rebound.enhanceChildrenIfNeeded();
    check_domain(rebound.query(), global.memory_resource());
    auto values =
        std::move(detail::clone_in(global.memory_resource(), retained_model)).into_values();
    const auto& campus = values.fields.at(String("campus_code", global.memory_resource()));
    check(campus.try_text()->size() == 1024 &&
              campus.memory_resource() == global.memory_resource() &&
              values.fields.contains("budget") && !values.fields.contains("name"),
          "retained descendant values serialize in Global scope after source deletion");
}

static void validate_extras_before_native_decode() {
    GlobalContext global;
    auto user_context = global.make_user_context();
    auto r = user_context->memory_resource();
    const std::array fields{DynamicPropertyDefinitions::Field{"_metric", DataType::I64}};
    auto definitions = DynamicPropertyDefinitions::from_fields(r, fields);
    auto& schema = user_context->make<DynamicPropertyDefinitions>(std::move(definitions).value());
    Record values(r);
    values.emplace(String("_metric", r), Value(r, "invalid readonly value"));
    values.emplace(String("rank", r), Value(r, "invalid native value"));
    std::array rows{CompactRow(std::allocator_arg, Allocator(r), values)};
    CompactRow::attach_dynamic_property_definitions(rows, schema);
    auto decoded = School::from_compact_row(r, std::move(rows[0]));
    check(!decoded && decoded.error().message.find("_metric") != String::npos,
          "generated decoding validates readonly metadata before native field conversion");
}

int main() {
    auto* previous = std::pmr::set_default_resource(std::pmr::null_memory_resource());
    try {
        construction_and_retention();
        validate_extras_before_native_decode();
    } catch (const std::exception& error) {
        std::pmr::set_default_resource(previous);
        std::cerr << error.what() << '\n';
        return 1;
    }
    std::pmr::set_default_resource(previous);
    std::cout << "Inherited query binding, snapshot and lifetime contracts passed\n";
}
