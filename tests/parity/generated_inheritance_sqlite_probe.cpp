#include "../generated_inheritance/include/cpp_inheritance.hpp"
#include "fixture_value.hpp"
#include <sqlite3.h>
#include <iostream>

using namespace cpp_inheritance;

template <Entity T>
static Json describe_return(Memory r, const T& entity) {
    Json::Object out(r), loaded(r), native(r);
    for (std::string_view name :
         {"id", "name", "rank", "version", "platform_id", "campus_code", "budget"}) {
        loaded.emplace(String(name, r), Json(r, entity.is_field_loaded(name)));
    }
    auto original = entity.original_values();
    auto values = std::move(detail::clone_in(r, entity)).into_values();
    for (const auto& [name, value] : values.fields) {
        if (!name.starts_with('_')) {
            native.emplace(String(name, r), value.to_json_value(r));
        }
    }
    out.emplace(String("type", r), Json(r, T::ENTITY_NAME));
    out.emplace(String("loaded", r), Json(r, loaded));
    out.emplace(String("original", r),
                original ? record_to_json_value(r, original->fields) : Json(r));
    out.emplace(String("native", r), Json(r, native));
    return Json(r, out);
}

template <class Request>
static Json observe(UserContext& user_context, Request request, const Json& input) {
    auto r = user_context.memory_resource();
    request.orderByIdAsc();
    if (std::get<bool>(field(input, "automatic").data)) {
        request.enhanceChildrenIfNeeded();
    }
    if (std::get<bool>(field(input, "manual").data)) {
        auto Q = user_context.queries<query_factory>();
        request.enhanceChild(Q.researchAcademiesMinimal().selectBudget().withRankGreaterThan(10));
    }
    if (std::get<bool>(field(input, "limit").data)) {
        request.limit(1);
    }
    if (const auto* failure = optional(input, "failure")) {
        if (text(*failure) == "type_filter") {
            const std::array types{Value(r, "Academy"), Value(r, "ResearchAcademy")};
            request.matchTypes(types);
        } else if (text(*failure) == "type_group") {
            request.withTypeGroup();
        }
    }
    if (text(field(input, "result")) == "stream") {
        request.stream(1);
    }
    auto executable = std::move(request)
                          .comment("Inherited SQLite qualification")
                          .purpose("Compare actual descendant enhancement");
    Json::Object out(r);
    Json::Array values(r);
    if (text(field(input, "result")) == "one") {
        const auto& result = executable.executeForOne(user_context).wait();
        if (!result) {
            out.emplace(String("error", r), Json(r, result.error().to_string(r)));
            return Json(r, out);
        }
        const auto& entity = result.value();
        if (entity) {
            values.emplace_back(describe_return(r, *entity));
        }
    } else if (text(field(input, "result")) == "stream") {
        const auto& result = executable.executeForStream(user_context).wait();
        if (!result) {
            out.emplace(String("error", r), Json(r, result.error().to_string(r)));
            return Json(r, out);
        }
        auto& stream = result.value().get();
        while (const auto* next = stream.wait_next()) {
            if (!*next) {
                out.emplace(String("error", r), Json(r, next->error().to_string(r)));
                return Json(r, out);
            }
            values.emplace_back(describe_return(r, require_value(*next)));
        }
    } else if (text(field(input, "result")) == "rows") {
        const auto& rows = require_value(executable.executeForRows(user_context).wait());
        for (const auto& row : rows.data) {
            values.emplace_back(record_to_json_value(r, row.into_map(r)));
        }
    } else {
        const auto& result = executable.executeForList(user_context).wait();
        if (!result) {
            out.emplace(String("error", r), Json(r, result.error().to_string(r)));
            return Json(r, out);
        }
        const auto& entities = result.value();
        for (const auto& entity : entities.data) {
            if (optional(input, "return_type")) {
                values.emplace_back(describe_return(r, entity));
                continue;
            }
            auto native = std::move(detail::clone_in(r, entity)).into_values();
            Record fields(r);
            for (const auto& [name, value] : native.fields) {
                if (!name.starts_with('_')) {
                    fields.emplace(String(name, r), value);
                }
            }
            values.emplace_back(record_to_json_value(r, fields));
        }
    }
    out.emplace(String("rows", r), Json(r, values));
    return Json(r, out);
}

template <class Request>
static Json adapt(UserContext& user_context, Request request, const Json& input) {
    if (const auto* target = optional(input, "return_type")) {
        if (text(*target) == "school") {
            return observe(user_context, std::move(request).template returnType<School>(), input);
        }
        if (text(*target) == "academy") {
            return observe(user_context, std::move(request).template returnType<Academy>(), input);
        }
        return observe(
            user_context, std::move(request).template returnType<ResearchAcademy>(), input);
    }
    return observe(user_context, std::move(request), input);
}

static Json dispatch(UserContext& user_context, const Json& input) {
    auto Q = user_context.queries<query_factory>();
    auto entry = text(field(input, "entry"));
    if (text(field(input, "entity")) == "school") {
        return adapt(user_context,
                     entry == "minimal"    ? Q.schoolsMinimal()
                     : entry == "children" ? Q.schoolsWithChildren()
                                           : Q.schools(),
                     input);
    }
    if (text(field(input, "entity")) == "academy") {
        return adapt(user_context,
                     entry == "minimal"    ? Q.academiesMinimal()
                     : entry == "children" ? Q.academiesWithChildren()
                                           : Q.academies(),
                     input);
    }
    return adapt(user_context,
                 entry == "minimal"    ? Q.researchAcademiesMinimal()
                 : entry == "children" ? Q.researchAcademiesWithChildren()
                                       : Q.researchAcademies(),
                 input);
}

int main() {
    try {
        for (std::string line; std::getline(std::cin, line);) {
            GlobalContext global;
            auto user_context = global.make_user_context();
            auto executor = setup(*user_context);
            auto* database = user_context->get(executor).transport.connection().native_handle();
            const auto* sql =
                "UPDATE platform_data SET name='root',version=3 WHERE id=1;"
                "INSERT INTO school_data(id,name,rank,platform,version) VALUES(7,'school',1,1,3),(8,'deleted school',2,1,-1),(9,'plain',3,1,3);"
                "INSERT INTO academy_data(id,name,rank,platform,version,campus_code) VALUES(7,'academy',7,1,4,'north'),(8,'deleted academy',8,1,-2,'south'),(10,'orphan academy',10,1,5,'orphan');"
                "INSERT INTO research_academy_data(id,name,rank,platform,version,campus_code,budget) VALUES(7,'research',17,1,5,'lab','12.50'),(8,'deleted research',18,1,-3,'old lab','0'),(11,'orphan research',11,1,5,'orphan lab','2.50');";
            if (sqlite3_exec(database, sql, nullptr, nullptr, nullptr) != SQLITE_OK) {
                throw std::runtime_error(sqlite3_errmsg(database));
            }
            if (require_value(user_context->require_active_root("Platform")).get().id != 1) {
                throw std::runtime_error(
                    "Generated initialization did not prepare the fixture root");
            }
            auto input = Json::parse(user_context->memory_resource(), line);
            if (!input) {
                throw std::runtime_error("Invalid inherited SQLite fixture");
            }
            std::cout << dispatch(*user_context, *input).dump(user_context->memory_resource())
                      << '\n';
        }
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
