#pragma once
#include "../../../examples/models.hpp"

namespace original_provider {
using namespace teaql;

// Private source models remain handwritten. Reuse the example's allocator-aware
// entity/ledger hooks and field mapper; transaction and save behavior is runtime code.
struct TransactionSchool : example::Model<TransactionSchool> {
    static constexpr std::string_view ENTITY_NAME = "TransactionSchool";
    std::int64_t revision = 0;

    TransactionSchool(std::allocator_arg_t, Allocator a) : Model(std::allocator_arg, a) {}

    TransactionSchool(std::allocator_arg_t, Allocator a, const TransactionSchool& other)
        : Model(std::allocator_arg, a, other), revision(other.revision) {}

    TransactionSchool(const TransactionSchool&) = delete;
    TransactionSchool(TransactionSchool&&) noexcept = default;

    static TransactionSchool fresh(Memory owner, std::uint64_t id, std::string_view name) {
        TransactionSchool result(std::allocator_arg, Allocator(owner));
        result.id = id;
        result.name = name;
        auto key = result.entity_key();
        result.root.mark_as_new(key);
        result.root.set(key, "id", Value(owner, id));
        result.root.set(key, "name", Value(owner, name));
        return result;
    }

    static EntityDescriptor entity_descriptor(Memory owner) {
        EntityDescriptor result(std::allocator_arg, Allocator(owner), ENTITY_NAME);
        result.table("transaction_school");
        result.property("id", DataType::U64).id().not_null();
        result.property("version", DataType::I64).version().not_null();
        result.property("name", DataType::Text).not_null();
        return result;
    }

    static Result<TransactionSchool, EntityError> from_compact_row(Memory owner, CompactRow row) {
        using Output = Result<TransactionSchool, EntityError>;
        TransactionSchool result(std::allocator_arg, Allocator(owner));
        auto id = example::decode_field<std::uint64_t>(owner, row, ENTITY_NAME, "id");
        auto version = example::decode_field<std::int64_t>(owner, row, ENTITY_NAME, "version");
        auto name = example::decode_field<String>(owner, row, ENTITY_NAME, "name");
        if (!id) {
            return Output::err(owner, std::move(id).error());
        }
        if (!version) {
            return Output::err(owner, std::move(version).error());
        }
        if (!name) {
            return Output::err(owner, std::move(name).error());
        }
        result.id = id.value();
        result.revision = version.value();
        result.name = std::move(name).value();
        result.root.set_original_compact_row(ENTITY_NAME, row);
        return Output::ok(owner, std::move(result));
    }

    std::int64_t version() const noexcept {
        return revision;
    }

    MutationValues into_values() && {
        auto result = scalar_values();
        result.fields.emplace("version", Value(memory_resource(), revision));
        return result;
    }
};

static_assert(LedgerEntity<TransactionSchool> && VersionedEntity<TransactionSchool>);

struct PaymentChannelCollision : EntityDefaults<PaymentChannelCollision> {
    using allocator_type = Allocator;
    static constexpr std::string_view ENTITY_NAME = "PaymentChannelCollision";
    CompactRow row;

    PaymentChannelCollision(std::allocator_arg_t, Allocator a, CompactRow value)
        : row(std::allocator_arg, a, std::move(value)) {}

    PaymentChannelCollision(std::allocator_arg_t, Allocator a, const PaymentChannelCollision& other)
        : row(std::allocator_arg, a, other.row) {}

    PaymentChannelCollision(const PaymentChannelCollision&) = delete;
    PaymentChannelCollision(PaymentChannelCollision&&) noexcept = default;

    Memory memory_resource() const {
        return row.memory_resource();
    }

    static EntityDescriptor entity_descriptor(Memory owner) {
        EntityDescriptor result(std::allocator_arg, Allocator(owner), ENTITY_NAME);
        result.table("payment_channel_collision");
        result.property("id", DataType::U64).id().not_null();
        result.property("version", DataType::I64).version().not_null();
        result.property("label", DataType::Text).not_null();
        return result;
    }

