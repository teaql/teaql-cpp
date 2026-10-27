#pragma once
#include <teaql/runtime/data_service.hpp>
#include <teaql/runtime/context_binding.hpp>
#include <teaql/core/request.hpp>
#include <teaql/runtime/detail/query_results.hpp>
#include <teaql/runtime/detail/generated_query_execution.hpp>
#include <functional>
#include <limits>

namespace teaql::detail {
// A managed query root. Nested payloads use its Context resource and do not
// publish separate object IDs. Wrappers contain only checked binding and root ID.
struct QueryDescription {
    using allocator_type = Allocator;
    SelectQuery query;
    String intent;
    request::QueryOptions analysis_origins;
    Vector<request::RelationSelection> relation_selections;
    Vector<request::RelationFilter> relation_filters;
    Vector<request::QuerySelection> child_selections;

    QueryDescription(std::allocator_arg_t, Allocator a, std::string_view entity)
        : query(std::allocator_arg, a, entity), intent(a.resource()),
          analysis_origins(std::allocator_arg, a), relation_selections(a.resource()),
          relation_filters(a.resource()), child_selections(a.resource()) {}

    QueryDescription(std::allocator_arg_t, Allocator a, const QueryDescription& other)
        : query(std::allocator_arg, a, other.query), intent(other.intent, a.resource()),
          analysis_origins(std::allocator_arg, a, other.analysis_origins),
          relation_selections(other.relation_selections, a.resource()),
          relation_filters(
              other.relation_filters.begin(), other.relation_filters.end(), a.resource()),
          child_selections(
              other.child_selections.begin(), other.child_selections.end(), a.resource()) {}

    Memory memory_resource() const {
        return query.memory_resource();
    }
};
}

namespace teaql {
// The executor type is a compile-time protocol, not a caller-supplied resource.
// The executing UserContext resolves its own registered executor and metadata.
template <Entity T, class E, bool Purposed = false, class R = T>
class EntityQuery {
    template <Entity, class, bool, class>
    friend class EntityQuery;

    // Generated request names remain model identities even when descriptor
    // metadata overrides the entity trait identity.
    static constexpr std::string_view query_entity_name() noexcept {
        if constexpr (requires {
                          { T::QUERY_ENTITY_NAME } -> std::convertible_to<std::string_view>;
                      }) {
            return T::QUERY_ENTITY_NAME;
        } else {
            return T::ENTITY_NAME;
        }
    }

public:
    explicit EntityQuery(UserContext& user_context, bool minimal = false, bool live_rows = true)
        requires(!Purposed)
        : binding_(user_context),
          description_(user_context.make_ref<detail::QueryDescription>(query_entity_name())) {
        auto descriptor = T::entity_descriptor(memory_resource());
        if constexpr (requires { T::QUERY_IDENTITY_FIELDS; }) {
            for (auto field : T::QUERY_IDENTITY_FIELDS) {
                description().query.project(field);
            }
        }
        for (const auto& property : descriptor.properties) {
            if constexpr (!requires { T::QUERY_IDENTITY_FIELDS; }) {
                if (property.is_id || property.is_version) {
                    description().query.project(property.name);
                }
            }
            if (live_rows && property.is_version) {
                description().query.and_filter(
                    Expr::gt(memory_resource(), property.name, std::int64_t(0)));
            }
        }
        if (!minimal) {
            if constexpr (requires { T::QUERY_PROJECTION_FIELDS; }) {
                for (auto field : T::QUERY_PROJECTION_FIELDS) {
                    description().query.project(field);
                }
            } else {
                for (const auto& property : descriptor.properties) {
                    description().query.project(property.name);
                }
            }
        }
    }

    EntityQuery(const EntityQuery& other)
        : binding_(other.binding_),
          description_(binding_.get().make_ref<detail::QueryDescription>(other.description())) {}

    EntityQuery(EntityQuery&& other) noexcept
        : binding_(other.binding_), description_(std::exchange(other.description_, {})) {}

    EntityQuery& operator=(const EntityQuery& other) {
        if (this != &other) {
            auto replacement = EntityQuery(other);
            *this = std::move(replacement);
        }
        return *this;
    }

    EntityQuery& operator=(EntityQuery&& other) noexcept {
        if (this != &other) {
            binding_ = other.binding_;
            description_ = std::exchange(other.description_, {});
        }
        return *this;
    }

    Memory memory_resource() const {
        return description().memory_resource();
    }

    const SelectQuery& query() const {
        return description().query;
    }

    // Source query(&self) maps to a non-consuming, explicit-domain snapshot.
    SelectQuery coreQuery() const
        requires(!Purposed)
    {
        return detail::capture_generated_core_query(description().query);
    }

    request::QuerySelection selection() const
        requires(!Purposed)
    {
        const auto& value = description();
        return detail::capture_generated_selection(value.query,
                                                   value.analysis_origins,
                                                   value.relation_filters,
                                                   value.child_selections,
                                                   value.relation_selections);
    }

    std::span<const request::RelationSelection> relationSelections() const
        requires(!Purposed)
    {
        return description().relation_selections;
    }

    std::span<const request::RelationFilter> relationFilters() const
        requires(!Purposed)
    {
        return description().relation_filters;
    }

    std::span<const request::QuerySelection> childEnhancements() const
        requires(!Purposed)
    {
        return description().child_selections;
    }

    // C++ snapshots preserve the source option boundary without a hidden cache.
    // The allocator belongs to the bound Context; destroy this value before close.
    request::QueryOptions queryOptions() const
        requires(!Purposed)
    {
        auto state = selection();
        return std::move(state.query_options);
    }

    // The source's consuming method returns only its core query. This differs
    // from QuerySelection::into_query(), which applies options and enhancements.
    SelectQuery intoQuery() &&
        requires(!Purposed)
    {
        auto result = coreQuery();
        description_ = {};
        return result;
    }

    // The result is an unconstrained marker until public execution/creation,
    // where the source facade requires Entity. No marker object is constructed.
    // Lvalues retain an independent Context-owned description.
    template <class Result>
    EntityQuery<T, E, Purposed, Result> returnType() const& {
        auto copy = binding_.get().make_ref<detail::QueryDescription>(description());
        return EntityQuery<T, E, Purposed, Result>(binding_, copy);
    }

    template <class Result>
    EntityQuery<T, E, Purposed, Result> returnType() && {
        (void)description();
        return EntityQuery<T, E, Purposed, Result>(binding_, std::exchange(description_, {}));
    }

    // Trusted model adapters translate typed names; ownership stays in runtime.
    template <class F>
    void mutate_selection(F&& mutation) {
        std::invoke(std::forward<F>(mutation), description().query);
    }

    template <Entity Child, class ChildExecutor, bool ChildPurposed, class ChildResult>
    void requireCompatible(
        const EntityQuery<Child, ChildExecutor, ChildPurposed, ChildResult>& child) const {
        binding_.require_same(child.binding_);
    }

    // Generated descendant adapters build requests inside this binding. Child
    // enhancement factories retain the source's unfiltered version semantics.
    template <Entity Child>
    EntityQuery<Child, E, false> childQuery() const {
        auto child = EntityQuery<Child, E, false>(binding_.get(), true, false);
        if (detail::reference_global(description_.id())) {
            return child.copyToGlobal(binding_.get());
        }
        return child;
    }

    template <Entity Child>
    EntityQuery<Child, E, false> dynamicChildQuery() const {
        auto child = EntityQuery<Child, E, false>(binding_.get(), true);
        if (detail::reference_global(description_.id())) {
            return child.copyToGlobal(binding_.get());
        }
        return child;
    }

