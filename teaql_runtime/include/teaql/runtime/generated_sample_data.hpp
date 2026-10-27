#pragma once
#include <teaql/memory/arena.hpp>
#include <teaql/memory/object_borrow.hpp>
#include <teaql/core/value.hpp>
#include <teaql/runtime/data_service.hpp>
#include <teaql/runtime/sample_data_logging.hpp>
#include <bit>
#include <charconv>
#include <cstdio>
#include <optional>
#include <limits>
#include <map>
#include <span>
#include <unordered_set>

namespace teaql {
inline std::uint64_t sample_reference_id(std::uint64_t id) {
    return id;
}

inline std::uint64_t sample_reference_id(const Value* value) {
    return value ? value->try_u64().value_or(0) : 0;
}

enum class SampleDataScale { Tiny, Small, Medium };

struct SampleDataPlan {
    SampleDataScale scale = SampleDataScale::Small;
    std::uint64_t seed = 0;

    static SampleDataPlan small() {
        return {};
    }
};

struct SampleDataSkipped {
    using allocator_type = Allocator;
    String entity, reason;

    SampleDataSkipped(std::allocator_arg_t,
                      Allocator a,
                      std::string_view name,
                      std::string_view why)
        : entity(name, a.resource()), reason(why, a.resource()) {}

    SampleDataSkipped(std::allocator_arg_t, Allocator a, const SampleDataSkipped& other)
        : SampleDataSkipped(std::allocator_arg, a, other.entity, other.reason) {}

    SampleDataSkipped(const SampleDataSkipped&) = delete;
    SampleDataSkipped(SampleDataSkipped&&) noexcept = default;

    Memory memory_resource() const {
        return entity.get_allocator().resource();
    }
};

struct SampleDataReport {
    using allocator_type = Allocator;
    using Counts = std::pmr::map<String, std::size_t, std::less<>>;
    Counts generated;
    Vector<SampleDataSkipped> skipped;

    SampleDataReport(std::allocator_arg_t, Allocator a)
        : generated(a.resource()), skipped(a.resource()) {}

    SampleDataReport(std::allocator_arg_t, Allocator a, const SampleDataReport& other)
        : generated(other.generated, a.resource()), skipped(other.skipped, a.resource()) {}

    SampleDataReport(std::allocator_arg_t, Allocator a, SampleDataReport&& other)
        : SampleDataReport(detail::move_to(a.resource(), std::move(other))) {}

    SampleDataReport(const SampleDataReport&) = delete;
    SampleDataReport(SampleDataReport&&) noexcept = default;

    SampleDataReport& operator=(SampleDataReport&& other) {
        return detail::assign_scoped(*this, std::move(other));
    }

    Memory memory_resource() const {
        return generated.get_allocator().resource();
    }
};

class SampleDataError {
public:
    using allocator_type = Allocator;

    SampleDataError(std::allocator_arg_t, Allocator a, std::string_view text)
        : message_(text, a.resource()) {}

    SampleDataError(std::allocator_arg_t, Allocator a, const SampleDataError& other)
        : SampleDataError(std::allocator_arg, a, other.message_) {}

    SampleDataError(const SampleDataError&) = delete;
    SampleDataError(SampleDataError&&) noexcept = default;

    Memory memory_resource() const {
        return message_.get_allocator().resource();
    }

    String to_string(Memory destination) const {
        return String(message_, destination);
    }

private:
    String message_;
};

struct SampleDataState {
    using allocator_type = Allocator;
    using References = std::pmr::map<String, Vector<std::uint64_t>, std::less<>>;
    SampleDataPlan plan;
    References references;
    SampleDataReport::Counts generated;
    Vector<SampleDataSkipped> skipped;

    SampleDataState(std::allocator_arg_t,
                    Allocator a,
                    SampleDataPlan selected = SampleDataPlan::small())
        : plan(selected), references(a.resource()), generated(a.resource()), skipped(a.resource()) {
    }

    SampleDataState(std::allocator_arg_t, Allocator a, const SampleDataState& other)
        : plan(other.plan), references(other.references, a.resource()),
          generated(other.generated, a.resource()), skipped(other.skipped, a.resource()) {}

    SampleDataState(std::allocator_arg_t, Allocator a, SampleDataState&& other)
        : SampleDataState(detail::move_to(a.resource(), std::move(other))) {}

    SampleDataState(const SampleDataState&) = delete;
    SampleDataState(SampleDataState&&) noexcept = default;

    SampleDataState& operator=(SampleDataState&& other) {
        return detail::assign_scoped(*this, std::move(other));
    }

    Memory memory_resource() const {
        return references.get_allocator().resource();
    }

    void addReference(std::string_view entity, std::uint64_t id) {
        auto [position, inserted] = references.try_emplace(String(entity, memory_resource()));
        position->second.push_back(id);
    }

