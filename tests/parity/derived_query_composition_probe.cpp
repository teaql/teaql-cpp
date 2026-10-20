// Reuse the real SQLite observer without changing its qualified implementation.
// Renaming main removes C++'s implicit-return rule for this unused included driver.
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wreturn-type"
#define main derived_query_base_driver
#include "derived_query_diagnostics_probe.cpp"
#undef main
#pragma clang diagnostic pop

static teaql::request::QueryOptions
future_options(UserContext& user_context, std::string_view secret, std::string_view shape) {
    auto r = user_context.memory_resource();
    auto& private_query = user_context.make<SelectQuery>("Order");
    private_query.filter(Expr::eq(r, "name", secret));
    teaql::request::QuerySelection private_selection(
        std::allocator_arg, Allocator(r), private_query);
    auto& outer = user_context.make<SelectQuery>("Order");
    teaql::request::QuerySelection selection(std::allocator_arg, Allocator(r), outer);
    if (shape == "aggregate") {
        selection.query_options.relation_aggregates.emplace_back(
            "unused", "private_count", private_selection, true);
    } else if (shape == "nested") {
        selection.query_options.facets.emplace_back("nested", "unused", private_selection, true);
    } else {
        selection = std::move(private_selection);
    }
    teaql::request::QueryOptions options(std::allocator_arg, Allocator(r));
    options.facets.emplace_back("future", "unused", selection, true);
    return options;
}

static PurposedSelectQuery& composed_request(UserContext& user_context, const Json& input) {
    auto r = user_context.memory_resource();
    auto secret = text(field(input, "secret"));
    auto& query = user_context.make<SelectQuery>("Order");
    query.filter(Expr::eq(r, "status", "ACTIVE")).project("id").project("version").limit(1);
    auto& child = user_context.make<SelectQuery>("Order");
    child.filter(Expr::eq(r, "name", secret));
    query.relation_query("unused_relation", child);
    String comment("count ACTIVE including ", r), purpose("page ACTIVE with ", r);
    comment += secret;
    purpose += secret;
    if (!std::get<bool>(field(input, "invalid_intent").data)) {
        query.comment(comment);
    }
    auto* request = &user_context.make<PurposedSelectQuery>(query, purpose);
    request->for_exact_count("count");
    for (const auto& step : std::get<Json::Array>(field(input, "diagnostic_steps").data)) {
        auto action = text(field(step, "action"));
        if (action == "facet") {
            auto options = future_options(
                user_context, text(field(step, "secret")), text(field(step, "shape")));
            request->with_facet_diagnostics(options);
        } else if (action == "empty") {
            teaql::request::QueryOptions options(std::allocator_arg, Allocator(r));
            request->with_facet_diagnostics(options);
        } else if (action == "count") {
            request->for_exact_count(text(field(step, "alias")));
        } else if (action == "clone") {
            request = &user_context.make<PurposedSelectQuery>(*request);
        } else {
            throw std::runtime_error("Unknown diagnostic sequence action");
        }
    }
    return *request;
}

int main() {
    auto* previous = std::pmr::set_default_resource(std::pmr::null_memory_resource());
    try {
        for (std::string line; std::getline(std::cin, line);) {
            GlobalContext global;
            auto user_context = global.make_user_context();
            auto input = Json::parse(user_context->memory_resource(), line);
            if (!input) {
                throw std::runtime_error("Invalid diagnostic composition input");
            }
            auto& request = composed_request(*user_context, *input);
            std::cout
                << observe(*user_context, *input, &request).dump(user_context->memory_resource())
                << '\n';
        }
    } catch (const std::exception& error) {
        std::pmr::set_default_resource(previous);
        std::cerr << error.what() << '\n';
        return 1;
    }
    std::pmr::set_default_resource(previous);
}