    template <Entity Child>
    EntityQuery<Child, E, false> relatedQuery(bool minimal = false) const {
        auto child = EntityQuery<Child, E, false>(binding_.get(), minimal);
        if (detail::reference_global(description_.id())) {
            return child.copyToGlobal(binding_.get());
        }
        return child;
    }

    EntityQuery& matchTypes(std::span<const Value> types) & {
        description().query.and_filter(
            Expr::in_list(memory_resource(), request::TYPE_FIELD, types));
        return *this;
    }

    EntityQuery&& matchTypes(std::span<const Value> types) && {
        matchTypes(types);
        return std::move(*this);
    }

    template <std::ranges::input_range Inputs>
        requires std::constructible_from<Value, Memory, std::ranges::range_reference_t<Inputs>>
    EntityQuery& matchTypes(Inputs&& inputs) & {
        auto values = criteria_values(std::forward<Inputs>(inputs));
        return matchTypes(std::span<const Value>(values));
    }

    template <std::ranges::input_range Inputs>
        requires std::constructible_from<Value, Memory, std::ranges::range_reference_t<Inputs>>
    EntityQuery&& matchTypes(Inputs&& inputs) && {
        matchTypes(std::forward<Inputs>(inputs));
        return std::move(*this);
    }

    EntityQuery& withTypeGroup() & {
        return select(request::TYPE_GROUP_FIELD);
    }

    EntityQuery&& withTypeGroup() && {
        withTypeGroup();
        return std::move(*this);
    }

    EntityQuery& matchingAnyOf(const SelectQuery& child) & {
        if (memory_resource() != child.memory_resource()) {
            throw std::invalid_argument("Membership requires an explicit destination copy");
        }
        return appendMatchingQuery(child);
    }

    EntityQuery&& matchingAnyOf(const SelectQuery& child) && {
        matchingAnyOf(child);
        return std::move(*this);
    }

    EntityQuery& matchingAnyOf(const request::QuerySelection& child) & {
        return matchingAnyOf(child.query);
    }

    EntityQuery&& matchingAnyOf(const request::QuerySelection& child) && {
        matchingAnyOf(child);
        return std::move(*this);
    }

    EntityQuery& matchAnyOf(const SelectQuery& child) & {
        return matchingAnyOf(child);
    }

    EntityQuery&& matchAnyOf(const SelectQuery& child) && {
        matchingAnyOf(child);
        return std::move(*this);
    }

    EntityQuery& matchAnyOf(const request::QuerySelection& child) & {
        return matchingAnyOf(child);
    }

    EntityQuery&& matchAnyOf(const request::QuerySelection& child) && {
        matchingAnyOf(child);
        return std::move(*this);
    }

    template <Entity Child, class ChildExecutor, bool ChildPurposed, class ChildResult>
        requires(!ChildPurposed)
    EntityQuery&
    matchingAnyOf(const EntityQuery<Child, ChildExecutor, ChildPurposed, ChildResult>& child) & {
        requireCompatible(child);
        auto selection = child.selection();
        return appendMatchingQuery(selection.query);
    }

    template <Entity Child, class ChildExecutor, bool ChildPurposed, class ChildResult>
        requires(!ChildPurposed)
    EntityQuery&&
    matchingAnyOf(const EntityQuery<Child, ChildExecutor, ChildPurposed, ChildResult>& child) && {
        matchingAnyOf(child);
        return std::move(*this);
    }

    template <Entity Child, class ChildExecutor, bool ChildPurposed, class ChildResult>
        requires(!ChildPurposed)
    EntityQuery&
    matchAnyOf(const EntityQuery<Child, ChildExecutor, ChildPurposed, ChildResult>& child) & {
        return matchingAnyOf(child);
    }

    template <Entity Child, class ChildExecutor, bool ChildPurposed, class ChildResult>
        requires(!ChildPurposed)
    EntityQuery&&
    matchAnyOf(const EntityQuery<Child, ChildExecutor, ChildPurposed, ChildResult>& child) && {
        matchingAnyOf(child);
        return std::move(*this);
    }

    EntityQuery& enhanceChild(const request::QuerySelection& child) & {
        auto r = memory_resource();
        if (r != child.memory_resource()) {
            throw std::invalid_argument("Child enhancement requires an explicit destination copy");
        }
        auto& value = description();
        request::QuerySelection captured(std::allocator_arg, Allocator(r), child);
        auto query = std::move(captured).into_query();
        value.child_selections.emplace_back(child);
        value.query.child_enhancement(query);
        return *this;
    }

    EntityQuery&& enhanceChild(const request::QuerySelection& child) && {
        enhanceChild(child);
        return std::move(*this);
    }

    EntityQuery& enhanceChild(const SelectQuery& child) & {
        if (memory_resource() != child.memory_resource()) {
            throw std::invalid_argument("Child enhancement requires an explicit destination copy");
        }
        request::QuerySelection selection(std::allocator_arg, Allocator(memory_resource()), child);
        return enhanceChild(selection);
    }

    EntityQuery&& enhanceChild(const SelectQuery& child) && {
        enhanceChild(child);
        return std::move(*this);
    }

    template <Entity Child, class ChildExecutor, bool ChildPurposed, class ChildResult>
        requires(!ChildPurposed)
    EntityQuery&
    enhanceChild(const EntityQuery<Child, ChildExecutor, ChildPurposed, ChildResult>& child) & {
        requireCompatible(child);
        if (memory_resource() != child.memory_resource()) {
            throw std::invalid_argument("Child enhancement requires an explicit destination copy");
        }
        return enhanceChild(child.selection());
    }

    template <Entity Child, class ChildExecutor, bool ChildPurposed, class ChildResult>
        requires(!ChildPurposed)
    EntityQuery&&
    enhanceChild(const EntityQuery<Child, ChildExecutor, ChildPurposed, ChildResult>& child) && {
        enhanceChild(child);
        return std::move(*this);
    }

    EntityQuery copyTo(UserContext& destination) const {
        const auto& source = description();
        auto copy = destination.make_ref<detail::QueryDescription>(source);
        return EntityQuery(ContextBinding(destination), copy);
    }

    EntityQuery copyToGlobal(UserContext& destination) const {
        const auto& source = description();
        auto copy = destination.make_global<detail::QueryDescription>(source);
        return EntityQuery(ContextBinding(destination), copy);
    }

    EntityQuery rebindGlobal(UserContext& destination) const {
        if (!detail::reference_global(description_.id())) {
            throw ReferenceError(ReferenceError::Kind::NotGlobal,
                                 description_.id(),
                                 std::source_location::current());
        }
        auto rebound = destination.rebind_global(description_);
        return EntityQuery(ContextBinding(destination), rebound);
    }

    EntityQuery<T, E, true, R> purpose(std::string_view value) const& {
        auto copy = binding_.get().make_ref<detail::QueryDescription>(description());
        binding_.get().get(copy).intent = value;
        return EntityQuery<T, E, true, R>(binding_, copy);
    }

    EntityQuery<T, E, true, R> purpose(std::string_view value) && {
        description().intent = value;
        return EntityQuery<T, E, true, R>(binding_, std::exchange(description_, {}));
    }

    EntityQuery& comment(std::string_view value) & {
        std::erase_if(description().query.trace_chain, [](const auto& node) {
            return node.kind == TraceKind::Comment;
        });
        description().query.comment(value);
        return *this;
    }

