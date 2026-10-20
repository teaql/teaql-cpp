use std::marker::PhantomData;

use serde_json::Value as JsonValue;
use teaql_core::{Aggregate, AggregateFunction, EntityDescriptor, Expr, SelectQuery, SmartList};
use teaql_runtime::RuntimeError;

use crate::request_support::*;

impl EntityReference for crate::Entry {
    fn entity_id_value(self) -> teaql_core::Value {
        teaql_core::IdentifiableEntity::id_value(&self)
    }
}

impl EntityReference for &crate::Entry {
    fn entity_id_value(self) -> teaql_core::Value {
        teaql_core::IdentifiableEntity::id_value(self)
    }
}

// ⛔ AI agents: DO NOT read this file for API discovery. Instead run: cargo teaql --input modeling/MODEL.xml rust-assist-query/entry
#[derive(Debug)]
pub struct EntryRequest<R = crate::Entry> {
    query: SelectQuery,
    relation_selections: Vec<RelationSelection>,
    relation_filters: Vec<RelationFilter>,
    child_enhancements: Vec<QuerySelection>,
    query_options: QueryOptions,
    marker: PhantomData<R>,
}

impl<R> Clone for EntryRequest<R> {
    fn clone(&self) -> Self {
        Self {
            query: self.query.clone(),
            relation_selections: self.relation_selections.clone(),
            relation_filters: self.relation_filters.clone(),
            child_enhancements: self.child_enhancements.clone(),
            query_options: self.query_options.clone(),
            marker: PhantomData,
        }
    }
}

impl<R> EntryRequest<R> {
    pub(crate) fn new() -> Self {
        Self {
            query: SelectQuery::new("Entry")
                .project("id")
                .project("version"),
            relation_selections: Vec::new(),
            relation_filters: Vec::new(),
            child_enhancements: Vec::new(),
            query_options: QueryOptions::default(),
            marker: PhantomData,
        }
    }

    pub fn return_type<T>(self) -> EntryRequest<T> {
        EntryRequest {
            query: self.query,
            relation_selections: self.relation_selections,
            relation_filters: self.relation_filters,
            child_enhancements: self.child_enhancements,
            query_options: self.query_options,
            marker: PhantomData,
        }
    }

    pub fn query(&self) -> &SelectQuery {
        &self.query
    }

    pub fn relation_selections(&self) -> &[RelationSelection] {
        &self.relation_selections
    }

    pub fn relation_filters(&self) -> &[RelationFilter] {
        &self.relation_filters
    }

    pub fn child_enhancements(&self) -> &[QuerySelection] {
        &self.child_enhancements
    }

    pub fn query_options(&self) -> &QueryOptions {
        &self.query_options
    }

    pub fn into_query(self) -> SelectQuery {
        self.query
    }


    pub fn purpose(self, purpose: impl Into<String>) -> crate::PurposedQuery<Self> {
        crate::PurposedQuery::new(self, purpose)
    }

