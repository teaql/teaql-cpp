use std::marker::PhantomData;

use serde_json::Value as JsonValue;
use teaql_core::{Aggregate, AggregateFunction, EntityDescriptor, Expr, SelectQuery, SmartList};
use teaql_runtime::RuntimeError;

use crate::request_support::*;

impl EntityReference for crate::Platform {
    fn entity_id_value(self) -> teaql_core::Value {
        teaql_core::IdentifiableEntity::id_value(&self)
    }
}

impl EntityReference for &crate::Platform {
    fn entity_id_value(self) -> teaql_core::Value {
        teaql_core::IdentifiableEntity::id_value(self)
    }
}

// ⛔ AI agents: DO NOT read this file for API discovery. Instead run: cargo teaql --input modeling/MODEL.xml rust-assist-query/platform
#[derive(Debug)]
pub struct PlatformRequest<R = crate::Platform> {
    query: SelectQuery,
    relation_selections: Vec<RelationSelection>,
    relation_filters: Vec<RelationFilter>,
    child_enhancements: Vec<QuerySelection>,
    query_options: QueryOptions,
    marker: PhantomData<R>,
}

impl<R> Clone for PlatformRequest<R> {
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

impl<R> PlatformRequest<R> {
    pub(crate) fn new() -> Self {
        Self {
            query: SelectQuery::new("Platform")
                .project("id")
                .project("version"),
            relation_selections: Vec::new(),
            relation_filters: Vec::new(),
            child_enhancements: Vec::new(),
            query_options: QueryOptions::default(),
            marker: PhantomData,
        }
    }

    pub fn return_type<T>(self) -> PlatformRequest<T> {
        PlatformRequest {
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
                "Platform",
                query,
                relation_aggregates,
            ).await?;
            (rows, std::collections::BTreeMap::new())
        } else {
            let rows = context.fetch_entity_smart_list::<R>(
                "Platform",
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
        context.fetch_compact_smart_list("Platform", &query).await
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
        Ok(context.fetch_entity_stream("Platform", query))
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
        let rows = context.fetch_compact_rows("Platform", &query).await?;
        rows.first()
            .and_then(|row| row.get(COUNT_ALIAS))
            .and_then(teaql_core::Value::try_u64)
            .ok_or_else(|| RuntimeError::Graph(format!("count result for Platform is missing or not numeric")))
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
        let rows = context.fetch_compact_rows("Platform", &query).await?;
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
            "name" => Some("name"),
            "version" => Some("version"),
            _ => None,
        }
    }