    EntityQuery&& comment(std::string_view value) && {
        comment(value);
        return std::move(*this);
    }

    EntityQuery& select(std::string_view field) & {
        description().query.project(field);
        return *this;
    }

    EntityQuery&& select(std::string_view field) && {
        select(field);
        return std::move(*this);
    }

    EntityQuery& withDeletedRows() & {
        auto& query = description().query;
        query.predicate = request::remove_default_live_filter(memory_resource(), query.predicate);
        return *this;
    }

    EntityQuery&& withDeletedRows() && {
        withDeletedRows();
        return std::move(*this);
    }

    EntityQuery& deletedRowsOnly() & {
        withDeletedRows();
        description().query.and_filter(Expr::lte(memory_resource(), "version", std::int64_t(0)));
        return *this;
    }

    EntityQuery&& deletedRowsOnly() && {
        deletedRowsOnly();
        return std::move(*this);
    }

    EntityQuery& appendSearchCriteria(const Expr& criteria) & {
        andFilter(criteria);
        return *this;
    }

    EntityQuery&& appendSearchCriteria(const Expr& criteria) && {
        appendSearchCriteria(criteria);
        return std::move(*this);
    }

    EntityQuery& appendSearchCriteria(Ref<Expr> criteria) & {
        return appendSearchCriteria(binding_.get().get(criteria));
    }

    EntityQuery&& appendSearchCriteria(Ref<Expr> criteria) && {
        appendSearchCriteria(criteria);
        return std::move(*this);
    }

    Ref<Expr> createFieldCriteria(std::string_view field,
                                  request::FieldOperator op,
                                  std::span<const Value> values) const {
        auto criterion = request::field_operator_expr(memory_resource(), field, op, values);
        if (detail::reference_global(description_.id())) {
            return binding_.get().template make_global<Expr>(std::move(criterion));
        }
        return binding_.get().template make_ref<Expr>(std::move(criterion));
    }

    template <std::ranges::input_range Inputs>
        requires std::constructible_from<Value, Memory, std::ranges::range_reference_t<Inputs>>
    Ref<Expr>
    createFieldCriteria(std::string_view field, request::FieldOperator op, Inputs&& inputs) const {
        auto values = criteria_values(std::forward<Inputs>(inputs));
        return createFieldCriteria(field, op, std::span<const Value>(values));
    }

    EntityQuery& filterField(std::string_view field,
                             request::FieldOperator op,
                             std::span<const Value> values) & {
        description().query.and_filter(
            request::field_operator_expr(memory_resource(), field, op, values));
        return *this;
    }

    EntityQuery&& filterField(std::string_view field,
                              request::FieldOperator op,
                              std::span<const Value> values) && {
        filterField(field, op, values);
        return std::move(*this);
    }

    template <std::ranges::input_range Inputs>
        requires std::constructible_from<Value, Memory, std::ranges::range_reference_t<Inputs>>
    EntityQuery& filterField(std::string_view field, request::FieldOperator op, Inputs&& inputs) & {
        auto values = criteria_values(std::forward<Inputs>(inputs));
        return filterField(field, op, std::span<const Value>(values));
    }

    template <std::ranges::input_range Inputs>
        requires std::constructible_from<Value, Memory, std::ranges::range_reference_t<Inputs>>
    EntityQuery&&
    filterField(std::string_view field, request::FieldOperator op, Inputs&& inputs) && {
        filterField(field, op, std::forward<Inputs>(inputs));
        return std::move(*this);
    }

    EntityQuery&
    filterProperty(std::string_view left, request::FieldOperator op, std::string_view right) & {
        description().query.and_filter(
            request::field_operator_column_expr(memory_resource(), left, op, right));
        return *this;
    }

    EntityQuery&&
    filterProperty(std::string_view left, request::FieldOperator op, std::string_view right) && {
        filterProperty(left, op, right);
        return std::move(*this);
    }

    EntityQuery& pageOffset(std::uint64_t offset, std::uint64_t size) & {
        description().query.page(offset, size);
        return *this;
    }

    EntityQuery&& pageOffset(std::uint64_t offset, std::uint64_t size) && {
        pageOffset(offset, size);
        return std::move(*this);
    }

    EntityQuery& offset(std::uint64_t offset, std::uint64_t size) & {
        pageOffset(offset, size);
        return *this;
    }

    EntityQuery&& offset(std::uint64_t offset, std::uint64_t size) && {
        this->offset(offset, size);
        return std::move(*this);
    }

    EntityQuery& offsetOnly(std::uint64_t value) & {
        offset(value);
        return *this;
    }

    EntityQuery&& offsetOnly(std::uint64_t value) && {
        offsetOnly(value);
        return std::move(*this);
    }

    EntityQuery& offsetSize(std::uint64_t offset, std::uint64_t size) & {
        pageOffset(offset, size);
        return *this;
    }

    EntityQuery&& offsetSize(std::uint64_t offset, std::uint64_t size) && {
        offsetSize(offset, size);
        return std::move(*this);
    }

    EntityQuery& top(std::uint64_t value) & {
        limit(value);
        return *this;
    }

    EntityQuery&& top(std::uint64_t value) && {
        top(value);
        return std::move(*this);
    }

    EntityQuery& pageNumber(std::uint64_t number, std::uint64_t size) & {
        auto previous = number ? number - 1 : 0;
        auto maximum = std::numeric_limits<std::uint64_t>::max();
        auto start = size && previous > maximum / size ? maximum : previous * size;
        pageOffset(start, size);
        return *this;
    }

    EntityQuery&& pageNumber(std::uint64_t number, std::uint64_t size) && {
        pageNumber(number, size);
        return std::move(*this);
    }

    EntityQuery& pageNumberDefault(std::uint64_t number) & {
        pageNumber(number, 10);
        return *this;
    }

    EntityQuery&& pageNumberDefault(std::uint64_t number) && {
        pageNumberDefault(number);
        return std::move(*this);
    }

    EntityQuery& page(std::uint64_t number, std::uint64_t size) & {
        pageNumber(number, size);
        return *this;
    }

    EntityQuery&& page(std::uint64_t number, std::uint64_t size) && {
        page(number, size);
        return std::move(*this);
    }

    EntityQuery& pageDefault(std::uint64_t number) & {
        pageNumberDefault(number);
        return *this;
    }

    EntityQuery&& pageDefault(std::uint64_t number) && {
        pageDefault(number);
        return std::move(*this);
    }

    EntityQuery& streamDefault() & {
        description().query.stream_default();
        return *this;
    }

    EntityQuery&& streamDefault() && {
        streamDefault();
        return std::move(*this);
    }

    EntityQuery& topNProbeParentThreshold(std::size_t threshold) & {
        description().query.top_n_probe_parent_threshold(threshold);
        return *this;
    }

    EntityQuery&& topNProbeParentThreshold(std::size_t threshold) && {
        topNProbeParentThreshold(threshold);
        return std::move(*this);
    }

    EntityQuery& selectAs(std::string_view field, std::string_view alias) & {
        description().query.project_expr(alias, Expr::column(memory_resource(), field));
        return *this;
    }

    EntityQuery&& selectAs(std::string_view field, std::string_view alias) && {
        selectAs(field, alias);
        return std::move(*this);
    }

    EntityQuery& selectWithFunction(std::string_view field,
                                    std::string_view alias,
                                    AggregateFunction function) & {
        description().query.aggregate(
            Aggregate(std::allocator_arg, Allocator(memory_resource()), function, field, alias));
        return *this;
    }

