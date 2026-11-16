#include <cpp_inheritance_graph.hpp>
#include <teaql/runtime.hpp>

#include <iostream>
#include <memory_resource>
#include <stdexcept>
#include <string>

using namespace teaql;
using namespace cpp_inheritance_graph;

template <class Model>
Json describe(Memory resource) {
    auto descriptor = Model::entity_descriptor(resource);
    Json::Object row(resource);
    row.emplace("owner", Json(resource, descriptor.name));
    row.emplace("limit",
                descriptor.audit_value_max_len
                    ? Json(resource, std::to_string(*descriptor.audit_value_max_len))
                    : Json(resource));
    Json::Array relations(resource);
    for (const auto& relation : descriptor.relations) {
        Json::Object item(resource);
        item.emplace("name", Json(resource, relation.name));
        item.emplace("target", Json(resource, relation.target_entity));
        item.emplace("many", Json(resource, relation.many));
        relations.emplace_back(Json(resource, std::move(item)));
    }
    row.emplace("relations", Json(resource, std::move(relations)));
    return Json(resource, std::move(row));
}

int main() {
    std::pmr::set_default_resource(std::pmr::null_memory_resource());
    GlobalContext global;
    auto user_context = global.make_user_context();
    {
        auto resource = user_context->memory_resource();
        Json::Array descriptors(resource);
        descriptors.emplace_back(describe<Platform>(resource));
        descriptors.emplace_back(describe<Specialist>(resource));
        descriptors.emplace_back(describe<School>(resource));
        descriptors.emplace_back(describe<Academy>(resource));
        descriptors.emplace_back(describe<ResearchAcademy>(resource));
        descriptors.emplace_back(describe<Enrollment>(resource));
        std::cout << Json(resource, std::move(descriptors)).dump(resource) << '\n';
    }
    user_context.reset();
    if (global.live_user_bytes() != 0) {
        throw std::runtime_error("Generated compiler projections retained UserContext memory");
    }
}