    fn apply_dynamic_json_chain_filter(self, head: &str, tail: &str, value: &JsonValue) -> Self {
        let _ = (tail, value);
        match head {
            "sample_record_list" => {
                self.with_sample_record_list_matching(
                    crate::Q::sample_records_minimal()
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
        self.query = self.query.project("name");
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
        let mut request = self.select_all();
        request = request.select_sample_record_list();
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


    pub fn select_name(mut self) -> Self {
        self.query = self.query.project("name");
        self
    }

    pub fn project_name(self) -> Self {
        self.select_name()
    }

    pub fn select_name_raw(self, raw_sql_segment: impl Into<String>) -> Self {
        self.select_name_unsafe_raw(UnsafeRawSqlSegment::trusted(raw_sql_segment))
    }

    pub fn select_name_unsafe_raw(mut self, raw_sql_segment: UnsafeRawSqlSegment) -> Self {
        self.query_options
            .raw_projections
            .push(RawProjection::new("name", raw_sql_segment));
        self
    }

    pub fn group_by_name(self) -> Self {
        self.group_by("name")
    }

    pub fn group_by_name_as(self, alias: impl Into<String>) -> Self {
        let alias = alias.into();
        let mut request = self.group_by("name");
        request.query = request
            .query
            .project_expr(alias, Expr::column("name"));
        request
    }

    pub fn group_by_name_with_function(
        self,
        alias: impl Into<String>,
        function: AggregateFunction,
    ) -> Self {
        self.group_by("name")
            .aggregate_with_function("name", alias, function)
    }

    pub fn count_name(self) -> Self {
        self.count_name_as("name_count")
    }

    pub fn count_name_as(self, alias: impl Into<String>) -> Self {
        self.aggregate_count_field("name", alias)
    }

    pub fn sum_name(self) -> Self {
        self.sum_name_as("sum_name")
    }

    pub fn sum_name_as(self, alias: impl Into<String>) -> Self {
        self.aggregate_sum("name", alias)
    }

    pub fn avg_name(self) -> Self {
        self.avg_name_as("avg_name")
    }

    pub fn avg_name_as(self, alias: impl Into<String>) -> Self {
        self.aggregate_avg("name", alias)
    }

    pub fn min_name(self) -> Self {
        self.min_name_as("min_name")
    }

    pub fn min_name_as(self, alias: impl Into<String>) -> Self {
        self.aggregate_min("name", alias)
    }

    pub fn max_name(self) -> Self {
        self.max_name_as("max_name")
    }

    pub fn max_name_as(self, alias: impl Into<String>) -> Self {
        self.aggregate_max("name", alias)
    }

    pub fn unselect_name(mut self) -> Self {
        self.query.projection.retain(|field| field != "name");
        self.query_options.raw_projections.retain(|projection| projection.property_name != "name");
        self
    }


    pub fn with_name(
        mut self,
        operator: FieldOperator,
        values: impl IntoIterator<Item = impl Into<teaql_core::Value>>,
    ) -> Self {
        self.query = self.query.and_filter(field_operator_expr(
            "name",
            operator,
            values.into_iter().map(Into::into).collect(),
        ));
        self
    }

    pub fn create_name_criteria(
        operator: FieldOperator,
        values: impl IntoIterator<Item = impl Into<teaql_core::Value>>,
    ) -> Expr {
        field_operator_expr(
            "name",
            operator,
            values.into_iter().map(Into::into).collect(),
        )
    }

    pub fn with_name_is(mut self, value: impl Into<teaql_core::Value>) -> Self {
        self.query = self.query.and_filter(Expr::eq("name", value));
        self
    }



    pub fn with_name_is_not(mut self, value: impl Into<teaql_core::Value>) -> Self {
        self.query = self.query.and_filter(Expr::ne("name", value));
        self
    }

    pub fn with_name_greater_than(mut self, value: impl Into<teaql_core::Value>) -> Self {
        self.query = self.query.and_filter(Expr::gt("name", value));
        self
    }

    pub fn with_name_greater_than_or_equal_to(mut self, value: impl Into<teaql_core::Value>) -> Self {
        self.query = self.query.and_filter(Expr::gte("name", value));
        self
    }

    pub fn with_name_less_than(mut self, value: impl Into<teaql_core::Value>) -> Self {
        self.query = self.query.and_filter(Expr::lt("name", value));
        self
    }

    pub fn with_name_less_than_or_equal_to(mut self, value: impl Into<teaql_core::Value>) -> Self {
        self.query = self.query.and_filter(Expr::lte("name", value));
        self
    }

    pub fn with_name_between(
        mut self,
        lower: impl Into<teaql_core::Value>,
        upper: impl Into<teaql_core::Value>,
    ) -> Self {
        self.query = self.query.and_filter(Expr::between("name", lower, upper));
        self
    }

    pub fn with_name_between_range<T>(mut self, range: DateRange<T>) -> Self
    where
        T: Into<teaql_core::Value>,
    {
        self.query = self.query.and_filter(Expr::between(
            "name",
            range.start,
            range.end,
        ));
        self
    }

    pub fn with_name_in(
        mut self,
        values: impl IntoIterator<Item = impl Into<teaql_core::Value>>,
    ) -> Self {
        self.query = self.query.and_filter(Expr::in_list(
            "name",
            values.into_iter().map(Into::into),
        ));
        self
    }

    pub fn with_name_not_in(
        mut self,
        values: impl IntoIterator<Item = impl Into<teaql_core::Value>>,
    ) -> Self {
        self.query = self.query.and_filter(Expr::not_in_list(
            "name",
            values.into_iter().map(Into::into),
        ));
        self
    }

    pub fn with_name_containing(mut self, value: impl Into<String>) -> Self {
        self.query = self.query.and_filter(Expr::contain("name", value));
        self
    }

    pub fn with_name_not_containing(mut self, value: impl Into<String>) -> Self {
        self.query = self.query.and_filter(Expr::not_contain("name", value));
        self
    }

    pub fn with_name_starting_with(mut self, value: impl Into<String>) -> Self {
        self.query = self.query.and_filter(Expr::begin_with("name", value));
        self
    }

    pub fn with_name_not_starting_with(mut self, value: impl Into<String>) -> Self {
        self.query = self.query.and_filter(Expr::not_begin_with("name", value));
        self
    }

    pub fn with_name_ending_with(mut self, value: impl Into<String>) -> Self {
        self.query = self.query.and_filter(Expr::end_with("name", value));
        self
    }

    pub fn with_name_not_ending_with(mut self, value: impl Into<String>) -> Self {
        self.query = self.query.and_filter(Expr::not_end_with("name", value));
        self
    }

    pub fn with_name_sounding_like(mut self, value: impl Into<teaql_core::Value>) -> Self {
        self.query = self.query.and_filter(Expr::sound_like("name", value));
        self
    }
    pub fn with_name_before(mut self, value: impl Into<teaql_core::Value>) -> Self {
        self.query = self.query.and_filter(Expr::lt("name", value));
        self
    }

    pub fn with_name_after(mut self, value: impl Into<teaql_core::Value>) -> Self {
        self.query = self.query.and_filter(Expr::gt("name", value));
        self
    }

    pub fn with_name_is_unknown(mut self) -> Self {
        self.query = self.query.and_filter(Expr::is_null("name"));
        self
    }



    pub fn with_name_is_known(mut self) -> Self {
        self.query = self.query.and_filter(Expr::is_not_null("name"));
        self
    }


    pub fn order_by_name_asc(mut self) -> Self {
        self.query = self.query.order_asc("name");
        self
    }

    pub fn order_by_name_desc(mut self) -> Self {
        self.query = self.query.order_desc("name");
        self
    }

    pub fn order_by_name_asc_using_gbk(mut self) -> Self {
        self.query = self.query.order_gbk_asc("name");
        self
    }

    pub fn order_by_name_desc_using_gbk(mut self) -> Self {
        self.query = self.query.order_gbk_desc("name");
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
    pub fn with_name_is_string(self) -> Self {
        self.with_name_is("string()")
    }



    pub fn with_name_is_not_string(self) -> Self {
        self.with_name_is_not("string()")
    }




    pub fn have_sample_records(self) -> Self {
        self.with_sample_record_list_matching(crate::Q::sample_records_minimal())
    }

    pub fn have_no_sample_records(self) -> Self {
        self.without_sample_record_list_matching(crate::Q::sample_records_minimal())
    }

    pub fn with_sample_record_list_matching(mut self, request: impl Into<QuerySelection>) -> Self {
        let selection = request.into();
        self.query = self.query.and_filter(Expr::in_subquery(
            "id",
            <crate::SampleRecord as teaql_core::TeaqlEntity>::entity_descriptor(),
            selection.query.clone(),
            "platform_id",
        ));
        self.relation_filters.push(RelationFilter::new("sample_record_list", selection));
        self
    }

    pub fn without_sample_record_list_matching(mut self, request: impl Into<QuerySelection>) -> Self {
        let selection = request.into();
        self.query = self.query.and_filter(Expr::not_in_subquery(
            "id",
            <crate::SampleRecord as teaql_core::TeaqlEntity>::entity_descriptor(),
            selection.query.clone(),
            "platform_id",
        ));
        self.relation_filters.push(RelationFilter::new("sample_record_list", selection));
        self
    }

    pub fn select_sample_record_list(mut self) -> Self {
        self.query = self.query.relation("sample_record_list");
        self
    }

    pub fn select_sample_record_list_with(mut self, request: impl Into<QuerySelection>) -> Self {
        let selection = request.into();
        self.query = self.query.relation_query("sample_record_list", selection.into_query());
        self
}
    pub fn count_sample_records(self) -> Self {
        self.count_sample_records_as("count_sample_records")
    }

    pub fn count_sample_records_as(self, alias: impl Into<String>) -> Self {
        self.count_sample_records_with(alias, crate::Q::sample_records().unlimited())
    }

    pub fn count_sample_records_with(mut self, alias: impl Into<String>, request: impl Into<QuerySelection>) -> Self {
        let selection = request.into();
        self.query_options.relation_aggregates.push(RelationAggregate::new(
            "sample_record_list",
            alias,
            selection,
            true,
        ));
        self
    }

    pub fn stats_from_sample_records(self, request: impl Into<QuerySelection>) -> Self {
        self.stats_from_sample_records_as("refinements", request)
    }

    pub fn stats_from_sample_records_as(mut self, alias: impl Into<String>, request: impl Into<QuerySelection>) -> Self {
        let selection = request.into();
        self.query_options.relation_aggregates.push(RelationAggregate::new(
            "sample_record_list",
            alias,
            selection,
            false,
        ));
        self
    }

    fn scalar_from_sample_records_as(mut self, alias: impl Into<String>, request: impl Into<QuerySelection>) -> Self {
        let selection = request.into();
        self.query_options.relation_aggregates.push(RelationAggregate::new(
            "sample_record_list",
            alias,
            selection,
            true,
        ));
        self
    }

    pub fn group_by_sample_records_with_details(self, request: impl Into<QuerySelection>) -> Self {
        self.stats_from_sample_records(request)
    }


    pub fn sum_quantity_of_sample_records(self) -> Self {
        self.sum_quantity_of_sample_records_as("sum_quantity_of_sample_records", crate::Q::sample_records().unlimited())
    }

    pub fn sum_quantity_of_sample_records_as(self, alias: impl Into<String>, request: impl Into<QuerySelection>) -> Self {
        self.scalar_from_sample_records_as(alias, request.into().into_query().sum("quantity", "sum_quantity"))
    }
    pub fn min_quantity_of_sample_records(self) -> Self {
        self.min_quantity_of_sample_records_as("min_quantity_of_sample_records", crate::Q::sample_records().unlimited())
    }

    pub fn min_quantity_of_sample_records_as(self, alias: impl Into<String>, request: impl Into<QuerySelection>) -> Self {
        self.scalar_from_sample_records_as(alias, request.into().into_query().min("quantity", "min_quantity"))
    }
    pub fn max_quantity_of_sample_records(self) -> Self {
        self.max_quantity_of_sample_records_as("max_quantity_of_sample_records", crate::Q::sample_records().unlimited())
    }

    pub fn max_quantity_of_sample_records_as(self, alias: impl Into<String>, request: impl Into<QuerySelection>) -> Self {
        self.scalar_from_sample_records_as(alias, request.into().into_query().max("quantity", "max_quantity"))
    }
    pub fn avg_quantity_of_sample_records(self) -> Self {
        self.avg_quantity_of_sample_records_as("avg_quantity_of_sample_records", crate::Q::sample_records().unlimited())
    }

    pub fn avg_quantity_of_sample_records_as(self, alias: impl Into<String>, request: impl Into<QuerySelection>) -> Self {
        self.scalar_from_sample_records_as(alias, request.into().into_query().avg("quantity", "avg_quantity"))
    }
    pub fn standard_deviation_quantity_of_sample_records(self) -> Self {
        self.standard_deviation_quantity_of_sample_records_as("standard_deviation_quantity_of_sample_records", crate::Q::sample_records().unlimited())
    }

    pub fn standard_deviation_quantity_of_sample_records_as(self, alias: impl Into<String>, request: impl Into<QuerySelection>) -> Self {
        self.scalar_from_sample_records_as(alias, request.into().into_query().stddev("quantity", "stdDev_quantity"))
    }
    pub fn square_root_of_population_standard_deviation_quantity_of_sample_records(self) -> Self {
        self.square_root_of_population_standard_deviation_quantity_of_sample_records_as("square_root_of_population_standard_deviation_quantity_of_sample_records", crate::Q::sample_records().unlimited())
    }

    pub fn square_root_of_population_standard_deviation_quantity_of_sample_records_as(self, alias: impl Into<String>, request: impl Into<QuerySelection>) -> Self {
        self.scalar_from_sample_records_as(alias, request.into().into_query().stddev_pop("quantity", "stdDevPop_quantity"))
    }
    pub fn sample_variance_quantity_of_sample_records(self) -> Self {
        self.sample_variance_quantity_of_sample_records_as("sample_variance_quantity_of_sample_records", crate::Q::sample_records().unlimited())
    }

    pub fn sample_variance_quantity_of_sample_records_as(self, alias: impl Into<String>, request: impl Into<QuerySelection>) -> Self {
        self.scalar_from_sample_records_as(alias, request.into().into_query().var_samp("quantity", "varSamp_quantity"))
    }
    pub fn sample_population_variance_quantity_of_sample_records(self) -> Self {
        self.sample_population_variance_quantity_of_sample_records_as("sample_population_variance_quantity_of_sample_records", crate::Q::sample_records().unlimited())
    }

    pub fn sample_population_variance_quantity_of_sample_records_as(self, alias: impl Into<String>, request: impl Into<QuerySelection>) -> Self {
        self.scalar_from_sample_records_as(alias, request.into().into_query().var_pop("quantity", "varPop_quantity"))
    }
    pub fn sum_zero_bound_of_sample_records(self) -> Self {
        self.sum_zero_bound_of_sample_records_as("sum_zero_bound_of_sample_records", crate::Q::sample_records().unlimited())
    }

    pub fn sum_zero_bound_of_sample_records_as(self, alias: impl Into<String>, request: impl Into<QuerySelection>) -> Self {
        self.scalar_from_sample_records_as(alias, request.into().into_query().sum("zero_bound", "sum_zero_bound"))
    }
    pub fn min_zero_bound_of_sample_records(self) -> Self {
        self.min_zero_bound_of_sample_records_as("min_zero_bound_of_sample_records", crate::Q::sample_records().unlimited())
    }

    pub fn min_zero_bound_of_sample_records_as(self, alias: impl Into<String>, request: impl Into<QuerySelection>) -> Self {
        self.scalar_from_sample_records_as(alias, request.into().into_query().min("zero_bound", "min_zero_bound"))
    }
    pub fn max_zero_bound_of_sample_records(self) -> Self {
        self.max_zero_bound_of_sample_records_as("max_zero_bound_of_sample_records", crate::Q::sample_records().unlimited())
    }

    pub fn max_zero_bound_of_sample_records_as(self, alias: impl Into<String>, request: impl Into<QuerySelection>) -> Self {
        self.scalar_from_sample_records_as(alias, request.into().into_query().max("zero_bound", "max_zero_bound"))
    }
    pub fn avg_zero_bound_of_sample_records(self) -> Self {
        self.avg_zero_bound_of_sample_records_as("avg_zero_bound_of_sample_records", crate::Q::sample_records().unlimited())
    }

    pub fn avg_zero_bound_of_sample_records_as(self, alias: impl Into<String>, request: impl Into<QuerySelection>) -> Self {
        self.scalar_from_sample_records_as(alias, request.into().into_query().avg("zero_bound", "avg_zero_bound"))
    }
    pub fn standard_deviation_zero_bound_of_sample_records(self) -> Self {
        self.standard_deviation_zero_bound_of_sample_records_as("standard_deviation_zero_bound_of_sample_records", crate::Q::sample_records().unlimited())
    }

    pub fn standard_deviation_zero_bound_of_sample_records_as(self, alias: impl Into<String>, request: impl Into<QuerySelection>) -> Self {
        self.scalar_from_sample_records_as(alias, request.into().into_query().stddev("zero_bound", "stdDev_zero_bound"))
    }
    pub fn square_root_of_population_standard_deviation_zero_bound_of_sample_records(self) -> Self {
        self.square_root_of_population_standard_deviation_zero_bound_of_sample_records_as("square_root_of_population_standard_deviation_zero_bound_of_sample_records", crate::Q::sample_records().unlimited())
    }

    pub fn square_root_of_population_standard_deviation_zero_bound_of_sample_records_as(self, alias: impl Into<String>, request: impl Into<QuerySelection>) -> Self {
        self.scalar_from_sample_records_as(alias, request.into().into_query().stddev_pop("zero_bound", "stdDevPop_zero_bound"))
    }
    pub fn sample_variance_zero_bound_of_sample_records(self) -> Self {
        self.sample_variance_zero_bound_of_sample_records_as("sample_variance_zero_bound_of_sample_records", crate::Q::sample_records().unlimited())
    }

    pub fn sample_variance_zero_bound_of_sample_records_as(self, alias: impl Into<String>, request: impl Into<QuerySelection>) -> Self {
        self.scalar_from_sample_records_as(alias, request.into().into_query().var_samp("zero_bound", "varSamp_zero_bound"))
    }
    pub fn sample_population_variance_zero_bound_of_sample_records(self) -> Self {
        self.sample_population_variance_zero_bound_of_sample_records_as("sample_population_variance_zero_bound_of_sample_records", crate::Q::sample_records().unlimited())
    }

    pub fn sample_population_variance_zero_bound_of_sample_records_as(self, alias: impl Into<String>, request: impl Into<QuerySelection>) -> Self {
        self.scalar_from_sample_records_as(alias, request.into().into_query().var_pop("zero_bound", "varPop_zero_bound"))
    }
    pub fn sum_negative_max_of_sample_records(self) -> Self {
        self.sum_negative_max_of_sample_records_as("sum_negative_max_of_sample_records", crate::Q::sample_records().unlimited())
    }

    pub fn sum_negative_max_of_sample_records_as(self, alias: impl Into<String>, request: impl Into<QuerySelection>) -> Self {
        self.scalar_from_sample_records_as(alias, request.into().into_query().sum("negative_max", "sum_negative_max"))
    }
    pub fn min_negative_max_of_sample_records(self) -> Self {
        self.min_negative_max_of_sample_records_as("min_negative_max_of_sample_records", crate::Q::sample_records().unlimited())
    }

    pub fn min_negative_max_of_sample_records_as(self, alias: impl Into<String>, request: impl Into<QuerySelection>) -> Self {
        self.scalar_from_sample_records_as(alias, request.into().into_query().min("negative_max", "min_negative_max"))
    }
    pub fn max_negative_max_of_sample_records(self) -> Self {
        self.max_negative_max_of_sample_records_as("max_negative_max_of_sample_records", crate::Q::sample_records().unlimited())
    }

    pub fn max_negative_max_of_sample_records_as(self, alias: impl Into<String>, request: impl Into<QuerySelection>) -> Self {
        self.scalar_from_sample_records_as(alias, request.into().into_query().max("negative_max", "max_negative_max"))
    }
    pub fn avg_negative_max_of_sample_records(self) -> Self {
        self.avg_negative_max_of_sample_records_as("avg_negative_max_of_sample_records", crate::Q::sample_records().unlimited())
    }

    pub fn avg_negative_max_of_sample_records_as(self, alias: impl Into<String>, request: impl Into<QuerySelection>) -> Self {
        self.scalar_from_sample_records_as(alias, request.into().into_query().avg("negative_max", "avg_negative_max"))
    }
    pub fn standard_deviation_negative_max_of_sample_records(self) -> Self {
        self.standard_deviation_negative_max_of_sample_records_as("standard_deviation_negative_max_of_sample_records", crate::Q::sample_records().unlimited())
    }

    pub fn standard_deviation_negative_max_of_sample_records_as(self, alias: impl Into<String>, request: impl Into<QuerySelection>) -> Self {
        self.scalar_from_sample_records_as(alias, request.into().into_query().stddev("negative_max", "stdDev_negative_max"))
    }
    pub fn square_root_of_population_standard_deviation_negative_max_of_sample_records(self) -> Self {
        self.square_root_of_population_standard_deviation_negative_max_of_sample_records_as("square_root_of_population_standard_deviation_negative_max_of_sample_records", crate::Q::sample_records().unlimited())
    }

    pub fn square_root_of_population_standard_deviation_negative_max_of_sample_records_as(self, alias: impl Into<String>, request: impl Into<QuerySelection>) -> Self {
        self.scalar_from_sample_records_as(alias, request.into().into_query().stddev_pop("negative_max", "stdDevPop_negative_max"))
    }
    pub fn sample_variance_negative_max_of_sample_records(self) -> Self {
        self.sample_variance_negative_max_of_sample_records_as("sample_variance_negative_max_of_sample_records", crate::Q::sample_records().unlimited())
    }

    pub fn sample_variance_negative_max_of_sample_records_as(self, alias: impl Into<String>, request: impl Into<QuerySelection>) -> Self {
        self.scalar_from_sample_records_as(alias, request.into().into_query().var_samp("negative_max", "varSamp_negative_max"))
    }
    pub fn sample_population_variance_negative_max_of_sample_records(self) -> Self {
        self.sample_population_variance_negative_max_of_sample_records_as("sample_population_variance_negative_max_of_sample_records", crate::Q::sample_records().unlimited())
    }

    pub fn sample_population_variance_negative_max_of_sample_records_as(self, alias: impl Into<String>, request: impl Into<QuerySelection>) -> Self {
        self.scalar_from_sample_records_as(alias, request.into().into_query().var_pop("negative_max", "varPop_negative_max"))
    }
    pub fn sum_signed_max_of_sample_records(self) -> Self {
        self.sum_signed_max_of_sample_records_as("sum_signed_max_of_sample_records", crate::Q::sample_records().unlimited())
    }

    pub fn sum_signed_max_of_sample_records_as(self, alias: impl Into<String>, request: impl Into<QuerySelection>) -> Self {
        self.scalar_from_sample_records_as(alias, request.into().into_query().sum("signed_max", "sum_signed_max"))
    }
    pub fn min_signed_max_of_sample_records(self) -> Self {
        self.min_signed_max_of_sample_records_as("min_signed_max_of_sample_records", crate::Q::sample_records().unlimited())
    }

    pub fn min_signed_max_of_sample_records_as(self, alias: impl Into<String>, request: impl Into<QuerySelection>) -> Self {
        self.scalar_from_sample_records_as(alias, request.into().into_query().min("signed_max", "min_signed_max"))
    }
    pub fn max_signed_max_of_sample_records(self) -> Self {
        self.max_signed_max_of_sample_records_as("max_signed_max_of_sample_records", crate::Q::sample_records().unlimited())
    }

    pub fn max_signed_max_of_sample_records_as(self, alias: impl Into<String>, request: impl Into<QuerySelection>) -> Self {
        self.scalar_from_sample_records_as(alias, request.into().into_query().max("signed_max", "max_signed_max"))
    }
    pub fn avg_signed_max_of_sample_records(self) -> Self {
        self.avg_signed_max_of_sample_records_as("avg_signed_max_of_sample_records", crate::Q::sample_records().unlimited())
    }

    pub fn avg_signed_max_of_sample_records_as(self, alias: impl Into<String>, request: impl Into<QuerySelection>) -> Self {
        self.scalar_from_sample_records_as(alias, request.into().into_query().avg("signed_max", "avg_signed_max"))
    }
    pub fn standard_deviation_signed_max_of_sample_records(self) -> Self {
        self.standard_deviation_signed_max_of_sample_records_as("standard_deviation_signed_max_of_sample_records", crate::Q::sample_records().unlimited())
    }

    pub fn standard_deviation_signed_max_of_sample_records_as(self, alias: impl Into<String>, request: impl Into<QuerySelection>) -> Self {
        self.scalar_from_sample_records_as(alias, request.into().into_query().stddev("signed_max", "stdDev_signed_max"))
    }
    pub fn square_root_of_population_standard_deviation_signed_max_of_sample_records(self) -> Self {
        self.square_root_of_population_standard_deviation_signed_max_of_sample_records_as("square_root_of_population_standard_deviation_signed_max_of_sample_records", crate::Q::sample_records().unlimited())
    }

    pub fn square_root_of_population_standard_deviation_signed_max_of_sample_records_as(self, alias: impl Into<String>, request: impl Into<QuerySelection>) -> Self {
        self.scalar_from_sample_records_as(alias, request.into().into_query().stddev_pop("signed_max", "stdDevPop_signed_max"))
    }
    pub fn sample_variance_signed_max_of_sample_records(self) -> Self {
        self.sample_variance_signed_max_of_sample_records_as("sample_variance_signed_max_of_sample_records", crate::Q::sample_records().unlimited())
    }

    pub fn sample_variance_signed_max_of_sample_records_as(self, alias: impl Into<String>, request: impl Into<QuerySelection>) -> Self {
        self.scalar_from_sample_records_as(alias, request.into().into_query().var_samp("signed_max", "varSamp_signed_max"))
    }
    pub fn sample_population_variance_signed_max_of_sample_records(self) -> Self {
        self.sample_population_variance_signed_max_of_sample_records_as("sample_population_variance_signed_max_of_sample_records", crate::Q::sample_records().unlimited())
    }

    pub fn sample_population_variance_signed_max_of_sample_records_as(self, alias: impl Into<String>, request: impl Into<QuerySelection>) -> Self {
        self.scalar_from_sample_records_as(alias, request.into().into_query().var_pop("signed_max", "varPop_signed_max"))
    }
    pub fn sum_wide_max_of_sample_records(self) -> Self {
        self.sum_wide_max_of_sample_records_as("sum_wide_max_of_sample_records", crate::Q::sample_records().unlimited())
    }

    pub fn sum_wide_max_of_sample_records_as(self, alias: impl Into<String>, request: impl Into<QuerySelection>) -> Self {
        self.scalar_from_sample_records_as(alias, request.into().into_query().sum("wide_max", "sum_wide_max"))
    }
    pub fn min_wide_max_of_sample_records(self) -> Self {
        self.min_wide_max_of_sample_records_as("min_wide_max_of_sample_records", crate::Q::sample_records().unlimited())
    }

    pub fn min_wide_max_of_sample_records_as(self, alias: impl Into<String>, request: impl Into<QuerySelection>) -> Self {
        self.scalar_from_sample_records_as(alias, request.into().into_query().min("wide_max", "min_wide_max"))
    }
    pub fn max_wide_max_of_sample_records(self) -> Self {
        self.max_wide_max_of_sample_records_as("max_wide_max_of_sample_records", crate::Q::sample_records().unlimited())
    }

    pub fn max_wide_max_of_sample_records_as(self, alias: impl Into<String>, request: impl Into<QuerySelection>) -> Self {
        self.scalar_from_sample_records_as(alias, request.into().into_query().max("wide_max", "max_wide_max"))
    }
    pub fn avg_wide_max_of_sample_records(self) -> Self {
        self.avg_wide_max_of_sample_records_as("avg_wide_max_of_sample_records", crate::Q::sample_records().unlimited())
    }

    pub fn avg_wide_max_of_sample_records_as(self, alias: impl Into<String>, request: impl Into<QuerySelection>) -> Self {
        self.scalar_from_sample_records_as(alias, request.into().into_query().avg("wide_max", "avg_wide_max"))
    }
    pub fn standard_deviation_wide_max_of_sample_records(self) -> Self {
        self.standard_deviation_wide_max_of_sample_records_as("standard_deviation_wide_max_of_sample_records", crate::Q::sample_records().unlimited())
    }

    pub fn standard_deviation_wide_max_of_sample_records_as(self, alias: impl Into<String>, request: impl Into<QuerySelection>) -> Self {
        self.scalar_from_sample_records_as(alias, request.into().into_query().stddev("wide_max", "stdDev_wide_max"))
    }
    pub fn square_root_of_population_standard_deviation_wide_max_of_sample_records(self) -> Self {
        self.square_root_of_population_standard_deviation_wide_max_of_sample_records_as("square_root_of_population_standard_deviation_wide_max_of_sample_records", crate::Q::sample_records().unlimited())
    }

    pub fn square_root_of_population_standard_deviation_wide_max_of_sample_records_as(self, alias: impl Into<String>, request: impl Into<QuerySelection>) -> Self {
        self.scalar_from_sample_records_as(alias, request.into().into_query().stddev_pop("wide_max", "stdDevPop_wide_max"))
    }
    pub fn sample_variance_wide_max_of_sample_records(self) -> Self {
        self.sample_variance_wide_max_of_sample_records_as("sample_variance_wide_max_of_sample_records", crate::Q::sample_records().unlimited())
    }

    pub fn sample_variance_wide_max_of_sample_records_as(self, alias: impl Into<String>, request: impl Into<QuerySelection>) -> Self {
        self.scalar_from_sample_records_as(alias, request.into().into_query().var_samp("wide_max", "varSamp_wide_max"))
    }
    pub fn sample_population_variance_wide_max_of_sample_records(self) -> Self {
        self.sample_population_variance_wide_max_of_sample_records_as("sample_population_variance_wide_max_of_sample_records", crate::Q::sample_records().unlimited())
    }

    pub fn sample_population_variance_wide_max_of_sample_records_as(self, alias: impl Into<String>, request: impl Into<QuerySelection>) -> Self {
        self.scalar_from_sample_records_as(alias, request.into().into_query().var_pop("wide_max", "varPop_wide_max"))
    }
    pub fn sum_amount_of_sample_records(self) -> Self {
        self.sum_amount_of_sample_records_as("sum_amount_of_sample_records", crate::Q::sample_records().unlimited())
    }

    pub fn sum_amount_of_sample_records_as(self, alias: impl Into<String>, request: impl Into<QuerySelection>) -> Self {
        self.scalar_from_sample_records_as(alias, request.into().into_query().sum("amount", "sum_amount"))
    }
    pub fn min_amount_of_sample_records(self) -> Self {
        self.min_amount_of_sample_records_as("min_amount_of_sample_records", crate::Q::sample_records().unlimited())
    }

    pub fn min_amount_of_sample_records_as(self, alias: impl Into<String>, request: impl Into<QuerySelection>) -> Self {
        self.scalar_from_sample_records_as(alias, request.into().into_query().min("amount", "min_amount"))
    }
    pub fn max_amount_of_sample_records(self) -> Self {
        self.max_amount_of_sample_records_as("max_amount_of_sample_records", crate::Q::sample_records().unlimited())
    }

    pub fn max_amount_of_sample_records_as(self, alias: impl Into<String>, request: impl Into<QuerySelection>) -> Self {
        self.scalar_from_sample_records_as(alias, request.into().into_query().max("amount", "max_amount"))
    }
    pub fn avg_amount_of_sample_records(self) -> Self {
        self.avg_amount_of_sample_records_as("avg_amount_of_sample_records", crate::Q::sample_records().unlimited())
    }

    pub fn avg_amount_of_sample_records_as(self, alias: impl Into<String>, request: impl Into<QuerySelection>) -> Self {
        self.scalar_from_sample_records_as(alias, request.into().into_query().avg("amount", "avg_amount"))
    }
    pub fn standard_deviation_amount_of_sample_records(self) -> Self {
        self.standard_deviation_amount_of_sample_records_as("standard_deviation_amount_of_sample_records", crate::Q::sample_records().unlimited())
    }

    pub fn standard_deviation_amount_of_sample_records_as(self, alias: impl Into<String>, request: impl Into<QuerySelection>) -> Self {
        self.scalar_from_sample_records_as(alias, request.into().into_query().stddev("amount", "stdDev_amount"))
    }
    pub fn square_root_of_population_standard_deviation_amount_of_sample_records(self) -> Self {
        self.square_root_of_population_standard_deviation_amount_of_sample_records_as("square_root_of_population_standard_deviation_amount_of_sample_records", crate::Q::sample_records().unlimited())
    }

    pub fn square_root_of_population_standard_deviation_amount_of_sample_records_as(self, alias: impl Into<String>, request: impl Into<QuerySelection>) -> Self {
        self.scalar_from_sample_records_as(alias, request.into().into_query().stddev_pop("amount", "stdDevPop_amount"))
    }
    pub fn sample_variance_amount_of_sample_records(self) -> Self {
        self.sample_variance_amount_of_sample_records_as("sample_variance_amount_of_sample_records", crate::Q::sample_records().unlimited())
    }

    pub fn sample_variance_amount_of_sample_records_as(self, alias: impl Into<String>, request: impl Into<QuerySelection>) -> Self {
        self.scalar_from_sample_records_as(alias, request.into().into_query().var_samp("amount", "varSamp_amount"))
    }
    pub fn sample_population_variance_amount_of_sample_records(self) -> Self {
        self.sample_population_variance_amount_of_sample_records_as("sample_population_variance_amount_of_sample_records", crate::Q::sample_records().unlimited())
    }

    pub fn sample_population_variance_amount_of_sample_records_as(self, alias: impl Into<String>, request: impl Into<QuerySelection>) -> Self {
        self.scalar_from_sample_records_as(alias, request.into().into_query().var_pop("amount", "varPop_amount"))
    }
    pub fn sum_height_of_sample_records(self) -> Self {
        self.sum_height_of_sample_records_as("sum_height_of_sample_records", crate::Q::sample_records().unlimited())
    }

    pub fn sum_height_of_sample_records_as(self, alias: impl Into<String>, request: impl Into<QuerySelection>) -> Self {
        self.scalar_from_sample_records_as(alias, request.into().into_query().sum("height", "sum_height"))
    }
    pub fn min_height_of_sample_records(self) -> Self {
        self.min_height_of_sample_records_as("min_height_of_sample_records", crate::Q::sample_records().unlimited())
    }

    pub fn min_height_of_sample_records_as(self, alias: impl Into<String>, request: impl Into<QuerySelection>) -> Self {
        self.scalar_from_sample_records_as(alias, request.into().into_query().min("height", "min_height"))
    }
    pub fn max_height_of_sample_records(self) -> Self {
        self.max_height_of_sample_records_as("max_height_of_sample_records", crate::Q::sample_records().unlimited())
    }

    pub fn max_height_of_sample_records_as(self, alias: impl Into<String>, request: impl Into<QuerySelection>) -> Self {
        self.scalar_from_sample_records_as(alias, request.into().into_query().max("height", "max_height"))
    }
    pub fn avg_height_of_sample_records(self) -> Self {
        self.avg_height_of_sample_records_as("avg_height_of_sample_records", crate::Q::sample_records().unlimited())
    }

    pub fn avg_height_of_sample_records_as(self, alias: impl Into<String>, request: impl Into<QuerySelection>) -> Self {
        self.scalar_from_sample_records_as(alias, request.into().into_query().avg("height", "avg_height"))
    }
    pub fn standard_deviation_height_of_sample_records(self) -> Self {
        self.standard_deviation_height_of_sample_records_as("standard_deviation_height_of_sample_records", crate::Q::sample_records().unlimited())
    }

    pub fn standard_deviation_height_of_sample_records_as(self, alias: impl Into<String>, request: impl Into<QuerySelection>) -> Self {
        self.scalar_from_sample_records_as(alias, request.into().into_query().stddev("height", "stdDev_height"))
    }
    pub fn square_root_of_population_standard_deviation_height_of_sample_records(self) -> Self {
        self.square_root_of_population_standard_deviation_height_of_sample_records_as("square_root_of_population_standard_deviation_height_of_sample_records", crate::Q::sample_records().unlimited())
    }

    pub fn square_root_of_population_standard_deviation_height_of_sample_records_as(self, alias: impl Into<String>, request: impl Into<QuerySelection>) -> Self {
        self.scalar_from_sample_records_as(alias, request.into().into_query().stddev_pop("height", "stdDevPop_height"))
    }
    pub fn sample_variance_height_of_sample_records(self) -> Self {
        self.sample_variance_height_of_sample_records_as("sample_variance_height_of_sample_records", crate::Q::sample_records().unlimited())
    }

    pub fn sample_variance_height_of_sample_records_as(self, alias: impl Into<String>, request: impl Into<QuerySelection>) -> Self {
        self.scalar_from_sample_records_as(alias, request.into().into_query().var_samp("height", "varSamp_height"))
    }
    pub fn sample_population_variance_height_of_sample_records(self) -> Self {
        self.sample_population_variance_height_of_sample_records_as("sample_population_variance_height_of_sample_records", crate::Q::sample_records().unlimited())
    }

    pub fn sample_population_variance_height_of_sample_records_as(self, alias: impl Into<String>, request: impl Into<QuerySelection>) -> Self {
        self.scalar_from_sample_records_as(alias, request.into().into_query().var_pop("height", "varPop_height"))
    }
    pub fn sum_optional_count_of_sample_records(self) -> Self {
        self.sum_optional_count_of_sample_records_as("sum_optional_count_of_sample_records", crate::Q::sample_records().unlimited())
    }

    pub fn sum_optional_count_of_sample_records_as(self, alias: impl Into<String>, request: impl Into<QuerySelection>) -> Self {
        self.scalar_from_sample_records_as(alias, request.into().into_query().sum("optional_count", "sum_optional_count"))
    }
    pub fn min_optional_count_of_sample_records(self) -> Self {
        self.min_optional_count_of_sample_records_as("min_optional_count_of_sample_records", crate::Q::sample_records().unlimited())
    }

    pub fn min_optional_count_of_sample_records_as(self, alias: impl Into<String>, request: impl Into<QuerySelection>) -> Self {
        self.scalar_from_sample_records_as(alias, request.into().into_query().min("optional_count", "min_optional_count"))
    }
    pub fn max_optional_count_of_sample_records(self) -> Self {
        self.max_optional_count_of_sample_records_as("max_optional_count_of_sample_records", crate::Q::sample_records().unlimited())
    }

    pub fn max_optional_count_of_sample_records_as(self, alias: impl Into<String>, request: impl Into<QuerySelection>) -> Self {
        self.scalar_from_sample_records_as(alias, request.into().into_query().max("optional_count", "max_optional_count"))
    }
    pub fn avg_optional_count_of_sample_records(self) -> Self {
        self.avg_optional_count_of_sample_records_as("avg_optional_count_of_sample_records", crate::Q::sample_records().unlimited())
    }

    pub fn avg_optional_count_of_sample_records_as(self, alias: impl Into<String>, request: impl Into<QuerySelection>) -> Self {
        self.scalar_from_sample_records_as(alias, request.into().into_query().avg("optional_count", "avg_optional_count"))
    }
    pub fn standard_deviation_optional_count_of_sample_records(self) -> Self {
        self.standard_deviation_optional_count_of_sample_records_as("standard_deviation_optional_count_of_sample_records", crate::Q::sample_records().unlimited())
    }

    pub fn standard_deviation_optional_count_of_sample_records_as(self, alias: impl Into<String>, request: impl Into<QuerySelection>) -> Self {
        self.scalar_from_sample_records_as(alias, request.into().into_query().stddev("optional_count", "stdDev_optional_count"))
    }
    pub fn square_root_of_population_standard_deviation_optional_count_of_sample_records(self) -> Self {
        self.square_root_of_population_standard_deviation_optional_count_of_sample_records_as("square_root_of_population_standard_deviation_optional_count_of_sample_records", crate::Q::sample_records().unlimited())
    }

    pub fn square_root_of_population_standard_deviation_optional_count_of_sample_records_as(self, alias: impl Into<String>, request: impl Into<QuerySelection>) -> Self {
        self.scalar_from_sample_records_as(alias, request.into().into_query().stddev_pop("optional_count", "stdDevPop_optional_count"))
    }
    pub fn sample_variance_optional_count_of_sample_records(self) -> Self {
        self.sample_variance_optional_count_of_sample_records_as("sample_variance_optional_count_of_sample_records", crate::Q::sample_records().unlimited())
    }

    pub fn sample_variance_optional_count_of_sample_records_as(self, alias: impl Into<String>, request: impl Into<QuerySelection>) -> Self {
        self.scalar_from_sample_records_as(alias, request.into().into_query().var_samp("optional_count", "varSamp_optional_count"))
    }
    pub fn sample_population_variance_optional_count_of_sample_records(self) -> Self {
        self.sample_population_variance_optional_count_of_sample_records_as("sample_population_variance_optional_count_of_sample_records", crate::Q::sample_records().unlimited())
    }

    pub fn sample_population_variance_optional_count_of_sample_records_as(self, alias: impl Into<String>, request: impl Into<QuerySelection>) -> Self {
        self.scalar_from_sample_records_as(alias, request.into().into_query().var_pop("optional_count", "varPop_optional_count"))
    }
    pub fn min_day_of_sample_records(self) -> Self {
        self.min_day_of_sample_records_as("min_day_of_sample_records", crate::Q::sample_records().unlimited())
    }

    pub fn min_day_of_sample_records_as(self, alias: impl Into<String>, request: impl Into<QuerySelection>) -> Self {
        self.scalar_from_sample_records_as(alias, request.into().into_query().min("day", "min_day"))
    }
    pub fn max_day_of_sample_records(self) -> Self {
        self.max_day_of_sample_records_as("max_day_of_sample_records", crate::Q::sample_records().unlimited())
    }

    pub fn max_day_of_sample_records_as(self, alias: impl Into<String>, request: impl Into<QuerySelection>) -> Self {
        self.scalar_from_sample_records_as(alias, request.into().into_query().max("day", "max_day"))
    }
    pub fn min_optional_day_of_sample_records(self) -> Self {
        self.min_optional_day_of_sample_records_as("min_optional_day_of_sample_records", crate::Q::sample_records().unlimited())
    }

    pub fn min_optional_day_of_sample_records_as(self, alias: impl Into<String>, request: impl Into<QuerySelection>) -> Self {
        self.scalar_from_sample_records_as(alias, request.into().into_query().min("optional_day", "min_optional_day"))
    }
    pub fn max_optional_day_of_sample_records(self) -> Self {
        self.max_optional_day_of_sample_records_as("max_optional_day_of_sample_records", crate::Q::sample_records().unlimited())
    }

    pub fn max_optional_day_of_sample_records_as(self, alias: impl Into<String>, request: impl Into<QuerySelection>) -> Self {
        self.scalar_from_sample_records_as(alias, request.into().into_query().max("optional_day", "max_optional_day"))
    }
    pub fn min_recorded_at_of_sample_records(self) -> Self {
        self.min_recorded_at_of_sample_records_as("min_recorded_at_of_sample_records", crate::Q::sample_records().unlimited())
    }

    pub fn min_recorded_at_of_sample_records_as(self, alias: impl Into<String>, request: impl Into<QuerySelection>) -> Self {
        self.scalar_from_sample_records_as(alias, request.into().into_query().min("recorded_at", "min_recorded_at"))
    }
    pub fn max_recorded_at_of_sample_records(self) -> Self {
        self.max_recorded_at_of_sample_records_as("max_recorded_at_of_sample_records", crate::Q::sample_records().unlimited())
    }

    pub fn max_recorded_at_of_sample_records_as(self, alias: impl Into<String>, request: impl Into<QuerySelection>) -> Self {
        self.scalar_from_sample_records_as(alias, request.into().into_query().max("recorded_at", "max_recorded_at"))
    }
    pub fn min_optional_recorded_at_of_sample_records(self) -> Self {
        self.min_optional_recorded_at_of_sample_records_as("min_optional_recorded_at_of_sample_records", crate::Q::sample_records().unlimited())
    }

    pub fn min_optional_recorded_at_of_sample_records_as(self, alias: impl Into<String>, request: impl Into<QuerySelection>) -> Self {
        self.scalar_from_sample_records_as(alias, request.into().into_query().min("optional_recorded_at", "min_optional_recorded_at"))
    }
    pub fn max_optional_recorded_at_of_sample_records(self) -> Self {
        self.max_optional_recorded_at_of_sample_records_as("max_optional_recorded_at_of_sample_records", crate::Q::sample_records().unlimited())
    }

    pub fn max_optional_recorded_at_of_sample_records_as(self, alias: impl Into<String>, request: impl Into<QuerySelection>) -> Self {
        self.scalar_from_sample_records_as(alias, request.into().into_query().max("optional_recorded_at", "max_optional_recorded_at"))
    }
    pub fn sum_ratio_of_sample_records(self) -> Self {
        self.sum_ratio_of_sample_records_as("sum_ratio_of_sample_records", crate::Q::sample_records().unlimited())
    }

    pub fn sum_ratio_of_sample_records_as(self, alias: impl Into<String>, request: impl Into<QuerySelection>) -> Self {
        self.scalar_from_sample_records_as(alias, request.into().into_query().sum("ratio", "sum_ratio"))
    }
    pub fn min_ratio_of_sample_records(self) -> Self {
        self.min_ratio_of_sample_records_as("min_ratio_of_sample_records", crate::Q::sample_records().unlimited())
    }

    pub fn min_ratio_of_sample_records_as(self, alias: impl Into<String>, request: impl Into<QuerySelection>) -> Self {
        self.scalar_from_sample_records_as(alias, request.into().into_query().min("ratio", "min_ratio"))
    }
    pub fn max_ratio_of_sample_records(self) -> Self {
        self.max_ratio_of_sample_records_as("max_ratio_of_sample_records", crate::Q::sample_records().unlimited())
    }

    pub fn max_ratio_of_sample_records_as(self, alias: impl Into<String>, request: impl Into<QuerySelection>) -> Self {
        self.scalar_from_sample_records_as(alias, request.into().into_query().max("ratio", "max_ratio"))
    }
    pub fn avg_ratio_of_sample_records(self) -> Self {
        self.avg_ratio_of_sample_records_as("avg_ratio_of_sample_records", crate::Q::sample_records().unlimited())
    }

    pub fn avg_ratio_of_sample_records_as(self, alias: impl Into<String>, request: impl Into<QuerySelection>) -> Self {
        self.scalar_from_sample_records_as(alias, request.into().into_query().avg("ratio", "avg_ratio"))
    }
    pub fn standard_deviation_ratio_of_sample_records(self) -> Self {
        self.standard_deviation_ratio_of_sample_records_as("standard_deviation_ratio_of_sample_records", crate::Q::sample_records().unlimited())
    }

    pub fn standard_deviation_ratio_of_sample_records_as(self, alias: impl Into<String>, request: impl Into<QuerySelection>) -> Self {
        self.scalar_from_sample_records_as(alias, request.into().into_query().stddev("ratio", "stdDev_ratio"))
    }
    pub fn square_root_of_population_standard_deviation_ratio_of_sample_records(self) -> Self {
        self.square_root_of_population_standard_deviation_ratio_of_sample_records_as("square_root_of_population_standard_deviation_ratio_of_sample_records", crate::Q::sample_records().unlimited())
    }

    pub fn square_root_of_population_standard_deviation_ratio_of_sample_records_as(self, alias: impl Into<String>, request: impl Into<QuerySelection>) -> Self {
        self.scalar_from_sample_records_as(alias, request.into().into_query().stddev_pop("ratio", "stdDevPop_ratio"))
    }
    pub fn sample_variance_ratio_of_sample_records(self) -> Self {
        self.sample_variance_ratio_of_sample_records_as("sample_variance_ratio_of_sample_records", crate::Q::sample_records().unlimited())
    }

    pub fn sample_variance_ratio_of_sample_records_as(self, alias: impl Into<String>, request: impl Into<QuerySelection>) -> Self {
        self.scalar_from_sample_records_as(alias, request.into().into_query().var_samp("ratio", "varSamp_ratio"))
    }
    pub fn sample_population_variance_ratio_of_sample_records(self) -> Self {
        self.sample_population_variance_ratio_of_sample_records_as("sample_population_variance_ratio_of_sample_records", crate::Q::sample_records().unlimited())
    }

    pub fn sample_population_variance_ratio_of_sample_records_as(self, alias: impl Into<String>, request: impl Into<QuerySelection>) -> Self {
        self.scalar_from_sample_records_as(alias, request.into().into_query().var_pop("ratio", "varPop_ratio"))
    }
}

impl<R> Default for PlatformRequest<R> {
    fn default() -> Self {
        Self::new()
    }
}

impl<R> From< PlatformRequest<R> > for SelectQuery {
    fn from(request: PlatformRequest<R>) -> Self {
        QuerySelection::from(request).into_query()
    }
}

impl<R> From< PlatformRequest<R> > for QuerySelection {
    fn from(request: PlatformRequest<R>) -> Self {
        Self {
            query: request.query,
            relation_selections: request.relation_selections,
            relation_filters: request.relation_filters,
            child_enhancements: request.child_enhancements,
            query_options: request.query_options,
        }
    }
}


impl<'a, C> crate::request_support::AuditedSave<'a, C> for teaql_core::Audited<crate::Platform> 
where C: crate::TeaqlRuntime + ?Sized + 'a
{
    type Error = teaql_runtime::RuntimeError;
    type Entity = crate::Platform;
    fn save(self, context: &'a C) -> std::pin::Pin<Box<dyn std::future::Future<Output = Result<Self::Entity, Self::Error>> + Send + '_>> {
        Box::pin(async move {
            context.save_audited_entity(self).await
        })
    }
}

impl<R: teaql_core::Entity> crate::PurposedQuery<PlatformRequest<R>> {
    pub fn comment(mut self, comment: impl Into<String>) -> Self {
        self.inner.query_options.comment = Some(comment.into());
        self
    }

    pub fn new_entity<C>(&self, context: &C) -> crate::Platform
    where
        C: crate::TeaqlRuntime + ?Sized,
    {
        self.require_comment();
        let mut entity = crate::Platform::runtime_new(context.user_context().entity_runtime_state());
        if let Ok(id) = context.user_context().next_id(crate::Platform::ENTITY_NAME) {
            entity.update_id(id);
        }
        teaql_core::Entity::mark_as_new(&mut entity);
        entity
    }

    fn into_inner_with_trace(mut self) -> PlatformRequest<R> {
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
