#define main retained_document_probe_main
#include "document_round_trip_probe.cpp"
#undef main
#include <teaql/runtime/document_model_debug.hpp>

static std::uint64_t document_seconds(const Json& input) {
    const auto& value = field(input, "seconds");
    if (auto result = std::get_if<std::uint64_t>(&value.data)) {
        return *result;
    }
    return static_cast<std::uint64_t>(std::get<std::int64_t>(value.data));
}

template <class T>
static Json model_debug(Memory r,
                        GlobalContext& global,
                        Vector<ContextFunction<String(Memory)>>& retained,
                        Vector<String>& expected,
                        const T& value) {
    auto& copy = global.make<T>(value);
    expected.emplace_back(debug_string(global.memory_resource(), value));
    retained.emplace_back([pointer = &copy](Memory memory) {
        return debug_string(memory, *pointer);
    });
    Json::Object result(r);
    result.emplace(String("debug", r), Json(r, debug_string(r, value)));
    DebugOptions pretty;
    pretty.pretty = true;
    result.emplace(String("pretty", r), Json(r, debug_string(r, value, pretty)));
    result.emplace(String("clone_equal", r), Json(r, copy == value));
    return Json(r, result);
}

template <class T, class F>
static bool changed_equal(Memory r, const T& original, F change) {
    auto copy = detail::clone_in(r, original);
    change(copy);
    return original == copy;
}

