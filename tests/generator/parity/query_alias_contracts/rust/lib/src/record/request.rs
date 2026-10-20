use std::marker::PhantomData;

use serde_json::Value as JsonValue;
use teaql_core::{Aggregate, AggregateFunction, EntityDescriptor, Expr, SelectQuery, SmartList};
use teaql_runtime::RuntimeError;

use crate::request_support::*;

impl EntityReference for crate::Record {
    fn entity_id_value(self) -> teaql_core::Value {
        teaql_core::IdentifiableEntity::id_value(&self)
    }
}

impl EntityReference for &crate::Record {
    fn entity_id_value(self) -> teaql_core::Value {
        teaql_core::IdentifiableEntity::id_value(self)
    }
}

// ⛔ AI agents: DO NOT read this file for API discovery. Instead run: cargo teaql --input modeling/MODEL.xml rust-assist-query/record
#[derive(Debug)]
pub struct RecordRequest<R = crate::Record> {
    query: SelectQuery,
    relation_selections: Vec<RelationSelection>,
    relation_filters: Vec<RelationFilter>,
    child_enhancements: Vec<QuerySelection>,
    query_options: QueryOptions,
    marker: PhantomData<R>,
}

impl<R> Clone for RecordRequest<R> {
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

impl<R> RecordRequest<R> {
    pub(crate) fn new() -> Self {
        Self {
            query: SelectQuery::new("Record")
                .project("id")
                .project("version"),
            relation_selections: Vec::new(),
            relation_filters: Vec::new(),
            child_enhancements: Vec::new(),
            query_options: QueryOptions::default(),
            marker: PhantomData,
        }
    }

