#include <teaql/runtime/business_id.hpp>
#include <teaql/runtime/context.hpp>
#include <limits>

namespace teaql {
Result<BusinessIdAllocation, BusinessIdError>
InMemoryBusinessIdAllocator::allocate(Memory r, const BusinessIdPlan& plan) const {
    if (std::this_thread::get_id() != owner_)
        throw std::logic_error(
            "In-memory business ID allocator must run on its Context owner thread");
    using R = Result<BusinessIdAllocation, BusinessIdError>;
    auto valid = plan.validate_allocation_range();
    if (!valid)
        return R::err(r, detail::clone_in(r, valid.error()));
    auto key = plan.scope.canonical_key(r);
    std::lock_guard guard(mutex_);
    if (poisoned_)
        return R::err(r,
                      BusinessIdError(std::allocator_arg,
                                      Allocator(r),
                                      BusinessIdErrorCode::Allocation,
                                      "in-memory business ID allocator lock is poisoned"));
    try {
        auto found = levels_.find(std::string_view(key));
        auto current = found == levels_.end() ? plan.initial_sequence : found->second;
        if (current > plan.max_sequence) {
            String message("business ID sequence exhausted for scope ", r);
            message += key;
            return R::err(
                r,
                BusinessIdError(
                    std::allocator_arg, Allocator(r), BusinessIdErrorCode::Exhausted, message));
        }
        if (current == std::numeric_limits<std::uint64_t>::max())
            return R::err(r,
                          BusinessIdError(std::allocator_arg,
                                          Allocator(r),
                                          BusinessIdErrorCode::Exhausted,
                                          "business ID sequence overflow"));
        if (found == levels_.end())
            levels_.emplace(String(key, resource_), current + 1);
        else
            found->second = current + 1;
        return R::ok(r,
                     BusinessIdAllocation(std::allocator_arg, Allocator(r), plan.scope, current));
    } catch (...) {
        poisoned_ = true;
        throw;
    }
}

Result<BusinessIdValue, BusinessIdError>
BusinessIdService::ensure_value(Memory r,
                                const UserContext& context,
                                const BusinessIdDefinition& definition,
                                std::string_view root,
                                std::string_view aggregate,
                                FunctionRef<std::optional<std::string_view>()> current,
                                FunctionRef<bool()> fresh,
                                FunctionRef<void(BusinessIdValue)> assign) const {
    using R = Result<BusinessIdValue, BusinessIdError>;
    DailySequenceBusinessIdProfile legacy;
    std::optional<PermutedDailyBusinessIdProfile> permuted;
    const BusinessIdProfile* profile = nullptr;
    if (definition.profile == DEFAULT_BUSINESS_ID_PROFILE) {
        if (!key_provider_)
            return R::err(
                r,
                BusinessIdError(std::allocator_arg,
                                Allocator(r),
                                BusinessIdErrorCode::KeyNotFound,
                                "BusinessIdKeyProvider is not registered for daily-permuted-v1"));
        auto key = key_provider_->get().current_key(r, context, definition);
        if (!key)
            return R::err(r, detail::clone_in(r, key.error()));
        permuted.emplace(key.value());
        profile = &*permuted;
    } else if (definition.profile == LEGACY_DAILY_SEQUENCE_PROFILE)
        profile = &legacy;
    else {
        String message("unsupported business ID profile ", r);
        message += definition.profile;
        return R::err(
            r,
            BusinessIdError(
                std::allocator_arg, Allocator(r), BusinessIdErrorCode::InvalidDefinition, message));
    }
    if (auto value = current(); value && !value->empty())
        return profile->validate(r, definition, *value);
    if (!fresh()) {
        String message(definition.field_name, r);
        message += " is immutable after the aggregate is persisted";
        return R::err(
            r,
            BusinessIdError(
                std::allocator_arg, Allocator(r), BusinessIdErrorCode::Immutable, message));
    }
    BusinessIdGenerationRequest request(
        std::allocator_arg, Allocator(r), definition, root, aggregate, context.business_date());
    auto plan = profile->plan(r, request);
    if (!plan)
        return R::err(r, detail::clone_in(r, plan.error()));
    auto allocation = allocator_.get().allocate(r, plan.value());
    if (!allocation)
        return R::err(r, detail::clone_in(r, allocation.error()));
    auto value = profile->format(r, plan.value(), allocation.value());
    if (!value)
        return value;
    assign(detail::clone_in(r, value.value()));
    return value;
}
}