    std::span<const std::uint64_t> ids(std::string_view entity) const {
        auto found = references.find(entity);
        return found == references.end() ? std::span<const std::uint64_t>{}
                                         : std::span<const std::uint64_t>(found->second);
    }

    std::optional<std::uint64_t> pickId(std::string_view entity, std::size_t salt) const {
        auto values = ids(entity);
        return values.empty() ? std::nullopt
                              : std::optional<std::uint64_t>(values[salt % values.size()]);
    }

    std::optional<std::uint64_t>
    pickUnusedId(std::string_view entity,
                 std::size_t salt,
                 const std::pmr::unordered_set<std::uint64_t>& used) const {
        auto values = ids(entity);
        if (values.empty()) {
            return {};
        }
        auto best = values[salt % values.size()];
        if (!used.contains(best)) {
            return best;
        }
        for (auto id : values) {
            if (!used.contains(id)) {
                return id;
            }
        }
        // Source deliberately reuses the preferred ID after exhausting the pool.
        return best;
    }

    void recordGenerated(std::string_view entity) {
        auto [position, inserted] = generated.try_emplace(String(entity, memory_resource()), 0);
        if (position->second == std::numeric_limits<std::size_t>::max()) {
            throw std::overflow_error("sample-data generated count overflow");
        }
        ++position->second;
    }

    void recordSkipped(std::string_view entity, std::string_view reason) {
        skipped.emplace_back(entity, reason);
    }

    SampleDataReport intoReport() && {
        SampleDataReport report(std::allocator_arg, Allocator(memory_resource()));
        report.generated.swap(generated);
        report.skipped.swap(skipped);
        references.clear();
        return report;
    }
};
}

namespace teaql {
using SampleDataResult = Result<SampleDataReport, SampleDataError>;
using SampleDataFuture = AsyncOperation<SampleDataResult>;
using SampleDataStepResult = Result<std::monostate, SampleDataError>;
using SampleDataStepFuture = AsyncOperation<SampleDataStepResult>;
using GeneratedSampleStep = SampleDataStepFuture& (*)(UserContext&, SampleDataState&);
}

namespace teaql::detail {
inline std::uint64_t sample_seed_index(std::uint64_t seed, std::size_t index) {
    if (index > std::numeric_limits<std::uint64_t>::max() - seed) {
        throw std::overflow_error("sample-data seed/index overflow");
    }
    return seed + index;
}

inline std::int64_t
sample_number(const SampleDataPlan& plan, std::size_t index, std::string_view maximum) {
    std::uint64_t bound = 1000;
    auto text = maximum;
    if (text.starts_with('+')) {
        text.remove_prefix(1);
    }
    std::uint64_t parsed = 0;
    auto [end, error] = std::from_chars(text.data(), text.data() + text.size(), parsed);
    if (error == std::errc{} && end == text.data() + text.size()) {
        bound = parsed;
    }
    auto value = sample_seed_index(plan.seed, index) % std::max(std::uint64_t(1), bound) + 1;
    return std::bit_cast<std::int64_t>(value);
}

inline String sample_text(Memory r, std::string_view prefix, std::size_t index) {
    String text(prefix, r);
    text += ' ';
    char digits[32];
    auto converted = std::to_chars(std::begin(digits), std::end(digits), index + 1);
    text.append(digits, converted.ptr);
    return text;
}

inline Timestamp sample_timestamp(const SampleDataPlan& plan, std::size_t index) {
    auto days = sample_seed_index(plan.seed, index) % (365 * 3);
    return Timestamp(Timestamp::now().millis - std::int64_t(days) * 86400000);
}

inline Date sample_date(const SampleDataPlan& plan, std::size_t index) {
    return sample_timestamp(plan, index).to_datetime().date;
}

inline String sample_time(Memory r, const SampleDataPlan& plan, std::size_t index) {
    auto instant = sample_timestamp(plan, index).to_datetime();
    String text(instant.date.to_string(r), r);
    char clock[10];
    std::snprintf(
        clock, sizeof(clock), " %02u:%02u:%02u", instant.hour, instant.minute, instant.second);
    text += clock;
    return text;
}

inline std::size_t sample_fanout(SampleDataScale scale, std::size_t object_fields) {
    auto factor = scale == SampleDataScale::Tiny    ? 20
                  : scale == SampleDataScale::Small ? 100
                                                    : 1000;
    auto fields = std::max(std::size_t(1), object_fields);
    if (fields > std::numeric_limits<std::size_t>::max() / factor) {
        throw std::overflow_error("sample-data fanout overflow");
    }
    return fields * factor;
}

template <class Recipe>
class GeneratedSampleModelOperation {
public:
    using allocator_type = Allocator;
    using Model = typename Recipe::Model;
    using QueryOperation =
        std::remove_reference_t<decltype(Recipe::query(std::declval<UserContext&>()))>;
    using SaveOperation = AsyncOperation<Result<Model, RuntimeError>>;