    EntityQuery&& selectWithFunction(std::string_view field,
                                     std::string_view alias,
                                     AggregateFunction function) && {
        selectWithFunction(field, alias, function);
        return std::move(*this);
    }

    EntityQuery& unselect(std::string_view field) & {
        auto& query = description().query;
        std::erase(query.projection, field);
        std::erase_if(query.raw_projections, [&](const auto& projection) {
            return projection.property_name == field;
        });
        return *this;
    }

    EntityQuery&& unselect(std::string_view field) && {
        unselect(field);
        return std::move(*this);
    }

    EntityQuery& unselectRelation(std::string_view field, std::string_view relation) & {
        auto& query = description().query;
        std::erase(query.projection, field);
        std::erase_if(query.relations, [&](const auto& selection) {
            return selection.name == relation;
        });
        return *this;
    }

    EntityQuery&& unselectRelation(std::string_view field, std::string_view relation) && {
        unselectRelation(field, relation);
        return std::move(*this);
    }

    EntityQuery& rawSql(std::string_view sql) & {
        auto& query = description().query;
        query.raw_sql_text.emplace(sql, memory_resource());
        return *this;
    }

    EntityQuery&& rawSql(std::string_view sql) && {
        rawSql(sql);
        return std::move(*this);
    }

    EntityQuery& rawSqlFilter(std::string_view sql) & {
        description().query.raw_sql_search_criteria.emplace_back(sql);
        return *this;
    }

    EntityQuery&& rawSqlFilter(std::string_view sql) && {
        rawSqlFilter(sql);
        return std::move(*this);
    }

    EntityQuery& createPropertyAs(std::string_view field, std::string_view sql) & {
        description().query.dynamic_property_raw(field, sql);
        return *this;
    }

    EntityQuery&& createPropertyAs(std::string_view field, std::string_view sql) && {
        createPropertyAs(field, sql);
        return std::move(*this);
    }

    EntityQuery& selectRaw(std::string_view field, std::string_view sql) & {
        description().query.project_raw(field, sql);
        return *this;
    }

    EntityQuery&& selectRaw(std::string_view field, std::string_view sql) && {
        selectRaw(field, sql);
        return std::move(*this);
    }

    EntityQuery& unsafeRawSql(request::UnsafeRawSqlSegment sql) & {
        auto value = std::move(sql).into_sql();
        rawSql(value);
        return *this;
    }

    EntityQuery&& unsafeRawSql(request::UnsafeRawSqlSegment sql) && {
        unsafeRawSql(std::move(sql));
        return std::move(*this);
    }

    EntityQuery& unsafeRawSqlFilter(request::UnsafeRawSqlSegment sql) & {
        auto value = std::move(sql).into_sql();
        rawSqlFilter(value);
        return *this;
    }

    EntityQuery&& unsafeRawSqlFilter(request::UnsafeRawSqlSegment sql) && {
        unsafeRawSqlFilter(std::move(sql));
        return std::move(*this);
    }

    EntityQuery& unsafeCreatePropertyAs(std::string_view field,
                                        request::UnsafeRawSqlSegment sql) & {
        auto value = std::move(sql).into_sql();
        createPropertyAs(field, value);
        return *this;
    }

    EntityQuery&& unsafeCreatePropertyAs(std::string_view field,
                                         request::UnsafeRawSqlSegment sql) && {
        unsafeCreatePropertyAs(field, std::move(sql));
        return std::move(*this);
    }

    EntityQuery& selectUnsafeRaw(std::string_view field, request::UnsafeRawSqlSegment sql) & {
        auto value = std::move(sql).into_sql();
        selectRaw(field, value);
        return *this;
    }

    EntityQuery&& selectUnsafeRaw(std::string_view field, request::UnsafeRawSqlSegment sql) && {
        selectUnsafeRaw(field, std::move(sql));
        return std::move(*this);
    }

    EntityQuery& groupBy(std::string_view field) & {
        description().query.group_by(field);
        return *this;
    }

    EntityQuery&& groupBy(std::string_view field) && {
        this->groupBy(field);
        return std::move(*this);
    }

    EntityQuery& count() & {
        countAs("count");
        return *this;
    }

    EntityQuery&& count() && {
        this->count();
        return std::move(*this);
    }

    EntityQuery& countAs(std::string_view alias) & {
        aggregateCount(alias);
        return *this;
    }

    EntityQuery&& countAs(std::string_view alias) && {
        this->countAs(alias);
        return std::move(*this);
    }

    EntityQuery& aggregateCount(std::string_view alias) & {
        description().query.count(alias);
        return *this;
    }

    EntityQuery&& aggregateCount(std::string_view alias) && {
        this->aggregateCount(alias);
        return std::move(*this);
    }

    EntityQuery& aggregateCountField(std::string_view field, std::string_view alias) & {
        description().query.count_field(field, alias);
        return *this;
    }

    EntityQuery&& aggregateCountField(std::string_view field, std::string_view alias) && {
        this->aggregateCountField(field, alias);
        return std::move(*this);
    }

    EntityQuery& aggregateWithFunction(std::string_view field,
                                       std::string_view alias,
                                       AggregateFunction function) & {
        description().query.aggregate(
            Aggregate(std::allocator_arg, Allocator(memory_resource()), function, field, alias));
        return *this;
    }

    EntityQuery&& aggregateWithFunction(std::string_view field,
                                        std::string_view alias,
                                        AggregateFunction function) && {
        this->aggregateWithFunction(field, alias, function);
        return std::move(*this);
    }

    EntityQuery& enableAggregationCache() & {
        description().query.enable_aggregation_cache();
        return *this;
    }

    EntityQuery&& enableAggregationCache() && {
        this->enableAggregationCache();
        return std::move(*this);
    }

    EntityQuery& enableAggregationCacheFor(std::uint64_t duration) & {
        description().query.enable_aggregation_cache_for(duration);
        return *this;
    }

    EntityQuery&& enableAggregationCacheFor(std::uint64_t duration) && {
        this->enableAggregationCacheFor(duration);
        return std::move(*this);
    }

    EntityQuery& propagateAggregationCache(std::uint64_t duration) & {
        description().query.propagate_aggregation_cache(duration);
        return *this;
    }

    EntityQuery&& propagateAggregationCache(std::uint64_t duration) && {
        this->propagateAggregationCache(duration);
        return std::move(*this);
    }

    EntityQuery& aggregateSum(std::string_view field, std::string_view alias) & {
        description().query.sum(field, alias);
        return *this;
    }

    EntityQuery&& aggregateSum(std::string_view field, std::string_view alias) && {
        this->aggregateSum(field, alias);
        return std::move(*this);
    }

    EntityQuery& aggregateAvg(std::string_view field, std::string_view alias) & {
        description().query.avg(field, alias);
        return *this;
    }

    EntityQuery&& aggregateAvg(std::string_view field, std::string_view alias) && {
        this->aggregateAvg(field, alias);
        return std::move(*this);
    }

    EntityQuery& aggregateMin(std::string_view field, std::string_view alias) & {
        description().query.min(field, alias);
        return *this;
    }

    EntityQuery&& aggregateMin(std::string_view field, std::string_view alias) && {
        this->aggregateMin(field, alias);
        return std::move(*this);
    }

    EntityQuery& aggregateMax(std::string_view field, std::string_view alias) & {
        description().query.max(field, alias);
        return *this;
    }

    EntityQuery&& aggregateMax(std::string_view field, std::string_view alias) && {
        this->aggregateMax(field, alias);
        return std::move(*this);
    }