static Json document_model_observe(UserContext& user_context,
                                   GlobalContext& global,
                                   Vector<ContextFunction<String(Memory)>>& retained,
                                   Vector<String>& expected,
                                   const Json& input) {
    auto r = user_context.memory_resource();
    auto document_snapshot = snapshot(r, input);
    const auto& entity = document_snapshot.entities.front();
    const auto& relation = document_snapshot.relations.front();
    auto token = text(field(input, "token"));
    auto command = text(field(input, "command"));
    const auto& body = field(input, "body");
    ContextualDocument document(
        std::allocator_arg, Allocator(r), document_snapshot.business_id, token, body);
    SubmittedContextualDocument submitted(
        std::allocator_arg, Allocator(r), document_snapshot.business_id, token, command, body);
    DocumentOpenRequest open_request(
        std::allocator_arg,
        Allocator(r),
        document_snapshot.scope.aggregate_type,
        document_snapshot.business_id,
        document_snapshot.scope.purpose,
        ReferenceLifetime(document_seconds(input), std::uint32_t(number(input, "nanos"))));
    DocumentAcceptRequest accept_request(std::allocator_arg,
                                         Allocator(r),
                                         document_snapshot.scope.aggregate_type,
                                         document_snapshot.scope.purpose,
                                         submitted);
    Vector<ReferenceIdentity> changed(r);
    for (const auto& item : document_snapshot.entities) {
        changed.emplace_back(item.identity);
    }
    AcceptedContextualDocument accepted(std::allocator_arg,
                                        Allocator(r),
                                        document,
                                        document_snapshot.aggregate.version,
                                        document_snapshot.aggregate.version + 1,
                                        changed);
    VerifiedDocumentSnapshot verified(
        std::allocator_arg,
        Allocator(r),
        document_snapshot,
        text(field(input, "key_id")),
        *UtcDateTime::from_timestamp(number(input, "issued"),
                                     std::uint32_t(number(input, "timestamp_nanos"))),
        *UtcDateTime::from_timestamp(number(input, "now"),
                                     std::uint32_t(number(input, "timestamp_nanos"))));
    Json::Object output(r);
    output.emplace(String("entity", r), model_debug(r, global, retained, expected, entity));
    {
        Json::Array values(r);
        values.emplace_back(changed_equal(r, entity, [&](auto& v) {
            v.row_key += "!";
        }));
        values.emplace_back(changed_equal(r, entity, [&](auto& v) {
            v.identity.entity_type += "!";
        }));
        values.emplace_back(changed_equal(r, entity, [&](auto& v) {
            v.identity.id ^= 1;
        }));
        values.emplace_back(changed_equal(r, entity, [&](auto& v) {
            v.identity.version ^= 1;
        }));
        values.emplace_back(changed_equal(r, entity, [&](auto& v) {
            v.loaded_fields.clear();
        }));
        values.emplace_back(changed_equal(r, entity, [&](auto& v) {
            v.writable_fields.clear();
        }));
        values.emplace_back(changed_equal(r, entity, [&](auto& v) {
            v.allowed_mutations.clear();
        }));
        output.emplace(String("entity_equality", r), Json(r, values));
    }
    output.emplace(String("relation", r), model_debug(r, global, retained, expected, relation));
    {
        Json::Array values(r);
        values.emplace_back(changed_equal(r, relation, [&](auto& v) {
            v.relation_name += "!";
        }));
        values.emplace_back(changed_equal(r, relation, [&](auto& v) {
            std::reverse(v.row_keys.begin(), v.row_keys.end());
        }));
        values.emplace_back(changed_equal(r, relation, [&](auto& v) {
            v.writable = !v.writable;
        }));
        values.emplace_back(changed_equal(r, relation, [&](auto& v) {
            v.completeness = v.completeness == DocumentRelationCompleteness::Complete
                                 ? DocumentRelationCompleteness::Partial
                                 : DocumentRelationCompleteness::Complete;
        }));
        values.emplace_back(changed_equal(r, relation, [&](auto& v) {
            v.allowed_mutations.clear();
        }));
        output.emplace(String("relation_equality", r), Json(r, values));
    }
    output.emplace(String("document_snapshot", r),
                   model_debug(r, global, retained, expected, document_snapshot));
    {
        Json::Array values(r);
        values.emplace_back(changed_equal(r, document_snapshot, [&](auto& v) {
            v.model_fingerprint += "!";
        }));
        values.emplace_back(changed_equal(r, document_snapshot, [&](auto& v) {
            v.business_id += "!";
        }));
        values.emplace_back(changed_equal(r, document_snapshot, [&](auto& v) {
            v.scope.document_id += "!";
        }));
        values.emplace_back(changed_equal(r, document_snapshot, [&](auto& v) {
            v.scope.purpose += "!";
        }));
        values.emplace_back(changed_equal(r, document_snapshot, [&](auto& v) {
            v.scope.aggregate_type += "!";
        }));
        values.emplace_back(changed_equal(r, document_snapshot, [&](auto& v) {
            v.scope.aggregate_id ^= 1;
        }));
        values.emplace_back(changed_equal(r, document_snapshot, [&](auto& v) {
            v.scope.aggregate_revision ^= 1;
        }));
        values.emplace_back(changed_equal(r, document_snapshot, [&](auto& v) {
            v.aggregate.entity_type += "!";
        }));
        values.emplace_back(changed_equal(r, document_snapshot, [&](auto& v) {
            v.aggregate.id ^= 1;
        }));
        values.emplace_back(changed_equal(r, document_snapshot, [&](auto& v) {
            v.aggregate.version ^= 1;
        }));
        values.emplace_back(changed_equal(r, document_snapshot, [&](auto& v) {
            v.entities.pop_back();
        }));
        values.emplace_back(changed_equal(r, document_snapshot, [&](auto& v) {
            v.relations.pop_back();
        }));
        output.emplace(String("document_snapshot_equality", r), Json(r, values));
    }
    output.emplace(String("document", r), model_debug(r, global, retained, expected, document));
    {
        Json::Array values(r);
        values.emplace_back(changed_equal(r, document, [&](auto& v) {
            v.business_id += "!";
        }));
        values.emplace_back(changed_equal(r, document, [&](auto& v) {
            v.document_token += "!";
        }));
        values.emplace_back(changed_equal(r, document, [&](auto& v) {
            v.body = Json(r);
        }));
        output.emplace(String("document_equality", r), Json(r, values));
    }
    output.emplace(String("submitted", r), model_debug(r, global, retained, expected, submitted));
    {
        Json::Array values(r);
        values.emplace_back(changed_equal(r, submitted, [&](auto& v) {
            v.business_id += "!";
        }));
        values.emplace_back(changed_equal(r, submitted, [&](auto& v) {
            v.document_token += "!";
        }));
        values.emplace_back(changed_equal(r, submitted, [&](auto& v) {
            v.command_id += "!";
        }));
        values.emplace_back(changed_equal(r, submitted, [&](auto& v) {
            v.body = Json(r);
        }));
        output.emplace(String("submitted_equality", r), Json(r, values));
    }
    output.emplace(String("open_request", r),
                   model_debug(r, global, retained, expected, open_request));
    {
        Json::Array values(r);
        values.emplace_back(changed_equal(r, open_request, [&](auto& v) {
            v.aggregate_type += "!";
        }));
        values.emplace_back(changed_equal(r, open_request, [&](auto& v) {
            v.business_id += "!";
        }));
        values.emplace_back(changed_equal(r, open_request, [&](auto& v) {
            v.purpose += "!";
        }));
        values.emplace_back(changed_equal(r, open_request, [&](auto& v) {
            v.lifetime.seconds ^= 1;
        }));
        values.emplace_back(changed_equal(r, open_request, [&](auto& v) {
            v.lifetime.nanoseconds ^= 1;
        }));
        output.emplace(String("open_request_equality", r), Json(r, values));
    }
    output.emplace(String("accept_request", r),
                   model_debug(r, global, retained, expected, accept_request));
    {
        Json::Array values(r);
        values.emplace_back(changed_equal(r, accept_request, [&](auto& v) {
            v.expected_aggregate_type += "!";
        }));
        values.emplace_back(changed_equal(r, accept_request, [&](auto& v) {
            v.purpose += "!";
        }));
        values.emplace_back(changed_equal(r, accept_request, [&](auto& v) {
            v.submitted.command_id += "!";
        }));
        output.emplace(String("accept_request_equality", r), Json(r, values));
    }
    output.emplace(String("accepted", r), model_debug(r, global, retained, expected, accepted));
    {
        Json::Array values(r);
        values.emplace_back(changed_equal(r, accepted, [&](auto& v) {
            v.document.document_token += "!";
        }));
        values.emplace_back(changed_equal(r, accepted, [&](auto& v) {
            v.previous_revision ^= 1;
        }));
        values.emplace_back(changed_equal(r, accepted, [&](auto& v) {
            v.new_revision ^= 1;
        }));
        values.emplace_back(changed_equal(r, accepted, [&](auto& v) {
            v.changed_entities.clear();
        }));
        output.emplace(String("accepted_equality", r), Json(r, values));
    }
    output.emplace(String("verified", r), model_debug(r, global, retained, expected, verified));
    {
        Json::Array values(r);
        values.emplace_back(changed_equal(r, verified, [&](auto& v) {
            v.snapshot.business_id += "!";
        }));
        values.emplace_back(changed_equal(r, verified, [&](auto& v) {
            v.key_id += "!";
        }));
        values.emplace_back(changed_equal(r, verified, [&](auto& v) {
            v.issued_at = *UtcDateTime::from_timestamp(v.issued_at.timestamp() ^ 1,
                                                       v.issued_at.timestamp_subsec_nanos());
        }));
        values.emplace_back(changed_equal(r, verified, [&](auto& v) {
            v.expires_at = *UtcDateTime::from_timestamp(v.expires_at.timestamp() ^ 1,
                                                        v.expires_at.timestamp_subsec_nanos());
        }));
        output.emplace(String("verified_equality", r), Json(r, values));
    }
    Json::Array errors(r);
    for (auto error : {DocumentRoundTripError::invalid(),
                       DocumentRoundTripError::expired(),
                       DocumentRoundTripError::scope_mismatch(),
                       DocumentRoundTripError::authorization_required(),
                       DocumentRoundTripError::configuration(),
                       DocumentRoundTripError::revision_conflict(),
                       DocumentRoundTripError::projection_violation(),
                       DocumentRoundTripError::validation_failed(),
                       DocumentRoundTripError::not_found()}) {
        auto value = model_debug(r, global, retained, expected, error);
        auto& fields = std::get<Json::Object>(value.data);
        fields.emplace(String("display", r), Json(r, error.what()));
        fields.emplace(String("source_none", r), Json(r, error.source() == nullptr));
        errors.emplace_back(std::move(value));
    }
    output.emplace(String("errors", r), Json(r, errors));
    Json::Array variants(r);
    for (auto value : {DocumentMutationKind::Update,
                       DocumentMutationKind::CreateChild,
                       DocumentMutationKind::RemoveChild}) {
        variants.emplace_back(model_debug(r, global, retained, expected, value));
    }
    for (auto value :
         {DocumentRelationCompleteness::Complete, DocumentRelationCompleteness::Partial}) {
        variants.emplace_back(model_debug(r, global, retained, expected, value));
    }
    output.emplace(String("variants", r), Json(r, variants));
    return Json(r, output);
}