    GeneratedSampleModelOperation(std::allocator_arg_t,
                                  Allocator a,
                                  UserContext& user_context,
                                  SampleDataState& state)
        : resource_(a.resource()), user_context_(user_context),
          state_(std::allocator_arg, a, state) {
        if (state.memory_resource() != resource_ || user_context.memory_resource() != resource_) {
            throw std::invalid_argument("sample generation must use its UserContext");
        }
    }

    std::optional<SampleDataStepResult> poll(WakeSignal& wake) {
        auto& state = const_cast<SampleDataState&>(state_.get());
        if constexpr (Recipe::load_references) {
            if (!query_) {
                query_ = &Recipe::query(user_context_);
                require_local(*query_);
            }
            forwarding_.attach(query_->wake_signal(), wake);
            if (!query_->poll()) {
                return {};
            }
            forwarding_.reset();
            auto rows = query_->take_result();
            query_ = nullptr;
            // The source intentionally defaults a failed root/constant query to an empty list.
            if (rows) {
                for (const auto& entity : rows.value().data) {
                    state.addReference(Recipe::name, sample_reference_id(entity.id()));
                }
            }
            return SampleDataStepResult::ok(resource_, {});
        } else {
            if (!initialized_) {
                for (auto dependency : Recipe::required_dependencies) {
                    if (state.ids(dependency).empty()) {
                        String reason("Required dependency ", resource_);
                        reason += dependency;
                        reason += " is missing in reference pool";
                        state.recordSkipped(Recipe::name, reason);
                        sample_info(user_context_, [&](Memory r) {
                            String message("Skipped generating ", r);
                            message += Recipe::name;
                            message += ": ";
                            message += reason;
                            message += '.';
                            return message;
                        });
                        return SampleDataStepResult::ok(resource_, {});
                    }
                }
                count_ = sample_fanout(state.plan.scale, Recipe::object_fields);
                initialized_ = true;
                sample_info(user_context_, [&](Memory r) {
                    String message("Generating sample data for ", r);
                    message += Recipe::name;
                    message += " (expected: ";
                    sample_log_number(message, count_);
                    message += ")...";
                    return message;
                });
            }
            while (index_ < count_) {
                if (!save_) {
                    auto& entity = Recipe::create(user_context_);
                    Recipe::fill(user_context_, entity, state, index_);
                    save_ = &AuditedSaveExt<Model>::save(
                        std::move(entity).audit_as("Init Sample Data"), user_context_);
                    require_local(*save_);
                }
                forwarding_.attach(save_->wake_signal(), wake);
                if (!save_->poll()) {
                    return {};
                }
                forwarding_.reset();
                auto saved = save_->take_result();
                save_ = nullptr;
                if (!saved) {
                    auto message = saved.error().to_string(resource_);
                    return SampleDataStepResult::err(
                        resource_,
                        SampleDataError(std::allocator_arg, Allocator(resource_), message));
                }
                state.recordGenerated(Recipe::name);
                if (index_ % 20 == 0) {
                    sample_info(user_context_, [&](Memory r) {
                        String message("Generating ", r);
                        message += Recipe::name;
                        message += ": ";
                        sample_log_number(message, index_);
                        message += '/';
                        sample_log_number(message, count_);
                        return message;
                    });
                }
                if constexpr (Recipe::has_reverse) {
                    state.addReference(Recipe::name, sample_reference_id(saved.value().id()));
                }
                ++index_;
            }
            sample_info(user_context_, [&](Memory r) {
                String message("Successfully generated sample records for ", r);
                message += Recipe::name;
                message += '.';
                return message;
            });
            return SampleDataStepResult::ok(resource_, {});
        }
    }

    void finish_and_wait() noexcept {
        cancel_and_wait();
    }

    void cancel_and_wait() noexcept {
        forwarding_.reset();
        if (query_) {
            query_->cancel();
            query_ = nullptr;
        }
        if (save_) {
            save_->cancel();
            save_ = nullptr;
        }
    }

private:
    template <class Operation>
    void require_local(Operation& operation) {
        if (operation.memory_resource() != resource_) {
            operation.cancel();
            throw std::invalid_argument("sample child operation has a foreign Context");
        }
    }

    Memory resource_;
    UserContext& user_context_;
    ArenaBorrow<SampleDataState> state_;
    QueryOperation* query_ = nullptr;
    SaveOperation* save_ = nullptr;
    WakeSignal::Forward forwarding_;
    bool initialized_ = false;
    std::size_t index_ = 0, count_ = 0;
};

class GeneratedSampleSequenceOperation {
public:
    using allocator_type = Allocator;

