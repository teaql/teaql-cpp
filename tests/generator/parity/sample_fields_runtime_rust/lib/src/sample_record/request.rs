use std::marker::PhantomData;

use serde_json::Value as JsonValue;
use teaql_core::{Aggregate, AggregateFunction, EntityDescriptor, Expr, SelectQuery, SmartList};
use teaql_runtime::RuntimeError;

use crate::request_support::*;

impl EntityReference for crate::SampleRecord {
    fn entity_id_value(self) -> teaql_core::Value {
        teaql_core::IdentifiableEntity::id_value(&self)
    }
}

impl EntityReference for &crate::SampleRecord {
    fn entity_id_value(self) -> teaql_core::Value {
        teaql_core::IdentifiableEntity::id_value(self)
    }
}

// ⛔ AI agents: DO NOT read this file for API discovery. Instead run: cargo teaql --input modeling/MODEL.xml rust-assist-query/sample_record
#[derive(Debug)]
pub struct SampleRecordRequest<R = crate::SampleRecord> {
    query: SelectQuery,
    relation_selections: Vec<RelationSelection>,
    relation_filters: Vec<RelationFilter>,
    child_enhancements: Vec<QuerySelection>,
    query_options: QueryOptions,
    marker: PhantomData<R>,
}

impl<R> Clone for SampleRecordRequest<R> {
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

impl<R> SampleRecordRequest<R> {
    pub(crate) fn new() -> Self {
        Self {
            query: SelectQuery::new("SampleRecord")
                .project("id")
                .project("version"),
            relation_selections: Vec::new(),
            relation_filters: Vec::new(),
            child_enhancements: Vec::new(),
            query_options: QueryOptions::default(),
            marker: PhantomData,
        }
    }

    pub fn return_type<T>(self) -> SampleRecordRequest<T> {
        SampleRecordRequest {
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
                "SampleRecord",
                query,
                relation_aggregates,
            ).await?;
            (rows, std::collections::BTreeMap::new())
        } else {
            let rows = context.fetch_entity_smart_list::<R>(
                "SampleRecord",
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
        context.fetch_compact_smart_list("SampleRecord", &query).await
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
        Ok(context.fetch_entity_stream("SampleRecord", query))
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
        let rows = context.fetch_compact_rows("SampleRecord", &query).await?;
        rows.first()
            .and_then(|row| row.get(COUNT_ALIAS))
            .and_then(teaql_core::Value::try_u64)
            .ok_or_else(|| RuntimeError::Graph(format!("count result for SampleRecord is missing or not numeric")))
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
        let rows = context.fetch_compact_rows("SampleRecord", &query).await?;
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
            "quantity" => Some("quantity"),
            "zero_bound" => Some("zero_bound"),
            "negative_max" => Some("negative_max"),
            "signed_max" => Some("signed_max"),
            "wide_max" => Some("wide_max"),
            "negative_bound" => Some("negative_bound"),
            "amount" => Some("amount"),
            "height" => Some("height"),
            "optional_count" => Some("optional_count"),
            "active" => Some("active"),
            "disabled" => Some("disabled"),
            "optional_active" => Some("optional_active"),
            "day" => Some("day"),
            "optional_day" => Some("optional_day"),
            "recorded_at" => Some("recorded_at"),
            "optional_recorded_at" => Some("optional_recorded_at"),
            "clock_text" => Some("clock_text"),
            "optional_clock_text" => Some("optional_clock_text"),
            "ratio" => Some("ratio"),
            "payload" => Some("payload"),
            "version" => Some("version"),
            "platform" | "platform_id" => Some("platform_id"),
            _ => None,
        }
    }

