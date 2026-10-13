#pragma once
#include <teaql/runtime/context.hpp>
#include <teaql/core/detail/text.hpp>
#include <charconv>
#include <array>

namespace teaql {
// Shared mechanics for generated model logic; field-specific wrappers are thin.
class GeneratedCheckerLogic {
public:
    using allocator_type = Allocator;

    GeneratedCheckerLogic(std::allocator_arg_t, Allocator a) : resource_(a.resource()) {}

    virtual ~GeneratedCheckerLogic() = default;

    Memory memory_resource() const noexcept {
        return resource_;
    }

    static void required(bool present,
                         std::string_view field,
                         const ObjectLocation& location,
                         CheckResults& results) {
        if (!present) {
            auto r = results.get_allocator().resource();
            auto path = detail::clone_in(r, location);
            path.member(field);
            results.emplace_back(CheckResult::required(r, std::move(path)));
        }
    }

    template <class T>
    static void requiredOption(const std::optional<T>& value,
                               std::string_view field,
                               const ObjectLocation& location,
                               CheckResults& results) {
        required(bool(value), field, location, results);
    }

    template <class T>
    static void requiredOption(const T* value,
                               std::string_view field,
                               const ObjectLocation& location,
                               CheckResults& results) {
        required(value != nullptr, field, location, results);
    }

    static void requiredText(std::string_view value,
                             std::string_view field,
                             const ObjectLocation& location,
                             CheckResults& results) {
        detail::trim(value);
        required(!value.empty(), field, location, results);
    }

    static void minStringLength(std::string_view value,
                                std::string_view field,
                                std::size_t bound,
                                const ObjectLocation& location,
                                CheckResults& results) {
        string_length(value, field, bound, location, results, true);
    }

    static void maxStringLength(std::string_view value,
                                std::string_view field,
                                std::size_t bound,
                                const ObjectLocation& location,
                                CheckResults& results) {
        string_length(value, field, bound, location, results, false);
    }

private:
    Memory resource_;

    static void string_length(std::string_view value,
                              std::string_view field,
                              std::size_t bound,
                              const ObjectLocation& location,
                              CheckResults& results,
                              bool minimum) {
        std::size_t count = 0;
        for (unsigned char byte : value) {
            if ((byte & 0xc0) != 0x80) {
                ++count;
            }
        }
        if (minimum ? count < bound : count > bound) {
            auto r = results.get_allocator().resource();
            auto path = detail::clone_in(r, location);
            path.member(field);
            results.emplace_back(minimum ? CheckResult::min_str(r, std::move(path), bound, value)
                                         : CheckResult::max_str(r, std::move(path), bound, value));
        }
    }
};

class GeneratedFieldChecks {
public:
    template <class Base, class Logic>
    static ResourceBorrow<const Base> bind_logic(Allocator a, const Logic& logic) {
        auto& owner = detail::resource_arena(logic.memory_resource());
        auto slot = owner.slot_for_address(&logic);
        if (!slot) {
            throw std::invalid_argument("Checker logic must be a managed Context object");
        }
        return ResourceBorrow<const Base>(std::allocator_arg, a, logic, owner, slot);
    }

    static void require_loaded(bool loaded,
                               CheckObjectStatus status,
                               std::string_view field,
                               const ObjectLocation& location,
                               CheckResults& results) {
        if (status == CheckObjectStatus::Update && !loaded) {
            error(CheckRule::InvalidType,
                  field,
                  location,
                  results,
                  "Mutation requires a fully loaded entity");
        }
    }

    static void immutable_business_id(bool dirty,
                                      CheckObjectStatus status,
                                      std::string_view field,
                                      const ObjectLocation& location,
                                      CheckResults& results) {
        if (status == CheckObjectStatus::Update && dirty) {
            error(CheckRule::InvalidType,
                  field,
                  location,
                  results,
                  "Business ID is immutable after create");
        }
    }

