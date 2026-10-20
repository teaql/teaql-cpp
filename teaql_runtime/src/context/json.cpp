#include <teaql/runtime/context.hpp>
#include <teaql/runtime/detail/context/entity.hpp>
#include <algorithm>
#include <cmath>
#include <limits>

namespace teaql::detail {
EntityError json_input_error(Memory r, std::string_view entity, std::string_view message) {
    String text("JSON_ENTITY_INPUT: ", r);
    text += message;
    return EntityError(std::allocator_arg, Allocator(r), entity, text);
}

namespace {
bool null(const Json& value) {
    return std::holds_alternative<std::monostate>(value.data);
}

bool object(const Json& value) {
    return std::holds_alternative<Json::Object>(value.data);
}

bool relation_input(const Json& value) {
    return null(value) || object(value) || std::holds_alternative<Json::Array>(value.data);
}

const Json* get(const Json& value, std::string_view name) {
    const auto* fields = std::get_if<Json::Object>(&value.data);
    if (!fields) {
        return nullptr;
    }
    auto found = fields->find(name);
    return found == fields->end() ? nullptr : &found->second;
}

const PropertyDescriptor* property(const EntityDescriptor& descriptor, std::string_view name) {
    auto found = std::ranges::find_if(descriptor.properties, [&](const auto& p) {
        return p.name == name || p.column_name == name;
    });
    return found == descriptor.properties.end() ? nullptr : &*found;
}

bool same_key(const Value& left, const Value& right) {
    auto integer = [](const Value& value) {
        return std::holds_alternative<std::int64_t>(value.data) ||
               std::holds_alternative<std::uint64_t>(value.data);
    };
    return left == right ||
           (integer(left) && integer(right) && left.try_u64() && left.try_u64() == right.try_u64());
}

std::optional<std::int64_t> signed_number(const Json& value) {
    if (auto* v = std::get_if<std::int64_t>(&value.data)) {
        return *v;
    }
    if (auto* v = std::get_if<std::uint64_t>(&value.data); v && *v <= INT64_MAX) {
        return static_cast<std::int64_t>(*v);
    }
    return {};
}

Value scalar(Memory r,
             const EntityDescriptor& descriptor,
             const PropertyDescriptor& field,
             const Json& value) {
    auto invalid = [&]() -> Value {
        String message("invalid field type: ", r);
        message += field.name;
        throw json_input_error(r, descriptor.name, message);
    };
    if (null(value) && field.nullable) {
        return Value(r);
    }
    const auto* text = std::get_if<String>(&value.data);
    switch (field.data_type) {
    case DataType::Bool:
        if (auto* v = std::get_if<bool>(&value.data)) {
            return Value(r, *v);
        }
        break;
    case DataType::I64:
        if (auto v = signed_number(value)) {
            return Value(r, *v);
        }
        break;
    case DataType::U64:
        if (auto* v = std::get_if<std::uint64_t>(&value.data)) {
            return Value(r, *v);
        }
        if (auto* v = std::get_if<std::int64_t>(&value.data); v && *v >= 0) {
            return Value(r, static_cast<std::uint64_t>(*v));
        }
        break;
    case DataType::F64: {
        std::optional<double> number;
        if (auto* v = std::get_if<double>(&value.data)) {
            number = *v;
        } else if (auto* v = std::get_if<std::uint64_t>(&value.data)) {
            number = static_cast<double>(*v);
        } else if (auto* v = std::get_if<std::int64_t>(&value.data)) {
            number = static_cast<double>(*v);
        }
        if (number && std::isfinite(*number)) {
            return Value(r, *number);
        }
        break;
    }
    case DataType::Decimal:
        if (text || std::holds_alternative<double>(value.data) ||
            std::holds_alternative<std::int64_t>(value.data) ||
            std::holds_alternative<std::uint64_t>(value.data)) {
            auto decoded =
                Decimal::parse(text ? std::string_view(*text) : std::string_view(value.dump(r)));
            if (decoded) {
                return Value(r, *decoded);
            }
        }
        break;
    case DataType::Text:
    case DataType::LargeText:
        if (text) {
            return Value(r, *text);
        }
        break;
    case DataType::Json:
        if (!null(value)) {
            return Value(r, value);
        }
        break;
    case DataType::Date:
        if (text) {
            if (auto date = Date::parse(*text)) {
                return Value(r, *date);
            }
        }
        break;
    case DataType::Timestamp:
        if (auto millis = signed_number(value)) {
            return Value(r, Timestamp(*millis));
        }
        // Do not accept Timestamp::parse's additional zone-less convenience form.
        if (text && text->size() >= 20 &&
            ((*text)[10] == 'T' || (*text)[10] == 't' || (*text)[10] == ' ')) {
            if (auto timestamp = Timestamp::parse(*text)) {
                return Value(r, *timestamp);
            }
        }
        break;
    }
    return invalid();
}
}

JsonReadSession::JsonReadSession(const UserContext& context)
    : context_(context), resource_(context.memory_resource()), shapes_(resource_) {}

Result<const EntityDescriptor*, EntityError>
JsonReadSession::descriptor(const EntityDescriptor& expected) const {
    using Output = Result<const EntityDescriptor*, EntityError>;
    auto* installed = context_.entity(expected.name);
    if (!installed) {
        return Output::err(
            resource_,
            json_input_error(resource_, expected.name, "entity is not installed in this context"));
    }
    if (installed->properties.size() != expected.properties.size() ||
        std::ranges::any_of(expected.properties,
                            [&](const auto& p) {
                                return std::ranges::find(installed->properties, p) ==
                                       installed->properties.end();
                            }) ||
        installed->relations.size() != expected.relations.size() ||
        std::ranges::any_of(expected.relations, [&](const auto& r) {
            return std::ranges::find(installed->relations, r) == installed->relations.end();
        })) {
        return Output::err(resource_,
                           json_input_error(resource_,
                                            expected.name,
                                            "installed metadata does not match the typed entity"));
    }
    return Output::ok(resource_, installed);
}

CompactRow JsonReadSession::scalar_row(const EntityDescriptor& descriptor,
                                       const Json& input,
                                       bool dynamic,
                                       bool graph) {
    auto fail = [&](std::string_view message) {
        throw json_input_error(resource_, descriptor.name, message);
    };
    const auto* fields = std::get_if<Json::Object>(&input.data);
    if (!fields) {
        fail("expected entity object");
    }
    Vector<std::pair<std::string_view, Value>> values(resource_);
    for (const auto& [name, value] : *fields) {
        if (name == "_comment" || name == "_dirty_fields" || name == "_original_values" ||
            name == "_is_new" || name == "_is_deleted" || name == "__load_state" ||
            name == "__teaql_runtime_state") {
            fail("incoming runtime state is forbidden");
        }
        if (name.starts_with('#')) {
            fail("persistent fields require trusted definitions and storage provenance");
        }
        if (name.starts_with('_')) {
            if (name.size() == 1 || !dynamic) {
                fail("entity lacks a readonly dynamic-property carrier");
            }
            values.emplace_back(name, null(value) ? Value(resource_) : Value(resource_, value));
            continue;
        }
        const bool relation = std::ranges::any_of(descriptor.relations, [&](const auto& r) {
            return r.name == name;
        });
        if (relation && relation_input(value)) {
            if (graph) {
                continue;
            }
            fail("relation input requires graph-aware hydration");
        }
        auto match = std::ranges::find_if(descriptor.properties, [&](const auto& candidate) {
            const auto& column =
                candidate.json_column_name ? *candidate.json_column_name : candidate.column_name;
            return candidate.name == name || column == name;
        });
        const auto* p = match == descriptor.properties.end() ? nullptr : &*match;
        if (!p) {
            String message("unknown model field: ", resource_);
            message += name;
            fail(message);
        }
        auto converted = scalar(resource_, descriptor, *p, value);
        values.emplace_back(p->name, std::move(converted));
    }
    std::ranges::sort(values, {}, &std::pair<std::string_view, Value>::first);
    for (std::size_t i = 1; i < values.size(); ++i) {
        if (values[i - 1].first == values[i].first) {
            String message("duplicate aliases for field: ", resource_);
            message += values[i].first;
            fail(message);
        }
    }
    Vector<String> names(resource_);
    Vector<Value> payload(resource_);
    for (auto& [name, value] : values) {
        names.emplace_back(name);
        payload.emplace_back(std::move(value));
    }
    auto found = shapes_.find(names);
    const ColumnLayout* layout;
    if (found != shapes_.end()) {
        layout = found->second;
    } else {
        layout = &resource_arena(resource_).make_payload<ColumnLayout>(std::move(names));
        Vector<String> key(resource_);
        for (const auto& name : layout->names()) {
            key.emplace_back(name);
        }
        shapes_.emplace(std::move(key), layout);
    }
    return CompactRow(std::allocator_arg, Allocator(resource_), *layout, std::move(payload));
}

struct JsonReadSession::Parsed {
    struct Edge {
        const RelationDescriptor* relation;
        std::uint64_t owner_id;
        Vector<Parsed> children;
    };

