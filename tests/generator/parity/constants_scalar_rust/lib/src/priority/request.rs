use std::marker::PhantomData;

use serde_json::Value as JsonValue;
use teaql_core::{Aggregate, AggregateFunction, EntityDescriptor, Expr, SelectQuery, SmartList};
use teaql_runtime::RuntimeError;

use crate::request_support::*;

impl EntityReference for crate::Priority {
    fn entity_id_value(self) -> teaql_core::Value {
        teaql_core::IdentifiableEntity::id_value(&self)
    }
}

impl EntityReference for &crate::Priority {
    fn entity_id_value(self) -> teaql_core::Value {
        teaql_core::IdentifiableEntity::id_value(self)
    }
}

// ⛔ AI agents: DO NOT read this file for API discovery. Instead run: cargo teaql --input modeling/MODEL.xml rust-assist-query/priority
#[derive(Debug)]
pub struct PriorityRequest<R = crate::Priority> {
    query: SelectQuery,
    relation_selections: Vec<RelationSelection>,
    relation_filters: Vec<RelationFilter>,
    child_enhancements: Vec<QuerySelection>,
    query_options: QueryOptions,
    marker: PhantomData<R>,
}

impl<R> Clone for PriorityRequest<R> {
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

impl<R> PriorityRequest<R> {
    pub(crate) fn new() -> Self {
        Self {
            query: SelectQuery::new("Priority")
                .project("id")
                .project("version"),
            relation_selections: Vec::new(),
            relation_filters: Vec::new(),
            child_enhancements: Vec::new(),
            query_options: QueryOptions::default(),
            marker: PhantomData,
        }
    }