    EntityQuery& aggregateStddev(std::string_view field, std::string_view alias) & {
        description().query.stddev(field, alias);
        return *this;
    }

    EntityQuery&& aggregateStddev(std::string_view field, std::string_view alias) && {
        this->aggregateStddev(field, alias);
        return std::move(*this);
    }

    EntityQuery& aggregateStddevPop(std::string_view field, std::string_view alias) & {
        description().query.stddev_pop(field, alias);
        return *this;
    }

    EntityQuery&& aggregateStddevPop(std::string_view field, std::string_view alias) && {
        this->aggregateStddevPop(field, alias);
        return std::move(*this);
    }

    EntityQuery& aggregateVarSamp(std::string_view field, std::string_view alias) & {
        description().query.var_samp(field, alias);
        return *this;
    }

    EntityQuery&& aggregateVarSamp(std::string_view field, std::string_view alias) && {
        this->aggregateVarSamp(field, alias);
        return std::move(*this);
    }

    EntityQuery& aggregateVarPop(std::string_view field, std::string_view alias) & {
        description().query.var_pop(field, alias);
        return *this;
    }

    EntityQuery&& aggregateVarPop(std::string_view field, std::string_view alias) && {
        this->aggregateVarPop(field, alias);
        return std::move(*this);
    }

    EntityQuery& aggregateBitAnd(std::string_view field, std::string_view alias) & {
        description().query.bit_and(field, alias);
        return *this;
    }

    EntityQuery&& aggregateBitAnd(std::string_view field, std::string_view alias) && {
        this->aggregateBitAnd(field, alias);
        return std::move(*this);
    }

    EntityQuery& aggregateBitOr(std::string_view field, std::string_view alias) & {
        description().query.bit_or(field, alias);
        return *this;
    }

    EntityQuery&& aggregateBitOr(std::string_view field, std::string_view alias) && {
        this->aggregateBitOr(field, alias);
        return std::move(*this);
    }

    EntityQuery& aggregateBitXor(std::string_view field, std::string_view alias) & {
        description().query.bit_xor(field, alias);
        return *this;
    }

    EntityQuery&& aggregateBitXor(std::string_view field, std::string_view alias) && {
        this->aggregateBitXor(field, alias);
        return std::move(*this);
    }

    EntityQuery& selectDynamicFields(const DynamicFieldSelection& selection) & {
        description().query.select_dynamic_fields(selection);
        return *this;
    }

    EntityQuery&& selectDynamicFields(const DynamicFieldSelection& selection) && {
        selectDynamicFields(selection);
        return std::move(*this);
    }

    EntityQuery& filter(const Expr& value) & {
        require_expression_domain(value);
        description().query.filter(value);
        return *this;
    }

    EntityQuery&& filter(const Expr& value) && {
        filter(value);
        return std::move(*this);
    }

    EntityQuery& andFilter(const Expr& value) & {
        require_expression_domain(value);
        description().query.and_filter(value);
        return *this;
    }

    EntityQuery&& andFilter(const Expr& value) && {
        andFilter(value);
        return std::move(*this);
    }

    EntityQuery& orFilter(const Expr& value) & {
        require_expression_domain(value);
        description().query.or_filter(value);
        return *this;
    }

    EntityQuery&& orFilter(const Expr& value) && {
        orFilter(value);
        return std::move(*this);
    }

    EntityQuery& orderAsc(std::string_view field) & {
        description().query.order_asc(field);
        return *this;
    }

    EntityQuery&& orderAsc(std::string_view field) && {
        orderAsc(field);
        return std::move(*this);
    }

    EntityQuery& orderDesc(std::string_view field) & {
        description().query.order_desc(field);
        return *this;
    }

    EntityQuery&& orderDesc(std::string_view field) && {
        orderDesc(field);
        return std::move(*this);
    }

    EntityQuery& limit(std::uint64_t value) & {
        description().query.limit(value);
        return *this;
    }

    EntityQuery&& limit(std::uint64_t value) && {
        limit(value);
        return std::move(*this);
    }

    EntityQuery& unlimited() & {
        description().query.slice.reset();
        return *this;
    }

    EntityQuery&& unlimited() && {
        unlimited();
        return std::move(*this);
    }

    EntityQuery& optimizePaginationWithIdSetConfig(std::string_view name,
                                                   std::uint64_t ttl,
                                                   std::uint64_t max_ids) & {
        description().query.optimize_pagination_with_id_set_config(name, ttl, max_ids);
        return *this;
    }

    EntityQuery&& optimizePaginationWithIdSetConfig(std::string_view name,
                                                    std::uint64_t ttl,
                                                    std::uint64_t max_ids) && {
        optimizePaginationWithIdSetConfig(name, ttl, max_ids);
        return std::move(*this);
    }

    EntityQuery& optimizePaginationWithIdSet() & {
        description().query.optimize_pagination_with_id_set();
        return *this;
    }

    EntityQuery&& optimizePaginationWithIdSet() && {
        optimizePaginationWithIdSet();
        return std::move(*this);
    }

    EntityQuery& optimizeForContinuousPageFetchWith(std::string_view name, std::uint64_t ttl) & {
        description().query.optimize_for_continuous_page_fetch_with(name, ttl);
        return *this;
    }

    EntityQuery&& optimizeForContinuousPageFetchWith(std::string_view name, std::uint64_t ttl) && {
        optimizeForContinuousPageFetchWith(name, ttl);
        return std::move(*this);
    }

    EntityQuery& optimizeForContinuousPageFetch() & {
        description().query.optimize_for_continuous_page_fetch();
        return *this;
    }

    EntityQuery&& optimizeForContinuousPageFetch() && {
        optimizeForContinuousPageFetch();
        return std::move(*this);
    }

    EntityQuery& offset(std::uint64_t value) & {
        description().query.offset(value);
        return *this;
    }

    EntityQuery&& offset(std::uint64_t value) && {
        offset(value);
        return std::move(*this);
    }

    EntityQuery& hardLimit(std::uint64_t value) & {
        description().query.hard_limit(value);
        return *this;
    }

    EntityQuery&& hardLimit(std::uint64_t value) && {
        hardLimit(value);
        return std::move(*this);
    }

    EntityQuery& stream(std::size_t chunk_size) & {
        if (chunk_size == 0) {
            throw std::invalid_argument("stream chunk size must be positive");
        }
        description().query.stream(chunk_size);
        return *this;
    }

    EntityQuery&& stream(std::size_t chunk_size) && {
        stream(chunk_size);
        return std::move(*this);
    }

    EntityQuery& selectRelation(std::string_view relation) & {
        description().query.relation(relation);
        return *this;
    }

    EntityQuery&& selectRelation(std::string_view relation) && {
        selectRelation(relation);
        return std::move(*this);
    }

    template <class V>
    EntityQuery& whereEqual(std::string_view field, const V& value) & {
        description().query.and_filter(Expr::eq(memory_resource(), field, value));
        return *this;
    }

    template <class V>
    EntityQuery&& whereEqual(std::string_view field, const V& value) && {
        whereEqual(field, value);
        return std::move(*this);
    }

    EntityQuery& selectRelation(std::string_view relation, const request::QuerySelection& child) & {
        auto r = memory_resource();
        if (r != child.memory_resource()) {
            throw std::invalid_argument("Relation selection requires an explicit destination copy");
        }
        request::QuerySelection captured(std::allocator_arg, Allocator(r), child);
        auto query = std::move(captured).into_query();
        description().query.relation_query(relation, query);
        return *this;
    }