    GeneratedSampleSequenceOperation(std::allocator_arg_t,
                                     Allocator a,
                                     UserContext& user_context,
                                     SampleDataPlan plan,
                                     std::span<const GeneratedSampleStep> steps)
        : resource_(a.resource()), user_context_(user_context),
          state_(user_context.make<SampleDataState>(plan)),
          steps_(steps.begin(), steps.end(), a.resource()) {}

    std::optional<SampleDataResult> poll(WakeSignal& wake) {
        if (!started_) {
            started_ = true;
            sample_info(user_context_, [&](Memory r) {
                String message("Starting sample data generation. Scale: ", r);
                message += state_.plan.scale == SampleDataScale::Tiny    ? "Tiny"
                           : state_.plan.scale == SampleDataScale::Small ? "Small"
                                                                         : "Medium";
                message += ", Seed: ";
                sample_log_number(message, state_.plan.seed);
                return message;
            });
        }
        while (index_ < steps_.size()) {
            if (!child_) {
                child_ = &steps_[index_](user_context_, state_);
                if (child_->memory_resource() != resource_) {
                    child_->cancel();
                    child_ = nullptr;
                    throw std::invalid_argument("sample step has a foreign Context");
                }
            }
            forwarding_.attach(child_->wake_signal(), wake);
            if (!child_->poll()) {
                return {};
            }
            forwarding_.reset();
            auto result = child_->take_result();
            child_ = nullptr;
            if (!result) {
                return SampleDataResult::err(resource_, std::move(result).error());
            }
            ++index_;
        }
        sample_info(user_context_, [&](Memory r) {
            String message("Sample data generation completed successfully. Generated: ", r);
            sample_log_number(message, state_.generated.size());
            message += " tables, Skipped: ";
            sample_log_number(message, state_.skipped.size());
            message += " tables.";
            return message;
        });
        return SampleDataResult::ok(resource_, std::move(state_).intoReport());
    }

    void finish_and_wait() noexcept {
        cancel_and_wait();
    }

    void cancel_and_wait() noexcept {
        forwarding_.reset();
        if (child_) {
            child_->cancel();
            child_ = nullptr;
        }
    }

private:
    Memory resource_;
    UserContext& user_context_;
    SampleDataState& state_;
    Vector<GeneratedSampleStep> steps_;
    bool started_ = false;
    std::size_t index_ = 0;
    SampleDataStepFuture* child_ = nullptr;
    WakeSignal::Forward forwarding_;
};
}

namespace teaql {
struct GeneratedSampleValues {
    template <class Model, class Scalar>
    static void
    assign_operand(Model& entity, std::string_view field, Scalar& stored, Value original) {
        auto resource = entity.memory_resource();
        detail::EntityRuntimeAccess::update_scalar_value(
            entity.root, entity.entity_key(), field, stored, original);
        auto& pool = detail::ContextArena::from_memory(resource).loaded_snapshots();
        entity.load_state.mark_loaded(field, &pool).value();
    }

    template <class Model>
    static void unconverted_value(Model& entity, std::string_view field, Value original) {
        auto resource = entity.memory_resource();
        if (original.memory_resource() != resource) {
            throw std::invalid_argument("Sample operand requires an explicit destination copy");
        }
        entity.root.set(entity.entity_key(), field, original);
        auto& pool = detail::ContextArena::from_memory(resource).loaded_snapshots();
        entity.load_state.mark_loaded(field, &pool).value();
    }

    static std::int64_t
    number(const SampleDataPlan& plan, std::size_t index, std::string_view maximum) {
        return detail::sample_number(plan, index, maximum);
    }

    static String text(Memory resource, std::string_view prefix, std::size_t index) {
        return detail::sample_text(resource, prefix, index);
    }

    static Timestamp timestamp(const SampleDataPlan& plan, std::size_t index) {
        return detail::sample_timestamp(plan, index);
    }

    static Date date(const SampleDataPlan& plan, std::size_t index) {
        return detail::sample_date(plan, index);
    }

    static String time(Memory resource, const SampleDataPlan& plan, std::size_t index) {
        return detail::sample_time(resource, plan, index);
    }
};

template <class Recipe>
SampleDataStepFuture& run_generated_sample_model(UserContext& user_context,
                                                 SampleDataState& state) {
    return make_operation<SampleDataStepResult, detail::GeneratedSampleModelOperation<Recipe>>(
        user_context.memory_resource(), user_context, state);
}

inline SampleDataFuture& run_generated_sample_data(UserContext& user_context,
                                                   SampleDataPlan plan,
                                                   std::span<const GeneratedSampleStep> steps) {
    return make_operation<SampleDataResult, detail::GeneratedSampleSequenceOperation>(
        user_context.memory_resource(), user_context, plan, steps);
}
}