    pub(crate) async fn _execute_for_list<'a, C>(
        self,
        context: &'a C,
    ) -> Result<SmartList<R>, RuntimeError>
    where
        C: TeaqlRuntime + ?Sized,
        R: teaql_core::Entity + Send + 'a,
    {
        let query_options = self.query_options.clone();
        let relation_aggregates = runtime_relation_aggregates(&query_options);
        let query = authorize_query(apply_runtime_metadata(
            self.query,
            &query_options,
            &self.child_enhancements,
        ))?;
        let (mut rows, facets) = if query_options.facets.is_empty() {
            let rows = context.fetch_entity_smart_list::<R>(
                "Entry",
                query,
                relation_aggregates,
            ).await?;
            (rows, std::collections::BTreeMap::new())
        } else {
            let rows = context.fetch_entity_smart_list::<R>(
                "Entry",
                query.clone(),
                relation_aggregates,
            ).await?;
            let facets = execute_facets(context, query.as_query(), &query_options)
                .await?;
            (rows, facets)
        };
        attach_facets(&mut rows, facets);
        Ok(rows)
    }

    pub(crate) async fn _execute_for_rows<'a, C>(
        self,
        context: &'a C,
    ) -> Result<SmartList<teaql_core::CompactRow>, RuntimeError>
    where
        C: TeaqlRuntime + ?Sized,
    {
        let query = authorize_query(apply_runtime_metadata(
            self.query,
            &self.query_options,
            &self.child_enhancements,
        ))?;
        context.fetch_compact_smart_list("Entry", &query).await
    }

    pub(crate) async fn _execute_for_stream<'a, C>(
        self,
        context: &'a C,
    ) -> Result<TeaqlEntityStream<'a, R, RuntimeError>, RuntimeError>
    where
        C: TeaqlRuntime + ?Sized,
        R: teaql_core::Entity + Send + 'a,
    {
        let query = authorize_query(apply_runtime_metadata(
            self.query,
            &self.query_options,
            &self.child_enhancements,
        ))?;
        Ok(context.fetch_entity_stream("Entry", query))
    }

    pub(crate) async fn _execute_for_first<'a, C>(
        self,
        context: &'a C,
    ) -> Result<Option<R>, RuntimeError>
    where
        C: TeaqlRuntime + ?Sized,
        R: teaql_core::Entity + Send + 'a,
    {
        let rows = self.limit(1)._execute_for_list(context).await?;
        Ok(rows.into_iter().next())
    }

    pub(crate) async fn _execute_for_one<'a, C>(
        self,
        context: &'a C,
    ) -> Result<Option<R>, RuntimeError>
    where
        C: TeaqlRuntime + ?Sized,
        R: teaql_core::Entity + Send + 'a,
    {
        self._execute_for_first(context).await
    }


    pub(crate) async fn _execute_for_page<'a, C>(
        self,
        context: &'a C,
        offset: u64,
        limit: u64,
    ) -> Result<SmartList<R>, RuntimeError>
    where
        C: TeaqlRuntime + ?Sized,
        R: teaql_core::Entity + Send + 'a,
    {
        if self.query.id_set_pagination.is_some() {
            let mut rows = self
                .clone()
                .page_offset(offset, limit)
                ._execute_for_list(context)
                .await?;
            if rows.total_count.is_none() {
                rows.total_count = Some(self._execute_for_count(context).await?);
            }
            return Ok(rows);
        }
        let total_count = self.clone()._execute_for_count(context).await?;
        let mut rows = self.page_offset(offset, limit)._execute_for_list(context).await?;
        rows.total_count = Some(total_count);
        Ok(rows)
    }

    pub(crate) async fn _execute_for_count<'a, C>(
        self,
        context: &'a C,
    ) -> Result<u64, RuntimeError>
    where
        C: TeaqlRuntime + ?Sized,
    {
        let query_options = self.query_options.clone();
        let mut query = apply_runtime_metadata(
            self.query,
            &query_options,
            &self.child_enhancements,
        );
        query.projection.clear();
        query.expr_projection.clear();
        query.order_by.clear();
        query.slice = None;
        query.relations.clear();
        query = query.count(COUNT_ALIAS);
        let query = authorize_query(query)?;
        let rows = context.fetch_compact_rows("Entry", &query).await?;
        rows.first()
            .and_then(|row| row.get(COUNT_ALIAS))
            .and_then(teaql_core::Value::try_u64)
            .ok_or_else(|| RuntimeError::Graph(format!("count result for Entry is missing or not numeric")))
    }

    pub(crate) async fn _execute_for_exists<'a, C>(
        self,
        context: &'a C,
    ) -> Result<bool, RuntimeError>
    where
        C: TeaqlRuntime + ?Sized,
    {
        let mut query = apply_runtime_metadata(
            self.query,
            &self.query_options,
            &self.child_enhancements,
        ).limit(1);
        query.relations.clear();
        let query = authorize_query(query)?;
        let rows = context.fetch_compact_rows("Entry", &query).await?;
        Ok(!rows.is_empty())
    }

    pub fn search_with_text(mut self, text: impl Into<String>) -> Self {
        self.query = self.query.search_with_text(text);
        self
    }

    pub fn filter(mut self, filter: Expr) -> Self {
        self.query = self.query.filter(filter);
        self
    }

    pub fn and_filter(mut self, filter: Expr) -> Self {
        self.query = self.query.and_filter(filter);
        self
    }

    pub fn or_filter(mut self, filter: Expr) -> Self {
        self.query = self.query.or_filter(filter);
        self
    }

    pub fn append_search_criteria(self, criteria: Expr) -> Self {
        self.and_filter(criteria)
    }

    pub fn filter_property(
        mut self,
        property1: impl AsRef<str>,
        operator: FieldOperator,
        property2: impl AsRef<str>,
    ) -> Self {
        self.query = self.query.and_filter(field_operator_column_expr(
            property1.as_ref(),
            operator,
            property2.as_ref(),
        ));
        self
    }

    pub fn with_deleted_rows(mut self) -> Self {
        self.query.filter = remove_default_live_filter(self.query.filter);
        self
    }

    pub fn deleted_rows_only(mut self) -> Self {
        self.query.filter = remove_default_live_filter(self.query.filter);
        self.query = self.query.and_filter(Expr::lte("version", 0_i64));
        self
    }

    pub fn match_types(
        mut self,
        types: impl IntoIterator<Item = impl Into<teaql_core::Value>>,
    ) -> Self {
        self.query = self.query.and_filter(Expr::in_list(TYPE_FIELD, types.into_iter().map(Into::into)));
        self
    }


    pub fn with_type_group(mut self) -> Self {
        self.query = self.query.project(TYPE_GROUP_FIELD);
        self
    }

    pub fn matching_any_of(mut self, request: impl Into<QuerySelection>) -> Self {
        let selection = request.into();
        let entity = EntityDescriptor::new(selection.query.entity.clone());
        self.query = self.query.and_filter(Expr::in_subquery("id", entity, selection.query.clone(), "id"));
        self
    }

    pub fn match_any_of(self, request: impl Into<QuerySelection>) -> Self {
        self.matching_any_of(request)
    }

    pub fn enhance_child(mut self, request: impl Into<QuerySelection>) -> Self {
        self.child_enhancements.push(request.into());
        self
    }

    pub fn enhance_children_if_needed(self) -> Self {
        let request = self;
        request
    }


    pub fn comment(mut self, comment: impl Into<String>) -> Self {
        self.query_options.comment = Some(comment.into());
        self
    }

    pub fn raw_sql(self, raw_sql: impl Into<String>) -> Self {
        self.unsafe_raw_sql(UnsafeRawSqlSegment::trusted(raw_sql))
    }

    pub fn unsafe_raw_sql(mut self, raw_sql: UnsafeRawSqlSegment) -> Self {
        self.query_options.raw_sql = Some(raw_sql.into_sql());
        self
    }

    pub fn raw_sql_filter(self, raw_sql: impl Into<String>) -> Self {
        self.unsafe_raw_sql_filter(UnsafeRawSqlSegment::trusted(raw_sql))
    }

    pub fn unsafe_raw_sql_filter(mut self, raw_sql: UnsafeRawSqlSegment) -> Self {
        self.query_options.raw_sql_search_criteria.push(raw_sql.into_sql());
        self
    }
    pub fn filter_with_json(self, json_expr: impl Into<String>) -> Self {
        self.merge_dynamic_json_expr(json_expr.into())
    }

    fn merge_dynamic_json_expr(self, json_expr: String) -> Self {
        let json = serde_json::from_str::<JsonValue>(&json_expr)
            .unwrap_or_else(|_| panic!("Input JSON format error: {json_expr}"));
        self.merge_dynamic_json(&json)
    }

    fn merge_dynamic_json(mut self, json: &JsonValue) -> Self {
        let Some(object) = json.as_object() else {
            return self;
        };

        for (field, value) in object {
            if field.starts_with('_') {
                continue;
            }
            self = self.apply_dynamic_json_filter(field, value);
        }

        self = self.apply_dynamic_json_order_by(object.get("_orderBy"));

        if let Some(offset) = dynamic_json_u64_field(object, "_start") {
            self = self.skip(offset);
        }
        if let Some(size) = dynamic_json_u64_field(object, "_size") {
            self = self.limit(size);
        }

        if let Some(page_size) = dynamic_json_u64_field(object, "_pageSize") {
            self = self.limit(page_size);
        }
        if let Some(page_number) = dynamic_json_u64_field(object, "_page") {
            if page_number > 0 {
                let size = dynamic_json_u64_field(object, "_pageSize")
                    .or_else(|| self.query.slice.as_ref().and_then(|slice| slice.limit))
                    .unwrap_or(10);
                let offset = page_number.saturating_sub(1).saturating_mul(size);
                self = self.page_offset(offset, size);
            }
        }

        self
    }

    pub(crate) fn apply_dynamic_json_filter(self, field: &str, value: &JsonValue) -> Self {
        if let Some((head, tail)) = field.split_once('.') {
            self.apply_dynamic_json_chain_filter(head, tail, value)
        } else if let Some(storage_field) = Self::dynamic_json_self_field(field) {
            self.and_filter(dynamic_json_filter_expr(storage_field, value))
        } else {
            self
        }
    }

    fn apply_dynamic_json_order_by(mut self, order_by: Option<&JsonValue>) -> Self {
        match order_by {
            Some(JsonValue::String(field)) => {
                if let Some(storage_field) = Self::dynamic_json_self_field(field) {
                    self.query = self.query.order_desc(storage_field);
                }
            }
            Some(JsonValue::Object(order_by)) => {
                self = self.apply_dynamic_json_single_order_by(order_by);
            }
            Some(JsonValue::Array(order_bys)) => {
                for order_by in order_bys {
                    if let Some(order_by) = order_by.as_object() {
                        self = self.apply_dynamic_json_single_order_by(order_by);
                    }
                }
            }
            _ => {}
        }
        self
    }

    fn apply_dynamic_json_single_order_by(
        mut self,
        order_by: &serde_json::Map<String, JsonValue>,
    ) -> Self {
        let Some(field) = order_by.get("field").and_then(JsonValue::as_str) else {
            return self;
        };
        let Some(storage_field) = Self::dynamic_json_self_field(field) else {
            return self;
        };
        if order_by
            .get("useAsc")
            .and_then(JsonValue::as_bool)
            .unwrap_or(false)
        {
            self.query = self.query.order_asc(storage_field);
        } else {
            self.query = self.query.order_desc(storage_field);
        }
        self
    }

    fn dynamic_json_self_field(field: &str) -> Option<&'static str> {
        match field {
            "id" => Some("id"),
            "label" => Some("label"),
            "optional_label" => Some("optional_label"),
            "notes" => Some("notes"),
            "secret" => Some("secret"),
            "active" => Some("active"),
            "optional_active" => Some("optional_active"),
            "amount" => Some("amount"),
            "optional_amount" => Some("optional_amount"),
            "height" => Some("height"),
            "ratio" => Some("ratio"),
            "counter" => Some("counter"),
            "day" => Some("day"),
            "optional_day" => Some("optional_day"),
            "moment" => Some("moment"),
            "optional_moment" => Some("optional_moment"),
            "version" => Some("version"),
            "tenant" | "tenant_id" => Some("tenant_id"),
            _ => None,
        }
    }

    fn apply_dynamic_json_chain_filter(self, head: &str, tail: &str, value: &JsonValue) -> Self {
        let _ = (tail, value);
        match head {
            "tenant" => {
                self.with_tenant_matching(
                    crate::Q::tenants_minimal()
                        .apply_dynamic_json_filter(tail, value),
                )
            }
            _ => self,
        }
    }

    pub fn create_property_as(
        self,
        property_name: impl Into<String>,
        raw_sql_segment: impl Into<String>,
    ) -> Self {
        self.unsafe_create_property_as(property_name, UnsafeRawSqlSegment::trusted(raw_sql_segment))
    }

    pub fn unsafe_create_property_as(
        mut self,
        property_name: impl Into<String>,
        raw_sql_segment: UnsafeRawSqlSegment,
    ) -> Self {
        self.query_options
            .dynamic_properties
            .push(RawDynamicProperty::new(property_name, raw_sql_segment));
        self
    }

    pub fn limit(mut self, limit: u64) -> Self {
        self.query = self.query.limit(limit);
        self
    }

    pub fn stream(mut self, chunk_size: usize) -> Self {
        assert!(chunk_size > 0, "stream chunk size must be positive");
        self.query = self.query.stream(chunk_size);
        self
    }

    pub fn stream_default(mut self) -> Self {
        self.query = self.query.stream_default();
        self
    }

    pub fn skip(mut self, offset: u64) -> Self {
        self.query = self.query.offset(offset);
        self
    }

    pub fn offset_only(self, offset: u64) -> Self {
        self.skip(offset)
    }

    pub fn offset(self, offset: u64, size: u64) -> Self {
        self.page_offset(offset, size)
    }

    pub fn page_offset(mut self, offset: u64, limit: u64) -> Self {
        self.query = self.query.page(offset, limit);
        self
    }

    pub fn optimize_for_continuous_page_fetch(mut self) -> Self {
        self.query = self.query.optimize_for_continuous_page_fetch();
        self
    }

    pub fn optimize_for_continuous_page_fetch_with(
        mut self,
        namespace: impl Into<String>,
        ttl_seconds: u64,
    ) -> Self {
        self.query = self
            .query
            .optimize_for_continuous_page_fetch_with(namespace, ttl_seconds);
        self
    }

    pub fn optimize_pagination_with_id_set(mut self) -> Self {
        self.query = self.query.optimize_pagination_with_id_set();
        self
    }

    pub fn optimize_pagination_with_id_set_config(
        mut self,
        namespace: impl Into<String>,
        ttl_seconds: u64,
        max_ids: u64,
    ) -> Self {
        self.query = self
            .query
            .optimize_pagination_with_id_set_config(namespace, ttl_seconds, max_ids);
        self
    }

    /// Select bounded indexed probes for a per-parent Top-N relation only
    /// when the already-loaded parent count is at or below `threshold`.
    /// Passing zero explicitly selects the provider window plan.
    pub fn top_n_probe_parent_threshold(mut self, threshold: usize) -> Self {
        self.query = self.query.top_n_probe_parent_threshold(threshold);
        self
    }

    pub fn top(self, top_n: u64) -> Self {
        self.limit(top_n)
    }

    pub fn offset_size(self, offset: u64, size: u64) -> Self {
        self.offset(offset, size)
    }

    pub fn unlimited(mut self) -> Self {
        self.query.slice = None;
        self
    }

    pub fn page_number(self, page_number: u64, page_size: u64) -> Self {
        let offset = page_number.saturating_sub(1).saturating_mul(page_size);
        self.page_offset(offset, page_size)
    }

    pub fn page_number_default(self, page_number: u64) -> Self {
        self.page_number(page_number, 10)
    }

    pub fn page(self, page_number: u64, page_size: u64) -> Self {
        self.page_number(page_number, page_size)
    }

    pub fn page_default(self, page_number: u64) -> Self {
        self.page_number_default(page_number)
    }

    pub fn select_self(mut self) -> Self {
        self.query = self.query.project("id");
        self.query = self.query.project("label");
        self.query = self.query.project("optional_label");
        self.query = self.query.project("notes");
        self.query = self.query.project("secret");
        self.query = self.query.project("active");
        self.query = self.query.project("optional_active");
        self.query = self.query.project("amount");
        self.query = self.query.project("optional_amount");
        self.query = self.query.project("height");
        self.query = self.query.project("ratio");
        self.query = self.query.project("counter");
        self.query = self.query.project("day");
        self.query = self.query.project("optional_day");
        self.query = self.query.project("moment");
        self.query = self.query.project("optional_moment");
        self.query = self.query.project("version");
        self.query = self.query.project("tenant_id");
        self
    }

    pub fn select_dynamic_fields_with(mut self, selection: teaql_core::dynamic_fields::DynamicFieldSelection) -> Self {
        self.query = self.query.select_dynamic_fields(selection);
        self
    }

    pub fn select_self_fields(self) -> Self {
        self.select_self()
    }

    pub fn select_self_without_parent(self) -> Self {
        self.select_self_fields()
    }

    pub fn select_all(self) -> Self {
        let mut request = self.select_self();
        request = request.select_tenant();
        request
    }

    pub fn select_children(self) -> Self {
        self.select_all()
    }

    pub fn select_any(self) -> Self {
        self.select_children()
    }

    pub fn group_by(mut self, field: impl Into<String>) -> Self {
        self.query = self.query.group_by(field);
        self
    }

    pub fn count(self) -> Self {
        self.count_as("count")
    }

    pub fn count_as(self, alias: impl Into<String>) -> Self {
        self.aggregate_count(alias)
    }

    pub fn aggregate_count(mut self, alias: impl Into<String>) -> Self {
        self.query = self.query.count(alias);
        self
    }

    pub fn aggregate_count_field(mut self, field: impl Into<String>, alias: impl Into<String>) -> Self {
        self.query = self.query.count_field(field, alias);
        self
    }

    pub fn aggregate_with_function(
        mut self,
        field: impl Into<String>,
        alias: impl Into<String>,
        function: AggregateFunction,
    ) -> Self {
        self.query = self.query.aggregate(Aggregate::new(function, field, alias));
        self
    }

    pub fn aggregate_sum(mut self, field: impl Into<String>, alias: impl Into<String>) -> Self {
        self.query = self.query.sum(field, alias);
        self
    }

    pub fn aggregate_avg(mut self, field: impl Into<String>, alias: impl Into<String>) -> Self {
        self.query = self.query.avg(field, alias);
        self
    }

    pub fn aggregate_min(mut self, field: impl Into<String>, alias: impl Into<String>) -> Self {
        self.query = self.query.min(field, alias);
        self
    }

    pub fn aggregate_max(mut self, field: impl Into<String>, alias: impl Into<String>) -> Self {
        self.query = self.query.max(field, alias);
        self
    }

    pub fn aggregate_stddev(mut self, field: impl Into<String>, alias: impl Into<String>) -> Self {
        self.query = self.query.stddev(field, alias);
        self
    }

    pub fn aggregate_stddev_pop(mut self, field: impl Into<String>, alias: impl Into<String>) -> Self {
        self.query = self.query.stddev_pop(field, alias);
        self
    }

    pub fn aggregate_var_samp(mut self, field: impl Into<String>, alias: impl Into<String>) -> Self {
        self.query = self.query.var_samp(field, alias);
        self
    }

    pub fn aggregate_var_pop(mut self, field: impl Into<String>, alias: impl Into<String>) -> Self {
        self.query = self.query.var_pop(field, alias);
        self
    }

    pub fn aggregate_bit_and(mut self, field: impl Into<String>, alias: impl Into<String>) -> Self {
        self.query = self.query.bit_and(field, alias);
        self
    }

    pub fn aggregate_bit_or(mut self, field: impl Into<String>, alias: impl Into<String>) -> Self {
        self.query = self.query.bit_or(field, alias);
        self
    }

    pub fn aggregate_bit_xor(mut self, field: impl Into<String>, alias: impl Into<String>) -> Self {
        self.query = self.query.bit_xor(field, alias);
        self
    }

    pub fn enable_aggregation_cache(mut self) -> Self {
        self.query = self.query.enable_aggregation_cache();
        self
    }

    pub fn enable_aggregation_cache_for(mut self, cache_expired_millis: u64) -> Self {
        self.query = self.query.enable_aggregation_cache_for(cache_expired_millis);
        self
    }

    pub fn propagate_aggregation_cache(mut self, cache_expired_millis: u64) -> Self {
        self.query = self.query.propagate_aggregation_cache(cache_expired_millis);
        self
    }

    pub fn group_by_id(self) -> Self {
        self.group_by("id")
    }

    pub fn group_by_id_as(self, alias: impl Into<String>) -> Self {
        let alias = alias.into();
        let mut request = self.group_by("id");
        request.query = request
            .query
            .project_expr(alias, Expr::column("id"));
        request
    }

    pub fn group_by_id_with_function(
        self,
        alias: impl Into<String>,
        function: AggregateFunction,
    ) -> Self {
        self.group_by("id")
            .aggregate_with_function("id", alias, function)
    }

    pub fn count_id(self) -> Self {
        self.count_id_as("id_count")
    }

    pub fn count_id_as(self, alias: impl Into<String>) -> Self {
        self.aggregate_count_field("id", alias)
    }

    pub fn sum_id(self) -> Self {
        self.sum_id_as("sum_id")
    }

    pub fn sum_id_as(self, alias: impl Into<String>) -> Self {
        self.aggregate_sum("id", alias)
    }

    pub fn avg_id(self) -> Self {
        self.avg_id_as("avg_id")
    }

    pub fn avg_id_as(self, alias: impl Into<String>) -> Self {
        self.aggregate_avg("id", alias)
    }

    pub fn min_id(self) -> Self {
        self.min_id_as("min_id")
    }

    pub fn min_id_as(self, alias: impl Into<String>) -> Self {
        self.aggregate_min("id", alias)
    }

    pub fn max_id(self) -> Self {
        self.max_id_as("max_id")
    }

    pub fn max_id_as(self, alias: impl Into<String>) -> Self {
        self.aggregate_max("id", alias)
    }


    pub fn with_id(
        mut self,
        operator: FieldOperator,
        values: impl IntoIterator<Item = impl Into<teaql_core::Value>>,
    ) -> Self {
        self.query = self.query.and_filter(field_operator_expr(
            "id",
            operator,
            values.into_iter().map(Into::into).collect(),
        ));
        self
    }

    pub fn create_id_criteria(
        operator: FieldOperator,
        values: impl IntoIterator<Item = impl Into<teaql_core::Value>>,
    ) -> Expr {
        field_operator_expr(
            "id",
            operator,
            values.into_iter().map(Into::into).collect(),
        )
    }

    pub fn with_id_is(mut self, value: impl Into<teaql_core::Value>) -> Self {
        self.query = self.query.and_filter(Expr::eq("id", value));
        self
    }



    pub fn with_id_is_not(mut self, value: impl Into<teaql_core::Value>) -> Self {
        self.query = self.query.and_filter(Expr::ne("id", value));
        self
    }

    pub fn with_id_in(
        mut self,
        values: impl IntoIterator<Item = impl Into<teaql_core::Value>>,
    ) -> Self {
        self.query = self.query.and_filter(Expr::in_list(
            "id",
            values.into_iter().map(Into::into),
        ));
        self
    }

    pub fn with_id_not_in(
        mut self,
        values: impl IntoIterator<Item = impl Into<teaql_core::Value>>,
    ) -> Self {
        self.query = self.query.and_filter(Expr::not_in_list(
            "id",
            values.into_iter().map(Into::into),
        ));
        self
    }

    pub fn order_by_id_asc(mut self) -> Self {
        self.query = self.query.order_asc("id");
        self
    }

    pub fn order_by_id_desc(mut self) -> Self {
        self.query = self.query.order_desc("id");
        self
    }

    pub fn order_by_id_asc_using_gbk(mut self) -> Self {
        self.query = self.query.order_gbk_asc("id");
        self
    }

    pub fn order_by_id_desc_using_gbk(mut self) -> Self {
        self.query = self.query.order_gbk_desc("id");
        self
    }


    pub fn select_label(mut self) -> Self {
        self.query = self.query.project("label");
        self
    }

    pub fn project_label(self) -> Self {
        self.select_label()
    }

    pub fn select_label_raw(self, raw_sql_segment: impl Into<String>) -> Self {
        self.select_label_unsafe_raw(UnsafeRawSqlSegment::trusted(raw_sql_segment))
    }

    pub fn select_label_unsafe_raw(mut self, raw_sql_segment: UnsafeRawSqlSegment) -> Self {
        self.query_options
            .raw_projections
            .push(RawProjection::new("label", raw_sql_segment));
        self
    }

    pub fn group_by_label(self) -> Self {
        self.group_by("label")
    }

    pub fn group_by_label_as(self, alias: impl Into<String>) -> Self {
        let alias = alias.into();
        let mut request = self.group_by("label");
        request.query = request
            .query
            .project_expr(alias, Expr::column("label"));
        request
    }

    pub fn group_by_label_with_function(
        self,
        alias: impl Into<String>,
        function: AggregateFunction,
    ) -> Self {
        self.group_by("label")
            .aggregate_with_function("label", alias, function)
    }

    pub fn count_label(self) -> Self {
        self.count_label_as("label_count")
    }

    pub fn count_label_as(self, alias: impl Into<String>) -> Self {
        self.aggregate_count_field("label", alias)
    }

    pub fn sum_label(self) -> Self {
        self.sum_label_as("sum_label")
    }

    pub fn sum_label_as(self, alias: impl Into<String>) -> Self {
        self.aggregate_sum("label", alias)
    }

    pub fn avg_label(self) -> Self {
        self.avg_label_as("avg_label")
    }

    pub fn avg_label_as(self, alias: impl Into<String>) -> Self {
        self.aggregate_avg("label", alias)
    }

    pub fn min_label(self) -> Self {
        self.min_label_as("min_label")
    }

    pub fn min_label_as(self, alias: impl Into<String>) -> Self {
        self.aggregate_min("label", alias)
    }

    pub fn max_label(self) -> Self {
        self.max_label_as("max_label")
    }

    pub fn max_label_as(self, alias: impl Into<String>) -> Self {
        self.aggregate_max("label", alias)
    }

    pub fn unselect_label(mut self) -> Self {
        self.query.projection.retain(|field| field != "label");
        self.query_options.raw_projections.retain(|projection| projection.property_name != "label");
        self
    }


    pub fn with_label(
        mut self,
        operator: FieldOperator,
        values: impl IntoIterator<Item = impl Into<teaql_core::Value>>,
    ) -> Self {
        self.query = self.query.and_filter(field_operator_expr(
            "label",
            operator,
            values.into_iter().map(Into::into).collect(),
        ));
        self
    }

    pub fn create_label_criteria(
        operator: FieldOperator,
        values: impl IntoIterator<Item = impl Into<teaql_core::Value>>,
    ) -> Expr {
        field_operator_expr(
            "label",
            operator,
            values.into_iter().map(Into::into).collect(),
        )
    }

    pub fn with_label_is(mut self, value: impl Into<teaql_core::Value>) -> Self {
        self.query = self.query.and_filter(Expr::eq("label", value));
        self
    }



    pub fn with_label_is_not(mut self, value: impl Into<teaql_core::Value>) -> Self {
        self.query = self.query.and_filter(Expr::ne("label", value));
        self
    }

    pub fn with_label_greater_than(mut self, value: impl Into<teaql_core::Value>) -> Self {
        self.query = self.query.and_filter(Expr::gt("label", value));
        self
    }

    pub fn with_label_greater_than_or_equal_to(mut self, value: impl Into<teaql_core::Value>) -> Self {
        self.query = self.query.and_filter(Expr::gte("label", value));
        self
    }

    pub fn with_label_less_than(mut self, value: impl Into<teaql_core::Value>) -> Self {
        self.query = self.query.and_filter(Expr::lt("label", value));
        self
    }

    pub fn with_label_less_than_or_equal_to(mut self, value: impl Into<teaql_core::Value>) -> Self {
        self.query = self.query.and_filter(Expr::lte("label", value));
        self
    }

    pub fn with_label_between(
        mut self,
        lower: impl Into<teaql_core::Value>,
        upper: impl Into<teaql_core::Value>,
    ) -> Self {
        self.query = self.query.and_filter(Expr::between("label", lower, upper));
        self
    }

    pub fn with_label_between_range<T>(mut self, range: DateRange<T>) -> Self
    where
        T: Into<teaql_core::Value>,
    {
        self.query = self.query.and_filter(Expr::between(
            "label",
            range.start,
            range.end,
        ));
        self
    }

    pub fn with_label_in(
        mut self,
        values: impl IntoIterator<Item = impl Into<teaql_core::Value>>,
    ) -> Self {
        self.query = self.query.and_filter(Expr::in_list(
            "label",
            values.into_iter().map(Into::into),
        ));
        self
    }

    pub fn with_label_not_in(
        mut self,
        values: impl IntoIterator<Item = impl Into<teaql_core::Value>>,
    ) -> Self {
        self.query = self.query.and_filter(Expr::not_in_list(
            "label",
            values.into_iter().map(Into::into),
        ));
        self
    }

    pub fn with_label_containing(mut self, value: impl Into<String>) -> Self {
        self.query = self.query.and_filter(Expr::contain("label", value));
        self
    }

    pub fn with_label_not_containing(mut self, value: impl Into<String>) -> Self {
        self.query = self.query.and_filter(Expr::not_contain("label", value));
        self
    }

    pub fn with_label_starting_with(mut self, value: impl Into<String>) -> Self {
        self.query = self.query.and_filter(Expr::begin_with("label", value));
        self
    }

    pub fn with_label_not_starting_with(mut self, value: impl Into<String>) -> Self {
        self.query = self.query.and_filter(Expr::not_begin_with("label", value));
        self
    }

    pub fn with_label_ending_with(mut self, value: impl Into<String>) -> Self {
        self.query = self.query.and_filter(Expr::end_with("label", value));
        self
    }

    pub fn with_label_not_ending_with(mut self, value: impl Into<String>) -> Self {
        self.query = self.query.and_filter(Expr::not_end_with("label", value));
        self
    }

    pub fn with_label_sounding_like(mut self, value: impl Into<teaql_core::Value>) -> Self {
        self.query = self.query.and_filter(Expr::sound_like("label", value));
        self
    }
    pub fn with_label_before(mut self, value: impl Into<teaql_core::Value>) -> Self {
        self.query = self.query.and_filter(Expr::lt("label", value));
        self
    }

    pub fn with_label_after(mut self, value: impl Into<teaql_core::Value>) -> Self {
        self.query = self.query.and_filter(Expr::gt("label", value));
        self
    }

    pub fn with_label_is_unknown(mut self) -> Self {
        self.query = self.query.and_filter(Expr::is_null("label"));
        self
    }



    pub fn with_label_is_known(mut self) -> Self {
        self.query = self.query.and_filter(Expr::is_not_null("label"));
        self
    }


    pub fn order_by_label_asc(mut self) -> Self {
        self.query = self.query.order_asc("label");
        self
    }

    pub fn order_by_label_desc(mut self) -> Self {
        self.query = self.query.order_desc("label");
        self
    }

    pub fn order_by_label_asc_using_gbk(mut self) -> Self {
        self.query = self.query.order_gbk_asc("label");
        self
    }

    pub fn order_by_label_desc_using_gbk(mut self) -> Self {
        self.query = self.query.order_gbk_desc("label");
        self
    }


    pub fn select_optional_label(mut self) -> Self {
        self.query = self.query.project("optional_label");
        self
    }

    pub fn project_optional_label(self) -> Self {
        self.select_optional_label()
    }

    pub fn select_optional_label_raw(self, raw_sql_segment: impl Into<String>) -> Self {
        self.select_optional_label_unsafe_raw(UnsafeRawSqlSegment::trusted(raw_sql_segment))
    }

    pub fn select_optional_label_unsafe_raw(mut self, raw_sql_segment: UnsafeRawSqlSegment) -> Self {
        self.query_options
            .raw_projections
            .push(RawProjection::new("optional_label", raw_sql_segment));
        self
    }

    pub fn group_by_optional_label(self) -> Self {
        self.group_by("optional_label")
    }

    pub fn group_by_optional_label_as(self, alias: impl Into<String>) -> Self {
        let alias = alias.into();
        let mut request = self.group_by("optional_label");
        request.query = request
            .query
            .project_expr(alias, Expr::column("optional_label"));
        request
    }

    pub fn group_by_optional_label_with_function(
        self,
        alias: impl Into<String>,
        function: AggregateFunction,
    ) -> Self {
        self.group_by("optional_label")
            .aggregate_with_function("optional_label", alias, function)
    }

    pub fn count_optional_label(self) -> Self {
        self.count_optional_label_as("optional_label_count")
    }

    pub fn count_optional_label_as(self, alias: impl Into<String>) -> Self {
        self.aggregate_count_field("optional_label", alias)
    }

    pub fn sum_optional_label(self) -> Self {
        self.sum_optional_label_as("sum_optional_label")
    }

    pub fn sum_optional_label_as(self, alias: impl Into<String>) -> Self {
        self.aggregate_sum("optional_label", alias)
    }

    pub fn avg_optional_label(self) -> Self {
        self.avg_optional_label_as("avg_optional_label")
    }

    pub fn avg_optional_label_as(self, alias: impl Into<String>) -> Self {
        self.aggregate_avg("optional_label", alias)
    }

    pub fn min_optional_label(self) -> Self {
        self.min_optional_label_as("min_optional_label")
    }

    pub fn min_optional_label_as(self, alias: impl Into<String>) -> Self {
        self.aggregate_min("optional_label", alias)
    }

    pub fn max_optional_label(self) -> Self {
        self.max_optional_label_as("max_optional_label")
    }

    pub fn max_optional_label_as(self, alias: impl Into<String>) -> Self {
        self.aggregate_max("optional_label", alias)
    }

    pub fn unselect_optional_label(mut self) -> Self {
        self.query.projection.retain(|field| field != "optional_label");
        self.query_options.raw_projections.retain(|projection| projection.property_name != "optional_label");
        self
    }


    pub fn with_optional_label(
        mut self,
        operator: FieldOperator,
        values: impl IntoIterator<Item = impl Into<teaql_core::Value>>,
    ) -> Self {
        self.query = self.query.and_filter(field_operator_expr(
            "optional_label",
            operator,
            values.into_iter().map(Into::into).collect(),
        ));
        self
    }

    pub fn create_optional_label_criteria(
        operator: FieldOperator,
        values: impl IntoIterator<Item = impl Into<teaql_core::Value>>,
    ) -> Expr {
        field_operator_expr(
            "optional_label",
            operator,
            values.into_iter().map(Into::into).collect(),
        )
    }

    pub fn with_optional_label_is(mut self, value: impl Into<teaql_core::Value>) -> Self {
        self.query = self.query.and_filter(Expr::eq("optional_label", value));
        self
    }



    pub fn with_optional_label_is_not(mut self, value: impl Into<teaql_core::Value>) -> Self {
        self.query = self.query.and_filter(Expr::ne("optional_label", value));
        self
    }

    pub fn with_optional_label_greater_than(mut self, value: impl Into<teaql_core::Value>) -> Self {
        self.query = self.query.and_filter(Expr::gt("optional_label", value));
        self
    }

    pub fn with_optional_label_greater_than_or_equal_to(mut self, value: impl Into<teaql_core::Value>) -> Self {
        self.query = self.query.and_filter(Expr::gte("optional_label", value));
        self
    }

    pub fn with_optional_label_less_than(mut self, value: impl Into<teaql_core::Value>) -> Self {
        self.query = self.query.and_filter(Expr::lt("optional_label", value));
        self
    }

    pub fn with_optional_label_less_than_or_equal_to(mut self, value: impl Into<teaql_core::Value>) -> Self {
        self.query = self.query.and_filter(Expr::lte("optional_label", value));
        self
    }

    pub fn with_optional_label_between(
        mut self,
        lower: impl Into<teaql_core::Value>,
        upper: impl Into<teaql_core::Value>,
    ) -> Self {
        self.query = self.query.and_filter(Expr::between("optional_label", lower, upper));
        self
    }

    pub fn with_optional_label_between_range<T>(mut self, range: DateRange<T>) -> Self
    where
        T: Into<teaql_core::Value>,
    {
        self.query = self.query.and_filter(Expr::between(
            "optional_label",
            range.start,
            range.end,
        ));
        self
    }

    pub fn with_optional_label_in(
        mut self,
        values: impl IntoIterator<Item = impl Into<teaql_core::Value>>,
    ) -> Self {
        self.query = self.query.and_filter(Expr::in_list(
            "optional_label",
            values.into_iter().map(Into::into),
        ));
        self
    }

    pub fn with_optional_label_not_in(
        mut self,
        values: impl IntoIterator<Item = impl Into<teaql_core::Value>>,
    ) -> Self {
        self.query = self.query.and_filter(Expr::not_in_list(
            "optional_label",
            values.into_iter().map(Into::into),
        ));
        self
    }

    pub fn with_optional_label_containing(mut self, value: impl Into<String>) -> Self {
        self.query = self.query.and_filter(Expr::contain("optional_label", value));
        self
    }

    pub fn with_optional_label_not_containing(mut self, value: impl Into<String>) -> Self {
        self.query = self.query.and_filter(Expr::not_contain("optional_label", value));
        self
    }

    pub fn with_optional_label_starting_with(mut self, value: impl Into<String>) -> Self {
        self.query = self.query.and_filter(Expr::begin_with("optional_label", value));
        self
    }

    pub fn with_optional_label_not_starting_with(mut self, value: impl Into<String>) -> Self {
        self.query = self.query.and_filter(Expr::not_begin_with("optional_label", value));
        self
    }

    pub fn with_optional_label_ending_with(mut self, value: impl Into<String>) -> Self {
        self.query = self.query.and_filter(Expr::end_with("optional_label", value));
        self
    }

    pub fn with_optional_label_not_ending_with(mut self, value: impl Into<String>) -> Self {
        self.query = self.query.and_filter(Expr::not_end_with("optional_label", value));
        self
    }

    pub fn with_optional_label_sounding_like(mut self, value: impl Into<teaql_core::Value>) -> Self {
        self.query = self.query.and_filter(Expr::sound_like("optional_label", value));
        self
    }
    pub fn with_optional_label_before(mut self, value: impl Into<teaql_core::Value>) -> Self {
        self.query = self.query.and_filter(Expr::lt("optional_label", value));
        self
    }

    pub fn with_optional_label_after(mut self, value: impl Into<teaql_core::Value>) -> Self {
        self.query = self.query.and_filter(Expr::gt("optional_label", value));
        self
    }

    pub fn with_optional_label_is_unknown(mut self) -> Self {
        self.query = self.query.and_filter(Expr::is_null("optional_label"));
        self
    }



    pub fn with_optional_label_is_known(mut self) -> Self {
        self.query = self.query.and_filter(Expr::is_not_null("optional_label"));
        self
    }


    pub fn order_by_optional_label_asc(mut self) -> Self {
        self.query = self.query.order_asc("optional_label");
        self
    }

    pub fn order_by_optional_label_desc(mut self) -> Self {
        self.query = self.query.order_desc("optional_label");
        self
    }

    pub fn order_by_optional_label_asc_using_gbk(mut self) -> Self {
        self.query = self.query.order_gbk_asc("optional_label");
        self
    }

    pub fn order_by_optional_label_desc_using_gbk(mut self) -> Self {
        self.query = self.query.order_gbk_desc("optional_label");
        self
    }


    pub fn select_notes(mut self) -> Self {
        self.query = self.query.project("notes");
        self
    }

    pub fn project_notes(self) -> Self {
        self.select_notes()
    }

    pub fn select_notes_raw(self, raw_sql_segment: impl Into<String>) -> Self {
        self.select_notes_unsafe_raw(UnsafeRawSqlSegment::trusted(raw_sql_segment))
    }

    pub fn select_notes_unsafe_raw(mut self, raw_sql_segment: UnsafeRawSqlSegment) -> Self {
        self.query_options
            .raw_projections
            .push(RawProjection::new("notes", raw_sql_segment));
        self
    }

    pub fn group_by_notes(self) -> Self {
        self.group_by("notes")
    }

    pub fn group_by_notes_as(self, alias: impl Into<String>) -> Self {
        let alias = alias.into();
        let mut request = self.group_by("notes");
        request.query = request
            .query
            .project_expr(alias, Expr::column("notes"));
        request
    }

    pub fn group_by_notes_with_function(
        self,
        alias: impl Into<String>,
        function: AggregateFunction,
    ) -> Self {
        self.group_by("notes")
            .aggregate_with_function("notes", alias, function)
    }

    pub fn count_notes(self) -> Self {
        self.count_notes_as("notes_count")
    }

    pub fn count_notes_as(self, alias: impl Into<String>) -> Self {
        self.aggregate_count_field("notes", alias)
    }

    pub fn sum_notes(self) -> Self {
        self.sum_notes_as("sum_notes")
    }

    pub fn sum_notes_as(self, alias: impl Into<String>) -> Self {
        self.aggregate_sum("notes", alias)
    }

    pub fn avg_notes(self) -> Self {
        self.avg_notes_as("avg_notes")
    }

    pub fn avg_notes_as(self, alias: impl Into<String>) -> Self {
        self.aggregate_avg("notes", alias)
    }

    pub fn min_notes(self) -> Self {
        self.min_notes_as("min_notes")
    }

    pub fn min_notes_as(self, alias: impl Into<String>) -> Self {
        self.aggregate_min("notes", alias)
    }

    pub fn max_notes(self) -> Self {
        self.max_notes_as("max_notes")
    }

    pub fn max_notes_as(self, alias: impl Into<String>) -> Self {
        self.aggregate_max("notes", alias)
    }

    pub fn unselect_notes(mut self) -> Self {
        self.query.projection.retain(|field| field != "notes");
        self.query_options.raw_projections.retain(|projection| projection.property_name != "notes");
        self
    }


    pub fn with_notes(
        mut self,
        operator: FieldOperator,
        values: impl IntoIterator<Item = impl Into<teaql_core::Value>>,
    ) -> Self {
        self.query = self.query.and_filter(field_operator_expr(
            "notes",
            operator,
            values.into_iter().map(Into::into).collect(),
        ));
        self
    }

    pub fn create_notes_criteria(
        operator: FieldOperator,
        values: impl IntoIterator<Item = impl Into<teaql_core::Value>>,
    ) -> Expr {
        field_operator_expr(
            "notes",
            operator,
            values.into_iter().map(Into::into).collect(),
        )
    }

    pub fn with_notes_is(mut self, value: impl Into<teaql_core::Value>) -> Self {
        self.query = self.query.and_filter(Expr::eq("notes", value));
        self
    }



    pub fn with_notes_is_not(mut self, value: impl Into<teaql_core::Value>) -> Self {
        self.query = self.query.and_filter(Expr::ne("notes", value));
        self
    }

    pub fn with_notes_greater_than(mut self, value: impl Into<teaql_core::Value>) -> Self {
        self.query = self.query.and_filter(Expr::gt("notes", value));
        self
    }

    pub fn with_notes_greater_than_or_equal_to(mut self, value: impl Into<teaql_core::Value>) -> Self {
        self.query = self.query.and_filter(Expr::gte("notes", value));
        self
    }

    pub fn with_notes_less_than(mut self, value: impl Into<teaql_core::Value>) -> Self {
        self.query = self.query.and_filter(Expr::lt("notes", value));
        self
    }

    pub fn with_notes_less_than_or_equal_to(mut self, value: impl Into<teaql_core::Value>) -> Self {
        self.query = self.query.and_filter(Expr::lte("notes", value));
        self
    }

    pub fn with_notes_between(
        mut self,
        lower: impl Into<teaql_core::Value>,
        upper: impl Into<teaql_core::Value>,
    ) -> Self {
        self.query = self.query.and_filter(Expr::between("notes", lower, upper));
        self
    }

    pub fn with_notes_between_range<T>(mut self, range: DateRange<T>) -> Self
    where
        T: Into<teaql_core::Value>,
    {
        self.query = self.query.and_filter(Expr::between(
            "notes",
            range.start,
            range.end,
        ));
        self
    }

    pub fn with_notes_in(
        mut self,
        values: impl IntoIterator<Item = impl Into<teaql_core::Value>>,
    ) -> Self {
        self.query = self.query.and_filter(Expr::in_list(
            "notes",
            values.into_iter().map(Into::into),
        ));
        self
    }

    pub fn with_notes_not_in(
        mut self,
        values: impl IntoIterator<Item = impl Into<teaql_core::Value>>,
    ) -> Self {
        self.query = self.query.and_filter(Expr::not_in_list(
            "notes",
            values.into_iter().map(Into::into),
        ));
        self
    }

    pub fn with_notes_before(mut self, value: impl Into<teaql_core::Value>) -> Self {
        self.query = self.query.and_filter(Expr::lt("notes", value));
        self
    }

    pub fn with_notes_after(mut self, value: impl Into<teaql_core::Value>) -> Self {
        self.query = self.query.and_filter(Expr::gt("notes", value));
        self
    }

    pub fn with_notes_is_unknown(mut self) -> Self {
        self.query = self.query.and_filter(Expr::is_null("notes"));
        self
    }



    pub fn with_notes_is_known(mut self) -> Self {
        self.query = self.query.and_filter(Expr::is_not_null("notes"));
        self
    }


    pub fn order_by_notes_asc(mut self) -> Self {
        self.query = self.query.order_asc("notes");
        self
    }

    pub fn order_by_notes_desc(mut self) -> Self {
        self.query = self.query.order_desc("notes");
        self
    }

    pub fn order_by_notes_asc_using_gbk(mut self) -> Self {
        self.query = self.query.order_gbk_asc("notes");
        self
    }

    pub fn order_by_notes_desc_using_gbk(mut self) -> Self {
        self.query = self.query.order_gbk_desc("notes");
        self
    }


    pub fn select_secret(mut self) -> Self {
        self.query = self.query.project("secret");
        self
    }

    pub fn project_secret(self) -> Self {
        self.select_secret()
    }

    pub fn select_secret_raw(self, raw_sql_segment: impl Into<String>) -> Self {
        self.select_secret_unsafe_raw(UnsafeRawSqlSegment::trusted(raw_sql_segment))
    }

    pub fn select_secret_unsafe_raw(mut self, raw_sql_segment: UnsafeRawSqlSegment) -> Self {
        self.query_options
            .raw_projections
            .push(RawProjection::new("secret", raw_sql_segment));
        self
    }

    pub fn group_by_secret(self) -> Self {
        self.group_by("secret")
    }

    pub fn group_by_secret_as(self, alias: impl Into<String>) -> Self {
        let alias = alias.into();
        let mut request = self.group_by("secret");
        request.query = request
            .query
            .project_expr(alias, Expr::column("secret"));
        request
    }

    pub fn group_by_secret_with_function(
        self,
        alias: impl Into<String>,
        function: AggregateFunction,
    ) -> Self {
        self.group_by("secret")
            .aggregate_with_function("secret", alias, function)
    }

    pub fn count_secret(self) -> Self {
        self.count_secret_as("secret_count")
    }

    pub fn count_secret_as(self, alias: impl Into<String>) -> Self {
        self.aggregate_count_field("secret", alias)
    }

    pub fn sum_secret(self) -> Self {
        self.sum_secret_as("sum_secret")
    }

    pub fn sum_secret_as(self, alias: impl Into<String>) -> Self {
        self.aggregate_sum("secret", alias)
    }

    pub fn avg_secret(self) -> Self {
        self.avg_secret_as("avg_secret")
    }

    pub fn avg_secret_as(self, alias: impl Into<String>) -> Self {
        self.aggregate_avg("secret", alias)
    }

    pub fn min_secret(self) -> Self {
        self.min_secret_as("min_secret")
    }

    pub fn min_secret_as(self, alias: impl Into<String>) -> Self {
        self.aggregate_min("secret", alias)
    }

    pub fn max_secret(self) -> Self {
        self.max_secret_as("max_secret")
    }

    pub fn max_secret_as(self, alias: impl Into<String>) -> Self {
        self.aggregate_max("secret", alias)
    }

    pub fn unselect_secret(mut self) -> Self {
        self.query.projection.retain(|field| field != "secret");
        self.query_options.raw_projections.retain(|projection| projection.property_name != "secret");
        self
    }


    pub fn with_secret(
        mut self,
        operator: FieldOperator,
        values: impl IntoIterator<Item = impl Into<teaql_core::Value>>,
    ) -> Self {
        self.query = self.query.and_filter(field_operator_expr(
            "secret",
            operator,
            values.into_iter().map(Into::into).collect(),
        ));
        self
    }

    pub fn create_secret_criteria(
        operator: FieldOperator,
        values: impl IntoIterator<Item = impl Into<teaql_core::Value>>,
    ) -> Expr {
        field_operator_expr(
            "secret",
            operator,
            values.into_iter().map(Into::into).collect(),
        )
    }

    pub fn with_secret_is(mut self, value: impl Into<teaql_core::Value>) -> Self {
        self.query = self.query.and_filter(Expr::eq("secret", value));
        self
    }



    pub fn with_secret_is_not(mut self, value: impl Into<teaql_core::Value>) -> Self {
        self.query = self.query.and_filter(Expr::ne("secret", value));
        self
    }

    pub fn with_secret_greater_than(mut self, value: impl Into<teaql_core::Value>) -> Self {
        self.query = self.query.and_filter(Expr::gt("secret", value));
        self
    }

    pub fn with_secret_greater_than_or_equal_to(mut self, value: impl Into<teaql_core::Value>) -> Self {
        self.query = self.query.and_filter(Expr::gte("secret", value));
        self
    }

    pub fn with_secret_less_than(mut self, value: impl Into<teaql_core::Value>) -> Self {
        self.query = self.query.and_filter(Expr::lt("secret", value));
        self
    }

    pub fn with_secret_less_than_or_equal_to(mut self, value: impl Into<teaql_core::Value>) -> Self {
        self.query = self.query.and_filter(Expr::lte("secret", value));
        self
    }

    pub fn with_secret_between(
        mut self,
        lower: impl Into<teaql_core::Value>,
        upper: impl Into<teaql_core::Value>,
    ) -> Self {
        self.query = self.query.and_filter(Expr::between("secret", lower, upper));
        self
    }

    pub fn with_secret_between_range<T>(mut self, range: DateRange<T>) -> Self
    where
        T: Into<teaql_core::Value>,
    {
        self.query = self.query.and_filter(Expr::between(
            "secret",
            range.start,
            range.end,
        ));
        self
    }

    pub fn with_secret_in(
        mut self,
        values: impl IntoIterator<Item = impl Into<teaql_core::Value>>,
    ) -> Self {
        self.query = self.query.and_filter(Expr::in_list(
            "secret",
            values.into_iter().map(Into::into),
        ));
        self
    }

    pub fn with_secret_not_in(
        mut self,
        values: impl IntoIterator<Item = impl Into<teaql_core::Value>>,
    ) -> Self {
        self.query = self.query.and_filter(Expr::not_in_list(
            "secret",
            values.into_iter().map(Into::into),
        ));
        self
    }

    pub fn with_secret_before(mut self, value: impl Into<teaql_core::Value>) -> Self {
        self.query = self.query.and_filter(Expr::lt("secret", value));
        self
    }

    pub fn with_secret_after(mut self, value: impl Into<teaql_core::Value>) -> Self {
        self.query = self.query.and_filter(Expr::gt("secret", value));
        self
    }

    pub fn with_secret_is_unknown(mut self) -> Self {
        self.query = self.query.and_filter(Expr::is_null("secret"));
        self
    }



    pub fn with_secret_is_known(mut self) -> Self {
        self.query = self.query.and_filter(Expr::is_not_null("secret"));
        self
    }


    pub fn order_by_secret_asc(mut self) -> Self {
        self.query = self.query.order_asc("secret");
        self
    }

    pub fn order_by_secret_desc(mut self) -> Self {
        self.query = self.query.order_desc("secret");
        self
    }

    pub fn order_by_secret_asc_using_gbk(mut self) -> Self {
        self.query = self.query.order_gbk_asc("secret");
        self
    }

    pub fn order_by_secret_desc_using_gbk(mut self) -> Self {
        self.query = self.query.order_gbk_desc("secret");
        self
    }


    pub fn select_active(mut self) -> Self {
        self.query = self.query.project("active");
        self
    }

    pub fn project_active(self) -> Self {
        self.select_active()
    }

    pub fn select_active_raw(self, raw_sql_segment: impl Into<String>) -> Self {
        self.select_active_unsafe_raw(UnsafeRawSqlSegment::trusted(raw_sql_segment))
    }

    pub fn select_active_unsafe_raw(mut self, raw_sql_segment: UnsafeRawSqlSegment) -> Self {
        self.query_options
            .raw_projections
            .push(RawProjection::new("active", raw_sql_segment));
        self
    }

    pub fn group_by_active(self) -> Self {
        self.group_by("active")
    }

    pub fn group_by_active_as(self, alias: impl Into<String>) -> Self {
        let alias = alias.into();
        let mut request = self.group_by("active");
        request.query = request
            .query
            .project_expr(alias, Expr::column("active"));
        request
    }

    pub fn group_by_active_with_function(
        self,
        alias: impl Into<String>,
        function: AggregateFunction,
    ) -> Self {
        self.group_by("active")
            .aggregate_with_function("active", alias, function)
    }

    pub fn count_active(self) -> Self {
        self.count_active_as("active_count")
    }

    pub fn count_active_as(self, alias: impl Into<String>) -> Self {
        self.aggregate_count_field("active", alias)
    }

    pub fn sum_active(self) -> Self {
        self.sum_active_as("sum_active")
    }

    pub fn sum_active_as(self, alias: impl Into<String>) -> Self {
        self.aggregate_sum("active", alias)
    }

    pub fn avg_active(self) -> Self {
        self.avg_active_as("avg_active")
    }

    pub fn avg_active_as(self, alias: impl Into<String>) -> Self {
        self.aggregate_avg("active", alias)
    }

    pub fn min_active(self) -> Self {
        self.min_active_as("min_active")
    }

    pub fn min_active_as(self, alias: impl Into<String>) -> Self {
        self.aggregate_min("active", alias)
    }

    pub fn max_active(self) -> Self {
        self.max_active_as("max_active")
    }

    pub fn max_active_as(self, alias: impl Into<String>) -> Self {
        self.aggregate_max("active", alias)
    }

    pub fn unselect_active(mut self) -> Self {
        self.query.projection.retain(|field| field != "active");
        self.query_options.raw_projections.retain(|projection| projection.property_name != "active");
        self
    }

    pub fn which_are_active(mut self) -> Self {
        self.query = self.query.and_filter(Expr::eq("active", true));
        self
    }

    pub fn which_are_not_active(mut self) -> Self {
        self.query = self.query.and_filter(Expr::eq("active", false));
        self
    }

    pub fn with_active_is_unknown(mut self) -> Self {
        self.query = self.query.and_filter(Expr::is_null("active"));
        self
    }

    pub fn with_active_is_known(mut self) -> Self {
        self.query = self.query.and_filter(Expr::is_not_null("active"));
        self
    }
    pub fn order_by_active_asc(mut self) -> Self {
        self.query = self.query.order_asc("active");
        self
    }

    pub fn order_by_active_desc(mut self) -> Self {
        self.query = self.query.order_desc("active");
        self
    }

    pub fn order_by_active_asc_using_gbk(mut self) -> Self {
        self.query = self.query.order_gbk_asc("active");
        self
    }

    pub fn order_by_active_desc_using_gbk(mut self) -> Self {
        self.query = self.query.order_gbk_desc("active");
        self
    }


    pub fn select_optional_active(mut self) -> Self {
        self.query = self.query.project("optional_active");
        self
    }

    pub fn project_optional_active(self) -> Self {
        self.select_optional_active()
    }

    pub fn select_optional_active_raw(self, raw_sql_segment: impl Into<String>) -> Self {
        self.select_optional_active_unsafe_raw(UnsafeRawSqlSegment::trusted(raw_sql_segment))
    }

    pub fn select_optional_active_unsafe_raw(mut self, raw_sql_segment: UnsafeRawSqlSegment) -> Self {
        self.query_options
            .raw_projections
            .push(RawProjection::new("optional_active", raw_sql_segment));
        self
    }

    pub fn group_by_optional_active(self) -> Self {
        self.group_by("optional_active")
    }

    pub fn group_by_optional_active_as(self, alias: impl Into<String>) -> Self {
        let alias = alias.into();
        let mut request = self.group_by("optional_active");
        request.query = request
            .query
            .project_expr(alias, Expr::column("optional_active"));
        request
    }

    pub fn group_by_optional_active_with_function(
        self,
        alias: impl Into<String>,
        function: AggregateFunction,
    ) -> Self {
        self.group_by("optional_active")
            .aggregate_with_function("optional_active", alias, function)
    }

    pub fn count_optional_active(self) -> Self {
        self.count_optional_active_as("optional_active_count")
    }

    pub fn count_optional_active_as(self, alias: impl Into<String>) -> Self {
        self.aggregate_count_field("optional_active", alias)
    }

    pub fn sum_optional_active(self) -> Self {
        self.sum_optional_active_as("sum_optional_active")
    }

    pub fn sum_optional_active_as(self, alias: impl Into<String>) -> Self {
        self.aggregate_sum("optional_active", alias)
    }

    pub fn avg_optional_active(self) -> Self {
        self.avg_optional_active_as("avg_optional_active")
    }

    pub fn avg_optional_active_as(self, alias: impl Into<String>) -> Self {
        self.aggregate_avg("optional_active", alias)
    }

    pub fn min_optional_active(self) -> Self {
        self.min_optional_active_as("min_optional_active")
    }

    pub fn min_optional_active_as(self, alias: impl Into<String>) -> Self {
        self.aggregate_min("optional_active", alias)
    }

    pub fn max_optional_active(self) -> Self {
        self.max_optional_active_as("max_optional_active")
    }

    pub fn max_optional_active_as(self, alias: impl Into<String>) -> Self {
        self.aggregate_max("optional_active", alias)
    }

    pub fn unselect_optional_active(mut self) -> Self {
        self.query.projection.retain(|field| field != "optional_active");
        self.query_options.raw_projections.retain(|projection| projection.property_name != "optional_active");
        self
    }

    pub fn which_are_optional_active(mut self) -> Self {
        self.query = self.query.and_filter(Expr::eq("optional_active", true));
        self
    }

    pub fn which_are_not_optional_active(mut self) -> Self {
        self.query = self.query.and_filter(Expr::eq("optional_active", false));
        self
    }

    pub fn with_optional_active_is_unknown(mut self) -> Self {
        self.query = self.query.and_filter(Expr::is_null("optional_active"));
        self
    }

    pub fn with_optional_active_is_known(mut self) -> Self {
        self.query = self.query.and_filter(Expr::is_not_null("optional_active"));
        self
    }
    pub fn order_by_optional_active_asc(mut self) -> Self {
        self.query = self.query.order_asc("optional_active");
        self
    }

    pub fn order_by_optional_active_desc(mut self) -> Self {
        self.query = self.query.order_desc("optional_active");
        self
    }

    pub fn order_by_optional_active_asc_using_gbk(mut self) -> Self {
        self.query = self.query.order_gbk_asc("optional_active");
        self
    }

    pub fn order_by_optional_active_desc_using_gbk(mut self) -> Self {
        self.query = self.query.order_gbk_desc("optional_active");
        self
    }


    pub fn select_amount(mut self) -> Self {
        self.query = self.query.project("amount");
        self
    }

    pub fn project_amount(self) -> Self {
        self.select_amount()
    }

    pub fn select_amount_raw(self, raw_sql_segment: impl Into<String>) -> Self {
        self.select_amount_unsafe_raw(UnsafeRawSqlSegment::trusted(raw_sql_segment))
    }

    pub fn select_amount_unsafe_raw(mut self, raw_sql_segment: UnsafeRawSqlSegment) -> Self {
        self.query_options
            .raw_projections
            .push(RawProjection::new("amount", raw_sql_segment));
        self
    }

    pub fn select_amount_with_function(self, function: AggregateFunction) -> Self {
        self.select_amount_as_with_function("amount", function)
    }

    pub fn select_amount_as_with_function(
        self,
        alias: impl Into<String>,
        function: AggregateFunction,
    ) -> Self {
        self.aggregate_with_function("amount", alias, function)
    }

    pub fn group_by_amount(self) -> Self {
        self.group_by("amount")
    }

    pub fn group_by_amount_as(self, alias: impl Into<String>) -> Self {
        let alias = alias.into();
        let mut request = self.group_by("amount");
        request.query = request
            .query
            .project_expr(alias, Expr::column("amount"));
        request
    }

    pub fn group_by_amount_with_function(
        self,
        alias: impl Into<String>,
        function: AggregateFunction,
    ) -> Self {
        self.group_by("amount")
            .aggregate_with_function("amount", alias, function)
    }

    pub fn count_amount(self) -> Self {
        self.count_amount_as("amount_count")
    }

    pub fn count_amount_as(self, alias: impl Into<String>) -> Self {
        self.aggregate_count_field("amount", alias)
    }

    pub fn sum_amount(self) -> Self {
        self.sum_amount_as("sum_amount")
    }

    pub fn sum_amount_as(self, alias: impl Into<String>) -> Self {
        self.aggregate_sum("amount", alias)
    }

    pub fn avg_amount(self) -> Self {
        self.avg_amount_as("avg_amount")
    }

    pub fn avg_amount_as(self, alias: impl Into<String>) -> Self {
        self.aggregate_avg("amount", alias)
    }

    pub fn min_amount(self) -> Self {
        self.min_amount_as("min_amount")
    }

    pub fn min_amount_as(self, alias: impl Into<String>) -> Self {
        self.aggregate_min("amount", alias)
    }

    pub fn max_amount(self) -> Self {
        self.max_amount_as("max_amount")
    }

    pub fn max_amount_as(self, alias: impl Into<String>) -> Self {
        self.aggregate_max("amount", alias)
    }

    pub fn standard_deviation_amount(self) -> Self {
        self.standard_deviation_amount_as("stdDev_amount")
    }

    pub fn standard_deviation_amount_as(self, alias: impl Into<String>) -> Self {
        self.aggregate_stddev("amount", alias)
    }

    pub fn square_root_of_population_standard_deviation_amount(self) -> Self {
        self.square_root_of_population_standard_deviation_amount_as("stdDevPop_amount")
    }

    pub fn square_root_of_population_standard_deviation_amount_as(self, alias: impl Into<String>) -> Self {
        self.aggregate_stddev_pop("amount", alias)
    }

    pub fn sample_variance_amount(self) -> Self {
        self.sample_variance_amount_as("varSamp_amount")
    }

    pub fn sample_variance_amount_as(self, alias: impl Into<String>) -> Self {
        self.aggregate_var_samp("amount", alias)
    }

    pub fn sample_population_variance_amount(self) -> Self {
        self.sample_population_variance_amount_as("varPop_amount")
    }

    pub fn sample_population_variance_amount_as(self, alias: impl Into<String>) -> Self {
        self.aggregate_var_pop("amount", alias)
    }

    pub fn unselect_amount(mut self) -> Self {
        self.query.projection.retain(|field| field != "amount");
        self.query_options.raw_projections.retain(|projection| projection.property_name != "amount");
        self
    }


    pub fn with_amount(
        mut self,
        operator: FieldOperator,
        values: impl IntoIterator<Item = impl Into<teaql_core::Value>>,
    ) -> Self {
        self.query = self.query.and_filter(field_operator_expr(
            "amount",
            operator,
            values.into_iter().map(Into::into).collect(),
        ));
        self
    }

    pub fn create_amount_criteria(
        operator: FieldOperator,
        values: impl IntoIterator<Item = impl Into<teaql_core::Value>>,
    ) -> Expr {
        field_operator_expr(
            "amount",
            operator,
            values.into_iter().map(Into::into).collect(),
        )
    }

    pub fn with_amount_is(mut self, value: impl Into<teaql_core::Value>) -> Self {
        self.query = self.query.and_filter(Expr::eq("amount", value));
        self
    }



    pub fn with_amount_is_not(mut self, value: impl Into<teaql_core::Value>) -> Self {
        self.query = self.query.and_filter(Expr::ne("amount", value));
        self
    }

    pub fn with_amount_greater_than(mut self, value: impl Into<teaql_core::Value>) -> Self {
        self.query = self.query.and_filter(Expr::gt("amount", value));
        self
    }

    pub fn with_amount_greater_than_or_equal_to(mut self, value: impl Into<teaql_core::Value>) -> Self {
        self.query = self.query.and_filter(Expr::gte("amount", value));
        self
    }

    pub fn with_amount_less_than(mut self, value: impl Into<teaql_core::Value>) -> Self {
        self.query = self.query.and_filter(Expr::lt("amount", value));
        self
    }

    pub fn with_amount_less_than_or_equal_to(mut self, value: impl Into<teaql_core::Value>) -> Self {
        self.query = self.query.and_filter(Expr::lte("amount", value));
        self
    }

    pub fn with_amount_between(
        mut self,
        lower: impl Into<teaql_core::Value>,
        upper: impl Into<teaql_core::Value>,
    ) -> Self {
        self.query = self.query.and_filter(Expr::between("amount", lower, upper));
        self
    }

    pub fn with_amount_between_range<T>(mut self, range: DateRange<T>) -> Self
    where
        T: Into<teaql_core::Value>,
    {
        self.query = self.query.and_filter(Expr::between(
            "amount",
            range.start,
            range.end,
        ));
        self
    }

    pub fn with_amount_in(
        mut self,
        values: impl IntoIterator<Item = impl Into<teaql_core::Value>>,
    ) -> Self {
        self.query = self.query.and_filter(Expr::in_list(
            "amount",
            values.into_iter().map(Into::into),
        ));
        self
    }

    pub fn with_amount_not_in(
        mut self,
        values: impl IntoIterator<Item = impl Into<teaql_core::Value>>,
    ) -> Self {
        self.query = self.query.and_filter(Expr::not_in_list(
            "amount",
            values.into_iter().map(Into::into),
        ));
        self
    }

    pub fn with_amount_before(mut self, value: impl Into<teaql_core::Value>) -> Self {
        self.query = self.query.and_filter(Expr::lt("amount", value));
        self
    }

    pub fn with_amount_after(mut self, value: impl Into<teaql_core::Value>) -> Self {
        self.query = self.query.and_filter(Expr::gt("amount", value));
        self
    }

    pub fn with_amount_is_unknown(mut self) -> Self {
        self.query = self.query.and_filter(Expr::is_null("amount"));
        self
    }



    pub fn with_amount_is_known(mut self) -> Self {
        self.query = self.query.and_filter(Expr::is_not_null("amount"));
        self
    }


    pub fn order_by_amount_asc(mut self) -> Self {
        self.query = self.query.order_asc("amount");
        self
    }

    pub fn order_by_amount_desc(mut self) -> Self {
        self.query = self.query.order_desc("amount");
        self
    }

    pub fn order_by_amount_asc_using_gbk(mut self) -> Self {
        self.query = self.query.order_gbk_asc("amount");
        self
    }

    pub fn order_by_amount_desc_using_gbk(mut self) -> Self {
        self.query = self.query.order_gbk_desc("amount");
        self
    }


    pub fn select_optional_amount(mut self) -> Self {
        self.query = self.query.project("optional_amount");
        self
    }

    pub fn project_optional_amount(self) -> Self {
        self.select_optional_amount()
    }

    pub fn select_optional_amount_raw(self, raw_sql_segment: impl Into<String>) -> Self {
        self.select_optional_amount_unsafe_raw(UnsafeRawSqlSegment::trusted(raw_sql_segment))
    }

    pub fn select_optional_amount_unsafe_raw(mut self, raw_sql_segment: UnsafeRawSqlSegment) -> Self {
        self.query_options
            .raw_projections
            .push(RawProjection::new("optional_amount", raw_sql_segment));
        self
    }

    pub fn select_optional_amount_with_function(self, function: AggregateFunction) -> Self {
        self.select_optional_amount_as_with_function("optional_amount", function)
    }

    pub fn select_optional_amount_as_with_function(
        self,
        alias: impl Into<String>,
        function: AggregateFunction,
    ) -> Self {
        self.aggregate_with_function("optional_amount", alias, function)
    }

    pub fn group_by_optional_amount(self) -> Self {
        self.group_by("optional_amount")
    }

    pub fn group_by_optional_amount_as(self, alias: impl Into<String>) -> Self {
        let alias = alias.into();
        let mut request = self.group_by("optional_amount");
        request.query = request
            .query
            .project_expr(alias, Expr::column("optional_amount"));
        request
    }

    pub fn group_by_optional_amount_with_function(
        self,
        alias: impl Into<String>,
        function: AggregateFunction,
    ) -> Self {
        self.group_by("optional_amount")
            .aggregate_with_function("optional_amount", alias, function)
    }

    pub fn count_optional_amount(self) -> Self {
        self.count_optional_amount_as("optional_amount_count")
    }

    pub fn count_optional_amount_as(self, alias: impl Into<String>) -> Self {
        self.aggregate_count_field("optional_amount", alias)
    }

    pub fn sum_optional_amount(self) -> Self {
        self.sum_optional_amount_as("sum_optional_amount")
    }

    pub fn sum_optional_amount_as(self, alias: impl Into<String>) -> Self {
        self.aggregate_sum("optional_amount", alias)
    }

    pub fn avg_optional_amount(self) -> Self {
        self.avg_optional_amount_as("avg_optional_amount")
    }

    pub fn avg_optional_amount_as(self, alias: impl Into<String>) -> Self {
        self.aggregate_avg("optional_amount", alias)
    }

    pub fn min_optional_amount(self) -> Self {
        self.min_optional_amount_as("min_optional_amount")
    }

    pub fn min_optional_amount_as(self, alias: impl Into<String>) -> Self {
        self.aggregate_min("optional_amount", alias)
    }

    pub fn max_optional_amount(self) -> Self {
        self.max_optional_amount_as("max_optional_amount")
    }

    pub fn max_optional_amount_as(self, alias: impl Into<String>) -> Self {
        self.aggregate_max("optional_amount", alias)
    }

    pub fn standard_deviation_optional_amount(self) -> Self {
        self.standard_deviation_optional_amount_as("stdDev_optional_amount")
    }

    pub fn standard_deviation_optional_amount_as(self, alias: impl Into<String>) -> Self {
        self.aggregate_stddev("optional_amount", alias)
    }

    pub fn square_root_of_population_standard_deviation_optional_amount(self) -> Self {
        self.square_root_of_population_standard_deviation_optional_amount_as("stdDevPop_optional_amount")
    }

    pub fn square_root_of_population_standard_deviation_optional_amount_as(self, alias: impl Into<String>) -> Self {
        self.aggregate_stddev_pop("optional_amount", alias)
    }

    pub fn sample_variance_optional_amount(self) -> Self {
        self.sample_variance_optional_amount_as("varSamp_optional_amount")
    }

    pub fn sample_variance_optional_amount_as(self, alias: impl Into<String>) -> Self {
        self.aggregate_var_samp("optional_amount", alias)
    }

    pub fn sample_population_variance_optional_amount(self) -> Self {
        self.sample_population_variance_optional_amount_as("varPop_optional_amount")
    }

    pub fn sample_population_variance_optional_amount_as(self, alias: impl Into<String>) -> Self {
        self.aggregate_var_pop("optional_amount", alias)
    }

    pub fn unselect_optional_amount(mut self) -> Self {
        self.query.projection.retain(|field| field != "optional_amount");
        self.query_options.raw_projections.retain(|projection| projection.property_name != "optional_amount");
        self
    }


    pub fn with_optional_amount(
        mut self,
        operator: FieldOperator,
        values: impl IntoIterator<Item = impl Into<teaql_core::Value>>,
    ) -> Self {
        self.query = self.query.and_filter(field_operator_expr(
            "optional_amount",
            operator,
            values.into_iter().map(Into::into).collect(),
        ));
        self
    }

    pub fn create_optional_amount_criteria(
        operator: FieldOperator,
        values: impl IntoIterator<Item = impl Into<teaql_core::Value>>,
    ) -> Expr {
        field_operator_expr(
            "optional_amount",
            operator,
            values.into_iter().map(Into::into).collect(),
        )
    }

    pub fn with_optional_amount_is(mut self, value: impl Into<teaql_core::Value>) -> Self {
        self.query = self.query.and_filter(Expr::eq("optional_amount", value));
        self
    }



    pub fn with_optional_amount_is_not(mut self, value: impl Into<teaql_core::Value>) -> Self {
        self.query = self.query.and_filter(Expr::ne("optional_amount", value));
        self
    }

    pub fn with_optional_amount_greater_than(mut self, value: impl Into<teaql_core::Value>) -> Self {
        self.query = self.query.and_filter(Expr::gt("optional_amount", value));
        self
    }

    pub fn with_optional_amount_greater_than_or_equal_to(mut self, value: impl Into<teaql_core::Value>) -> Self {
        self.query = self.query.and_filter(Expr::gte("optional_amount", value));
        self
    }

    pub fn with_optional_amount_less_than(mut self, value: impl Into<teaql_core::Value>) -> Self {
        self.query = self.query.and_filter(Expr::lt("optional_amount", value));
        self
    }

    pub fn with_optional_amount_less_than_or_equal_to(mut self, value: impl Into<teaql_core::Value>) -> Self {
        self.query = self.query.and_filter(Expr::lte("optional_amount", value));
        self
    }

    pub fn with_optional_amount_between(
        mut self,
        lower: impl Into<teaql_core::Value>,
        upper: impl Into<teaql_core::Value>,
    ) -> Self {
        self.query = self.query.and_filter(Expr::between("optional_amount", lower, upper));
        self
    }

    pub fn with_optional_amount_between_range<T>(mut self, range: DateRange<T>) -> Self
    where
        T: Into<teaql_core::Value>,
    {
        self.query = self.query.and_filter(Expr::between(
            "optional_amount",
            range.start,
            range.end,
        ));
        self
    }

    pub fn with_optional_amount_in(
        mut self,
        values: impl IntoIterator<Item = impl Into<teaql_core::Value>>,
    ) -> Self {
        self.query = self.query.and_filter(Expr::in_list(
            "optional_amount",
            values.into_iter().map(Into::into),
        ));
        self
    }

    pub fn with_optional_amount_not_in(
        mut self,
        values: impl IntoIterator<Item = impl Into<teaql_core::Value>>,
    ) -> Self {
        self.query = self.query.and_filter(Expr::not_in_list(
            "optional_amount",
            values.into_iter().map(Into::into),
        ));
        self
    }

    pub fn with_optional_amount_before(mut self, value: impl Into<teaql_core::Value>) -> Self {
        self.query = self.query.and_filter(Expr::lt("optional_amount", value));
        self
    }

    pub fn with_optional_amount_after(mut self, value: impl Into<teaql_core::Value>) -> Self {
        self.query = self.query.and_filter(Expr::gt("optional_amount", value));
        self
    }

    pub fn with_optional_amount_is_unknown(mut self) -> Self {
        self.query = self.query.and_filter(Expr::is_null("optional_amount"));
        self
    }



    pub fn with_optional_amount_is_known(mut self) -> Self {
        self.query = self.query.and_filter(Expr::is_not_null("optional_amount"));
        self
    }


    pub fn order_by_optional_amount_asc(mut self) -> Self {
        self.query = self.query.order_asc("optional_amount");
        self
    }

    pub fn order_by_optional_amount_desc(mut self) -> Self {
        self.query = self.query.order_desc("optional_amount");
        self
    }

    pub fn order_by_optional_amount_asc_using_gbk(mut self) -> Self {
        self.query = self.query.order_gbk_asc("optional_amount");
        self
    }

    pub fn order_by_optional_amount_desc_using_gbk(mut self) -> Self {
        self.query = self.query.order_gbk_desc("optional_amount");
        self
    }


    pub fn select_height(mut self) -> Self {
        self.query = self.query.project("height");
        self
    }

    pub fn project_height(self) -> Self {
        self.select_height()
    }

    pub fn select_height_raw(self, raw_sql_segment: impl Into<String>) -> Self {
        self.select_height_unsafe_raw(UnsafeRawSqlSegment::trusted(raw_sql_segment))
    }

    pub fn select_height_unsafe_raw(mut self, raw_sql_segment: UnsafeRawSqlSegment) -> Self {
        self.query_options
            .raw_projections
            .push(RawProjection::new("height", raw_sql_segment));
        self
    }

    pub fn select_height_with_function(self, function: AggregateFunction) -> Self {
        self.select_height_as_with_function("height", function)
    }

    pub fn select_height_as_with_function(
        self,
        alias: impl Into<String>,
        function: AggregateFunction,
    ) -> Self {
        self.aggregate_with_function("height", alias, function)
    }

    pub fn group_by_height(self) -> Self {
        self.group_by("height")
    }

    pub fn group_by_height_as(self, alias: impl Into<String>) -> Self {
        let alias = alias.into();
        let mut request = self.group_by("height");
        request.query = request
            .query
            .project_expr(alias, Expr::column("height"));
        request
    }

    pub fn group_by_height_with_function(
        self,
        alias: impl Into<String>,
        function: AggregateFunction,
    ) -> Self {
        self.group_by("height")
            .aggregate_with_function("height", alias, function)
    }

    pub fn count_height(self) -> Self {
        self.count_height_as("height_count")
    }

    pub fn count_height_as(self, alias: impl Into<String>) -> Self {
        self.aggregate_count_field("height", alias)
    }

    pub fn sum_height(self) -> Self {
        self.sum_height_as("sum_height")
    }

    pub fn sum_height_as(self, alias: impl Into<String>) -> Self {
        self.aggregate_sum("height", alias)
    }

    pub fn avg_height(self) -> Self {
        self.avg_height_as("avg_height")
    }

    pub fn avg_height_as(self, alias: impl Into<String>) -> Self {
        self.aggregate_avg("height", alias)
    }

    pub fn min_height(self) -> Self {
        self.min_height_as("min_height")
    }

    pub fn min_height_as(self, alias: impl Into<String>) -> Self {
        self.aggregate_min("height", alias)
    }

    pub fn max_height(self) -> Self {
        self.max_height_as("max_height")
    }

    pub fn max_height_as(self, alias: impl Into<String>) -> Self {
        self.aggregate_max("height", alias)
    }

    pub fn standard_deviation_height(self) -> Self {
        self.standard_deviation_height_as("stdDev_height")
    }

    pub fn standard_deviation_height_as(self, alias: impl Into<String>) -> Self {
        self.aggregate_stddev("height", alias)
    }

    pub fn square_root_of_population_standard_deviation_height(self) -> Self {
        self.square_root_of_population_standard_deviation_height_as("stdDevPop_height")
    }

    pub fn square_root_of_population_standard_deviation_height_as(self, alias: impl Into<String>) -> Self {
        self.aggregate_stddev_pop("height", alias)
    }

    pub fn sample_variance_height(self) -> Self {
        self.sample_variance_height_as("varSamp_height")
    }

    pub fn sample_variance_height_as(self, alias: impl Into<String>) -> Self {
        self.aggregate_var_samp("height", alias)
    }

    pub fn sample_population_variance_height(self) -> Self {
        self.sample_population_variance_height_as("varPop_height")
    }

    pub fn sample_population_variance_height_as(self, alias: impl Into<String>) -> Self {
        self.aggregate_var_pop("height", alias)
    }

    pub fn unselect_height(mut self) -> Self {
        self.query.projection.retain(|field| field != "height");
        self.query_options.raw_projections.retain(|projection| projection.property_name != "height");
        self
    }


    pub fn with_height(
        mut self,
        operator: FieldOperator,
        values: impl IntoIterator<Item = impl Into<teaql_core::Value>>,
    ) -> Self {
        self.query = self.query.and_filter(field_operator_expr(
            "height",
            operator,
            values.into_iter().map(Into::into).collect(),
        ));
        self
    }

    pub fn create_height_criteria(
        operator: FieldOperator,
        values: impl IntoIterator<Item = impl Into<teaql_core::Value>>,
    ) -> Expr {
        field_operator_expr(
            "height",
            operator,
            values.into_iter().map(Into::into).collect(),
        )
    }

    pub fn with_height_is(mut self, value: impl Into<teaql_core::Value>) -> Self {
        self.query = self.query.and_filter(Expr::eq("height", value));
        self
    }



    pub fn with_height_is_not(mut self, value: impl Into<teaql_core::Value>) -> Self {
        self.query = self.query.and_filter(Expr::ne("height", value));
        self
    }

    pub fn with_height_greater_than(mut self, value: impl Into<teaql_core::Value>) -> Self {
        self.query = self.query.and_filter(Expr::gt("height", value));
        self
    }

    pub fn with_height_greater_than_or_equal_to(mut self, value: impl Into<teaql_core::Value>) -> Self {
        self.query = self.query.and_filter(Expr::gte("height", value));
        self
    }

    pub fn with_height_less_than(mut self, value: impl Into<teaql_core::Value>) -> Self {
        self.query = self.query.and_filter(Expr::lt("height", value));
        self
    }

    pub fn with_height_less_than_or_equal_to(mut self, value: impl Into<teaql_core::Value>) -> Self {
        self.query = self.query.and_filter(Expr::lte("height", value));
        self
    }

    pub fn with_height_between(
        mut self,
        lower: impl Into<teaql_core::Value>,
        upper: impl Into<teaql_core::Value>,
    ) -> Self {
        self.query = self.query.and_filter(Expr::between("height", lower, upper));
        self
    }

    pub fn with_height_between_range<T>(mut self, range: DateRange<T>) -> Self
    where
        T: Into<teaql_core::Value>,
    {
        self.query = self.query.and_filter(Expr::between(
            "height",
            range.start,
            range.end,
        ));
        self
    }

    pub fn with_height_in(
        mut self,
        values: impl IntoIterator<Item = impl Into<teaql_core::Value>>,
    ) -> Self {
        self.query = self.query.and_filter(Expr::in_list(
            "height",
            values.into_iter().map(Into::into),
        ));
        self
    }

    pub fn with_height_not_in(
        mut self,
        values: impl IntoIterator<Item = impl Into<teaql_core::Value>>,
    ) -> Self {
        self.query = self.query.and_filter(Expr::not_in_list(
            "height",
            values.into_iter().map(Into::into),
        ));
        self
    }

    pub fn with_height_before(mut self, value: impl Into<teaql_core::Value>) -> Self {
        self.query = self.query.and_filter(Expr::lt("height", value));
        self
    }

    pub fn with_height_after(mut self, value: impl Into<teaql_core::Value>) -> Self {
        self.query = self.query.and_filter(Expr::gt("height", value));
        self
    }

    pub fn with_height_is_unknown(mut self) -> Self {
        self.query = self.query.and_filter(Expr::is_null("height"));
        self
    }



    pub fn with_height_is_known(mut self) -> Self {
        self.query = self.query.and_filter(Expr::is_not_null("height"));
        self
    }


    pub fn order_by_height_asc(mut self) -> Self {
        self.query = self.query.order_asc("height");
        self
    }

    pub fn order_by_height_desc(mut self) -> Self {
        self.query = self.query.order_desc("height");
        self
    }

    pub fn order_by_height_asc_using_gbk(mut self) -> Self {
        self.query = self.query.order_gbk_asc("height");
        self
    }

    pub fn order_by_height_desc_using_gbk(mut self) -> Self {
        self.query = self.query.order_gbk_desc("height");
        self
    }


    pub fn select_ratio(mut self) -> Self {
        self.query = self.query.project("ratio");
        self
    }

    pub fn project_ratio(self) -> Self {
        self.select_ratio()
    }

    pub fn select_ratio_raw(self, raw_sql_segment: impl Into<String>) -> Self {
        self.select_ratio_unsafe_raw(UnsafeRawSqlSegment::trusted(raw_sql_segment))
    }

    pub fn select_ratio_unsafe_raw(mut self, raw_sql_segment: UnsafeRawSqlSegment) -> Self {
        self.query_options
            .raw_projections
            .push(RawProjection::new("ratio", raw_sql_segment));
        self
    }

    pub fn select_ratio_with_function(self, function: AggregateFunction) -> Self {
        self.select_ratio_as_with_function("ratio", function)
    }

    pub fn select_ratio_as_with_function(
        self,
        alias: impl Into<String>,
        function: AggregateFunction,
    ) -> Self {
        self.aggregate_with_function("ratio", alias, function)
    }

    pub fn group_by_ratio(self) -> Self {
        self.group_by("ratio")
    }

    pub fn group_by_ratio_as(self, alias: impl Into<String>) -> Self {
        let alias = alias.into();
        let mut request = self.group_by("ratio");
        request.query = request
            .query
            .project_expr(alias, Expr::column("ratio"));
        request
    }

    pub fn group_by_ratio_with_function(
        self,
        alias: impl Into<String>,
        function: AggregateFunction,
    ) -> Self {
        self.group_by("ratio")
            .aggregate_with_function("ratio", alias, function)
    }

    pub fn count_ratio(self) -> Self {
        self.count_ratio_as("ratio_count")
    }

    pub fn count_ratio_as(self, alias: impl Into<String>) -> Self {
        self.aggregate_count_field("ratio", alias)
    }

    pub fn sum_ratio(self) -> Self {
        self.sum_ratio_as("sum_ratio")
    }

    pub fn sum_ratio_as(self, alias: impl Into<String>) -> Self {
        self.aggregate_sum("ratio", alias)
    }

    pub fn avg_ratio(self) -> Self {
        self.avg_ratio_as("avg_ratio")
    }

    pub fn avg_ratio_as(self, alias: impl Into<String>) -> Self {
        self.aggregate_avg("ratio", alias)
    }

    pub fn min_ratio(self) -> Self {
        self.min_ratio_as("min_ratio")
    }

    pub fn min_ratio_as(self, alias: impl Into<String>) -> Self {
        self.aggregate_min("ratio", alias)
    }

    pub fn max_ratio(self) -> Self {
        self.max_ratio_as("max_ratio")
    }

    pub fn max_ratio_as(self, alias: impl Into<String>) -> Self {
        self.aggregate_max("ratio", alias)
    }

    pub fn standard_deviation_ratio(self) -> Self {
        self.standard_deviation_ratio_as("stdDev_ratio")
    }

    pub fn standard_deviation_ratio_as(self, alias: impl Into<String>) -> Self {
        self.aggregate_stddev("ratio", alias)
    }

    pub fn square_root_of_population_standard_deviation_ratio(self) -> Self {
        self.square_root_of_population_standard_deviation_ratio_as("stdDevPop_ratio")
    }

    pub fn square_root_of_population_standard_deviation_ratio_as(self, alias: impl Into<String>) -> Self {
        self.aggregate_stddev_pop("ratio", alias)
    }

    pub fn sample_variance_ratio(self) -> Self {
        self.sample_variance_ratio_as("varSamp_ratio")
    }

    pub fn sample_variance_ratio_as(self, alias: impl Into<String>) -> Self {
        self.aggregate_var_samp("ratio", alias)
    }

    pub fn sample_population_variance_ratio(self) -> Self {
        self.sample_population_variance_ratio_as("varPop_ratio")
    }

    pub fn sample_population_variance_ratio_as(self, alias: impl Into<String>) -> Self {
        self.aggregate_var_pop("ratio", alias)
    }

    pub fn unselect_ratio(mut self) -> Self {
        self.query.projection.retain(|field| field != "ratio");
        self.query_options.raw_projections.retain(|projection| projection.property_name != "ratio");
        self
    }


    pub fn with_ratio(
        mut self,
        operator: FieldOperator,
        values: impl IntoIterator<Item = impl Into<teaql_core::Value>>,
    ) -> Self {
        self.query = self.query.and_filter(field_operator_expr(
            "ratio",
            operator,
            values.into_iter().map(Into::into).collect(),
        ));
        self
    }

    pub fn create_ratio_criteria(
        operator: FieldOperator,
        values: impl IntoIterator<Item = impl Into<teaql_core::Value>>,
    ) -> Expr {
        field_operator_expr(
            "ratio",
            operator,
            values.into_iter().map(Into::into).collect(),
        )
    }

    pub fn with_ratio_is(mut self, value: impl Into<teaql_core::Value>) -> Self {
        self.query = self.query.and_filter(Expr::eq("ratio", value));
        self
    }



    pub fn with_ratio_is_not(mut self, value: impl Into<teaql_core::Value>) -> Self {
        self.query = self.query.and_filter(Expr::ne("ratio", value));
        self
    }

    pub fn with_ratio_greater_than(mut self, value: impl Into<teaql_core::Value>) -> Self {
        self.query = self.query.and_filter(Expr::gt("ratio", value));
        self
    }

    pub fn with_ratio_greater_than_or_equal_to(mut self, value: impl Into<teaql_core::Value>) -> Self {
        self.query = self.query.and_filter(Expr::gte("ratio", value));
        self
    }

    pub fn with_ratio_less_than(mut self, value: impl Into<teaql_core::Value>) -> Self {
        self.query = self.query.and_filter(Expr::lt("ratio", value));
        self
    }

    pub fn with_ratio_less_than_or_equal_to(mut self, value: impl Into<teaql_core::Value>) -> Self {
        self.query = self.query.and_filter(Expr::lte("ratio", value));
        self
    }

    pub fn with_ratio_between(
        mut self,
        lower: impl Into<teaql_core::Value>,
        upper: impl Into<teaql_core::Value>,
    ) -> Self {
        self.query = self.query.and_filter(Expr::between("ratio", lower, upper));
        self
    }

    pub fn with_ratio_between_range<T>(mut self, range: DateRange<T>) -> Self
    where
        T: Into<teaql_core::Value>,
    {
        self.query = self.query.and_filter(Expr::between(
            "ratio",
            range.start,
            range.end,
        ));
        self
    }

    pub fn with_ratio_in(
        mut self,
        values: impl IntoIterator<Item = impl Into<teaql_core::Value>>,
    ) -> Self {
        self.query = self.query.and_filter(Expr::in_list(
            "ratio",
            values.into_iter().map(Into::into),
        ));
        self
    }

    pub fn with_ratio_not_in(
        mut self,
        values: impl IntoIterator<Item = impl Into<teaql_core::Value>>,
    ) -> Self {
        self.query = self.query.and_filter(Expr::not_in_list(
            "ratio",
            values.into_iter().map(Into::into),
        ));
        self
    }

    pub fn with_ratio_before(mut self, value: impl Into<teaql_core::Value>) -> Self {
        self.query = self.query.and_filter(Expr::lt("ratio", value));
        self
    }

    pub fn with_ratio_after(mut self, value: impl Into<teaql_core::Value>) -> Self {
        self.query = self.query.and_filter(Expr::gt("ratio", value));
        self
    }

    pub fn with_ratio_is_unknown(mut self) -> Self {
        self.query = self.query.and_filter(Expr::is_null("ratio"));
        self
    }



    pub fn with_ratio_is_known(mut self) -> Self {
        self.query = self.query.and_filter(Expr::is_not_null("ratio"));
        self
    }


    pub fn order_by_ratio_asc(mut self) -> Self {
        self.query = self.query.order_asc("ratio");
        self
    }

    pub fn order_by_ratio_desc(mut self) -> Self {
        self.query = self.query.order_desc("ratio");
        self
    }

    pub fn order_by_ratio_asc_using_gbk(mut self) -> Self {
        self.query = self.query.order_gbk_asc("ratio");
        self
    }

    pub fn order_by_ratio_desc_using_gbk(mut self) -> Self {
        self.query = self.query.order_gbk_desc("ratio");
        self
    }


    pub fn select_counter(mut self) -> Self {
        self.query = self.query.project("counter");
        self
    }

    pub fn project_counter(self) -> Self {
        self.select_counter()
    }

    pub fn select_counter_raw(self, raw_sql_segment: impl Into<String>) -> Self {
        self.select_counter_unsafe_raw(UnsafeRawSqlSegment::trusted(raw_sql_segment))
    }

    pub fn select_counter_unsafe_raw(mut self, raw_sql_segment: UnsafeRawSqlSegment) -> Self {
        self.query_options
            .raw_projections
            .push(RawProjection::new("counter", raw_sql_segment));
        self
    }

    pub fn select_counter_with_function(self, function: AggregateFunction) -> Self {
        self.select_counter_as_with_function("counter", function)
    }

    pub fn select_counter_as_with_function(
        self,
        alias: impl Into<String>,
        function: AggregateFunction,
    ) -> Self {
        self.aggregate_with_function("counter", alias, function)
    }

    pub fn group_by_counter(self) -> Self {
        self.group_by("counter")
    }

    pub fn group_by_counter_as(self, alias: impl Into<String>) -> Self {
        let alias = alias.into();
        let mut request = self.group_by("counter");
        request.query = request
            .query
            .project_expr(alias, Expr::column("counter"));
        request
    }

    pub fn group_by_counter_with_function(
        self,
        alias: impl Into<String>,
        function: AggregateFunction,
    ) -> Self {
        self.group_by("counter")
            .aggregate_with_function("counter", alias, function)
    }

    pub fn count_counter(self) -> Self {
        self.count_counter_as("counter_count")
    }

    pub fn count_counter_as(self, alias: impl Into<String>) -> Self {
        self.aggregate_count_field("counter", alias)
    }

    pub fn sum_counter(self) -> Self {
        self.sum_counter_as("sum_counter")
    }

    pub fn sum_counter_as(self, alias: impl Into<String>) -> Self {
        self.aggregate_sum("counter", alias)
    }

    pub fn avg_counter(self) -> Self {
        self.avg_counter_as("avg_counter")
    }

    pub fn avg_counter_as(self, alias: impl Into<String>) -> Self {
        self.aggregate_avg("counter", alias)
    }

    pub fn min_counter(self) -> Self {
        self.min_counter_as("min_counter")
    }

    pub fn min_counter_as(self, alias: impl Into<String>) -> Self {
        self.aggregate_min("counter", alias)
    }

    pub fn max_counter(self) -> Self {
        self.max_counter_as("max_counter")
    }

    pub fn max_counter_as(self, alias: impl Into<String>) -> Self {
        self.aggregate_max("counter", alias)
    }

    pub fn standard_deviation_counter(self) -> Self {
        self.standard_deviation_counter_as("stdDev_counter")
    }

    pub fn standard_deviation_counter_as(self, alias: impl Into<String>) -> Self {
        self.aggregate_stddev("counter", alias)
    }

    pub fn square_root_of_population_standard_deviation_counter(self) -> Self {
        self.square_root_of_population_standard_deviation_counter_as("stdDevPop_counter")
    }

    pub fn square_root_of_population_standard_deviation_counter_as(self, alias: impl Into<String>) -> Self {
        self.aggregate_stddev_pop("counter", alias)
    }

    pub fn sample_variance_counter(self) -> Self {
        self.sample_variance_counter_as("varSamp_counter")
    }

    pub fn sample_variance_counter_as(self, alias: impl Into<String>) -> Self {
        self.aggregate_var_samp("counter", alias)
    }

    pub fn sample_population_variance_counter(self) -> Self {
        self.sample_population_variance_counter_as("varPop_counter")
    }

    pub fn sample_population_variance_counter_as(self, alias: impl Into<String>) -> Self {
        self.aggregate_var_pop("counter", alias)
    }

    pub fn unselect_counter(mut self) -> Self {
        self.query.projection.retain(|field| field != "counter");
        self.query_options.raw_projections.retain(|projection| projection.property_name != "counter");
        self
    }


    pub fn with_counter(
        mut self,
        operator: FieldOperator,
        values: impl IntoIterator<Item = impl Into<teaql_core::Value>>,
    ) -> Self {
        self.query = self.query.and_filter(field_operator_expr(
            "counter",
            operator,
            values.into_iter().map(Into::into).collect(),
        ));
        self
    }

    pub fn create_counter_criteria(
        operator: FieldOperator,
        values: impl IntoIterator<Item = impl Into<teaql_core::Value>>,
    ) -> Expr {
        field_operator_expr(
            "counter",
            operator,
            values.into_iter().map(Into::into).collect(),
        )
    }

    pub fn with_counter_is(mut self, value: impl Into<teaql_core::Value>) -> Self {
        self.query = self.query.and_filter(Expr::eq("counter", value));
        self
    }



    pub fn with_counter_is_not(mut self, value: impl Into<teaql_core::Value>) -> Self {
        self.query = self.query.and_filter(Expr::ne("counter", value));
        self
    }

    pub fn with_counter_greater_than(mut self, value: impl Into<teaql_core::Value>) -> Self {
        self.query = self.query.and_filter(Expr::gt("counter", value));
        self
    }

    pub fn with_counter_greater_than_or_equal_to(mut self, value: impl Into<teaql_core::Value>) -> Self {
        self.query = self.query.and_filter(Expr::gte("counter", value));
        self
    }

    pub fn with_counter_less_than(mut self, value: impl Into<teaql_core::Value>) -> Self {
        self.query = self.query.and_filter(Expr::lt("counter", value));
        self
    }

    pub fn with_counter_less_than_or_equal_to(mut self, value: impl Into<teaql_core::Value>) -> Self {
        self.query = self.query.and_filter(Expr::lte("counter", value));
        self
    }

    pub fn with_counter_between(
        mut self,
        lower: impl Into<teaql_core::Value>,
        upper: impl Into<teaql_core::Value>,
    ) -> Self {
        self.query = self.query.and_filter(Expr::between("counter", lower, upper));
        self
    }

    pub fn with_counter_between_range<T>(mut self, range: DateRange<T>) -> Self
    where
        T: Into<teaql_core::Value>,
    {
        self.query = self.query.and_filter(Expr::between(
            "counter",
            range.start,
            range.end,
        ));
        self
    }

    pub fn with_counter_in(
        mut self,
        values: impl IntoIterator<Item = impl Into<teaql_core::Value>>,
    ) -> Self {
        self.query = self.query.and_filter(Expr::in_list(
            "counter",
            values.into_iter().map(Into::into),
        ));
        self
    }

    pub fn with_counter_not_in(
        mut self,
        values: impl IntoIterator<Item = impl Into<teaql_core::Value>>,
    ) -> Self {
        self.query = self.query.and_filter(Expr::not_in_list(
            "counter",
            values.into_iter().map(Into::into),
        ));
        self
    }

    pub fn with_counter_before(mut self, value: impl Into<teaql_core::Value>) -> Self {
        self.query = self.query.and_filter(Expr::lt("counter", value));
        self
    }

    pub fn with_counter_after(mut self, value: impl Into<teaql_core::Value>) -> Self {
        self.query = self.query.and_filter(Expr::gt("counter", value));
        self
    }

    pub fn with_counter_is_unknown(mut self) -> Self {
        self.query = self.query.and_filter(Expr::is_null("counter"));
        self
    }



    pub fn with_counter_is_known(mut self) -> Self {
        self.query = self.query.and_filter(Expr::is_not_null("counter"));
        self
    }


    pub fn order_by_counter_asc(mut self) -> Self {
        self.query = self.query.order_asc("counter");
        self
    }

    pub fn order_by_counter_desc(mut self) -> Self {
        self.query = self.query.order_desc("counter");
        self
    }

    pub fn order_by_counter_asc_using_gbk(mut self) -> Self {
        self.query = self.query.order_gbk_asc("counter");
        self
    }

    pub fn order_by_counter_desc_using_gbk(mut self) -> Self {
        self.query = self.query.order_gbk_desc("counter");
        self
    }


    pub fn select_day(mut self) -> Self {
        self.query = self.query.project("day");
        self
    }

    pub fn project_day(self) -> Self {
        self.select_day()
    }

    pub fn select_day_raw(self, raw_sql_segment: impl Into<String>) -> Self {
        self.select_day_unsafe_raw(UnsafeRawSqlSegment::trusted(raw_sql_segment))
    }

    pub fn select_day_unsafe_raw(mut self, raw_sql_segment: UnsafeRawSqlSegment) -> Self {
        self.query_options
            .raw_projections
            .push(RawProjection::new("day", raw_sql_segment));
        self
    }

    pub fn group_by_day(self) -> Self {
        self.group_by("day")
    }

    pub fn group_by_day_as(self, alias: impl Into<String>) -> Self {
        let alias = alias.into();
        let mut request = self.group_by("day");
        request.query = request
            .query
            .project_expr(alias, Expr::column("day"));
        request
    }

    pub fn group_by_day_with_function(
        self,
        alias: impl Into<String>,
        function: AggregateFunction,
    ) -> Self {
        self.group_by("day")
            .aggregate_with_function("day", alias, function)
    }

    pub fn count_day(self) -> Self {
        self.count_day_as("day_count")
    }

    pub fn count_day_as(self, alias: impl Into<String>) -> Self {
        self.aggregate_count_field("day", alias)
    }

    pub fn sum_day(self) -> Self {
        self.sum_day_as("sum_day")
    }

    pub fn sum_day_as(self, alias: impl Into<String>) -> Self {
        self.aggregate_sum("day", alias)
    }

    pub fn avg_day(self) -> Self {
        self.avg_day_as("avg_day")
    }

    pub fn avg_day_as(self, alias: impl Into<String>) -> Self {
        self.aggregate_avg("day", alias)
    }

    pub fn min_day(self) -> Self {
        self.min_day_as("min_day")
    }

    pub fn min_day_as(self, alias: impl Into<String>) -> Self {
        self.aggregate_min("day", alias)
    }

    pub fn max_day(self) -> Self {
        self.max_day_as("max_day")
    }

    pub fn max_day_as(self, alias: impl Into<String>) -> Self {
        self.aggregate_max("day", alias)
    }

    pub fn unselect_day(mut self) -> Self {
        self.query.projection.retain(|field| field != "day");
        self.query_options.raw_projections.retain(|projection| projection.property_name != "day");
        self
    }


    pub fn with_day(
        mut self,
        operator: FieldOperator,
        values: impl IntoIterator<Item = impl Into<teaql_core::Value>>,
    ) -> Self {
        self.query = self.query.and_filter(field_operator_expr(
            "day",
            operator,
            values.into_iter().map(Into::into).collect(),
        ));
        self
    }

    pub fn create_day_criteria(
        operator: FieldOperator,
        values: impl IntoIterator<Item = impl Into<teaql_core::Value>>,
    ) -> Expr {
        field_operator_expr(
            "day",
            operator,
            values.into_iter().map(Into::into).collect(),
        )
    }

    pub fn with_day_is(mut self, value: impl Into<teaql_core::Value>) -> Self {
        self.query = self.query.and_filter(Expr::eq("day", value));
        self
    }



    pub fn with_day_is_not(mut self, value: impl Into<teaql_core::Value>) -> Self {
        self.query = self.query.and_filter(Expr::ne("day", value));
        self
    }

    pub fn with_day_greater_than(mut self, value: impl Into<teaql_core::Value>) -> Self {
        self.query = self.query.and_filter(Expr::gt("day", value));
        self
    }

    pub fn with_day_greater_than_or_equal_to(mut self, value: impl Into<teaql_core::Value>) -> Self {
        self.query = self.query.and_filter(Expr::gte("day", value));
        self
    }

    pub fn with_day_less_than(mut self, value: impl Into<teaql_core::Value>) -> Self {
        self.query = self.query.and_filter(Expr::lt("day", value));
        self
    }

    pub fn with_day_less_than_or_equal_to(mut self, value: impl Into<teaql_core::Value>) -> Self {
        self.query = self.query.and_filter(Expr::lte("day", value));
        self
    }

    pub fn with_day_between(
        mut self,
        lower: impl Into<teaql_core::Value>,
        upper: impl Into<teaql_core::Value>,
    ) -> Self {
        self.query = self.query.and_filter(Expr::between("day", lower, upper));
        self
    }

    pub fn with_day_between_range<T>(mut self, range: DateRange<T>) -> Self
    where
        T: Into<teaql_core::Value>,
    {
        self.query = self.query.and_filter(Expr::between(
            "day",
            range.start,
            range.end,
        ));
        self
    }

    pub fn with_day_in(
        mut self,
        values: impl IntoIterator<Item = impl Into<teaql_core::Value>>,
    ) -> Self {
        self.query = self.query.and_filter(Expr::in_list(
            "day",
            values.into_iter().map(Into::into),
        ));
        self
    }

    pub fn with_day_not_in(
        mut self,
        values: impl IntoIterator<Item = impl Into<teaql_core::Value>>,
    ) -> Self {
        self.query = self.query.and_filter(Expr::not_in_list(
            "day",
            values.into_iter().map(Into::into),
        ));
        self
    }

    pub fn with_day_before(mut self, value: impl Into<teaql_core::Value>) -> Self {
        self.query = self.query.and_filter(Expr::lt("day", value));
        self
    }

    pub fn with_day_after(mut self, value: impl Into<teaql_core::Value>) -> Self {
        self.query = self.query.and_filter(Expr::gt("day", value));
        self
    }

    pub fn with_day_is_unknown(mut self) -> Self {
        self.query = self.query.and_filter(Expr::is_null("day"));
        self
    }



    pub fn with_day_is_known(mut self) -> Self {
        self.query = self.query.and_filter(Expr::is_not_null("day"));
        self
    }


    pub fn order_by_day_asc(mut self) -> Self {
        self.query = self.query.order_asc("day");
        self
    }

    pub fn order_by_day_desc(mut self) -> Self {
        self.query = self.query.order_desc("day");
        self
    }

    pub fn order_by_day_asc_using_gbk(mut self) -> Self {
        self.query = self.query.order_gbk_asc("day");
        self
    }

    pub fn order_by_day_desc_using_gbk(mut self) -> Self {
        self.query = self.query.order_gbk_desc("day");
        self
    }


    pub fn select_optional_day(mut self) -> Self {
        self.query = self.query.project("optional_day");
        self
    }

    pub fn project_optional_day(self) -> Self {
        self.select_optional_day()
    }

    pub fn select_optional_day_raw(self, raw_sql_segment: impl Into<String>) -> Self {
        self.select_optional_day_unsafe_raw(UnsafeRawSqlSegment::trusted(raw_sql_segment))
    }

    pub fn select_optional_day_unsafe_raw(mut self, raw_sql_segment: UnsafeRawSqlSegment) -> Self {
        self.query_options
            .raw_projections
            .push(RawProjection::new("optional_day", raw_sql_segment));
        self
    }

    pub fn group_by_optional_day(self) -> Self {
        self.group_by("optional_day")
    }

    pub fn group_by_optional_day_as(self, alias: impl Into<String>) -> Self {
        let alias = alias.into();
        let mut request = self.group_by("optional_day");
        request.query = request
            .query
            .project_expr(alias, Expr::column("optional_day"));
        request
    }

    pub fn group_by_optional_day_with_function(
        self,
        alias: impl Into<String>,
        function: AggregateFunction,
    ) -> Self {
        self.group_by("optional_day")
            .aggregate_with_function("optional_day", alias, function)
    }

    pub fn count_optional_day(self) -> Self {
        self.count_optional_day_as("optional_day_count")
    }

    pub fn count_optional_day_as(self, alias: impl Into<String>) -> Self {
        self.aggregate_count_field("optional_day", alias)
    }

    pub fn sum_optional_day(self) -> Self {
        self.sum_optional_day_as("sum_optional_day")
    }

    pub fn sum_optional_day_as(self, alias: impl Into<String>) -> Self {
        self.aggregate_sum("optional_day", alias)
    }

    pub fn avg_optional_day(self) -> Self {
        self.avg_optional_day_as("avg_optional_day")
    }

    pub fn avg_optional_day_as(self, alias: impl Into<String>) -> Self {
        self.aggregate_avg("optional_day", alias)
    }

    pub fn min_optional_day(self) -> Self {
        self.min_optional_day_as("min_optional_day")
    }

    pub fn min_optional_day_as(self, alias: impl Into<String>) -> Self {
        self.aggregate_min("optional_day", alias)
    }

    pub fn max_optional_day(self) -> Self {
        self.max_optional_day_as("max_optional_day")
    }

    pub fn max_optional_day_as(self, alias: impl Into<String>) -> Self {
        self.aggregate_max("optional_day", alias)
    }

    pub fn unselect_optional_day(mut self) -> Self {
        self.query.projection.retain(|field| field != "optional_day");
        self.query_options.raw_projections.retain(|projection| projection.property_name != "optional_day");
        self
    }


    pub fn with_optional_day(
        mut self,
        operator: FieldOperator,
        values: impl IntoIterator<Item = impl Into<teaql_core::Value>>,
    ) -> Self {
        self.query = self.query.and_filter(field_operator_expr(
            "optional_day",
            operator,
            values.into_iter().map(Into::into).collect(),
        ));
        self
    }

    pub fn create_optional_day_criteria(
        operator: FieldOperator,
        values: impl IntoIterator<Item = impl Into<teaql_core::Value>>,
    ) -> Expr {
        field_operator_expr(
            "optional_day",
            operator,
            values.into_iter().map(Into::into).collect(),
        )
    }

    pub fn with_optional_day_is(mut self, value: impl Into<teaql_core::Value>) -> Self {
        self.query = self.query.and_filter(Expr::eq("optional_day", value));
        self
    }



    pub fn with_optional_day_is_not(mut self, value: impl Into<teaql_core::Value>) -> Self {
        self.query = self.query.and_filter(Expr::ne("optional_day", value));
        self
    }

    pub fn with_optional_day_greater_than(mut self, value: impl Into<teaql_core::Value>) -> Self {
        self.query = self.query.and_filter(Expr::gt("optional_day", value));
        self
    }

    pub fn with_optional_day_greater_than_or_equal_to(mut self, value: impl Into<teaql_core::Value>) -> Self {
        self.query = self.query.and_filter(Expr::gte("optional_day", value));
        self
    }

    pub fn with_optional_day_less_than(mut self, value: impl Into<teaql_core::Value>) -> Self {
        self.query = self.query.and_filter(Expr::lt("optional_day", value));
        self
    }

    pub fn with_optional_day_less_than_or_equal_to(mut self, value: impl Into<teaql_core::Value>) -> Self {
        self.query = self.query.and_filter(Expr::lte("optional_day", value));
        self
    }

    pub fn with_optional_day_between(
        mut self,
        lower: impl Into<teaql_core::Value>,
        upper: impl Into<teaql_core::Value>,
    ) -> Self {
        self.query = self.query.and_filter(Expr::between("optional_day", lower, upper));
        self
    }

    pub fn with_optional_day_between_range<T>(mut self, range: DateRange<T>) -> Self
    where
        T: Into<teaql_core::Value>,
    {
        self.query = self.query.and_filter(Expr::between(
            "optional_day",
            range.start,
            range.end,
        ));
        self
    }

    pub fn with_optional_day_in(
        mut self,
        values: impl IntoIterator<Item = impl Into<teaql_core::Value>>,
    ) -> Self {
        self.query = self.query.and_filter(Expr::in_list(
            "optional_day",
            values.into_iter().map(Into::into),
        ));
        self
    }

    pub fn with_optional_day_not_in(
        mut self,
        values: impl IntoIterator<Item = impl Into<teaql_core::Value>>,
    ) -> Self {
        self.query = self.query.and_filter(Expr::not_in_list(
            "optional_day",
            values.into_iter().map(Into::into),
        ));
        self
    }

    pub fn with_optional_day_before(mut self, value: impl Into<teaql_core::Value>) -> Self {
        self.query = self.query.and_filter(Expr::lt("optional_day", value));
        self
    }

    pub fn with_optional_day_after(mut self, value: impl Into<teaql_core::Value>) -> Self {
        self.query = self.query.and_filter(Expr::gt("optional_day", value));
        self
    }

    pub fn with_optional_day_is_unknown(mut self) -> Self {
        self.query = self.query.and_filter(Expr::is_null("optional_day"));
        self
    }



    pub fn with_optional_day_is_known(mut self) -> Self {
        self.query = self.query.and_filter(Expr::is_not_null("optional_day"));
        self
    }


    pub fn order_by_optional_day_asc(mut self) -> Self {
        self.query = self.query.order_asc("optional_day");
        self
    }

    pub fn order_by_optional_day_desc(mut self) -> Self {
        self.query = self.query.order_desc("optional_day");
        self
    }

    pub fn order_by_optional_day_asc_using_gbk(mut self) -> Self {
        self.query = self.query.order_gbk_asc("optional_day");
        self
    }

    pub fn order_by_optional_day_desc_using_gbk(mut self) -> Self {
        self.query = self.query.order_gbk_desc("optional_day");
        self
    }


    pub fn select_moment(mut self) -> Self {
        self.query = self.query.project("moment");
        self
    }

    pub fn project_moment(self) -> Self {
        self.select_moment()
    }

    pub fn select_moment_raw(self, raw_sql_segment: impl Into<String>) -> Self {
        self.select_moment_unsafe_raw(UnsafeRawSqlSegment::trusted(raw_sql_segment))
    }

    pub fn select_moment_unsafe_raw(mut self, raw_sql_segment: UnsafeRawSqlSegment) -> Self {
        self.query_options
            .raw_projections
            .push(RawProjection::new("moment", raw_sql_segment));
        self
    }

    pub fn group_by_moment(self) -> Self {
        self.group_by("moment")
    }

    pub fn group_by_moment_as(self, alias: impl Into<String>) -> Self {
        let alias = alias.into();
        let mut request = self.group_by("moment");
        request.query = request
            .query
            .project_expr(alias, Expr::column("moment"));
        request
    }

    pub fn group_by_moment_with_function(
        self,
        alias: impl Into<String>,
        function: AggregateFunction,
    ) -> Self {
        self.group_by("moment")
            .aggregate_with_function("moment", alias, function)
    }

    pub fn count_moment(self) -> Self {
        self.count_moment_as("moment_count")
    }

    pub fn count_moment_as(self, alias: impl Into<String>) -> Self {
        self.aggregate_count_field("moment", alias)
    }

    pub fn sum_moment(self) -> Self {
        self.sum_moment_as("sum_moment")
    }

    pub fn sum_moment_as(self, alias: impl Into<String>) -> Self {
        self.aggregate_sum("moment", alias)
    }

    pub fn avg_moment(self) -> Self {
        self.avg_moment_as("avg_moment")
    }

    pub fn avg_moment_as(self, alias: impl Into<String>) -> Self {
        self.aggregate_avg("moment", alias)
    }

    pub fn min_moment(self) -> Self {
        self.min_moment_as("min_moment")
    }

    pub fn min_moment_as(self, alias: impl Into<String>) -> Self {
        self.aggregate_min("moment", alias)
    }

    pub fn max_moment(self) -> Self {
        self.max_moment_as("max_moment")
    }

    pub fn max_moment_as(self, alias: impl Into<String>) -> Self {
        self.aggregate_max("moment", alias)
    }

    pub fn unselect_moment(mut self) -> Self {
        self.query.projection.retain(|field| field != "moment");
        self.query_options.raw_projections.retain(|projection| projection.property_name != "moment");
        self
    }


    pub fn with_moment(
        mut self,
        operator: FieldOperator,
        values: impl IntoIterator<Item = impl Into<teaql_core::Value>>,
    ) -> Self {
        self.query = self.query.and_filter(field_operator_expr(
            "moment",
            operator,
            values.into_iter().map(Into::into).collect(),
        ));
        self
    }

    pub fn create_moment_criteria(
        operator: FieldOperator,
        values: impl IntoIterator<Item = impl Into<teaql_core::Value>>,
    ) -> Expr {
        field_operator_expr(
            "moment",
            operator,
            values.into_iter().map(Into::into).collect(),
        )
    }

    pub fn with_moment_is(mut self, value: impl Into<teaql_core::Value>) -> Self {
        self.query = self.query.and_filter(Expr::eq("moment", value));
        self
    }



    pub fn with_moment_is_not(mut self, value: impl Into<teaql_core::Value>) -> Self {
        self.query = self.query.and_filter(Expr::ne("moment", value));
        self
    }

    pub fn with_moment_greater_than(mut self, value: impl Into<teaql_core::Value>) -> Self {
        self.query = self.query.and_filter(Expr::gt("moment", value));
        self
    }

    pub fn with_moment_greater_than_or_equal_to(mut self, value: impl Into<teaql_core::Value>) -> Self {
        self.query = self.query.and_filter(Expr::gte("moment", value));
        self
    }

    pub fn with_moment_less_than(mut self, value: impl Into<teaql_core::Value>) -> Self {
        self.query = self.query.and_filter(Expr::lt("moment", value));
        self
    }

    pub fn with_moment_less_than_or_equal_to(mut self, value: impl Into<teaql_core::Value>) -> Self {
        self.query = self.query.and_filter(Expr::lte("moment", value));
        self
    }

    pub fn with_moment_between(
        mut self,
        lower: impl Into<teaql_core::Value>,
        upper: impl Into<teaql_core::Value>,
    ) -> Self {
        self.query = self.query.and_filter(Expr::between("moment", lower, upper));
        self
    }

    pub fn with_moment_between_range<T>(mut self, range: DateRange<T>) -> Self
    where
        T: Into<teaql_core::Value>,
    {
        self.query = self.query.and_filter(Expr::between(
            "moment",
            range.start,
            range.end,
        ));
        self
    }

    pub fn with_moment_in(
        mut self,
        values: impl IntoIterator<Item = impl Into<teaql_core::Value>>,
    ) -> Self {
        self.query = self.query.and_filter(Expr::in_list(
            "moment",
            values.into_iter().map(Into::into),
        ));
        self
    }

    pub fn with_moment_not_in(
        mut self,
        values: impl IntoIterator<Item = impl Into<teaql_core::Value>>,
    ) -> Self {
        self.query = self.query.and_filter(Expr::not_in_list(
            "moment",
            values.into_iter().map(Into::into),
        ));
        self
    }

    pub fn with_moment_before(mut self, value: impl Into<teaql_core::Value>) -> Self {
        self.query = self.query.and_filter(Expr::lt("moment", value));
        self
    }

    pub fn with_moment_after(mut self, value: impl Into<teaql_core::Value>) -> Self {
        self.query = self.query.and_filter(Expr::gt("moment", value));
        self
    }

    pub fn with_moment_is_unknown(mut self) -> Self {
        self.query = self.query.and_filter(Expr::is_null("moment"));
        self
    }



    pub fn with_moment_is_known(mut self) -> Self {
        self.query = self.query.and_filter(Expr::is_not_null("moment"));
        self
    }


    pub fn order_by_moment_asc(mut self) -> Self {
        self.query = self.query.order_asc("moment");
        self
    }

    pub fn order_by_moment_desc(mut self) -> Self {
        self.query = self.query.order_desc("moment");
        self
    }

    pub fn order_by_moment_asc_using_gbk(mut self) -> Self {
        self.query = self.query.order_gbk_asc("moment");
        self
    }

    pub fn order_by_moment_desc_using_gbk(mut self) -> Self {
        self.query = self.query.order_gbk_desc("moment");
        self
    }


    pub fn select_optional_moment(mut self) -> Self {
        self.query = self.query.project("optional_moment");
        self
    }

    pub fn project_optional_moment(self) -> Self {
        self.select_optional_moment()
    }

    pub fn select_optional_moment_raw(self, raw_sql_segment: impl Into<String>) -> Self {
        self.select_optional_moment_unsafe_raw(UnsafeRawSqlSegment::trusted(raw_sql_segment))
    }

    pub fn select_optional_moment_unsafe_raw(mut self, raw_sql_segment: UnsafeRawSqlSegment) -> Self {
        self.query_options
            .raw_projections
            .push(RawProjection::new("optional_moment", raw_sql_segment));
        self
    }

    pub fn group_by_optional_moment(self) -> Self {
        self.group_by("optional_moment")
    }

    pub fn group_by_optional_moment_as(self, alias: impl Into<String>) -> Self {
        let alias = alias.into();
        let mut request = self.group_by("optional_moment");
        request.query = request
            .query
            .project_expr(alias, Expr::column("optional_moment"));
        request
    }

    pub fn group_by_optional_moment_with_function(
        self,
        alias: impl Into<String>,
        function: AggregateFunction,
    ) -> Self {
        self.group_by("optional_moment")
            .aggregate_with_function("optional_moment", alias, function)
    }

    pub fn count_optional_moment(self) -> Self {
        self.count_optional_moment_as("optional_moment_count")
    }

    pub fn count_optional_moment_as(self, alias: impl Into<String>) -> Self {
        self.aggregate_count_field("optional_moment", alias)
    }

    pub fn sum_optional_moment(self) -> Self {
        self.sum_optional_moment_as("sum_optional_moment")
    }

    pub fn sum_optional_moment_as(self, alias: impl Into<String>) -> Self {
        self.aggregate_sum("optional_moment", alias)
    }

    pub fn avg_optional_moment(self) -> Self {
        self.avg_optional_moment_as("avg_optional_moment")
    }

    pub fn avg_optional_moment_as(self, alias: impl Into<String>) -> Self {
        self.aggregate_avg("optional_moment", alias)
    }

    pub fn min_optional_moment(self) -> Self {
        self.min_optional_moment_as("min_optional_moment")
    }

    pub fn min_optional_moment_as(self, alias: impl Into<String>) -> Self {
        self.aggregate_min("optional_moment", alias)
    }

    pub fn max_optional_moment(self) -> Self {
        self.max_optional_moment_as("max_optional_moment")
    }

    pub fn max_optional_moment_as(self, alias: impl Into<String>) -> Self {
        self.aggregate_max("optional_moment", alias)
    }

    pub fn unselect_optional_moment(mut self) -> Self {
        self.query.projection.retain(|field| field != "optional_moment");
        self.query_options.raw_projections.retain(|projection| projection.property_name != "optional_moment");
        self
    }


    pub fn with_optional_moment(
        mut self,
        operator: FieldOperator,
        values: impl IntoIterator<Item = impl Into<teaql_core::Value>>,
    ) -> Self {
        self.query = self.query.and_filter(field_operator_expr(
            "optional_moment",
            operator,
            values.into_iter().map(Into::into).collect(),
        ));
        self
    }

    pub fn create_optional_moment_criteria(
        operator: FieldOperator,
        values: impl IntoIterator<Item = impl Into<teaql_core::Value>>,
    ) -> Expr {
        field_operator_expr(
            "optional_moment",
            operator,
            values.into_iter().map(Into::into).collect(),
        )
    }

    pub fn with_optional_moment_is(mut self, value: impl Into<teaql_core::Value>) -> Self {
        self.query = self.query.and_filter(Expr::eq("optional_moment", value));
        self
    }



    pub fn with_optional_moment_is_not(mut self, value: impl Into<teaql_core::Value>) -> Self {
        self.query = self.query.and_filter(Expr::ne("optional_moment", value));
        self
    }

    pub fn with_optional_moment_greater_than(mut self, value: impl Into<teaql_core::Value>) -> Self {
        self.query = self.query.and_filter(Expr::gt("optional_moment", value));
        self
    }

    pub fn with_optional_moment_greater_than_or_equal_to(mut self, value: impl Into<teaql_core::Value>) -> Self {
        self.query = self.query.and_filter(Expr::gte("optional_moment", value));
        self
    }

    pub fn with_optional_moment_less_than(mut self, value: impl Into<teaql_core::Value>) -> Self {
        self.query = self.query.and_filter(Expr::lt("optional_moment", value));
        self
    }

    pub fn with_optional_moment_less_than_or_equal_to(mut self, value: impl Into<teaql_core::Value>) -> Self {
        self.query = self.query.and_filter(Expr::lte("optional_moment", value));
        self
    }

    pub fn with_optional_moment_between(
        mut self,
        lower: impl Into<teaql_core::Value>,
        upper: impl Into<teaql_core::Value>,
    ) -> Self {
        self.query = self.query.and_filter(Expr::between("optional_moment", lower, upper));
        self
    }

    pub fn with_optional_moment_between_range<T>(mut self, range: DateRange<T>) -> Self
    where
        T: Into<teaql_core::Value>,
    {
        self.query = self.query.and_filter(Expr::between(
            "optional_moment",
            range.start,
            range.end,
        ));
        self
    }

    pub fn with_optional_moment_in(
        mut self,
        values: impl IntoIterator<Item = impl Into<teaql_core::Value>>,
    ) -> Self {
        self.query = self.query.and_filter(Expr::in_list(
            "optional_moment",
            values.into_iter().map(Into::into),
        ));
        self
    }

    pub fn with_optional_moment_not_in(
        mut self,
        values: impl IntoIterator<Item = impl Into<teaql_core::Value>>,
    ) -> Self {
        self.query = self.query.and_filter(Expr::not_in_list(
            "optional_moment",
            values.into_iter().map(Into::into),
        ));
        self
    }

    pub fn with_optional_moment_before(mut self, value: impl Into<teaql_core::Value>) -> Self {
        self.query = self.query.and_filter(Expr::lt("optional_moment", value));
        self
    }

    pub fn with_optional_moment_after(mut self, value: impl Into<teaql_core::Value>) -> Self {
        self.query = self.query.and_filter(Expr::gt("optional_moment", value));
        self
    }

    pub fn with_optional_moment_is_unknown(mut self) -> Self {
        self.query = self.query.and_filter(Expr::is_null("optional_moment"));
        self
    }



    pub fn with_optional_moment_is_known(mut self) -> Self {
        self.query = self.query.and_filter(Expr::is_not_null("optional_moment"));
        self
    }


    pub fn order_by_optional_moment_asc(mut self) -> Self {
        self.query = self.query.order_asc("optional_moment");
        self
    }

    pub fn order_by_optional_moment_desc(mut self) -> Self {
        self.query = self.query.order_desc("optional_moment");
        self
    }

    pub fn order_by_optional_moment_asc_using_gbk(mut self) -> Self {
        self.query = self.query.order_gbk_asc("optional_moment");
        self
    }

    pub fn order_by_optional_moment_desc_using_gbk(mut self) -> Self {
        self.query = self.query.order_gbk_desc("optional_moment");
        self
    }

    pub fn group_by_version(self) -> Self {
        self.group_by("version")
    }

    pub fn group_by_version_as(self, alias: impl Into<String>) -> Self {
        let alias = alias.into();
        let mut request = self.group_by("version");
        request.query = request
            .query
            .project_expr(alias, Expr::column("version"));
        request
    }

    pub fn group_by_version_with_function(
        self,
        alias: impl Into<String>,
        function: AggregateFunction,
    ) -> Self {
        self.group_by("version")
            .aggregate_with_function("version", alias, function)
    }

    pub fn count_version(self) -> Self {
        self.count_version_as("version_count")
    }

    pub fn count_version_as(self, alias: impl Into<String>) -> Self {
        self.aggregate_count_field("version", alias)
    }

    pub fn sum_version(self) -> Self {
        self.sum_version_as("sum_version")
    }

    pub fn sum_version_as(self, alias: impl Into<String>) -> Self {
        self.aggregate_sum("version", alias)
    }

    pub fn avg_version(self) -> Self {
        self.avg_version_as("avg_version")
    }

    pub fn avg_version_as(self, alias: impl Into<String>) -> Self {
        self.aggregate_avg("version", alias)
    }

    pub fn min_version(self) -> Self {
        self.min_version_as("min_version")
    }

    pub fn min_version_as(self, alias: impl Into<String>) -> Self {
        self.aggregate_min("version", alias)
    }

    pub fn max_version(self) -> Self {
        self.max_version_as("max_version")
    }

    pub fn max_version_as(self, alias: impl Into<String>) -> Self {
        self.aggregate_max("version", alias)
    }

    pub fn order_by_version_asc(mut self) -> Self {
        self.query = self.query.order_asc("version");
        self
    }

    pub fn order_by_version_desc(mut self) -> Self {
        self.query = self.query.order_desc("version");
        self
    }

    pub fn order_by_version_asc_using_gbk(mut self) -> Self {
        self.query = self.query.order_gbk_asc("version");
        self
    }

    pub fn order_by_version_desc_using_gbk(mut self) -> Self {
        self.query = self.query.order_gbk_desc("version");
        self
    }
    pub fn filter_by_tenant(mut self, value: impl EntityReference) -> Self {
        self.query = self.query.and_filter(Expr::eq("tenant_id", value.entity_id_value()));
        self
    }

    pub fn with_tenant_matching(mut self, request: impl Into<QuerySelection>) -> Self {
        let selection = request.into();
        self.query = self.query.and_filter(Expr::in_subquery(
            "tenant_id",
            <crate::Tenant as teaql_core::TeaqlEntity>::entity_descriptor(),
            selection.query.clone(),
            "id",
        ));
        self.relation_filters.push(RelationFilter::new("tenant", selection));
        self
    }


    pub fn without_tenant_matching(mut self, request: impl Into<QuerySelection>) -> Self {
        let selection = request.into();
        self.query = self.query.and_filter(Expr::not_in_subquery(
            "tenant_id",
            <crate::Tenant as teaql_core::TeaqlEntity>::entity_descriptor(),
            selection.query.clone(),
            "id",
        ));
        self.relation_filters.push(RelationFilter::new("tenant", selection));
        self
    }


    pub fn have_tenant(mut self) -> Self {
        self.query = self.query.and_filter(Expr::is_not_null("tenant_id"));
        self
    }

    pub fn have_no_tenant(mut self) -> Self {
        self.query = self.query.and_filter(Expr::is_null("tenant_id"));
        self
    }


    pub fn group_by_tenant(self) -> Self {
        self.group_by("tenant_id")
    }

    pub fn group_by_tenant_as(self, alias: impl Into<String>) -> Self {
        let alias = alias.into();
        let mut request = self.group_by("tenant_id");
        request.query = request
            .query
            .project_expr(alias, Expr::column("tenant_id"));
        request
    }

    pub fn group_by_tenant_with_function(
        self,
        alias: impl Into<String>,
        function: AggregateFunction,
    ) -> Self {
        self.group_by("tenant_id")
            .aggregate_with_function("tenant_id", alias, function)
    }

    pub fn group_by_tenant_with(mut self, request: impl Into<QuerySelection>) -> Self {
        self.query = self.query.group_by("tenant_id");
        self.query_options.object_group_bys.push(ObjectGroupBy::new(
            "tenant",
            "tenant_id",
            request,
        ));
        self
    }

    pub fn group_by_tenant_with_details(self) -> Self {
        self.group_by_tenant_with_details_from(crate::Q::tenants().unlimited())
    }

    pub fn group_by_tenant_with_details_from(self, request: impl Into<QuerySelection>) -> Self {
        self.group_by_tenant_with(request)
    }


    pub fn roll_up_to_tenant(self) -> Self {
        self.roll_up_to_tenant_with(crate::Q::tenants().unlimited())
    }

    pub fn roll_up_to_tenant_with(self, request: impl Into<QuerySelection>) -> Self {
        let selection = request.into();
        self.with_tenant_matching(selection.clone())
            .group_by_tenant_with(selection)
    }

    pub fn count_tenant(self) -> Self {
        self.count_tenant_as("tenant_count")
    }

    pub fn count_tenant_as(self, alias: impl Into<String>) -> Self {
        self.aggregate_count_field("tenant_id", alias)
    }

    pub fn unselect_tenant(mut self) -> Self {
        self.query.projection.retain(|field| field != "tenant_id");
        self.query.relations.retain(|relation| relation.name != "tenant");
        self
    }
    pub fn select_tenant(mut self) -> Self {
        self.query = self.query.relation("tenant");
        self
    }

    pub fn select_tenant_with(mut self, request: impl Into<QuerySelection>) -> Self {
        let selection = request.into();
        self.query = self.query.relation_query("tenant", selection.into_query());
        self
}

    pub fn facet_by_tenant_as(self, facet_name: impl Into<String>, request: impl Into<QuerySelection>) -> Self {
        self.facet_by_tenant_as_with_options(facet_name, request, true)
    }

    pub fn facet_by_tenant_as_with_options(
        mut self,
        facet_name: impl Into<String>,
        request: impl Into<QuerySelection>,
        include_all_facets: bool,
    ) -> Self {
        self.query_options.facets.push(FacetRequest::new(
            facet_name,
            "tenant",
            request,
            include_all_facets,
        ));
        self
    }
}

