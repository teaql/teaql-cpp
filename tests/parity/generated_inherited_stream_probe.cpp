#include "../generated_inheritance/include/cpp_inheritance.hpp"
#include "fixture_value.hpp"
#include <iostream>
#include <sqlite3.h>

using namespace cpp_inheritance;

static void sql(sqlite3* database, const std::string& command) {
    if (sqlite3_exec(database, command.c_str(), nullptr, nullptr, nullptr) != SQLITE_OK) {
        throw std::runtime_error(sqlite3_errmsg(database));
    }
}

template <Entity T>
static Json native(Memory r, const T& entity) {
    auto values = std::move(detail::clone_in(r, entity)).into_values();
    Json::Object out(r);
    for (const auto& [name, value] : values.fields) {
        if (!name.starts_with('_')) {
            out.emplace(String(name, r), value.to_json_value(r));
        }
    }
    return Json(r, out);
}

template <Entity T, class Request>
static Json run(UserContext& user_context, Request request, const Json& input) {
    auto r = user_context.memory_resource();
    auto chunk = std::get<std::uint64_t>(field(input, "chunk").data);
    auto stop = std::get<std::uint64_t>(field(input, "stop").data);
    auto executable = std::move(request)
                          .template returnType<T>()
                          .orderByIdAsc()
                          .stream(chunk)
                          .purpose("Qualify inherited stream cleanup")
                          .comment("Early close and decode failure");
    const auto& result = executable.executeForStream(user_context).wait();
    Json::Array rows(r);
    Json error(r);
    if (!result) {
        error = Json(r, result.error().to_string(r));
    } else {
        auto& stream = result.value().get();
        for (std::uint64_t index = 0; index < stop; ++index) {
            const auto* next = stream.wait_next();
            if (!next) {
                break;
            }
            if (!*next) {
                error = Json(r, next->error().to_string(r));
                break;
            }
            rows.emplace_back(native(r, require_value(*next)));
        }
        stream.cancel();
    }
    auto Q = user_context.queries<query_factory>();
    // Cached prepared statements remain owned by the connection after cancellation.
    // An immediate real query observes lease availability without waiting on a
    // potentially leaked cursor lease or mistaking the cache for a live cursor.
    auto& reuse = Q.schools()
                      .withIdIs(7)
                      .purpose("Reuse executor after stream")
                      .comment("No remaining cursor lease")
                      .executeForOne(user_context);
    const auto* one = reuse.poll();
    bool available = one != nullptr;
    bool reused = one && *one && bool(one->value());
    Json::Object out(r);
    out.emplace(String("rows", r), Json(r, rows));
    out.emplace(String("error", r), std::move(error));
    out.emplace(String("available", r), Json(r, available));
    out.emplace(String("reused", r), Json(r, reused));
    return Json(r, out);
}

template <class Request>
static Json target(UserContext& user_context, Request request, const Json& input) {
    auto model = text(field(input, "target"));
    if (model == "school") {
        return run<School>(user_context, std::move(request), input);
    }
    if (model == "academy") {
        return run<Academy>(user_context, std::move(request), input);
    }
    return run<ResearchAcademy>(user_context, std::move(request), input);
}

static Json observe(UserContext& user_context, const Json& input) {
    auto Q = user_context.queries<query_factory>();
    auto entry = text(field(input, "entry"));
    auto model = text(field(input, "source"));
    if (model == "school") {
        return target(user_context, entry == "minimal" ? Q.schoolsMinimal() : Q.schools(), input);
    }
    if (model == "academy") {
        return target(
            user_context, entry == "minimal" ? Q.academiesMinimal() : Q.academies(), input);
    }
    return target(user_context,
                  entry == "minimal" ? Q.researchAcademiesMinimal() : Q.researchAcademies(),
                  input);
}

int main() {
    try {
        for (std::string line; std::getline(std::cin, line);) {
            GlobalContext global;
            auto user_context = global.make_user_context();
            auto executor = setup(*user_context);
            auto* database = user_context->get(executor).transport.connection().native_handle();
            sql(database,
                "UPDATE platform_data SET name='root',version=3 WHERE id=1;"
                "INSERT INTO school_data(id,name,rank,platform,version) VALUES(7,'school',1,1,3),(8,'next',2,1,3),(9,'last',3,1,3);"
                "INSERT INTO academy_data(id,name,rank,platform,version,campus_code) VALUES(7,'academy',7,1,4,'north'),(8,'next',8,1,4,'south'),(9,'last',9,1,4,'east');"
                "INSERT INTO research_academy_data(id,name,rank,platform,version,campus_code,budget) VALUES(7,'research',17,1,5,'lab','12.50'),(8,'next',18,1,5,'lab','10.50'),(9,'last',19,1,5,'lab','8.50');");
            if (require_value(user_context->require_active_root("Platform")).get().id != 1) {
                throw std::runtime_error(
                    "Generated initialization did not prepare the fixture root");
            }
            auto input = Json::parse(user_context->memory_resource(), line);
            if (!input) {
                throw std::runtime_error("Invalid inherited stream fixture");
            }
            if (text(field(*input, "fault")) == "bad_rank") {
                auto model = text(field(*input, "source"));
                auto table = model == "school"    ? "school_data"
                             : model == "academy" ? "academy_data"
                                                  : "research_academy_data";
                sql(database,
                    "UPDATE " + std::string(table) + " SET rank='invalid rank' WHERE id=8");
            }
            std::cout << observe(*user_context, *input).dump(user_context->memory_resource())
                      << '\n';
        }
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