    static Result<PaymentChannelCollision, EntityError> from_compact_row(Memory owner,
                                                                         CompactRow row) {
        return Result<PaymentChannelCollision, EntityError>::ok(
            owner, PaymentChannelCollision(std::allocator_arg, Allocator(owner), std::move(row)));
    }

    Value id_value() const {
        return Value(memory_resource(), *row.get("id"));
    }

    MutationValues into_values() && {
        MutationValues result(memory_resource());
        result.fields = row.into_map(memory_resource());
        return result;
    }
};

struct PaymentOrderCollision : EntityDefaults<PaymentOrderCollision> {
    using allocator_type = Allocator;
    static constexpr std::string_view ENTITY_NAME = "PaymentOrderCollision";
    std::uint64_t id = 0, channel_id = 0;
    std::int64_t revision = 0;
    String name;
    std::optional<PaymentChannelCollision> channel;

    PaymentOrderCollision(std::allocator_arg_t, Allocator a) : name(a.resource()) {}

    PaymentOrderCollision(std::allocator_arg_t, Allocator a, const PaymentOrderCollision& other)
        : id(other.id), channel_id(other.channel_id), revision(other.revision),
          name(other.name, a.resource()), channel(detail::clone_in(a.resource(), other.channel)) {}

    PaymentOrderCollision(const PaymentOrderCollision&) = delete;
    PaymentOrderCollision(PaymentOrderCollision&&) noexcept = default;

    Memory memory_resource() const {
        return name.get_allocator().resource();
    }

    static EntityDescriptor entity_descriptor(Memory owner) {
        EntityDescriptor result(std::allocator_arg, Allocator(owner), ENTITY_NAME);
        result.table("payment_order_collision");
        result.property("id", DataType::U64).id().not_null();
        result.property("version", DataType::I64).version().not_null();
        result.property("channel_id", DataType::U64).not_null().column("channel");
        result.property("name", DataType::Text).not_null();
        result.relation("channel", "PaymentChannelCollision").local("channel_id").foreign("id");
        return result;
    }

    static Result<PaymentOrderCollision, EntityError> from_compact_row(Memory owner,
                                                                       CompactRow row) {
        using Output = Result<PaymentOrderCollision, EntityError>;
        PaymentOrderCollision result(std::allocator_arg, Allocator(owner));
        auto id = example::decode_field<std::uint64_t>(owner, row, ENTITY_NAME, "id");
        auto version = example::decode_field<std::int64_t>(owner, row, ENTITY_NAME, "version");
        auto fk = example::decode_field<std::uint64_t>(owner, row, ENTITY_NAME, "channel_id");
        auto name = example::decode_field<String>(owner, row, ENTITY_NAME, "name");
        if (!id) {
            return Output::err(owner, std::move(id).error());
        }
        if (!version) {
            return Output::err(owner, std::move(version).error());
        }
        if (!fk) {
            return Output::err(owner, std::move(fk).error());
        }
        if (!name) {
            return Output::err(owner, std::move(name).error());
        }
        result.id = id.value();
        result.revision = version.value();
        result.channel_id = fk.value();
        result.name = std::move(name).value();
        if (auto* value = row.get("channel")) {
            if (auto* object = std::get_if<Record>(&value->data)) {
                auto decoded = PaymentChannelCollision::from_compact_row(
                    owner, CompactRow(std::allocator_arg, Allocator(owner), *object));
                if (!decoded) {
                    return Output::err(owner, std::move(decoded).error());
                }
                result.channel.emplace(std::move(decoded).value());
            }
        }
        return Output::ok(owner, std::move(result));
    }

    std::int64_t version() const noexcept {
        return revision;
    }

    Value id_value() const {
        return Value(memory_resource(), id);
    }

    MutationValues into_values() && {
        auto owner = memory_resource();
        MutationValues result(owner);
        result.fields.emplace("id", Value(owner, id));
        result.fields.emplace("version", Value(owner, revision));
        result.fields.emplace("channel_id", Value(owner, channel_id));
        result.fields.emplace("name", Value(owner, name));
        result.fields.emplace("channel",
                              channel ? Value(owner, std::move(*channel).into_values().fields)
                                      : Value(owner));
        return result;
    }
};
}