impl<R> Default for EntryRequest<R> {
    fn default() -> Self {
        Self::new()
    }
}

impl<R> From< EntryRequest<R> > for SelectQuery {
    fn from(request: EntryRequest<R>) -> Self {
        QuerySelection::from(request).into_query()
    }
}

impl<R> From< EntryRequest<R> > for QuerySelection {
    fn from(request: EntryRequest<R>) -> Self {
        Self {
            query: request.query,
            relation_selections: request.relation_selections,
            relation_filters: request.relation_filters,
            child_enhancements: request.child_enhancements,
            query_options: request.query_options,
        }
    }
}


impl<'a, C> crate::request_support::AuditedSave<'a, C> for teaql_core::Audited<crate::Entry> 
where C: crate::TeaqlRuntime + ?Sized + 'a
{
    type Error = teaql_runtime::RuntimeError;
    type Entity = crate::Entry;
    fn save(self, context: &'a C) -> std::pin::Pin<Box<dyn std::future::Future<Output = Result<Self::Entity, Self::Error>> + Send + '_>> {
        Box::pin(async move {
            context.save_audited_entity(self).await
        })
    }
}

impl<R: teaql_core::Entity> crate::PurposedQuery<EntryRequest<R>> {
    pub fn comment(mut self, comment: impl Into<String>) -> Self {
        self.inner.query_options.comment = Some(comment.into());
        self
    }

    pub fn new_entity<C>(&self, context: &C) -> crate::Entry
    where
        C: crate::TeaqlRuntime + ?Sized,
    {
        self.require_comment();
        let mut entity = crate::Entry::runtime_new(context.user_context().entity_runtime_state());
        if let Ok(id) = context.user_context().next_id(crate::Entry::ENTITY_NAME) {
            entity.update_id(id);
        }
        teaql_core::Entity::mark_as_new(&mut entity);
        entity
    }

    fn into_inner_with_trace(mut self) -> EntryRequest<R> {
        self.require_comment();
        self.inner.query.trace_chain.push(teaql_core::TraceNode::typed(
            teaql_core::TraceKind::Purpose,
            self.inner.query.entity.clone(),
            None,
            self.purpose,
        ));
        self.inner
    }

    fn require_comment(&self) {
        assert!(
            self.inner
                .query_options
                .comment
                .as_deref()
                .is_some_and(|comment| !comment.trim().is_empty()),
            "query comment must not be empty"
        );
    }

    pub async fn execute_for_page<'a, C>(
        self,
        context: &'a C,
        offset: u64,
        limit: u64,
    ) -> Result<teaql_core::SmartList<R>, teaql_runtime::RuntimeError>
    where
        C: crate::TeaqlRuntime + ?Sized,
        R: teaql_core::Entity + Send + 'a,
    {
        self.into_inner_with_trace()._execute_for_page(context, offset, limit).await
    }

    pub async fn execute_for_exists<'a, C>(
        self,
        context: &'a C,
    ) -> Result<bool, teaql_runtime::RuntimeError>
    where
        C: crate::TeaqlRuntime + ?Sized,
    {
        self.into_inner_with_trace()._execute_for_exists(context).await
    }

    pub async fn execute_for_list<'a, C>(self, context: &'a C) -> Result<teaql_core::SmartList<R>, teaql_runtime::RuntimeError>
    where
        C: crate::TeaqlRuntime + ?Sized,
        R: teaql_core::Entity + Send + 'a,
    {
        self.into_inner_with_trace()._execute_for_list(context).await
    }

    pub async fn execute_for_rows<'a, C>(self, context: &'a C) -> Result<teaql_core::SmartList<teaql_core::CompactRow>, teaql_runtime::RuntimeError>
    where
        C: crate::TeaqlRuntime + ?Sized,
    {
        self.into_inner_with_trace()._execute_for_rows(context).await
    }

    /// Execute query as a lazy entity stream without materializing the result set.
    /// Set chunk size via .stream(chunk_size) or .stream_default() on the query.
    pub async fn execute_for_stream<'a, C>(self, context: &'a C) -> Result<crate::request_support::TeaqlEntityStream<'a, R, teaql_runtime::RuntimeError>, teaql_runtime::RuntimeError>
    where
        C: crate::TeaqlRuntime + ?Sized,
        R: teaql_core::Entity + Send + 'a,
    {
        self.into_inner_with_trace()._execute_for_stream(context).await
    }

    pub async fn execute_for_first<'a, C>(self, context: &'a C) -> Result<Option<R>, teaql_runtime::RuntimeError>
    where
        C: crate::TeaqlRuntime + ?Sized,
        R: teaql_core::Entity + Send + 'a,
    {
        self.into_inner_with_trace()._execute_for_first(context).await
    }

    pub async fn execute_for_one<'a, C>(self, context: &'a C) -> Result<Option<R>, teaql_runtime::RuntimeError>
    where
        C: crate::TeaqlRuntime + ?Sized,
        R: teaql_core::Entity + Send + 'a,
    {
        self.into_inner_with_trace()._execute_for_one(context).await
    }


    pub async fn execute_for_count<'a, C>(self, context: &'a C) -> Result<u64, teaql_runtime::RuntimeError>
    where
        C: crate::TeaqlRuntime + ?Sized,
    {
        self.into_inner_with_trace()._execute_for_count(context).await
    }
}
