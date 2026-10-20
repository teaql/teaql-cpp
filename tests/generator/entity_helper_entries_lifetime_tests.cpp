#include "../generated_relations/include/cpp_relations.hpp"
#include <iostream>

using namespace cpp_relations;

static void require(bool condition, const char* message) {
    if (!condition) {
        throw std::runtime_error(message);
    }
}

template <class Model>
static void verify_cross_user() {
    for (auto mode : {"partial", "compact", "columns", "indexed"}) {
        GlobalContext global;
        auto source_user_context = global.make_user_context();
        auto target_user_context = global.make_user_context();
        auto& model = target_user_context->make<Model>();
        model.updateName("unchanged");
        auto r = source_user_context->memory_resource();
        {
            String long_alias("#", r);
            long_alias.append(256, 'x');
            std::array<std::string_view, 2> names{"name", long_alias};
            auto state = std::string_view(mode) == "compact"
                             ? eval::LoadState::partial_compact(r, names)
                             : eval::LoadState::partial(r, names);
            if (std::string_view(mode) == "columns") {
                auto& columns = source_user_context->make<ColumnLayout>(
                    std::initializer_list<std::string_view>{"name", long_alias});
                state = eval::LoadState::shared_columns(r, columns);
            } else if (std::string_view(mode) == "indexed") {
                state = state
                            .into_indexed(detail::ContextArena::from_memory(r).loaded_snapshots(),
                                          Model::field_layout(r))
                            .value();
            }
            model.setLoadState(state);
        }
        source_user_context.reset();
        auto copy = target_user_context->make_global<Model>(model);
        {
            String alias("#", target_user_context->memory_resource());
            alias.append(256, 'x');
            require(model.isLoaded("name") && model.isLoaded(alias),
                    "load-state names must belong to the destination after source User deletion");
            require(!model.isLoaded("id") && model.name() == "unchanged",
                    "availability replacement preserves values and excludes unselected fields");
            auto changed = model.changedName();
            require(changed && changed->try_text() == "unchanged",
                    "load-state replacement must not stage another mutation");
            require(target_user_context->get(copy).isLoaded(alias),
                    "explicit Global model copy retains selected names");
        }
        auto retained_ref = copy;
        target_user_context.reset();
        auto next_user_context = global.make_user_context();
        const auto& global_model =
            next_user_context->get(next_user_context->rebind_global(retained_ref));
        require(global_model.isLoaded("name") && global_model.name() == "unchanged",
                "Global model copy survives actual destination User deletion");
    }
}

int main() {
    std::pmr::set_default_resource(std::pmr::null_memory_resource());
    try {
        verify_cross_user<Platform>();
        verify_cross_user<School>();
        verify_cross_user<Device>();
        std::cout << "entity helper allocation and lifetime controls passed\n";
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
