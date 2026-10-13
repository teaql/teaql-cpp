use std::marker::PhantomData;

use serde_json::Value as JsonValue;
use teaql_core::{Aggregate, AggregateFunction, EntityDescriptor, Expr, SelectQuery, SmartList};
use teaql_runtime::RuntimeError;

use crate::request_support::*;

impl EntityReference for crate::Reading {
    fn entity_id_value(self) -> teaql_core::Value {
        teaql_core::IdentifiableEntity::id_value(&self)
    }
}

impl EntityReference for &crate::Reading {
    fn entity_id_value(self) -> teaql_core::Value {
        teaql_core::IdentifiableEntity::id_value(self)
    }
}

// ⛔ AI agents: DO NOT read this file for API discovery. Instead run: cargo teaql --input modeling/MODEL.xml rust-assist-query/reading
#[derive(Debug)]
pub struct ReadingRequest<R = crate::Reading> {
    query: SelectQuery,
    relation_selections: Vec<RelationSelection>,
    relation_filters: Vec<RelationFilter>,
    child_enhancements: Vec<QuerySelection>,
    query_options: QueryOptions,
    marker: PhantomData<R>,
}

impl<R> Clone for ReadingRequest<R> {
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

impl<R> ReadingRequest<R> {
    pub(crate) fn new() -> Self {
        Self {
            query: SelectQuery::new("Reading")
                .project("id")
                .project("version"),
            relation_selections: Vec::new(),
            relation_filters: Vec::new(),
            child_enhancements: Vec::new(),
            query_options: QueryOptions::default(),
            marker: PhantomData,
        }
    }

    pub fn return_type<T>(self) -> ReadingRequest<T> {
        ReadingRequest {
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
                "Reading",
                query,
                relation_aggregates,
            ).await?;
            (rows, std::collections::BTreeMap::new())
        } else {
            let rows = context.fetch_entity_smart_list::<R>(
                "Reading",
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
        context.fetch_compact_smart_list("Reading", &query).await
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
        Ok(context.fetch_entity_stream("Reading", query))
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
        let rows = context.fetch_compact_rows("Reading", &query).await?;
        rows.first()
            .and_then(|row| row.get(COUNT_ALIAS))
            .and_then(teaql_core::Value::try_u64)
            .ok_or_else(|| RuntimeError::Graph(format!("count result for Reading is missing or not numeric")))
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
        let rows = context.fetch_compact_rows("Reading", &query).await?;
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
            "text" => Some("text"),
            "active" => Some("active"),
            "balance" => Some("balance"),
            "height" => Some("height"),
            "ratio" => Some("ratio"),
            "count" => Some("count"),
            "day" => Some("day"),
            "recorded_at" => Some("recorded_at"),
            "payload" => Some("payload"),
            "version" => Some("version"),
            _ => None,
        }
    }