    EntityQuery&& selectRelation(std::string_view relation,
                                 const request::QuerySelection& child) && {
        selectRelation(relation, child);
        return std::move(*this);
    }

    EntityQuery& selectRelation(std::string_view relation, const SelectQuery& child) & {
        if (memory_resource() != child.memory_resource()) {
            throw std::invalid_argument("Relation selection requires an explicit destination copy");
        }
        request::QuerySelection selection(std::allocator_arg, Allocator(memory_resource()), child);
        return selectRelation(relation, selection);
    }

    EntityQuery&& selectRelation(std::string_view relation, const SelectQuery& child) && {
        selectRelation(relation, child);
        return std::move(*this);
    }

    template <Entity Child, class ChildExecutor, bool ChildPurposed, class ChildResult>
        requires(!ChildPurposed)
    EntityQuery&
    selectRelation(std::string_view relation,
                   const EntityQuery<Child, ChildExecutor, ChildPurposed, ChildResult>& child) & {
        requireCompatible(child);
        return selectRelation(relation, child.selection());
    }

    template <Entity Child, class ChildExecutor, bool ChildPurposed, class ChildResult>
        requires(!ChildPurposed)
    EntityQuery&&
    selectRelation(std::string_view relation,
                   const EntityQuery<Child, ChildExecutor, ChildPurposed, ChildResult>& child) && {
        selectRelation(relation, child);
        return std::move(*this);
    }

    EntityQuery& aggregateRelation(std::string_view relation,
                                   std::string_view alias,
                                   const request::QuerySelection& child,
                                   bool single_result) & {
        auto r = memory_resource();
        if (r != child.memory_resource()) {
            throw std::invalid_argument("Relation analysis requires an explicit destination copy");
        }
        request::QuerySelection captured(std::allocator_arg, Allocator(r), child);
        auto query = std::move(captured).into_query();
        description().analysis_origins.relation_aggregates.emplace_back(
            relation, alias, child, single_result);
        description().query.relation_aggregates.emplace_back(relation, alias, query, single_result);
        return *this;
    }

    EntityQuery&& aggregateRelation(std::string_view relation,
                                    std::string_view alias,
                                    const request::QuerySelection& child,
                                    bool single_result) && {
        aggregateRelation(relation, alias, child, single_result);
        return std::move(*this);
    }

    EntityQuery& aggregateRelation(std::string_view relation,
                                   std::string_view alias,
                                   const SelectQuery& child,
                                   bool single_result) & {
        if (memory_resource() != child.memory_resource()) {
            throw std::invalid_argument("Relation analysis requires an explicit destination copy");
        }
        request::QuerySelection selection(std::allocator_arg, Allocator(memory_resource()), child);
        return aggregateRelation(relation, alias, selection, single_result);
    }

    EntityQuery&& aggregateRelation(std::string_view relation,
                                    std::string_view alias,
                                    const SelectQuery& child,
                                    bool single_result) && {
        aggregateRelation(relation, alias, child, single_result);
        return std::move(*this);
    }

    template <Entity Child, class ChildExecutor, bool ChildPurposed, class ChildResult>
        requires(!ChildPurposed)
    EntityQuery&
    aggregateRelation(std::string_view relation,
                      std::string_view alias,
                      const EntityQuery<Child, ChildExecutor, ChildPurposed, ChildResult>& child,
                      bool single_result) & {
        requireCompatible(child);
        return aggregateRelation(relation, alias, child.selection(), single_result);
    }

    template <Entity Child, class ChildExecutor, bool ChildPurposed, class ChildResult>
        requires(!ChildPurposed)
    EntityQuery&&
    aggregateRelation(std::string_view relation,
                      std::string_view alias,
                      const EntityQuery<Child, ChildExecutor, ChildPurposed, ChildResult>& child,
                      bool single_result) && {
        aggregateRelation(relation, alias, child, single_result);
        return std::move(*this);
    }

    EntityQuery& groupByObject(std::string_view relation,
                               std::string_view storage_field,
                               const request::QuerySelection& child) & {
        auto r = memory_resource();
        if (r != child.memory_resource()) {
            throw std::invalid_argument("Object grouping requires an explicit destination copy");
        }
        request::QuerySelection captured(std::allocator_arg, Allocator(r), child);
        auto query = std::move(captured).into_query();
        auto& value = description();
        value.query.group_by(storage_field);
        value.analysis_origins.object_group_bys.emplace_back(relation, storage_field, child);
        value.query.object_group_bys.emplace_back(relation, storage_field, query);
        return *this;
    }

    EntityQuery&& groupByObject(std::string_view relation,
                                std::string_view storage_field,
                                const request::QuerySelection& child) && {
        groupByObject(relation, storage_field, child);
        return std::move(*this);
    }

    EntityQuery& groupByObject(std::string_view relation,
                               std::string_view storage_field,
                               const SelectQuery& child) & {
        if (memory_resource() != child.memory_resource()) {
            throw std::invalid_argument("Object grouping requires an explicit destination copy");
        }
        request::QuerySelection selection(std::allocator_arg, Allocator(memory_resource()), child);
        return groupByObject(relation, storage_field, selection);
    }

    EntityQuery&& groupByObject(std::string_view relation,
                                std::string_view storage_field,
                                const SelectQuery& child) && {
        groupByObject(relation, storage_field, child);
        return std::move(*this);
    }

    template <Entity Child, class ChildExecutor, bool ChildPurposed, class ChildResult>
        requires(!ChildPurposed)
    EntityQuery&
    groupByObject(std::string_view relation,
                  std::string_view storage_field,
                  const EntityQuery<Child, ChildExecutor, ChildPurposed, ChildResult>& child) & {
        requireCompatible(child);
        return groupByObject(relation, storage_field, child.selection());
    }

    template <Entity Child, class ChildExecutor, bool ChildPurposed, class ChildResult>
        requires(!ChildPurposed)
    EntityQuery&&
    groupByObject(std::string_view relation,
                  std::string_view storage_field,
                  const EntityQuery<Child, ChildExecutor, ChildPurposed, ChildResult>& child) && {
        groupByObject(relation, storage_field, child);
        return std::move(*this);
    }

    EntityQuery& filterRelation(std::string_view local_key,
                                std::string_view foreign_key,
                                const EntityDescriptor& target,
                                const request::QuerySelection& child,
                                bool negative = false,
                                std::string_view relation = {}) & {
        auto r = memory_resource();
        if (r != child.memory_resource()) {
            throw std::invalid_argument("Relation matching requires an explicit destination copy");
        }
        auto query = child.query.clone(r);
        query.purpose_text.reset();
        if (!relation.empty()) {
            description().relation_filters.emplace_back(relation, child);
        }
        description().query.and_filter(
            negative ? Expr::not_in_subquery(r, local_key, target, query, foreign_key)
                     : Expr::in_subquery(r, local_key, target, query, foreign_key));
        return *this;
    }

    EntityQuery&& filterRelation(std::string_view local_key,
                                 std::string_view foreign_key,
                                 const EntityDescriptor& target,
                                 const request::QuerySelection& child,
                                 bool negative = false,
                                 std::string_view relation = {}) && {
        filterRelation(local_key, foreign_key, target, child, negative, relation);
        return std::move(*this);
    }