    pub fn return_type<T>(self) -> PriorityRequest<T> {
        PriorityRequest {
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
                "Priority",
                query,
                relation_aggregates,
            ).await?;
            (rows, std::collections::BTreeMap::new())
        } else {
            let rows = context.fetch_entity_smart_list::<R>(
                "Priority",
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
        context.fetch_compact_smart_list("Priority", &query).await
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
        Ok(context.fetch_entity_stream("Priority", query))
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
        let rows = context.fetch_compact_rows("Priority", &query).await?;
        rows.first()
            .and_then(|row| row.get(COUNT_ALIAS))
            .and_then(teaql_core::Value::try_u64)
            .ok_or_else(|| RuntimeError::Graph(format!("count result for Priority is missing or not numeric")))
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
        let rows = context.fetch_compact_rows("Priority", &query).await?;
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
            "code" => Some("code"),
            "active" => Some("active"),
            "rank" => Some("rank"),
            "weight" => Some("weight"),
            "ratio" => Some("ratio"),
            "cost" => Some("cost"),
            "day" => Some("day"),
            "payload" => Some("payload"),
            "note" => Some("note"),
            "optional_rank" => Some("optional_rank"),
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
            "label_list" => {
                self.with_label_list_matching(
                    crate::Q::labels_minimal()
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
        self.query = self.query.project("code");
        self.query = self.query.project("active");
        self.query = self.query.project("rank");
        self.query = self.query.project("weight");
        self.query = self.query.project("ratio");
        self.query = self.query.project("cost");
        self.query = self.query.project("day");
        self.query = self.query.project("payload");
        self.query = self.query.project("note");
        self.query = self.query.project("optional_rank");
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
        request = request.select_label_list();
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


    pub fn select_code(mut self) -> Self {
        self.query = self.query.project("code");
        self
    }

    pub fn project_code(self) -> Self {
        self.select_code()
    }

    pub fn select_code_raw(self, raw_sql_segment: impl Into<String>) -> Self {
        self.select_code_unsafe_raw(UnsafeRawSqlSegment::trusted(raw_sql_segment))
    }

    pub fn select_code_unsafe_raw(mut self, raw_sql_segment: UnsafeRawSqlSegment) -> Self {
        self.query_options
            .raw_projections
            .push(RawProjection::new("code", raw_sql_segment));
        self
    }

    pub fn group_by_code(self) -> Self {
        self.group_by("code")
    }

    pub fn group_by_code_as(self, alias: impl Into<String>) -> Self {
        let alias = alias.into();
        let mut request = self.group_by("code");
        request.query = request
            .query
            .project_expr(alias, Expr::column("code"));
        request
    }

    pub fn group_by_code_with_function(
        self,
        alias: impl Into<String>,
        function: AggregateFunction,
    ) -> Self {
        self.group_by("code")
            .aggregate_with_function("code", alias, function)
    }

    pub fn count_code(self) -> Self {
        self.count_code_as("code_count")
    }

    pub fn count_code_as(self, alias: impl Into<String>) -> Self {
        self.aggregate_count_field("code", alias)
    }

    pub fn sum_code(self) -> Self {
        self.sum_code_as("sum_code")
    }

    pub fn sum_code_as(self, alias: impl Into<String>) -> Self {
        self.aggregate_sum("code", alias)
    }

    pub fn avg_code(self) -> Self {
        self.avg_code_as("avg_code")
    }

    pub fn avg_code_as(self, alias: impl Into<String>) -> Self {
        self.aggregate_avg("code", alias)
    }

    pub fn min_code(self) -> Self {
        self.min_code_as("min_code")
    }

    pub fn min_code_as(self, alias: impl Into<String>) -> Self {
        self.aggregate_min("code", alias)
    }

    pub fn max_code(self) -> Self {
        self.max_code_as("max_code")
    }

    pub fn max_code_as(self, alias: impl Into<String>) -> Self {
        self.aggregate_max("code", alias)
    }

    pub fn unselect_code(mut self) -> Self {
        self.query.projection.retain(|field| field != "code");
        self.query_options.raw_projections.retain(|projection| projection.property_name != "code");
        self
    }


    pub fn with_code(
        mut self,
        operator: FieldOperator,
        values: impl IntoIterator<Item = impl Into<teaql_core::Value>>,
    ) -> Self {
        self.query = self.query.and_filter(field_operator_expr(
            "code",
            operator,
            values.into_iter().map(Into::into).collect(),
        ));
        self
    }

    pub fn create_code_criteria(
        operator: FieldOperator,
        values: impl IntoIterator<Item = impl Into<teaql_core::Value>>,
    ) -> Expr {
        field_operator_expr(
            "code",
            operator,
            values.into_iter().map(Into::into).collect(),
        )
    }

    pub fn with_code_is(mut self, value: impl Into<teaql_core::Value>) -> Self {
        self.query = self.query.and_filter(Expr::eq("code", value));
        self
    }



    pub fn with_code_is_not(mut self, value: impl Into<teaql_core::Value>) -> Self {
        self.query = self.query.and_filter(Expr::ne("code", value));
        self
    }

    pub fn with_code_greater_than(mut self, value: impl Into<teaql_core::Value>) -> Self {
        self.query = self.query.and_filter(Expr::gt("code", value));
        self
    }

    pub fn with_code_greater_than_or_equal_to(mut self, value: impl Into<teaql_core::Value>) -> Self {
        self.query = self.query.and_filter(Expr::gte("code", value));
        self
    }

    pub fn with_code_less_than(mut self, value: impl Into<teaql_core::Value>) -> Self {
        self.query = self.query.and_filter(Expr::lt("code", value));
        self
    }

    pub fn with_code_less_than_or_equal_to(mut self, value: impl Into<teaql_core::Value>) -> Self {
        self.query = self.query.and_filter(Expr::lte("code", value));
        self
    }

    pub fn with_code_between(
        mut self,
        lower: impl Into<teaql_core::Value>,
        upper: impl Into<teaql_core::Value>,
    ) -> Self {
        self.query = self.query.and_filter(Expr::between("code", lower, upper));
        self
    }

    pub fn with_code_between_range<T>(mut self, range: DateRange<T>) -> Self
    where
        T: Into<teaql_core::Value>,
    {
        self.query = self.query.and_filter(Expr::between(
            "code",
            range.start,
            range.end,
        ));
        self
    }

    pub fn with_code_in(
        mut self,
        values: impl IntoIterator<Item = impl Into<teaql_core::Value>>,
    ) -> Self {
        self.query = self.query.and_filter(Expr::in_list(
            "code",
            values.into_iter().map(Into::into),
        ));
        self
    }

    pub fn with_code_not_in(
        mut self,
        values: impl IntoIterator<Item = impl Into<teaql_core::Value>>,
    ) -> Self {
        self.query = self.query.and_filter(Expr::not_in_list(
            "code",
            values.into_iter().map(Into::into),
        ));
        self
    }

    pub fn with_code_containing(mut self, value: impl Into<String>) -> Self {
        self.query = self.query.and_filter(Expr::contain("code", value));
        self
    }

    pub fn with_code_not_containing(mut self, value: impl Into<String>) -> Self {
        self.query = self.query.and_filter(Expr::not_contain("code", value));
        self
    }

    pub fn with_code_starting_with(mut self, value: impl Into<String>) -> Self {
        self.query = self.query.and_filter(Expr::begin_with("code", value));
        self
    }

    pub fn with_code_not_starting_with(mut self, value: impl Into<String>) -> Self {
        self.query = self.query.and_filter(Expr::not_begin_with("code", value));
        self
    }

    pub fn with_code_ending_with(mut self, value: impl Into<String>) -> Self {
        self.query = self.query.and_filter(Expr::end_with("code", value));
        self
    }

    pub fn with_code_not_ending_with(mut self, value: impl Into<String>) -> Self {
        self.query = self.query.and_filter(Expr::not_end_with("code", value));
        self
    }

    pub fn with_code_sounding_like(mut self, value: impl Into<teaql_core::Value>) -> Self {
        self.query = self.query.and_filter(Expr::sound_like("code", value));
        self
    }
    pub fn with_code_before(mut self, value: impl Into<teaql_core::Value>) -> Self {
        self.query = self.query.and_filter(Expr::lt("code", value));
        self
    }

    pub fn with_code_after(mut self, value: impl Into<teaql_core::Value>) -> Self {
        self.query = self.query.and_filter(Expr::gt("code", value));
        self
    }

    pub fn with_code_is_unknown(mut self) -> Self {
        self.query = self.query.and_filter(Expr::is_null("code"));
        self
    }



    pub fn with_code_is_known(mut self) -> Self {
        self.query = self.query.and_filter(Expr::is_not_null("code"));
        self
    }


    pub fn order_by_code_asc(mut self) -> Self {
        self.query = self.query.order_asc("code");
        self
    }

    pub fn order_by_code_desc(mut self) -> Self {
        self.query = self.query.order_desc("code");
        self
    }

    pub fn order_by_code_asc_using_gbk(mut self) -> Self {
        self.query = self.query.order_gbk_asc("code");
        self
    }

    pub fn order_by_code_desc_using_gbk(mut self) -> Self {
        self.query = self.query.order_gbk_desc("code");
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


    pub fn select_rank(mut self) -> Self {
        self.query = self.query.project("rank");
        self
    }

    pub fn project_rank(self) -> Self {
        self.select_rank()
    }

    pub fn select_rank_raw(self, raw_sql_segment: impl Into<String>) -> Self {
        self.select_rank_unsafe_raw(UnsafeRawSqlSegment::trusted(raw_sql_segment))
    }

    pub fn select_rank_unsafe_raw(mut self, raw_sql_segment: UnsafeRawSqlSegment) -> Self {
        self.query_options
            .raw_projections
            .push(RawProjection::new("rank", raw_sql_segment));
        self
    }

    pub fn select_rank_with_function(self, function: AggregateFunction) -> Self {
        self.select_rank_as_with_function("rank", function)
    }

    pub fn select_rank_as_with_function(
        self,
        alias: impl Into<String>,
        function: AggregateFunction,
    ) -> Self {
        self.aggregate_with_function("rank", alias, function)
    }

    pub fn group_by_rank(self) -> Self {
        self.group_by("rank")
    }

    pub fn group_by_rank_as(self, alias: impl Into<String>) -> Self {
        let alias = alias.into();
        let mut request = self.group_by("rank");
        request.query = request
            .query
            .project_expr(alias, Expr::column("rank"));
        request
    }

    pub fn group_by_rank_with_function(
        self,
        alias: impl Into<String>,
        function: AggregateFunction,
    ) -> Self {
        self.group_by("rank")
            .aggregate_with_function("rank", alias, function)
    }

    pub fn count_rank(self) -> Self {
        self.count_rank_as("rank_count")
    }

    pub fn count_rank_as(self, alias: impl Into<String>) -> Self {
        self.aggregate_count_field("rank", alias)
    }

    pub fn sum_rank(self) -> Self {
        self.sum_rank_as("sum_rank")
    }

    pub fn sum_rank_as(self, alias: impl Into<String>) -> Self {
        self.aggregate_sum("rank", alias)
    }

    pub fn avg_rank(self) -> Self {
        self.avg_rank_as("avg_rank")
    }

    pub fn avg_rank_as(self, alias: impl Into<String>) -> Self {
        self.aggregate_avg("rank", alias)
    }

    pub fn min_rank(self) -> Self {
        self.min_rank_as("min_rank")
    }

    pub fn min_rank_as(self, alias: impl Into<String>) -> Self {
        self.aggregate_min("rank", alias)
    }

    pub fn max_rank(self) -> Self {
        self.max_rank_as("max_rank")
    }

    pub fn max_rank_as(self, alias: impl Into<String>) -> Self {
        self.aggregate_max("rank", alias)
    }

    pub fn standard_deviation_rank(self) -> Self {
        self.standard_deviation_rank_as("stdDev_rank")
    }

    pub fn standard_deviation_rank_as(self, alias: impl Into<String>) -> Self {
        self.aggregate_stddev("rank", alias)
    }

    pub fn square_root_of_population_standard_deviation_rank(self) -> Self {
        self.square_root_of_population_standard_deviation_rank_as("stdDevPop_rank")
    }

    pub fn square_root_of_population_standard_deviation_rank_as(self, alias: impl Into<String>) -> Self {
        self.aggregate_stddev_pop("rank", alias)
    }

    pub fn sample_variance_rank(self) -> Self {
        self.sample_variance_rank_as("varSamp_rank")
    }

    pub fn sample_variance_rank_as(self, alias: impl Into<String>) -> Self {
        self.aggregate_var_samp("rank", alias)
    }

    pub fn sample_population_variance_rank(self) -> Self {
        self.sample_population_variance_rank_as("varPop_rank")
    }

    pub fn sample_population_variance_rank_as(self, alias: impl Into<String>) -> Self {
        self.aggregate_var_pop("rank", alias)
    }

    pub fn unselect_rank(mut self) -> Self {
        self.query.projection.retain(|field| field != "rank");
        self.query_options.raw_projections.retain(|projection| projection.property_name != "rank");
        self
    }


    pub fn with_rank(
        mut self,
        operator: FieldOperator,
        values: impl IntoIterator<Item = impl Into<teaql_core::Value>>,
    ) -> Self {
        self.query = self.query.and_filter(field_operator_expr(
            "rank",
            operator,
            values.into_iter().map(Into::into).collect(),
        ));
        self
    }

    pub fn create_rank_criteria(
        operator: FieldOperator,
        values: impl IntoIterator<Item = impl Into<teaql_core::Value>>,
    ) -> Expr {
        field_operator_expr(
            "rank",
            operator,
            values.into_iter().map(Into::into).collect(),
        )
    }

    pub fn with_rank_is(mut self, value: impl Into<teaql_core::Value>) -> Self {
        self.query = self.query.and_filter(Expr::eq("rank", value));
        self
    }



    pub fn with_rank_is_not(mut self, value: impl Into<teaql_core::Value>) -> Self {
        self.query = self.query.and_filter(Expr::ne("rank", value));
        self
    }

    pub fn with_rank_greater_than(mut self, value: impl Into<teaql_core::Value>) -> Self {
        self.query = self.query.and_filter(Expr::gt("rank", value));
        self
    }

    pub fn with_rank_greater_than_or_equal_to(mut self, value: impl Into<teaql_core::Value>) -> Self {
        self.query = self.query.and_filter(Expr::gte("rank", value));
        self
    }

    pub fn with_rank_less_than(mut self, value: impl Into<teaql_core::Value>) -> Self {
        self.query = self.query.and_filter(Expr::lt("rank", value));
        self
    }

    pub fn with_rank_less_than_or_equal_to(mut self, value: impl Into<teaql_core::Value>) -> Self {
        self.query = self.query.and_filter(Expr::lte("rank", value));
        self
    }

    pub fn with_rank_between(
        mut self,
        lower: impl Into<teaql_core::Value>,
        upper: impl Into<teaql_core::Value>,
    ) -> Self {
        self.query = self.query.and_filter(Expr::between("rank", lower, upper));
        self
    }

    pub fn with_rank_between_range<T>(mut self, range: DateRange<T>) -> Self
    where
        T: Into<teaql_core::Value>,
    {
        self.query = self.query.and_filter(Expr::between(
            "rank",
            range.start,
            range.end,
        ));
        self
    }

    pub fn with_rank_in(
        mut self,
        values: impl IntoIterator<Item = impl Into<teaql_core::Value>>,
    ) -> Self {
        self.query = self.query.and_filter(Expr::in_list(
            "rank",
            values.into_iter().map(Into::into),
        ));
        self
    }

    pub fn with_rank_not_in(
        mut self,
        values: impl IntoIterator<Item = impl Into<teaql_core::Value>>,
    ) -> Self {
        self.query = self.query.and_filter(Expr::not_in_list(
            "rank",
            values.into_iter().map(Into::into),
        ));
        self
    }

    pub fn with_rank_before(mut self, value: impl Into<teaql_core::Value>) -> Self {
        self.query = self.query.and_filter(Expr::lt("rank", value));
        self
    }

    pub fn with_rank_after(mut self, value: impl Into<teaql_core::Value>) -> Self {
        self.query = self.query.and_filter(Expr::gt("rank", value));
        self
    }

    pub fn with_rank_is_unknown(mut self) -> Self {
        self.query = self.query.and_filter(Expr::is_null("rank"));
        self
    }



    pub fn with_rank_is_known(mut self) -> Self {
        self.query = self.query.and_filter(Expr::is_not_null("rank"));
        self
    }


    pub fn order_by_rank_asc(mut self) -> Self {
        self.query = self.query.order_asc("rank");
        self
    }

    pub fn order_by_rank_desc(mut self) -> Self {
        self.query = self.query.order_desc("rank");
        self
    }

    pub fn order_by_rank_asc_using_gbk(mut self) -> Self {
        self.query = self.query.order_gbk_asc("rank");
        self
    }

    pub fn order_by_rank_desc_using_gbk(mut self) -> Self {
        self.query = self.query.order_gbk_desc("rank");
        self
    }


    pub fn select_weight(mut self) -> Self {
        self.query = self.query.project("weight");
        self
    }

    pub fn project_weight(self) -> Self {
        self.select_weight()
    }

    pub fn select_weight_raw(self, raw_sql_segment: impl Into<String>) -> Self {
        self.select_weight_unsafe_raw(UnsafeRawSqlSegment::trusted(raw_sql_segment))
    }

    pub fn select_weight_unsafe_raw(mut self, raw_sql_segment: UnsafeRawSqlSegment) -> Self {
        self.query_options
            .raw_projections
            .push(RawProjection::new("weight", raw_sql_segment));
        self
    }

    pub fn select_weight_with_function(self, function: AggregateFunction) -> Self {
        self.select_weight_as_with_function("weight", function)
    }

    pub fn select_weight_as_with_function(
        self,
        alias: impl Into<String>,
        function: AggregateFunction,
    ) -> Self {
        self.aggregate_with_function("weight", alias, function)
    }

    pub fn group_by_weight(self) -> Self {
        self.group_by("weight")
    }

    pub fn group_by_weight_as(self, alias: impl Into<String>) -> Self {
        let alias = alias.into();
        let mut request = self.group_by("weight");
        request.query = request
            .query
            .project_expr(alias, Expr::column("weight"));
        request
    }

    pub fn group_by_weight_with_function(
        self,
        alias: impl Into<String>,
        function: AggregateFunction,
    ) -> Self {
        self.group_by("weight")
            .aggregate_with_function("weight", alias, function)
    }

    pub fn count_weight(self) -> Self {
        self.count_weight_as("weight_count")
    }

    pub fn count_weight_as(self, alias: impl Into<String>) -> Self {
        self.aggregate_count_field("weight", alias)
    }

    pub fn sum_weight(self) -> Self {
        self.sum_weight_as("sum_weight")
    }

    pub fn sum_weight_as(self, alias: impl Into<String>) -> Self {
        self.aggregate_sum("weight", alias)
    }

    pub fn avg_weight(self) -> Self {
        self.avg_weight_as("avg_weight")
    }

    pub fn avg_weight_as(self, alias: impl Into<String>) -> Self {
        self.aggregate_avg("weight", alias)
    }

    pub fn min_weight(self) -> Self {
        self.min_weight_as("min_weight")
    }

    pub fn min_weight_as(self, alias: impl Into<String>) -> Self {
        self.aggregate_min("weight", alias)
    }

    pub fn max_weight(self) -> Self {
        self.max_weight_as("max_weight")
    }

    pub fn max_weight_as(self, alias: impl Into<String>) -> Self {
        self.aggregate_max("weight", alias)
    }

    pub fn standard_deviation_weight(self) -> Self {
        self.standard_deviation_weight_as("stdDev_weight")
    }

    pub fn standard_deviation_weight_as(self, alias: impl Into<String>) -> Self {
        self.aggregate_stddev("weight", alias)
    }

    pub fn square_root_of_population_standard_deviation_weight(self) -> Self {
        self.square_root_of_population_standard_deviation_weight_as("stdDevPop_weight")
    }

    pub fn square_root_of_population_standard_deviation_weight_as(self, alias: impl Into<String>) -> Self {
        self.aggregate_stddev_pop("weight", alias)
    }

    pub fn sample_variance_weight(self) -> Self {
        self.sample_variance_weight_as("varSamp_weight")
    }

    pub fn sample_variance_weight_as(self, alias: impl Into<String>) -> Self {
        self.aggregate_var_samp("weight", alias)
    }

    pub fn sample_population_variance_weight(self) -> Self {
        self.sample_population_variance_weight_as("varPop_weight")
    }

    pub fn sample_population_variance_weight_as(self, alias: impl Into<String>) -> Self {
        self.aggregate_var_pop("weight", alias)
    }

    pub fn unselect_weight(mut self) -> Self {
        self.query.projection.retain(|field| field != "weight");
        self.query_options.raw_projections.retain(|projection| projection.property_name != "weight");
        self
    }


    pub fn with_weight(
        mut self,
        operator: FieldOperator,
        values: impl IntoIterator<Item = impl Into<teaql_core::Value>>,
    ) -> Self {
        self.query = self.query.and_filter(field_operator_expr(
            "weight",
            operator,
            values.into_iter().map(Into::into).collect(),
        ));
        self
    }

    pub fn create_weight_criteria(
        operator: FieldOperator,
        values: impl IntoIterator<Item = impl Into<teaql_core::Value>>,
    ) -> Expr {
        field_operator_expr(
            "weight",
            operator,
            values.into_iter().map(Into::into).collect(),
        )
    }

    pub fn with_weight_is(mut self, value: impl Into<teaql_core::Value>) -> Self {
        self.query = self.query.and_filter(Expr::eq("weight", value));
        self
    }



    pub fn with_weight_is_not(mut self, value: impl Into<teaql_core::Value>) -> Self {
        self.query = self.query.and_filter(Expr::ne("weight", value));
        self
    }

    pub fn with_weight_greater_than(mut self, value: impl Into<teaql_core::Value>) -> Self {
        self.query = self.query.and_filter(Expr::gt("weight", value));
        self
    }

    pub fn with_weight_greater_than_or_equal_to(mut self, value: impl Into<teaql_core::Value>) -> Self {
        self.query = self.query.and_filter(Expr::gte("weight", value));
        self
    }

    pub fn with_weight_less_than(mut self, value: impl Into<teaql_core::Value>) -> Self {
        self.query = self.query.and_filter(Expr::lt("weight", value));
        self
    }

    pub fn with_weight_less_than_or_equal_to(mut self, value: impl Into<teaql_core::Value>) -> Self {
        self.query = self.query.and_filter(Expr::lte("weight", value));
        self
    }

    pub fn with_weight_between(
        mut self,
        lower: impl Into<teaql_core::Value>,
        upper: impl Into<teaql_core::Value>,
    ) -> Self {
        self.query = self.query.and_filter(Expr::between("weight", lower, upper));
        self
    }

    pub fn with_weight_between_range<T>(mut self, range: DateRange<T>) -> Self
    where
        T: Into<teaql_core::Value>,
    {
        self.query = self.query.and_filter(Expr::between(
            "weight",
            range.start,
            range.end,
        ));
        self
    }

    pub fn with_weight_in(
        mut self,
        values: impl IntoIterator<Item = impl Into<teaql_core::Value>>,
    ) -> Self {
        self.query = self.query.and_filter(Expr::in_list(
            "weight",
            values.into_iter().map(Into::into),
        ));
        self
    }

    pub fn with_weight_not_in(
        mut self,
        values: impl IntoIterator<Item = impl Into<teaql_core::Value>>,
    ) -> Self {
        self.query = self.query.and_filter(Expr::not_in_list(
            "weight",
            values.into_iter().map(Into::into),
        ));
        self
    }

    pub fn with_weight_before(mut self, value: impl Into<teaql_core::Value>) -> Self {
        self.query = self.query.and_filter(Expr::lt("weight", value));
        self
    }

    pub fn with_weight_after(mut self, value: impl Into<teaql_core::Value>) -> Self {
        self.query = self.query.and_filter(Expr::gt("weight", value));
        self
    }

    pub fn with_weight_is_unknown(mut self) -> Self {
        self.query = self.query.and_filter(Expr::is_null("weight"));
        self
    }



    pub fn with_weight_is_known(mut self) -> Self {
        self.query = self.query.and_filter(Expr::is_not_null("weight"));
        self
    }


    pub fn order_by_weight_asc(mut self) -> Self {
        self.query = self.query.order_asc("weight");
        self
    }

    pub fn order_by_weight_desc(mut self) -> Self {
        self.query = self.query.order_desc("weight");
        self
    }

    pub fn order_by_weight_asc_using_gbk(mut self) -> Self {
        self.query = self.query.order_gbk_asc("weight");
        self
    }

    pub fn order_by_weight_desc_using_gbk(mut self) -> Self {
        self.query = self.query.order_gbk_desc("weight");
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


    pub fn select_cost(mut self) -> Self {
        self.query = self.query.project("cost");
        self
    }

    pub fn project_cost(self) -> Self {
        self.select_cost()
    }

    pub fn select_cost_raw(self, raw_sql_segment: impl Into<String>) -> Self {
        self.select_cost_unsafe_raw(UnsafeRawSqlSegment::trusted(raw_sql_segment))
    }

    pub fn select_cost_unsafe_raw(mut self, raw_sql_segment: UnsafeRawSqlSegment) -> Self {
        self.query_options
            .raw_projections
            .push(RawProjection::new("cost", raw_sql_segment));
        self
    }

    pub fn select_cost_with_function(self, function: AggregateFunction) -> Self {
        self.select_cost_as_with_function("cost", function)
    }

    pub fn select_cost_as_with_function(
        self,
        alias: impl Into<String>,
        function: AggregateFunction,
    ) -> Self {
        self.aggregate_with_function("cost", alias, function)
    }

    pub fn group_by_cost(self) -> Self {
        self.group_by("cost")
    }

    pub fn group_by_cost_as(self, alias: impl Into<String>) -> Self {
        let alias = alias.into();
        let mut request = self.group_by("cost");
        request.query = request
            .query
            .project_expr(alias, Expr::column("cost"));
        request
    }

    pub fn group_by_cost_with_function(
        self,
        alias: impl Into<String>,
        function: AggregateFunction,
    ) -> Self {
        self.group_by("cost")
            .aggregate_with_function("cost", alias, function)
    }

    pub fn count_cost(self) -> Self {
        self.count_cost_as("cost_count")
    }

    pub fn count_cost_as(self, alias: impl Into<String>) -> Self {
        self.aggregate_count_field("cost", alias)
    }

    pub fn sum_cost(self) -> Self {
        self.sum_cost_as("sum_cost")
    }

    pub fn sum_cost_as(self, alias: impl Into<String>) -> Self {
        self.aggregate_sum("cost", alias)
    }

    pub fn avg_cost(self) -> Self {
        self.avg_cost_as("avg_cost")
    }

    pub fn avg_cost_as(self, alias: impl Into<String>) -> Self {
        self.aggregate_avg("cost", alias)
    }

    pub fn min_cost(self) -> Self {
        self.min_cost_as("min_cost")
    }

    pub fn min_cost_as(self, alias: impl Into<String>) -> Self {
        self.aggregate_min("cost", alias)
    }

    pub fn max_cost(self) -> Self {
        self.max_cost_as("max_cost")
    }

    pub fn max_cost_as(self, alias: impl Into<String>) -> Self {
        self.aggregate_max("cost", alias)
    }

    pub fn standard_deviation_cost(self) -> Self {
        self.standard_deviation_cost_as("stdDev_cost")
    }

    pub fn standard_deviation_cost_as(self, alias: impl Into<String>) -> Self {
        self.aggregate_stddev("cost", alias)
    }

    pub fn square_root_of_population_standard_deviation_cost(self) -> Self {
        self.square_root_of_population_standard_deviation_cost_as("stdDevPop_cost")
    }

    pub fn square_root_of_population_standard_deviation_cost_as(self, alias: impl Into<String>) -> Self {
        self.aggregate_stddev_pop("cost", alias)
    }

    pub fn sample_variance_cost(self) -> Self {
        self.sample_variance_cost_as("varSamp_cost")
    }

    pub fn sample_variance_cost_as(self, alias: impl Into<String>) -> Self {
        self.aggregate_var_samp("cost", alias)
    }

    pub fn sample_population_variance_cost(self) -> Self {
        self.sample_population_variance_cost_as("varPop_cost")
    }

    pub fn sample_population_variance_cost_as(self, alias: impl Into<String>) -> Self {
        self.aggregate_var_pop("cost", alias)
    }

    pub fn unselect_cost(mut self) -> Self {
        self.query.projection.retain(|field| field != "cost");
        self.query_options.raw_projections.retain(|projection| projection.property_name != "cost");
        self
    }


    pub fn with_cost(
        mut self,
        operator: FieldOperator,
        values: impl IntoIterator<Item = impl Into<teaql_core::Value>>,
    ) -> Self {
        self.query = self.query.and_filter(field_operator_expr(
            "cost",
            operator,
            values.into_iter().map(Into::into).collect(),
        ));
        self
    }

    pub fn create_cost_criteria(
        operator: FieldOperator,
        values: impl IntoIterator<Item = impl Into<teaql_core::Value>>,
    ) -> Expr {
        field_operator_expr(
            "cost",
            operator,
            values.into_iter().map(Into::into).collect(),
        )
    }

    pub fn with_cost_is(mut self, value: impl Into<teaql_core::Value>) -> Self {
        self.query = self.query.and_filter(Expr::eq("cost", value));
        self
    }



    pub fn with_cost_is_not(mut self, value: impl Into<teaql_core::Value>) -> Self {
        self.query = self.query.and_filter(Expr::ne("cost", value));
        self
    }

    pub fn with_cost_greater_than(mut self, value: impl Into<teaql_core::Value>) -> Self {
        self.query = self.query.and_filter(Expr::gt("cost", value));
        self
    }

    pub fn with_cost_greater_than_or_equal_to(mut self, value: impl Into<teaql_core::Value>) -> Self {
        self.query = self.query.and_filter(Expr::gte("cost", value));
        self
    }

    pub fn with_cost_less_than(mut self, value: impl Into<teaql_core::Value>) -> Self {
        self.query = self.query.and_filter(Expr::lt("cost", value));
        self
    }

    pub fn with_cost_less_than_or_equal_to(mut self, value: impl Into<teaql_core::Value>) -> Self {
        self.query = self.query.and_filter(Expr::lte("cost", value));
        self
    }

    pub fn with_cost_between(
        mut self,
        lower: impl Into<teaql_core::Value>,
        upper: impl Into<teaql_core::Value>,
    ) -> Self {
        self.query = self.query.and_filter(Expr::between("cost", lower, upper));
        self
    }

    pub fn with_cost_between_range<T>(mut self, range: DateRange<T>) -> Self
    where
        T: Into<teaql_core::Value>,
    {
        self.query = self.query.and_filter(Expr::between(
            "cost",
            range.start,
            range.end,
        ));
        self
    }

    pub fn with_cost_in(
        mut self,
        values: impl IntoIterator<Item = impl Into<teaql_core::Value>>,
    ) -> Self {
        self.query = self.query.and_filter(Expr::in_list(
            "cost",
            values.into_iter().map(Into::into),
        ));
        self
    }

    pub fn with_cost_not_in(
        mut self,
        values: impl IntoIterator<Item = impl Into<teaql_core::Value>>,
    ) -> Self {
        self.query = self.query.and_filter(Expr::not_in_list(
            "cost",
            values.into_iter().map(Into::into),
        ));
        self
    }

    pub fn with_cost_before(mut self, value: impl Into<teaql_core::Value>) -> Self {
        self.query = self.query.and_filter(Expr::lt("cost", value));
        self
    }

    pub fn with_cost_after(mut self, value: impl Into<teaql_core::Value>) -> Self {
        self.query = self.query.and_filter(Expr::gt("cost", value));
        self
    }

    pub fn with_cost_is_unknown(mut self) -> Self {
        self.query = self.query.and_filter(Expr::is_null("cost"));
        self
    }



    pub fn with_cost_is_known(mut self) -> Self {
        self.query = self.query.and_filter(Expr::is_not_null("cost"));
        self
    }


    pub fn order_by_cost_asc(mut self) -> Self {
        self.query = self.query.order_asc("cost");
        self
    }

    pub fn order_by_cost_desc(mut self) -> Self {
        self.query = self.query.order_desc("cost");
        self
    }

    pub fn order_by_cost_asc_using_gbk(mut self) -> Self {
        self.query = self.query.order_gbk_asc("cost");
        self
    }

    pub fn order_by_cost_desc_using_gbk(mut self) -> Self {
        self.query = self.query.order_gbk_desc("cost");
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


    pub fn select_note(mut self) -> Self {
        self.query = self.query.project("note");
        self
    }

    pub fn project_note(self) -> Self {
        self.select_note()
    }

    pub fn select_note_raw(self, raw_sql_segment: impl Into<String>) -> Self {
        self.select_note_unsafe_raw(UnsafeRawSqlSegment::trusted(raw_sql_segment))
    }

    pub fn select_note_unsafe_raw(mut self, raw_sql_segment: UnsafeRawSqlSegment) -> Self {
        self.query_options
            .raw_projections
            .push(RawProjection::new("note", raw_sql_segment));
        self
    }

    pub fn group_by_note(self) -> Self {
        self.group_by("note")
    }

    pub fn group_by_note_as(self, alias: impl Into<String>) -> Self {
        let alias = alias.into();
        let mut request = self.group_by("note");
        request.query = request
            .query
            .project_expr(alias, Expr::column("note"));
        request
    }

    pub fn group_by_note_with_function(
        self,
        alias: impl Into<String>,
        function: AggregateFunction,
    ) -> Self {
        self.group_by("note")
            .aggregate_with_function("note", alias, function)
    }

    pub fn count_note(self) -> Self {
        self.count_note_as("note_count")
    }

    pub fn count_note_as(self, alias: impl Into<String>) -> Self {
        self.aggregate_count_field("note", alias)
    }

    pub fn sum_note(self) -> Self {
        self.sum_note_as("sum_note")
    }

    pub fn sum_note_as(self, alias: impl Into<String>) -> Self {
        self.aggregate_sum("note", alias)
    }

    pub fn avg_note(self) -> Self {
        self.avg_note_as("avg_note")
    }

    pub fn avg_note_as(self, alias: impl Into<String>) -> Self {
        self.aggregate_avg("note", alias)
    }

    pub fn min_note(self) -> Self {
        self.min_note_as("min_note")
    }

    pub fn min_note_as(self, alias: impl Into<String>) -> Self {
        self.aggregate_min("note", alias)
    }

    pub fn max_note(self) -> Self {
        self.max_note_as("max_note")
    }

    pub fn max_note_as(self, alias: impl Into<String>) -> Self {
        self.aggregate_max("note", alias)
    }

    pub fn unselect_note(mut self) -> Self {
        self.query.projection.retain(|field| field != "note");
        self.query_options.raw_projections.retain(|projection| projection.property_name != "note");
        self
    }


    pub fn with_note(
        mut self,
        operator: FieldOperator,
        values: impl IntoIterator<Item = impl Into<teaql_core::Value>>,
    ) -> Self {
        self.query = self.query.and_filter(field_operator_expr(
            "note",
            operator,
            values.into_iter().map(Into::into).collect(),
        ));
        self
    }

    pub fn create_note_criteria(
        operator: FieldOperator,
        values: impl IntoIterator<Item = impl Into<teaql_core::Value>>,
    ) -> Expr {
        field_operator_expr(
            "note",
            operator,
            values.into_iter().map(Into::into).collect(),
        )
    }

    pub fn with_note_is(mut self, value: impl Into<teaql_core::Value>) -> Self {
        self.query = self.query.and_filter(Expr::eq("note", value));
        self
    }



    pub fn with_note_is_not(mut self, value: impl Into<teaql_core::Value>) -> Self {
        self.query = self.query.and_filter(Expr::ne("note", value));
        self
    }

    pub fn with_note_greater_than(mut self, value: impl Into<teaql_core::Value>) -> Self {
        self.query = self.query.and_filter(Expr::gt("note", value));
        self
    }

    pub fn with_note_greater_than_or_equal_to(mut self, value: impl Into<teaql_core::Value>) -> Self {
        self.query = self.query.and_filter(Expr::gte("note", value));
        self
    }

    pub fn with_note_less_than(mut self, value: impl Into<teaql_core::Value>) -> Self {
        self.query = self.query.and_filter(Expr::lt("note", value));
        self
    }

    pub fn with_note_less_than_or_equal_to(mut self, value: impl Into<teaql_core::Value>) -> Self {
        self.query = self.query.and_filter(Expr::lte("note", value));
        self
    }

    pub fn with_note_between(
        mut self,
        lower: impl Into<teaql_core::Value>,
        upper: impl Into<teaql_core::Value>,
    ) -> Self {
        self.query = self.query.and_filter(Expr::between("note", lower, upper));
        self
    }

    pub fn with_note_between_range<T>(mut self, range: DateRange<T>) -> Self
    where
        T: Into<teaql_core::Value>,
    {
        self.query = self.query.and_filter(Expr::between(
            "note",
            range.start,
            range.end,
        ));
        self
    }

    pub fn with_note_in(
        mut self,
        values: impl IntoIterator<Item = impl Into<teaql_core::Value>>,
    ) -> Self {
        self.query = self.query.and_filter(Expr::in_list(
            "note",
            values.into_iter().map(Into::into),
        ));
        self
    }

    pub fn with_note_not_in(
        mut self,
        values: impl IntoIterator<Item = impl Into<teaql_core::Value>>,
    ) -> Self {
        self.query = self.query.and_filter(Expr::not_in_list(
            "note",
            values.into_iter().map(Into::into),
        ));
        self
    }

    pub fn with_note_containing(mut self, value: impl Into<String>) -> Self {
        self.query = self.query.and_filter(Expr::contain("note", value));
        self
    }

    pub fn with_note_not_containing(mut self, value: impl Into<String>) -> Self {
        self.query = self.query.and_filter(Expr::not_contain("note", value));
        self
    }

    pub fn with_note_starting_with(mut self, value: impl Into<String>) -> Self {
        self.query = self.query.and_filter(Expr::begin_with("note", value));
        self
    }

    pub fn with_note_not_starting_with(mut self, value: impl Into<String>) -> Self {
        self.query = self.query.and_filter(Expr::not_begin_with("note", value));
        self
    }

    pub fn with_note_ending_with(mut self, value: impl Into<String>) -> Self {
        self.query = self.query.and_filter(Expr::end_with("note", value));
        self
    }

    pub fn with_note_not_ending_with(mut self, value: impl Into<String>) -> Self {
        self.query = self.query.and_filter(Expr::not_end_with("note", value));
        self
    }

    pub fn with_note_sounding_like(mut self, value: impl Into<teaql_core::Value>) -> Self {
        self.query = self.query.and_filter(Expr::sound_like("note", value));
        self
    }
    pub fn with_note_before(mut self, value: impl Into<teaql_core::Value>) -> Self {
        self.query = self.query.and_filter(Expr::lt("note", value));
        self
    }

    pub fn with_note_after(mut self, value: impl Into<teaql_core::Value>) -> Self {
        self.query = self.query.and_filter(Expr::gt("note", value));
        self
    }

    pub fn with_note_is_unknown(mut self) -> Self {
        self.query = self.query.and_filter(Expr::is_null("note"));
        self
    }



    pub fn with_note_is_known(mut self) -> Self {
        self.query = self.query.and_filter(Expr::is_not_null("note"));
        self
    }


    pub fn order_by_note_asc(mut self) -> Self {
        self.query = self.query.order_asc("note");
        self
    }

    pub fn order_by_note_desc(mut self) -> Self {
        self.query = self.query.order_desc("note");
        self
    }

    pub fn order_by_note_asc_using_gbk(mut self) -> Self {
        self.query = self.query.order_gbk_asc("note");
        self
    }

    pub fn order_by_note_desc_using_gbk(mut self) -> Self {
        self.query = self.query.order_gbk_desc("note");
        self
    }


    pub fn select_optional_rank(mut self) -> Self {
        self.query = self.query.project("optional_rank");
        self
    }

    pub fn project_optional_rank(self) -> Self {
        self.select_optional_rank()
    }

    pub fn select_optional_rank_raw(self, raw_sql_segment: impl Into<String>) -> Self {
        self.select_optional_rank_unsafe_raw(UnsafeRawSqlSegment::trusted(raw_sql_segment))
    }

    pub fn select_optional_rank_unsafe_raw(mut self, raw_sql_segment: UnsafeRawSqlSegment) -> Self {
        self.query_options
            .raw_projections
            .push(RawProjection::new("optional_rank", raw_sql_segment));
        self
    }

    pub fn select_optional_rank_with_function(self, function: AggregateFunction) -> Self {
        self.select_optional_rank_as_with_function("optional_rank", function)
    }

    pub fn select_optional_rank_as_with_function(
        self,
        alias: impl Into<String>,
        function: AggregateFunction,
    ) -> Self {
        self.aggregate_with_function("optional_rank", alias, function)
    }

    pub fn group_by_optional_rank(self) -> Self {
        self.group_by("optional_rank")
    }

    pub fn group_by_optional_rank_as(self, alias: impl Into<String>) -> Self {
        let alias = alias.into();
        let mut request = self.group_by("optional_rank");
        request.query = request
            .query
            .project_expr(alias, Expr::column("optional_rank"));
        request
    }

    pub fn group_by_optional_rank_with_function(
        self,
        alias: impl Into<String>,
        function: AggregateFunction,
    ) -> Self {
        self.group_by("optional_rank")
            .aggregate_with_function("optional_rank", alias, function)
    }

    pub fn count_optional_rank(self) -> Self {
        self.count_optional_rank_as("optional_rank_count")
    }

    pub fn count_optional_rank_as(self, alias: impl Into<String>) -> Self {
        self.aggregate_count_field("optional_rank", alias)
    }

    pub fn sum_optional_rank(self) -> Self {
        self.sum_optional_rank_as("sum_optional_rank")
    }

    pub fn sum_optional_rank_as(self, alias: impl Into<String>) -> Self {
        self.aggregate_sum("optional_rank", alias)
    }

    pub fn avg_optional_rank(self) -> Self {
        self.avg_optional_rank_as("avg_optional_rank")
    }

    pub fn avg_optional_rank_as(self, alias: impl Into<String>) -> Self {
        self.aggregate_avg("optional_rank", alias)
    }

    pub fn min_optional_rank(self) -> Self {
        self.min_optional_rank_as("min_optional_rank")
    }

    pub fn min_optional_rank_as(self, alias: impl Into<String>) -> Self {
        self.aggregate_min("optional_rank", alias)
    }

    pub fn max_optional_rank(self) -> Self {
        self.max_optional_rank_as("max_optional_rank")
    }

    pub fn max_optional_rank_as(self, alias: impl Into<String>) -> Self {
        self.aggregate_max("optional_rank", alias)
    }

    pub fn standard_deviation_optional_rank(self) -> Self {
        self.standard_deviation_optional_rank_as("stdDev_optional_rank")
    }

    pub fn standard_deviation_optional_rank_as(self, alias: impl Into<String>) -> Self {
        self.aggregate_stddev("optional_rank", alias)
    }

    pub fn square_root_of_population_standard_deviation_optional_rank(self) -> Self {
        self.square_root_of_population_standard_deviation_optional_rank_as("stdDevPop_optional_rank")
    }

    pub fn square_root_of_population_standard_deviation_optional_rank_as(self, alias: impl Into<String>) -> Self {
        self.aggregate_stddev_pop("optional_rank", alias)
    }

    pub fn sample_variance_optional_rank(self) -> Self {
        self.sample_variance_optional_rank_as("varSamp_optional_rank")
    }

    pub fn sample_variance_optional_rank_as(self, alias: impl Into<String>) -> Self {
        self.aggregate_var_samp("optional_rank", alias)
    }

    pub fn sample_population_variance_optional_rank(self) -> Self {
        self.sample_population_variance_optional_rank_as("varPop_optional_rank")
    }

    pub fn sample_population_variance_optional_rank_as(self, alias: impl Into<String>) -> Self {
        self.aggregate_var_pop("optional_rank", alias)
    }

    pub fn unselect_optional_rank(mut self) -> Self {
        self.query.projection.retain(|field| field != "optional_rank");
        self.query_options.raw_projections.retain(|projection| projection.property_name != "optional_rank");
        self
    }


    pub fn with_optional_rank(
        mut self,
        operator: FieldOperator,
        values: impl IntoIterator<Item = impl Into<teaql_core::Value>>,
    ) -> Self {
        self.query = self.query.and_filter(field_operator_expr(
            "optional_rank",
            operator,
            values.into_iter().map(Into::into).collect(),
        ));
        self
    }

    pub fn create_optional_rank_criteria(
        operator: FieldOperator,
        values: impl IntoIterator<Item = impl Into<teaql_core::Value>>,
    ) -> Expr {
        field_operator_expr(
            "optional_rank",
            operator,
            values.into_iter().map(Into::into).collect(),
        )
    }

    pub fn with_optional_rank_is(mut self, value: impl Into<teaql_core::Value>) -> Self {
        self.query = self.query.and_filter(Expr::eq("optional_rank", value));
        self
    }



    pub fn with_optional_rank_is_not(mut self, value: impl Into<teaql_core::Value>) -> Self {
        self.query = self.query.and_filter(Expr::ne("optional_rank", value));
        self
    }

    pub fn with_optional_rank_greater_than(mut self, value: impl Into<teaql_core::Value>) -> Self {
        self.query = self.query.and_filter(Expr::gt("optional_rank", value));
        self
    }

    pub fn with_optional_rank_greater_than_or_equal_to(mut self, value: impl Into<teaql_core::Value>) -> Self {
        self.query = self.query.and_filter(Expr::gte("optional_rank", value));
        self
    }

    pub fn with_optional_rank_less_than(mut self, value: impl Into<teaql_core::Value>) -> Self {
        self.query = self.query.and_filter(Expr::lt("optional_rank", value));
        self
    }

    pub fn with_optional_rank_less_than_or_equal_to(mut self, value: impl Into<teaql_core::Value>) -> Self {
        self.query = self.query.and_filter(Expr::lte("optional_rank", value));
        self
    }

    pub fn with_optional_rank_between(
        mut self,
        lower: impl Into<teaql_core::Value>,
        upper: impl Into<teaql_core::Value>,
    ) -> Self {
        self.query = self.query.and_filter(Expr::between("optional_rank", lower, upper));
        self
    }

    pub fn with_optional_rank_between_range<T>(mut self, range: DateRange<T>) -> Self
    where
        T: Into<teaql_core::Value>,
    {
        self.query = self.query.and_filter(Expr::between(
            "optional_rank",
            range.start,
            range.end,
        ));
        self
    }

    pub fn with_optional_rank_in(
        mut self,
        values: impl IntoIterator<Item = impl Into<teaql_core::Value>>,
    ) -> Self {
        self.query = self.query.and_filter(Expr::in_list(
            "optional_rank",
            values.into_iter().map(Into::into),
        ));
        self
    }

    pub fn with_optional_rank_not_in(
        mut self,
        values: impl IntoIterator<Item = impl Into<teaql_core::Value>>,
    ) -> Self {
        self.query = self.query.and_filter(Expr::not_in_list(
            "optional_rank",
            values.into_iter().map(Into::into),
        ));
        self
    }

    pub fn with_optional_rank_before(mut self, value: impl Into<teaql_core::Value>) -> Self {
        self.query = self.query.and_filter(Expr::lt("optional_rank", value));
        self
    }

    pub fn with_optional_rank_after(mut self, value: impl Into<teaql_core::Value>) -> Self {
        self.query = self.query.and_filter(Expr::gt("optional_rank", value));
        self
    }

    pub fn with_optional_rank_is_unknown(mut self) -> Self {
        self.query = self.query.and_filter(Expr::is_null("optional_rank"));
        self
    }



    pub fn with_optional_rank_is_known(mut self) -> Self {
        self.query = self.query.and_filter(Expr::is_not_null("optional_rank"));
        self
    }


    pub fn order_by_optional_rank_asc(mut self) -> Self {
        self.query = self.query.order_asc("optional_rank");
        self
    }

    pub fn order_by_optional_rank_desc(mut self) -> Self {
        self.query = self.query.order_desc("optional_rank");
        self
    }

    pub fn order_by_optional_rank_asc_using_gbk(mut self) -> Self {
        self.query = self.query.order_gbk_asc("optional_rank");
        self
    }

    pub fn order_by_optional_rank_desc_using_gbk(mut self) -> Self {
        self.query = self.query.order_gbk_desc("optional_rank");
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
    pub fn with_id_is_value_1(self) -> Self {
        self.with_id_is("1")
    }



    pub fn with_id_is_not_value_1(self) -> Self {
        self.with_id_is_not("1")
    }


    pub fn with_id_is_value_22(self) -> Self {
        self.with_id_is("22")
    }



    pub fn with_id_is_not_value_22(self) -> Self {
        self.with_id_is_not("22")
    }



    pub fn with_name_is_basic(self) -> Self {
        self.with_name_is("Basic")
    }



    pub fn with_name_is_not_basic(self) -> Self {
        self.with_name_is_not("Basic")
    }


    pub fn with_name_is_extended(self) -> Self {
        self.with_name_is("Extended")
    }



    pub fn with_name_is_not_extended(self) -> Self {
        self.with_name_is_not("Extended")
    }



    pub fn with_code_is_basi_c(self) -> Self {
        self.with_code_is("BASIC")
    }



    pub fn with_code_is_not_basi_c(self) -> Self {
        self.with_code_is_not("BASIC")
    }


    pub fn with_code_is_extende_d(self) -> Self {
        self.with_code_is("EXTENDED")
    }



    pub fn with_code_is_not_extende_d(self) -> Self {
        self.with_code_is_not("EXTENDED")
    }



    pub fn with_active_is_false(self) -> Self {
        self.with_active_is("false")
    }



    pub fn with_active_is_not_false(self) -> Self {
        self.with_active_is_not("false")
    }


    pub fn with_active_is_true(self) -> Self {
        self.with_active_is("true")
    }



    pub fn with_active_is_not_true(self) -> Self {
        self.with_active_is_not("true")
    }



    pub fn with_rank_is_value_7(self) -> Self {
        self.with_rank_is("-7")
    }



    pub fn with_rank_is_not_value_7(self) -> Self {
        self.with_rank_is_not("-7")
    }


    pub fn with_rank_is_value_9(self) -> Self {
        self.with_rank_is("9")
    }



    pub fn with_rank_is_not_value_9(self) -> Self {
        self.with_rank_is_not("9")
    }



    pub fn with_weight_is_value_2_5(self) -> Self {
        self.with_weight_is("2.5")
    }



    pub fn with_weight_is_not_value_2_5(self) -> Self {
        self.with_weight_is_not("2.5")
    }


    pub fn with_weight_is_value_4_75(self) -> Self {
        self.with_weight_is("-4.75")
    }



    pub fn with_weight_is_not_value_4_75(self) -> Self {
        self.with_weight_is_not("-4.75")
    }



    pub fn with_ratio_is_value_1_25f(self) -> Self {
        self.with_ratio_is("1.25f")
    }



    pub fn with_ratio_is_not_value_1_25f(self) -> Self {
        self.with_ratio_is_not("1.25f")
    }


    pub fn with_ratio_is_value_0_5f(self) -> Self {
        self.with_ratio_is("-0.5f")
    }



    pub fn with_ratio_is_not_value_0_5f(self) -> Self {
        self.with_ratio_is_not("-0.5f")
    }



    pub fn with_cost_is_value_12_50(self) -> Self {
        self.with_cost_is("12.50")
    }



    pub fn with_cost_is_not_value_12_50(self) -> Self {
        self.with_cost_is_not("12.50")
    }


    pub fn with_cost_is_value_3_125(self) -> Self {
        self.with_cost_is("-3.125")
    }



    pub fn with_cost_is_not_value_3_125(self) -> Self {
        self.with_cost_is_not("-3.125")
    }



    pub fn with_day_is_value_2024_02_01(self) -> Self {
        self.with_day_is("2024-02-01")
    }



    pub fn with_day_is_not_value_2024_02_01(self) -> Self {
        self.with_day_is_not("2024-02-01")
    }


    pub fn with_day_is_value_2020_04_09(self) -> Self {
        self.with_day_is("2020-04-09")
    }



    pub fn with_day_is_not_value_2020_04_09(self) -> Self {
        self.with_day_is_not("2020-04-09")
    }



    pub fn with_payload_is_x_1(self) -> Self {
        self.with_payload_is("{"x":1}")
    }



    pub fn with_payload_is_not_x_1(self) -> Self {
        self.with_payload_is_not("{"x":1}")
    }


    pub fn with_payload_is_x_2(self) -> Self {
        self.with_payload_is("{"x":2}")
    }



    pub fn with_payload_is_not_x_2(self) -> Self {
        self.with_payload_is_not("{"x":2}")
    }



    pub fn with_note_is_first_note(self) -> Self {
        self.with_note_is("First note")
    }



    pub fn with_note_is_not_first_note(self) -> Self {
        self.with_note_is_not("First note")
    }


    pub fn with_note_is_second_note(self) -> Self {
        self.with_note_is("Second note")
    }



    pub fn with_note_is_not_second_note(self) -> Self {
        self.with_note_is_not("Second note")
    }



    pub fn with_optional_rank_is_value_0(self) -> Self {
        self.with_optional_rank_is("0")
    }



    pub fn with_optional_rank_is_not_value_0(self) -> Self {
        self.with_optional_rank_is_not("0")
    }


    pub fn with_optional_rank_is_value_5(self) -> Self {
        self.with_optional_rank_is("5")
    }



    pub fn with_optional_rank_is_not_value_5(self) -> Self {
        self.with_optional_rank_is_not("5")
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
    pub fn have_labels(self) -> Self {
        self.with_label_list_matching(crate::Q::labels_minimal())
    }

    pub fn have_no_labels(self) -> Self {
        self.without_label_list_matching(crate::Q::labels_minimal())
    }

    pub fn with_label_list_matching(mut self, request: impl Into<QuerySelection>) -> Self {
        let selection = request.into();
        self.query = self.query.and_filter(Expr::in_subquery(
            "id",
            <crate::Label as teaql_core::TeaqlEntity>::entity_descriptor(),
            selection.query.clone(),
            "priority_id",
        ));
        self.relation_filters.push(RelationFilter::new("label_list", selection));
        self
    }

    pub fn without_label_list_matching(mut self, request: impl Into<QuerySelection>) -> Self {
        let selection = request.into();
        self.query = self.query.and_filter(Expr::not_in_subquery(
            "id",
            <crate::Label as teaql_core::TeaqlEntity>::entity_descriptor(),
            selection.query.clone(),
            "priority_id",
        ));
        self.relation_filters.push(RelationFilter::new("label_list", selection));
        self
    }

    pub fn select_label_list(mut self) -> Self {
        self.query = self.query.relation("label_list");
        self
    }

    pub fn select_label_list_with(mut self, request: impl Into<QuerySelection>) -> Self {
        let selection = request.into();
        self.query = self.query.relation_query("label_list", selection.into_query());
        self
}
    pub fn count_labels(self) -> Self {
        self.count_labels_as("count_labels")
    }

    pub fn count_labels_as(self, alias: impl Into<String>) -> Self {
        self.count_labels_with(alias, crate::Q::labels().unlimited())
    }

    pub fn count_labels_with(mut self, alias: impl Into<String>, request: impl Into<QuerySelection>) -> Self {
        let selection = request.into();
        self.query_options.relation_aggregates.push(RelationAggregate::new(
            "label_list",
            alias,
            selection,
            true,
        ));
        self
    }

    pub fn stats_from_labels(self, request: impl Into<QuerySelection>) -> Self {
        self.stats_from_labels_as("refinements", request)
    }

    pub fn stats_from_labels_as(mut self, alias: impl Into<String>, request: impl Into<QuerySelection>) -> Self {
        let selection = request.into();
        self.query_options.relation_aggregates.push(RelationAggregate::new(
            "label_list",
            alias,
            selection,
            false,
        ));
        self
    }

    fn scalar_from_labels_as(mut self, alias: impl Into<String>, request: impl Into<QuerySelection>) -> Self {
        let selection = request.into();
        self.query_options.relation_aggregates.push(RelationAggregate::new(
            "label_list",
            alias,
            selection,
            true,
        ));
        self
    }

    pub fn group_by_labels_with_details(self, request: impl Into<QuerySelection>) -> Self {
        self.stats_from_labels(request)
    }



}

impl<R> Default for PriorityRequest<R> {
    fn default() -> Self {
        Self::new()
    }
}

impl<R> From< PriorityRequest<R> > for SelectQuery {
    fn from(request: PriorityRequest<R>) -> Self {
        QuerySelection::from(request).into_query()
    }
}

impl<R> From< PriorityRequest<R> > for QuerySelection {
    fn from(request: PriorityRequest<R>) -> Self {
        Self {
            query: request.query,
            relation_selections: request.relation_selections,
            relation_filters: request.relation_filters,
            child_enhancements: request.child_enhancements,
            query_options: request.query_options,
        }
    }
}


impl<'a, C> crate::request_support::AuditedSave<'a, C> for teaql_core::Audited<crate::Priority> 
where C: crate::TeaqlRuntime + ?Sized + 'a
{
    type Error = teaql_runtime::RuntimeError;
    type Entity = crate::Priority;
    fn save(self, context: &'a C) -> std::pin::Pin<Box<dyn std::future::Future<Output = Result<Self::Entity, Self::Error>> + Send + '_>> {
        Box::pin(async move {
            context.save_audited_entity(self).await
        })
    }
}

impl<R: teaql_core::Entity> crate::PurposedQuery<PriorityRequest<R>> {
    pub fn comment(mut self, comment: impl Into<String>) -> Self {
        self.inner.query_options.comment = Some(comment.into());
        self
    }

    pub fn new_entity<C>(&self, context: &C) -> crate::Priority
    where
        C: crate::TeaqlRuntime + ?Sized,
    {
        self.require_comment();
        let mut entity = crate::Priority::runtime_new(context.user_context().entity_runtime_state());
        if let Ok(id) = context.user_context().next_id(crate::Priority::ENTITY_NAME) {
            entity.update_id(id);
        }
        teaql_core::Entity::mark_as_new(&mut entity);
        entity
    }

    fn into_inner_with_trace(mut self) -> PriorityRequest<R> {
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