int main() {
    auto previous = std::pmr::set_default_resource(std::pmr::null_memory_resource());
    try {
        for (std::string line; std::getline(std::cin, line);) {
            GlobalContext global;
            auto memory = global.memory_resource();
            Vector<ContextFunction<String(Memory)>> retained(memory);
            Vector<String> expected(memory);
            Json output(memory);
            auto user_context_owner = global.make_user_context();
            {
                auto& user_context = *user_context_owner;
                auto input = Json::parse(user_context.memory_resource(), line);
                if (!input) {
                    throw std::runtime_error("Invalid document model input");
                }
                output =
                    Json(memory,
                         document_model_observe(user_context, global, retained, expected, *input));
            }
            user_context_owner.reset();
            if (global.live_user_bytes() != 0) {
                throw std::runtime_error("Document model User memory survived destruction");
            }
            for (std::size_t i = 0; i < retained.size(); ++i) {
                if (retained[i](memory) != expected[i]) {
                    throw std::runtime_error(
                        "Explicit Global document copy changed after User destruction");
                }
            }
            std::cout << output.dump(memory) << '\n';
        }
    } catch (const std::exception& error) {
        std::pmr::set_default_resource(previous);
        std::cerr << error.what() << '\n';
        return 1;
    }
    std::pmr::set_default_resource(previous);
}