    fn apply_dynamic_json_chain_filter(self, head: &str, tail: &str, value: &JsonValue) -> Self {
        let _ = (tail, value);
        match head {
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
        self.query = self.query.project("text");
        self.query = self.query.project("active");
        self.query = self.query.project("balance");
        self.query = self.query.project("height");
        self.query = self.query.project("ratio");
        self.query = self.query.project("count");
        self.query = self.query.project("day");
        self.query = self.query.project("recorded_at");
        self.query = self.query.project("payload");
        self.query = self.query.project("version");
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
        self.select_self()
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


    pub fn select_text(mut self) -> Self {
        self.query = self.query.project("text");
        self
    }

    pub fn project_text(self) -> Self {
        self.select_text()
    }

    pub fn select_text_raw(self, raw_sql_segment: impl Into<String>) -> Self {
        self.select_text_unsafe_raw(UnsafeRawSqlSegment::trusted(raw_sql_segment))
    }

    pub fn select_text_unsafe_raw(mut self, raw_sql_segment: UnsafeRawSqlSegment) -> Self {
        self.query_options
            .raw_projections
            .push(RawProjection::new("text", raw_sql_segment));
        self
    }

    pub fn group_by_text(self) -> Self {
        self.group_by("text")
    }

    pub fn group_by_text_as(self, alias: impl Into<String>) -> Self {
        let alias = alias.into();
        let mut request = self.group_by("text");
        request.query = request
            .query
            .project_expr(alias, Expr::column("text"));
        request
    }

    pub fn group_by_text_with_function(
        self,
        alias: impl Into<String>,
        function: AggregateFunction,
    ) -> Self {
        self.group_by("text")
            .aggregate_with_function("text", alias, function)
    }

    pub fn count_text(self) -> Self {
        self.count_text_as("text_count")
    }

    pub fn count_text_as(self, alias: impl Into<String>) -> Self {
        self.aggregate_count_field("text", alias)
    }

    pub fn sum_text(self) -> Self {
        self.sum_text_as("sum_text")
    }

    pub fn sum_text_as(self, alias: impl Into<String>) -> Self {
        self.aggregate_sum("text", alias)
    }

    pub fn avg_text(self) -> Self {
        self.avg_text_as("avg_text")
    }

    pub fn avg_text_as(self, alias: impl Into<String>) -> Self {
        self.aggregate_avg("text", alias)
    }

    pub fn min_text(self) -> Self {
        self.min_text_as("min_text")
    }

    pub fn min_text_as(self, alias: impl Into<String>) -> Self {
        self.aggregate_min("text", alias)
    }

    pub fn max_text(self) -> Self {
        self.max_text_as("max_text")
    }

    pub fn max_text_as(self, alias: impl Into<String>) -> Self {
        self.aggregate_max("text", alias)
    }

    pub fn unselect_text(mut self) -> Self {
        self.query.projection.retain(|field| field != "text");
        self.query_options.raw_projections.retain(|projection| projection.property_name != "text");
        self
    }


    pub fn with_text(
        mut self,
        operator: FieldOperator,
        values: impl IntoIterator<Item = impl Into<teaql_core::Value>>,
    ) -> Self {
        self.query = self.query.and_filter(field_operator_expr(
            "text",
            operator,
            values.into_iter().map(Into::into).collect(),
        ));
        self
    }

    pub fn create_text_criteria(
        operator: FieldOperator,
        values: impl IntoIterator<Item = impl Into<teaql_core::Value>>,
    ) -> Expr {
        field_operator_expr(
            "text",
            operator,
            values.into_iter().map(Into::into).collect(),
        )
    }

    pub fn with_text_is(mut self, value: impl Into<teaql_core::Value>) -> Self {
        self.query = self.query.and_filter(Expr::eq("text", value));
        self
    }



    pub fn with_text_is_not(mut self, value: impl Into<teaql_core::Value>) -> Self {
        self.query = self.query.and_filter(Expr::ne("text", value));
        self
    }

    pub fn with_text_greater_than(mut self, value: impl Into<teaql_core::Value>) -> Self {
        self.query = self.query.and_filter(Expr::gt("text", value));
        self
    }

    pub fn with_text_greater_than_or_equal_to(mut self, value: impl Into<teaql_core::Value>) -> Self {
        self.query = self.query.and_filter(Expr::gte("text", value));
        self
    }

    pub fn with_text_less_than(mut self, value: impl Into<teaql_core::Value>) -> Self {
        self.query = self.query.and_filter(Expr::lt("text", value));
        self
    }

    pub fn with_text_less_than_or_equal_to(mut self, value: impl Into<teaql_core::Value>) -> Self {
        self.query = self.query.and_filter(Expr::lte("text", value));
        self
    }

    pub fn with_text_between(
        mut self,
        lower: impl Into<teaql_core::Value>,
        upper: impl Into<teaql_core::Value>,
    ) -> Self {
        self.query = self.query.and_filter(Expr::between("text", lower, upper));
        self
    }

    pub fn with_text_between_range<T>(mut self, range: DateRange<T>) -> Self
    where
        T: Into<teaql_core::Value>,
    {
        self.query = self.query.and_filter(Expr::between(
            "text",
            range.start,
            range.end,
        ));
        self
    }

    pub fn with_text_in(
        mut self,
        values: impl IntoIterator<Item = impl Into<teaql_core::Value>>,
    ) -> Self {
        self.query = self.query.and_filter(Expr::in_list(
            "text",
            values.into_iter().map(Into::into),
        ));
        self
    }

    pub fn with_text_not_in(
        mut self,
        values: impl IntoIterator<Item = impl Into<teaql_core::Value>>,
    ) -> Self {
        self.query = self.query.and_filter(Expr::not_in_list(
            "text",
            values.into_iter().map(Into::into),
        ));
        self
    }

    pub fn with_text_containing(mut self, value: impl Into<String>) -> Self {
        self.query = self.query.and_filter(Expr::contain("text", value));
        self
    }

    pub fn with_text_not_containing(mut self, value: impl Into<String>) -> Self {
        self.query = self.query.and_filter(Expr::not_contain("text", value));
        self
    }

    pub fn with_text_starting_with(mut self, value: impl Into<String>) -> Self {
        self.query = self.query.and_filter(Expr::begin_with("text", value));
        self
    }

    pub fn with_text_not_starting_with(mut self, value: impl Into<String>) -> Self {
        self.query = self.query.and_filter(Expr::not_begin_with("text", value));
        self
    }

    pub fn with_text_ending_with(mut self, value: impl Into<String>) -> Self {
        self.query = self.query.and_filter(Expr::end_with("text", value));
        self
    }

    pub fn with_text_not_ending_with(mut self, value: impl Into<String>) -> Self {
        self.query = self.query.and_filter(Expr::not_end_with("text", value));
        self
    }

    pub fn with_text_sounding_like(mut self, value: impl Into<teaql_core::Value>) -> Self {
        self.query = self.query.and_filter(Expr::sound_like("text", value));
        self
    }
    pub fn with_text_before(mut self, value: impl Into<teaql_core::Value>) -> Self {
        self.query = self.query.and_filter(Expr::lt("text", value));
        self
    }

    pub fn with_text_after(mut self, value: impl Into<teaql_core::Value>) -> Self {
        self.query = self.query.and_filter(Expr::gt("text", value));
        self
    }

    pub fn with_text_is_unknown(mut self) -> Self {
        self.query = self.query.and_filter(Expr::is_null("text"));
        self
    }



    pub fn with_text_is_known(mut self) -> Self {
        self.query = self.query.and_filter(Expr::is_not_null("text"));
        self
    }


    pub fn order_by_text_asc(mut self) -> Self {
        self.query = self.query.order_asc("text");
        self
    }

    pub fn order_by_text_desc(mut self) -> Self {
        self.query = self.query.order_desc("text");
        self
    }

    pub fn order_by_text_asc_using_gbk(mut self) -> Self {
        self.query = self.query.order_gbk_asc("text");
        self
    }

    pub fn order_by_text_desc_using_gbk(mut self) -> Self {
        self.query = self.query.order_gbk_desc("text");
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


    pub fn select_balance(mut self) -> Self {
        self.query = self.query.project("balance");
        self
    }

    pub fn project_balance(self) -> Self {
        self.select_balance()
    }

    pub fn select_balance_raw(self, raw_sql_segment: impl Into<String>) -> Self {
        self.select_balance_unsafe_raw(UnsafeRawSqlSegment::trusted(raw_sql_segment))
    }

    pub fn select_balance_unsafe_raw(mut self, raw_sql_segment: UnsafeRawSqlSegment) -> Self {
        self.query_options
            .raw_projections
            .push(RawProjection::new("balance", raw_sql_segment));
        self
    }

    pub fn select_balance_with_function(self, function: AggregateFunction) -> Self {
        self.select_balance_as_with_function("balance", function)
    }

    pub fn select_balance_as_with_function(
        self,
        alias: impl Into<String>,
        function: AggregateFunction,
    ) -> Self {
        self.aggregate_with_function("balance", alias, function)
    }

    pub fn group_by_balance(self) -> Self {
        self.group_by("balance")
    }

    pub fn group_by_balance_as(self, alias: impl Into<String>) -> Self {
        let alias = alias.into();
        let mut request = self.group_by("balance");
        request.query = request
            .query
            .project_expr(alias, Expr::column("balance"));
        request
    }

    pub fn group_by_balance_with_function(
        self,
        alias: impl Into<String>,
        function: AggregateFunction,
    ) -> Self {
        self.group_by("balance")
            .aggregate_with_function("balance", alias, function)
    }

    pub fn count_balance(self) -> Self {
        self.count_balance_as("balance_count")
    }

    pub fn count_balance_as(self, alias: impl Into<String>) -> Self {
        self.aggregate_count_field("balance", alias)
    }

    pub fn sum_balance(self) -> Self {
        self.sum_balance_as("sum_balance")
    }

    pub fn sum_balance_as(self, alias: impl Into<String>) -> Self {
        self.aggregate_sum("balance", alias)
    }

    pub fn avg_balance(self) -> Self {
        self.avg_balance_as("avg_balance")
    }

    pub fn avg_balance_as(self, alias: impl Into<String>) -> Self {
        self.aggregate_avg("balance", alias)
    }

    pub fn min_balance(self) -> Self {
        self.min_balance_as("min_balance")
    }

    pub fn min_balance_as(self, alias: impl Into<String>) -> Self {
        self.aggregate_min("balance", alias)
    }

    pub fn max_balance(self) -> Self {
        self.max_balance_as("max_balance")
    }

    pub fn max_balance_as(self, alias: impl Into<String>) -> Self {
        self.aggregate_max("balance", alias)
    }

    pub fn standard_deviation_balance(self) -> Self {
        self.standard_deviation_balance_as("stdDev_balance")
    }

    pub fn standard_deviation_balance_as(self, alias: impl Into<String>) -> Self {
        self.aggregate_stddev("balance", alias)
    }

    pub fn square_root_of_population_standard_deviation_balance(self) -> Self {
        self.square_root_of_population_standard_deviation_balance_as("stdDevPop_balance")
    }

    pub fn square_root_of_population_standard_deviation_balance_as(self, alias: impl Into<String>) -> Self {
        self.aggregate_stddev_pop("balance", alias)
    }

    pub fn sample_variance_balance(self) -> Self {
        self.sample_variance_balance_as("varSamp_balance")
    }

    pub fn sample_variance_balance_as(self, alias: impl Into<String>) -> Self {
        self.aggregate_var_samp("balance", alias)
    }

    pub fn sample_population_variance_balance(self) -> Self {
        self.sample_population_variance_balance_as("varPop_balance")
    }

    pub fn sample_population_variance_balance_as(self, alias: impl Into<String>) -> Self {
        self.aggregate_var_pop("balance", alias)
    }

    pub fn unselect_balance(mut self) -> Self {
        self.query.projection.retain(|field| field != "balance");
        self.query_options.raw_projections.retain(|projection| projection.property_name != "balance");
        self
    }


    pub fn with_balance(
        mut self,
        operator: FieldOperator,
        values: impl IntoIterator<Item = impl Into<teaql_core::Value>>,
    ) -> Self {
        self.query = self.query.and_filter(field_operator_expr(
            "balance",
            operator,
            values.into_iter().map(Into::into).collect(),
        ));
        self
    }

    pub fn create_balance_criteria(
        operator: FieldOperator,
        values: impl IntoIterator<Item = impl Into<teaql_core::Value>>,
    ) -> Expr {
        field_operator_expr(
            "balance",
            operator,
            values.into_iter().map(Into::into).collect(),
        )
    }

    pub fn with_balance_is(mut self, value: impl Into<teaql_core::Value>) -> Self {
        self.query = self.query.and_filter(Expr::eq("balance", value));
        self
    }



    pub fn with_balance_is_not(mut self, value: impl Into<teaql_core::Value>) -> Self {
        self.query = self.query.and_filter(Expr::ne("balance", value));
        self
    }

    pub fn with_balance_greater_than(mut self, value: impl Into<teaql_core::Value>) -> Self {
        self.query = self.query.and_filter(Expr::gt("balance", value));
        self
    }

    pub fn with_balance_greater_than_or_equal_to(mut self, value: impl Into<teaql_core::Value>) -> Self {
        self.query = self.query.and_filter(Expr::gte("balance", value));
        self
    }

    pub fn with_balance_less_than(mut self, value: impl Into<teaql_core::Value>) -> Self {
        self.query = self.query.and_filter(Expr::lt("balance", value));
        self
    }

    pub fn with_balance_less_than_or_equal_to(mut self, value: impl Into<teaql_core::Value>) -> Self {
        self.query = self.query.and_filter(Expr::lte("balance", value));
        self
    }

    pub fn with_balance_between(
        mut self,
        lower: impl Into<teaql_core::Value>,
        upper: impl Into<teaql_core::Value>,
    ) -> Self {
        self.query = self.query.and_filter(Expr::between("balance", lower, upper));
        self
    }

    pub fn with_balance_between_range<T>(mut self, range: DateRange<T>) -> Self
    where
        T: Into<teaql_core::Value>,
    {
        self.query = self.query.and_filter(Expr::between(
            "balance",
            range.start,
            range.end,
        ));
        self
    }

    pub fn with_balance_in(
        mut self,
        values: impl IntoIterator<Item = impl Into<teaql_core::Value>>,
    ) -> Self {
        self.query = self.query.and_filter(Expr::in_list(
            "balance",
            values.into_iter().map(Into::into),
        ));
        self
    }

    pub fn with_balance_not_in(
        mut self,
        values: impl IntoIterator<Item = impl Into<teaql_core::Value>>,
    ) -> Self {
        self.query = self.query.and_filter(Expr::not_in_list(
            "balance",
            values.into_iter().map(Into::into),
        ));
        self
    }

    pub fn with_balance_before(mut self, value: impl Into<teaql_core::Value>) -> Self {
        self.query = self.query.and_filter(Expr::lt("balance", value));
        self
    }

    pub fn with_balance_after(mut self, value: impl Into<teaql_core::Value>) -> Self {
        self.query = self.query.and_filter(Expr::gt("balance", value));
        self
    }

    pub fn with_balance_is_unknown(mut self) -> Self {
        self.query = self.query.and_filter(Expr::is_null("balance"));
        self
    }



    pub fn with_balance_is_known(mut self) -> Self {
        self.query = self.query.and_filter(Expr::is_not_null("balance"));
        self
    }


    pub fn order_by_balance_asc(mut self) -> Self {
        self.query = self.query.order_asc("balance");
        self
    }

    pub fn order_by_balance_desc(mut self) -> Self {
        self.query = self.query.order_desc("balance");
        self
    }

    pub fn order_by_balance_asc_using_gbk(mut self) -> Self {
        self.query = self.query.order_gbk_asc("balance");
        self
    }

    pub fn order_by_balance_desc_using_gbk(mut self) -> Self {
        self.query = self.query.order_gbk_desc("balance");
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


    pub fn select_count(mut self) -> Self {
        self.query = self.query.project("count");
        self
    }

    pub fn project_count(self) -> Self {
        self.select_count()
    }

    pub fn select_count_raw(self, raw_sql_segment: impl Into<String>) -> Self {
        self.select_count_unsafe_raw(UnsafeRawSqlSegment::trusted(raw_sql_segment))
    }

    pub fn select_count_unsafe_raw(mut self, raw_sql_segment: UnsafeRawSqlSegment) -> Self {
        self.query_options
            .raw_projections
            .push(RawProjection::new("count", raw_sql_segment));
        self
    }

    pub fn select_count_with_function(self, function: AggregateFunction) -> Self {
        self.select_count_as_with_function("count", function)
    }

    pub fn select_count_as_with_function(
        self,
        alias: impl Into<String>,
        function: AggregateFunction,
    ) -> Self {
        self.aggregate_with_function("count", alias, function)
    }

    pub fn group_by_count(self) -> Self {
        self.group_by("count")
    }

    pub fn group_by_count_as(self, alias: impl Into<String>) -> Self {
        let alias = alias.into();
        let mut request = self.group_by("count");
        request.query = request
            .query
            .project_expr(alias, Expr::column("count"));
        request
    }

    pub fn group_by_count_with_function(
        self,
        alias: impl Into<String>,
        function: AggregateFunction,
    ) -> Self {
        self.group_by("count")
            .aggregate_with_function("count", alias, function)
    }

    pub fn count_count(self) -> Self {
        self.count_count_as("count_count")
    }

    pub fn count_count_as(self, alias: impl Into<String>) -> Self {
        self.aggregate_count_field("count", alias)
    }

    pub fn sum_count(self) -> Self {
        self.sum_count_as("sum_count")
    }

    pub fn sum_count_as(self, alias: impl Into<String>) -> Self {
        self.aggregate_sum("count", alias)
    }

    pub fn avg_count(self) -> Self {
        self.avg_count_as("avg_count")
    }

    pub fn avg_count_as(self, alias: impl Into<String>) -> Self {
        self.aggregate_avg("count", alias)
    }

    pub fn min_count(self) -> Self {
        self.min_count_as("min_count")
    }

    pub fn min_count_as(self, alias: impl Into<String>) -> Self {
        self.aggregate_min("count", alias)
    }

    pub fn max_count(self) -> Self {
        self.max_count_as("max_count")
    }

    pub fn max_count_as(self, alias: impl Into<String>) -> Self {
        self.aggregate_max("count", alias)
    }

    pub fn standard_deviation_count(self) -> Self {
        self.standard_deviation_count_as("stdDev_count")
    }

    pub fn standard_deviation_count_as(self, alias: impl Into<String>) -> Self {
        self.aggregate_stddev("count", alias)
    }

    pub fn square_root_of_population_standard_deviation_count(self) -> Self {
        self.square_root_of_population_standard_deviation_count_as("stdDevPop_count")
    }

    pub fn square_root_of_population_standard_deviation_count_as(self, alias: impl Into<String>) -> Self {
        self.aggregate_stddev_pop("count", alias)
    }

    pub fn sample_variance_count(self) -> Self {
        self.sample_variance_count_as("varSamp_count")
    }

    pub fn sample_variance_count_as(self, alias: impl Into<String>) -> Self {
        self.aggregate_var_samp("count", alias)
    }

    pub fn sample_population_variance_count(self) -> Self {
        self.sample_population_variance_count_as("varPop_count")
    }

    pub fn sample_population_variance_count_as(self, alias: impl Into<String>) -> Self {
        self.aggregate_var_pop("count", alias)
    }

    pub fn unselect_count(mut self) -> Self {
        self.query.projection.retain(|field| field != "count");
        self.query_options.raw_projections.retain(|projection| projection.property_name != "count");
        self
    }


    pub fn with_count(
        mut self,
        operator: FieldOperator,
        values: impl IntoIterator<Item = impl Into<teaql_core::Value>>,
    ) -> Self {
        self.query = self.query.and_filter(field_operator_expr(
            "count",
            operator,
            values.into_iter().map(Into::into).collect(),
        ));
        self
    }

    pub fn create_count_criteria(
        operator: FieldOperator,
        values: impl IntoIterator<Item = impl Into<teaql_core::Value>>,
    ) -> Expr {
        field_operator_expr(
            "count",
            operator,
            values.into_iter().map(Into::into).collect(),
        )
    }

    pub fn with_count_is(mut self, value: impl Into<teaql_core::Value>) -> Self {
        self.query = self.query.and_filter(Expr::eq("count", value));
        self
    }



    pub fn with_count_is_not(mut self, value: impl Into<teaql_core::Value>) -> Self {
        self.query = self.query.and_filter(Expr::ne("count", value));
        self
    }

    pub fn with_count_greater_than(mut self, value: impl Into<teaql_core::Value>) -> Self {
        self.query = self.query.and_filter(Expr::gt("count", value));
        self
    }

    pub fn with_count_greater_than_or_equal_to(mut self, value: impl Into<teaql_core::Value>) -> Self {
        self.query = self.query.and_filter(Expr::gte("count", value));
        self
    }

    pub fn with_count_less_than(mut self, value: impl Into<teaql_core::Value>) -> Self {
        self.query = self.query.and_filter(Expr::lt("count", value));
        self
    }

    pub fn with_count_less_than_or_equal_to(mut self, value: impl Into<teaql_core::Value>) -> Self {
        self.query = self.query.and_filter(Expr::lte("count", value));
        self
    }

    pub fn with_count_between(
        mut self,
        lower: impl Into<teaql_core::Value>,
        upper: impl Into<teaql_core::Value>,
    ) -> Self {
        self.query = self.query.and_filter(Expr::between("count", lower, upper));
        self
    }

    pub fn with_count_between_range<T>(mut self, range: DateRange<T>) -> Self
    where
        T: Into<teaql_core::Value>,
    {
        self.query = self.query.and_filter(Expr::between(
            "count",
            range.start,
            range.end,
        ));
        self
    }

    pub fn with_count_in(
        mut self,
        values: impl IntoIterator<Item = impl Into<teaql_core::Value>>,
    ) -> Self {
        self.query = self.query.and_filter(Expr::in_list(
            "count",
            values.into_iter().map(Into::into),
        ));
        self
    }

    pub fn with_count_not_in(
        mut self,
        values: impl IntoIterator<Item = impl Into<teaql_core::Value>>,
    ) -> Self {
        self.query = self.query.and_filter(Expr::not_in_list(
            "count",
            values.into_iter().map(Into::into),
        ));
        self
    }

    pub fn with_count_before(mut self, value: impl Into<teaql_core::Value>) -> Self {
        self.query = self.query.and_filter(Expr::lt("count", value));
        self
    }

    pub fn with_count_after(mut self, value: impl Into<teaql_core::Value>) -> Self {
        self.query = self.query.and_filter(Expr::gt("count", value));
        self
    }

    pub fn with_count_is_unknown(mut self) -> Self {
        self.query = self.query.and_filter(Expr::is_null("count"));
        self
    }



    pub fn with_count_is_known(mut self) -> Self {
        self.query = self.query.and_filter(Expr::is_not_null("count"));
        self
    }


    pub fn order_by_count_asc(mut self) -> Self {
        self.query = self.query.order_asc("count");
        self
    }

    pub fn order_by_count_desc(mut self) -> Self {
        self.query = self.query.order_desc("count");
        self
    }

    pub fn order_by_count_asc_using_gbk(mut self) -> Self {
        self.query = self.query.order_gbk_asc("count");
        self
    }

    pub fn order_by_count_desc_using_gbk(mut self) -> Self {
        self.query = self.query.order_gbk_desc("count");
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
    pub fn with_text_is_string(self) -> Self {
        self.with_text_is("string()")
    }



    pub fn with_text_is_not_string(self) -> Self {
        self.with_text_is_not("string()")
    }



    pub fn with_active_is_bool(self) -> Self {
        self.with_active_is("bool()")
    }



    pub fn with_active_is_not_bool(self) -> Self {
        self.with_active_is_not("bool()")
    }



    pub fn with_balance_is_number(self) -> Self {
        self.with_balance_is("number()")
    }



    pub fn with_balance_is_not_number(self) -> Self {
        self.with_balance_is_not("number()")
    }



    pub fn with_height_is_double(self) -> Self {
        self.with_height_is("double()")
    }



    pub fn with_height_is_not_double(self) -> Self {
        self.with_height_is_not("double()")
    }



    pub fn with_ratio_is_float(self) -> Self {
        self.with_ratio_is("float()")
    }



    pub fn with_ratio_is_not_float(self) -> Self {
        self.with_ratio_is_not("float()")
    }



    pub fn with_count_is_integer(self) -> Self {
        self.with_count_is("integer()")
    }



    pub fn with_count_is_not_integer(self) -> Self {
        self.with_count_is_not("integer()")
    }



    pub fn with_day_is_date(self) -> Self {
        self.with_day_is("date()")
    }



    pub fn with_day_is_not_date(self) -> Self {
        self.with_day_is_not("date()")
    }



    pub fn with_recorded_at_is_date_time(self) -> Self {
        self.with_recorded_at_is("dateTime()")
    }



    pub fn with_recorded_at_is_not_date_time(self) -> Self {
        self.with_recorded_at_is_not("dateTime()")
    }



    pub fn with_payload_is_json_me(self) -> Self {
        self.with_payload_is("jsonMe()")
    }



    pub fn with_payload_is_not_json_me(self) -> Self {
        self.with_payload_is_not("jsonMe()")
    }




}

impl<R> Default for ReadingRequest<R> {
    fn default() -> Self {
        Self::new()
    }
}

impl<R> From< ReadingRequest<R> > for SelectQuery {
    fn from(request: ReadingRequest<R>) -> Self {
        QuerySelection::from(request).into_query()
    }
}

impl<R> From< ReadingRequest<R> > for QuerySelection {
    fn from(request: ReadingRequest<R>) -> Self {
        Self {
            query: request.query,
            relation_selections: request.relation_selections,
            relation_filters: request.relation_filters,
            child_enhancements: request.child_enhancements,
            query_options: request.query_options,
        }
    }
}


impl<'a, C> crate::request_support::AuditedSave<'a, C> for teaql_core::Audited<crate::Reading> 
where C: crate::TeaqlRuntime + ?Sized + 'a
{
    type Error = teaql_runtime::RuntimeError;
    type Entity = crate::Reading;
    fn save(self, context: &'a C) -> std::pin::Pin<Box<dyn std::future::Future<Output = Result<Self::Entity, Self::Error>> + Send + '_>> {
        Box::pin(async move {
            context.save_audited_entity(self).await
        })
    }
}

impl<R: teaql_core::Entity> crate::PurposedQuery<ReadingRequest<R>> {
    pub fn comment(mut self, comment: impl Into<String>) -> Self {
        self.inner.query_options.comment = Some(comment.into());
        self
    }

    pub fn new_entity<C>(&self, context: &C) -> crate::Reading
    where
        C: crate::TeaqlRuntime + ?Sized,
    {
        self.require_comment();
        let mut entity = crate::Reading::runtime_new(context.user_context().entity_runtime_state());
        if let Ok(id) = context.user_context().next_id(crate::Reading::ENTITY_NAME) {
            entity.update_id(id);
        }
        teaql_core::Entity::mark_as_new(&mut entity);
        entity
    }

    fn into_inner_with_trace(mut self) -> ReadingRequest<R> {
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