    template <class Set>
    static void business_id(const UserContext& user_context,
                            CheckObjectStatus status,
                            const BusinessIdDefinition& definition,
                            std::string_view root_type,
                            std::string_view entity,
                            std::string_view current_storage,
                            bool was_missing,
                            Set&& set,
                            const ObjectLocation& location,
                            CheckResults& results) {
        if (status != CheckObjectStatus::Create) {
            return;
        }
        auto r = user_context.memory_resource();
        auto root = user_context.require_active_root(root_type);
        if (!root) {
            error(CheckRule::ContextRootMissing,
                  definition.field_name,
                  location,
                  results,
                  root.error().to_string(r));
            return;
        }
        auto service = user_context.business_ids();
        if (!service) {
            error(CheckRule::InvalidType,
                  definition.field_name,
                  location,
                  results,
                  service.error().to_string(r));
            return;
        }

        struct Slot {
            std::string_view current;
            Set& assign;

            std::optional<std::string_view> current_business_id() const {
                return current.empty() ? std::nullopt : std::optional(current);
            }

            bool is_new_aggregate() const {
                return true;
            }

            void assign_business_id(BusinessIdValue value) {
                std::invoke(assign, value.value);
            }
        } slot{current_storage, set};

        std::array<char, 20> buffer;
        auto converted =
            std::to_chars(buffer.data(), buffer.data() + buffer.size(), root.value().get().id);
        auto assigned = service.value().get().ensure(r,
                                                     user_context,
                                                     definition,
                                                     std::string_view(buffer.data(), converted.ptr),
                                                     entity,
                                                     slot);
        if (!assigned) {
            error(CheckRule::InvalidType,
                  definition.field_name,
                  location,
                  results,
                  assigned.error().to_string(r));
        } else if (was_missing) {
            user_context.record_fix_evidence(FixEvidence(std::allocator_arg,
                                                         Allocator(r),
                                                         entity,
                                                         definition.field_name,
                                                         FixEvidenceSource::Context,
                                                         "businessIdAllocator"));
        }
    }

    template <class Set>
    static void context_root(const UserContext& user_context,
                             CheckObjectStatus status,
                             std::string_view entity,
                             std::string_view field,
                             std::string_view root_type,
                             std::uint64_t current,
                             Set&& set,
                             const ObjectLocation& location,
                             CheckResults& results) {
        if (status != CheckObjectStatus::Create &&
            (status != CheckObjectStatus::Update || current == 0)) {
            return;
        }
        auto root = user_context.require_active_root(root_type);
        if (!root) {
            error(CheckRule::ContextRootMissing,
                  field,
                  location,
                  results,
                  root.error().to_string(user_context.memory_resource()));
        } else if (status == CheckObjectStatus::Create && current == 0) {
            std::invoke(std::forward<Set>(set), root.value().get().id);
            String label("activeRoot:", user_context.memory_resource());
            label += root_type;
            user_context.record_fix_evidence(FixEvidence(std::allocator_arg,
                                                         Allocator(user_context.memory_resource()),
                                                         entity,
                                                         field,
                                                         FixEvidenceSource::Context,
                                                         label));
        } else if (current != root.value().get().id) {
            error(CheckRule::ContextRootMismatch, field, location, results);
        }
    }

    template <class Set>
    static void clock(const UserContext& user_context,
                      CheckObjectStatus status,
                      bool update,
                      std::string_view entity,
                      std::string_view field,
                      Set&& set) {
        if (status == CheckObjectStatus::Create ||
            (update && status == CheckObjectStatus::Update)) {
            std::invoke(std::forward<Set>(set), user_context.fix_time());
            user_context.record_fix_evidence(FixEvidence(std::allocator_arg,
                                                         Allocator(user_context.memory_resource()),
                                                         entity,
                                                         field,
                                                         FixEvidenceSource::Clock,
                                                         "graphClock"));
        }
    }

private:
    static void error(CheckRule rule,
                      std::string_view field,
                      const ObjectLocation& location,
                      CheckResults& results,
                      std::string_view message = {}) {
        auto path = detail::clone_in(results.get_allocator().resource(), location);
        path.member(field);
        results.emplace_back(rule, std::move(path));
        if (!message.empty()) {
            results.back().with_message(message);
        }
    }
};
}