    pub fn return_type<T>(self) -> RecordRequest<T> {
        RecordRequest {
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
                "Record",
                query,
                relation_aggregates,
            ).await?;
            (rows, std::collections::BTreeMap::new())
        } else {
            let rows = context.fetch_entity_smart_list::<R>(
                "Record",
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
        context.fetch_compact_smart_list("Record", &query).await
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
        Ok(context.fetch_entity_stream("Record", query))
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
        let rows = context.fetch_compact_rows("Record", &query).await?;
        rows.first()
            .and_then(|row| row.get(COUNT_ALIAS))
            .and_then(teaql_core::Value::try_u64)
            .ok_or_else(|| RuntimeError::Graph(format!("count result for Record is missing or not numeric")))
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
        let rows = context.fetch_compact_rows("Record", &query).await?;
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
            "class" => Some("class"),
            "operator" => Some("operator"),
            "template" => Some("template"),
            "type" => Some("type"),
            "match" => Some("match"),
            "ordinary" => Some("ordinary"),
            "version" => Some("version"),
            "platform" | "platform_id" => Some("platform_id"),
            "namespace" | "namespace_id" => Some("namespace_id"),
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
            "namespace" => {
                self.with_namespace_matching(
                    crate::Q::platforms_minimal()
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
        self.query = self.query.project("class");
        self.query = self.query.project("operator");
        self.query = self.query.project("template");
        self.query = self.query.project("type");
        self.query = self.query.project("match");
        self.query = self.query.project("ordinary");
        self.query = self.query.project("version");
        self.query = self.query.project("platform_id");
        self.query = self.query.project("namespace_id");
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
        request = request.select_namespace();
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


    pub fn select_class(mut self) -> Self {
        self.query = self.query.project("class");
        self
    }

    pub fn project_class(self) -> Self {
        self.select_class()
    }

    pub fn select_class_raw(self, raw_sql_segment: impl Into<String>) -> Self {
        self.select_class_unsafe_raw(UnsafeRawSqlSegment::trusted(raw_sql_segment))
    }

    pub fn select_class_unsafe_raw(mut self, raw_sql_segment: UnsafeRawSqlSegment) -> Self {
        self.query_options
            .raw_projections
            .push(RawProjection::new("class", raw_sql_segment));
        self
    }

    pub fn select_class_with_function(self, function: AggregateFunction) -> Self {
        self.select_class_as_with_function("class", function)
    }

    pub fn select_class_as_with_function(
        self,
        alias: impl Into<String>,
        function: AggregateFunction,
    ) -> Self {
        self.aggregate_with_function("class", alias, function)
    }

    pub fn group_by_class(self) -> Self {
        self.group_by("class")
    }

    pub fn group_by_class_as(self, alias: impl Into<String>) -> Self {
        let alias = alias.into();
        let mut request = self.group_by("class");
        request.query = request
            .query
            .project_expr(alias, Expr::column("class"));
        request
    }

    pub fn group_by_class_with_function(
        self,
        alias: impl Into<String>,
        function: AggregateFunction,
    ) -> Self {
        self.group_by("class")
            .aggregate_with_function("class", alias, function)
    }

    pub fn count_class(self) -> Self {
        self.count_class_as("class_count")
    }

    pub fn count_class_as(self, alias: impl Into<String>) -> Self {
        self.aggregate_count_field("class", alias)
    }

    pub fn sum_class(self) -> Self {
        self.sum_class_as("sum_class")
    }

    pub fn sum_class_as(self, alias: impl Into<String>) -> Self {
        self.aggregate_sum("class", alias)
    }

    pub fn avg_class(self) -> Self {
        self.avg_class_as("avg_class")
    }

    pub fn avg_class_as(self, alias: impl Into<String>) -> Self {
        self.aggregate_avg("class", alias)
    }

    pub fn min_class(self) -> Self {
        self.min_class_as("min_class")
    }

    pub fn min_class_as(self, alias: impl Into<String>) -> Self {
        self.aggregate_min("class", alias)
    }

    pub fn max_class(self) -> Self {
        self.max_class_as("max_class")
    }

    pub fn max_class_as(self, alias: impl Into<String>) -> Self {
        self.aggregate_max("class", alias)
    }

    pub fn standard_deviation_class(self) -> Self {
        self.standard_deviation_class_as("stdDev_class")
    }

    pub fn standard_deviation_class_as(self, alias: impl Into<String>) -> Self {
        self.aggregate_stddev("class", alias)
    }

    pub fn square_root_of_population_standard_deviation_class(self) -> Self {
        self.square_root_of_population_standard_deviation_class_as("stdDevPop_class")
    }

    pub fn square_root_of_population_standard_deviation_class_as(self, alias: impl Into<String>) -> Self {
        self.aggregate_stddev_pop("class", alias)
    }

    pub fn sample_variance_class(self) -> Self {
        self.sample_variance_class_as("varSamp_class")
    }

    pub fn sample_variance_class_as(self, alias: impl Into<String>) -> Self {
        self.aggregate_var_samp("class", alias)
    }

    pub fn sample_population_variance_class(self) -> Self {
        self.sample_population_variance_class_as("varPop_class")
    }

    pub fn sample_population_variance_class_as(self, alias: impl Into<String>) -> Self {
        self.aggregate_var_pop("class", alias)
    }

    pub fn unselect_class(mut self) -> Self {
        self.query.projection.retain(|field| field != "class");
        self.query_options.raw_projections.retain(|projection| projection.property_name != "class");
        self
    }


    pub fn with_class(
        mut self,
        operator: FieldOperator,
        values: impl IntoIterator<Item = impl Into<teaql_core::Value>>,
    ) -> Self {
        self.query = self.query.and_filter(field_operator_expr(
            "class",
            operator,
            values.into_iter().map(Into::into).collect(),
        ));
        self
    }

    pub fn create_class_criteria(
        operator: FieldOperator,
        values: impl IntoIterator<Item = impl Into<teaql_core::Value>>,
    ) -> Expr {
        field_operator_expr(
            "class",
            operator,
            values.into_iter().map(Into::into).collect(),
        )
    }

    pub fn with_class_is(mut self, value: impl Into<teaql_core::Value>) -> Self {
        self.query = self.query.and_filter(Expr::eq("class", value));
        self
    }



    pub fn with_class_is_not(mut self, value: impl Into<teaql_core::Value>) -> Self {
        self.query = self.query.and_filter(Expr::ne("class", value));
        self
    }

    pub fn with_class_greater_than(mut self, value: impl Into<teaql_core::Value>) -> Self {
        self.query = self.query.and_filter(Expr::gt("class", value));
        self
    }

    pub fn with_class_greater_than_or_equal_to(mut self, value: impl Into<teaql_core::Value>) -> Self {
        self.query = self.query.and_filter(Expr::gte("class", value));
        self
    }

    pub fn with_class_less_than(mut self, value: impl Into<teaql_core::Value>) -> Self {
        self.query = self.query.and_filter(Expr::lt("class", value));
        self
    }

    pub fn with_class_less_than_or_equal_to(mut self, value: impl Into<teaql_core::Value>) -> Self {
        self.query = self.query.and_filter(Expr::lte("class", value));
        self
    }

    pub fn with_class_between(
        mut self,
        lower: impl Into<teaql_core::Value>,
        upper: impl Into<teaql_core::Value>,
    ) -> Self {
        self.query = self.query.and_filter(Expr::between("class", lower, upper));
        self
    }

    pub fn with_class_between_range<T>(mut self, range: DateRange<T>) -> Self
    where
        T: Into<teaql_core::Value>,
    {
        self.query = self.query.and_filter(Expr::between(
            "class",
            range.start,
            range.end,
        ));
        self
    }

    pub fn with_class_in(
        mut self,
        values: impl IntoIterator<Item = impl Into<teaql_core::Value>>,
    ) -> Self {
        self.query = self.query.and_filter(Expr::in_list(
            "class",
            values.into_iter().map(Into::into),
        ));
        self
    }

    pub fn with_class_not_in(
        mut self,
        values: impl IntoIterator<Item = impl Into<teaql_core::Value>>,
    ) -> Self {
        self.query = self.query.and_filter(Expr::not_in_list(
            "class",
            values.into_iter().map(Into::into),
        ));
        self
    }

    pub fn with_class_before(mut self, value: impl Into<teaql_core::Value>) -> Self {
        self.query = self.query.and_filter(Expr::lt("class", value));
        self
    }

    pub fn with_class_after(mut self, value: impl Into<teaql_core::Value>) -> Self {
        self.query = self.query.and_filter(Expr::gt("class", value));
        self
    }

    pub fn with_class_is_unknown(mut self) -> Self {
        self.query = self.query.and_filter(Expr::is_null("class"));
        self
    }



    pub fn with_class_is_known(mut self) -> Self {
        self.query = self.query.and_filter(Expr::is_not_null("class"));
        self
    }


    pub fn order_by_class_asc(mut self) -> Self {
        self.query = self.query.order_asc("class");
        self
    }

    pub fn order_by_class_desc(mut self) -> Self {
        self.query = self.query.order_desc("class");
        self
    }

    pub fn order_by_class_asc_using_gbk(mut self) -> Self {
        self.query = self.query.order_gbk_asc("class");
        self
    }

    pub fn order_by_class_desc_using_gbk(mut self) -> Self {
        self.query = self.query.order_gbk_desc("class");
        self
    }


    pub fn select_operator(mut self) -> Self {
        self.query = self.query.project("operator");
        self
    }

    pub fn project_operator(self) -> Self {
        self.select_operator()
    }

    pub fn select_operator_raw(self, raw_sql_segment: impl Into<String>) -> Self {
        self.select_operator_unsafe_raw(UnsafeRawSqlSegment::trusted(raw_sql_segment))
    }

    pub fn select_operator_unsafe_raw(mut self, raw_sql_segment: UnsafeRawSqlSegment) -> Self {
        self.query_options
            .raw_projections
            .push(RawProjection::new("operator", raw_sql_segment));
        self
    }

    pub fn select_operator_with_function(self, function: AggregateFunction) -> Self {
        self.select_operator_as_with_function("operator", function)
    }

    pub fn select_operator_as_with_function(
        self,
        alias: impl Into<String>,
        function: AggregateFunction,
    ) -> Self {
        self.aggregate_with_function("operator", alias, function)
    }

    pub fn group_by_operator(self) -> Self {
        self.group_by("operator")
    }

    pub fn group_by_operator_as(self, alias: impl Into<String>) -> Self {
        let alias = alias.into();
        let mut request = self.group_by("operator");
        request.query = request
            .query
            .project_expr(alias, Expr::column("operator"));
        request
    }

    pub fn group_by_operator_with_function(
        self,
        alias: impl Into<String>,
        function: AggregateFunction,
    ) -> Self {
        self.group_by("operator")
            .aggregate_with_function("operator", alias, function)
    }

    pub fn count_operator(self) -> Self {
        self.count_operator_as("operator_count")
    }

    pub fn count_operator_as(self, alias: impl Into<String>) -> Self {
        self.aggregate_count_field("operator", alias)
    }

    pub fn sum_operator(self) -> Self {
        self.sum_operator_as("sum_operator")
    }

    pub fn sum_operator_as(self, alias: impl Into<String>) -> Self {
        self.aggregate_sum("operator", alias)
    }

    pub fn avg_operator(self) -> Self {
        self.avg_operator_as("avg_operator")
    }

    pub fn avg_operator_as(self, alias: impl Into<String>) -> Self {
        self.aggregate_avg("operator", alias)
    }

    pub fn min_operator(self) -> Self {
        self.min_operator_as("min_operator")
    }

    pub fn min_operator_as(self, alias: impl Into<String>) -> Self {
        self.aggregate_min("operator", alias)
    }

    pub fn max_operator(self) -> Self {
        self.max_operator_as("max_operator")
    }

    pub fn max_operator_as(self, alias: impl Into<String>) -> Self {
        self.aggregate_max("operator", alias)
    }

    pub fn standard_deviation_operator(self) -> Self {
        self.standard_deviation_operator_as("stdDev_operator")
    }

    pub fn standard_deviation_operator_as(self, alias: impl Into<String>) -> Self {
        self.aggregate_stddev("operator", alias)
    }

    pub fn square_root_of_population_standard_deviation_operator(self) -> Self {
        self.square_root_of_population_standard_deviation_operator_as("stdDevPop_operator")
    }

    pub fn square_root_of_population_standard_deviation_operator_as(self, alias: impl Into<String>) -> Self {
        self.aggregate_stddev_pop("operator", alias)
    }

    pub fn sample_variance_operator(self) -> Self {
        self.sample_variance_operator_as("varSamp_operator")
    }

    pub fn sample_variance_operator_as(self, alias: impl Into<String>) -> Self {
        self.aggregate_var_samp("operator", alias)
    }

    pub fn sample_population_variance_operator(self) -> Self {
        self.sample_population_variance_operator_as("varPop_operator")
    }

    pub fn sample_population_variance_operator_as(self, alias: impl Into<String>) -> Self {
        self.aggregate_var_pop("operator", alias)
    }

    pub fn unselect_operator(mut self) -> Self {
        self.query.projection.retain(|field| field != "operator");
        self.query_options.raw_projections.retain(|projection| projection.property_name != "operator");
        self
    }


    pub fn with_operator(
        mut self,
        operator: FieldOperator,
        values: impl IntoIterator<Item = impl Into<teaql_core::Value>>,
    ) -> Self {
        self.query = self.query.and_filter(field_operator_expr(
            "operator",
            operator,
            values.into_iter().map(Into::into).collect(),
        ));
        self
    }

    pub fn create_operator_criteria(
        operator: FieldOperator,
        values: impl IntoIterator<Item = impl Into<teaql_core::Value>>,
    ) -> Expr {
        field_operator_expr(
            "operator",
            operator,
            values.into_iter().map(Into::into).collect(),
        )
    }

    pub fn with_operator_is(mut self, value: impl Into<teaql_core::Value>) -> Self {
        self.query = self.query.and_filter(Expr::eq("operator", value));
        self
    }



    pub fn with_operator_is_not(mut self, value: impl Into<teaql_core::Value>) -> Self {
        self.query = self.query.and_filter(Expr::ne("operator", value));
        self
    }

    pub fn with_operator_greater_than(mut self, value: impl Into<teaql_core::Value>) -> Self {
        self.query = self.query.and_filter(Expr::gt("operator", value));
        self
    }

    pub fn with_operator_greater_than_or_equal_to(mut self, value: impl Into<teaql_core::Value>) -> Self {
        self.query = self.query.and_filter(Expr::gte("operator", value));
        self
    }

    pub fn with_operator_less_than(mut self, value: impl Into<teaql_core::Value>) -> Self {
        self.query = self.query.and_filter(Expr::lt("operator", value));
        self
    }

    pub fn with_operator_less_than_or_equal_to(mut self, value: impl Into<teaql_core::Value>) -> Self {
        self.query = self.query.and_filter(Expr::lte("operator", value));
        self
    }

    pub fn with_operator_between(
        mut self,
        lower: impl Into<teaql_core::Value>,
        upper: impl Into<teaql_core::Value>,
    ) -> Self {
        self.query = self.query.and_filter(Expr::between("operator", lower, upper));
        self
    }

    pub fn with_operator_between_range<T>(mut self, range: DateRange<T>) -> Self
    where
        T: Into<teaql_core::Value>,
    {
        self.query = self.query.and_filter(Expr::between(
            "operator",
            range.start,
            range.end,
        ));
        self
    }

    pub fn with_operator_in(
        mut self,
        values: impl IntoIterator<Item = impl Into<teaql_core::Value>>,
    ) -> Self {
        self.query = self.query.and_filter(Expr::in_list(
            "operator",
            values.into_iter().map(Into::into),
        ));
        self
    }

    pub fn with_operator_not_in(
        mut self,
        values: impl IntoIterator<Item = impl Into<teaql_core::Value>>,
    ) -> Self {
        self.query = self.query.and_filter(Expr::not_in_list(
            "operator",
            values.into_iter().map(Into::into),
        ));
        self
    }

    pub fn with_operator_before(mut self, value: impl Into<teaql_core::Value>) -> Self {
        self.query = self.query.and_filter(Expr::lt("operator", value));
        self
    }

    pub fn with_operator_after(mut self, value: impl Into<teaql_core::Value>) -> Self {
        self.query = self.query.and_filter(Expr::gt("operator", value));
        self
    }

    pub fn with_operator_is_unknown(mut self) -> Self {
        self.query = self.query.and_filter(Expr::is_null("operator"));
        self
    }



    pub fn with_operator_is_known(mut self) -> Self {
        self.query = self.query.and_filter(Expr::is_not_null("operator"));
        self
    }


    pub fn order_by_operator_asc(mut self) -> Self {
        self.query = self.query.order_asc("operator");
        self
    }

    pub fn order_by_operator_desc(mut self) -> Self {
        self.query = self.query.order_desc("operator");
        self
    }

    pub fn order_by_operator_asc_using_gbk(mut self) -> Self {
        self.query = self.query.order_gbk_asc("operator");
        self
    }

    pub fn order_by_operator_desc_using_gbk(mut self) -> Self {
        self.query = self.query.order_gbk_desc("operator");
        self
    }


    pub fn select_template(mut self) -> Self {
        self.query = self.query.project("template");
        self
    }

    pub fn project_template(self) -> Self {
        self.select_template()
    }

    pub fn select_template_raw(self, raw_sql_segment: impl Into<String>) -> Self {
        self.select_template_unsafe_raw(UnsafeRawSqlSegment::trusted(raw_sql_segment))
    }

    pub fn select_template_unsafe_raw(mut self, raw_sql_segment: UnsafeRawSqlSegment) -> Self {
        self.query_options
            .raw_projections
            .push(RawProjection::new("template", raw_sql_segment));
        self
    }

    pub fn select_template_with_function(self, function: AggregateFunction) -> Self {
        self.select_template_as_with_function("template", function)
    }

    pub fn select_template_as_with_function(
        self,
        alias: impl Into<String>,
        function: AggregateFunction,
    ) -> Self {
        self.aggregate_with_function("template", alias, function)
    }

    pub fn group_by_template(self) -> Self {
        self.group_by("template")
    }

    pub fn group_by_template_as(self, alias: impl Into<String>) -> Self {
        let alias = alias.into();
        let mut request = self.group_by("template");
        request.query = request
            .query
            .project_expr(alias, Expr::column("template"));
        request
    }

    pub fn group_by_template_with_function(
        self,
        alias: impl Into<String>,
        function: AggregateFunction,
    ) -> Self {
        self.group_by("template")
            .aggregate_with_function("template", alias, function)
    }

    pub fn count_template(self) -> Self {
        self.count_template_as("template_count")
    }

    pub fn count_template_as(self, alias: impl Into<String>) -> Self {
        self.aggregate_count_field("template", alias)
    }

    pub fn sum_template(self) -> Self {
        self.sum_template_as("sum_template")
    }

    pub fn sum_template_as(self, alias: impl Into<String>) -> Self {
        self.aggregate_sum("template", alias)
    }

    pub fn avg_template(self) -> Self {
        self.avg_template_as("avg_template")
    }

    pub fn avg_template_as(self, alias: impl Into<String>) -> Self {
        self.aggregate_avg("template", alias)
    }

    pub fn min_template(self) -> Self {
        self.min_template_as("min_template")
    }

    pub fn min_template_as(self, alias: impl Into<String>) -> Self {
        self.aggregate_min("template", alias)
    }

    pub fn max_template(self) -> Self {
        self.max_template_as("max_template")
    }

    pub fn max_template_as(self, alias: impl Into<String>) -> Self {
        self.aggregate_max("template", alias)
    }

    pub fn standard_deviation_template(self) -> Self {
        self.standard_deviation_template_as("stdDev_template")
    }

    pub fn standard_deviation_template_as(self, alias: impl Into<String>) -> Self {
        self.aggregate_stddev("template", alias)
    }

    pub fn square_root_of_population_standard_deviation_template(self) -> Self {
        self.square_root_of_population_standard_deviation_template_as("stdDevPop_template")
    }

    pub fn square_root_of_population_standard_deviation_template_as(self, alias: impl Into<String>) -> Self {
        self.aggregate_stddev_pop("template", alias)
    }

    pub fn sample_variance_template(self) -> Self {
        self.sample_variance_template_as("varSamp_template")
    }

    pub fn sample_variance_template_as(self, alias: impl Into<String>) -> Self {
        self.aggregate_var_samp("template", alias)
    }

    pub fn sample_population_variance_template(self) -> Self {
        self.sample_population_variance_template_as("varPop_template")
    }

    pub fn sample_population_variance_template_as(self, alias: impl Into<String>) -> Self {
        self.aggregate_var_pop("template", alias)
    }

    pub fn unselect_template(mut self) -> Self {
        self.query.projection.retain(|field| field != "template");
        self.query_options.raw_projections.retain(|projection| projection.property_name != "template");
        self
    }


    pub fn with_template(
        mut self,
        operator: FieldOperator,
        values: impl IntoIterator<Item = impl Into<teaql_core::Value>>,
    ) -> Self {
        self.query = self.query.and_filter(field_operator_expr(
            "template",
            operator,
            values.into_iter().map(Into::into).collect(),
        ));
        self
    }

    pub fn create_template_criteria(
        operator: FieldOperator,
        values: impl IntoIterator<Item = impl Into<teaql_core::Value>>,
    ) -> Expr {
        field_operator_expr(
            "template",
            operator,
            values.into_iter().map(Into::into).collect(),
        )
    }

    pub fn with_template_is(mut self, value: impl Into<teaql_core::Value>) -> Self {
        self.query = self.query.and_filter(Expr::eq("template", value));
        self
    }



    pub fn with_template_is_not(mut self, value: impl Into<teaql_core::Value>) -> Self {
        self.query = self.query.and_filter(Expr::ne("template", value));
        self
    }

    pub fn with_template_greater_than(mut self, value: impl Into<teaql_core::Value>) -> Self {
        self.query = self.query.and_filter(Expr::gt("template", value));
        self
    }

    pub fn with_template_greater_than_or_equal_to(mut self, value: impl Into<teaql_core::Value>) -> Self {
        self.query = self.query.and_filter(Expr::gte("template", value));
        self
    }

    pub fn with_template_less_than(mut self, value: impl Into<teaql_core::Value>) -> Self {
        self.query = self.query.and_filter(Expr::lt("template", value));
        self
    }

    pub fn with_template_less_than_or_equal_to(mut self, value: impl Into<teaql_core::Value>) -> Self {
        self.query = self.query.and_filter(Expr::lte("template", value));
        self
    }

    pub fn with_template_between(
        mut self,
        lower: impl Into<teaql_core::Value>,
        upper: impl Into<teaql_core::Value>,
    ) -> Self {
        self.query = self.query.and_filter(Expr::between("template", lower, upper));
        self
    }

    pub fn with_template_between_range<T>(mut self, range: DateRange<T>) -> Self
    where
        T: Into<teaql_core::Value>,
    {
        self.query = self.query.and_filter(Expr::between(
            "template",
            range.start,
            range.end,
        ));
        self
    }

    pub fn with_template_in(
        mut self,
        values: impl IntoIterator<Item = impl Into<teaql_core::Value>>,
    ) -> Self {
        self.query = self.query.and_filter(Expr::in_list(
            "template",
            values.into_iter().map(Into::into),
        ));
        self
    }

    pub fn with_template_not_in(
        mut self,
        values: impl IntoIterator<Item = impl Into<teaql_core::Value>>,
    ) -> Self {
        self.query = self.query.and_filter(Expr::not_in_list(
            "template",
            values.into_iter().map(Into::into),
        ));
        self
    }

    pub fn with_template_before(mut self, value: impl Into<teaql_core::Value>) -> Self {
        self.query = self.query.and_filter(Expr::lt("template", value));
        self
    }

    pub fn with_template_after(mut self, value: impl Into<teaql_core::Value>) -> Self {
        self.query = self.query.and_filter(Expr::gt("template", value));
        self
    }

    pub fn with_template_is_unknown(mut self) -> Self {
        self.query = self.query.and_filter(Expr::is_null("template"));
        self
    }



    pub fn with_template_is_known(mut self) -> Self {
        self.query = self.query.and_filter(Expr::is_not_null("template"));
        self
    }


    pub fn order_by_template_asc(mut self) -> Self {
        self.query = self.query.order_asc("template");
        self
    }

    pub fn order_by_template_desc(mut self) -> Self {
        self.query = self.query.order_desc("template");
        self
    }

    pub fn order_by_template_asc_using_gbk(mut self) -> Self {
        self.query = self.query.order_gbk_asc("template");
        self
    }

    pub fn order_by_template_desc_using_gbk(mut self) -> Self {
        self.query = self.query.order_gbk_desc("template");
        self
    }


    pub fn select_type_field(mut self) -> Self {
        self.query = self.query.project("type");
        self
    }

    pub fn project_type_field(self) -> Self {
        self.select_type_field()
    }

    pub fn select_type_field_raw(self, raw_sql_segment: impl Into<String>) -> Self {
        self.select_type_field_unsafe_raw(UnsafeRawSqlSegment::trusted(raw_sql_segment))
    }

    pub fn select_type_field_unsafe_raw(mut self, raw_sql_segment: UnsafeRawSqlSegment) -> Self {
        self.query_options
            .raw_projections
            .push(RawProjection::new("type", raw_sql_segment));
        self
    }

    pub fn select_type_field_with_function(self, function: AggregateFunction) -> Self {
        self.select_type_field_as_with_function("type", function)
    }

    pub fn select_type_field_as_with_function(
        self,
        alias: impl Into<String>,
        function: AggregateFunction,
    ) -> Self {
        self.aggregate_with_function("type", alias, function)
    }

    pub fn group_by_type_field(self) -> Self {
        self.group_by("type")
    }

    pub fn group_by_type_field_as(self, alias: impl Into<String>) -> Self {
        let alias = alias.into();
        let mut request = self.group_by("type");
        request.query = request
            .query
            .project_expr(alias, Expr::column("type"));
        request
    }

    pub fn group_by_type_field_with_function(
        self,
        alias: impl Into<String>,
        function: AggregateFunction,
    ) -> Self {
        self.group_by("type")
            .aggregate_with_function("type", alias, function)
    }

    pub fn count_type_field(self) -> Self {
        self.count_type_field_as("type_field_count")
    }

    pub fn count_type_field_as(self, alias: impl Into<String>) -> Self {
        self.aggregate_count_field("type", alias)
    }

    pub fn sum_type_field(self) -> Self {
        self.sum_type_field_as("sum_type_field")
    }

    pub fn sum_type_field_as(self, alias: impl Into<String>) -> Self {
        self.aggregate_sum("type", alias)
    }

    pub fn avg_type_field(self) -> Self {
        self.avg_type_field_as("avg_type_field")
    }

    pub fn avg_type_field_as(self, alias: impl Into<String>) -> Self {
        self.aggregate_avg("type", alias)
    }

    pub fn min_type_field(self) -> Self {
        self.min_type_field_as("min_type_field")
    }

    pub fn min_type_field_as(self, alias: impl Into<String>) -> Self {
        self.aggregate_min("type", alias)
    }

    pub fn max_type_field(self) -> Self {
        self.max_type_field_as("max_type_field")
    }

    pub fn max_type_field_as(self, alias: impl Into<String>) -> Self {
        self.aggregate_max("type", alias)
    }

    pub fn standard_deviation_type_field(self) -> Self {
        self.standard_deviation_type_field_as("stdDev_type_field")
    }

    pub fn standard_deviation_type_field_as(self, alias: impl Into<String>) -> Self {
        self.aggregate_stddev("type", alias)
    }

    pub fn square_root_of_population_standard_deviation_type_field(self) -> Self {
        self.square_root_of_population_standard_deviation_type_field_as("stdDevPop_type_field")
    }

    pub fn square_root_of_population_standard_deviation_type_field_as(self, alias: impl Into<String>) -> Self {
        self.aggregate_stddev_pop("type", alias)
    }

    pub fn sample_variance_type_field(self) -> Self {
        self.sample_variance_type_field_as("varSamp_type_field")
    }

    pub fn sample_variance_type_field_as(self, alias: impl Into<String>) -> Self {
        self.aggregate_var_samp("type", alias)
    }

    pub fn sample_population_variance_type_field(self) -> Self {
        self.sample_population_variance_type_field_as("varPop_type_field")
    }

    pub fn sample_population_variance_type_field_as(self, alias: impl Into<String>) -> Self {
        self.aggregate_var_pop("type", alias)
    }

    pub fn unselect_type_field(mut self) -> Self {
        self.query.projection.retain(|field| field != "type");
        self.query_options.raw_projections.retain(|projection| projection.property_name != "type");
        self
    }


    pub fn with_type(
        mut self,
        operator: FieldOperator,
        values: impl IntoIterator<Item = impl Into<teaql_core::Value>>,
    ) -> Self {
        self.query = self.query.and_filter(field_operator_expr(
            "type",
            operator,
            values.into_iter().map(Into::into).collect(),
        ));
        self
    }

    pub fn create_type_field_criteria(
        operator: FieldOperator,
        values: impl IntoIterator<Item = impl Into<teaql_core::Value>>,
    ) -> Expr {
        field_operator_expr(
            "type",
            operator,
            values.into_iter().map(Into::into).collect(),
        )
    }

    pub fn with_type_is(mut self, value: impl Into<teaql_core::Value>) -> Self {
        self.query = self.query.and_filter(Expr::eq("type", value));
        self
    }



    pub fn with_type_is_not(mut self, value: impl Into<teaql_core::Value>) -> Self {
        self.query = self.query.and_filter(Expr::ne("type", value));
        self
    }

    pub fn with_type_greater_than(mut self, value: impl Into<teaql_core::Value>) -> Self {
        self.query = self.query.and_filter(Expr::gt("type", value));
        self
    }

    pub fn with_type_greater_than_or_equal_to(mut self, value: impl Into<teaql_core::Value>) -> Self {
        self.query = self.query.and_filter(Expr::gte("type", value));
        self
    }

    pub fn with_type_less_than(mut self, value: impl Into<teaql_core::Value>) -> Self {
        self.query = self.query.and_filter(Expr::lt("type", value));
        self
    }

    pub fn with_type_less_than_or_equal_to(mut self, value: impl Into<teaql_core::Value>) -> Self {
        self.query = self.query.and_filter(Expr::lte("type", value));
        self
    }

    pub fn with_type_between(
        mut self,
        lower: impl Into<teaql_core::Value>,
        upper: impl Into<teaql_core::Value>,
    ) -> Self {
        self.query = self.query.and_filter(Expr::between("type", lower, upper));
        self
    }

    pub fn with_type_between_range<T>(mut self, range: DateRange<T>) -> Self
    where
        T: Into<teaql_core::Value>,
    {
        self.query = self.query.and_filter(Expr::between(
            "type",
            range.start,
            range.end,
        ));
        self
    }

    pub fn with_type_in(
        mut self,
        values: impl IntoIterator<Item = impl Into<teaql_core::Value>>,
    ) -> Self {
        self.query = self.query.and_filter(Expr::in_list(
            "type",
            values.into_iter().map(Into::into),
        ));
        self
    }

    pub fn with_type_not_in(
        mut self,
        values: impl IntoIterator<Item = impl Into<teaql_core::Value>>,
    ) -> Self {
        self.query = self.query.and_filter(Expr::not_in_list(
            "type",
            values.into_iter().map(Into::into),
        ));
        self
    }

    pub fn with_type_before(mut self, value: impl Into<teaql_core::Value>) -> Self {
        self.query = self.query.and_filter(Expr::lt("type", value));
        self
    }

    pub fn with_type_after(mut self, value: impl Into<teaql_core::Value>) -> Self {
        self.query = self.query.and_filter(Expr::gt("type", value));
        self
    }

    pub fn with_type_is_unknown(mut self) -> Self {
        self.query = self.query.and_filter(Expr::is_null("type"));
        self
    }



    pub fn with_type_is_known(mut self) -> Self {
        self.query = self.query.and_filter(Expr::is_not_null("type"));
        self
    }


    pub fn order_by_type_field_asc(mut self) -> Self {
        self.query = self.query.order_asc("type");
        self
    }

    pub fn order_by_type_field_desc(mut self) -> Self {
        self.query = self.query.order_desc("type");
        self
    }

    pub fn order_by_type_field_asc_using_gbk(mut self) -> Self {
        self.query = self.query.order_gbk_asc("type");
        self
    }

    pub fn order_by_type_field_desc_using_gbk(mut self) -> Self {
        self.query = self.query.order_gbk_desc("type");
        self
    }


    pub fn select_match_field(mut self) -> Self {
        self.query = self.query.project("match");
        self
    }

    pub fn project_match_field(self) -> Self {
        self.select_match_field()
    }

    pub fn select_match_field_raw(self, raw_sql_segment: impl Into<String>) -> Self {
        self.select_match_field_unsafe_raw(UnsafeRawSqlSegment::trusted(raw_sql_segment))
    }

    pub fn select_match_field_unsafe_raw(mut self, raw_sql_segment: UnsafeRawSqlSegment) -> Self {
        self.query_options
            .raw_projections
            .push(RawProjection::new("match", raw_sql_segment));
        self
    }

    pub fn select_match_field_with_function(self, function: AggregateFunction) -> Self {
        self.select_match_field_as_with_function("match", function)
    }

    pub fn select_match_field_as_with_function(
        self,
        alias: impl Into<String>,
        function: AggregateFunction,
    ) -> Self {
        self.aggregate_with_function("match", alias, function)
    }

    pub fn group_by_match_field(self) -> Self {
        self.group_by("match")
    }

    pub fn group_by_match_field_as(self, alias: impl Into<String>) -> Self {
        let alias = alias.into();
        let mut request = self.group_by("match");
        request.query = request
            .query
            .project_expr(alias, Expr::column("match"));
        request
    }

    pub fn group_by_match_field_with_function(
        self,
        alias: impl Into<String>,
        function: AggregateFunction,
    ) -> Self {
        self.group_by("match")
            .aggregate_with_function("match", alias, function)
    }

    pub fn count_match_field(self) -> Self {
        self.count_match_field_as("match_field_count")
    }

    pub fn count_match_field_as(self, alias: impl Into<String>) -> Self {
        self.aggregate_count_field("match", alias)
    }

    pub fn sum_match_field(self) -> Self {
        self.sum_match_field_as("sum_match_field")
    }

    pub fn sum_match_field_as(self, alias: impl Into<String>) -> Self {
        self.aggregate_sum("match", alias)
    }

    pub fn avg_match_field(self) -> Self {
        self.avg_match_field_as("avg_match_field")
    }

    pub fn avg_match_field_as(self, alias: impl Into<String>) -> Self {
        self.aggregate_avg("match", alias)
    }

    pub fn min_match_field(self) -> Self {
        self.min_match_field_as("min_match_field")
    }

    pub fn min_match_field_as(self, alias: impl Into<String>) -> Self {
        self.aggregate_min("match", alias)
    }

    pub fn max_match_field(self) -> Self {
        self.max_match_field_as("max_match_field")
    }

    pub fn max_match_field_as(self, alias: impl Into<String>) -> Self {
        self.aggregate_max("match", alias)
    }

    pub fn standard_deviation_match_field(self) -> Self {
        self.standard_deviation_match_field_as("stdDev_match_field")
    }

    pub fn standard_deviation_match_field_as(self, alias: impl Into<String>) -> Self {
        self.aggregate_stddev("match", alias)
    }

    pub fn square_root_of_population_standard_deviation_match_field(self) -> Self {
        self.square_root_of_population_standard_deviation_match_field_as("stdDevPop_match_field")
    }

    pub fn square_root_of_population_standard_deviation_match_field_as(self, alias: impl Into<String>) -> Self {
        self.aggregate_stddev_pop("match", alias)
    }

    pub fn sample_variance_match_field(self) -> Self {
        self.sample_variance_match_field_as("varSamp_match_field")
    }

    pub fn sample_variance_match_field_as(self, alias: impl Into<String>) -> Self {
        self.aggregate_var_samp("match", alias)
    }

    pub fn sample_population_variance_match_field(self) -> Self {
        self.sample_population_variance_match_field_as("varPop_match_field")
    }

    pub fn sample_population_variance_match_field_as(self, alias: impl Into<String>) -> Self {
        self.aggregate_var_pop("match", alias)
    }

    pub fn unselect_match_field(mut self) -> Self {
        self.query.projection.retain(|field| field != "match");
        self.query_options.raw_projections.retain(|projection| projection.property_name != "match");
        self
    }


    pub fn with_match(
        mut self,
        operator: FieldOperator,
        values: impl IntoIterator<Item = impl Into<teaql_core::Value>>,
    ) -> Self {
        self.query = self.query.and_filter(field_operator_expr(
            "match",
            operator,
            values.into_iter().map(Into::into).collect(),
        ));
        self
    }

    pub fn create_match_field_criteria(
        operator: FieldOperator,
        values: impl IntoIterator<Item = impl Into<teaql_core::Value>>,
    ) -> Expr {
        field_operator_expr(
            "match",
            operator,
            values.into_iter().map(Into::into).collect(),
        )
    }

    pub fn with_match_is(mut self, value: impl Into<teaql_core::Value>) -> Self {
        self.query = self.query.and_filter(Expr::eq("match", value));
        self
    }



    pub fn with_match_is_not(mut self, value: impl Into<teaql_core::Value>) -> Self {
        self.query = self.query.and_filter(Expr::ne("match", value));
        self
    }

    pub fn with_match_greater_than(mut self, value: impl Into<teaql_core::Value>) -> Self {
        self.query = self.query.and_filter(Expr::gt("match", value));
        self
    }

    pub fn with_match_greater_than_or_equal_to(mut self, value: impl Into<teaql_core::Value>) -> Self {
        self.query = self.query.and_filter(Expr::gte("match", value));
        self
    }

    pub fn with_match_less_than(mut self, value: impl Into<teaql_core::Value>) -> Self {
        self.query = self.query.and_filter(Expr::lt("match", value));
        self
    }

    pub fn with_match_less_than_or_equal_to(mut self, value: impl Into<teaql_core::Value>) -> Self {
        self.query = self.query.and_filter(Expr::lte("match", value));
        self
    }

    pub fn with_match_between(
        mut self,
        lower: impl Into<teaql_core::Value>,
        upper: impl Into<teaql_core::Value>,
    ) -> Self {
        self.query = self.query.and_filter(Expr::between("match", lower, upper));
        self
    }

    pub fn with_match_between_range<T>(mut self, range: DateRange<T>) -> Self
    where
        T: Into<teaql_core::Value>,
    {
        self.query = self.query.and_filter(Expr::between(
            "match",
            range.start,
            range.end,
        ));
        self
    }

    pub fn with_match_in(
        mut self,
        values: impl IntoIterator<Item = impl Into<teaql_core::Value>>,
    ) -> Self {
        self.query = self.query.and_filter(Expr::in_list(
            "match",
            values.into_iter().map(Into::into),
        ));
        self
    }

    pub fn with_match_not_in(
        mut self,
        values: impl IntoIterator<Item = impl Into<teaql_core::Value>>,
    ) -> Self {
        self.query = self.query.and_filter(Expr::not_in_list(
            "match",
            values.into_iter().map(Into::into),
        ));
        self
    }

    pub fn with_match_before(mut self, value: impl Into<teaql_core::Value>) -> Self {
        self.query = self.query.and_filter(Expr::lt("match", value));
        self
    }

    pub fn with_match_after(mut self, value: impl Into<teaql_core::Value>) -> Self {
        self.query = self.query.and_filter(Expr::gt("match", value));
        self
    }

    pub fn with_match_is_unknown(mut self) -> Self {
        self.query = self.query.and_filter(Expr::is_null("match"));
        self
    }



    pub fn with_match_is_known(mut self) -> Self {
        self.query = self.query.and_filter(Expr::is_not_null("match"));
        self
    }


    pub fn order_by_match_field_asc(mut self) -> Self {
        self.query = self.query.order_asc("match");
        self
    }

    pub fn order_by_match_field_desc(mut self) -> Self {
        self.query = self.query.order_desc("match");
        self
    }

    pub fn order_by_match_field_asc_using_gbk(mut self) -> Self {
        self.query = self.query.order_gbk_asc("match");
        self
    }

    pub fn order_by_match_field_desc_using_gbk(mut self) -> Self {
        self.query = self.query.order_gbk_desc("match");
        self
    }


    pub fn select_ordinary(mut self) -> Self {
        self.query = self.query.project("ordinary");
        self
    }

    pub fn project_ordinary(self) -> Self {
        self.select_ordinary()
    }

    pub fn select_ordinary_raw(self, raw_sql_segment: impl Into<String>) -> Self {
        self.select_ordinary_unsafe_raw(UnsafeRawSqlSegment::trusted(raw_sql_segment))
    }

    pub fn select_ordinary_unsafe_raw(mut self, raw_sql_segment: UnsafeRawSqlSegment) -> Self {
        self.query_options
            .raw_projections
            .push(RawProjection::new("ordinary", raw_sql_segment));
        self
    }

    pub fn select_ordinary_with_function(self, function: AggregateFunction) -> Self {
        self.select_ordinary_as_with_function("ordinary", function)
    }

    pub fn select_ordinary_as_with_function(
        self,
        alias: impl Into<String>,
        function: AggregateFunction,
    ) -> Self {
        self.aggregate_with_function("ordinary", alias, function)
    }

    pub fn group_by_ordinary(self) -> Self {
        self.group_by("ordinary")
    }

    pub fn group_by_ordinary_as(self, alias: impl Into<String>) -> Self {
        let alias = alias.into();
        let mut request = self.group_by("ordinary");
        request.query = request
            .query
            .project_expr(alias, Expr::column("ordinary"));
        request
    }

    pub fn group_by_ordinary_with_function(
        self,
        alias: impl Into<String>,
        function: AggregateFunction,
    ) -> Self {
        self.group_by("ordinary")
            .aggregate_with_function("ordinary", alias, function)
    }

    pub fn count_ordinary(self) -> Self {
        self.count_ordinary_as("ordinary_count")
    }

    pub fn count_ordinary_as(self, alias: impl Into<String>) -> Self {
        self.aggregate_count_field("ordinary", alias)
    }

    pub fn sum_ordinary(self) -> Self {
        self.sum_ordinary_as("sum_ordinary")
    }

    pub fn sum_ordinary_as(self, alias: impl Into<String>) -> Self {
        self.aggregate_sum("ordinary", alias)
    }

    pub fn avg_ordinary(self) -> Self {
        self.avg_ordinary_as("avg_ordinary")
    }

    pub fn avg_ordinary_as(self, alias: impl Into<String>) -> Self {
        self.aggregate_avg("ordinary", alias)
    }

    pub fn min_ordinary(self) -> Self {
        self.min_ordinary_as("min_ordinary")
    }

    pub fn min_ordinary_as(self, alias: impl Into<String>) -> Self {
        self.aggregate_min("ordinary", alias)
    }

    pub fn max_ordinary(self) -> Self {
        self.max_ordinary_as("max_ordinary")
    }

    pub fn max_ordinary_as(self, alias: impl Into<String>) -> Self {
        self.aggregate_max("ordinary", alias)
    }

    pub fn standard_deviation_ordinary(self) -> Self {
        self.standard_deviation_ordinary_as("stdDev_ordinary")
    }

    pub fn standard_deviation_ordinary_as(self, alias: impl Into<String>) -> Self {
        self.aggregate_stddev("ordinary", alias)
    }

    pub fn square_root_of_population_standard_deviation_ordinary(self) -> Self {
        self.square_root_of_population_standard_deviation_ordinary_as("stdDevPop_ordinary")
    }

    pub fn square_root_of_population_standard_deviation_ordinary_as(self, alias: impl Into<String>) -> Self {
        self.aggregate_stddev_pop("ordinary", alias)
    }

    pub fn sample_variance_ordinary(self) -> Self {
        self.sample_variance_ordinary_as("varSamp_ordinary")
    }

    pub fn sample_variance_ordinary_as(self, alias: impl Into<String>) -> Self {
        self.aggregate_var_samp("ordinary", alias)
    }

    pub fn sample_population_variance_ordinary(self) -> Self {
        self.sample_population_variance_ordinary_as("varPop_ordinary")
    }

    pub fn sample_population_variance_ordinary_as(self, alias: impl Into<String>) -> Self {
        self.aggregate_var_pop("ordinary", alias)
    }

    pub fn unselect_ordinary(mut self) -> Self {
        self.query.projection.retain(|field| field != "ordinary");
        self.query_options.raw_projections.retain(|projection| projection.property_name != "ordinary");
        self
    }


    pub fn with_ordinary(
        mut self,
        operator: FieldOperator,
        values: impl IntoIterator<Item = impl Into<teaql_core::Value>>,
    ) -> Self {
        self.query = self.query.and_filter(field_operator_expr(
            "ordinary",
            operator,
            values.into_iter().map(Into::into).collect(),
        ));
        self
    }

    pub fn create_ordinary_criteria(
        operator: FieldOperator,
        values: impl IntoIterator<Item = impl Into<teaql_core::Value>>,
    ) -> Expr {
        field_operator_expr(
            "ordinary",
            operator,
            values.into_iter().map(Into::into).collect(),
        )
    }

    pub fn with_ordinary_is(mut self, value: impl Into<teaql_core::Value>) -> Self {
        self.query = self.query.and_filter(Expr::eq("ordinary", value));
        self
    }



    pub fn with_ordinary_is_not(mut self, value: impl Into<teaql_core::Value>) -> Self {
        self.query = self.query.and_filter(Expr::ne("ordinary", value));
        self
    }

    pub fn with_ordinary_greater_than(mut self, value: impl Into<teaql_core::Value>) -> Self {
        self.query = self.query.and_filter(Expr::gt("ordinary", value));
        self
    }

    pub fn with_ordinary_greater_than_or_equal_to(mut self, value: impl Into<teaql_core::Value>) -> Self {
        self.query = self.query.and_filter(Expr::gte("ordinary", value));
        self
    }

    pub fn with_ordinary_less_than(mut self, value: impl Into<teaql_core::Value>) -> Self {
        self.query = self.query.and_filter(Expr::lt("ordinary", value));
        self
    }

    pub fn with_ordinary_less_than_or_equal_to(mut self, value: impl Into<teaql_core::Value>) -> Self {
        self.query = self.query.and_filter(Expr::lte("ordinary", value));
        self
    }

    pub fn with_ordinary_between(
        mut self,
        lower: impl Into<teaql_core::Value>,
        upper: impl Into<teaql_core::Value>,
    ) -> Self {
        self.query = self.query.and_filter(Expr::between("ordinary", lower, upper));
        self
    }

    pub fn with_ordinary_between_range<T>(mut self, range: DateRange<T>) -> Self
    where
        T: Into<teaql_core::Value>,
    {
        self.query = self.query.and_filter(Expr::between(
            "ordinary",
            range.start,
            range.end,
        ));
        self
    }

    pub fn with_ordinary_in(
        mut self,
        values: impl IntoIterator<Item = impl Into<teaql_core::Value>>,
    ) -> Self {
        self.query = self.query.and_filter(Expr::in_list(
            "ordinary",
            values.into_iter().map(Into::into),
        ));
        self
    }

    pub fn with_ordinary_not_in(
        mut self,
        values: impl IntoIterator<Item = impl Into<teaql_core::Value>>,
    ) -> Self {
        self.query = self.query.and_filter(Expr::not_in_list(
            "ordinary",
            values.into_iter().map(Into::into),
        ));
        self
    }

    pub fn with_ordinary_before(mut self, value: impl Into<teaql_core::Value>) -> Self {
        self.query = self.query.and_filter(Expr::lt("ordinary", value));
        self
    }

    pub fn with_ordinary_after(mut self, value: impl Into<teaql_core::Value>) -> Self {
        self.query = self.query.and_filter(Expr::gt("ordinary", value));
        self
    }

    pub fn with_ordinary_is_unknown(mut self) -> Self {
        self.query = self.query.and_filter(Expr::is_null("ordinary"));
        self
    }



    pub fn with_ordinary_is_known(mut self) -> Self {
        self.query = self.query.and_filter(Expr::is_not_null("ordinary"));
        self
    }


    pub fn order_by_ordinary_asc(mut self) -> Self {
        self.query = self.query.order_asc("ordinary");
        self
    }

    pub fn order_by_ordinary_desc(mut self) -> Self {
        self.query = self.query.order_desc("ordinary");
        self
    }

    pub fn order_by_ordinary_asc_using_gbk(mut self) -> Self {
        self.query = self.query.order_gbk_asc("ordinary");
        self
    }

    pub fn order_by_ordinary_desc_using_gbk(mut self) -> Self {
        self.query = self.query.order_gbk_desc("ordinary");
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


    pub fn filter_by_namespace(mut self, value: impl EntityReference) -> Self {
        self.query = self.query.and_filter(Expr::eq("namespace_id", value.entity_id_value()));
        self
    }

    pub fn with_namespace_matching(mut self, request: impl Into<QuerySelection>) -> Self {
        let selection = request.into();
        self.query = self.query.and_filter(Expr::in_subquery(
            "namespace_id",
            <crate::Platform as teaql_core::TeaqlEntity>::entity_descriptor(),
            selection.query.clone(),
            "id",
        ));
        self.relation_filters.push(RelationFilter::new("namespace", selection));
        self
    }


    pub fn without_namespace_matching(mut self, request: impl Into<QuerySelection>) -> Self {
        let selection = request.into();
        self.query = self.query.and_filter(Expr::not_in_subquery(
            "namespace_id",
            <crate::Platform as teaql_core::TeaqlEntity>::entity_descriptor(),
            selection.query.clone(),
            "id",
        ));
        self.relation_filters.push(RelationFilter::new("namespace", selection));
        self
    }


    pub fn have_namespace(mut self) -> Self {
        self.query = self.query.and_filter(Expr::is_not_null("namespace_id"));
        self
    }

    pub fn have_no_namespace(mut self) -> Self {
        self.query = self.query.and_filter(Expr::is_null("namespace_id"));
        self
    }


    pub fn group_by_namespace(self) -> Self {
        self.group_by("namespace_id")
    }

    pub fn group_by_namespace_as(self, alias: impl Into<String>) -> Self {
        let alias = alias.into();
        let mut request = self.group_by("namespace_id");
        request.query = request
            .query
            .project_expr(alias, Expr::column("namespace_id"));
        request
    }

    pub fn group_by_namespace_with_function(
        self,
        alias: impl Into<String>,
        function: AggregateFunction,
    ) -> Self {
        self.group_by("namespace_id")
            .aggregate_with_function("namespace_id", alias, function)
    }

    pub fn group_by_namespace_with(mut self, request: impl Into<QuerySelection>) -> Self {
        self.query = self.query.group_by("namespace_id");
        self.query_options.object_group_bys.push(ObjectGroupBy::new(
            "namespace",
            "namespace_id",
            request,
        ));
        self
    }

    pub fn group_by_namespace_with_details(self) -> Self {
        self.group_by_namespace_with_details_from(crate::Q::platforms().unlimited())
    }

    pub fn group_by_namespace_with_details_from(self, request: impl Into<QuerySelection>) -> Self {
        self.group_by_namespace_with(request)
    }


    pub fn roll_up_to_namespace(self) -> Self {
        self.roll_up_to_namespace_with(crate::Q::platforms().unlimited())
    }

    pub fn roll_up_to_namespace_with(self, request: impl Into<QuerySelection>) -> Self {
        let selection = request.into();
        self.with_namespace_matching(selection.clone())
            .group_by_namespace_with(selection)
    }

    pub fn count_namespace(self) -> Self {
        self.count_namespace_as("namespace_count")
    }

    pub fn count_namespace_as(self, alias: impl Into<String>) -> Self {
        self.aggregate_count_field("namespace_id", alias)
    }

    pub fn unselect_namespace(mut self) -> Self {
        self.query.projection.retain(|field| field != "namespace_id");
        self.query.relations.retain(|relation| relation.name != "namespace");
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

    pub fn select_namespace(mut self) -> Self {
        self.query = self.query.relation("namespace");
        self
    }

    pub fn select_namespace_with(mut self, request: impl Into<QuerySelection>) -> Self {
        let selection = request.into();
        self.query = self.query.relation_query("namespace", selection.into_query());
        self
}

    pub fn facet_by_namespace_as(self, facet_name: impl Into<String>, request: impl Into<QuerySelection>) -> Self {
        self.facet_by_namespace_as_with_options(facet_name, request, true)
    }

    pub fn facet_by_namespace_as_with_options(
        mut self,
        facet_name: impl Into<String>,
        request: impl Into<QuerySelection>,
        include_all_facets: bool,
    ) -> Self {
        self.query_options.facets.push(FacetRequest::new(
            facet_name,
            "namespace",
            request,
            include_all_facets,
        ));
        self
    }
}

impl<R> Default for RecordRequest<R> {
    fn default() -> Self {
        Self::new()
    }
}

impl<R> From< RecordRequest<R> > for SelectQuery {
    fn from(request: RecordRequest<R>) -> Self {
        QuerySelection::from(request).into_query()
    }
}

impl<R> From< RecordRequest<R> > for QuerySelection {
    fn from(request: RecordRequest<R>) -> Self {
        Self {
            query: request.query,
            relation_selections: request.relation_selections,
            relation_filters: request.relation_filters,
            child_enhancements: request.child_enhancements,
            query_options: request.query_options,
        }
    }
}


impl<'a, C> crate::request_support::AuditedSave<'a, C> for teaql_core::Audited<crate::Record> 
where C: crate::TeaqlRuntime + ?Sized + 'a
{
    type Error = teaql_runtime::RuntimeError;
    type Entity = crate::Record;
    fn save(self, context: &'a C) -> std::pin::Pin<Box<dyn std::future::Future<Output = Result<Self::Entity, Self::Error>> + Send + '_>> {
        Box::pin(async move {
            context.save_audited_entity(self).await
        })
    }
}

impl<R: teaql_core::Entity> crate::PurposedQuery<RecordRequest<R>> {
    pub fn comment(mut self, comment: impl Into<String>) -> Self {
        self.inner.query_options.comment = Some(comment.into());
        self
    }

    pub fn new_entity<C>(&self, context: &C) -> crate::Record
    where
        C: crate::TeaqlRuntime + ?Sized,
    {
        self.require_comment();
        let mut entity = crate::Record::runtime_new(context.user_context().entity_runtime_state());
        if let Ok(id) = context.user_context().next_id(crate::Record::ENTITY_NAME) {
            entity.update_id(id);
        }
        teaql_core::Entity::mark_as_new(&mut entity);
        entity
    }

    fn into_inner_with_trace(mut self) -> RecordRequest<R> {
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