    EntityQuery& filterRelation(std::string_view local_key,
                                std::string_view foreign_key,
                                const EntityDescriptor& target,
                                const SelectQuery& child,
                                bool negative = false,
                                std::string_view relation = {}) & {
        if (memory_resource() != child.memory_resource()) {
            throw std::invalid_argument("Relation matching requires an explicit destination copy");
        }
        request::QuerySelection selection(std::allocator_arg, Allocator(memory_resource()), child);
        return filterRelation(local_key, foreign_key, target, selection, negative, relation);
    }

    EntityQuery&& filterRelation(std::string_view local_key,
                                 std::string_view foreign_key,
                                 const EntityDescriptor& target,
                                 const SelectQuery& child,
                                 bool negative = false,
                                 std::string_view relation = {}) && {
        filterRelation(local_key, foreign_key, target, child, negative, relation);
        return std::move(*this);
    }

    template <Entity Child, class ChildExecutor, bool ChildPurposed, class ChildResult>
        requires(!ChildPurposed)
    EntityQuery&
    filterRelation(std::string_view local_key,
                   std::string_view foreign_key,
                   const EntityDescriptor& target,
                   const EntityQuery<Child, ChildExecutor, ChildPurposed, ChildResult>& child,
                   bool negative = false,
                   std::string_view relation = {}) & {
        requireCompatible(child);
        return filterRelation(
            local_key, foreign_key, target, child.selection(), negative, relation);
    }

    template <Entity Child, class ChildExecutor, bool ChildPurposed, class ChildResult>
        requires(!ChildPurposed)
    EntityQuery&&
    filterRelation(std::string_view local_key,
                   std::string_view foreign_key,
                   const EntityDescriptor& target,
                   const EntityQuery<Child, ChildExecutor, ChildPurposed, ChildResult>& child,
                   bool negative = false,
                   std::string_view relation = {}) && {
        filterRelation(local_key, foreign_key, target, child, negative, relation);
        return std::move(*this);
    }

    template <Entity Child, class ChildExecutor, bool ChildPurposed, class ChildResult>
        requires(!ChildPurposed)
    EntityQuery&
    filterRelation(std::string_view local_key,
                   std::string_view foreign_key,
                   const EntityQuery<Child, ChildExecutor, ChildPurposed, ChildResult>& child,
                   bool negative = false,
                   std::string_view relation = {}) & {
        auto target = Child::entity_descriptor(memory_resource());
        return filterRelation(local_key, foreign_key, target, child, negative, relation);
    }

    template <Entity Child, class ChildExecutor, bool ChildPurposed, class ChildResult>
        requires(!ChildPurposed)
    EntityQuery&&
    filterRelation(std::string_view local_key,
                   std::string_view foreign_key,
                   const EntityQuery<Child, ChildExecutor, ChildPurposed, ChildResult>& child,
                   bool negative = false,
                   std::string_view relation = {}) && {
        filterRelation(local_key, foreign_key, child, negative, relation);
        return std::move(*this);
    }

    // Field statistics materialize their selection before adding the scalar
    // operation, then store the bare query in a fresh selection as in the source.
    EntityQuery& aggregateRelationField(std::string_view relation,
                                        std::string_view alias,
                                        const request::QuerySelection& child,
                                        std::string_view field,
                                        std::string_view query_alias,
                                        AggregateFunction function) & {
        auto r = memory_resource();
        if (r != child.memory_resource()) {
            throw std::invalid_argument(
                "Relation field statistics require an explicit destination copy");
        }
        request::QuerySelection captured(std::allocator_arg, Allocator(r), child);
        auto origin_query = std::move(captured).into_query();
        origin_query.aggregate(
            Aggregate(std::allocator_arg, Allocator(r), function, field, query_alias));
        auto query = origin_query.clone(r);
        query.raw_sql_text.reset();
        query.raw_sql_search_criteria.clear();
        query.dynamic_properties.clear();
        query.raw_projections.clear();
        query.facets.clear();
        query.relation_aggregates.clear();
        query.object_group_bys.clear();
        query.child_enhancements.clear();
        description().analysis_origins.relation_aggregates.emplace_back(
            relation,
            alias,
            request::QuerySelection(std::allocator_arg, Allocator(r), origin_query),
            true);
        description().query.relation_aggregates.emplace_back(relation, alias, query, true);
        return *this;
    }

    EntityQuery&& aggregateRelationField(std::string_view relation,
                                         std::string_view alias,
                                         const request::QuerySelection& child,
                                         std::string_view field,
                                         std::string_view query_alias,
                                         AggregateFunction function) && {
        aggregateRelationField(relation, alias, child, field, query_alias, function);
        return std::move(*this);
    }

    EntityQuery& aggregateRelationField(std::string_view relation,
                                        std::string_view alias,
                                        const SelectQuery& child,
                                        std::string_view field,
                                        std::string_view query_alias,
                                        AggregateFunction function) & {
        if (memory_resource() != child.memory_resource()) {
            throw std::invalid_argument(
                "Relation field statistics require an explicit destination copy");
        }
        request::QuerySelection selection(std::allocator_arg, Allocator(memory_resource()), child);
        return aggregateRelationField(relation, alias, selection, field, query_alias, function);
    }

    EntityQuery&& aggregateRelationField(std::string_view relation,
                                         std::string_view alias,
                                         const SelectQuery& child,
                                         std::string_view field,
                                         std::string_view query_alias,
                                         AggregateFunction function) && {
        aggregateRelationField(relation, alias, child, field, query_alias, function);
        return std::move(*this);
    }

    template <Entity Child, class ChildExecutor, bool ChildPurposed, class ChildResult>
        requires(!ChildPurposed)
    EntityQuery& aggregateRelationField(
        std::string_view relation,
        std::string_view alias,
        const EntityQuery<Child, ChildExecutor, ChildPurposed, ChildResult>& child,
        std::string_view field,
        std::string_view query_alias,
        AggregateFunction function) & {
        requireCompatible(child);
        return aggregateRelationField(
            relation, alias, child.selection(), field, query_alias, function);
    }

    template <Entity Child, class ChildExecutor, bool ChildPurposed, class ChildResult>
        requires(!ChildPurposed)
    EntityQuery&& aggregateRelationField(
        std::string_view relation,
        std::string_view alias,
        const EntityQuery<Child, ChildExecutor, ChildPurposed, ChildResult>& child,
        std::string_view field,
        std::string_view query_alias,
        AggregateFunction function) && {
        aggregateRelationField(relation, alias, child, field, query_alias, function);
        return std::move(*this);
    }

    EntityQuery& facetBy(std::string_view relation,
                         std::string_view name,
                         const request::QuerySelection& child,
                         bool include_all = true) & {
        if (memory_resource() != child.memory_resource()) {
            throw std::invalid_argument("Facet capture requires an explicit destination copy");
        }
        description().query.facets.emplace_back(name, relation, child, include_all);
        return *this;
    }

    EntityQuery&& facetBy(std::string_view relation,
                          std::string_view name,
                          const request::QuerySelection& child,
                          bool include_all = true) && {
        facetBy(relation, name, child, include_all);
        return std::move(*this);
    }

    EntityQuery& facetBy(std::string_view relation,
                         std::string_view name,
                         const SelectQuery& child,
                         bool include_all = true) & {
        if (memory_resource() != child.memory_resource()) {
            throw std::invalid_argument("Facet capture requires an explicit destination copy");
        }
        request::QuerySelection selection(std::allocator_arg, Allocator(memory_resource()), child);
        return facetBy(relation, name, selection, include_all);
    }

