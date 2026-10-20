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
            "instant" => Some("instant"),
            "clock" => Some("clock"),
            "optional_clock" => Some("optional_clock"),
            "created_at" => Some("created_at"),
            "updated_at" => Some("updated_at"),
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
        self.query = self.query.project("instant");
        self.query = self.query.project("clock");
        self.query = self.query.project("optional_clock");
        self.query = self.query.project("created_at");
        self.query = self.query.project("updated_at");
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


    pub fn select_instant(mut self) -> Self {
        self.query = self.query.project("instant");
        self
    }

    pub fn project_instant(self) -> Self {
        self.select_instant()
    }

    pub fn select_instant_raw(self, raw_sql_segment: impl Into<String>) -> Self {
        self.select_instant_unsafe_raw(UnsafeRawSqlSegment::trusted(raw_sql_segment))
    }

    pub fn select_instant_unsafe_raw(mut self, raw_sql_segment: UnsafeRawSqlSegment) -> Self {
        self.query_options
            .raw_projections
            .push(RawProjection::new("instant", raw_sql_segment));
        self
    }

    pub fn group_by_instant(self) -> Self {
        self.group_by("instant")
    }

    pub fn group_by_instant_as(self, alias: impl Into<String>) -> Self {
        let alias = alias.into();
        let mut request = self.group_by("instant");
        request.query = request
            .query
            .project_expr(alias, Expr::column("instant"));
        request
    }

    pub fn group_by_instant_with_function(
        self,
        alias: impl Into<String>,
        function: AggregateFunction,
    ) -> Self {
        self.group_by("instant")
            .aggregate_with_function("instant", alias, function)
    }

    pub fn count_instant(self) -> Self {
        self.count_instant_as("instant_count")
    }

    pub fn count_instant_as(self, alias: impl Into<String>) -> Self {
        self.aggregate_count_field("instant", alias)
    }

    pub fn sum_instant(self) -> Self {
        self.sum_instant_as("sum_instant")
    }

    pub fn sum_instant_as(self, alias: impl Into<String>) -> Self {
        self.aggregate_sum("instant", alias)
    }

    pub fn avg_instant(self) -> Self {
        self.avg_instant_as("avg_instant")
    }

    pub fn avg_instant_as(self, alias: impl Into<String>) -> Self {
        self.aggregate_avg("instant", alias)
    }

    pub fn min_instant(self) -> Self {
        self.min_instant_as("min_instant")
    }

    pub fn min_instant_as(self, alias: impl Into<String>) -> Self {
        self.aggregate_min("instant", alias)
    }

    pub fn max_instant(self) -> Self {
        self.max_instant_as("max_instant")
    }

    pub fn max_instant_as(self, alias: impl Into<String>) -> Self {
        self.aggregate_max("instant", alias)
    }

    pub fn unselect_instant(mut self) -> Self {
        self.query.projection.retain(|field| field != "instant");
        self.query_options.raw_projections.retain(|projection| projection.property_name != "instant");
        self
    }


    pub fn with_instant(
        mut self,
        operator: FieldOperator,
        values: impl IntoIterator<Item = impl Into<teaql_core::Value>>,
    ) -> Self {
        self.query = self.query.and_filter(field_operator_expr(
            "instant",
            operator,
            values.into_iter().map(Into::into).collect(),
        ));
        self
    }

    pub fn create_instant_criteria(
        operator: FieldOperator,
        values: impl IntoIterator<Item = impl Into<teaql_core::Value>>,
    ) -> Expr {
        field_operator_expr(
            "instant",
            operator,
            values.into_iter().map(Into::into).collect(),
        )
    }

    pub fn with_instant_is(mut self, value: impl Into<teaql_core::Value>) -> Self {
        self.query = self.query.and_filter(Expr::eq("instant", value));
        self
    }



    pub fn with_instant_is_not(mut self, value: impl Into<teaql_core::Value>) -> Self {
        self.query = self.query.and_filter(Expr::ne("instant", value));
        self
    }

    pub fn with_instant_greater_than(mut self, value: impl Into<teaql_core::Value>) -> Self {
        self.query = self.query.and_filter(Expr::gt("instant", value));
        self
    }

    pub fn with_instant_greater_than_or_equal_to(mut self, value: impl Into<teaql_core::Value>) -> Self {
        self.query = self.query.and_filter(Expr::gte("instant", value));
        self
    }

    pub fn with_instant_less_than(mut self, value: impl Into<teaql_core::Value>) -> Self {
        self.query = self.query.and_filter(Expr::lt("instant", value));
        self
    }

    pub fn with_instant_less_than_or_equal_to(mut self, value: impl Into<teaql_core::Value>) -> Self {
        self.query = self.query.and_filter(Expr::lte("instant", value));
        self
    }

    pub fn with_instant_between(
        mut self,
        lower: impl Into<teaql_core::Value>,
        upper: impl Into<teaql_core::Value>,
    ) -> Self {
        self.query = self.query.and_filter(Expr::between("instant", lower, upper));
        self
    }

    pub fn with_instant_between_range<T>(mut self, range: DateRange<T>) -> Self
    where
        T: Into<teaql_core::Value>,
    {
        self.query = self.query.and_filter(Expr::between(
            "instant",
            range.start,
            range.end,
        ));
        self
    }

    pub fn with_instant_in(
        mut self,
        values: impl IntoIterator<Item = impl Into<teaql_core::Value>>,
    ) -> Self {
        self.query = self.query.and_filter(Expr::in_list(
            "instant",
            values.into_iter().map(Into::into),
        ));
        self
    }

    pub fn with_instant_not_in(
        mut self,
        values: impl IntoIterator<Item = impl Into<teaql_core::Value>>,
    ) -> Self {
        self.query = self.query.and_filter(Expr::not_in_list(
            "instant",
            values.into_iter().map(Into::into),
        ));
        self
    }

    pub fn with_instant_before(mut self, value: impl Into<teaql_core::Value>) -> Self {
        self.query = self.query.and_filter(Expr::lt("instant", value));
        self
    }

    pub fn with_instant_after(mut self, value: impl Into<teaql_core::Value>) -> Self {
        self.query = self.query.and_filter(Expr::gt("instant", value));
        self
    }

    pub fn with_instant_is_unknown(mut self) -> Self {
        self.query = self.query.and_filter(Expr::is_null("instant"));
        self
    }



    pub fn with_instant_is_known(mut self) -> Self {
        self.query = self.query.and_filter(Expr::is_not_null("instant"));
        self
    }


    pub fn order_by_instant_asc(mut self) -> Self {
        self.query = self.query.order_asc("instant");
        self
    }

    pub fn order_by_instant_desc(mut self) -> Self {
        self.query = self.query.order_desc("instant");
        self
    }

    pub fn order_by_instant_asc_using_gbk(mut self) -> Self {
        self.query = self.query.order_gbk_asc("instant");
        self
    }

    pub fn order_by_instant_desc_using_gbk(mut self) -> Self {
        self.query = self.query.order_gbk_desc("instant");
        self
    }


    pub fn select_clock(mut self) -> Self {
        self.query = self.query.project("clock");
        self
    }

    pub fn project_clock(self) -> Self {
        self.select_clock()
    }

    pub fn select_clock_raw(self, raw_sql_segment: impl Into<String>) -> Self {
        self.select_clock_unsafe_raw(UnsafeRawSqlSegment::trusted(raw_sql_segment))
    }

    pub fn select_clock_unsafe_raw(mut self, raw_sql_segment: UnsafeRawSqlSegment) -> Self {
        self.query_options
            .raw_projections
            .push(RawProjection::new("clock", raw_sql_segment));
        self
    }

    pub fn group_by_clock(self) -> Self {
        self.group_by("clock")
    }

    pub fn group_by_clock_as(self, alias: impl Into<String>) -> Self {
        let alias = alias.into();
        let mut request = self.group_by("clock");
        request.query = request
            .query
            .project_expr(alias, Expr::column("clock"));
        request
    }

    pub fn group_by_clock_with_function(
        self,
        alias: impl Into<String>,
        function: AggregateFunction,
    ) -> Self {
        self.group_by("clock")
            .aggregate_with_function("clock", alias, function)
    }

    pub fn count_clock(self) -> Self {
        self.count_clock_as("clock_count")
    }

    pub fn count_clock_as(self, alias: impl Into<String>) -> Self {
        self.aggregate_count_field("clock", alias)
    }

    pub fn sum_clock(self) -> Self {
        self.sum_clock_as("sum_clock")
    }

    pub fn sum_clock_as(self, alias: impl Into<String>) -> Self {
        self.aggregate_sum("clock", alias)
    }

    pub fn avg_clock(self) -> Self {
        self.avg_clock_as("avg_clock")
    }

    pub fn avg_clock_as(self, alias: impl Into<String>) -> Self {
        self.aggregate_avg("clock", alias)
    }

    pub fn min_clock(self) -> Self {
        self.min_clock_as("min_clock")
    }

    pub fn min_clock_as(self, alias: impl Into<String>) -> Self {
        self.aggregate_min("clock", alias)
    }

    pub fn max_clock(self) -> Self {
        self.max_clock_as("max_clock")
    }

    pub fn max_clock_as(self, alias: impl Into<String>) -> Self {
        self.aggregate_max("clock", alias)
    }

    pub fn unselect_clock(mut self) -> Self {
        self.query.projection.retain(|field| field != "clock");
        self.query_options.raw_projections.retain(|projection| projection.property_name != "clock");
        self
    }


    pub fn with_clock(
        mut self,
        operator: FieldOperator,
        values: impl IntoIterator<Item = impl Into<teaql_core::Value>>,
    ) -> Self {
        self.query = self.query.and_filter(field_operator_expr(
            "clock",
            operator,
            values.into_iter().map(Into::into).collect(),
        ));
        self
    }

    pub fn create_clock_criteria(
        operator: FieldOperator,
        values: impl IntoIterator<Item = impl Into<teaql_core::Value>>,
    ) -> Expr {
        field_operator_expr(
            "clock",
            operator,
            values.into_iter().map(Into::into).collect(),
        )
    }

    pub fn with_clock_is(mut self, value: impl Into<teaql_core::Value>) -> Self {
        self.query = self.query.and_filter(Expr::eq("clock", value));
        self
    }



    pub fn with_clock_is_not(mut self, value: impl Into<teaql_core::Value>) -> Self {
        self.query = self.query.and_filter(Expr::ne("clock", value));
        self
    }

    pub fn with_clock_greater_than(mut self, value: impl Into<teaql_core::Value>) -> Self {
        self.query = self.query.and_filter(Expr::gt("clock", value));
        self
    }

    pub fn with_clock_greater_than_or_equal_to(mut self, value: impl Into<teaql_core::Value>) -> Self {
        self.query = self.query.and_filter(Expr::gte("clock", value));
        self
    }

    pub fn with_clock_less_than(mut self, value: impl Into<teaql_core::Value>) -> Self {
        self.query = self.query.and_filter(Expr::lt("clock", value));
        self
    }

    pub fn with_clock_less_than_or_equal_to(mut self, value: impl Into<teaql_core::Value>) -> Self {
        self.query = self.query.and_filter(Expr::lte("clock", value));
        self
    }

    pub fn with_clock_between(
        mut self,
        lower: impl Into<teaql_core::Value>,
        upper: impl Into<teaql_core::Value>,
    ) -> Self {
        self.query = self.query.and_filter(Expr::between("clock", lower, upper));
        self
    }

    pub fn with_clock_between_range<T>(mut self, range: DateRange<T>) -> Self
    where
        T: Into<teaql_core::Value>,
    {
        self.query = self.query.and_filter(Expr::between(
            "clock",
            range.start,
            range.end,
        ));
        self
    }

    pub fn with_clock_in(
        mut self,
        values: impl IntoIterator<Item = impl Into<teaql_core::Value>>,
    ) -> Self {
        self.query = self.query.and_filter(Expr::in_list(
            "clock",
            values.into_iter().map(Into::into),
        ));
        self
    }

    pub fn with_clock_not_in(
        mut self,
        values: impl IntoIterator<Item = impl Into<teaql_core::Value>>,
    ) -> Self {
        self.query = self.query.and_filter(Expr::not_in_list(
            "clock",
            values.into_iter().map(Into::into),
        ));
        self
    }

    pub fn with_clock_before(mut self, value: impl Into<teaql_core::Value>) -> Self {
        self.query = self.query.and_filter(Expr::lt("clock", value));
        self
    }

    pub fn with_clock_after(mut self, value: impl Into<teaql_core::Value>) -> Self {
        self.query = self.query.and_filter(Expr::gt("clock", value));
        self
    }

    pub fn with_clock_is_unknown(mut self) -> Self {
        self.query = self.query.and_filter(Expr::is_null("clock"));
        self
    }



    pub fn with_clock_is_known(mut self) -> Self {
        self.query = self.query.and_filter(Expr::is_not_null("clock"));
        self
    }


    pub fn order_by_clock_asc(mut self) -> Self {
        self.query = self.query.order_asc("clock");
        self
    }

    pub fn order_by_clock_desc(mut self) -> Self {
        self.query = self.query.order_desc("clock");
        self
    }

    pub fn order_by_clock_asc_using_gbk(mut self) -> Self {
        self.query = self.query.order_gbk_asc("clock");
        self
    }

    pub fn order_by_clock_desc_using_gbk(mut self) -> Self {
        self.query = self.query.order_gbk_desc("clock");
        self
    }


    pub fn select_optional_clock(mut self) -> Self {
        self.query = self.query.project("optional_clock");
        self
    }

    pub fn project_optional_clock(self) -> Self {
        self.select_optional_clock()
    }

    pub fn select_optional_clock_raw(self, raw_sql_segment: impl Into<String>) -> Self {
        self.select_optional_clock_unsafe_raw(UnsafeRawSqlSegment::trusted(raw_sql_segment))
    }

    pub fn select_optional_clock_unsafe_raw(mut self, raw_sql_segment: UnsafeRawSqlSegment) -> Self {
        self.query_options
            .raw_projections
            .push(RawProjection::new("optional_clock", raw_sql_segment));
        self
    }

    pub fn group_by_optional_clock(self) -> Self {
        self.group_by("optional_clock")
    }

    pub fn group_by_optional_clock_as(self, alias: impl Into<String>) -> Self {
        let alias = alias.into();
        let mut request = self.group_by("optional_clock");
        request.query = request
            .query
            .project_expr(alias, Expr::column("optional_clock"));
        request
    }

    pub fn group_by_optional_clock_with_function(
        self,
        alias: impl Into<String>,
        function: AggregateFunction,
    ) -> Self {
        self.group_by("optional_clock")
            .aggregate_with_function("optional_clock", alias, function)
    }

    pub fn count_optional_clock(self) -> Self {
        self.count_optional_clock_as("optional_clock_count")
    }

    pub fn count_optional_clock_as(self, alias: impl Into<String>) -> Self {
        self.aggregate_count_field("optional_clock", alias)
    }

    pub fn sum_optional_clock(self) -> Self {
        self.sum_optional_clock_as("sum_optional_clock")
    }

    pub fn sum_optional_clock_as(self, alias: impl Into<String>) -> Self {
        self.aggregate_sum("optional_clock", alias)
    }

    pub fn avg_optional_clock(self) -> Self {
        self.avg_optional_clock_as("avg_optional_clock")
    }

    pub fn avg_optional_clock_as(self, alias: impl Into<String>) -> Self {
        self.aggregate_avg("optional_clock", alias)
    }

    pub fn min_optional_clock(self) -> Self {
        self.min_optional_clock_as("min_optional_clock")
    }

    pub fn min_optional_clock_as(self, alias: impl Into<String>) -> Self {
        self.aggregate_min("optional_clock", alias)
    }

    pub fn max_optional_clock(self) -> Self {
        self.max_optional_clock_as("max_optional_clock")
    }

    pub fn max_optional_clock_as(self, alias: impl Into<String>) -> Self {
        self.aggregate_max("optional_clock", alias)
    }

    pub fn unselect_optional_clock(mut self) -> Self {
        self.query.projection.retain(|field| field != "optional_clock");
        self.query_options.raw_projections.retain(|projection| projection.property_name != "optional_clock");
        self
    }


    pub fn with_optional_clock(
        mut self,
        operator: FieldOperator,
        values: impl IntoIterator<Item = impl Into<teaql_core::Value>>,
    ) -> Self {
        self.query = self.query.and_filter(field_operator_expr(
            "optional_clock",
            operator,
            values.into_iter().map(Into::into).collect(),
        ));
        self
    }

    pub fn create_optional_clock_criteria(
        operator: FieldOperator,
        values: impl IntoIterator<Item = impl Into<teaql_core::Value>>,
    ) -> Expr {
        field_operator_expr(
            "optional_clock",
            operator,
            values.into_iter().map(Into::into).collect(),
        )
    }

    pub fn with_optional_clock_is(mut self, value: impl Into<teaql_core::Value>) -> Self {
        self.query = self.query.and_filter(Expr::eq("optional_clock", value));
        self
    }



    pub fn with_optional_clock_is_not(mut self, value: impl Into<teaql_core::Value>) -> Self {
        self.query = self.query.and_filter(Expr::ne("optional_clock", value));
        self
    }

    pub fn with_optional_clock_greater_than(mut self, value: impl Into<teaql_core::Value>) -> Self {
        self.query = self.query.and_filter(Expr::gt("optional_clock", value));
        self
    }

    pub fn with_optional_clock_greater_than_or_equal_to(mut self, value: impl Into<teaql_core::Value>) -> Self {
        self.query = self.query.and_filter(Expr::gte("optional_clock", value));
        self
    }

    pub fn with_optional_clock_less_than(mut self, value: impl Into<teaql_core::Value>) -> Self {
        self.query = self.query.and_filter(Expr::lt("optional_clock", value));
        self
    }

    pub fn with_optional_clock_less_than_or_equal_to(mut self, value: impl Into<teaql_core::Value>) -> Self {
        self.query = self.query.and_filter(Expr::lte("optional_clock", value));
        self
    }

    pub fn with_optional_clock_between(
        mut self,
        lower: impl Into<teaql_core::Value>,
        upper: impl Into<teaql_core::Value>,
    ) -> Self {
        self.query = self.query.and_filter(Expr::between("optional_clock", lower, upper));
        self
    }

    pub fn with_optional_clock_between_range<T>(mut self, range: DateRange<T>) -> Self
    where
        T: Into<teaql_core::Value>,
    {
        self.query = self.query.and_filter(Expr::between(
            "optional_clock",
            range.start,
            range.end,
        ));
        self
    }

    pub fn with_optional_clock_in(
        mut self,
        values: impl IntoIterator<Item = impl Into<teaql_core::Value>>,
    ) -> Self {
        self.query = self.query.and_filter(Expr::in_list(
            "optional_clock",
            values.into_iter().map(Into::into),
        ));
        self
    }

    pub fn with_optional_clock_not_in(
        mut self,
        values: impl IntoIterator<Item = impl Into<teaql_core::Value>>,
    ) -> Self {
        self.query = self.query.and_filter(Expr::not_in_list(
            "optional_clock",
            values.into_iter().map(Into::into),
        ));
        self
    }

    pub fn with_optional_clock_before(mut self, value: impl Into<teaql_core::Value>) -> Self {
        self.query = self.query.and_filter(Expr::lt("optional_clock", value));
        self
    }

    pub fn with_optional_clock_after(mut self, value: impl Into<teaql_core::Value>) -> Self {
        self.query = self.query.and_filter(Expr::gt("optional_clock", value));
        self
    }

    pub fn with_optional_clock_is_unknown(mut self) -> Self {
        self.query = self.query.and_filter(Expr::is_null("optional_clock"));
        self
    }



    pub fn with_optional_clock_is_known(mut self) -> Self {
        self.query = self.query.and_filter(Expr::is_not_null("optional_clock"));
        self
    }


    pub fn order_by_optional_clock_asc(mut self) -> Self {
        self.query = self.query.order_asc("optional_clock");
        self
    }

    pub fn order_by_optional_clock_desc(mut self) -> Self {
        self.query = self.query.order_desc("optional_clock");
        self
    }

    pub fn order_by_optional_clock_asc_using_gbk(mut self) -> Self {
        self.query = self.query.order_gbk_asc("optional_clock");
        self
    }

    pub fn order_by_optional_clock_desc_using_gbk(mut self) -> Self {
        self.query = self.query.order_gbk_desc("optional_clock");
        self
    }


    pub fn select_created_at(mut self) -> Self {
        self.query = self.query.project("created_at");
        self
    }

    pub fn project_created_at(self) -> Self {
        self.select_created_at()
    }

    pub fn select_created_at_raw(self, raw_sql_segment: impl Into<String>) -> Self {
        self.select_created_at_unsafe_raw(UnsafeRawSqlSegment::trusted(raw_sql_segment))
    }

    pub fn select_created_at_unsafe_raw(mut self, raw_sql_segment: UnsafeRawSqlSegment) -> Self {
        self.query_options
            .raw_projections
            .push(RawProjection::new("created_at", raw_sql_segment));
        self
    }

    pub fn group_by_created_at(self) -> Self {
        self.group_by("created_at")
    }

    pub fn group_by_created_at_as(self, alias: impl Into<String>) -> Self {
        let alias = alias.into();
        let mut request = self.group_by("created_at");
        request.query = request
            .query
            .project_expr(alias, Expr::column("created_at"));
        request
    }

    pub fn group_by_created_at_with_function(
        self,
        alias: impl Into<String>,
        function: AggregateFunction,
    ) -> Self {
        self.group_by("created_at")
            .aggregate_with_function("created_at", alias, function)
    }

    pub fn count_created_at(self) -> Self {
        self.count_created_at_as("created_at_count")
    }

    pub fn count_created_at_as(self, alias: impl Into<String>) -> Self {
        self.aggregate_count_field("created_at", alias)
    }

    pub fn sum_created_at(self) -> Self {
        self.sum_created_at_as("sum_created_at")
    }

    pub fn sum_created_at_as(self, alias: impl Into<String>) -> Self {
        self.aggregate_sum("created_at", alias)
    }

    pub fn avg_created_at(self) -> Self {
        self.avg_created_at_as("avg_created_at")
    }

    pub fn avg_created_at_as(self, alias: impl Into<String>) -> Self {
        self.aggregate_avg("created_at", alias)
    }

    pub fn min_created_at(self) -> Self {
        self.min_created_at_as("min_created_at")
    }

    pub fn min_created_at_as(self, alias: impl Into<String>) -> Self {
        self.aggregate_min("created_at", alias)
    }

    pub fn max_created_at(self) -> Self {
        self.max_created_at_as("max_created_at")
    }

    pub fn max_created_at_as(self, alias: impl Into<String>) -> Self {
        self.aggregate_max("created_at", alias)
    }

    pub fn unselect_created_at(mut self) -> Self {
        self.query.projection.retain(|field| field != "created_at");
        self.query_options.raw_projections.retain(|projection| projection.property_name != "created_at");
        self
    }


    pub fn with_created_at(
        mut self,
        operator: FieldOperator,
        values: impl IntoIterator<Item = impl Into<teaql_core::Value>>,
    ) -> Self {
        self.query = self.query.and_filter(field_operator_expr(
            "created_at",
            operator,
            values.into_iter().map(Into::into).collect(),
        ));
        self
    }

    pub fn create_created_at_criteria(
        operator: FieldOperator,
        values: impl IntoIterator<Item = impl Into<teaql_core::Value>>,
    ) -> Expr {
        field_operator_expr(
            "created_at",
            operator,
            values.into_iter().map(Into::into).collect(),
        )
    }

    pub fn with_created_at_is(mut self, value: impl Into<teaql_core::Value>) -> Self {
        self.query = self.query.and_filter(Expr::eq("created_at", value));
        self
    }



    pub fn with_created_at_is_not(mut self, value: impl Into<teaql_core::Value>) -> Self {
        self.query = self.query.and_filter(Expr::ne("created_at", value));
        self
    }

    pub fn with_created_at_greater_than(mut self, value: impl Into<teaql_core::Value>) -> Self {
        self.query = self.query.and_filter(Expr::gt("created_at", value));
        self
    }

    pub fn with_created_at_greater_than_or_equal_to(mut self, value: impl Into<teaql_core::Value>) -> Self {
        self.query = self.query.and_filter(Expr::gte("created_at", value));
        self
    }

    pub fn with_created_at_less_than(mut self, value: impl Into<teaql_core::Value>) -> Self {
        self.query = self.query.and_filter(Expr::lt("created_at", value));
        self
    }

    pub fn with_created_at_less_than_or_equal_to(mut self, value: impl Into<teaql_core::Value>) -> Self {
        self.query = self.query.and_filter(Expr::lte("created_at", value));
        self
    }

    pub fn with_created_at_between(
        mut self,
        lower: impl Into<teaql_core::Value>,
        upper: impl Into<teaql_core::Value>,
    ) -> Self {
        self.query = self.query.and_filter(Expr::between("created_at", lower, upper));
        self
    }

    pub fn with_created_at_between_range<T>(mut self, range: DateRange<T>) -> Self
    where
        T: Into<teaql_core::Value>,
    {
        self.query = self.query.and_filter(Expr::between(
            "created_at",
            range.start,
            range.end,
        ));
        self
    }

    pub fn with_created_at_in(
        mut self,
        values: impl IntoIterator<Item = impl Into<teaql_core::Value>>,
    ) -> Self {
        self.query = self.query.and_filter(Expr::in_list(
            "created_at",
            values.into_iter().map(Into::into),
        ));
        self
    }

    pub fn with_created_at_not_in(
        mut self,
        values: impl IntoIterator<Item = impl Into<teaql_core::Value>>,
    ) -> Self {
        self.query = self.query.and_filter(Expr::not_in_list(
            "created_at",
            values.into_iter().map(Into::into),
        ));
        self
    }

    pub fn with_created_at_before(mut self, value: impl Into<teaql_core::Value>) -> Self {
        self.query = self.query.and_filter(Expr::lt("created_at", value));
        self
    }

    pub fn with_created_at_after(mut self, value: impl Into<teaql_core::Value>) -> Self {
        self.query = self.query.and_filter(Expr::gt("created_at", value));
        self
    }

    pub fn with_created_at_is_unknown(mut self) -> Self {
        self.query = self.query.and_filter(Expr::is_null("created_at"));
        self
    }



    pub fn with_created_at_is_known(mut self) -> Self {
        self.query = self.query.and_filter(Expr::is_not_null("created_at"));
        self
    }


    pub fn order_by_created_at_asc(mut self) -> Self {
        self.query = self.query.order_asc("created_at");
        self
    }

    pub fn order_by_created_at_desc(mut self) -> Self {
        self.query = self.query.order_desc("created_at");
        self
    }

    pub fn order_by_created_at_asc_using_gbk(mut self) -> Self {
        self.query = self.query.order_gbk_asc("created_at");
        self
    }

    pub fn order_by_created_at_desc_using_gbk(mut self) -> Self {
        self.query = self.query.order_gbk_desc("created_at");
        self
    }


    pub fn select_updated_at(mut self) -> Self {
        self.query = self.query.project("updated_at");
        self
    }

    pub fn project_updated_at(self) -> Self {
        self.select_updated_at()
    }

    pub fn select_updated_at_raw(self, raw_sql_segment: impl Into<String>) -> Self {
        self.select_updated_at_unsafe_raw(UnsafeRawSqlSegment::trusted(raw_sql_segment))
    }

    pub fn select_updated_at_unsafe_raw(mut self, raw_sql_segment: UnsafeRawSqlSegment) -> Self {
        self.query_options
            .raw_projections
            .push(RawProjection::new("updated_at", raw_sql_segment));
        self
    }

    pub fn group_by_updated_at(self) -> Self {
        self.group_by("updated_at")
    }

    pub fn group_by_updated_at_as(self, alias: impl Into<String>) -> Self {
        let alias = alias.into();
        let mut request = self.group_by("updated_at");
        request.query = request
            .query
            .project_expr(alias, Expr::column("updated_at"));
        request
    }

    pub fn group_by_updated_at_with_function(
        self,
        alias: impl Into<String>,
        function: AggregateFunction,
    ) -> Self {
        self.group_by("updated_at")
            .aggregate_with_function("updated_at", alias, function)
    }

    pub fn count_updated_at(self) -> Self {
        self.count_updated_at_as("updated_at_count")
    }

    pub fn count_updated_at_as(self, alias: impl Into<String>) -> Self {
        self.aggregate_count_field("updated_at", alias)
    }

    pub fn sum_updated_at(self) -> Self {
        self.sum_updated_at_as("sum_updated_at")
    }

    pub fn sum_updated_at_as(self, alias: impl Into<String>) -> Self {
        self.aggregate_sum("updated_at", alias)
    }

    pub fn avg_updated_at(self) -> Self {
        self.avg_updated_at_as("avg_updated_at")
    }

    pub fn avg_updated_at_as(self, alias: impl Into<String>) -> Self {
        self.aggregate_avg("updated_at", alias)
    }

    pub fn min_updated_at(self) -> Self {
        self.min_updated_at_as("min_updated_at")
    }

    pub fn min_updated_at_as(self, alias: impl Into<String>) -> Self {
        self.aggregate_min("updated_at", alias)
    }

    pub fn max_updated_at(self) -> Self {
        self.max_updated_at_as("max_updated_at")
    }

    pub fn max_updated_at_as(self, alias: impl Into<String>) -> Self {
        self.aggregate_max("updated_at", alias)
    }

    pub fn unselect_updated_at(mut self) -> Self {
        self.query.projection.retain(|field| field != "updated_at");
        self.query_options.raw_projections.retain(|projection| projection.property_name != "updated_at");
        self
    }


    pub fn with_updated_at(
        mut self,
        operator: FieldOperator,
        values: impl IntoIterator<Item = impl Into<teaql_core::Value>>,
    ) -> Self {
        self.query = self.query.and_filter(field_operator_expr(
            "updated_at",
            operator,
            values.into_iter().map(Into::into).collect(),
        ));
        self
    }

    pub fn create_updated_at_criteria(
        operator: FieldOperator,
        values: impl IntoIterator<Item = impl Into<teaql_core::Value>>,
    ) -> Expr {
        field_operator_expr(
            "updated_at",
            operator,
            values.into_iter().map(Into::into).collect(),
        )
    }

    pub fn with_updated_at_is(mut self, value: impl Into<teaql_core::Value>) -> Self {
        self.query = self.query.and_filter(Expr::eq("updated_at", value));
        self
    }



    pub fn with_updated_at_is_not(mut self, value: impl Into<teaql_core::Value>) -> Self {
        self.query = self.query.and_filter(Expr::ne("updated_at", value));
        self
    }

    pub fn with_updated_at_greater_than(mut self, value: impl Into<teaql_core::Value>) -> Self {
        self.query = self.query.and_filter(Expr::gt("updated_at", value));
        self
    }

    pub fn with_updated_at_greater_than_or_equal_to(mut self, value: impl Into<teaql_core::Value>) -> Self {
        self.query = self.query.and_filter(Expr::gte("updated_at", value));
        self
    }

    pub fn with_updated_at_less_than(mut self, value: impl Into<teaql_core::Value>) -> Self {
        self.query = self.query.and_filter(Expr::lt("updated_at", value));
        self
    }

    pub fn with_updated_at_less_than_or_equal_to(mut self, value: impl Into<teaql_core::Value>) -> Self {
        self.query = self.query.and_filter(Expr::lte("updated_at", value));
        self
    }

    pub fn with_updated_at_between(
        mut self,
        lower: impl Into<teaql_core::Value>,
        upper: impl Into<teaql_core::Value>,
    ) -> Self {
        self.query = self.query.and_filter(Expr::between("updated_at", lower, upper));
        self
    }

    pub fn with_updated_at_between_range<T>(mut self, range: DateRange<T>) -> Self
    where
        T: Into<teaql_core::Value>,
    {
        self.query = self.query.and_filter(Expr::between(
            "updated_at",
            range.start,
            range.end,
        ));
        self
    }

    pub fn with_updated_at_in(
        mut self,
        values: impl IntoIterator<Item = impl Into<teaql_core::Value>>,
    ) -> Self {
        self.query = self.query.and_filter(Expr::in_list(
            "updated_at",
            values.into_iter().map(Into::into),
        ));
        self
    }

    pub fn with_updated_at_not_in(
        mut self,
        values: impl IntoIterator<Item = impl Into<teaql_core::Value>>,
    ) -> Self {
        self.query = self.query.and_filter(Expr::not_in_list(
            "updated_at",
            values.into_iter().map(Into::into),
        ));
        self
    }

    pub fn with_updated_at_before(mut self, value: impl Into<teaql_core::Value>) -> Self {
        self.query = self.query.and_filter(Expr::lt("updated_at", value));
        self
    }

    pub fn with_updated_at_after(mut self, value: impl Into<teaql_core::Value>) -> Self {
        self.query = self.query.and_filter(Expr::gt("updated_at", value));
        self
    }

    pub fn with_updated_at_is_unknown(mut self) -> Self {
        self.query = self.query.and_filter(Expr::is_null("updated_at"));
        self
    }



    pub fn with_updated_at_is_known(mut self) -> Self {
        self.query = self.query.and_filter(Expr::is_not_null("updated_at"));
        self
    }


    pub fn order_by_updated_at_asc(mut self) -> Self {
        self.query = self.query.order_asc("updated_at");
        self
    }

    pub fn order_by_updated_at_desc(mut self) -> Self {
        self.query = self.query.order_desc("updated_at");
        self
    }

    pub fn order_by_updated_at_asc_using_gbk(mut self) -> Self {
        self.query = self.query.order_gbk_asc("updated_at");
        self
    }

    pub fn order_by_updated_at_desc_using_gbk(mut self) -> Self {
        self.query = self.query.order_gbk_desc("updated_at");
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



    pub fn with_instant_is_value_2001_01_01t01_02_03(self) -> Self {
        self.with_instant_is("2001-01-01T01:02:03")
    }



    pub fn with_instant_is_not_value_2001_01_01t01_02_03(self) -> Self {
        self.with_instant_is_not("2001-01-01T01:02:03")
    }


    pub fn with_instant_is_value_2003_03_03t03_04_05(self) -> Self {
        self.with_instant_is("2003-03-03T03:04:05")
    }



    pub fn with_instant_is_not_value_2003_03_03t03_04_05(self) -> Self {
        self.with_instant_is_not("2003-03-03T03:04:05")
    }



    pub fn with_clock_is_value_07_30_00(self) -> Self {
        self.with_clock_is("07:30:00")
    }



    pub fn with_clock_is_not_value_07_30_00(self) -> Self {
        self.with_clock_is_not("07:30:00")
    }


    pub fn with_clock_is_value_23_59_59(self) -> Self {
        self.with_clock_is("23:59:59")
    }



    pub fn with_clock_is_not_value_23_59_59(self) -> Self {
        self.with_clock_is_not("23:59:59")
    }



    pub fn with_optional_clock_is_value_08_45_00(self) -> Self {
        self.with_optional_clock_is("08:45:00")
    }



    pub fn with_optional_clock_is_not_value_08_45_00(self) -> Self {
        self.with_optional_clock_is_not("08:45:00")
    }


    pub fn with_optional_clock_is_value_00_00_00(self) -> Self {
        self.with_optional_clock_is("00:00:00")
    }



    pub fn with_optional_clock_is_not_value_00_00_00(self) -> Self {
        self.with_optional_clock_is_not("00:00:00")
    }



    pub fn with_created_at_is_create_time(self) -> Self {
        self.with_created_at_is("createTime()")
    }



    pub fn with_created_at_is_not_create_time(self) -> Self {
        self.with_created_at_is_not("createTime()")
    }



    pub fn with_updated_at_is_update_time(self) -> Self {
        self.with_updated_at_is("updateTime()")
    }



    pub fn with_updated_at_is_not_update_time(self) -> Self {
        self.with_updated_at_is_not("updateTime()")
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