    fn apply_dynamic_json_chain_filter(self, head: &str, tail: &str, value: &JsonValue) -> Self {
        let _ = (tail, value);
        match head {
            "platform" => {
                self.with_platform_matching(
                    crate::Q::platforms_minimal()
                        .apply_dynamic_json_filter(tail, value),
                )
            }
            "first_links" => {
                self.with_first_links_matching(
                    crate::Q::sample_links_minimal()
                        .apply_dynamic_json_filter(tail, value),
                )
            }
            "second_links" => {
                self.with_second_links_matching(
                    crate::Q::sample_links_minimal()
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
        self.query = self.query.project("quantity");
        self.query = self.query.project("zero_bound");
        self.query = self.query.project("negative_max");
        self.query = self.query.project("signed_max");
        self.query = self.query.project("wide_max");
        self.query = self.query.project("negative_bound");
        self.query = self.query.project("amount");
        self.query = self.query.project("height");
        self.query = self.query.project("optional_count");
        self.query = self.query.project("active");
        self.query = self.query.project("disabled");
        self.query = self.query.project("optional_active");
        self.query = self.query.project("day");
        self.query = self.query.project("optional_day");
        self.query = self.query.project("recorded_at");
        self.query = self.query.project("optional_recorded_at");
        self.query = self.query.project("clock_text");
        self.query = self.query.project("optional_clock_text");
        self.query = self.query.project("ratio");
        self.query = self.query.project("payload");
        self.query = self.query.project("version");
        self.query = self.query.project("platform_id");
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
        request = request.select_platform();
        request
    }

    pub fn select_children(self) -> Self {
        let mut request = self.select_all();
        request = request.select_first_links();
        request = request.select_second_links();
        request
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


    pub fn select_quantity(mut self) -> Self {
        self.query = self.query.project("quantity");
        self
    }

    pub fn project_quantity(self) -> Self {
        self.select_quantity()
    }

    pub fn select_quantity_raw(self, raw_sql_segment: impl Into<String>) -> Self {
        self.select_quantity_unsafe_raw(UnsafeRawSqlSegment::trusted(raw_sql_segment))
    }

    pub fn select_quantity_unsafe_raw(mut self, raw_sql_segment: UnsafeRawSqlSegment) -> Self {
        self.query_options
            .raw_projections
            .push(RawProjection::new("quantity", raw_sql_segment));
        self
    }

    pub fn select_quantity_with_function(self, function: AggregateFunction) -> Self {
        self.select_quantity_as_with_function("quantity", function)
    }

    pub fn select_quantity_as_with_function(
        self,
        alias: impl Into<String>,
        function: AggregateFunction,
    ) -> Self {
        self.aggregate_with_function("quantity", alias, function)
    }

    pub fn group_by_quantity(self) -> Self {
        self.group_by("quantity")
    }

    pub fn group_by_quantity_as(self, alias: impl Into<String>) -> Self {
        let alias = alias.into();
        let mut request = self.group_by("quantity");
        request.query = request
            .query
            .project_expr(alias, Expr::column("quantity"));
        request
    }

    pub fn group_by_quantity_with_function(
        self,
        alias: impl Into<String>,
        function: AggregateFunction,
    ) -> Self {
        self.group_by("quantity")
            .aggregate_with_function("quantity", alias, function)
    }

    pub fn count_quantity(self) -> Self {
        self.count_quantity_as("quantity_count")
    }

    pub fn count_quantity_as(self, alias: impl Into<String>) -> Self {
        self.aggregate_count_field("quantity", alias)
    }

    pub fn sum_quantity(self) -> Self {
        self.sum_quantity_as("sum_quantity")
    }

    pub fn sum_quantity_as(self, alias: impl Into<String>) -> Self {
        self.aggregate_sum("quantity", alias)
    }

    pub fn avg_quantity(self) -> Self {
        self.avg_quantity_as("avg_quantity")
    }

    pub fn avg_quantity_as(self, alias: impl Into<String>) -> Self {
        self.aggregate_avg("quantity", alias)
    }

    pub fn min_quantity(self) -> Self {
        self.min_quantity_as("min_quantity")
    }

    pub fn min_quantity_as(self, alias: impl Into<String>) -> Self {
        self.aggregate_min("quantity", alias)
    }

    pub fn max_quantity(self) -> Self {
        self.max_quantity_as("max_quantity")
    }

    pub fn max_quantity_as(self, alias: impl Into<String>) -> Self {
        self.aggregate_max("quantity", alias)
    }

    pub fn standard_deviation_quantity(self) -> Self {
        self.standard_deviation_quantity_as("stdDev_quantity")
    }

    pub fn standard_deviation_quantity_as(self, alias: impl Into<String>) -> Self {
        self.aggregate_stddev("quantity", alias)
    }

    pub fn square_root_of_population_standard_deviation_quantity(self) -> Self {
        self.square_root_of_population_standard_deviation_quantity_as("stdDevPop_quantity")
    }

    pub fn square_root_of_population_standard_deviation_quantity_as(self, alias: impl Into<String>) -> Self {
        self.aggregate_stddev_pop("quantity", alias)
    }

    pub fn sample_variance_quantity(self) -> Self {
        self.sample_variance_quantity_as("varSamp_quantity")
    }

    pub fn sample_variance_quantity_as(self, alias: impl Into<String>) -> Self {
        self.aggregate_var_samp("quantity", alias)
    }

    pub fn sample_population_variance_quantity(self) -> Self {
        self.sample_population_variance_quantity_as("varPop_quantity")
    }

    pub fn sample_population_variance_quantity_as(self, alias: impl Into<String>) -> Self {
        self.aggregate_var_pop("quantity", alias)
    }

    pub fn unselect_quantity(mut self) -> Self {
        self.query.projection.retain(|field| field != "quantity");
        self.query_options.raw_projections.retain(|projection| projection.property_name != "quantity");
        self
    }


    pub fn with_quantity(
        mut self,
        operator: FieldOperator,
        values: impl IntoIterator<Item = impl Into<teaql_core::Value>>,
    ) -> Self {
        self.query = self.query.and_filter(field_operator_expr(
            "quantity",
            operator,
            values.into_iter().map(Into::into).collect(),
        ));
        self
    }

    pub fn create_quantity_criteria(
        operator: FieldOperator,
        values: impl IntoIterator<Item = impl Into<teaql_core::Value>>,
    ) -> Expr {
        field_operator_expr(
            "quantity",
            operator,
            values.into_iter().map(Into::into).collect(),
        )
    }

    pub fn with_quantity_is(mut self, value: impl Into<teaql_core::Value>) -> Self {
        self.query = self.query.and_filter(Expr::eq("quantity", value));
        self
    }



    pub fn with_quantity_is_not(mut self, value: impl Into<teaql_core::Value>) -> Self {
        self.query = self.query.and_filter(Expr::ne("quantity", value));
        self
    }

    pub fn with_quantity_greater_than(mut self, value: impl Into<teaql_core::Value>) -> Self {
        self.query = self.query.and_filter(Expr::gt("quantity", value));
        self
    }

    pub fn with_quantity_greater_than_or_equal_to(mut self, value: impl Into<teaql_core::Value>) -> Self {
        self.query = self.query.and_filter(Expr::gte("quantity", value));
        self
    }

    pub fn with_quantity_less_than(mut self, value: impl Into<teaql_core::Value>) -> Self {
        self.query = self.query.and_filter(Expr::lt("quantity", value));
        self
    }

    pub fn with_quantity_less_than_or_equal_to(mut self, value: impl Into<teaql_core::Value>) -> Self {
        self.query = self.query.and_filter(Expr::lte("quantity", value));
        self
    }

    pub fn with_quantity_between(
        mut self,
        lower: impl Into<teaql_core::Value>,
        upper: impl Into<teaql_core::Value>,
    ) -> Self {
        self.query = self.query.and_filter(Expr::between("quantity", lower, upper));
        self
    }

    pub fn with_quantity_between_range<T>(mut self, range: DateRange<T>) -> Self
    where
        T: Into<teaql_core::Value>,
    {
        self.query = self.query.and_filter(Expr::between(
            "quantity",
            range.start,
            range.end,
        ));
        self
    }

    pub fn with_quantity_in(
        mut self,
        values: impl IntoIterator<Item = impl Into<teaql_core::Value>>,
    ) -> Self {
        self.query = self.query.and_filter(Expr::in_list(
            "quantity",
            values.into_iter().map(Into::into),
        ));
        self
    }

    pub fn with_quantity_not_in(
        mut self,
        values: impl IntoIterator<Item = impl Into<teaql_core::Value>>,
    ) -> Self {
        self.query = self.query.and_filter(Expr::not_in_list(
            "quantity",
            values.into_iter().map(Into::into),
        ));
        self
    }

    pub fn with_quantity_before(mut self, value: impl Into<teaql_core::Value>) -> Self {
        self.query = self.query.and_filter(Expr::lt("quantity", value));
        self
    }

    pub fn with_quantity_after(mut self, value: impl Into<teaql_core::Value>) -> Self {
        self.query = self.query.and_filter(Expr::gt("quantity", value));
        self
    }

    pub fn with_quantity_is_unknown(mut self) -> Self {
        self.query = self.query.and_filter(Expr::is_null("quantity"));
        self
    }



    pub fn with_quantity_is_known(mut self) -> Self {
        self.query = self.query.and_filter(Expr::is_not_null("quantity"));
        self
    }


    pub fn order_by_quantity_asc(mut self) -> Self {
        self.query = self.query.order_asc("quantity");
        self
    }

    pub fn order_by_quantity_desc(mut self) -> Self {
        self.query = self.query.order_desc("quantity");
        self
    }

    pub fn order_by_quantity_asc_using_gbk(mut self) -> Self {
        self.query = self.query.order_gbk_asc("quantity");
        self
    }

    pub fn order_by_quantity_desc_using_gbk(mut self) -> Self {
        self.query = self.query.order_gbk_desc("quantity");
        self
    }


    pub fn select_zero_bound(mut self) -> Self {
        self.query = self.query.project("zero_bound");
        self
    }

    pub fn project_zero_bound(self) -> Self {
        self.select_zero_bound()
    }

    pub fn select_zero_bound_raw(self, raw_sql_segment: impl Into<String>) -> Self {
        self.select_zero_bound_unsafe_raw(UnsafeRawSqlSegment::trusted(raw_sql_segment))
    }

    pub fn select_zero_bound_unsafe_raw(mut self, raw_sql_segment: UnsafeRawSqlSegment) -> Self {
        self.query_options
            .raw_projections
            .push(RawProjection::new("zero_bound", raw_sql_segment));
        self
    }

    pub fn select_zero_bound_with_function(self, function: AggregateFunction) -> Self {
        self.select_zero_bound_as_with_function("zero_bound", function)
    }

    pub fn select_zero_bound_as_with_function(
        self,
        alias: impl Into<String>,
        function: AggregateFunction,
    ) -> Self {
        self.aggregate_with_function("zero_bound", alias, function)
    }

    pub fn group_by_zero_bound(self) -> Self {
        self.group_by("zero_bound")
    }

    pub fn group_by_zero_bound_as(self, alias: impl Into<String>) -> Self {
        let alias = alias.into();
        let mut request = self.group_by("zero_bound");
        request.query = request
            .query
            .project_expr(alias, Expr::column("zero_bound"));
        request
    }

    pub fn group_by_zero_bound_with_function(
        self,
        alias: impl Into<String>,
        function: AggregateFunction,
    ) -> Self {
        self.group_by("zero_bound")
            .aggregate_with_function("zero_bound", alias, function)
    }

    pub fn count_zero_bound(self) -> Self {
        self.count_zero_bound_as("zero_bound_count")
    }

    pub fn count_zero_bound_as(self, alias: impl Into<String>) -> Self {
        self.aggregate_count_field("zero_bound", alias)
    }

    pub fn sum_zero_bound(self) -> Self {
        self.sum_zero_bound_as("sum_zero_bound")
    }

    pub fn sum_zero_bound_as(self, alias: impl Into<String>) -> Self {
        self.aggregate_sum("zero_bound", alias)
    }

    pub fn avg_zero_bound(self) -> Self {
        self.avg_zero_bound_as("avg_zero_bound")
    }

    pub fn avg_zero_bound_as(self, alias: impl Into<String>) -> Self {
        self.aggregate_avg("zero_bound", alias)
    }

    pub fn min_zero_bound(self) -> Self {
        self.min_zero_bound_as("min_zero_bound")
    }

    pub fn min_zero_bound_as(self, alias: impl Into<String>) -> Self {
        self.aggregate_min("zero_bound", alias)
    }

    pub fn max_zero_bound(self) -> Self {
        self.max_zero_bound_as("max_zero_bound")
    }

    pub fn max_zero_bound_as(self, alias: impl Into<String>) -> Self {
        self.aggregate_max("zero_bound", alias)
    }

    pub fn standard_deviation_zero_bound(self) -> Self {
        self.standard_deviation_zero_bound_as("stdDev_zero_bound")
    }

    pub fn standard_deviation_zero_bound_as(self, alias: impl Into<String>) -> Self {
        self.aggregate_stddev("zero_bound", alias)
    }

    pub fn square_root_of_population_standard_deviation_zero_bound(self) -> Self {
        self.square_root_of_population_standard_deviation_zero_bound_as("stdDevPop_zero_bound")
    }

    pub fn square_root_of_population_standard_deviation_zero_bound_as(self, alias: impl Into<String>) -> Self {
        self.aggregate_stddev_pop("zero_bound", alias)
    }

    pub fn sample_variance_zero_bound(self) -> Self {
        self.sample_variance_zero_bound_as("varSamp_zero_bound")
    }

    pub fn sample_variance_zero_bound_as(self, alias: impl Into<String>) -> Self {
        self.aggregate_var_samp("zero_bound", alias)
    }

    pub fn sample_population_variance_zero_bound(self) -> Self {
        self.sample_population_variance_zero_bound_as("varPop_zero_bound")
    }

    pub fn sample_population_variance_zero_bound_as(self, alias: impl Into<String>) -> Self {
        self.aggregate_var_pop("zero_bound", alias)
    }

    pub fn unselect_zero_bound(mut self) -> Self {
        self.query.projection.retain(|field| field != "zero_bound");
        self.query_options.raw_projections.retain(|projection| projection.property_name != "zero_bound");
        self
    }


    pub fn with_zero_bound(
        mut self,
        operator: FieldOperator,
        values: impl IntoIterator<Item = impl Into<teaql_core::Value>>,
    ) -> Self {
        self.query = self.query.and_filter(field_operator_expr(
            "zero_bound",
            operator,
            values.into_iter().map(Into::into).collect(),
        ));
        self
    }

    pub fn create_zero_bound_criteria(
        operator: FieldOperator,
        values: impl IntoIterator<Item = impl Into<teaql_core::Value>>,
    ) -> Expr {
        field_operator_expr(
            "zero_bound",
            operator,
            values.into_iter().map(Into::into).collect(),
        )
    }

    pub fn with_zero_bound_is(mut self, value: impl Into<teaql_core::Value>) -> Self {
        self.query = self.query.and_filter(Expr::eq("zero_bound", value));
        self
    }



    pub fn with_zero_bound_is_not(mut self, value: impl Into<teaql_core::Value>) -> Self {
        self.query = self.query.and_filter(Expr::ne("zero_bound", value));
        self
    }

    pub fn with_zero_bound_greater_than(mut self, value: impl Into<teaql_core::Value>) -> Self {
        self.query = self.query.and_filter(Expr::gt("zero_bound", value));
        self
    }

    pub fn with_zero_bound_greater_than_or_equal_to(mut self, value: impl Into<teaql_core::Value>) -> Self {
        self.query = self.query.and_filter(Expr::gte("zero_bound", value));
        self
    }

    pub fn with_zero_bound_less_than(mut self, value: impl Into<teaql_core::Value>) -> Self {
        self.query = self.query.and_filter(Expr::lt("zero_bound", value));
        self
    }

    pub fn with_zero_bound_less_than_or_equal_to(mut self, value: impl Into<teaql_core::Value>) -> Self {
        self.query = self.query.and_filter(Expr::lte("zero_bound", value));
        self
    }

    pub fn with_zero_bound_between(
        mut self,
        lower: impl Into<teaql_core::Value>,
        upper: impl Into<teaql_core::Value>,
    ) -> Self {
        self.query = self.query.and_filter(Expr::between("zero_bound", lower, upper));
        self
    }

    pub fn with_zero_bound_between_range<T>(mut self, range: DateRange<T>) -> Self
    where
        T: Into<teaql_core::Value>,
    {
        self.query = self.query.and_filter(Expr::between(
            "zero_bound",
            range.start,
            range.end,
        ));
        self
    }

    pub fn with_zero_bound_in(
        mut self,
        values: impl IntoIterator<Item = impl Into<teaql_core::Value>>,
    ) -> Self {
        self.query = self.query.and_filter(Expr::in_list(
            "zero_bound",
            values.into_iter().map(Into::into),
        ));
        self
    }

    pub fn with_zero_bound_not_in(
        mut self,
        values: impl IntoIterator<Item = impl Into<teaql_core::Value>>,
    ) -> Self {
        self.query = self.query.and_filter(Expr::not_in_list(
            "zero_bound",
            values.into_iter().map(Into::into),
        ));
        self
    }

    pub fn with_zero_bound_before(mut self, value: impl Into<teaql_core::Value>) -> Self {
        self.query = self.query.and_filter(Expr::lt("zero_bound", value));
        self
    }

    pub fn with_zero_bound_after(mut self, value: impl Into<teaql_core::Value>) -> Self {
        self.query = self.query.and_filter(Expr::gt("zero_bound", value));
        self
    }

    pub fn with_zero_bound_is_unknown(mut self) -> Self {
        self.query = self.query.and_filter(Expr::is_null("zero_bound"));
        self
    }



    pub fn with_zero_bound_is_known(mut self) -> Self {
        self.query = self.query.and_filter(Expr::is_not_null("zero_bound"));
        self
    }


    pub fn order_by_zero_bound_asc(mut self) -> Self {
        self.query = self.query.order_asc("zero_bound");
        self
    }

    pub fn order_by_zero_bound_desc(mut self) -> Self {
        self.query = self.query.order_desc("zero_bound");
        self
    }

    pub fn order_by_zero_bound_asc_using_gbk(mut self) -> Self {
        self.query = self.query.order_gbk_asc("zero_bound");
        self
    }

    pub fn order_by_zero_bound_desc_using_gbk(mut self) -> Self {
        self.query = self.query.order_gbk_desc("zero_bound");
        self
    }


    pub fn select_negative_max(mut self) -> Self {
        self.query = self.query.project("negative_max");
        self
    }

    pub fn project_negative_max(self) -> Self {
        self.select_negative_max()
    }

    pub fn select_negative_max_raw(self, raw_sql_segment: impl Into<String>) -> Self {
        self.select_negative_max_unsafe_raw(UnsafeRawSqlSegment::trusted(raw_sql_segment))
    }

    pub fn select_negative_max_unsafe_raw(mut self, raw_sql_segment: UnsafeRawSqlSegment) -> Self {
        self.query_options
            .raw_projections
            .push(RawProjection::new("negative_max", raw_sql_segment));
        self
    }

    pub fn select_negative_max_with_function(self, function: AggregateFunction) -> Self {
        self.select_negative_max_as_with_function("negative_max", function)
    }

    pub fn select_negative_max_as_with_function(
        self,
        alias: impl Into<String>,
        function: AggregateFunction,
    ) -> Self {
        self.aggregate_with_function("negative_max", alias, function)
    }

    pub fn group_by_negative_max(self) -> Self {
        self.group_by("negative_max")
    }

    pub fn group_by_negative_max_as(self, alias: impl Into<String>) -> Self {
        let alias = alias.into();
        let mut request = self.group_by("negative_max");
        request.query = request
            .query
            .project_expr(alias, Expr::column("negative_max"));
        request
    }

    pub fn group_by_negative_max_with_function(
        self,
        alias: impl Into<String>,
        function: AggregateFunction,
    ) -> Self {
        self.group_by("negative_max")
            .aggregate_with_function("negative_max", alias, function)
    }

    pub fn count_negative_max(self) -> Self {
        self.count_negative_max_as("negative_max_count")
    }

    pub fn count_negative_max_as(self, alias: impl Into<String>) -> Self {
        self.aggregate_count_field("negative_max", alias)
    }

    pub fn sum_negative_max(self) -> Self {
        self.sum_negative_max_as("sum_negative_max")
    }

    pub fn sum_negative_max_as(self, alias: impl Into<String>) -> Self {
        self.aggregate_sum("negative_max", alias)
    }

    pub fn avg_negative_max(self) -> Self {
        self.avg_negative_max_as("avg_negative_max")
    }

    pub fn avg_negative_max_as(self, alias: impl Into<String>) -> Self {
        self.aggregate_avg("negative_max", alias)
    }

    pub fn min_negative_max(self) -> Self {
        self.min_negative_max_as("min_negative_max")
    }

    pub fn min_negative_max_as(self, alias: impl Into<String>) -> Self {
        self.aggregate_min("negative_max", alias)
    }

    pub fn max_negative_max(self) -> Self {
        self.max_negative_max_as("max_negative_max")
    }

    pub fn max_negative_max_as(self, alias: impl Into<String>) -> Self {
        self.aggregate_max("negative_max", alias)
    }

    pub fn standard_deviation_negative_max(self) -> Self {
        self.standard_deviation_negative_max_as("stdDev_negative_max")
    }

    pub fn standard_deviation_negative_max_as(self, alias: impl Into<String>) -> Self {
        self.aggregate_stddev("negative_max", alias)
    }

    pub fn square_root_of_population_standard_deviation_negative_max(self) -> Self {
        self.square_root_of_population_standard_deviation_negative_max_as("stdDevPop_negative_max")
    }

    pub fn square_root_of_population_standard_deviation_negative_max_as(self, alias: impl Into<String>) -> Self {
        self.aggregate_stddev_pop("negative_max", alias)
    }

    pub fn sample_variance_negative_max(self) -> Self {
        self.sample_variance_negative_max_as("varSamp_negative_max")
    }

    pub fn sample_variance_negative_max_as(self, alias: impl Into<String>) -> Self {
        self.aggregate_var_samp("negative_max", alias)
    }

    pub fn sample_population_variance_negative_max(self) -> Self {
        self.sample_population_variance_negative_max_as("varPop_negative_max")
    }

    pub fn sample_population_variance_negative_max_as(self, alias: impl Into<String>) -> Self {
        self.aggregate_var_pop("negative_max", alias)
    }

    pub fn unselect_negative_max(mut self) -> Self {
        self.query.projection.retain(|field| field != "negative_max");
        self.query_options.raw_projections.retain(|projection| projection.property_name != "negative_max");
        self
    }


    pub fn with_negative_max(
        mut self,
        operator: FieldOperator,
        values: impl IntoIterator<Item = impl Into<teaql_core::Value>>,
    ) -> Self {
        self.query = self.query.and_filter(field_operator_expr(
            "negative_max",
            operator,
            values.into_iter().map(Into::into).collect(),
        ));
        self
    }

    pub fn create_negative_max_criteria(
        operator: FieldOperator,
        values: impl IntoIterator<Item = impl Into<teaql_core::Value>>,
    ) -> Expr {
        field_operator_expr(
            "negative_max",
            operator,
            values.into_iter().map(Into::into).collect(),
        )
    }

    pub fn with_negative_max_is(mut self, value: impl Into<teaql_core::Value>) -> Self {
        self.query = self.query.and_filter(Expr::eq("negative_max", value));
        self
    }



    pub fn with_negative_max_is_not(mut self, value: impl Into<teaql_core::Value>) -> Self {
        self.query = self.query.and_filter(Expr::ne("negative_max", value));
        self
    }

    pub fn with_negative_max_greater_than(mut self, value: impl Into<teaql_core::Value>) -> Self {
        self.query = self.query.and_filter(Expr::gt("negative_max", value));
        self
    }

    pub fn with_negative_max_greater_than_or_equal_to(mut self, value: impl Into<teaql_core::Value>) -> Self {
        self.query = self.query.and_filter(Expr::gte("negative_max", value));
        self
    }

    pub fn with_negative_max_less_than(mut self, value: impl Into<teaql_core::Value>) -> Self {
        self.query = self.query.and_filter(Expr::lt("negative_max", value));
        self
    }

    pub fn with_negative_max_less_than_or_equal_to(mut self, value: impl Into<teaql_core::Value>) -> Self {
        self.query = self.query.and_filter(Expr::lte("negative_max", value));
        self
    }

    pub fn with_negative_max_between(
        mut self,
        lower: impl Into<teaql_core::Value>,
        upper: impl Into<teaql_core::Value>,
    ) -> Self {
        self.query = self.query.and_filter(Expr::between("negative_max", lower, upper));
        self
    }

    pub fn with_negative_max_between_range<T>(mut self, range: DateRange<T>) -> Self
    where
        T: Into<teaql_core::Value>,
    {
        self.query = self.query.and_filter(Expr::between(
            "negative_max",
            range.start,
            range.end,
        ));
        self
    }

    pub fn with_negative_max_in(
        mut self,
        values: impl IntoIterator<Item = impl Into<teaql_core::Value>>,
    ) -> Self {
        self.query = self.query.and_filter(Expr::in_list(
            "negative_max",
            values.into_iter().map(Into::into),
        ));
        self
    }

    pub fn with_negative_max_not_in(
        mut self,
        values: impl IntoIterator<Item = impl Into<teaql_core::Value>>,
    ) -> Self {
        self.query = self.query.and_filter(Expr::not_in_list(
            "negative_max",
            values.into_iter().map(Into::into),
        ));
        self
    }

    pub fn with_negative_max_before(mut self, value: impl Into<teaql_core::Value>) -> Self {
        self.query = self.query.and_filter(Expr::lt("negative_max", value));
        self
    }

    pub fn with_negative_max_after(mut self, value: impl Into<teaql_core::Value>) -> Self {
        self.query = self.query.and_filter(Expr::gt("negative_max", value));
        self
    }

    pub fn with_negative_max_is_unknown(mut self) -> Self {
        self.query = self.query.and_filter(Expr::is_null("negative_max"));
        self
    }



    pub fn with_negative_max_is_known(mut self) -> Self {
        self.query = self.query.and_filter(Expr::is_not_null("negative_max"));
        self
    }


    pub fn order_by_negative_max_asc(mut self) -> Self {
        self.query = self.query.order_asc("negative_max");
        self
    }

    pub fn order_by_negative_max_desc(mut self) -> Self {
        self.query = self.query.order_desc("negative_max");
        self
    }

    pub fn order_by_negative_max_asc_using_gbk(mut self) -> Self {
        self.query = self.query.order_gbk_asc("negative_max");
        self
    }

    pub fn order_by_negative_max_desc_using_gbk(mut self) -> Self {
        self.query = self.query.order_gbk_desc("negative_max");
        self
    }


    pub fn select_signed_max(mut self) -> Self {
        self.query = self.query.project("signed_max");
        self
    }

    pub fn project_signed_max(self) -> Self {
        self.select_signed_max()
    }

    pub fn select_signed_max_raw(self, raw_sql_segment: impl Into<String>) -> Self {
        self.select_signed_max_unsafe_raw(UnsafeRawSqlSegment::trusted(raw_sql_segment))
    }

    pub fn select_signed_max_unsafe_raw(mut self, raw_sql_segment: UnsafeRawSqlSegment) -> Self {
        self.query_options
            .raw_projections
            .push(RawProjection::new("signed_max", raw_sql_segment));
        self
    }

    pub fn select_signed_max_with_function(self, function: AggregateFunction) -> Self {
        self.select_signed_max_as_with_function("signed_max", function)
    }

    pub fn select_signed_max_as_with_function(
        self,
        alias: impl Into<String>,
        function: AggregateFunction,
    ) -> Self {
        self.aggregate_with_function("signed_max", alias, function)
    }

    pub fn group_by_signed_max(self) -> Self {
        self.group_by("signed_max")
    }

    pub fn group_by_signed_max_as(self, alias: impl Into<String>) -> Self {
        let alias = alias.into();
        let mut request = self.group_by("signed_max");
        request.query = request
            .query
            .project_expr(alias, Expr::column("signed_max"));
        request
    }

    pub fn group_by_signed_max_with_function(
        self,
        alias: impl Into<String>,
        function: AggregateFunction,
    ) -> Self {
        self.group_by("signed_max")
            .aggregate_with_function("signed_max", alias, function)
    }

    pub fn count_signed_max(self) -> Self {
        self.count_signed_max_as("signed_max_count")
    }

    pub fn count_signed_max_as(self, alias: impl Into<String>) -> Self {
        self.aggregate_count_field("signed_max", alias)
    }

    pub fn sum_signed_max(self) -> Self {
        self.sum_signed_max_as("sum_signed_max")
    }

    pub fn sum_signed_max_as(self, alias: impl Into<String>) -> Self {
        self.aggregate_sum("signed_max", alias)
    }

    pub fn avg_signed_max(self) -> Self {
        self.avg_signed_max_as("avg_signed_max")
    }

    pub fn avg_signed_max_as(self, alias: impl Into<String>) -> Self {
        self.aggregate_avg("signed_max", alias)
    }

    pub fn min_signed_max(self) -> Self {
        self.min_signed_max_as("min_signed_max")
    }

    pub fn min_signed_max_as(self, alias: impl Into<String>) -> Self {
        self.aggregate_min("signed_max", alias)
    }

    pub fn max_signed_max(self) -> Self {
        self.max_signed_max_as("max_signed_max")
    }

    pub fn max_signed_max_as(self, alias: impl Into<String>) -> Self {
        self.aggregate_max("signed_max", alias)
    }

    pub fn standard_deviation_signed_max(self) -> Self {
        self.standard_deviation_signed_max_as("stdDev_signed_max")
    }

    pub fn standard_deviation_signed_max_as(self, alias: impl Into<String>) -> Self {
        self.aggregate_stddev("signed_max", alias)
    }

    pub fn square_root_of_population_standard_deviation_signed_max(self) -> Self {
        self.square_root_of_population_standard_deviation_signed_max_as("stdDevPop_signed_max")
    }

    pub fn square_root_of_population_standard_deviation_signed_max_as(self, alias: impl Into<String>) -> Self {
        self.aggregate_stddev_pop("signed_max", alias)
    }

    pub fn sample_variance_signed_max(self) -> Self {
        self.sample_variance_signed_max_as("varSamp_signed_max")
    }

    pub fn sample_variance_signed_max_as(self, alias: impl Into<String>) -> Self {
        self.aggregate_var_samp("signed_max", alias)
    }

    pub fn sample_population_variance_signed_max(self) -> Self {
        self.sample_population_variance_signed_max_as("varPop_signed_max")
    }

    pub fn sample_population_variance_signed_max_as(self, alias: impl Into<String>) -> Self {
        self.aggregate_var_pop("signed_max", alias)
    }

    pub fn unselect_signed_max(mut self) -> Self {
        self.query.projection.retain(|field| field != "signed_max");
        self.query_options.raw_projections.retain(|projection| projection.property_name != "signed_max");
        self
    }


    pub fn with_signed_max(
        mut self,
        operator: FieldOperator,
        values: impl IntoIterator<Item = impl Into<teaql_core::Value>>,
    ) -> Self {
        self.query = self.query.and_filter(field_operator_expr(
            "signed_max",
            operator,
            values.into_iter().map(Into::into).collect(),
        ));
        self
    }

    pub fn create_signed_max_criteria(
        operator: FieldOperator,
        values: impl IntoIterator<Item = impl Into<teaql_core::Value>>,
    ) -> Expr {
        field_operator_expr(
            "signed_max",
            operator,
            values.into_iter().map(Into::into).collect(),
        )
    }

    pub fn with_signed_max_is(mut self, value: impl Into<teaql_core::Value>) -> Self {
        self.query = self.query.and_filter(Expr::eq("signed_max", value));
        self
    }



    pub fn with_signed_max_is_not(mut self, value: impl Into<teaql_core::Value>) -> Self {
        self.query = self.query.and_filter(Expr::ne("signed_max", value));
        self
    }

    pub fn with_signed_max_greater_than(mut self, value: impl Into<teaql_core::Value>) -> Self {
        self.query = self.query.and_filter(Expr::gt("signed_max", value));
        self
    }

    pub fn with_signed_max_greater_than_or_equal_to(mut self, value: impl Into<teaql_core::Value>) -> Self {
        self.query = self.query.and_filter(Expr::gte("signed_max", value));
        self
    }

    pub fn with_signed_max_less_than(mut self, value: impl Into<teaql_core::Value>) -> Self {
        self.query = self.query.and_filter(Expr::lt("signed_max", value));
        self
    }

    pub fn with_signed_max_less_than_or_equal_to(mut self, value: impl Into<teaql_core::Value>) -> Self {
        self.query = self.query.and_filter(Expr::lte("signed_max", value));
        self
    }

    pub fn with_signed_max_between(
        mut self,
        lower: impl Into<teaql_core::Value>,
        upper: impl Into<teaql_core::Value>,
    ) -> Self {
        self.query = self.query.and_filter(Expr::between("signed_max", lower, upper));
        self
    }

    pub fn with_signed_max_between_range<T>(mut self, range: DateRange<T>) -> Self
    where
        T: Into<teaql_core::Value>,
    {
        self.query = self.query.and_filter(Expr::between(
            "signed_max",
            range.start,
            range.end,
        ));
        self
    }

    pub fn with_signed_max_in(
        mut self,
        values: impl IntoIterator<Item = impl Into<teaql_core::Value>>,
    ) -> Self {
        self.query = self.query.and_filter(Expr::in_list(
            "signed_max",
            values.into_iter().map(Into::into),
        ));
        self
    }

    pub fn with_signed_max_not_in(
        mut self,
        values: impl IntoIterator<Item = impl Into<teaql_core::Value>>,
    ) -> Self {
        self.query = self.query.and_filter(Expr::not_in_list(
            "signed_max",
            values.into_iter().map(Into::into),
        ));
        self
    }

    pub fn with_signed_max_before(mut self, value: impl Into<teaql_core::Value>) -> Self {
        self.query = self.query.and_filter(Expr::lt("signed_max", value));
        self
    }

    pub fn with_signed_max_after(mut self, value: impl Into<teaql_core::Value>) -> Self {
        self.query = self.query.and_filter(Expr::gt("signed_max", value));
        self
    }

    pub fn with_signed_max_is_unknown(mut self) -> Self {
        self.query = self.query.and_filter(Expr::is_null("signed_max"));
        self
    }



    pub fn with_signed_max_is_known(mut self) -> Self {
        self.query = self.query.and_filter(Expr::is_not_null("signed_max"));
        self
    }


    pub fn order_by_signed_max_asc(mut self) -> Self {
        self.query = self.query.order_asc("signed_max");
        self
    }

    pub fn order_by_signed_max_desc(mut self) -> Self {
        self.query = self.query.order_desc("signed_max");
        self
    }

    pub fn order_by_signed_max_asc_using_gbk(mut self) -> Self {
        self.query = self.query.order_gbk_asc("signed_max");
        self
    }

    pub fn order_by_signed_max_desc_using_gbk(mut self) -> Self {
        self.query = self.query.order_gbk_desc("signed_max");
        self
    }


    pub fn select_wide_max(mut self) -> Self {
        self.query = self.query.project("wide_max");
        self
    }

    pub fn project_wide_max(self) -> Self {
        self.select_wide_max()
    }

    pub fn select_wide_max_raw(self, raw_sql_segment: impl Into<String>) -> Self {
        self.select_wide_max_unsafe_raw(UnsafeRawSqlSegment::trusted(raw_sql_segment))
    }

    pub fn select_wide_max_unsafe_raw(mut self, raw_sql_segment: UnsafeRawSqlSegment) -> Self {
        self.query_options
            .raw_projections
            .push(RawProjection::new("wide_max", raw_sql_segment));
        self
    }

    pub fn select_wide_max_with_function(self, function: AggregateFunction) -> Self {
        self.select_wide_max_as_with_function("wide_max", function)
    }

    pub fn select_wide_max_as_with_function(
        self,
        alias: impl Into<String>,
        function: AggregateFunction,
    ) -> Self {
        self.aggregate_with_function("wide_max", alias, function)
    }

    pub fn group_by_wide_max(self) -> Self {
        self.group_by("wide_max")
    }

    pub fn group_by_wide_max_as(self, alias: impl Into<String>) -> Self {
        let alias = alias.into();
        let mut request = self.group_by("wide_max");
        request.query = request
            .query
            .project_expr(alias, Expr::column("wide_max"));
        request
    }

    pub fn group_by_wide_max_with_function(
        self,
        alias: impl Into<String>,
        function: AggregateFunction,
    ) -> Self {
        self.group_by("wide_max")
            .aggregate_with_function("wide_max", alias, function)
    }

    pub fn count_wide_max(self) -> Self {
        self.count_wide_max_as("wide_max_count")
    }

    pub fn count_wide_max_as(self, alias: impl Into<String>) -> Self {
        self.aggregate_count_field("wide_max", alias)
    }

    pub fn sum_wide_max(self) -> Self {
        self.sum_wide_max_as("sum_wide_max")
    }

    pub fn sum_wide_max_as(self, alias: impl Into<String>) -> Self {
        self.aggregate_sum("wide_max", alias)
    }

    pub fn avg_wide_max(self) -> Self {
        self.avg_wide_max_as("avg_wide_max")
    }

    pub fn avg_wide_max_as(self, alias: impl Into<String>) -> Self {
        self.aggregate_avg("wide_max", alias)
    }

    pub fn min_wide_max(self) -> Self {
        self.min_wide_max_as("min_wide_max")
    }

    pub fn min_wide_max_as(self, alias: impl Into<String>) -> Self {
        self.aggregate_min("wide_max", alias)
    }

    pub fn max_wide_max(self) -> Self {
        self.max_wide_max_as("max_wide_max")
    }

    pub fn max_wide_max_as(self, alias: impl Into<String>) -> Self {
        self.aggregate_max("wide_max", alias)
    }

    pub fn standard_deviation_wide_max(self) -> Self {
        self.standard_deviation_wide_max_as("stdDev_wide_max")
    }

    pub fn standard_deviation_wide_max_as(self, alias: impl Into<String>) -> Self {
        self.aggregate_stddev("wide_max", alias)
    }

    pub fn square_root_of_population_standard_deviation_wide_max(self) -> Self {
        self.square_root_of_population_standard_deviation_wide_max_as("stdDevPop_wide_max")
    }

    pub fn square_root_of_population_standard_deviation_wide_max_as(self, alias: impl Into<String>) -> Self {
        self.aggregate_stddev_pop("wide_max", alias)
    }

    pub fn sample_variance_wide_max(self) -> Self {
        self.sample_variance_wide_max_as("varSamp_wide_max")
    }

    pub fn sample_variance_wide_max_as(self, alias: impl Into<String>) -> Self {
        self.aggregate_var_samp("wide_max", alias)
    }

    pub fn sample_population_variance_wide_max(self) -> Self {
        self.sample_population_variance_wide_max_as("varPop_wide_max")
    }

    pub fn sample_population_variance_wide_max_as(self, alias: impl Into<String>) -> Self {
        self.aggregate_var_pop("wide_max", alias)
    }

    pub fn unselect_wide_max(mut self) -> Self {
        self.query.projection.retain(|field| field != "wide_max");
        self.query_options.raw_projections.retain(|projection| projection.property_name != "wide_max");
        self
    }


    pub fn with_wide_max(
        mut self,
        operator: FieldOperator,
        values: impl IntoIterator<Item = impl Into<teaql_core::Value>>,
    ) -> Self {
        self.query = self.query.and_filter(field_operator_expr(
            "wide_max",
            operator,
            values.into_iter().map(Into::into).collect(),
        ));
        self
    }

    pub fn create_wide_max_criteria(
        operator: FieldOperator,
        values: impl IntoIterator<Item = impl Into<teaql_core::Value>>,
    ) -> Expr {
        field_operator_expr(
            "wide_max",
            operator,
            values.into_iter().map(Into::into).collect(),
        )
    }

    pub fn with_wide_max_is(mut self, value: impl Into<teaql_core::Value>) -> Self {
        self.query = self.query.and_filter(Expr::eq("wide_max", value));
        self
    }



    pub fn with_wide_max_is_not(mut self, value: impl Into<teaql_core::Value>) -> Self {
        self.query = self.query.and_filter(Expr::ne("wide_max", value));
        self
    }

    pub fn with_wide_max_greater_than(mut self, value: impl Into<teaql_core::Value>) -> Self {
        self.query = self.query.and_filter(Expr::gt("wide_max", value));
        self
    }

    pub fn with_wide_max_greater_than_or_equal_to(mut self, value: impl Into<teaql_core::Value>) -> Self {
        self.query = self.query.and_filter(Expr::gte("wide_max", value));
        self
    }

    pub fn with_wide_max_less_than(mut self, value: impl Into<teaql_core::Value>) -> Self {
        self.query = self.query.and_filter(Expr::lt("wide_max", value));
        self
    }

    pub fn with_wide_max_less_than_or_equal_to(mut self, value: impl Into<teaql_core::Value>) -> Self {
        self.query = self.query.and_filter(Expr::lte("wide_max", value));
        self
    }

    pub fn with_wide_max_between(
        mut self,
        lower: impl Into<teaql_core::Value>,
        upper: impl Into<teaql_core::Value>,
    ) -> Self {
        self.query = self.query.and_filter(Expr::between("wide_max", lower, upper));
        self
    }

    pub fn with_wide_max_between_range<T>(mut self, range: DateRange<T>) -> Self
    where
        T: Into<teaql_core::Value>,
    {
        self.query = self.query.and_filter(Expr::between(
            "wide_max",
            range.start,
            range.end,
        ));
        self
    }

    pub fn with_wide_max_in(
        mut self,
        values: impl IntoIterator<Item = impl Into<teaql_core::Value>>,
    ) -> Self {
        self.query = self.query.and_filter(Expr::in_list(
            "wide_max",
            values.into_iter().map(Into::into),
        ));
        self
    }

    pub fn with_wide_max_not_in(
        mut self,
        values: impl IntoIterator<Item = impl Into<teaql_core::Value>>,
    ) -> Self {
        self.query = self.query.and_filter(Expr::not_in_list(
            "wide_max",
            values.into_iter().map(Into::into),
        ));
        self
    }

    pub fn with_wide_max_before(mut self, value: impl Into<teaql_core::Value>) -> Self {
        self.query = self.query.and_filter(Expr::lt("wide_max", value));
        self
    }

    pub fn with_wide_max_after(mut self, value: impl Into<teaql_core::Value>) -> Self {
        self.query = self.query.and_filter(Expr::gt("wide_max", value));
        self
    }

    pub fn with_wide_max_is_unknown(mut self) -> Self {
        self.query = self.query.and_filter(Expr::is_null("wide_max"));
        self
    }



    pub fn with_wide_max_is_known(mut self) -> Self {
        self.query = self.query.and_filter(Expr::is_not_null("wide_max"));
        self
    }


    pub fn order_by_wide_max_asc(mut self) -> Self {
        self.query = self.query.order_asc("wide_max");
        self
    }

    pub fn order_by_wide_max_desc(mut self) -> Self {
        self.query = self.query.order_desc("wide_max");
        self
    }

    pub fn order_by_wide_max_asc_using_gbk(mut self) -> Self {
        self.query = self.query.order_gbk_asc("wide_max");
        self
    }

    pub fn order_by_wide_max_desc_using_gbk(mut self) -> Self {
        self.query = self.query.order_gbk_desc("wide_max");
        self
    }


    pub fn select_negative_bound(mut self) -> Self {
        self.query = self.query.project("negative_bound");
        self
    }

    pub fn project_negative_bound(self) -> Self {
        self.select_negative_bound()
    }

    pub fn select_negative_bound_raw(self, raw_sql_segment: impl Into<String>) -> Self {
        self.select_negative_bound_unsafe_raw(UnsafeRawSqlSegment::trusted(raw_sql_segment))
    }

    pub fn select_negative_bound_unsafe_raw(mut self, raw_sql_segment: UnsafeRawSqlSegment) -> Self {
        self.query_options
            .raw_projections
            .push(RawProjection::new("negative_bound", raw_sql_segment));
        self
    }

    pub fn group_by_negative_bound(self) -> Self {
        self.group_by("negative_bound")
    }

    pub fn group_by_negative_bound_as(self, alias: impl Into<String>) -> Self {
        let alias = alias.into();
        let mut request = self.group_by("negative_bound");
        request.query = request
            .query
            .project_expr(alias, Expr::column("negative_bound"));
        request
    }

    pub fn group_by_negative_bound_with_function(
        self,
        alias: impl Into<String>,
        function: AggregateFunction,
    ) -> Self {
        self.group_by("negative_bound")
            .aggregate_with_function("negative_bound", alias, function)
    }

    pub fn count_negative_bound(self) -> Self {
        self.count_negative_bound_as("negative_bound_count")
    }

    pub fn count_negative_bound_as(self, alias: impl Into<String>) -> Self {
        self.aggregate_count_field("negative_bound", alias)
    }

    pub fn sum_negative_bound(self) -> Self {
        self.sum_negative_bound_as("sum_negative_bound")
    }

    pub fn sum_negative_bound_as(self, alias: impl Into<String>) -> Self {
        self.aggregate_sum("negative_bound", alias)
    }

    pub fn avg_negative_bound(self) -> Self {
        self.avg_negative_bound_as("avg_negative_bound")
    }

    pub fn avg_negative_bound_as(self, alias: impl Into<String>) -> Self {
        self.aggregate_avg("negative_bound", alias)
    }

    pub fn min_negative_bound(self) -> Self {
        self.min_negative_bound_as("min_negative_bound")
    }

    pub fn min_negative_bound_as(self, alias: impl Into<String>) -> Self {
        self.aggregate_min("negative_bound", alias)
    }

    pub fn max_negative_bound(self) -> Self {
        self.max_negative_bound_as("max_negative_bound")
    }

    pub fn max_negative_bound_as(self, alias: impl Into<String>) -> Self {
        self.aggregate_max("negative_bound", alias)
    }

    pub fn unselect_negative_bound(mut self) -> Self {
        self.query.projection.retain(|field| field != "negative_bound");
        self.query_options.raw_projections.retain(|projection| projection.property_name != "negative_bound");
        self
    }


    pub fn with_negative_bound(
        mut self,
        operator: FieldOperator,
        values: impl IntoIterator<Item = impl Into<teaql_core::Value>>,
    ) -> Self {
        self.query = self.query.and_filter(field_operator_expr(
            "negative_bound",
            operator,
            values.into_iter().map(Into::into).collect(),
        ));
        self
    }

    pub fn create_negative_bound_criteria(
        operator: FieldOperator,
        values: impl IntoIterator<Item = impl Into<teaql_core::Value>>,
    ) -> Expr {
        field_operator_expr(
            "negative_bound",
            operator,
            values.into_iter().map(Into::into).collect(),
        )
    }

    pub fn with_negative_bound_is(mut self, value: impl Into<teaql_core::Value>) -> Self {
        self.query = self.query.and_filter(Expr::eq("negative_bound", value));
        self
    }



    pub fn with_negative_bound_is_not(mut self, value: impl Into<teaql_core::Value>) -> Self {
        self.query = self.query.and_filter(Expr::ne("negative_bound", value));
        self
    }

    pub fn with_negative_bound_greater_than(mut self, value: impl Into<teaql_core::Value>) -> Self {
        self.query = self.query.and_filter(Expr::gt("negative_bound", value));
        self
    }

    pub fn with_negative_bound_greater_than_or_equal_to(mut self, value: impl Into<teaql_core::Value>) -> Self {
        self.query = self.query.and_filter(Expr::gte("negative_bound", value));
        self
    }

    pub fn with_negative_bound_less_than(mut self, value: impl Into<teaql_core::Value>) -> Self {
        self.query = self.query.and_filter(Expr::lt("negative_bound", value));
        self
    }

    pub fn with_negative_bound_less_than_or_equal_to(mut self, value: impl Into<teaql_core::Value>) -> Self {
        self.query = self.query.and_filter(Expr::lte("negative_bound", value));
        self
    }

    pub fn with_negative_bound_between(
        mut self,
        lower: impl Into<teaql_core::Value>,
        upper: impl Into<teaql_core::Value>,
    ) -> Self {
        self.query = self.query.and_filter(Expr::between("negative_bound", lower, upper));
        self
    }

    pub fn with_negative_bound_between_range<T>(mut self, range: DateRange<T>) -> Self
    where
        T: Into<teaql_core::Value>,
    {
        self.query = self.query.and_filter(Expr::between(
            "negative_bound",
            range.start,
            range.end,
        ));
        self
    }

    pub fn with_negative_bound_in(
        mut self,
        values: impl IntoIterator<Item = impl Into<teaql_core::Value>>,
    ) -> Self {
        self.query = self.query.and_filter(Expr::in_list(
            "negative_bound",
            values.into_iter().map(Into::into),
        ));
        self
    }

    pub fn with_negative_bound_not_in(
        mut self,
        values: impl IntoIterator<Item = impl Into<teaql_core::Value>>,
    ) -> Self {
        self.query = self.query.and_filter(Expr::not_in_list(
            "negative_bound",
            values.into_iter().map(Into::into),
        ));
        self
    }

    pub fn with_negative_bound_containing(mut self, value: impl Into<String>) -> Self {
        self.query = self.query.and_filter(Expr::contain("negative_bound", value));
        self
    }

    pub fn with_negative_bound_not_containing(mut self, value: impl Into<String>) -> Self {
        self.query = self.query.and_filter(Expr::not_contain("negative_bound", value));
        self
    }

    pub fn with_negative_bound_starting_with(mut self, value: impl Into<String>) -> Self {
        self.query = self.query.and_filter(Expr::begin_with("negative_bound", value));
        self
    }

    pub fn with_negative_bound_not_starting_with(mut self, value: impl Into<String>) -> Self {
        self.query = self.query.and_filter(Expr::not_begin_with("negative_bound", value));
        self
    }

    pub fn with_negative_bound_ending_with(mut self, value: impl Into<String>) -> Self {
        self.query = self.query.and_filter(Expr::end_with("negative_bound", value));
        self
    }

    pub fn with_negative_bound_not_ending_with(mut self, value: impl Into<String>) -> Self {
        self.query = self.query.and_filter(Expr::not_end_with("negative_bound", value));
        self
    }

    pub fn with_negative_bound_sounding_like(mut self, value: impl Into<teaql_core::Value>) -> Self {
        self.query = self.query.and_filter(Expr::sound_like("negative_bound", value));
        self
    }
    pub fn with_negative_bound_before(mut self, value: impl Into<teaql_core::Value>) -> Self {
        self.query = self.query.and_filter(Expr::lt("negative_bound", value));
        self
    }

    pub fn with_negative_bound_after(mut self, value: impl Into<teaql_core::Value>) -> Self {
        self.query = self.query.and_filter(Expr::gt("negative_bound", value));
        self
    }

    pub fn with_negative_bound_is_unknown(mut self) -> Self {
        self.query = self.query.and_filter(Expr::is_null("negative_bound"));
        self
    }



    pub fn with_negative_bound_is_known(mut self) -> Self {
        self.query = self.query.and_filter(Expr::is_not_null("negative_bound"));
        self
    }


    pub fn order_by_negative_bound_asc(mut self) -> Self {
        self.query = self.query.order_asc("negative_bound");
        self
    }

    pub fn order_by_negative_bound_desc(mut self) -> Self {
        self.query = self.query.order_desc("negative_bound");
        self
    }

    pub fn order_by_negative_bound_asc_using_gbk(mut self) -> Self {
        self.query = self.query.order_gbk_asc("negative_bound");
        self
    }

    pub fn order_by_negative_bound_desc_using_gbk(mut self) -> Self {
        self.query = self.query.order_gbk_desc("negative_bound");
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


    pub fn select_optional_count(mut self) -> Self {
        self.query = self.query.project("optional_count");
        self
    }

    pub fn project_optional_count(self) -> Self {
        self.select_optional_count()
    }

    pub fn select_optional_count_raw(self, raw_sql_segment: impl Into<String>) -> Self {
        self.select_optional_count_unsafe_raw(UnsafeRawSqlSegment::trusted(raw_sql_segment))
    }

    pub fn select_optional_count_unsafe_raw(mut self, raw_sql_segment: UnsafeRawSqlSegment) -> Self {
        self.query_options
            .raw_projections
            .push(RawProjection::new("optional_count", raw_sql_segment));
        self
    }

    pub fn select_optional_count_with_function(self, function: AggregateFunction) -> Self {
        self.select_optional_count_as_with_function("optional_count", function)
    }

    pub fn select_optional_count_as_with_function(
        self,
        alias: impl Into<String>,
        function: AggregateFunction,
    ) -> Self {
        self.aggregate_with_function("optional_count", alias, function)
    }

    pub fn group_by_optional_count(self) -> Self {
        self.group_by("optional_count")
    }

    pub fn group_by_optional_count_as(self, alias: impl Into<String>) -> Self {
        let alias = alias.into();
        let mut request = self.group_by("optional_count");
        request.query = request
            .query
            .project_expr(alias, Expr::column("optional_count"));
        request
    }

    pub fn group_by_optional_count_with_function(
        self,
        alias: impl Into<String>,
        function: AggregateFunction,
    ) -> Self {
        self.group_by("optional_count")
            .aggregate_with_function("optional_count", alias, function)
    }

    pub fn count_optional_count(self) -> Self {
        self.count_optional_count_as("optional_count_count")
    }

    pub fn count_optional_count_as(self, alias: impl Into<String>) -> Self {
        self.aggregate_count_field("optional_count", alias)
    }

    pub fn sum_optional_count(self) -> Self {
        self.sum_optional_count_as("sum_optional_count")
    }

    pub fn sum_optional_count_as(self, alias: impl Into<String>) -> Self {
        self.aggregate_sum("optional_count", alias)
    }

    pub fn avg_optional_count(self) -> Self {
        self.avg_optional_count_as("avg_optional_count")
    }

    pub fn avg_optional_count_as(self, alias: impl Into<String>) -> Self {
        self.aggregate_avg("optional_count", alias)
    }

    pub fn min_optional_count(self) -> Self {
        self.min_optional_count_as("min_optional_count")
    }

    pub fn min_optional_count_as(self, alias: impl Into<String>) -> Self {
        self.aggregate_min("optional_count", alias)
    }

    pub fn max_optional_count(self) -> Self {
        self.max_optional_count_as("max_optional_count")
    }

    pub fn max_optional_count_as(self, alias: impl Into<String>) -> Self {
        self.aggregate_max("optional_count", alias)
    }

    pub fn standard_deviation_optional_count(self) -> Self {
        self.standard_deviation_optional_count_as("stdDev_optional_count")
    }

    pub fn standard_deviation_optional_count_as(self, alias: impl Into<String>) -> Self {
        self.aggregate_stddev("optional_count", alias)
    }

    pub fn square_root_of_population_standard_deviation_optional_count(self) -> Self {
        self.square_root_of_population_standard_deviation_optional_count_as("stdDevPop_optional_count")
    }

    pub fn square_root_of_population_standard_deviation_optional_count_as(self, alias: impl Into<String>) -> Self {
        self.aggregate_stddev_pop("optional_count", alias)
    }

    pub fn sample_variance_optional_count(self) -> Self {
        self.sample_variance_optional_count_as("varSamp_optional_count")
    }

    pub fn sample_variance_optional_count_as(self, alias: impl Into<String>) -> Self {
        self.aggregate_var_samp("optional_count", alias)
    }

    pub fn sample_population_variance_optional_count(self) -> Self {
        self.sample_population_variance_optional_count_as("varPop_optional_count")
    }

    pub fn sample_population_variance_optional_count_as(self, alias: impl Into<String>) -> Self {
        self.aggregate_var_pop("optional_count", alias)
    }

    pub fn unselect_optional_count(mut self) -> Self {
        self.query.projection.retain(|field| field != "optional_count");
        self.query_options.raw_projections.retain(|projection| projection.property_name != "optional_count");
        self
    }


    pub fn with_optional_count(
        mut self,
        operator: FieldOperator,
        values: impl IntoIterator<Item = impl Into<teaql_core::Value>>,
    ) -> Self {
        self.query = self.query.and_filter(field_operator_expr(
            "optional_count",
            operator,
            values.into_iter().map(Into::into).collect(),
        ));
        self
    }

    pub fn create_optional_count_criteria(
        operator: FieldOperator,
        values: impl IntoIterator<Item = impl Into<teaql_core::Value>>,
    ) -> Expr {
        field_operator_expr(
            "optional_count",
            operator,
            values.into_iter().map(Into::into).collect(),
        )
    }

    pub fn with_optional_count_is(mut self, value: impl Into<teaql_core::Value>) -> Self {
        self.query = self.query.and_filter(Expr::eq("optional_count", value));
        self
    }



    pub fn with_optional_count_is_not(mut self, value: impl Into<teaql_core::Value>) -> Self {
        self.query = self.query.and_filter(Expr::ne("optional_count", value));
        self
    }

    pub fn with_optional_count_greater_than(mut self, value: impl Into<teaql_core::Value>) -> Self {
        self.query = self.query.and_filter(Expr::gt("optional_count", value));
        self
    }

    pub fn with_optional_count_greater_than_or_equal_to(mut self, value: impl Into<teaql_core::Value>) -> Self {
        self.query = self.query.and_filter(Expr::gte("optional_count", value));
        self
    }

    pub fn with_optional_count_less_than(mut self, value: impl Into<teaql_core::Value>) -> Self {
        self.query = self.query.and_filter(Expr::lt("optional_count", value));
        self
    }

    pub fn with_optional_count_less_than_or_equal_to(mut self, value: impl Into<teaql_core::Value>) -> Self {
        self.query = self.query.and_filter(Expr::lte("optional_count", value));
        self
    }

    pub fn with_optional_count_between(
        mut self,
        lower: impl Into<teaql_core::Value>,
        upper: impl Into<teaql_core::Value>,
    ) -> Self {
        self.query = self.query.and_filter(Expr::between("optional_count", lower, upper));
        self
    }

    pub fn with_optional_count_between_range<T>(mut self, range: DateRange<T>) -> Self
    where
        T: Into<teaql_core::Value>,
    {
        self.query = self.query.and_filter(Expr::between(
            "optional_count",
            range.start,
            range.end,
        ));
        self
    }

    pub fn with_optional_count_in(
        mut self,
        values: impl IntoIterator<Item = impl Into<teaql_core::Value>>,
    ) -> Self {
        self.query = self.query.and_filter(Expr::in_list(
            "optional_count",
            values.into_iter().map(Into::into),
        ));
        self
    }

    pub fn with_optional_count_not_in(
        mut self,
        values: impl IntoIterator<Item = impl Into<teaql_core::Value>>,
    ) -> Self {
        self.query = self.query.and_filter(Expr::not_in_list(
            "optional_count",
            values.into_iter().map(Into::into),
        ));
        self
    }

    pub fn with_optional_count_before(mut self, value: impl Into<teaql_core::Value>) -> Self {
        self.query = self.query.and_filter(Expr::lt("optional_count", value));
        self
    }

    pub fn with_optional_count_after(mut self, value: impl Into<teaql_core::Value>) -> Self {
        self.query = self.query.and_filter(Expr::gt("optional_count", value));
        self
    }

    pub fn with_optional_count_is_unknown(mut self) -> Self {
        self.query = self.query.and_filter(Expr::is_null("optional_count"));
        self
    }



    pub fn with_optional_count_is_known(mut self) -> Self {
        self.query = self.query.and_filter(Expr::is_not_null("optional_count"));
        self
    }


    pub fn order_by_optional_count_asc(mut self) -> Self {
        self.query = self.query.order_asc("optional_count");
        self
    }

    pub fn order_by_optional_count_desc(mut self) -> Self {
        self.query = self.query.order_desc("optional_count");
        self
    }

    pub fn order_by_optional_count_asc_using_gbk(mut self) -> Self {
        self.query = self.query.order_gbk_asc("optional_count");
        self
    }

    pub fn order_by_optional_count_desc_using_gbk(mut self) -> Self {
        self.query = self.query.order_gbk_desc("optional_count");
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


    pub fn select_disabled(mut self) -> Self {
        self.query = self.query.project("disabled");
        self
    }

    pub fn project_disabled(self) -> Self {
        self.select_disabled()
    }

    pub fn select_disabled_raw(self, raw_sql_segment: impl Into<String>) -> Self {
        self.select_disabled_unsafe_raw(UnsafeRawSqlSegment::trusted(raw_sql_segment))
    }

    pub fn select_disabled_unsafe_raw(mut self, raw_sql_segment: UnsafeRawSqlSegment) -> Self {
        self.query_options
            .raw_projections
            .push(RawProjection::new("disabled", raw_sql_segment));
        self
    }

    pub fn group_by_disabled(self) -> Self {
        self.group_by("disabled")
    }

    pub fn group_by_disabled_as(self, alias: impl Into<String>) -> Self {
        let alias = alias.into();
        let mut request = self.group_by("disabled");
        request.query = request
            .query
            .project_expr(alias, Expr::column("disabled"));
        request
    }

    pub fn group_by_disabled_with_function(
        self,
        alias: impl Into<String>,
        function: AggregateFunction,
    ) -> Self {
        self.group_by("disabled")
            .aggregate_with_function("disabled", alias, function)
    }

    pub fn count_disabled(self) -> Self {
        self.count_disabled_as("disabled_count")
    }

    pub fn count_disabled_as(self, alias: impl Into<String>) -> Self {
        self.aggregate_count_field("disabled", alias)
    }

    pub fn sum_disabled(self) -> Self {
        self.sum_disabled_as("sum_disabled")
    }

    pub fn sum_disabled_as(self, alias: impl Into<String>) -> Self {
        self.aggregate_sum("disabled", alias)
    }

    pub fn avg_disabled(self) -> Self {
        self.avg_disabled_as("avg_disabled")
    }

    pub fn avg_disabled_as(self, alias: impl Into<String>) -> Self {
        self.aggregate_avg("disabled", alias)
    }

    pub fn min_disabled(self) -> Self {
        self.min_disabled_as("min_disabled")
    }

    pub fn min_disabled_as(self, alias: impl Into<String>) -> Self {
        self.aggregate_min("disabled", alias)
    }

    pub fn max_disabled(self) -> Self {
        self.max_disabled_as("max_disabled")
    }

    pub fn max_disabled_as(self, alias: impl Into<String>) -> Self {
        self.aggregate_max("disabled", alias)
    }

    pub fn unselect_disabled(mut self) -> Self {
        self.query.projection.retain(|field| field != "disabled");
        self.query_options.raw_projections.retain(|projection| projection.property_name != "disabled");
        self
    }

    pub fn which_are_disabled(mut self) -> Self {
        self.query = self.query.and_filter(Expr::eq("disabled", true));
        self
    }

    pub fn which_are_not_disabled(mut self) -> Self {
        self.query = self.query.and_filter(Expr::eq("disabled", false));
        self
    }

    pub fn with_disabled_is_unknown(mut self) -> Self {
        self.query = self.query.and_filter(Expr::is_null("disabled"));
        self
    }

    pub fn with_disabled_is_known(mut self) -> Self {
        self.query = self.query.and_filter(Expr::is_not_null("disabled"));
        self
    }
    pub fn order_by_disabled_asc(mut self) -> Self {
        self.query = self.query.order_asc("disabled");
        self
    }

    pub fn order_by_disabled_desc(mut self) -> Self {
        self.query = self.query.order_desc("disabled");
        self
    }

    pub fn order_by_disabled_asc_using_gbk(mut self) -> Self {
        self.query = self.query.order_gbk_asc("disabled");
        self
    }

    pub fn order_by_disabled_desc_using_gbk(mut self) -> Self {
        self.query = self.query.order_gbk_desc("disabled");
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


    pub fn select_recorded_at(mut self) -> Self {
        self.query = self.query.project("recorded_at");
        self
    }

    pub fn project_recorded_at(self) -> Self {
        self.select_recorded_at()
    }

    pub fn select_recorded_at_raw(self, raw_sql_segment: impl Into<String>) -> Self {
        self.select_recorded_at_unsafe_raw(UnsafeRawSqlSegment::trusted(raw_sql_segment))
    }

    pub fn select_recorded_at_unsafe_raw(mut self, raw_sql_segment: UnsafeRawSqlSegment) -> Self {
        self.query_options
            .raw_projections
            .push(RawProjection::new("recorded_at", raw_sql_segment));
        self
    }

    pub fn group_by_recorded_at(self) -> Self {
        self.group_by("recorded_at")
    }

    pub fn group_by_recorded_at_as(self, alias: impl Into<String>) -> Self {
        let alias = alias.into();
        let mut request = self.group_by("recorded_at");
        request.query = request
            .query
            .project_expr(alias, Expr::column("recorded_at"));
        request
    }

    pub fn group_by_recorded_at_with_function(
        self,
        alias: impl Into<String>,
        function: AggregateFunction,
    ) -> Self {
        self.group_by("recorded_at")
            .aggregate_with_function("recorded_at", alias, function)
    }

    pub fn count_recorded_at(self) -> Self {
        self.count_recorded_at_as("recorded_at_count")
    }

    pub fn count_recorded_at_as(self, alias: impl Into<String>) -> Self {
        self.aggregate_count_field("recorded_at", alias)
    }

    pub fn sum_recorded_at(self) -> Self {
        self.sum_recorded_at_as("sum_recorded_at")
    }

    pub fn sum_recorded_at_as(self, alias: impl Into<String>) -> Self {
        self.aggregate_sum("recorded_at", alias)
    }

    pub fn avg_recorded_at(self) -> Self {
        self.avg_recorded_at_as("avg_recorded_at")
    }

    pub fn avg_recorded_at_as(self, alias: impl Into<String>) -> Self {
        self.aggregate_avg("recorded_at", alias)
    }

    pub fn min_recorded_at(self) -> Self {
        self.min_recorded_at_as("min_recorded_at")
    }

    pub fn min_recorded_at_as(self, alias: impl Into<String>) -> Self {
        self.aggregate_min("recorded_at", alias)
    }

    pub fn max_recorded_at(self) -> Self {
        self.max_recorded_at_as("max_recorded_at")
    }

    pub fn max_recorded_at_as(self, alias: impl Into<String>) -> Self {
        self.aggregate_max("recorded_at", alias)
    }

    pub fn unselect_recorded_at(mut self) -> Self {
        self.query.projection.retain(|field| field != "recorded_at");
        self.query_options.raw_projections.retain(|projection| projection.property_name != "recorded_at");
        self
    }


    pub fn with_recorded_at(
        mut self,
        operator: FieldOperator,
        values: impl IntoIterator<Item = impl Into<teaql_core::Value>>,
    ) -> Self {
        self.query = self.query.and_filter(field_operator_expr(
            "recorded_at",
            operator,
            values.into_iter().map(Into::into).collect(),
        ));
        self
    }

    pub fn create_recorded_at_criteria(
        operator: FieldOperator,
        values: impl IntoIterator<Item = impl Into<teaql_core::Value>>,
    ) -> Expr {
        field_operator_expr(
            "recorded_at",
            operator,
            values.into_iter().map(Into::into).collect(),
        )
    }

    pub fn with_recorded_at_is(mut self, value: impl Into<teaql_core::Value>) -> Self {
        self.query = self.query.and_filter(Expr::eq("recorded_at", value));
        self
    }



    pub fn with_recorded_at_is_not(mut self, value: impl Into<teaql_core::Value>) -> Self {
        self.query = self.query.and_filter(Expr::ne("recorded_at", value));
        self
    }

    pub fn with_recorded_at_greater_than(mut self, value: impl Into<teaql_core::Value>) -> Self {
        self.query = self.query.and_filter(Expr::gt("recorded_at", value));
        self
    }

    pub fn with_recorded_at_greater_than_or_equal_to(mut self, value: impl Into<teaql_core::Value>) -> Self {
        self.query = self.query.and_filter(Expr::gte("recorded_at", value));
        self
    }

    pub fn with_recorded_at_less_than(mut self, value: impl Into<teaql_core::Value>) -> Self {
        self.query = self.query.and_filter(Expr::lt("recorded_at", value));
        self
    }

    pub fn with_recorded_at_less_than_or_equal_to(mut self, value: impl Into<teaql_core::Value>) -> Self {
        self.query = self.query.and_filter(Expr::lte("recorded_at", value));
        self
    }

    pub fn with_recorded_at_between(
        mut self,
        lower: impl Into<teaql_core::Value>,
        upper: impl Into<teaql_core::Value>,
    ) -> Self {
        self.query = self.query.and_filter(Expr::between("recorded_at", lower, upper));
        self
    }

    pub fn with_recorded_at_between_range<T>(mut self, range: DateRange<T>) -> Self
    where
        T: Into<teaql_core::Value>,
    {
        self.query = self.query.and_filter(Expr::between(
            "recorded_at",
            range.start,
            range.end,
        ));
        self
    }

    pub fn with_recorded_at_in(
        mut self,
        values: impl IntoIterator<Item = impl Into<teaql_core::Value>>,
    ) -> Self {
        self.query = self.query.and_filter(Expr::in_list(
            "recorded_at",
            values.into_iter().map(Into::into),
        ));
        self
    }

    pub fn with_recorded_at_not_in(
        mut self,
        values: impl IntoIterator<Item = impl Into<teaql_core::Value>>,
    ) -> Self {
        self.query = self.query.and_filter(Expr::not_in_list(
            "recorded_at",
            values.into_iter().map(Into::into),
        ));
        self
    }

    pub fn with_recorded_at_before(mut self, value: impl Into<teaql_core::Value>) -> Self {
        self.query = self.query.and_filter(Expr::lt("recorded_at", value));
        self
    }

    pub fn with_recorded_at_after(mut self, value: impl Into<teaql_core::Value>) -> Self {
        self.query = self.query.and_filter(Expr::gt("recorded_at", value));
        self
    }

    pub fn with_recorded_at_is_unknown(mut self) -> Self {
        self.query = self.query.and_filter(Expr::is_null("recorded_at"));
        self
    }



    pub fn with_recorded_at_is_known(mut self) -> Self {
        self.query = self.query.and_filter(Expr::is_not_null("recorded_at"));
        self
    }


    pub fn order_by_recorded_at_asc(mut self) -> Self {
        self.query = self.query.order_asc("recorded_at");
        self
    }

    pub fn order_by_recorded_at_desc(mut self) -> Self {
        self.query = self.query.order_desc("recorded_at");
        self
    }

    pub fn order_by_recorded_at_asc_using_gbk(mut self) -> Self {
        self.query = self.query.order_gbk_asc("recorded_at");
        self
    }

    pub fn order_by_recorded_at_desc_using_gbk(mut self) -> Self {
        self.query = self.query.order_gbk_desc("recorded_at");
        self
    }


    pub fn select_optional_recorded_at(mut self) -> Self {
        self.query = self.query.project("optional_recorded_at");
        self
    }

    pub fn project_optional_recorded_at(self) -> Self {
        self.select_optional_recorded_at()
    }

    pub fn select_optional_recorded_at_raw(self, raw_sql_segment: impl Into<String>) -> Self {
        self.select_optional_recorded_at_unsafe_raw(UnsafeRawSqlSegment::trusted(raw_sql_segment))
    }

    pub fn select_optional_recorded_at_unsafe_raw(mut self, raw_sql_segment: UnsafeRawSqlSegment) -> Self {
        self.query_options
            .raw_projections
            .push(RawProjection::new("optional_recorded_at", raw_sql_segment));
        self
    }

    pub fn group_by_optional_recorded_at(self) -> Self {
        self.group_by("optional_recorded_at")
    }

    pub fn group_by_optional_recorded_at_as(self, alias: impl Into<String>) -> Self {
        let alias = alias.into();
        let mut request = self.group_by("optional_recorded_at");
        request.query = request
            .query
            .project_expr(alias, Expr::column("optional_recorded_at"));
        request
    }

    pub fn group_by_optional_recorded_at_with_function(
        self,
        alias: impl Into<String>,
        function: AggregateFunction,
    ) -> Self {
        self.group_by("optional_recorded_at")
            .aggregate_with_function("optional_recorded_at", alias, function)
    }

    pub fn count_optional_recorded_at(self) -> Self {
        self.count_optional_recorded_at_as("optional_recorded_at_count")
    }

    pub fn count_optional_recorded_at_as(self, alias: impl Into<String>) -> Self {
        self.aggregate_count_field("optional_recorded_at", alias)
    }

    pub fn sum_optional_recorded_at(self) -> Self {
        self.sum_optional_recorded_at_as("sum_optional_recorded_at")
    }

    pub fn sum_optional_recorded_at_as(self, alias: impl Into<String>) -> Self {
        self.aggregate_sum("optional_recorded_at", alias)
    }

    pub fn avg_optional_recorded_at(self) -> Self {
        self.avg_optional_recorded_at_as("avg_optional_recorded_at")
    }

    pub fn avg_optional_recorded_at_as(self, alias: impl Into<String>) -> Self {
        self.aggregate_avg("optional_recorded_at", alias)
    }

    pub fn min_optional_recorded_at(self) -> Self {
        self.min_optional_recorded_at_as("min_optional_recorded_at")
    }

    pub fn min_optional_recorded_at_as(self, alias: impl Into<String>) -> Self {
        self.aggregate_min("optional_recorded_at", alias)
    }

    pub fn max_optional_recorded_at(self) -> Self {
        self.max_optional_recorded_at_as("max_optional_recorded_at")
    }

    pub fn max_optional_recorded_at_as(self, alias: impl Into<String>) -> Self {
        self.aggregate_max("optional_recorded_at", alias)
    }

    pub fn unselect_optional_recorded_at(mut self) -> Self {
        self.query.projection.retain(|field| field != "optional_recorded_at");
        self.query_options.raw_projections.retain(|projection| projection.property_name != "optional_recorded_at");
        self
    }


    pub fn with_optional_recorded_at(
        mut self,
        operator: FieldOperator,
        values: impl IntoIterator<Item = impl Into<teaql_core::Value>>,
    ) -> Self {
        self.query = self.query.and_filter(field_operator_expr(
            "optional_recorded_at",
            operator,
            values.into_iter().map(Into::into).collect(),
        ));
        self
    }

    pub fn create_optional_recorded_at_criteria(
        operator: FieldOperator,
        values: impl IntoIterator<Item = impl Into<teaql_core::Value>>,
    ) -> Expr {
        field_operator_expr(
            "optional_recorded_at",
            operator,
            values.into_iter().map(Into::into).collect(),
        )
    }

    pub fn with_optional_recorded_at_is(mut self, value: impl Into<teaql_core::Value>) -> Self {
        self.query = self.query.and_filter(Expr::eq("optional_recorded_at", value));
        self
    }



    pub fn with_optional_recorded_at_is_not(mut self, value: impl Into<teaql_core::Value>) -> Self {
        self.query = self.query.and_filter(Expr::ne("optional_recorded_at", value));
        self
    }

    pub fn with_optional_recorded_at_greater_than(mut self, value: impl Into<teaql_core::Value>) -> Self {
        self.query = self.query.and_filter(Expr::gt("optional_recorded_at", value));
        self
    }

    pub fn with_optional_recorded_at_greater_than_or_equal_to(mut self, value: impl Into<teaql_core::Value>) -> Self {
        self.query = self.query.and_filter(Expr::gte("optional_recorded_at", value));
        self
    }

    pub fn with_optional_recorded_at_less_than(mut self, value: impl Into<teaql_core::Value>) -> Self {
        self.query = self.query.and_filter(Expr::lt("optional_recorded_at", value));
        self
    }

    pub fn with_optional_recorded_at_less_than_or_equal_to(mut self, value: impl Into<teaql_core::Value>) -> Self {
        self.query = self.query.and_filter(Expr::lte("optional_recorded_at", value));
        self
    }

    pub fn with_optional_recorded_at_between(
        mut self,
        lower: impl Into<teaql_core::Value>,
        upper: impl Into<teaql_core::Value>,
    ) -> Self {
        self.query = self.query.and_filter(Expr::between("optional_recorded_at", lower, upper));
        self
    }

    pub fn with_optional_recorded_at_between_range<T>(mut self, range: DateRange<T>) -> Self
    where
        T: Into<teaql_core::Value>,
    {
        self.query = self.query.and_filter(Expr::between(
            "optional_recorded_at",
            range.start,
            range.end,
        ));
        self
    }

    pub fn with_optional_recorded_at_in(
        mut self,
        values: impl IntoIterator<Item = impl Into<teaql_core::Value>>,
    ) -> Self {
        self.query = self.query.and_filter(Expr::in_list(
            "optional_recorded_at",
            values.into_iter().map(Into::into),
        ));
        self
    }

    pub fn with_optional_recorded_at_not_in(
        mut self,
        values: impl IntoIterator<Item = impl Into<teaql_core::Value>>,
    ) -> Self {
        self.query = self.query.and_filter(Expr::not_in_list(
            "optional_recorded_at",
            values.into_iter().map(Into::into),
        ));
        self
    }

    pub fn with_optional_recorded_at_before(mut self, value: impl Into<teaql_core::Value>) -> Self {
        self.query = self.query.and_filter(Expr::lt("optional_recorded_at", value));
        self
    }

    pub fn with_optional_recorded_at_after(mut self, value: impl Into<teaql_core::Value>) -> Self {
        self.query = self.query.and_filter(Expr::gt("optional_recorded_at", value));
        self
    }

    pub fn with_optional_recorded_at_is_unknown(mut self) -> Self {
        self.query = self.query.and_filter(Expr::is_null("optional_recorded_at"));
        self
    }



    pub fn with_optional_recorded_at_is_known(mut self) -> Self {
        self.query = self.query.and_filter(Expr::is_not_null("optional_recorded_at"));
        self
    }


    pub fn order_by_optional_recorded_at_asc(mut self) -> Self {
        self.query = self.query.order_asc("optional_recorded_at");
        self
    }

    pub fn order_by_optional_recorded_at_desc(mut self) -> Self {
        self.query = self.query.order_desc("optional_recorded_at");
        self
    }

    pub fn order_by_optional_recorded_at_asc_using_gbk(mut self) -> Self {
        self.query = self.query.order_gbk_asc("optional_recorded_at");
        self
    }

    pub fn order_by_optional_recorded_at_desc_using_gbk(mut self) -> Self {
        self.query = self.query.order_gbk_desc("optional_recorded_at");
        self
    }


    pub fn select_clock_text(mut self) -> Self {
        self.query = self.query.project("clock_text");
        self
    }

    pub fn project_clock_text(self) -> Self {
        self.select_clock_text()
    }

    pub fn select_clock_text_raw(self, raw_sql_segment: impl Into<String>) -> Self {
        self.select_clock_text_unsafe_raw(UnsafeRawSqlSegment::trusted(raw_sql_segment))
    }

    pub fn select_clock_text_unsafe_raw(mut self, raw_sql_segment: UnsafeRawSqlSegment) -> Self {
        self.query_options
            .raw_projections
            .push(RawProjection::new("clock_text", raw_sql_segment));
        self
    }

    pub fn group_by_clock_text(self) -> Self {
        self.group_by("clock_text")
    }

    pub fn group_by_clock_text_as(self, alias: impl Into<String>) -> Self {
        let alias = alias.into();
        let mut request = self.group_by("clock_text");
        request.query = request
            .query
            .project_expr(alias, Expr::column("clock_text"));
        request
    }

    pub fn group_by_clock_text_with_function(
        self,
        alias: impl Into<String>,
        function: AggregateFunction,
    ) -> Self {
        self.group_by("clock_text")
            .aggregate_with_function("clock_text", alias, function)
    }

    pub fn count_clock_text(self) -> Self {
        self.count_clock_text_as("clock_text_count")
    }

    pub fn count_clock_text_as(self, alias: impl Into<String>) -> Self {
        self.aggregate_count_field("clock_text", alias)
    }

    pub fn sum_clock_text(self) -> Self {
        self.sum_clock_text_as("sum_clock_text")
    }

    pub fn sum_clock_text_as(self, alias: impl Into<String>) -> Self {
        self.aggregate_sum("clock_text", alias)
    }

    pub fn avg_clock_text(self) -> Self {
        self.avg_clock_text_as("avg_clock_text")
    }

    pub fn avg_clock_text_as(self, alias: impl Into<String>) -> Self {
        self.aggregate_avg("clock_text", alias)
    }

    pub fn min_clock_text(self) -> Self {
        self.min_clock_text_as("min_clock_text")
    }

    pub fn min_clock_text_as(self, alias: impl Into<String>) -> Self {
        self.aggregate_min("clock_text", alias)
    }

    pub fn max_clock_text(self) -> Self {
        self.max_clock_text_as("max_clock_text")
    }

    pub fn max_clock_text_as(self, alias: impl Into<String>) -> Self {
        self.aggregate_max("clock_text", alias)
    }

    pub fn unselect_clock_text(mut self) -> Self {
        self.query.projection.retain(|field| field != "clock_text");
        self.query_options.raw_projections.retain(|projection| projection.property_name != "clock_text");
        self
    }


    pub fn with_clock_text(
        mut self,
        operator: FieldOperator,
        values: impl IntoIterator<Item = impl Into<teaql_core::Value>>,
    ) -> Self {
        self.query = self.query.and_filter(field_operator_expr(
            "clock_text",
            operator,
            values.into_iter().map(Into::into).collect(),
        ));
        self
    }

    pub fn create_clock_text_criteria(
        operator: FieldOperator,
        values: impl IntoIterator<Item = impl Into<teaql_core::Value>>,
    ) -> Expr {
        field_operator_expr(
            "clock_text",
            operator,
            values.into_iter().map(Into::into).collect(),
        )
    }

    pub fn with_clock_text_is(mut self, value: impl Into<teaql_core::Value>) -> Self {
        self.query = self.query.and_filter(Expr::eq("clock_text", value));
        self
    }



    pub fn with_clock_text_is_not(mut self, value: impl Into<teaql_core::Value>) -> Self {
        self.query = self.query.and_filter(Expr::ne("clock_text", value));
        self
    }

    pub fn with_clock_text_greater_than(mut self, value: impl Into<teaql_core::Value>) -> Self {
        self.query = self.query.and_filter(Expr::gt("clock_text", value));
        self
    }

    pub fn with_clock_text_greater_than_or_equal_to(mut self, value: impl Into<teaql_core::Value>) -> Self {
        self.query = self.query.and_filter(Expr::gte("clock_text", value));
        self
    }

    pub fn with_clock_text_less_than(mut self, value: impl Into<teaql_core::Value>) -> Self {
        self.query = self.query.and_filter(Expr::lt("clock_text", value));
        self
    }

    pub fn with_clock_text_less_than_or_equal_to(mut self, value: impl Into<teaql_core::Value>) -> Self {
        self.query = self.query.and_filter(Expr::lte("clock_text", value));
        self
    }

    pub fn with_clock_text_between(
        mut self,
        lower: impl Into<teaql_core::Value>,
        upper: impl Into<teaql_core::Value>,
    ) -> Self {
        self.query = self.query.and_filter(Expr::between("clock_text", lower, upper));
        self
    }

    pub fn with_clock_text_between_range<T>(mut self, range: DateRange<T>) -> Self
    where
        T: Into<teaql_core::Value>,
    {
        self.query = self.query.and_filter(Expr::between(
            "clock_text",
            range.start,
            range.end,
        ));
        self
    }

    pub fn with_clock_text_in(
        mut self,
        values: impl IntoIterator<Item = impl Into<teaql_core::Value>>,
    ) -> Self {
        self.query = self.query.and_filter(Expr::in_list(
            "clock_text",
            values.into_iter().map(Into::into),
        ));
        self
    }

    pub fn with_clock_text_not_in(
        mut self,
        values: impl IntoIterator<Item = impl Into<teaql_core::Value>>,
    ) -> Self {
        self.query = self.query.and_filter(Expr::not_in_list(
            "clock_text",
            values.into_iter().map(Into::into),
        ));
        self
    }

    pub fn with_clock_text_before(mut self, value: impl Into<teaql_core::Value>) -> Self {
        self.query = self.query.and_filter(Expr::lt("clock_text", value));
        self
    }

    pub fn with_clock_text_after(mut self, value: impl Into<teaql_core::Value>) -> Self {
        self.query = self.query.and_filter(Expr::gt("clock_text", value));
        self
    }

    pub fn with_clock_text_is_unknown(mut self) -> Self {
        self.query = self.query.and_filter(Expr::is_null("clock_text"));
        self
    }



    pub fn with_clock_text_is_known(mut self) -> Self {
        self.query = self.query.and_filter(Expr::is_not_null("clock_text"));
        self
    }


    pub fn order_by_clock_text_asc(mut self) -> Self {
        self.query = self.query.order_asc("clock_text");
        self
    }

    pub fn order_by_clock_text_desc(mut self) -> Self {
        self.query = self.query.order_desc("clock_text");
        self
    }

    pub fn order_by_clock_text_asc_using_gbk(mut self) -> Self {
        self.query = self.query.order_gbk_asc("clock_text");
        self
    }

    pub fn order_by_clock_text_desc_using_gbk(mut self) -> Self {
        self.query = self.query.order_gbk_desc("clock_text");
        self
    }


    pub fn select_optional_clock_text(mut self) -> Self {
        self.query = self.query.project("optional_clock_text");
        self
    }

    pub fn project_optional_clock_text(self) -> Self {
        self.select_optional_clock_text()
    }

    pub fn select_optional_clock_text_raw(self, raw_sql_segment: impl Into<String>) -> Self {
        self.select_optional_clock_text_unsafe_raw(UnsafeRawSqlSegment::trusted(raw_sql_segment))
    }

    pub fn select_optional_clock_text_unsafe_raw(mut self, raw_sql_segment: UnsafeRawSqlSegment) -> Self {
        self.query_options
            .raw_projections
            .push(RawProjection::new("optional_clock_text", raw_sql_segment));
        self
    }

    pub fn group_by_optional_clock_text(self) -> Self {
        self.group_by("optional_clock_text")
    }

    pub fn group_by_optional_clock_text_as(self, alias: impl Into<String>) -> Self {
        let alias = alias.into();
        let mut request = self.group_by("optional_clock_text");
        request.query = request
            .query
            .project_expr(alias, Expr::column("optional_clock_text"));
        request
    }

    pub fn group_by_optional_clock_text_with_function(
        self,
        alias: impl Into<String>,
        function: AggregateFunction,
    ) -> Self {
        self.group_by("optional_clock_text")
            .aggregate_with_function("optional_clock_text", alias, function)
    }

    pub fn count_optional_clock_text(self) -> Self {
        self.count_optional_clock_text_as("optional_clock_text_count")
    }

    pub fn count_optional_clock_text_as(self, alias: impl Into<String>) -> Self {
        self.aggregate_count_field("optional_clock_text", alias)
    }

    pub fn sum_optional_clock_text(self) -> Self {
        self.sum_optional_clock_text_as("sum_optional_clock_text")
    }

    pub fn sum_optional_clock_text_as(self, alias: impl Into<String>) -> Self {
        self.aggregate_sum("optional_clock_text", alias)
    }

    pub fn avg_optional_clock_text(self) -> Self {
        self.avg_optional_clock_text_as("avg_optional_clock_text")
    }

    pub fn avg_optional_clock_text_as(self, alias: impl Into<String>) -> Self {
        self.aggregate_avg("optional_clock_text", alias)
    }

    pub fn min_optional_clock_text(self) -> Self {
        self.min_optional_clock_text_as("min_optional_clock_text")
    }

    pub fn min_optional_clock_text_as(self, alias: impl Into<String>) -> Self {
        self.aggregate_min("optional_clock_text", alias)
    }

    pub fn max_optional_clock_text(self) -> Self {
        self.max_optional_clock_text_as("max_optional_clock_text")
    }

    pub fn max_optional_clock_text_as(self, alias: impl Into<String>) -> Self {
        self.aggregate_max("optional_clock_text", alias)
    }

    pub fn unselect_optional_clock_text(mut self) -> Self {
        self.query.projection.retain(|field| field != "optional_clock_text");
        self.query_options.raw_projections.retain(|projection| projection.property_name != "optional_clock_text");
        self
    }


    pub fn with_optional_clock_text(
        mut self,
        operator: FieldOperator,
        values: impl IntoIterator<Item = impl Into<teaql_core::Value>>,
    ) -> Self {
        self.query = self.query.and_filter(field_operator_expr(
            "optional_clock_text",
            operator,
            values.into_iter().map(Into::into).collect(),
        ));
        self
    }

    pub fn create_optional_clock_text_criteria(
        operator: FieldOperator,
        values: impl IntoIterator<Item = impl Into<teaql_core::Value>>,
    ) -> Expr {
        field_operator_expr(
            "optional_clock_text",
            operator,
            values.into_iter().map(Into::into).collect(),
        )
    }

    pub fn with_optional_clock_text_is(mut self, value: impl Into<teaql_core::Value>) -> Self {
        self.query = self.query.and_filter(Expr::eq("optional_clock_text", value));
        self
    }



    pub fn with_optional_clock_text_is_not(mut self, value: impl Into<teaql_core::Value>) -> Self {
        self.query = self.query.and_filter(Expr::ne("optional_clock_text", value));
        self
    }

    pub fn with_optional_clock_text_greater_than(mut self, value: impl Into<teaql_core::Value>) -> Self {
        self.query = self.query.and_filter(Expr::gt("optional_clock_text", value));
        self
    }

    pub fn with_optional_clock_text_greater_than_or_equal_to(mut self, value: impl Into<teaql_core::Value>) -> Self {
        self.query = self.query.and_filter(Expr::gte("optional_clock_text", value));
        self
    }

    pub fn with_optional_clock_text_less_than(mut self, value: impl Into<teaql_core::Value>) -> Self {
        self.query = self.query.and_filter(Expr::lt("optional_clock_text", value));
        self
    }

    pub fn with_optional_clock_text_less_than_or_equal_to(mut self, value: impl Into<teaql_core::Value>) -> Self {
        self.query = self.query.and_filter(Expr::lte("optional_clock_text", value));
        self
    }

    pub fn with_optional_clock_text_between(
        mut self,
        lower: impl Into<teaql_core::Value>,
        upper: impl Into<teaql_core::Value>,
    ) -> Self {
        self.query = self.query.and_filter(Expr::between("optional_clock_text", lower, upper));
        self
    }

    pub fn with_optional_clock_text_between_range<T>(mut self, range: DateRange<T>) -> Self
    where
        T: Into<teaql_core::Value>,
    {
        self.query = self.query.and_filter(Expr::between(
            "optional_clock_text",
            range.start,
            range.end,
        ));
        self
    }

    pub fn with_optional_clock_text_in(
        mut self,
        values: impl IntoIterator<Item = impl Into<teaql_core::Value>>,
    ) -> Self {
        self.query = self.query.and_filter(Expr::in_list(
            "optional_clock_text",
            values.into_iter().map(Into::into),
        ));
        self
    }

    pub fn with_optional_clock_text_not_in(
        mut self,
        values: impl IntoIterator<Item = impl Into<teaql_core::Value>>,
    ) -> Self {
        self.query = self.query.and_filter(Expr::not_in_list(
            "optional_clock_text",
            values.into_iter().map(Into::into),
        ));
        self
    }

    pub fn with_optional_clock_text_before(mut self, value: impl Into<teaql_core::Value>) -> Self {
        self.query = self.query.and_filter(Expr::lt("optional_clock_text", value));
        self
    }

    pub fn with_optional_clock_text_after(mut self, value: impl Into<teaql_core::Value>) -> Self {
        self.query = self.query.and_filter(Expr::gt("optional_clock_text", value));
        self
    }

    pub fn with_optional_clock_text_is_unknown(mut self) -> Self {
        self.query = self.query.and_filter(Expr::is_null("optional_clock_text"));
        self
    }



    pub fn with_optional_clock_text_is_known(mut self) -> Self {
        self.query = self.query.and_filter(Expr::is_not_null("optional_clock_text"));
        self
    }


    pub fn order_by_optional_clock_text_asc(mut self) -> Self {
        self.query = self.query.order_asc("optional_clock_text");
        self
    }

    pub fn order_by_optional_clock_text_desc(mut self) -> Self {
        self.query = self.query.order_desc("optional_clock_text");
        self
    }

    pub fn order_by_optional_clock_text_asc_using_gbk(mut self) -> Self {
        self.query = self.query.order_gbk_asc("optional_clock_text");
        self
    }

    pub fn order_by_optional_clock_text_desc_using_gbk(mut self) -> Self {
        self.query = self.query.order_gbk_desc("optional_clock_text");
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


    pub fn select_payload(mut self) -> Self {
        self.query = self.query.project("payload");
        self
    }

    pub fn project_payload(self) -> Self {
        self.select_payload()
    }

    pub fn select_payload_raw(self, raw_sql_segment: impl Into<String>) -> Self {
        self.select_payload_unsafe_raw(UnsafeRawSqlSegment::trusted(raw_sql_segment))
    }

    pub fn select_payload_unsafe_raw(mut self, raw_sql_segment: UnsafeRawSqlSegment) -> Self {
        self.query_options
            .raw_projections
            .push(RawProjection::new("payload", raw_sql_segment));
        self
    }

    pub fn group_by_payload(self) -> Self {
        self.group_by("payload")
    }

    pub fn group_by_payload_as(self, alias: impl Into<String>) -> Self {
        let alias = alias.into();
        let mut request = self.group_by("payload");
        request.query = request
            .query
            .project_expr(alias, Expr::column("payload"));
        request
    }

    pub fn group_by_payload_with_function(
        self,
        alias: impl Into<String>,
        function: AggregateFunction,
    ) -> Self {
        self.group_by("payload")
            .aggregate_with_function("payload", alias, function)
    }

    pub fn count_payload(self) -> Self {
        self.count_payload_as("payload_count")
    }

    pub fn count_payload_as(self, alias: impl Into<String>) -> Self {
        self.aggregate_count_field("payload", alias)
    }

    pub fn sum_payload(self) -> Self {
        self.sum_payload_as("sum_payload")
    }

    pub fn sum_payload_as(self, alias: impl Into<String>) -> Self {
        self.aggregate_sum("payload", alias)
    }

    pub fn avg_payload(self) -> Self {
        self.avg_payload_as("avg_payload")
    }

    pub fn avg_payload_as(self, alias: impl Into<String>) -> Self {
        self.aggregate_avg("payload", alias)
    }

    pub fn min_payload(self) -> Self {
        self.min_payload_as("min_payload")
    }

    pub fn min_payload_as(self, alias: impl Into<String>) -> Self {
        self.aggregate_min("payload", alias)
    }

    pub fn max_payload(self) -> Self {
        self.max_payload_as("max_payload")
    }

    pub fn max_payload_as(self, alias: impl Into<String>) -> Self {
        self.aggregate_max("payload", alias)
    }

    pub fn unselect_payload(mut self) -> Self {
        self.query.projection.retain(|field| field != "payload");
        self.query_options.raw_projections.retain(|projection| projection.property_name != "payload");
        self
    }


    pub fn with_payload(
        mut self,
        operator: FieldOperator,
        values: impl IntoIterator<Item = impl Into<teaql_core::Value>>,
    ) -> Self {
        self.query = self.query.and_filter(field_operator_expr(
            "payload",
            operator,
            values.into_iter().map(Into::into).collect(),
        ));
        self
    }

    pub fn create_payload_criteria(
        operator: FieldOperator,
        values: impl IntoIterator<Item = impl Into<teaql_core::Value>>,
    ) -> Expr {
        field_operator_expr(
            "payload",
            operator,
            values.into_iter().map(Into::into).collect(),
        )
    }

    pub fn with_payload_is(mut self, value: impl Into<teaql_core::Value>) -> Self {
        self.query = self.query.and_filter(Expr::eq("payload", value));
        self
    }



    pub fn with_payload_is_not(mut self, value: impl Into<teaql_core::Value>) -> Self {
        self.query = self.query.and_filter(Expr::ne("payload", value));
        self
    }

    pub fn with_payload_greater_than(mut self, value: impl Into<teaql_core::Value>) -> Self {
        self.query = self.query.and_filter(Expr::gt("payload", value));
        self
    }

    pub fn with_payload_greater_than_or_equal_to(mut self, value: impl Into<teaql_core::Value>) -> Self {
        self.query = self.query.and_filter(Expr::gte("payload", value));
        self
    }

    pub fn with_payload_less_than(mut self, value: impl Into<teaql_core::Value>) -> Self {
        self.query = self.query.and_filter(Expr::lt("payload", value));
        self
    }

    pub fn with_payload_less_than_or_equal_to(mut self, value: impl Into<teaql_core::Value>) -> Self {
        self.query = self.query.and_filter(Expr::lte("payload", value));
        self
    }

    pub fn with_payload_between(
        mut self,
        lower: impl Into<teaql_core::Value>,
        upper: impl Into<teaql_core::Value>,
    ) -> Self {
        self.query = self.query.and_filter(Expr::between("payload", lower, upper));
        self
    }

    pub fn with_payload_between_range<T>(mut self, range: DateRange<T>) -> Self
    where
        T: Into<teaql_core::Value>,
    {
        self.query = self.query.and_filter(Expr::between(
            "payload",
            range.start,
            range.end,
        ));
        self
    }

    pub fn with_payload_in(
        mut self,
        values: impl IntoIterator<Item = impl Into<teaql_core::Value>>,
    ) -> Self {
        self.query = self.query.and_filter(Expr::in_list(
            "payload",
            values.into_iter().map(Into::into),
        ));
        self
    }

    pub fn with_payload_not_in(
        mut self,
        values: impl IntoIterator<Item = impl Into<teaql_core::Value>>,
    ) -> Self {
        self.query = self.query.and_filter(Expr::not_in_list(
            "payload",
            values.into_iter().map(Into::into),
        ));
        self
    }

    pub fn with_payload_before(mut self, value: impl Into<teaql_core::Value>) -> Self {
        self.query = self.query.and_filter(Expr::lt("payload", value));
        self
    }

    pub fn with_payload_after(mut self, value: impl Into<teaql_core::Value>) -> Self {
        self.query = self.query.and_filter(Expr::gt("payload", value));
        self
    }

    pub fn with_payload_is_unknown(mut self) -> Self {
        self.query = self.query.and_filter(Expr::is_null("payload"));
        self
    }



    pub fn with_payload_is_known(mut self) -> Self {
        self.query = self.query.and_filter(Expr::is_not_null("payload"));
        self
    }


    pub fn order_by_payload_asc(mut self) -> Self {
        self.query = self.query.order_asc("payload");
        self
    }

    pub fn order_by_payload_desc(mut self) -> Self {
        self.query = self.query.order_desc("payload");
        self
    }

    pub fn order_by_payload_asc_using_gbk(mut self) -> Self {
        self.query = self.query.order_gbk_asc("payload");
        self
    }

    pub fn order_by_payload_desc_using_gbk(mut self) -> Self {
        self.query = self.query.order_gbk_desc("payload");
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
    pub fn filter_by_platform(mut self, value: impl EntityReference) -> Self {
        self.query = self.query.and_filter(Expr::eq("platform_id", value.entity_id_value()));
        self
    }

    pub fn with_platform_matching(mut self, request: impl Into<QuerySelection>) -> Self {
        let selection = request.into();
        self.query = self.query.and_filter(Expr::in_subquery(
            "platform_id",
            <crate::Platform as teaql_core::TeaqlEntity>::entity_descriptor(),
            selection.query.clone(),
            "id",
        ));
        self.relation_filters.push(RelationFilter::new("platform", selection));
        self
    }


    pub fn without_platform_matching(mut self, request: impl Into<QuerySelection>) -> Self {
        let selection = request.into();
        self.query = self.query.and_filter(Expr::not_in_subquery(
            "platform_id",
            <crate::Platform as teaql_core::TeaqlEntity>::entity_descriptor(),
            selection.query.clone(),
            "id",
        ));
        self.relation_filters.push(RelationFilter::new("platform", selection));
        self
    }


    pub fn have_platform(mut self) -> Self {
        self.query = self.query.and_filter(Expr::is_not_null("platform_id"));
        self
    }

    pub fn have_no_platform(mut self) -> Self {
        self.query = self.query.and_filter(Expr::is_null("platform_id"));
        self
    }


    pub fn group_by_platform(self) -> Self {
        self.group_by("platform_id")
    }

    pub fn group_by_platform_as(self, alias: impl Into<String>) -> Self {
        let alias = alias.into();
        let mut request = self.group_by("platform_id");
        request.query = request
            .query
            .project_expr(alias, Expr::column("platform_id"));
        request
    }

    pub fn group_by_platform_with_function(
        self,
        alias: impl Into<String>,
        function: AggregateFunction,
    ) -> Self {
        self.group_by("platform_id")
            .aggregate_with_function("platform_id", alias, function)
    }

    pub fn group_by_platform_with(mut self, request: impl Into<QuerySelection>) -> Self {
        self.query = self.query.group_by("platform_id");
        self.query_options.object_group_bys.push(ObjectGroupBy::new(
            "platform",
            "platform_id",
            request,
        ));
        self
    }

    pub fn group_by_platform_with_details(self) -> Self {
        self.group_by_platform_with_details_from(crate::Q::platforms().unlimited())
    }

    pub fn group_by_platform_with_details_from(self, request: impl Into<QuerySelection>) -> Self {
        self.group_by_platform_with(request)
    }


    pub fn roll_up_to_platform(self) -> Self {
        self.roll_up_to_platform_with(crate::Q::platforms().unlimited())
    }

    pub fn roll_up_to_platform_with(self, request: impl Into<QuerySelection>) -> Self {
        let selection = request.into();
        self.with_platform_matching(selection.clone())
            .group_by_platform_with(selection)
    }

    pub fn count_platform(self) -> Self {
        self.count_platform_as("platform_count")
    }

    pub fn count_platform_as(self, alias: impl Into<String>) -> Self {
        self.aggregate_count_field("platform_id", alias)
    }

    pub fn unselect_platform(mut self) -> Self {
        self.query.projection.retain(|field| field != "platform_id");
        self.query.relations.retain(|relation| relation.name != "platform");
        self
    }
    pub fn select_platform(mut self) -> Self {
        self.query = self.query.relation("platform");
        self
    }

    pub fn select_platform_with(mut self, request: impl Into<QuerySelection>) -> Self {
        let selection = request.into();
        self.query = self.query.relation_query("platform", selection.into_query());
        self
}

    pub fn facet_by_platform_as(self, facet_name: impl Into<String>, request: impl Into<QuerySelection>) -> Self {
        self.facet_by_platform_as_with_options(facet_name, request, true)
    }

    pub fn facet_by_platform_as_with_options(
        mut self,
        facet_name: impl Into<String>,
        request: impl Into<QuerySelection>,
        include_all_facets: bool,
    ) -> Self {
        self.query_options.facets.push(FacetRequest::new(
            facet_name,
            "platform",
            request,
            include_all_facets,
        ));
        self
    }
    pub fn have_first_links(self) -> Self {
        self.with_first_links_matching(crate::Q::sample_links_minimal())
    }

    pub fn have_no_first_links(self) -> Self {
        self.without_first_links_matching(crate::Q::sample_links_minimal())
    }

    pub fn with_first_links_matching(mut self, request: impl Into<QuerySelection>) -> Self {
        let selection = request.into();
        self.query = self.query.and_filter(Expr::in_subquery(
            "id",
            <crate::SampleLink as teaql_core::TeaqlEntity>::entity_descriptor(),
            selection.query.clone(),
            "first_record_id",
        ));
        self.relation_filters.push(RelationFilter::new("first_links", selection));
        self
    }

    pub fn without_first_links_matching(mut self, request: impl Into<QuerySelection>) -> Self {
        let selection = request.into();
        self.query = self.query.and_filter(Expr::not_in_subquery(
            "id",
            <crate::SampleLink as teaql_core::TeaqlEntity>::entity_descriptor(),
            selection.query.clone(),
            "first_record_id",
        ));
        self.relation_filters.push(RelationFilter::new("first_links", selection));
        self
    }

    pub fn select_first_links(mut self) -> Self {
        self.query = self.query.relation("first_links");
        self
    }

    pub fn select_first_links_with(mut self, request: impl Into<QuerySelection>) -> Self {
        let selection = request.into();
        self.query = self.query.relation_query("first_links", selection.into_query());
        self
}

    pub fn have_second_links(self) -> Self {
        self.with_second_links_matching(crate::Q::sample_links_minimal())
    }

    pub fn have_no_second_links(self) -> Self {
        self.without_second_links_matching(crate::Q::sample_links_minimal())
    }

    pub fn with_second_links_matching(mut self, request: impl Into<QuerySelection>) -> Self {
        let selection = request.into();
        self.query = self.query.and_filter(Expr::in_subquery(
            "id",
            <crate::SampleLink as teaql_core::TeaqlEntity>::entity_descriptor(),
            selection.query.clone(),
            "second_record_id",
        ));
        self.relation_filters.push(RelationFilter::new("second_links", selection));
        self
    }

    pub fn without_second_links_matching(mut self, request: impl Into<QuerySelection>) -> Self {
        let selection = request.into();
        self.query = self.query.and_filter(Expr::not_in_subquery(
            "id",
            <crate::SampleLink as teaql_core::TeaqlEntity>::entity_descriptor(),
            selection.query.clone(),
            "second_record_id",
        ));
        self.relation_filters.push(RelationFilter::new("second_links", selection));
        self
    }

    pub fn select_second_links(mut self) -> Self {
        self.query = self.query.relation("second_links");
        self
    }

    pub fn select_second_links_with(mut self, request: impl Into<QuerySelection>) -> Self {
        let selection = request.into();
        self.query = self.query.relation_query("second_links", selection.into_query());
        self
}
    pub fn count_first_links(self) -> Self {
        self.count_first_links_as("count_first_links")
    }

    pub fn count_first_links_as(self, alias: impl Into<String>) -> Self {
        self.count_first_links_with(alias, crate::Q::sample_links().unlimited())
    }

    pub fn count_first_links_with(mut self, alias: impl Into<String>, request: impl Into<QuerySelection>) -> Self {
        let selection = request.into();
        self.query_options.relation_aggregates.push(RelationAggregate::new(
            "first_links",
            alias,
            selection,
            true,
        ));
        self
    }

    pub fn stats_from_first_links(self, request: impl Into<QuerySelection>) -> Self {
        self.stats_from_first_links_as("refinements", request)
    }

    pub fn stats_from_first_links_as(mut self, alias: impl Into<String>, request: impl Into<QuerySelection>) -> Self {
        let selection = request.into();
        self.query_options.relation_aggregates.push(RelationAggregate::new(
            "first_links",
            alias,
            selection,
            false,
        ));
        self
    }

    fn scalar_from_first_links_as(mut self, alias: impl Into<String>, request: impl Into<QuerySelection>) -> Self {
        let selection = request.into();
        self.query_options.relation_aggregates.push(RelationAggregate::new(
            "first_links",
            alias,
            selection,
            true,
        ));
        self
    }

    pub fn group_by_first_links_with_details(self, request: impl Into<QuerySelection>) -> Self {
        self.stats_from_first_links(request)
    }




    pub fn count_second_links(self) -> Self {
        self.count_second_links_as("count_second_links")
    }

    pub fn count_second_links_as(self, alias: impl Into<String>) -> Self {
        self.count_second_links_with(alias, crate::Q::sample_links().unlimited())
    }

    pub fn count_second_links_with(mut self, alias: impl Into<String>, request: impl Into<QuerySelection>) -> Self {
        let selection = request.into();
        self.query_options.relation_aggregates.push(RelationAggregate::new(
            "second_links",
            alias,
            selection,
            true,
        ));
        self
    }

    pub fn stats_from_second_links(self, request: impl Into<QuerySelection>) -> Self {
        self.stats_from_second_links_as("refinements", request)
    }

    pub fn stats_from_second_links_as(mut self, alias: impl Into<String>, request: impl Into<QuerySelection>) -> Self {
        let selection = request.into();
        self.query_options.relation_aggregates.push(RelationAggregate::new(
            "second_links",
            alias,
            selection,
            false,
        ));
        self
    }

    fn scalar_from_second_links_as(mut self, alias: impl Into<String>, request: impl Into<QuerySelection>) -> Self {
        let selection = request.into();
        self.query_options.relation_aggregates.push(RelationAggregate::new(
            "second_links",
            alias,
            selection,
            true,
        ));
        self
    }

    pub fn group_by_second_links_with_details(self, request: impl Into<QuerySelection>) -> Self {
        self.stats_from_second_links(request)
    }



}

impl<R> Default for SampleRecordRequest<R> {
    fn default() -> Self {
        Self::new()
    }
}

impl<R> From< SampleRecordRequest<R> > for SelectQuery {
    fn from(request: SampleRecordRequest<R>) -> Self {
        QuerySelection::from(request).into_query()
    }
}

impl<R> From< SampleRecordRequest<R> > for QuerySelection {
    fn from(request: SampleRecordRequest<R>) -> Self {
        Self {
            query: request.query,
            relation_selections: request.relation_selections,
            relation_filters: request.relation_filters,
            child_enhancements: request.child_enhancements,
            query_options: request.query_options,
        }
    }
}


impl<'a, C> crate::request_support::AuditedSave<'a, C> for teaql_core::Audited<crate::SampleRecord> 
where C: crate::TeaqlRuntime + ?Sized + 'a
{
    type Error = teaql_runtime::RuntimeError;
    type Entity = crate::SampleRecord;
    fn save(self, context: &'a C) -> std::pin::Pin<Box<dyn std::future::Future<Output = Result<Self::Entity, Self::Error>> + Send + '_>> {
        Box::pin(async move {
            context.save_audited_entity(self).await
        })
    }
}

impl<R: teaql_core::Entity> crate::PurposedQuery<SampleRecordRequest<R>> {
    pub fn comment(mut self, comment: impl Into<String>) -> Self {
        self.inner.query_options.comment = Some(comment.into());
        self
    }

    pub fn new_entity<C>(&self, context: &C) -> crate::SampleRecord
    where
        C: crate::TeaqlRuntime + ?Sized,
    {
        self.require_comment();
        let mut entity = crate::SampleRecord::runtime_new(context.user_context().entity_runtime_state());
        if let Ok(id) = context.user_context().next_id(crate::SampleRecord::ENTITY_NAME) {
            entity.update_id(id);
        }
        teaql_core::Entity::mark_as_new(&mut entity);
        entity
    }

    fn into_inner_with_trace(mut self) -> SampleRecordRequest<R> {
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