    EntityQuery&& facetBy(std::string_view relation,
                          std::string_view name,
                          const SelectQuery& child,
                          bool include_all = true) && {
        facetBy(relation, name, child, include_all);
        return std::move(*this);
    }

    template <Entity Child, class ChildExecutor, bool ChildPurposed, class ChildResult>
        requires(!ChildPurposed)
    EntityQuery& facetBy(std::string_view relation,
                         std::string_view name,
                         const EntityQuery<Child, ChildExecutor, ChildPurposed, ChildResult>& child,
                         bool include_all = true) & {
        requireCompatible(child);
        return facetBy(relation, name, child.selection(), include_all);
    }

    template <Entity Child, class ChildExecutor, bool ChildPurposed, class ChildResult>
        requires(!ChildPurposed)
    EntityQuery&&
    facetBy(std::string_view relation,
            std::string_view name,
            const EntityQuery<Child, ChildExecutor, ChildPurposed, ChildResult>& child,
            bool include_all = true) && {
        facetBy(relation, name, child, include_all);
        return std::move(*this);
    }

    auto& executeForList(UserContext& user_context) const
        requires Purposed && Entity<R>
    {
        auto authorized = authorize(user_context);
        auto service = user_context.template entity_data_service<E>(query_entity_name());
        if (!service) {
            return detail::generated_query_lookup_failure<SmartList<R>, typename E::Error>(
                user_context.memory_resource(), service.error());
        }
        return detail::execute_generated_list_query<R>(service.value().get(), authorized);
    }

    auto& executeForOne(UserContext& user_context) const
        requires Purposed && Entity<R>
    {
        auto authorized = authorize(user_context);
        auto service = user_context.template entity_data_service<E>(query_entity_name());
        if (!service) {
            return detail::generated_query_lookup_failure<std::optional<R>, typename E::Error>(
                user_context.memory_resource(), service.error());
        }
        authorized.limit(1);
        auto& source = detail::execute_generated_list_query<R>(service.value().get(), authorized);
        using State = detail::EntityOneOperation<R, typename E::Error>;
        auto& first =
            make_operation<typename State::Output, State>(user_context.memory_resource(), source);
        return first;
    }

    auto& executeForFirst(UserContext& user_context) const
        requires Purposed && Entity<R>
    {
        return executeForOne(user_context);
    }

    auto& executeForCount(UserContext& user_context) const
        requires Purposed && Entity<R>
    {
        auto authorized = authorize(user_context);
        auto service = user_context.template entity_data_service<E>(query_entity_name());
        if (!service) {
            return detail::generated_query_lookup_failure<std::uint64_t, typename E::Error>(
                user_context.memory_resource(), service.error());
        }
        return detail::execute_generated_scalar_query<true>(service.value().get(), authorized);
    }

    auto& executeForExists(UserContext& user_context) const
        requires Purposed && Entity<R>
    {
        auto authorized = authorize(user_context);
        auto service = user_context.template entity_data_service<E>(query_entity_name());
        if (!service) {
            return detail::generated_query_lookup_failure<bool, typename E::Error>(
                user_context.memory_resource(), service.error());
        }
        return detail::execute_generated_scalar_query<false>(service.value().get(), authorized);
    }

    auto& executeForPage(UserContext& user_context, std::uint64_t offset, std::uint64_t limit) const
        requires Purposed && Entity<R>
    {
        auto authorized = authorize(user_context);
        auto service = user_context.template entity_data_service<E>(query_entity_name());
        if (!service) {
            return detail::generated_query_lookup_failure<SmartList<R>, typename E::Error>(
                user_context.memory_resource(), service.error());
        }
        using State = detail::GeneratedPageQueryOperation<R, E>;
        return make_operation<typename State::Output, State>(
            user_context.memory_resource(), service.value().get(), authorized, offset, limit);
    }

    auto& executeForRows(UserContext& user_context) const
        requires Purposed && Entity<R>
    {
        auto authorized = authorize(user_context);
        auto service = user_context.template entity_data_service<E>(query_entity_name());
        if (!service) {
            return detail::generated_query_lookup_failure<SmartList<CompactRow>, typename E::Error>(
                user_context.memory_resource(), service.error());
        }
        return detail::generated_query_result(service.value().get().fetch_smart_list(authorized));
    }

    auto& executeForStream(UserContext& user_context) const
        requires Purposed
                 && Entity<R> && std::derived_from<E, StreamQueryExecutor<typename E::Error>>
    {
        auto authorized = authorize(user_context);
        return detail::defer_generated_query_stream<R, E>(
            user_context, query_entity_name(), authorized);
    }

    T& newEntity(UserContext& user_context) const
        requires Purposed && Entity<R>
    {
        (void)authorize(user_context);
        auto& value = user_context.template make<T>();
        if constexpr (requires {
                          value.attachRuntimeStateRecursive(user_context.entity_runtime_state());
                      }) {
            value.attachRuntimeStateRecursive(user_context.entity_runtime_state());
        }
        constexpr auto id_entity_name = [] {
            if constexpr (requires { T::ID_SPACE_ENTITY_NAME; }) {
                return T::ID_SPACE_ENTITY_NAME;
            } else {
                return T::ENTITY_NAME;
            }
        }();
        auto id = user_context.next_id(id_entity_name);
        if (id) {
            if constexpr (requires { value.updateId(id.value()); }) {
                value.updateId(id.value());
            } else {
                value.update_id(id.value());
            }
        }
        value.mark_as_new();
        return value;
    }

private:
    template <std::ranges::input_range Inputs>
    Vector<Value> criteria_values(Inputs&& inputs) const {
        Vector<Value> values(memory_resource());
        for (auto&& value : inputs) {
            values.emplace_back(std::forward<decltype(value)>(value));
        }
        return values;
    }

    EntityQuery& appendMatchingQuery(const SelectQuery& child) {
        auto r = memory_resource();
        // QuerySelection options stay independent; direct core queries already
        // contain any metadata explicitly applied before this conversion.
        EntityDescriptor descriptor(std::allocator_arg, Allocator(r), child.entity);
        description().query.and_filter(Expr::in_subquery(r, "id", descriptor, child, "id"));
        return *this;
    }

    void require_expression_domain(const Expr& value) const {
        if (memory_resource() != value.memory_resource()) {
            throw std::invalid_argument("Query expressions require an explicit destination copy");
        }
    }

    explicit EntityQuery(ContextBinding binding, Ref<detail::QueryDescription> description)
        : binding_(binding), description_(description) {}

    detail::QueryDescription& description() const {
        if (!description_.id()) {
            throw std::logic_error("Cannot use a moved-from query description");
        }
        return binding_.get().get(description_);
    }

    PurposedSelectQuery authorize(UserContext& user_context) const {
        binding_.require_same(ContextBinding(user_context));
        auto& source = description();
        auto comment = source.query.comment_text ? std::string_view(*source.query.comment_text)
                                                 : std::string_view{};
        auto intent = std::string_view(source.intent);
        detail::trim(comment);
        detail::trim(intent);
        if (intent.empty()) {
            throw std::invalid_argument("query purpose must not be empty");
        }
        if (comment.empty()) {
            throw std::invalid_argument("query comment must not be empty");
        }
        // Freeze a Context-owned execution snapshot before returning asynchronous work.
        // Later builder modifications do not affect the operation.
        return PurposedSelectQuery(std::allocator_arg,
                                   Allocator(user_context.memory_resource()),
                                   source.query,
                                   source.intent);
    }

    ContextBinding binding_;
    Ref<detail::QueryDescription> description_;
};
}