    std::uint64_t view;
    const EntityDescriptor* descriptor;
    CompactRow row;
    Vector<Edge> edges;
};

struct JsonReadSession::Plan {
    explicit Plan(Memory r)
        : capabilities(r), native_views(r), relation_shapes(std::allocator_arg, Allocator(r)) {}

    std::size_t nodes = 0;
    std::pmr::map<std::string_view, bool> capabilities;
    std::pmr::map<std::pair<std::string_view, std::uint64_t>, Vector<const Json*>> native_views;
    RelationShapeCache relation_shapes;
};

JsonReadSession::Parsed JsonReadSession::parse_graph(const EntityDescriptor& descriptor,
                                                     const Json& value,
                                                     Plan& plan,
                                                     std::size_t depth) {
    const auto view = ++plan.nodes;
    auto fail = [&](std::string_view message) {
        throw json_input_error(resource_, descriptor.name, message);
    };
    if (depth > 64 || plan.nodes > 100000) {
        fail("graph input exceeds depth or node limit");
    }
    auto capability = [&](const EntityDescriptor& entity) {
        auto found = plan.capabilities.find(entity.name);
        if (found != plan.capabilities.end()) {
            return found->second;
        }
        auto result = ContextEntityAccess::json_graph_capability(context_, entity);
        if (!result) {
            throw std::move(result).error();
        }
        plan.capabilities.emplace(entity.name, result.value());
        return result.value();
    };
    const bool dynamic = capability(descriptor);
    auto row = scalar_row(descriptor, value, dynamic, true);
    Vector<Parsed::Edge> edges(resource_);
    for (const auto& relation : descriptor.relations) {
        const auto* payload = get(value, relation.name);
        if (!payload || !relation_input(*payload)) {
            continue;
        }
        auto identity = std::ranges::find_if(descriptor.properties, [](const auto& p) {
            return p.is_id;
        });
        if (identity == descriptor.properties.end()) {
            fail("graph owner lacks an identity definition");
        }
        const auto* id_value = row.get(identity->name);
        auto owner_id = id_value ? id_value->try_u64() : std::nullopt;
        if (!owner_id) {
            fail("graph relation requires an explicitly loaded owner id");
        }
        const auto* target = context_.entity(relation.target_entity);
        if (!target) {
            fail("relation target is not installed");
        }
        (void)capability(*target);
        Vector<Parsed> children(resource_);
        if (relation.many) {
            const auto* array = std::get_if<Json::Array>(&payload->data);
            if (!array) {
                fail("to-many relation requires an array");
            }
            if (array->size() > 100000 - plan.nodes) {
                fail("graph input exceeds depth or node limit");
            }
            children.reserve(array->size());
            for (const auto& child : *array) {
                if (!object(child)) {
                    fail("relation item must be an entity object");
                }
                children.emplace_back(parse_graph(*target, child, plan, depth + 1));
            }
        } else if (object(*payload)) {
            if (plan.nodes == 100000) {
                fail("graph input exceeds depth or node limit");
            }
            children.emplace_back(parse_graph(*target, *payload, plan, depth + 1));
        } else if (!null(*payload)) {
            fail("to-one relation requires object or null");
        }
        const auto* local = property(descriptor, relation.local_key);
        if (!local) {
            fail("relation local key is not a model property");
        }
        if (!relation.many && !local->is_id) {
            Value supplied(resource_);
            if (!children.empty()) {
                const auto* foreign = property(*target, relation.foreign_key);
                if (!foreign) {
                    throw json_input_error(
                        resource_, target->name, "relation foreign key is not a model property");
                }
                const auto* key = children.front().row.get(foreign->name);
                if (!key) {
                    throw json_input_error(
                        resource_, target->name, "relation target key was not loaded");
                }
                supplied = clone_in(resource_, *key);
            }
            if (const auto* existing = row.get(local->name)) {
                if (!same_key(*existing, supplied)) {
                    fail("relation target conflicts with the loaded foreign key");
                }
            } else {
                row.insert(
                    local->name,
                    scalar(resource_, descriptor, *local, supplied.to_json_value(resource_)));
            }
        }
        if (relation.many || local->is_id) {
            const auto* foreign = property(*target, relation.foreign_key);
            if (!foreign) {
                throw json_input_error(
                    resource_, target->name, "relation foreign key is not a model property");
            }
            if (const auto* expected = row.get(local->name)) {
                for (const auto& child : children) {
                    if (const auto* actual = child.row.get(foreign->name);
                        actual && !same_key(*expected, *actual)) {
                        fail("relation child conflicts with the loaded join key");
                    }
                }
            }
        }
        row.mark_relation_loaded(relation.name, plan.relation_shapes);
        edges.emplace_back(Parsed::Edge{&relation, *owner_id, std::move(children)});
    }
    auto identity = std::ranges::find_if(descriptor.properties, [](const auto& p) {
        return p.is_id;
    });
    auto id = identity == descriptor.properties.end() ? std::nullopt
              : row.get(identity->name)               ? row.get(identity->name)->try_u64()
                                                      : std::nullopt;
    if (id) {
        auto [found, inserted] = plan.native_views.try_emplace({descriptor.name, *id});
        for (const auto* previous : found->second) {
            auto prior = inferred_row(descriptor, *previous, dynamic);
            for (const auto& p : descriptor.properties) {
                const auto* left = prior.get(p.name);
                const auto* right = row.get(p.name);
                if (left && right && !same_key(*left, *right)) {
                    fail("conflicting native values for one identity");
                }
            }
        }
        found->second.push_back(&value);
    }
    return {view, &descriptor, std::move(row), std::move(edges)};
}

CompactRow
JsonReadSession::inferred_row(const EntityDescriptor& descriptor, const Json& input, bool dynamic) {
    auto row = scalar_row(descriptor, input, dynamic, true);
    for (const auto& relation : descriptor.relations) {
        if (relation.many) {
            continue;
        }
        const auto* local = property(descriptor, relation.local_key);
        if (!local || local->is_id || row.get(local->name)) {
            continue;
        }
        const auto* payload = get(input, relation.name);
        if (!payload) {
            continue;
        }
        if (null(*payload)) {
            row.insert(local->name, Value(resource_));
            continue;
        }
        if (!object(*payload)) {
            continue;
        }
        const auto* target = context_.entity(relation.target_entity);
        if (!target) {
            throw json_input_error(resource_, descriptor.name, "relation target is not installed");
        }
        auto capability = ContextEntityAccess::json_graph_capability(context_, *target);
        if (!capability) {
            throw std::move(capability).error();
        }
        auto target_row = scalar_row(*target, *payload, capability.value(), true);
        const auto* foreign = property(*target, relation.foreign_key);
        if (foreign) {
            if (const auto* key = target_row.get(foreign->name)) {
                row.insert(local->name, clone_in(resource_, *key));
            }
        }
    }
    return row;
}

std::pair<std::uint64_t, CompactRow> JsonReadSession::install_edges(Parsed&& parsed,
                                                                    const EntityRuntimeState& root,
                                                                    EntityGraphBuilder& graph) {
    auto owner =
        EntityRuntimeAccess::fresh_with_weak_graph(resource_, root).with_json_view(parsed.view);
    for (auto& edge : parsed.edges) {
        InMemoryEntityGraphDecoderRegistry::JsonRows rows(resource_);
        rows.reserve(edge.children.size());
        for (auto& child : edge.children) {
            rows.emplace_back(install_edges(std::move(child), root, graph));
        }
        auto result = ContextEntityAccess::decode_json_edge(
            context_,
            edge.relation->target_entity,
            std::move(rows),
            owner,
            graph,
            {parsed.descriptor->name, edge.owner_id, edge.relation->name, edge.relation->many});
        if (!result) {
            throw std::move(result).error();
        }
    }
    return {parsed.view, std::move(parsed.row)};
}

Result<CompactRow, EntityError> JsonReadSession::decode(const EntityDescriptor& descriptor,
                                                        const Json& value,
                                                        bool dynamic,
                                                        std::optional<EntityRuntimeState>& state) {
    using Output = Result<CompactRow, EntityError>;
    try {
        const bool graph_input =
            std::ranges::any_of(descriptor.relations, [&](const auto& relation) {
                const auto* payload = get(value, relation.name);
                return payload && relation_input(*payload);
            });
        if (!graph_input) {
            return Output::ok(resource_, scalar_row(descriptor, value, dynamic, false));
        }
        Plan plan(resource_);
        plan.capabilities.emplace(descriptor.name, dynamic);
        auto parsed = parse_graph(descriptor, value, plan, 0);
        EntityRuntimeState root(std::allocator_arg, Allocator(resource_));
        auto graph = EntityGraphBuilder::for_json_read(resource_);
        auto [view, row] = install_edges(std::move(parsed), root, graph);
        if (!root.freeze_graph(std::move(graph))) {
            throw json_input_error(resource_, descriptor.name, "graph could not be frozen");
        }
        state.emplace(std::move(root).with_json_view(view));
        return Output::ok(resource_, std::move(row));
    } catch (const EntityError& error) {
        return Output::err(resource_, clone_in(resource_, error));
    }
}
}
