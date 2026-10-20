use std::marker::PhantomData;

use serde_json::Value as JsonValue;
use teaql_core::{Aggregate, AggregateFunction, EntityDescriptor, Expr, SelectQuery, SmartList};
use teaql_runtime::RuntimeError;

use crate::request_support::*;

impl EntityReference for crate::Note {
    fn entity_id_value(self) -> teaql_core::Value {
        teaql_core::IdentifiableEntity::id_value(&self)
    }
}

impl EntityReference for &crate::Note {
    fn entity_id_value(self) -> teaql_core::Value {
        teaql_core::IdentifiableEntity::id_value(self)
    }
}

// ⛔ AI agents: DO NOT read this file for API discovery. Instead run: cargo teaql --input modeling/MODEL.xml rust-assist-query/note
#[derive(Debug)]
pub struct NoteRequest<R = crate::Note> {
    query: SelectQuery,
    relation_selections: Vec<RelationSelection>,
    relation_filters: Vec<RelationFilter>,
    child_enhancements: Vec<QuerySelection>,
    query_options: QueryOptions,
    marker: PhantomData<R>,
}

impl<R> Clone for NoteRequest<R> {
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

impl<R> NoteRequest<R> {
    pub(crate) fn new() -> Self {
        Self {
            query: SelectQuery::new("Note")
                .project("id")
                .project("version"),
            relation_selections: Vec::new(),
            relation_filters: Vec::new(),
            child_enhancements: Vec::new(),
            query_options: QueryOptions::default(),
            marker: PhantomData,
        }
    }

    pub fn return_type<T>(self) -> NoteRequest<T> {
        NoteRequest {
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
                "Note",
                query,
                relation_aggregates,
            ).await?;
            (rows, std::collections::BTreeMap::new())
        } else {
            let rows = context.fetch_entity_smart_list::<R>(
                "Note",
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
        context.fetch_compact_smart_list("Note", &query).await
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
        Ok(context.fetch_entity_stream("Note", query))
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
        let rows = context.fetch_compact_rows("Note", &query).await?;
        rows.first()
            .and_then(|row| row.get(COUNT_ALIAS))
            .and_then(teaql_core::Value::try_u64)
            .ok_or_else(|| RuntimeError::Graph(format!("count result for Note is missing or not numeric")))
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
        let rows = context.fetch_compact_rows("Note", &query).await?;
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
            "optional_text" => Some("optional_text"),
            "code" => Some("code"),
            "optional_code" => Some("optional_code"),
            "integer" => Some("integer"),
            "optional_integer" => Some("optional_integer"),
            "long" => Some("long"),
            "optional_long" => Some("optional_long"),
            "decimal" => Some("decimal"),
            "optional_decimal" => Some("optional_decimal"),
            "float" => Some("float"),
            "optional_float" => Some("optional_float"),
            "double" => Some("double"),
            "optional_double" => Some("optional_double"),
            "active" => Some("active"),
            "optional_active" => Some("optional_active"),
            "date" => Some("date"),
            "optional_date" => Some("optional_date"),
            "time" => Some("time"),
            "optional_time" => Some("optional_time"),
            "payload" => Some("payload"),
            "optional_payload" => Some("optional_payload"),
            "timestamp" => Some("timestamp"),
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
        self.query = self.query.project("optional_text");
        self.query = self.query.project("code");
        self.query = self.query.project("optional_code");
        self.query = self.query.project("integer");
        self.query = self.query.project("optional_integer");
        self.query = self.query.project("long");
        self.query = self.query.project("optional_long");
        self.query = self.query.project("decimal");
        self.query = self.query.project("optional_decimal");
        self.query = self.query.project("float");
        self.query = self.query.project("optional_float");
        self.query = self.query.project("double");
        self.query = self.query.project("optional_double");
        self.query = self.query.project("active");
        self.query = self.query.project("optional_active");
        self.query = self.query.project("date");
        self.query = self.query.project("optional_date");
        self.query = self.query.project("time");
        self.query = self.query.project("optional_time");
        self.query = self.query.project("payload");
        self.query = self.query.project("optional_payload");
        self.query = self.query.project("timestamp");
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


    pub fn select_optional_text(mut self) -> Self {
        self.query = self.query.project("optional_text");
        self
    }

    pub fn project_optional_text(self) -> Self {
        self.select_optional_text()
    }

    pub fn select_optional_text_raw(self, raw_sql_segment: impl Into<String>) -> Self {
        self.select_optional_text_unsafe_raw(UnsafeRawSqlSegment::trusted(raw_sql_segment))
    }

    pub fn select_optional_text_unsafe_raw(mut self, raw_sql_segment: UnsafeRawSqlSegment) -> Self {
        self.query_options
            .raw_projections
            .push(RawProjection::new("optional_text", raw_sql_segment));
        self
    }

    pub fn group_by_optional_text(self) -> Self {
        self.group_by("optional_text")
    }

    pub fn group_by_optional_text_as(self, alias: impl Into<String>) -> Self {
        let alias = alias.into();
        let mut request = self.group_by("optional_text");
        request.query = request
            .query
            .project_expr(alias, Expr::column("optional_text"));
        request
    }

    pub fn group_by_optional_text_with_function(
        self,
        alias: impl Into<String>,
        function: AggregateFunction,
    ) -> Self {
        self.group_by("optional_text")
            .aggregate_with_function("optional_text", alias, function)
    }

    pub fn count_optional_text(self) -> Self {
        self.count_optional_text_as("optional_text_count")
    }

    pub fn count_optional_text_as(self, alias: impl Into<String>) -> Self {
        self.aggregate_count_field("optional_text", alias)
    }

    pub fn sum_optional_text(self) -> Self {
        self.sum_optional_text_as("sum_optional_text")
    }

    pub fn sum_optional_text_as(self, alias: impl Into<String>) -> Self {
        self.aggregate_sum("optional_text", alias)
    }

    pub fn avg_optional_text(self) -> Self {
        self.avg_optional_text_as("avg_optional_text")
    }

    pub fn avg_optional_text_as(self, alias: impl Into<String>) -> Self {
        self.aggregate_avg("optional_text", alias)
    }

    pub fn min_optional_text(self) -> Self {
        self.min_optional_text_as("min_optional_text")
    }

    pub fn min_optional_text_as(self, alias: impl Into<String>) -> Self {
        self.aggregate_min("optional_text", alias)
    }

    pub fn max_optional_text(self) -> Self {
        self.max_optional_text_as("max_optional_text")
    }

    pub fn max_optional_text_as(self, alias: impl Into<String>) -> Self {
        self.aggregate_max("optional_text", alias)
    }

    pub fn unselect_optional_text(mut self) -> Self {
        self.query.projection.retain(|field| field != "optional_text");
        self.query_options.raw_projections.retain(|projection| projection.property_name != "optional_text");
        self
    }


    pub fn with_optional_text(
        mut self,
        operator: FieldOperator,
        values: impl IntoIterator<Item = impl Into<teaql_core::Value>>,
    ) -> Self {
        self.query = self.query.and_filter(field_operator_expr(
            "optional_text",
            operator,
            values.into_iter().map(Into::into).collect(),
        ));
        self
    }

    pub fn create_optional_text_criteria(
        operator: FieldOperator,
        values: impl IntoIterator<Item = impl Into<teaql_core::Value>>,
    ) -> Expr {
        field_operator_expr(
            "optional_text",
            operator,
            values.into_iter().map(Into::into).collect(),
        )
    }

    pub fn with_optional_text_is(mut self, value: impl Into<teaql_core::Value>) -> Self {
        self.query = self.query.and_filter(Expr::eq("optional_text", value));
        self
    }



    pub fn with_optional_text_is_not(mut self, value: impl Into<teaql_core::Value>) -> Self {
        self.query = self.query.and_filter(Expr::ne("optional_text", value));
        self
    }

    pub fn with_optional_text_greater_than(mut self, value: impl Into<teaql_core::Value>) -> Self {
        self.query = self.query.and_filter(Expr::gt("optional_text", value));
        self
    }

    pub fn with_optional_text_greater_than_or_equal_to(mut self, value: impl Into<teaql_core::Value>) -> Self {
        self.query = self.query.and_filter(Expr::gte("optional_text", value));
        self
    }

    pub fn with_optional_text_less_than(mut self, value: impl Into<teaql_core::Value>) -> Self {
        self.query = self.query.and_filter(Expr::lt("optional_text", value));
        self
    }

    pub fn with_optional_text_less_than_or_equal_to(mut self, value: impl Into<teaql_core::Value>) -> Self {
        self.query = self.query.and_filter(Expr::lte("optional_text", value));
        self
    }

    pub fn with_optional_text_between(
        mut self,
        lower: impl Into<teaql_core::Value>,
        upper: impl Into<teaql_core::Value>,
    ) -> Self {
        self.query = self.query.and_filter(Expr::between("optional_text", lower, upper));
        self
    }

    pub fn with_optional_text_between_range<T>(mut self, range: DateRange<T>) -> Self
    where
        T: Into<teaql_core::Value>,
    {
        self.query = self.query.and_filter(Expr::between(
            "optional_text",
            range.start,
            range.end,
        ));
        self
    }

    pub fn with_optional_text_in(
        mut self,
        values: impl IntoIterator<Item = impl Into<teaql_core::Value>>,
    ) -> Self {
        self.query = self.query.and_filter(Expr::in_list(
            "optional_text",
            values.into_iter().map(Into::into),
        ));
        self
    }

    pub fn with_optional_text_not_in(
        mut self,
        values: impl IntoIterator<Item = impl Into<teaql_core::Value>>,
    ) -> Self {
        self.query = self.query.and_filter(Expr::not_in_list(
            "optional_text",
            values.into_iter().map(Into::into),
        ));
        self
    }

    pub fn with_optional_text_containing(mut self, value: impl Into<String>) -> Self {
        self.query = self.query.and_filter(Expr::contain("optional_text", value));
        self
    }

    pub fn with_optional_text_not_containing(mut self, value: impl Into<String>) -> Self {
        self.query = self.query.and_filter(Expr::not_contain("optional_text", value));
        self
    }

    pub fn with_optional_text_starting_with(mut self, value: impl Into<String>) -> Self {
        self.query = self.query.and_filter(Expr::begin_with("optional_text", value));
        self
    }

    pub fn with_optional_text_not_starting_with(mut self, value: impl Into<String>) -> Self {
        self.query = self.query.and_filter(Expr::not_begin_with("optional_text", value));
        self
    }

    pub fn with_optional_text_ending_with(mut self, value: impl Into<String>) -> Self {
        self.query = self.query.and_filter(Expr::end_with("optional_text", value));
        self
    }

    pub fn with_optional_text_not_ending_with(mut self, value: impl Into<String>) -> Self {
        self.query = self.query.and_filter(Expr::not_end_with("optional_text", value));
        self
    }

    pub fn with_optional_text_sounding_like(mut self, value: impl Into<teaql_core::Value>) -> Self {
        self.query = self.query.and_filter(Expr::sound_like("optional_text", value));
        self
    }
    pub fn with_optional_text_before(mut self, value: impl Into<teaql_core::Value>) -> Self {
        self.query = self.query.and_filter(Expr::lt("optional_text", value));
        self
    }

    pub fn with_optional_text_after(mut self, value: impl Into<teaql_core::Value>) -> Self {
        self.query = self.query.and_filter(Expr::gt("optional_text", value));
        self
    }

    pub fn with_optional_text_is_unknown(mut self) -> Self {
        self.query = self.query.and_filter(Expr::is_null("optional_text"));
        self
    }



    pub fn with_optional_text_is_known(mut self) -> Self {
        self.query = self.query.and_filter(Expr::is_not_null("optional_text"));
        self
    }


    pub fn order_by_optional_text_asc(mut self) -> Self {
        self.query = self.query.order_asc("optional_text");
        self
    }

    pub fn order_by_optional_text_desc(mut self) -> Self {
        self.query = self.query.order_desc("optional_text");
        self
    }

    pub fn order_by_optional_text_asc_using_gbk(mut self) -> Self {
        self.query = self.query.order_gbk_asc("optional_text");
        self
    }

    pub fn order_by_optional_text_desc_using_gbk(mut self) -> Self {
        self.query = self.query.order_gbk_desc("optional_text");
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


    pub fn select_optional_code(mut self) -> Self {
        self.query = self.query.project("optional_code");
        self
    }

    pub fn project_optional_code(self) -> Self {
        self.select_optional_code()
    }

    pub fn select_optional_code_raw(self, raw_sql_segment: impl Into<String>) -> Self {
        self.select_optional_code_unsafe_raw(UnsafeRawSqlSegment::trusted(raw_sql_segment))
    }

    pub fn select_optional_code_unsafe_raw(mut self, raw_sql_segment: UnsafeRawSqlSegment) -> Self {
        self.query_options
            .raw_projections
            .push(RawProjection::new("optional_code", raw_sql_segment));
        self
    }

    pub fn group_by_optional_code(self) -> Self {
        self.group_by("optional_code")
    }

    pub fn group_by_optional_code_as(self, alias: impl Into<String>) -> Self {
        let alias = alias.into();
        let mut request = self.group_by("optional_code");
        request.query = request
            .query
            .project_expr(alias, Expr::column("optional_code"));
        request
    }

    pub fn group_by_optional_code_with_function(
        self,
        alias: impl Into<String>,
        function: AggregateFunction,
    ) -> Self {
        self.group_by("optional_code")
            .aggregate_with_function("optional_code", alias, function)
    }

    pub fn count_optional_code(self) -> Self {
        self.count_optional_code_as("optional_code_count")
    }

    pub fn count_optional_code_as(self, alias: impl Into<String>) -> Self {
        self.aggregate_count_field("optional_code", alias)
    }

    pub fn sum_optional_code(self) -> Self {
        self.sum_optional_code_as("sum_optional_code")
    }

    pub fn sum_optional_code_as(self, alias: impl Into<String>) -> Self {
        self.aggregate_sum("optional_code", alias)
    }

    pub fn avg_optional_code(self) -> Self {
        self.avg_optional_code_as("avg_optional_code")
    }

    pub fn avg_optional_code_as(self, alias: impl Into<String>) -> Self {
        self.aggregate_avg("optional_code", alias)
    }

    pub fn min_optional_code(self) -> Self {
        self.min_optional_code_as("min_optional_code")
    }

    pub fn min_optional_code_as(self, alias: impl Into<String>) -> Self {
        self.aggregate_min("optional_code", alias)
    }

    pub fn max_optional_code(self) -> Self {
        self.max_optional_code_as("max_optional_code")
    }

    pub fn max_optional_code_as(self, alias: impl Into<String>) -> Self {
        self.aggregate_max("optional_code", alias)
    }

    pub fn unselect_optional_code(mut self) -> Self {
        self.query.projection.retain(|field| field != "optional_code");
        self.query_options.raw_projections.retain(|projection| projection.property_name != "optional_code");
        self
    }


    pub fn with_optional_code(
        mut self,
        operator: FieldOperator,
        values: impl IntoIterator<Item = impl Into<teaql_core::Value>>,
    ) -> Self {
        self.query = self.query.and_filter(field_operator_expr(
            "optional_code",
            operator,
            values.into_iter().map(Into::into).collect(),
        ));
        self
    }

    pub fn create_optional_code_criteria(
        operator: FieldOperator,
        values: impl IntoIterator<Item = impl Into<teaql_core::Value>>,
    ) -> Expr {
        field_operator_expr(
            "optional_code",
            operator,
            values.into_iter().map(Into::into).collect(),
        )
    }

    pub fn with_optional_code_is(mut self, value: impl Into<teaql_core::Value>) -> Self {
        self.query = self.query.and_filter(Expr::eq("optional_code", value));
        self
    }



    pub fn with_optional_code_is_not(mut self, value: impl Into<teaql_core::Value>) -> Self {
        self.query = self.query.and_filter(Expr::ne("optional_code", value));
        self
    }

    pub fn with_optional_code_greater_than(mut self, value: impl Into<teaql_core::Value>) -> Self {
        self.query = self.query.and_filter(Expr::gt("optional_code", value));
        self
    }

    pub fn with_optional_code_greater_than_or_equal_to(mut self, value: impl Into<teaql_core::Value>) -> Self {
        self.query = self.query.and_filter(Expr::gte("optional_code", value));
        self
    }

    pub fn with_optional_code_less_than(mut self, value: impl Into<teaql_core::Value>) -> Self {
        self.query = self.query.and_filter(Expr::lt("optional_code", value));
        self
    }

    pub fn with_optional_code_less_than_or_equal_to(mut self, value: impl Into<teaql_core::Value>) -> Self {
        self.query = self.query.and_filter(Expr::lte("optional_code", value));
        self
    }

    pub fn with_optional_code_between(
        mut self,
        lower: impl Into<teaql_core::Value>,
        upper: impl Into<teaql_core::Value>,
    ) -> Self {
        self.query = self.query.and_filter(Expr::between("optional_code", lower, upper));
        self
    }

    pub fn with_optional_code_between_range<T>(mut self, range: DateRange<T>) -> Self
    where
        T: Into<teaql_core::Value>,
    {
        self.query = self.query.and_filter(Expr::between(
            "optional_code",
            range.start,
            range.end,
        ));
        self
    }

    pub fn with_optional_code_in(
        mut self,
        values: impl IntoIterator<Item = impl Into<teaql_core::Value>>,
    ) -> Self {
        self.query = self.query.and_filter(Expr::in_list(
            "optional_code",
            values.into_iter().map(Into::into),
        ));
        self
    }

    pub fn with_optional_code_not_in(
        mut self,
        values: impl IntoIterator<Item = impl Into<teaql_core::Value>>,
    ) -> Self {
        self.query = self.query.and_filter(Expr::not_in_list(
            "optional_code",
            values.into_iter().map(Into::into),
        ));
        self
    }

    pub fn with_optional_code_containing(mut self, value: impl Into<String>) -> Self {
        self.query = self.query.and_filter(Expr::contain("optional_code", value));
        self
    }

    pub fn with_optional_code_not_containing(mut self, value: impl Into<String>) -> Self {
        self.query = self.query.and_filter(Expr::not_contain("optional_code", value));
        self
    }

    pub fn with_optional_code_starting_with(mut self, value: impl Into<String>) -> Self {
        self.query = self.query.and_filter(Expr::begin_with("optional_code", value));
        self
    }

    pub fn with_optional_code_not_starting_with(mut self, value: impl Into<String>) -> Self {
        self.query = self.query.and_filter(Expr::not_begin_with("optional_code", value));
        self
    }

    pub fn with_optional_code_ending_with(mut self, value: impl Into<String>) -> Self {
        self.query = self.query.and_filter(Expr::end_with("optional_code", value));
        self
    }

    pub fn with_optional_code_not_ending_with(mut self, value: impl Into<String>) -> Self {
        self.query = self.query.and_filter(Expr::not_end_with("optional_code", value));
        self
    }

    pub fn with_optional_code_sounding_like(mut self, value: impl Into<teaql_core::Value>) -> Self {
        self.query = self.query.and_filter(Expr::sound_like("optional_code", value));
        self
    }
    pub fn with_optional_code_before(mut self, value: impl Into<teaql_core::Value>) -> Self {
        self.query = self.query.and_filter(Expr::lt("optional_code", value));
        self
    }

    pub fn with_optional_code_after(mut self, value: impl Into<teaql_core::Value>) -> Self {
        self.query = self.query.and_filter(Expr::gt("optional_code", value));
        self
    }

    pub fn with_optional_code_is_unknown(mut self) -> Self {
        self.query = self.query.and_filter(Expr::is_null("optional_code"));
        self
    }



    pub fn with_optional_code_is_known(mut self) -> Self {
        self.query = self.query.and_filter(Expr::is_not_null("optional_code"));
        self
    }


    pub fn order_by_optional_code_asc(mut self) -> Self {
        self.query = self.query.order_asc("optional_code");
        self
    }

    pub fn order_by_optional_code_desc(mut self) -> Self {
        self.query = self.query.order_desc("optional_code");
        self
    }

    pub fn order_by_optional_code_asc_using_gbk(mut self) -> Self {
        self.query = self.query.order_gbk_asc("optional_code");
        self
    }

    pub fn order_by_optional_code_desc_using_gbk(mut self) -> Self {
        self.query = self.query.order_gbk_desc("optional_code");
        self
    }


    pub fn select_integer(mut self) -> Self {
        self.query = self.query.project("integer");
        self
    }

    pub fn project_integer(self) -> Self {
        self.select_integer()
    }

    pub fn select_integer_raw(self, raw_sql_segment: impl Into<String>) -> Self {
        self.select_integer_unsafe_raw(UnsafeRawSqlSegment::trusted(raw_sql_segment))
    }

    pub fn select_integer_unsafe_raw(mut self, raw_sql_segment: UnsafeRawSqlSegment) -> Self {
        self.query_options
            .raw_projections
            .push(RawProjection::new("integer", raw_sql_segment));
        self
    }

    pub fn select_integer_with_function(self, function: AggregateFunction) -> Self {
        self.select_integer_as_with_function("integer", function)
    }

    pub fn select_integer_as_with_function(
        self,
        alias: impl Into<String>,
        function: AggregateFunction,
    ) -> Self {
        self.aggregate_with_function("integer", alias, function)
    }

    pub fn group_by_integer(self) -> Self {
        self.group_by("integer")
    }

    pub fn group_by_integer_as(self, alias: impl Into<String>) -> Self {
        let alias = alias.into();
        let mut request = self.group_by("integer");
        request.query = request
            .query
            .project_expr(alias, Expr::column("integer"));
        request
    }

    pub fn group_by_integer_with_function(
        self,
        alias: impl Into<String>,
        function: AggregateFunction,
    ) -> Self {
        self.group_by("integer")
            .aggregate_with_function("integer", alias, function)
    }

    pub fn count_integer(self) -> Self {
        self.count_integer_as("integer_count")
    }

    pub fn count_integer_as(self, alias: impl Into<String>) -> Self {
        self.aggregate_count_field("integer", alias)
    }

    pub fn sum_integer(self) -> Self {
        self.sum_integer_as("sum_integer")
    }

    pub fn sum_integer_as(self, alias: impl Into<String>) -> Self {
        self.aggregate_sum("integer", alias)
    }

    pub fn avg_integer(self) -> Self {
        self.avg_integer_as("avg_integer")
    }

    pub fn avg_integer_as(self, alias: impl Into<String>) -> Self {
        self.aggregate_avg("integer", alias)
    }

    pub fn min_integer(self) -> Self {
        self.min_integer_as("min_integer")
    }

    pub fn min_integer_as(self, alias: impl Into<String>) -> Self {
        self.aggregate_min("integer", alias)
    }

    pub fn max_integer(self) -> Self {
        self.max_integer_as("max_integer")
    }

    pub fn max_integer_as(self, alias: impl Into<String>) -> Self {
        self.aggregate_max("integer", alias)
    }

    pub fn standard_deviation_integer(self) -> Self {
        self.standard_deviation_integer_as("stdDev_integer")
    }

    pub fn standard_deviation_integer_as(self, alias: impl Into<String>) -> Self {
        self.aggregate_stddev("integer", alias)
    }

    pub fn square_root_of_population_standard_deviation_integer(self) -> Self {
        self.square_root_of_population_standard_deviation_integer_as("stdDevPop_integer")
    }

    pub fn square_root_of_population_standard_deviation_integer_as(self, alias: impl Into<String>) -> Self {
        self.aggregate_stddev_pop("integer", alias)
    }

    pub fn sample_variance_integer(self) -> Self {
        self.sample_variance_integer_as("varSamp_integer")
    }

    pub fn sample_variance_integer_as(self, alias: impl Into<String>) -> Self {
        self.aggregate_var_samp("integer", alias)
    }

    pub fn sample_population_variance_integer(self) -> Self {
        self.sample_population_variance_integer_as("varPop_integer")
    }

    pub fn sample_population_variance_integer_as(self, alias: impl Into<String>) -> Self {
        self.aggregate_var_pop("integer", alias)
    }

    pub fn unselect_integer(mut self) -> Self {
        self.query.projection.retain(|field| field != "integer");
        self.query_options.raw_projections.retain(|projection| projection.property_name != "integer");
        self
    }


    pub fn with_integer(
        mut self,
        operator: FieldOperator,
        values: impl IntoIterator<Item = impl Into<teaql_core::Value>>,
    ) -> Self {
        self.query = self.query.and_filter(field_operator_expr(
            "integer",
            operator,
            values.into_iter().map(Into::into).collect(),
        ));
        self
    }

    pub fn create_integer_criteria(
        operator: FieldOperator,
        values: impl IntoIterator<Item = impl Into<teaql_core::Value>>,
    ) -> Expr {
        field_operator_expr(
            "integer",
            operator,
            values.into_iter().map(Into::into).collect(),
        )
    }

    pub fn with_integer_is(mut self, value: impl Into<teaql_core::Value>) -> Self {
        self.query = self.query.and_filter(Expr::eq("integer", value));
        self
    }



    pub fn with_integer_is_not(mut self, value: impl Into<teaql_core::Value>) -> Self {
        self.query = self.query.and_filter(Expr::ne("integer", value));
        self
    }

    pub fn with_integer_greater_than(mut self, value: impl Into<teaql_core::Value>) -> Self {
        self.query = self.query.and_filter(Expr::gt("integer", value));
        self
    }

    pub fn with_integer_greater_than_or_equal_to(mut self, value: impl Into<teaql_core::Value>) -> Self {
        self.query = self.query.and_filter(Expr::gte("integer", value));
        self
    }

    pub fn with_integer_less_than(mut self, value: impl Into<teaql_core::Value>) -> Self {
        self.query = self.query.and_filter(Expr::lt("integer", value));
        self
    }

    pub fn with_integer_less_than_or_equal_to(mut self, value: impl Into<teaql_core::Value>) -> Self {
        self.query = self.query.and_filter(Expr::lte("integer", value));
        self
    }

    pub fn with_integer_between(
        mut self,
        lower: impl Into<teaql_core::Value>,
        upper: impl Into<teaql_core::Value>,
    ) -> Self {
        self.query = self.query.and_filter(Expr::between("integer", lower, upper));
        self
    }

    pub fn with_integer_between_range<T>(mut self, range: DateRange<T>) -> Self
    where
        T: Into<teaql_core::Value>,
    {
        self.query = self.query.and_filter(Expr::between(
            "integer",
            range.start,
            range.end,
        ));
        self
    }

    pub fn with_integer_in(
        mut self,
        values: impl IntoIterator<Item = impl Into<teaql_core::Value>>,
    ) -> Self {
        self.query = self.query.and_filter(Expr::in_list(
            "integer",
            values.into_iter().map(Into::into),
        ));
        self
    }

    pub fn with_integer_not_in(
        mut self,
        values: impl IntoIterator<Item = impl Into<teaql_core::Value>>,
    ) -> Self {
        self.query = self.query.and_filter(Expr::not_in_list(
            "integer",
            values.into_iter().map(Into::into),
        ));
        self
    }

    pub fn with_integer_before(mut self, value: impl Into<teaql_core::Value>) -> Self {
        self.query = self.query.and_filter(Expr::lt("integer", value));
        self
    }

    pub fn with_integer_after(mut self, value: impl Into<teaql_core::Value>) -> Self {
        self.query = self.query.and_filter(Expr::gt("integer", value));
        self
    }

    pub fn with_integer_is_unknown(mut self) -> Self {
        self.query = self.query.and_filter(Expr::is_null("integer"));
        self
    }



    pub fn with_integer_is_known(mut self) -> Self {
        self.query = self.query.and_filter(Expr::is_not_null("integer"));
        self
    }


    pub fn order_by_integer_asc(mut self) -> Self {
        self.query = self.query.order_asc("integer");
        self
    }

    pub fn order_by_integer_desc(mut self) -> Self {
        self.query = self.query.order_desc("integer");
        self
    }

    pub fn order_by_integer_asc_using_gbk(mut self) -> Self {
        self.query = self.query.order_gbk_asc("integer");
        self
    }

    pub fn order_by_integer_desc_using_gbk(mut self) -> Self {
        self.query = self.query.order_gbk_desc("integer");
        self
    }


    pub fn select_optional_integer(mut self) -> Self {
        self.query = self.query.project("optional_integer");
        self
    }

    pub fn project_optional_integer(self) -> Self {
        self.select_optional_integer()
    }

    pub fn select_optional_integer_raw(self, raw_sql_segment: impl Into<String>) -> Self {
        self.select_optional_integer_unsafe_raw(UnsafeRawSqlSegment::trusted(raw_sql_segment))
    }

    pub fn select_optional_integer_unsafe_raw(mut self, raw_sql_segment: UnsafeRawSqlSegment) -> Self {
        self.query_options
            .raw_projections
            .push(RawProjection::new("optional_integer", raw_sql_segment));
        self
    }

    pub fn select_optional_integer_with_function(self, function: AggregateFunction) -> Self {
        self.select_optional_integer_as_with_function("optional_integer", function)
    }

    pub fn select_optional_integer_as_with_function(
        self,
        alias: impl Into<String>,
        function: AggregateFunction,
    ) -> Self {
        self.aggregate_with_function("optional_integer", alias, function)
    }

    pub fn group_by_optional_integer(self) -> Self {
        self.group_by("optional_integer")
    }

    pub fn group_by_optional_integer_as(self, alias: impl Into<String>) -> Self {
        let alias = alias.into();
        let mut request = self.group_by("optional_integer");
        request.query = request
            .query
            .project_expr(alias, Expr::column("optional_integer"));
        request
    }

    pub fn group_by_optional_integer_with_function(
        self,
        alias: impl Into<String>,
        function: AggregateFunction,
    ) -> Self {
        self.group_by("optional_integer")
            .aggregate_with_function("optional_integer", alias, function)
    }

    pub fn count_optional_integer(self) -> Self {
        self.count_optional_integer_as("optional_integer_count")
    }

    pub fn count_optional_integer_as(self, alias: impl Into<String>) -> Self {
        self.aggregate_count_field("optional_integer", alias)
    }

    pub fn sum_optional_integer(self) -> Self {
        self.sum_optional_integer_as("sum_optional_integer")
    }

    pub fn sum_optional_integer_as(self, alias: impl Into<String>) -> Self {
        self.aggregate_sum("optional_integer", alias)
    }

    pub fn avg_optional_integer(self) -> Self {
        self.avg_optional_integer_as("avg_optional_integer")
    }

    pub fn avg_optional_integer_as(self, alias: impl Into<String>) -> Self {
        self.aggregate_avg("optional_integer", alias)
    }

    pub fn min_optional_integer(self) -> Self {
        self.min_optional_integer_as("min_optional_integer")
    }

    pub fn min_optional_integer_as(self, alias: impl Into<String>) -> Self {
        self.aggregate_min("optional_integer", alias)
    }

    pub fn max_optional_integer(self) -> Self {
        self.max_optional_integer_as("max_optional_integer")
    }

    pub fn max_optional_integer_as(self, alias: impl Into<String>) -> Self {
        self.aggregate_max("optional_integer", alias)
    }

    pub fn standard_deviation_optional_integer(self) -> Self {
        self.standard_deviation_optional_integer_as("stdDev_optional_integer")
    }

    pub fn standard_deviation_optional_integer_as(self, alias: impl Into<String>) -> Self {
        self.aggregate_stddev("optional_integer", alias)
    }

    pub fn square_root_of_population_standard_deviation_optional_integer(self) -> Self {
        self.square_root_of_population_standard_deviation_optional_integer_as("stdDevPop_optional_integer")
    }

    pub fn square_root_of_population_standard_deviation_optional_integer_as(self, alias: impl Into<String>) -> Self {
        self.aggregate_stddev_pop("optional_integer", alias)
    }

    pub fn sample_variance_optional_integer(self) -> Self {
        self.sample_variance_optional_integer_as("varSamp_optional_integer")
    }

    pub fn sample_variance_optional_integer_as(self, alias: impl Into<String>) -> Self {
        self.aggregate_var_samp("optional_integer", alias)
    }

    pub fn sample_population_variance_optional_integer(self) -> Self {
        self.sample_population_variance_optional_integer_as("varPop_optional_integer")
    }

    pub fn sample_population_variance_optional_integer_as(self, alias: impl Into<String>) -> Self {
        self.aggregate_var_pop("optional_integer", alias)
    }

    pub fn unselect_optional_integer(mut self) -> Self {
        self.query.projection.retain(|field| field != "optional_integer");
        self.query_options.raw_projections.retain(|projection| projection.property_name != "optional_integer");
        self
    }


    pub fn with_optional_integer(
        mut self,
        operator: FieldOperator,
        values: impl IntoIterator<Item = impl Into<teaql_core::Value>>,
    ) -> Self {
        self.query = self.query.and_filter(field_operator_expr(
            "optional_integer",
            operator,
            values.into_iter().map(Into::into).collect(),
        ));
        self
    }

    pub fn create_optional_integer_criteria(
        operator: FieldOperator,
        values: impl IntoIterator<Item = impl Into<teaql_core::Value>>,
    ) -> Expr {
        field_operator_expr(
            "optional_integer",
            operator,
            values.into_iter().map(Into::into).collect(),
        )
    }

    pub fn with_optional_integer_is(mut self, value: impl Into<teaql_core::Value>) -> Self {
        self.query = self.query.and_filter(Expr::eq("optional_integer", value));
        self
    }



    pub fn with_optional_integer_is_not(mut self, value: impl Into<teaql_core::Value>) -> Self {
        self.query = self.query.and_filter(Expr::ne("optional_integer", value));
        self
    }

    pub fn with_optional_integer_greater_than(mut self, value: impl Into<teaql_core::Value>) -> Self {
        self.query = self.query.and_filter(Expr::gt("optional_integer", value));
        self
    }

    pub fn with_optional_integer_greater_than_or_equal_to(mut self, value: impl Into<teaql_core::Value>) -> Self {
        self.query = self.query.and_filter(Expr::gte("optional_integer", value));
        self
    }

    pub fn with_optional_integer_less_than(mut self, value: impl Into<teaql_core::Value>) -> Self {
        self.query = self.query.and_filter(Expr::lt("optional_integer", value));
        self
    }

    pub fn with_optional_integer_less_than_or_equal_to(mut self, value: impl Into<teaql_core::Value>) -> Self {
        self.query = self.query.and_filter(Expr::lte("optional_integer", value));
        self
    }

    pub fn with_optional_integer_between(
        mut self,
        lower: impl Into<teaql_core::Value>,
        upper: impl Into<teaql_core::Value>,
    ) -> Self {
        self.query = self.query.and_filter(Expr::between("optional_integer", lower, upper));
        self
    }

    pub fn with_optional_integer_between_range<T>(mut self, range: DateRange<T>) -> Self
    where
        T: Into<teaql_core::Value>,
    {
        self.query = self.query.and_filter(Expr::between(
            "optional_integer",
            range.start,
            range.end,
        ));
        self
    }

    pub fn with_optional_integer_in(
        mut self,
        values: impl IntoIterator<Item = impl Into<teaql_core::Value>>,
    ) -> Self {
        self.query = self.query.and_filter(Expr::in_list(
            "optional_integer",
            values.into_iter().map(Into::into),
        ));
        self
    }

    pub fn with_optional_integer_not_in(
        mut self,
        values: impl IntoIterator<Item = impl Into<teaql_core::Value>>,
    ) -> Self {
        self.query = self.query.and_filter(Expr::not_in_list(
            "optional_integer",
            values.into_iter().map(Into::into),
        ));
        self
    }

    pub fn with_optional_integer_before(mut self, value: impl Into<teaql_core::Value>) -> Self {
        self.query = self.query.and_filter(Expr::lt("optional_integer", value));
        self
    }

    pub fn with_optional_integer_after(mut self, value: impl Into<teaql_core::Value>) -> Self {
        self.query = self.query.and_filter(Expr::gt("optional_integer", value));
        self
    }

    pub fn with_optional_integer_is_unknown(mut self) -> Self {
        self.query = self.query.and_filter(Expr::is_null("optional_integer"));
        self
    }



    pub fn with_optional_integer_is_known(mut self) -> Self {
        self.query = self.query.and_filter(Expr::is_not_null("optional_integer"));
        self
    }


    pub fn order_by_optional_integer_asc(mut self) -> Self {
        self.query = self.query.order_asc("optional_integer");
        self
    }

    pub fn order_by_optional_integer_desc(mut self) -> Self {
        self.query = self.query.order_desc("optional_integer");
        self
    }

    pub fn order_by_optional_integer_asc_using_gbk(mut self) -> Self {
        self.query = self.query.order_gbk_asc("optional_integer");
        self
    }

    pub fn order_by_optional_integer_desc_using_gbk(mut self) -> Self {
        self.query = self.query.order_gbk_desc("optional_integer");
        self
    }


    pub fn select_long(mut self) -> Self {
        self.query = self.query.project("long");
        self
    }

    pub fn project_long(self) -> Self {
        self.select_long()
    }

    pub fn select_long_raw(self, raw_sql_segment: impl Into<String>) -> Self {
        self.select_long_unsafe_raw(UnsafeRawSqlSegment::trusted(raw_sql_segment))
    }

    pub fn select_long_unsafe_raw(mut self, raw_sql_segment: UnsafeRawSqlSegment) -> Self {
        self.query_options
            .raw_projections
            .push(RawProjection::new("long", raw_sql_segment));
        self
    }

    pub fn select_long_with_function(self, function: AggregateFunction) -> Self {
        self.select_long_as_with_function("long", function)
    }

    pub fn select_long_as_with_function(
        self,
        alias: impl Into<String>,
        function: AggregateFunction,
    ) -> Self {
        self.aggregate_with_function("long", alias, function)
    }

    pub fn group_by_long(self) -> Self {
        self.group_by("long")
    }

    pub fn group_by_long_as(self, alias: impl Into<String>) -> Self {
        let alias = alias.into();
        let mut request = self.group_by("long");
        request.query = request
            .query
            .project_expr(alias, Expr::column("long"));
        request
    }

    pub fn group_by_long_with_function(
        self,
        alias: impl Into<String>,
        function: AggregateFunction,
    ) -> Self {
        self.group_by("long")
            .aggregate_with_function("long", alias, function)
    }

    pub fn count_long(self) -> Self {
        self.count_long_as("long_count")
    }

    pub fn count_long_as(self, alias: impl Into<String>) -> Self {
        self.aggregate_count_field("long", alias)
    }

    pub fn sum_long(self) -> Self {
        self.sum_long_as("sum_long")
    }

    pub fn sum_long_as(self, alias: impl Into<String>) -> Self {
        self.aggregate_sum("long", alias)
    }

    pub fn avg_long(self) -> Self {
        self.avg_long_as("avg_long")
    }

    pub fn avg_long_as(self, alias: impl Into<String>) -> Self {
        self.aggregate_avg("long", alias)
    }

    pub fn min_long(self) -> Self {
        self.min_long_as("min_long")
    }

    pub fn min_long_as(self, alias: impl Into<String>) -> Self {
        self.aggregate_min("long", alias)
    }

    pub fn max_long(self) -> Self {
        self.max_long_as("max_long")
    }

    pub fn max_long_as(self, alias: impl Into<String>) -> Self {
        self.aggregate_max("long", alias)
    }

    pub fn standard_deviation_long(self) -> Self {
        self.standard_deviation_long_as("stdDev_long")
    }

    pub fn standard_deviation_long_as(self, alias: impl Into<String>) -> Self {
        self.aggregate_stddev("long", alias)
    }

    pub fn square_root_of_population_standard_deviation_long(self) -> Self {
        self.square_root_of_population_standard_deviation_long_as("stdDevPop_long")
    }

    pub fn square_root_of_population_standard_deviation_long_as(self, alias: impl Into<String>) -> Self {
        self.aggregate_stddev_pop("long", alias)
    }

    pub fn sample_variance_long(self) -> Self {
        self.sample_variance_long_as("varSamp_long")
    }

    pub fn sample_variance_long_as(self, alias: impl Into<String>) -> Self {
        self.aggregate_var_samp("long", alias)
    }

    pub fn sample_population_variance_long(self) -> Self {
        self.sample_population_variance_long_as("varPop_long")
    }

    pub fn sample_population_variance_long_as(self, alias: impl Into<String>) -> Self {
        self.aggregate_var_pop("long", alias)
    }

    pub fn unselect_long(mut self) -> Self {
        self.query.projection.retain(|field| field != "long");
        self.query_options.raw_projections.retain(|projection| projection.property_name != "long");
        self
    }


    pub fn with_long(
        mut self,
        operator: FieldOperator,
        values: impl IntoIterator<Item = impl Into<teaql_core::Value>>,
    ) -> Self {
        self.query = self.query.and_filter(field_operator_expr(
            "long",
            operator,
            values.into_iter().map(Into::into).collect(),
        ));
        self
    }

    pub fn create_long_criteria(
        operator: FieldOperator,
        values: impl IntoIterator<Item = impl Into<teaql_core::Value>>,
    ) -> Expr {
        field_operator_expr(
            "long",
            operator,
            values.into_iter().map(Into::into).collect(),
        )
    }

    pub fn with_long_is(mut self, value: impl Into<teaql_core::Value>) -> Self {
        self.query = self.query.and_filter(Expr::eq("long", value));
        self
    }



    pub fn with_long_is_not(mut self, value: impl Into<teaql_core::Value>) -> Self {
        self.query = self.query.and_filter(Expr::ne("long", value));
        self
    }

    pub fn with_long_greater_than(mut self, value: impl Into<teaql_core::Value>) -> Self {
        self.query = self.query.and_filter(Expr::gt("long", value));
        self
    }

    pub fn with_long_greater_than_or_equal_to(mut self, value: impl Into<teaql_core::Value>) -> Self {
        self.query = self.query.and_filter(Expr::gte("long", value));
        self
    }

    pub fn with_long_less_than(mut self, value: impl Into<teaql_core::Value>) -> Self {
        self.query = self.query.and_filter(Expr::lt("long", value));
        self
    }

    pub fn with_long_less_than_or_equal_to(mut self, value: impl Into<teaql_core::Value>) -> Self {
        self.query = self.query.and_filter(Expr::lte("long", value));
        self
    }

    pub fn with_long_between(
        mut self,
        lower: impl Into<teaql_core::Value>,
        upper: impl Into<teaql_core::Value>,
    ) -> Self {
        self.query = self.query.and_filter(Expr::between("long", lower, upper));
        self
    }

    pub fn with_long_between_range<T>(mut self, range: DateRange<T>) -> Self
    where
        T: Into<teaql_core::Value>,
    {
        self.query = self.query.and_filter(Expr::between(
            "long",
            range.start,
            range.end,
        ));
        self
    }

    pub fn with_long_in(
        mut self,
        values: impl IntoIterator<Item = impl Into<teaql_core::Value>>,
    ) -> Self {
        self.query = self.query.and_filter(Expr::in_list(
            "long",
            values.into_iter().map(Into::into),
        ));
        self
    }

    pub fn with_long_not_in(
        mut self,
        values: impl IntoIterator<Item = impl Into<teaql_core::Value>>,
    ) -> Self {
        self.query = self.query.and_filter(Expr::not_in_list(
            "long",
            values.into_iter().map(Into::into),
        ));
        self
    }

    pub fn with_long_before(mut self, value: impl Into<teaql_core::Value>) -> Self {
        self.query = self.query.and_filter(Expr::lt("long", value));
        self
    }

    pub fn with_long_after(mut self, value: impl Into<teaql_core::Value>) -> Self {
        self.query = self.query.and_filter(Expr::gt("long", value));
        self
    }

    pub fn with_long_is_unknown(mut self) -> Self {
        self.query = self.query.and_filter(Expr::is_null("long"));
        self
    }



    pub fn with_long_is_known(mut self) -> Self {
        self.query = self.query.and_filter(Expr::is_not_null("long"));
        self
    }


    pub fn order_by_long_asc(mut self) -> Self {
        self.query = self.query.order_asc("long");
        self
    }

    pub fn order_by_long_desc(mut self) -> Self {
        self.query = self.query.order_desc("long");
        self
    }

    pub fn order_by_long_asc_using_gbk(mut self) -> Self {
        self.query = self.query.order_gbk_asc("long");
        self
    }

    pub fn order_by_long_desc_using_gbk(mut self) -> Self {
        self.query = self.query.order_gbk_desc("long");
        self
    }


    pub fn select_optional_long(mut self) -> Self {
        self.query = self.query.project("optional_long");
        self
    }

    pub fn project_optional_long(self) -> Self {
        self.select_optional_long()
    }

    pub fn select_optional_long_raw(self, raw_sql_segment: impl Into<String>) -> Self {
        self.select_optional_long_unsafe_raw(UnsafeRawSqlSegment::trusted(raw_sql_segment))
    }

    pub fn select_optional_long_unsafe_raw(mut self, raw_sql_segment: UnsafeRawSqlSegment) -> Self {
        self.query_options
            .raw_projections
            .push(RawProjection::new("optional_long", raw_sql_segment));
        self
    }

    pub fn select_optional_long_with_function(self, function: AggregateFunction) -> Self {
        self.select_optional_long_as_with_function("optional_long", function)
    }

    pub fn select_optional_long_as_with_function(
        self,
        alias: impl Into<String>,
        function: AggregateFunction,
    ) -> Self {
        self.aggregate_with_function("optional_long", alias, function)
    }

    pub fn group_by_optional_long(self) -> Self {
        self.group_by("optional_long")
    }

    pub fn group_by_optional_long_as(self, alias: impl Into<String>) -> Self {
        let alias = alias.into();
        let mut request = self.group_by("optional_long");
        request.query = request
            .query
            .project_expr(alias, Expr::column("optional_long"));
        request
    }

    pub fn group_by_optional_long_with_function(
        self,
        alias: impl Into<String>,
        function: AggregateFunction,
    ) -> Self {
        self.group_by("optional_long")
            .aggregate_with_function("optional_long", alias, function)
    }

    pub fn count_optional_long(self) -> Self {
        self.count_optional_long_as("optional_long_count")
    }

    pub fn count_optional_long_as(self, alias: impl Into<String>) -> Self {
        self.aggregate_count_field("optional_long", alias)
    }

    pub fn sum_optional_long(self) -> Self {
        self.sum_optional_long_as("sum_optional_long")
    }

    pub fn sum_optional_long_as(self, alias: impl Into<String>) -> Self {
        self.aggregate_sum("optional_long", alias)
    }

    pub fn avg_optional_long(self) -> Self {
        self.avg_optional_long_as("avg_optional_long")
    }

    pub fn avg_optional_long_as(self, alias: impl Into<String>) -> Self {
        self.aggregate_avg("optional_long", alias)
    }

    pub fn min_optional_long(self) -> Self {
        self.min_optional_long_as("min_optional_long")
    }

    pub fn min_optional_long_as(self, alias: impl Into<String>) -> Self {
        self.aggregate_min("optional_long", alias)
    }

    pub fn max_optional_long(self) -> Self {
        self.max_optional_long_as("max_optional_long")
    }

    pub fn max_optional_long_as(self, alias: impl Into<String>) -> Self {
        self.aggregate_max("optional_long", alias)
    }

    pub fn standard_deviation_optional_long(self) -> Self {
        self.standard_deviation_optional_long_as("stdDev_optional_long")
    }

    pub fn standard_deviation_optional_long_as(self, alias: impl Into<String>) -> Self {
        self.aggregate_stddev("optional_long", alias)
    }

    pub fn square_root_of_population_standard_deviation_optional_long(self) -> Self {
        self.square_root_of_population_standard_deviation_optional_long_as("stdDevPop_optional_long")
    }

    pub fn square_root_of_population_standard_deviation_optional_long_as(self, alias: impl Into<String>) -> Self {
        self.aggregate_stddev_pop("optional_long", alias)
    }

    pub fn sample_variance_optional_long(self) -> Self {
        self.sample_variance_optional_long_as("varSamp_optional_long")
    }

    pub fn sample_variance_optional_long_as(self, alias: impl Into<String>) -> Self {
        self.aggregate_var_samp("optional_long", alias)
    }

    pub fn sample_population_variance_optional_long(self) -> Self {
        self.sample_population_variance_optional_long_as("varPop_optional_long")
    }

    pub fn sample_population_variance_optional_long_as(self, alias: impl Into<String>) -> Self {
        self.aggregate_var_pop("optional_long", alias)
    }

    pub fn unselect_optional_long(mut self) -> Self {
        self.query.projection.retain(|field| field != "optional_long");
        self.query_options.raw_projections.retain(|projection| projection.property_name != "optional_long");
        self
    }


    pub fn with_optional_long(
        mut self,
        operator: FieldOperator,
        values: impl IntoIterator<Item = impl Into<teaql_core::Value>>,
    ) -> Self {
        self.query = self.query.and_filter(field_operator_expr(
            "optional_long",
            operator,
            values.into_iter().map(Into::into).collect(),
        ));
        self
    }

    pub fn create_optional_long_criteria(
        operator: FieldOperator,
        values: impl IntoIterator<Item = impl Into<teaql_core::Value>>,
    ) -> Expr {
        field_operator_expr(
            "optional_long",
            operator,
            values.into_iter().map(Into::into).collect(),
        )
    }

    pub fn with_optional_long_is(mut self, value: impl Into<teaql_core::Value>) -> Self {
        self.query = self.query.and_filter(Expr::eq("optional_long", value));
        self
    }



    pub fn with_optional_long_is_not(mut self, value: impl Into<teaql_core::Value>) -> Self {
        self.query = self.query.and_filter(Expr::ne("optional_long", value));
        self
    }

    pub fn with_optional_long_greater_than(mut self, value: impl Into<teaql_core::Value>) -> Self {
        self.query = self.query.and_filter(Expr::gt("optional_long", value));
        self
    }

    pub fn with_optional_long_greater_than_or_equal_to(mut self, value: impl Into<teaql_core::Value>) -> Self {
        self.query = self.query.and_filter(Expr::gte("optional_long", value));
        self
    }

    pub fn with_optional_long_less_than(mut self, value: impl Into<teaql_core::Value>) -> Self {
        self.query = self.query.and_filter(Expr::lt("optional_long", value));
        self
    }

    pub fn with_optional_long_less_than_or_equal_to(mut self, value: impl Into<teaql_core::Value>) -> Self {
        self.query = self.query.and_filter(Expr::lte("optional_long", value));
        self
    }

    pub fn with_optional_long_between(
        mut self,
        lower: impl Into<teaql_core::Value>,
        upper: impl Into<teaql_core::Value>,
    ) -> Self {
        self.query = self.query.and_filter(Expr::between("optional_long", lower, upper));
        self
    }

    pub fn with_optional_long_between_range<T>(mut self, range: DateRange<T>) -> Self
    where
        T: Into<teaql_core::Value>,
    {
        self.query = self.query.and_filter(Expr::between(
            "optional_long",
            range.start,
            range.end,
        ));
        self
    }

    pub fn with_optional_long_in(
        mut self,
        values: impl IntoIterator<Item = impl Into<teaql_core::Value>>,
    ) -> Self {
        self.query = self.query.and_filter(Expr::in_list(
            "optional_long",
            values.into_iter().map(Into::into),
        ));
        self
    }

    pub fn with_optional_long_not_in(
        mut self,
        values: impl IntoIterator<Item = impl Into<teaql_core::Value>>,
    ) -> Self {
        self.query = self.query.and_filter(Expr::not_in_list(
            "optional_long",
            values.into_iter().map(Into::into),
        ));
        self
    }

    pub fn with_optional_long_before(mut self, value: impl Into<teaql_core::Value>) -> Self {
        self.query = self.query.and_filter(Expr::lt("optional_long", value));
        self
    }

    pub fn with_optional_long_after(mut self, value: impl Into<teaql_core::Value>) -> Self {
        self.query = self.query.and_filter(Expr::gt("optional_long", value));
        self
    }

    pub fn with_optional_long_is_unknown(mut self) -> Self {
        self.query = self.query.and_filter(Expr::is_null("optional_long"));
        self
    }



    pub fn with_optional_long_is_known(mut self) -> Self {
        self.query = self.query.and_filter(Expr::is_not_null("optional_long"));
        self
    }


    pub fn order_by_optional_long_asc(mut self) -> Self {
        self.query = self.query.order_asc("optional_long");
        self
    }

    pub fn order_by_optional_long_desc(mut self) -> Self {
        self.query = self.query.order_desc("optional_long");
        self
    }

    pub fn order_by_optional_long_asc_using_gbk(mut self) -> Self {
        self.query = self.query.order_gbk_asc("optional_long");
        self
    }

    pub fn order_by_optional_long_desc_using_gbk(mut self) -> Self {
        self.query = self.query.order_gbk_desc("optional_long");
        self
    }


    pub fn select_decimal(mut self) -> Self {
        self.query = self.query.project("decimal");
        self
    }

    pub fn project_decimal(self) -> Self {
        self.select_decimal()
    }

    pub fn select_decimal_raw(self, raw_sql_segment: impl Into<String>) -> Self {
        self.select_decimal_unsafe_raw(UnsafeRawSqlSegment::trusted(raw_sql_segment))
    }

    pub fn select_decimal_unsafe_raw(mut self, raw_sql_segment: UnsafeRawSqlSegment) -> Self {
        self.query_options
            .raw_projections
            .push(RawProjection::new("decimal", raw_sql_segment));
        self
    }

    pub fn select_decimal_with_function(self, function: AggregateFunction) -> Self {
        self.select_decimal_as_with_function("decimal", function)
    }

    pub fn select_decimal_as_with_function(
        self,
        alias: impl Into<String>,
        function: AggregateFunction,
    ) -> Self {
        self.aggregate_with_function("decimal", alias, function)
    }

    pub fn group_by_decimal(self) -> Self {
        self.group_by("decimal")
    }

    pub fn group_by_decimal_as(self, alias: impl Into<String>) -> Self {
        let alias = alias.into();
        let mut request = self.group_by("decimal");
        request.query = request
            .query
            .project_expr(alias, Expr::column("decimal"));
        request
    }

    pub fn group_by_decimal_with_function(
        self,
        alias: impl Into<String>,
        function: AggregateFunction,
    ) -> Self {
        self.group_by("decimal")
            .aggregate_with_function("decimal", alias, function)
    }

    pub fn count_decimal(self) -> Self {
        self.count_decimal_as("decimal_count")
    }

    pub fn count_decimal_as(self, alias: impl Into<String>) -> Self {
        self.aggregate_count_field("decimal", alias)
    }

    pub fn sum_decimal(self) -> Self {
        self.sum_decimal_as("sum_decimal")
    }

    pub fn sum_decimal_as(self, alias: impl Into<String>) -> Self {
        self.aggregate_sum("decimal", alias)
    }

    pub fn avg_decimal(self) -> Self {
        self.avg_decimal_as("avg_decimal")
    }

    pub fn avg_decimal_as(self, alias: impl Into<String>) -> Self {
        self.aggregate_avg("decimal", alias)
    }

    pub fn min_decimal(self) -> Self {
        self.min_decimal_as("min_decimal")
    }

    pub fn min_decimal_as(self, alias: impl Into<String>) -> Self {
        self.aggregate_min("decimal", alias)
    }

    pub fn max_decimal(self) -> Self {
        self.max_decimal_as("max_decimal")
    }

    pub fn max_decimal_as(self, alias: impl Into<String>) -> Self {
        self.aggregate_max("decimal", alias)
    }

    pub fn standard_deviation_decimal(self) -> Self {
        self.standard_deviation_decimal_as("stdDev_decimal")
    }

    pub fn standard_deviation_decimal_as(self, alias: impl Into<String>) -> Self {
        self.aggregate_stddev("decimal", alias)
    }

    pub fn square_root_of_population_standard_deviation_decimal(self) -> Self {
        self.square_root_of_population_standard_deviation_decimal_as("stdDevPop_decimal")
    }

    pub fn square_root_of_population_standard_deviation_decimal_as(self, alias: impl Into<String>) -> Self {
        self.aggregate_stddev_pop("decimal", alias)
    }

    pub fn sample_variance_decimal(self) -> Self {
        self.sample_variance_decimal_as("varSamp_decimal")
    }

    pub fn sample_variance_decimal_as(self, alias: impl Into<String>) -> Self {
        self.aggregate_var_samp("decimal", alias)
    }

    pub fn sample_population_variance_decimal(self) -> Self {
        self.sample_population_variance_decimal_as("varPop_decimal")
    }

    pub fn sample_population_variance_decimal_as(self, alias: impl Into<String>) -> Self {
        self.aggregate_var_pop("decimal", alias)
    }

    pub fn unselect_decimal(mut self) -> Self {
        self.query.projection.retain(|field| field != "decimal");
        self.query_options.raw_projections.retain(|projection| projection.property_name != "decimal");
        self
    }


    pub fn with_decimal(
        mut self,
        operator: FieldOperator,
        values: impl IntoIterator<Item = impl Into<teaql_core::Value>>,
    ) -> Self {
        self.query = self.query.and_filter(field_operator_expr(
            "decimal",
            operator,
            values.into_iter().map(Into::into).collect(),
        ));
        self
    }

    pub fn create_decimal_criteria(
        operator: FieldOperator,
        values: impl IntoIterator<Item = impl Into<teaql_core::Value>>,
    ) -> Expr {
        field_operator_expr(
            "decimal",
            operator,
            values.into_iter().map(Into::into).collect(),
        )
    }

    pub fn with_decimal_is(mut self, value: impl Into<teaql_core::Value>) -> Self {
        self.query = self.query.and_filter(Expr::eq("decimal", value));
        self
    }



    pub fn with_decimal_is_not(mut self, value: impl Into<teaql_core::Value>) -> Self {
        self.query = self.query.and_filter(Expr::ne("decimal", value));
        self
    }

    pub fn with_decimal_greater_than(mut self, value: impl Into<teaql_core::Value>) -> Self {
        self.query = self.query.and_filter(Expr::gt("decimal", value));
        self
    }

    pub fn with_decimal_greater_than_or_equal_to(mut self, value: impl Into<teaql_core::Value>) -> Self {
        self.query = self.query.and_filter(Expr::gte("decimal", value));
        self
    }

    pub fn with_decimal_less_than(mut self, value: impl Into<teaql_core::Value>) -> Self {
        self.query = self.query.and_filter(Expr::lt("decimal", value));
        self
    }

    pub fn with_decimal_less_than_or_equal_to(mut self, value: impl Into<teaql_core::Value>) -> Self {
        self.query = self.query.and_filter(Expr::lte("decimal", value));
        self
    }

    pub fn with_decimal_between(
        mut self,
        lower: impl Into<teaql_core::Value>,
        upper: impl Into<teaql_core::Value>,
    ) -> Self {
        self.query = self.query.and_filter(Expr::between("decimal", lower, upper));
        self
    }

    pub fn with_decimal_between_range<T>(mut self, range: DateRange<T>) -> Self
    where
        T: Into<teaql_core::Value>,
    {
        self.query = self.query.and_filter(Expr::between(
            "decimal",
            range.start,
            range.end,
        ));
        self
    }

    pub fn with_decimal_in(
        mut self,
        values: impl IntoIterator<Item = impl Into<teaql_core::Value>>,
    ) -> Self {
        self.query = self.query.and_filter(Expr::in_list(
            "decimal",
            values.into_iter().map(Into::into),
        ));
        self
    }

    pub fn with_decimal_not_in(
        mut self,
        values: impl IntoIterator<Item = impl Into<teaql_core::Value>>,
    ) -> Self {
        self.query = self.query.and_filter(Expr::not_in_list(
            "decimal",
            values.into_iter().map(Into::into),
        ));
        self
    }

    pub fn with_decimal_before(mut self, value: impl Into<teaql_core::Value>) -> Self {
        self.query = self.query.and_filter(Expr::lt("decimal", value));
        self
    }

    pub fn with_decimal_after(mut self, value: impl Into<teaql_core::Value>) -> Self {
        self.query = self.query.and_filter(Expr::gt("decimal", value));
        self
    }

    pub fn with_decimal_is_unknown(mut self) -> Self {
        self.query = self.query.and_filter(Expr::is_null("decimal"));
        self
    }



    pub fn with_decimal_is_known(mut self) -> Self {
        self.query = self.query.and_filter(Expr::is_not_null("decimal"));
        self
    }


    pub fn order_by_decimal_asc(mut self) -> Self {
        self.query = self.query.order_asc("decimal");
        self
    }

    pub fn order_by_decimal_desc(mut self) -> Self {
        self.query = self.query.order_desc("decimal");
        self
    }

    pub fn order_by_decimal_asc_using_gbk(mut self) -> Self {
        self.query = self.query.order_gbk_asc("decimal");
        self
    }

    pub fn order_by_decimal_desc_using_gbk(mut self) -> Self {
        self.query = self.query.order_gbk_desc("decimal");
        self
    }


    pub fn select_optional_decimal(mut self) -> Self {
        self.query = self.query.project("optional_decimal");
        self
    }

    pub fn project_optional_decimal(self) -> Self {
        self.select_optional_decimal()
    }

    pub fn select_optional_decimal_raw(self, raw_sql_segment: impl Into<String>) -> Self {
        self.select_optional_decimal_unsafe_raw(UnsafeRawSqlSegment::trusted(raw_sql_segment))
    }

    pub fn select_optional_decimal_unsafe_raw(mut self, raw_sql_segment: UnsafeRawSqlSegment) -> Self {
        self.query_options
            .raw_projections
            .push(RawProjection::new("optional_decimal", raw_sql_segment));
        self
    }

    pub fn select_optional_decimal_with_function(self, function: AggregateFunction) -> Self {
        self.select_optional_decimal_as_with_function("optional_decimal", function)
    }

    pub fn select_optional_decimal_as_with_function(
        self,
        alias: impl Into<String>,
        function: AggregateFunction,
    ) -> Self {
        self.aggregate_with_function("optional_decimal", alias, function)
    }

    pub fn group_by_optional_decimal(self) -> Self {
        self.group_by("optional_decimal")
    }

    pub fn group_by_optional_decimal_as(self, alias: impl Into<String>) -> Self {
        let alias = alias.into();
        let mut request = self.group_by("optional_decimal");
        request.query = request
            .query
            .project_expr(alias, Expr::column("optional_decimal"));
        request
    }

    pub fn group_by_optional_decimal_with_function(
        self,
        alias: impl Into<String>,
        function: AggregateFunction,
    ) -> Self {
        self.group_by("optional_decimal")
            .aggregate_with_function("optional_decimal", alias, function)
    }

    pub fn count_optional_decimal(self) -> Self {
        self.count_optional_decimal_as("optional_decimal_count")
    }

    pub fn count_optional_decimal_as(self, alias: impl Into<String>) -> Self {
        self.aggregate_count_field("optional_decimal", alias)
    }

    pub fn sum_optional_decimal(self) -> Self {
        self.sum_optional_decimal_as("sum_optional_decimal")
    }

    pub fn sum_optional_decimal_as(self, alias: impl Into<String>) -> Self {
        self.aggregate_sum("optional_decimal", alias)
    }

    pub fn avg_optional_decimal(self) -> Self {
        self.avg_optional_decimal_as("avg_optional_decimal")
    }

    pub fn avg_optional_decimal_as(self, alias: impl Into<String>) -> Self {
        self.aggregate_avg("optional_decimal", alias)
    }

    pub fn min_optional_decimal(self) -> Self {
        self.min_optional_decimal_as("min_optional_decimal")
    }

    pub fn min_optional_decimal_as(self, alias: impl Into<String>) -> Self {
        self.aggregate_min("optional_decimal", alias)
    }

    pub fn max_optional_decimal(self) -> Self {
        self.max_optional_decimal_as("max_optional_decimal")
    }

    pub fn max_optional_decimal_as(self, alias: impl Into<String>) -> Self {
        self.aggregate_max("optional_decimal", alias)
    }

    pub fn standard_deviation_optional_decimal(self) -> Self {
        self.standard_deviation_optional_decimal_as("stdDev_optional_decimal")
    }

    pub fn standard_deviation_optional_decimal_as(self, alias: impl Into<String>) -> Self {
        self.aggregate_stddev("optional_decimal", alias)
    }

    pub fn square_root_of_population_standard_deviation_optional_decimal(self) -> Self {
        self.square_root_of_population_standard_deviation_optional_decimal_as("stdDevPop_optional_decimal")
    }

    pub fn square_root_of_population_standard_deviation_optional_decimal_as(self, alias: impl Into<String>) -> Self {
        self.aggregate_stddev_pop("optional_decimal", alias)
    }

    pub fn sample_variance_optional_decimal(self) -> Self {
        self.sample_variance_optional_decimal_as("varSamp_optional_decimal")
    }

    pub fn sample_variance_optional_decimal_as(self, alias: impl Into<String>) -> Self {
        self.aggregate_var_samp("optional_decimal", alias)
    }

    pub fn sample_population_variance_optional_decimal(self) -> Self {
        self.sample_population_variance_optional_decimal_as("varPop_optional_decimal")
    }

    pub fn sample_population_variance_optional_decimal_as(self, alias: impl Into<String>) -> Self {
        self.aggregate_var_pop("optional_decimal", alias)
    }

    pub fn unselect_optional_decimal(mut self) -> Self {
        self.query.projection.retain(|field| field != "optional_decimal");
        self.query_options.raw_projections.retain(|projection| projection.property_name != "optional_decimal");
        self
    }


    pub fn with_optional_decimal(
        mut self,
        operator: FieldOperator,
        values: impl IntoIterator<Item = impl Into<teaql_core::Value>>,
    ) -> Self {
        self.query = self.query.and_filter(field_operator_expr(
            "optional_decimal",
            operator,
            values.into_iter().map(Into::into).collect(),
        ));
        self
    }

    pub fn create_optional_decimal_criteria(
        operator: FieldOperator,
        values: impl IntoIterator<Item = impl Into<teaql_core::Value>>,
    ) -> Expr {
        field_operator_expr(
            "optional_decimal",
            operator,
            values.into_iter().map(Into::into).collect(),
        )
    }

    pub fn with_optional_decimal_is(mut self, value: impl Into<teaql_core::Value>) -> Self {
        self.query = self.query.and_filter(Expr::eq("optional_decimal", value));
        self
    }



    pub fn with_optional_decimal_is_not(mut self, value: impl Into<teaql_core::Value>) -> Self {
        self.query = self.query.and_filter(Expr::ne("optional_decimal", value));
        self
    }

    pub fn with_optional_decimal_greater_than(mut self, value: impl Into<teaql_core::Value>) -> Self {
        self.query = self.query.and_filter(Expr::gt("optional_decimal", value));
        self
    }

    pub fn with_optional_decimal_greater_than_or_equal_to(mut self, value: impl Into<teaql_core::Value>) -> Self {
        self.query = self.query.and_filter(Expr::gte("optional_decimal", value));
        self
    }

    pub fn with_optional_decimal_less_than(mut self, value: impl Into<teaql_core::Value>) -> Self {
        self.query = self.query.and_filter(Expr::lt("optional_decimal", value));
        self
    }

    pub fn with_optional_decimal_less_than_or_equal_to(mut self, value: impl Into<teaql_core::Value>) -> Self {
        self.query = self.query.and_filter(Expr::lte("optional_decimal", value));
        self
    }

    pub fn with_optional_decimal_between(
        mut self,
        lower: impl Into<teaql_core::Value>,
        upper: impl Into<teaql_core::Value>,
    ) -> Self {
        self.query = self.query.and_filter(Expr::between("optional_decimal", lower, upper));
        self
    }

    pub fn with_optional_decimal_between_range<T>(mut self, range: DateRange<T>) -> Self
    where
        T: Into<teaql_core::Value>,
    {
        self.query = self.query.and_filter(Expr::between(
            "optional_decimal",
            range.start,
            range.end,
        ));
        self
    }

    pub fn with_optional_decimal_in(
        mut self,
        values: impl IntoIterator<Item = impl Into<teaql_core::Value>>,
    ) -> Self {
        self.query = self.query.and_filter(Expr::in_list(
            "optional_decimal",
            values.into_iter().map(Into::into),
        ));
        self
    }

    pub fn with_optional_decimal_not_in(
        mut self,
        values: impl IntoIterator<Item = impl Into<teaql_core::Value>>,
    ) -> Self {
        self.query = self.query.and_filter(Expr::not_in_list(
            "optional_decimal",
            values.into_iter().map(Into::into),
        ));
        self
    }

    pub fn with_optional_decimal_before(mut self, value: impl Into<teaql_core::Value>) -> Self {
        self.query = self.query.and_filter(Expr::lt("optional_decimal", value));
        self
    }

    pub fn with_optional_decimal_after(mut self, value: impl Into<teaql_core::Value>) -> Self {
        self.query = self.query.and_filter(Expr::gt("optional_decimal", value));
        self
    }

    pub fn with_optional_decimal_is_unknown(mut self) -> Self {
        self.query = self.query.and_filter(Expr::is_null("optional_decimal"));
        self
    }



    pub fn with_optional_decimal_is_known(mut self) -> Self {
        self.query = self.query.and_filter(Expr::is_not_null("optional_decimal"));
        self
    }


    pub fn order_by_optional_decimal_asc(mut self) -> Self {
        self.query = self.query.order_asc("optional_decimal");
        self
    }

    pub fn order_by_optional_decimal_desc(mut self) -> Self {
        self.query = self.query.order_desc("optional_decimal");
        self
    }

    pub fn order_by_optional_decimal_asc_using_gbk(mut self) -> Self {
        self.query = self.query.order_gbk_asc("optional_decimal");
        self
    }

    pub fn order_by_optional_decimal_desc_using_gbk(mut self) -> Self {
        self.query = self.query.order_gbk_desc("optional_decimal");
        self
    }


    pub fn select_float(mut self) -> Self {
        self.query = self.query.project("float");
        self
    }

    pub fn project_float(self) -> Self {
        self.select_float()
    }

    pub fn select_float_raw(self, raw_sql_segment: impl Into<String>) -> Self {
        self.select_float_unsafe_raw(UnsafeRawSqlSegment::trusted(raw_sql_segment))
    }

    pub fn select_float_unsafe_raw(mut self, raw_sql_segment: UnsafeRawSqlSegment) -> Self {
        self.query_options
            .raw_projections
            .push(RawProjection::new("float", raw_sql_segment));
        self
    }

    pub fn select_float_with_function(self, function: AggregateFunction) -> Self {
        self.select_float_as_with_function("float", function)
    }

    pub fn select_float_as_with_function(
        self,
        alias: impl Into<String>,
        function: AggregateFunction,
    ) -> Self {
        self.aggregate_with_function("float", alias, function)
    }

    pub fn group_by_float(self) -> Self {
        self.group_by("float")
    }

    pub fn group_by_float_as(self, alias: impl Into<String>) -> Self {
        let alias = alias.into();
        let mut request = self.group_by("float");
        request.query = request
            .query
            .project_expr(alias, Expr::column("float"));
        request
    }

    pub fn group_by_float_with_function(
        self,
        alias: impl Into<String>,
        function: AggregateFunction,
    ) -> Self {
        self.group_by("float")
            .aggregate_with_function("float", alias, function)
    }

    pub fn count_float(self) -> Self {
        self.count_float_as("float_count")
    }

    pub fn count_float_as(self, alias: impl Into<String>) -> Self {
        self.aggregate_count_field("float", alias)
    }

    pub fn sum_float(self) -> Self {
        self.sum_float_as("sum_float")
    }

    pub fn sum_float_as(self, alias: impl Into<String>) -> Self {
        self.aggregate_sum("float", alias)
    }

    pub fn avg_float(self) -> Self {
        self.avg_float_as("avg_float")
    }

    pub fn avg_float_as(self, alias: impl Into<String>) -> Self {
        self.aggregate_avg("float", alias)
    }

    pub fn min_float(self) -> Self {
        self.min_float_as("min_float")
    }

    pub fn min_float_as(self, alias: impl Into<String>) -> Self {
        self.aggregate_min("float", alias)
    }

    pub fn max_float(self) -> Self {
        self.max_float_as("max_float")
    }

    pub fn max_float_as(self, alias: impl Into<String>) -> Self {
        self.aggregate_max("float", alias)
    }

    pub fn standard_deviation_float(self) -> Self {
        self.standard_deviation_float_as("stdDev_float")
    }

    pub fn standard_deviation_float_as(self, alias: impl Into<String>) -> Self {
        self.aggregate_stddev("float", alias)
    }

    pub fn square_root_of_population_standard_deviation_float(self) -> Self {
        self.square_root_of_population_standard_deviation_float_as("stdDevPop_float")
    }

    pub fn square_root_of_population_standard_deviation_float_as(self, alias: impl Into<String>) -> Self {
        self.aggregate_stddev_pop("float", alias)
    }

    pub fn sample_variance_float(self) -> Self {
        self.sample_variance_float_as("varSamp_float")
    }

    pub fn sample_variance_float_as(self, alias: impl Into<String>) -> Self {
        self.aggregate_var_samp("float", alias)
    }

    pub fn sample_population_variance_float(self) -> Self {
        self.sample_population_variance_float_as("varPop_float")
    }

    pub fn sample_population_variance_float_as(self, alias: impl Into<String>) -> Self {
        self.aggregate_var_pop("float", alias)
    }

    pub fn unselect_float(mut self) -> Self {
        self.query.projection.retain(|field| field != "float");
        self.query_options.raw_projections.retain(|projection| projection.property_name != "float");
        self
    }


    pub fn with_float(
        mut self,
        operator: FieldOperator,
        values: impl IntoIterator<Item = impl Into<teaql_core::Value>>,
    ) -> Self {
        self.query = self.query.and_filter(field_operator_expr(
            "float",
            operator,
            values.into_iter().map(Into::into).collect(),
        ));
        self
    }

    pub fn create_float_criteria(
        operator: FieldOperator,
        values: impl IntoIterator<Item = impl Into<teaql_core::Value>>,
    ) -> Expr {
        field_operator_expr(
            "float",
            operator,
            values.into_iter().map(Into::into).collect(),
        )
    }

    pub fn with_float_is(mut self, value: impl Into<teaql_core::Value>) -> Self {
        self.query = self.query.and_filter(Expr::eq("float", value));
        self
    }



    pub fn with_float_is_not(mut self, value: impl Into<teaql_core::Value>) -> Self {
        self.query = self.query.and_filter(Expr::ne("float", value));
        self
    }

    pub fn with_float_greater_than(mut self, value: impl Into<teaql_core::Value>) -> Self {
        self.query = self.query.and_filter(Expr::gt("float", value));
        self
    }

    pub fn with_float_greater_than_or_equal_to(mut self, value: impl Into<teaql_core::Value>) -> Self {
        self.query = self.query.and_filter(Expr::gte("float", value));
        self
    }

    pub fn with_float_less_than(mut self, value: impl Into<teaql_core::Value>) -> Self {
        self.query = self.query.and_filter(Expr::lt("float", value));
        self
    }

    pub fn with_float_less_than_or_equal_to(mut self, value: impl Into<teaql_core::Value>) -> Self {
        self.query = self.query.and_filter(Expr::lte("float", value));
        self
    }

    pub fn with_float_between(
        mut self,
        lower: impl Into<teaql_core::Value>,
        upper: impl Into<teaql_core::Value>,
    ) -> Self {
        self.query = self.query.and_filter(Expr::between("float", lower, upper));
        self
    }

    pub fn with_float_between_range<T>(mut self, range: DateRange<T>) -> Self
    where
        T: Into<teaql_core::Value>,
    {
        self.query = self.query.and_filter(Expr::between(
            "float",
            range.start,
            range.end,
        ));
        self
    }

    pub fn with_float_in(
        mut self,
        values: impl IntoIterator<Item = impl Into<teaql_core::Value>>,
    ) -> Self {
        self.query = self.query.and_filter(Expr::in_list(
            "float",
            values.into_iter().map(Into::into),
        ));
        self
    }

    pub fn with_float_not_in(
        mut self,
        values: impl IntoIterator<Item = impl Into<teaql_core::Value>>,
    ) -> Self {
        self.query = self.query.and_filter(Expr::not_in_list(
            "float",
            values.into_iter().map(Into::into),
        ));
        self
    }

    pub fn with_float_before(mut self, value: impl Into<teaql_core::Value>) -> Self {
        self.query = self.query.and_filter(Expr::lt("float", value));
        self
    }

    pub fn with_float_after(mut self, value: impl Into<teaql_core::Value>) -> Self {
        self.query = self.query.and_filter(Expr::gt("float", value));
        self
    }

    pub fn with_float_is_unknown(mut self) -> Self {
        self.query = self.query.and_filter(Expr::is_null("float"));
        self
    }



    pub fn with_float_is_known(mut self) -> Self {
        self.query = self.query.and_filter(Expr::is_not_null("float"));
        self
    }


    pub fn order_by_float_asc(mut self) -> Self {
        self.query = self.query.order_asc("float");
        self
    }

    pub fn order_by_float_desc(mut self) -> Self {
        self.query = self.query.order_desc("float");
        self
    }

    pub fn order_by_float_asc_using_gbk(mut self) -> Self {
        self.query = self.query.order_gbk_asc("float");
        self
    }

    pub fn order_by_float_desc_using_gbk(mut self) -> Self {
        self.query = self.query.order_gbk_desc("float");
        self
    }


    pub fn select_optional_float(mut self) -> Self {
        self.query = self.query.project("optional_float");
        self
    }

    pub fn project_optional_float(self) -> Self {
        self.select_optional_float()
    }

    pub fn select_optional_float_raw(self, raw_sql_segment: impl Into<String>) -> Self {
        self.select_optional_float_unsafe_raw(UnsafeRawSqlSegment::trusted(raw_sql_segment))
    }

    pub fn select_optional_float_unsafe_raw(mut self, raw_sql_segment: UnsafeRawSqlSegment) -> Self {
        self.query_options
            .raw_projections
            .push(RawProjection::new("optional_float", raw_sql_segment));
        self
    }

    pub fn select_optional_float_with_function(self, function: AggregateFunction) -> Self {
        self.select_optional_float_as_with_function("optional_float", function)
    }

    pub fn select_optional_float_as_with_function(
        self,
        alias: impl Into<String>,
        function: AggregateFunction,
    ) -> Self {
        self.aggregate_with_function("optional_float", alias, function)
    }

    pub fn group_by_optional_float(self) -> Self {
        self.group_by("optional_float")
    }

    pub fn group_by_optional_float_as(self, alias: impl Into<String>) -> Self {
        let alias = alias.into();
        let mut request = self.group_by("optional_float");
        request.query = request
            .query
            .project_expr(alias, Expr::column("optional_float"));
        request
    }

    pub fn group_by_optional_float_with_function(
        self,
        alias: impl Into<String>,
        function: AggregateFunction,
    ) -> Self {
        self.group_by("optional_float")
            .aggregate_with_function("optional_float", alias, function)
    }

    pub fn count_optional_float(self) -> Self {
        self.count_optional_float_as("optional_float_count")
    }

    pub fn count_optional_float_as(self, alias: impl Into<String>) -> Self {
        self.aggregate_count_field("optional_float", alias)
    }

    pub fn sum_optional_float(self) -> Self {
        self.sum_optional_float_as("sum_optional_float")
    }

    pub fn sum_optional_float_as(self, alias: impl Into<String>) -> Self {
        self.aggregate_sum("optional_float", alias)
    }

    pub fn avg_optional_float(self) -> Self {
        self.avg_optional_float_as("avg_optional_float")
    }

    pub fn avg_optional_float_as(self, alias: impl Into<String>) -> Self {
        self.aggregate_avg("optional_float", alias)
    }

    pub fn min_optional_float(self) -> Self {
        self.min_optional_float_as("min_optional_float")
    }

    pub fn min_optional_float_as(self, alias: impl Into<String>) -> Self {
        self.aggregate_min("optional_float", alias)
    }

    pub fn max_optional_float(self) -> Self {
        self.max_optional_float_as("max_optional_float")
    }

    pub fn max_optional_float_as(self, alias: impl Into<String>) -> Self {
        self.aggregate_max("optional_float", alias)
    }

    pub fn standard_deviation_optional_float(self) -> Self {
        self.standard_deviation_optional_float_as("stdDev_optional_float")
    }

    pub fn standard_deviation_optional_float_as(self, alias: impl Into<String>) -> Self {
        self.aggregate_stddev("optional_float", alias)
    }

    pub fn square_root_of_population_standard_deviation_optional_float(self) -> Self {
        self.square_root_of_population_standard_deviation_optional_float_as("stdDevPop_optional_float")
    }

    pub fn square_root_of_population_standard_deviation_optional_float_as(self, alias: impl Into<String>) -> Self {
        self.aggregate_stddev_pop("optional_float", alias)
    }

    pub fn sample_variance_optional_float(self) -> Self {
        self.sample_variance_optional_float_as("varSamp_optional_float")
    }

    pub fn sample_variance_optional_float_as(self, alias: impl Into<String>) -> Self {
        self.aggregate_var_samp("optional_float", alias)
    }

    pub fn sample_population_variance_optional_float(self) -> Self {
        self.sample_population_variance_optional_float_as("varPop_optional_float")
    }

    pub fn sample_population_variance_optional_float_as(self, alias: impl Into<String>) -> Self {
        self.aggregate_var_pop("optional_float", alias)
    }

    pub fn unselect_optional_float(mut self) -> Self {
        self.query.projection.retain(|field| field != "optional_float");
        self.query_options.raw_projections.retain(|projection| projection.property_name != "optional_float");
        self
    }


    pub fn with_optional_float(
        mut self,
        operator: FieldOperator,
        values: impl IntoIterator<Item = impl Into<teaql_core::Value>>,
    ) -> Self {
        self.query = self.query.and_filter(field_operator_expr(
            "optional_float",
            operator,
            values.into_iter().map(Into::into).collect(),
        ));
        self
    }

    pub fn create_optional_float_criteria(
        operator: FieldOperator,
        values: impl IntoIterator<Item = impl Into<teaql_core::Value>>,
    ) -> Expr {
        field_operator_expr(
            "optional_float",
            operator,
            values.into_iter().map(Into::into).collect(),
        )
    }

    pub fn with_optional_float_is(mut self, value: impl Into<teaql_core::Value>) -> Self {
        self.query = self.query.and_filter(Expr::eq("optional_float", value));
        self
    }



    pub fn with_optional_float_is_not(mut self, value: impl Into<teaql_core::Value>) -> Self {
        self.query = self.query.and_filter(Expr::ne("optional_float", value));
        self
    }

    pub fn with_optional_float_greater_than(mut self, value: impl Into<teaql_core::Value>) -> Self {
        self.query = self.query.and_filter(Expr::gt("optional_float", value));
        self
    }

    pub fn with_optional_float_greater_than_or_equal_to(mut self, value: impl Into<teaql_core::Value>) -> Self {
        self.query = self.query.and_filter(Expr::gte("optional_float", value));
        self
    }

    pub fn with_optional_float_less_than(mut self, value: impl Into<teaql_core::Value>) -> Self {
        self.query = self.query.and_filter(Expr::lt("optional_float", value));
        self
    }

    pub fn with_optional_float_less_than_or_equal_to(mut self, value: impl Into<teaql_core::Value>) -> Self {
        self.query = self.query.and_filter(Expr::lte("optional_float", value));
        self
    }

    pub fn with_optional_float_between(
        mut self,
        lower: impl Into<teaql_core::Value>,
        upper: impl Into<teaql_core::Value>,
    ) -> Self {
        self.query = self.query.and_filter(Expr::between("optional_float", lower, upper));
        self
    }

    pub fn with_optional_float_between_range<T>(mut self, range: DateRange<T>) -> Self
    where
        T: Into<teaql_core::Value>,
    {
        self.query = self.query.and_filter(Expr::between(
            "optional_float",
            range.start,
            range.end,
        ));
        self
    }

    pub fn with_optional_float_in(
        mut self,
        values: impl IntoIterator<Item = impl Into<teaql_core::Value>>,
    ) -> Self {
        self.query = self.query.and_filter(Expr::in_list(
            "optional_float",
            values.into_iter().map(Into::into),
        ));
        self
    }

    pub fn with_optional_float_not_in(
        mut self,
        values: impl IntoIterator<Item = impl Into<teaql_core::Value>>,
    ) -> Self {
        self.query = self.query.and_filter(Expr::not_in_list(
            "optional_float",
            values.into_iter().map(Into::into),
        ));
        self
    }

    pub fn with_optional_float_before(mut self, value: impl Into<teaql_core::Value>) -> Self {
        self.query = self.query.and_filter(Expr::lt("optional_float", value));
        self
    }

    pub fn with_optional_float_after(mut self, value: impl Into<teaql_core::Value>) -> Self {
        self.query = self.query.and_filter(Expr::gt("optional_float", value));
        self
    }

    pub fn with_optional_float_is_unknown(mut self) -> Self {
        self.query = self.query.and_filter(Expr::is_null("optional_float"));
        self
    }



    pub fn with_optional_float_is_known(mut self) -> Self {
        self.query = self.query.and_filter(Expr::is_not_null("optional_float"));
        self
    }


    pub fn order_by_optional_float_asc(mut self) -> Self {
        self.query = self.query.order_asc("optional_float");
        self
    }

    pub fn order_by_optional_float_desc(mut self) -> Self {
        self.query = self.query.order_desc("optional_float");
        self
    }

    pub fn order_by_optional_float_asc_using_gbk(mut self) -> Self {
        self.query = self.query.order_gbk_asc("optional_float");
        self
    }

    pub fn order_by_optional_float_desc_using_gbk(mut self) -> Self {
        self.query = self.query.order_gbk_desc("optional_float");
        self
    }


    pub fn select_double(mut self) -> Self {
        self.query = self.query.project("double");
        self
    }

    pub fn project_double(self) -> Self {
        self.select_double()
    }

    pub fn select_double_raw(self, raw_sql_segment: impl Into<String>) -> Self {
        self.select_double_unsafe_raw(UnsafeRawSqlSegment::trusted(raw_sql_segment))
    }

    pub fn select_double_unsafe_raw(mut self, raw_sql_segment: UnsafeRawSqlSegment) -> Self {
        self.query_options
            .raw_projections
            .push(RawProjection::new("double", raw_sql_segment));
        self
    }

    pub fn select_double_with_function(self, function: AggregateFunction) -> Self {
        self.select_double_as_with_function("double", function)
    }

    pub fn select_double_as_with_function(
        self,
        alias: impl Into<String>,
        function: AggregateFunction,
    ) -> Self {
        self.aggregate_with_function("double", alias, function)
    }

    pub fn group_by_double(self) -> Self {
        self.group_by("double")
    }

    pub fn group_by_double_as(self, alias: impl Into<String>) -> Self {
        let alias = alias.into();
        let mut request = self.group_by("double");
        request.query = request
            .query
            .project_expr(alias, Expr::column("double"));
        request
    }

    pub fn group_by_double_with_function(
        self,
        alias: impl Into<String>,
        function: AggregateFunction,
    ) -> Self {
        self.group_by("double")
            .aggregate_with_function("double", alias, function)
    }

    pub fn count_double(self) -> Self {
        self.count_double_as("double_count")
    }

    pub fn count_double_as(self, alias: impl Into<String>) -> Self {
        self.aggregate_count_field("double", alias)
    }

    pub fn sum_double(self) -> Self {
        self.sum_double_as("sum_double")
    }

    pub fn sum_double_as(self, alias: impl Into<String>) -> Self {
        self.aggregate_sum("double", alias)
    }

    pub fn avg_double(self) -> Self {
        self.avg_double_as("avg_double")
    }

    pub fn avg_double_as(self, alias: impl Into<String>) -> Self {
        self.aggregate_avg("double", alias)
    }

    pub fn min_double(self) -> Self {
        self.min_double_as("min_double")
    }

    pub fn min_double_as(self, alias: impl Into<String>) -> Self {
        self.aggregate_min("double", alias)
    }

    pub fn max_double(self) -> Self {
        self.max_double_as("max_double")
    }

    pub fn max_double_as(self, alias: impl Into<String>) -> Self {
        self.aggregate_max("double", alias)
    }

    pub fn standard_deviation_double(self) -> Self {
        self.standard_deviation_double_as("stdDev_double")
    }

    pub fn standard_deviation_double_as(self, alias: impl Into<String>) -> Self {
        self.aggregate_stddev("double", alias)
    }

    pub fn square_root_of_population_standard_deviation_double(self) -> Self {
        self.square_root_of_population_standard_deviation_double_as("stdDevPop_double")
    }

    pub fn square_root_of_population_standard_deviation_double_as(self, alias: impl Into<String>) -> Self {
        self.aggregate_stddev_pop("double", alias)
    }

    pub fn sample_variance_double(self) -> Self {
        self.sample_variance_double_as("varSamp_double")
    }

    pub fn sample_variance_double_as(self, alias: impl Into<String>) -> Self {
        self.aggregate_var_samp("double", alias)
    }

    pub fn sample_population_variance_double(self) -> Self {
        self.sample_population_variance_double_as("varPop_double")
    }

    pub fn sample_population_variance_double_as(self, alias: impl Into<String>) -> Self {
        self.aggregate_var_pop("double", alias)
    }

    pub fn unselect_double(mut self) -> Self {
        self.query.projection.retain(|field| field != "double");
        self.query_options.raw_projections.retain(|projection| projection.property_name != "double");
        self
    }


    pub fn with_double(
        mut self,
        operator: FieldOperator,
        values: impl IntoIterator<Item = impl Into<teaql_core::Value>>,
    ) -> Self {
        self.query = self.query.and_filter(field_operator_expr(
            "double",
            operator,
            values.into_iter().map(Into::into).collect(),
        ));
        self
    }

    pub fn create_double_criteria(
        operator: FieldOperator,
        values: impl IntoIterator<Item = impl Into<teaql_core::Value>>,
    ) -> Expr {
        field_operator_expr(
            "double",
            operator,
            values.into_iter().map(Into::into).collect(),
        )
    }

    pub fn with_double_is(mut self, value: impl Into<teaql_core::Value>) -> Self {
        self.query = self.query.and_filter(Expr::eq("double", value));
        self
    }



    pub fn with_double_is_not(mut self, value: impl Into<teaql_core::Value>) -> Self {
        self.query = self.query.and_filter(Expr::ne("double", value));
        self
    }

    pub fn with_double_greater_than(mut self, value: impl Into<teaql_core::Value>) -> Self {
        self.query = self.query.and_filter(Expr::gt("double", value));
        self
    }

    pub fn with_double_greater_than_or_equal_to(mut self, value: impl Into<teaql_core::Value>) -> Self {
        self.query = self.query.and_filter(Expr::gte("double", value));
        self
    }

    pub fn with_double_less_than(mut self, value: impl Into<teaql_core::Value>) -> Self {
        self.query = self.query.and_filter(Expr::lt("double", value));
        self
    }

    pub fn with_double_less_than_or_equal_to(mut self, value: impl Into<teaql_core::Value>) -> Self {
        self.query = self.query.and_filter(Expr::lte("double", value));
        self
    }

    pub fn with_double_between(
        mut self,
        lower: impl Into<teaql_core::Value>,
        upper: impl Into<teaql_core::Value>,
    ) -> Self {
        self.query = self.query.and_filter(Expr::between("double", lower, upper));
        self
    }

    pub fn with_double_between_range<T>(mut self, range: DateRange<T>) -> Self
    where
        T: Into<teaql_core::Value>,
    {
        self.query = self.query.and_filter(Expr::between(
            "double",
            range.start,
            range.end,
        ));
        self
    }

    pub fn with_double_in(
        mut self,
        values: impl IntoIterator<Item = impl Into<teaql_core::Value>>,
    ) -> Self {
        self.query = self.query.and_filter(Expr::in_list(
            "double",
            values.into_iter().map(Into::into),
        ));
        self
    }

    pub fn with_double_not_in(
        mut self,
        values: impl IntoIterator<Item = impl Into<teaql_core::Value>>,
    ) -> Self {
        self.query = self.query.and_filter(Expr::not_in_list(
            "double",
            values.into_iter().map(Into::into),
        ));
        self
    }

    pub fn with_double_before(mut self, value: impl Into<teaql_core::Value>) -> Self {
        self.query = self.query.and_filter(Expr::lt("double", value));
        self
    }

    pub fn with_double_after(mut self, value: impl Into<teaql_core::Value>) -> Self {
        self.query = self.query.and_filter(Expr::gt("double", value));
        self
    }

    pub fn with_double_is_unknown(mut self) -> Self {
        self.query = self.query.and_filter(Expr::is_null("double"));
        self
    }



    pub fn with_double_is_known(mut self) -> Self {
        self.query = self.query.and_filter(Expr::is_not_null("double"));
        self
    }


    pub fn order_by_double_asc(mut self) -> Self {
        self.query = self.query.order_asc("double");
        self
    }

    pub fn order_by_double_desc(mut self) -> Self {
        self.query = self.query.order_desc("double");
        self
    }

    pub fn order_by_double_asc_using_gbk(mut self) -> Self {
        self.query = self.query.order_gbk_asc("double");
        self
    }

    pub fn order_by_double_desc_using_gbk(mut self) -> Self {
        self.query = self.query.order_gbk_desc("double");
        self
    }


    pub fn select_optional_double(mut self) -> Self {
        self.query = self.query.project("optional_double");
        self
    }

    pub fn project_optional_double(self) -> Self {
        self.select_optional_double()
    }

    pub fn select_optional_double_raw(self, raw_sql_segment: impl Into<String>) -> Self {
        self.select_optional_double_unsafe_raw(UnsafeRawSqlSegment::trusted(raw_sql_segment))
    }

    pub fn select_optional_double_unsafe_raw(mut self, raw_sql_segment: UnsafeRawSqlSegment) -> Self {
        self.query_options
            .raw_projections
            .push(RawProjection::new("optional_double", raw_sql_segment));
        self
    }

    pub fn select_optional_double_with_function(self, function: AggregateFunction) -> Self {
        self.select_optional_double_as_with_function("optional_double", function)
    }

    pub fn select_optional_double_as_with_function(
        self,
        alias: impl Into<String>,
        function: AggregateFunction,
    ) -> Self {
        self.aggregate_with_function("optional_double", alias, function)
    }

    pub fn group_by_optional_double(self) -> Self {
        self.group_by("optional_double")
    }

    pub fn group_by_optional_double_as(self, alias: impl Into<String>) -> Self {
        let alias = alias.into();
        let mut request = self.group_by("optional_double");
        request.query = request
            .query
            .project_expr(alias, Expr::column("optional_double"));
        request
    }

    pub fn group_by_optional_double_with_function(
        self,
        alias: impl Into<String>,
        function: AggregateFunction,
    ) -> Self {
        self.group_by("optional_double")
            .aggregate_with_function("optional_double", alias, function)
    }

    pub fn count_optional_double(self) -> Self {
        self.count_optional_double_as("optional_double_count")
    }

    pub fn count_optional_double_as(self, alias: impl Into<String>) -> Self {
        self.aggregate_count_field("optional_double", alias)
    }

    pub fn sum_optional_double(self) -> Self {
        self.sum_optional_double_as("sum_optional_double")
    }

    pub fn sum_optional_double_as(self, alias: impl Into<String>) -> Self {
        self.aggregate_sum("optional_double", alias)
    }

    pub fn avg_optional_double(self) -> Self {
        self.avg_optional_double_as("avg_optional_double")
    }

    pub fn avg_optional_double_as(self, alias: impl Into<String>) -> Self {
        self.aggregate_avg("optional_double", alias)
    }

    pub fn min_optional_double(self) -> Self {
        self.min_optional_double_as("min_optional_double")
    }

    pub fn min_optional_double_as(self, alias: impl Into<String>) -> Self {
        self.aggregate_min("optional_double", alias)
    }

    pub fn max_optional_double(self) -> Self {
        self.max_optional_double_as("max_optional_double")
    }

    pub fn max_optional_double_as(self, alias: impl Into<String>) -> Self {
        self.aggregate_max("optional_double", alias)
    }

    pub fn standard_deviation_optional_double(self) -> Self {
        self.standard_deviation_optional_double_as("stdDev_optional_double")
    }

    pub fn standard_deviation_optional_double_as(self, alias: impl Into<String>) -> Self {
        self.aggregate_stddev("optional_double", alias)
    }

    pub fn square_root_of_population_standard_deviation_optional_double(self) -> Self {
        self.square_root_of_population_standard_deviation_optional_double_as("stdDevPop_optional_double")
    }

    pub fn square_root_of_population_standard_deviation_optional_double_as(self, alias: impl Into<String>) -> Self {
        self.aggregate_stddev_pop("optional_double", alias)
    }

    pub fn sample_variance_optional_double(self) -> Self {
        self.sample_variance_optional_double_as("varSamp_optional_double")
    }

    pub fn sample_variance_optional_double_as(self, alias: impl Into<String>) -> Self {
        self.aggregate_var_samp("optional_double", alias)
    }

    pub fn sample_population_variance_optional_double(self) -> Self {
        self.sample_population_variance_optional_double_as("varPop_optional_double")
    }

    pub fn sample_population_variance_optional_double_as(self, alias: impl Into<String>) -> Self {
        self.aggregate_var_pop("optional_double", alias)
    }

    pub fn unselect_optional_double(mut self) -> Self {
        self.query.projection.retain(|field| field != "optional_double");
        self.query_options.raw_projections.retain(|projection| projection.property_name != "optional_double");
        self
    }


    pub fn with_optional_double(
        mut self,
        operator: FieldOperator,
        values: impl IntoIterator<Item = impl Into<teaql_core::Value>>,
    ) -> Self {
        self.query = self.query.and_filter(field_operator_expr(
            "optional_double",
            operator,
            values.into_iter().map(Into::into).collect(),
        ));
        self
    }

    pub fn create_optional_double_criteria(
        operator: FieldOperator,
        values: impl IntoIterator<Item = impl Into<teaql_core::Value>>,
    ) -> Expr {
        field_operator_expr(
            "optional_double",
            operator,
            values.into_iter().map(Into::into).collect(),
        )
    }

    pub fn with_optional_double_is(mut self, value: impl Into<teaql_core::Value>) -> Self {
        self.query = self.query.and_filter(Expr::eq("optional_double", value));
        self
    }



    pub fn with_optional_double_is_not(mut self, value: impl Into<teaql_core::Value>) -> Self {
        self.query = self.query.and_filter(Expr::ne("optional_double", value));
        self
    }

    pub fn with_optional_double_greater_than(mut self, value: impl Into<teaql_core::Value>) -> Self {
        self.query = self.query.and_filter(Expr::gt("optional_double", value));
        self
    }

    pub fn with_optional_double_greater_than_or_equal_to(mut self, value: impl Into<teaql_core::Value>) -> Self {
        self.query = self.query.and_filter(Expr::gte("optional_double", value));
        self
    }

    pub fn with_optional_double_less_than(mut self, value: impl Into<teaql_core::Value>) -> Self {
        self.query = self.query.and_filter(Expr::lt("optional_double", value));
        self
    }

    pub fn with_optional_double_less_than_or_equal_to(mut self, value: impl Into<teaql_core::Value>) -> Self {
        self.query = self.query.and_filter(Expr::lte("optional_double", value));
        self
    }

    pub fn with_optional_double_between(
        mut self,
        lower: impl Into<teaql_core::Value>,
        upper: impl Into<teaql_core::Value>,
    ) -> Self {
        self.query = self.query.and_filter(Expr::between("optional_double", lower, upper));
        self
    }

    pub fn with_optional_double_between_range<T>(mut self, range: DateRange<T>) -> Self
    where
        T: Into<teaql_core::Value>,
    {
        self.query = self.query.and_filter(Expr::between(
            "optional_double",
            range.start,
            range.end,
        ));
        self
    }

    pub fn with_optional_double_in(
        mut self,
        values: impl IntoIterator<Item = impl Into<teaql_core::Value>>,
    ) -> Self {
        self.query = self.query.and_filter(Expr::in_list(
            "optional_double",
            values.into_iter().map(Into::into),
        ));
        self
    }

    pub fn with_optional_double_not_in(
        mut self,
        values: impl IntoIterator<Item = impl Into<teaql_core::Value>>,
    ) -> Self {
        self.query = self.query.and_filter(Expr::not_in_list(
            "optional_double",
            values.into_iter().map(Into::into),
        ));
        self
    }

    pub fn with_optional_double_before(mut self, value: impl Into<teaql_core::Value>) -> Self {
        self.query = self.query.and_filter(Expr::lt("optional_double", value));
        self
    }

    pub fn with_optional_double_after(mut self, value: impl Into<teaql_core::Value>) -> Self {
        self.query = self.query.and_filter(Expr::gt("optional_double", value));
        self
    }

    pub fn with_optional_double_is_unknown(mut self) -> Self {
        self.query = self.query.and_filter(Expr::is_null("optional_double"));
        self
    }



    pub fn with_optional_double_is_known(mut self) -> Self {
        self.query = self.query.and_filter(Expr::is_not_null("optional_double"));
        self
    }


    pub fn order_by_optional_double_asc(mut self) -> Self {
        self.query = self.query.order_asc("optional_double");
        self
    }

    pub fn order_by_optional_double_desc(mut self) -> Self {
        self.query = self.query.order_desc("optional_double");
        self
    }

    pub fn order_by_optional_double_asc_using_gbk(mut self) -> Self {
        self.query = self.query.order_gbk_asc("optional_double");
        self
    }

    pub fn order_by_optional_double_desc_using_gbk(mut self) -> Self {
        self.query = self.query.order_gbk_desc("optional_double");
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


    pub fn select_date(mut self) -> Self {
        self.query = self.query.project("date");
        self
    }

    pub fn project_date(self) -> Self {
        self.select_date()
    }

    pub fn select_date_raw(self, raw_sql_segment: impl Into<String>) -> Self {
        self.select_date_unsafe_raw(UnsafeRawSqlSegment::trusted(raw_sql_segment))
    }

    pub fn select_date_unsafe_raw(mut self, raw_sql_segment: UnsafeRawSqlSegment) -> Self {
        self.query_options
            .raw_projections
            .push(RawProjection::new("date", raw_sql_segment));
        self
    }

    pub fn group_by_date(self) -> Self {
        self.group_by("date")
    }

    pub fn group_by_date_as(self, alias: impl Into<String>) -> Self {
        let alias = alias.into();
        let mut request = self.group_by("date");
        request.query = request
            .query
            .project_expr(alias, Expr::column("date"));
        request
    }

    pub fn group_by_date_with_function(
        self,
        alias: impl Into<String>,
        function: AggregateFunction,
    ) -> Self {
        self.group_by("date")
            .aggregate_with_function("date", alias, function)
    }

    pub fn count_date(self) -> Self {
        self.count_date_as("date_count")
    }

    pub fn count_date_as(self, alias: impl Into<String>) -> Self {
        self.aggregate_count_field("date", alias)
    }

    pub fn sum_date(self) -> Self {
        self.sum_date_as("sum_date")
    }

    pub fn sum_date_as(self, alias: impl Into<String>) -> Self {
        self.aggregate_sum("date", alias)
    }

    pub fn avg_date(self) -> Self {
        self.avg_date_as("avg_date")
    }

    pub fn avg_date_as(self, alias: impl Into<String>) -> Self {
        self.aggregate_avg("date", alias)
    }

    pub fn min_date(self) -> Self {
        self.min_date_as("min_date")
    }

    pub fn min_date_as(self, alias: impl Into<String>) -> Self {
        self.aggregate_min("date", alias)
    }

    pub fn max_date(self) -> Self {
        self.max_date_as("max_date")
    }

    pub fn max_date_as(self, alias: impl Into<String>) -> Self {
        self.aggregate_max("date", alias)
    }

    pub fn unselect_date(mut self) -> Self {
        self.query.projection.retain(|field| field != "date");
        self.query_options.raw_projections.retain(|projection| projection.property_name != "date");
        self
    }


    pub fn with_date(
        mut self,
        operator: FieldOperator,
        values: impl IntoIterator<Item = impl Into<teaql_core::Value>>,
    ) -> Self {
        self.query = self.query.and_filter(field_operator_expr(
            "date",
            operator,
            values.into_iter().map(Into::into).collect(),
        ));
        self
    }

    pub fn create_date_criteria(
        operator: FieldOperator,
        values: impl IntoIterator<Item = impl Into<teaql_core::Value>>,
    ) -> Expr {
        field_operator_expr(
            "date",
            operator,
            values.into_iter().map(Into::into).collect(),
        )
    }

    pub fn with_date_is(mut self, value: impl Into<teaql_core::Value>) -> Self {
        self.query = self.query.and_filter(Expr::eq("date", value));
        self
    }



    pub fn with_date_is_not(mut self, value: impl Into<teaql_core::Value>) -> Self {
        self.query = self.query.and_filter(Expr::ne("date", value));
        self
    }

    pub fn with_date_greater_than(mut self, value: impl Into<teaql_core::Value>) -> Self {
        self.query = self.query.and_filter(Expr::gt("date", value));
        self
    }

    pub fn with_date_greater_than_or_equal_to(mut self, value: impl Into<teaql_core::Value>) -> Self {
        self.query = self.query.and_filter(Expr::gte("date", value));
        self
    }

    pub fn with_date_less_than(mut self, value: impl Into<teaql_core::Value>) -> Self {
        self.query = self.query.and_filter(Expr::lt("date", value));
        self
    }

    pub fn with_date_less_than_or_equal_to(mut self, value: impl Into<teaql_core::Value>) -> Self {
        self.query = self.query.and_filter(Expr::lte("date", value));
        self
    }

    pub fn with_date_between(
        mut self,
        lower: impl Into<teaql_core::Value>,
        upper: impl Into<teaql_core::Value>,
    ) -> Self {
        self.query = self.query.and_filter(Expr::between("date", lower, upper));
        self
    }

    pub fn with_date_between_range<T>(mut self, range: DateRange<T>) -> Self
    where
        T: Into<teaql_core::Value>,
    {
        self.query = self.query.and_filter(Expr::between(
            "date",
            range.start,
            range.end,
        ));
        self
    }

    pub fn with_date_in(
        mut self,
        values: impl IntoIterator<Item = impl Into<teaql_core::Value>>,
    ) -> Self {
        self.query = self.query.and_filter(Expr::in_list(
            "date",
            values.into_iter().map(Into::into),
        ));
        self
    }

    pub fn with_date_not_in(
        mut self,
        values: impl IntoIterator<Item = impl Into<teaql_core::Value>>,
    ) -> Self {
        self.query = self.query.and_filter(Expr::not_in_list(
            "date",
            values.into_iter().map(Into::into),
        ));
        self
    }

    pub fn with_date_before(mut self, value: impl Into<teaql_core::Value>) -> Self {
        self.query = self.query.and_filter(Expr::lt("date", value));
        self
    }

    pub fn with_date_after(mut self, value: impl Into<teaql_core::Value>) -> Self {
        self.query = self.query.and_filter(Expr::gt("date", value));
        self
    }

    pub fn with_date_is_unknown(mut self) -> Self {
        self.query = self.query.and_filter(Expr::is_null("date"));
        self
    }



    pub fn with_date_is_known(mut self) -> Self {
        self.query = self.query.and_filter(Expr::is_not_null("date"));
        self
    }


    pub fn order_by_date_asc(mut self) -> Self {
        self.query = self.query.order_asc("date");
        self
    }

    pub fn order_by_date_desc(mut self) -> Self {
        self.query = self.query.order_desc("date");
        self
    }

    pub fn order_by_date_asc_using_gbk(mut self) -> Self {
        self.query = self.query.order_gbk_asc("date");
        self
    }

    pub fn order_by_date_desc_using_gbk(mut self) -> Self {
        self.query = self.query.order_gbk_desc("date");
        self
    }


    pub fn select_optional_date(mut self) -> Self {
        self.query = self.query.project("optional_date");
        self
    }

    pub fn project_optional_date(self) -> Self {
        self.select_optional_date()
    }

    pub fn select_optional_date_raw(self, raw_sql_segment: impl Into<String>) -> Self {
        self.select_optional_date_unsafe_raw(UnsafeRawSqlSegment::trusted(raw_sql_segment))
    }

    pub fn select_optional_date_unsafe_raw(mut self, raw_sql_segment: UnsafeRawSqlSegment) -> Self {
        self.query_options
            .raw_projections
            .push(RawProjection::new("optional_date", raw_sql_segment));
        self
    }

    pub fn group_by_optional_date(self) -> Self {
        self.group_by("optional_date")
    }

    pub fn group_by_optional_date_as(self, alias: impl Into<String>) -> Self {
        let alias = alias.into();
        let mut request = self.group_by("optional_date");
        request.query = request
            .query
            .project_expr(alias, Expr::column("optional_date"));
        request
    }

    pub fn group_by_optional_date_with_function(
        self,
        alias: impl Into<String>,
        function: AggregateFunction,
    ) -> Self {
        self.group_by("optional_date")
            .aggregate_with_function("optional_date", alias, function)
    }

    pub fn count_optional_date(self) -> Self {
        self.count_optional_date_as("optional_date_count")
    }

    pub fn count_optional_date_as(self, alias: impl Into<String>) -> Self {
        self.aggregate_count_field("optional_date", alias)
    }

    pub fn sum_optional_date(self) -> Self {
        self.sum_optional_date_as("sum_optional_date")
    }

    pub fn sum_optional_date_as(self, alias: impl Into<String>) -> Self {
        self.aggregate_sum("optional_date", alias)
    }

    pub fn avg_optional_date(self) -> Self {
        self.avg_optional_date_as("avg_optional_date")
    }

    pub fn avg_optional_date_as(self, alias: impl Into<String>) -> Self {
        self.aggregate_avg("optional_date", alias)
    }

    pub fn min_optional_date(self) -> Self {
        self.min_optional_date_as("min_optional_date")
    }

    pub fn min_optional_date_as(self, alias: impl Into<String>) -> Self {
        self.aggregate_min("optional_date", alias)
    }

    pub fn max_optional_date(self) -> Self {
        self.max_optional_date_as("max_optional_date")
    }

    pub fn max_optional_date_as(self, alias: impl Into<String>) -> Self {
        self.aggregate_max("optional_date", alias)
    }

    pub fn unselect_optional_date(mut self) -> Self {
        self.query.projection.retain(|field| field != "optional_date");
        self.query_options.raw_projections.retain(|projection| projection.property_name != "optional_date");
        self
    }


    pub fn with_optional_date(
        mut self,
        operator: FieldOperator,
        values: impl IntoIterator<Item = impl Into<teaql_core::Value>>,
    ) -> Self {
        self.query = self.query.and_filter(field_operator_expr(
            "optional_date",
            operator,
            values.into_iter().map(Into::into).collect(),
        ));
        self
    }

    pub fn create_optional_date_criteria(
        operator: FieldOperator,
        values: impl IntoIterator<Item = impl Into<teaql_core::Value>>,
    ) -> Expr {
        field_operator_expr(
            "optional_date",
            operator,
            values.into_iter().map(Into::into).collect(),
        )
    }

    pub fn with_optional_date_is(mut self, value: impl Into<teaql_core::Value>) -> Self {
        self.query = self.query.and_filter(Expr::eq("optional_date", value));
        self
    }



    pub fn with_optional_date_is_not(mut self, value: impl Into<teaql_core::Value>) -> Self {
        self.query = self.query.and_filter(Expr::ne("optional_date", value));
        self
    }

    pub fn with_optional_date_greater_than(mut self, value: impl Into<teaql_core::Value>) -> Self {
        self.query = self.query.and_filter(Expr::gt("optional_date", value));
        self
    }

    pub fn with_optional_date_greater_than_or_equal_to(mut self, value: impl Into<teaql_core::Value>) -> Self {
        self.query = self.query.and_filter(Expr::gte("optional_date", value));
        self
    }

    pub fn with_optional_date_less_than(mut self, value: impl Into<teaql_core::Value>) -> Self {
        self.query = self.query.and_filter(Expr::lt("optional_date", value));
        self
    }

    pub fn with_optional_date_less_than_or_equal_to(mut self, value: impl Into<teaql_core::Value>) -> Self {
        self.query = self.query.and_filter(Expr::lte("optional_date", value));
        self
    }

    pub fn with_optional_date_between(
        mut self,
        lower: impl Into<teaql_core::Value>,
        upper: impl Into<teaql_core::Value>,
    ) -> Self {
        self.query = self.query.and_filter(Expr::between("optional_date", lower, upper));
        self
    }

    pub fn with_optional_date_between_range<T>(mut self, range: DateRange<T>) -> Self
    where
        T: Into<teaql_core::Value>,
    {
        self.query = self.query.and_filter(Expr::between(
            "optional_date",
            range.start,
            range.end,
        ));
        self
    }

    pub fn with_optional_date_in(
        mut self,
        values: impl IntoIterator<Item = impl Into<teaql_core::Value>>,
    ) -> Self {
        self.query = self.query.and_filter(Expr::in_list(
            "optional_date",
            values.into_iter().map(Into::into),
        ));
        self
    }

    pub fn with_optional_date_not_in(
        mut self,
        values: impl IntoIterator<Item = impl Into<teaql_core::Value>>,
    ) -> Self {
        self.query = self.query.and_filter(Expr::not_in_list(
            "optional_date",
            values.into_iter().map(Into::into),
        ));
        self
    }

    pub fn with_optional_date_before(mut self, value: impl Into<teaql_core::Value>) -> Self {
        self.query = self.query.and_filter(Expr::lt("optional_date", value));
        self
    }

    pub fn with_optional_date_after(mut self, value: impl Into<teaql_core::Value>) -> Self {
        self.query = self.query.and_filter(Expr::gt("optional_date", value));
        self
    }

    pub fn with_optional_date_is_unknown(mut self) -> Self {
        self.query = self.query.and_filter(Expr::is_null("optional_date"));
        self
    }



    pub fn with_optional_date_is_known(mut self) -> Self {
        self.query = self.query.and_filter(Expr::is_not_null("optional_date"));
        self
    }


    pub fn order_by_optional_date_asc(mut self) -> Self {
        self.query = self.query.order_asc("optional_date");
        self
    }

    pub fn order_by_optional_date_desc(mut self) -> Self {
        self.query = self.query.order_desc("optional_date");
        self
    }

    pub fn order_by_optional_date_asc_using_gbk(mut self) -> Self {
        self.query = self.query.order_gbk_asc("optional_date");
        self
    }

    pub fn order_by_optional_date_desc_using_gbk(mut self) -> Self {
        self.query = self.query.order_gbk_desc("optional_date");
        self
    }


    pub fn select_time(mut self) -> Self {
        self.query = self.query.project("time");
        self
    }

    pub fn project_time(self) -> Self {
        self.select_time()
    }

    pub fn select_time_raw(self, raw_sql_segment: impl Into<String>) -> Self {
        self.select_time_unsafe_raw(UnsafeRawSqlSegment::trusted(raw_sql_segment))
    }

    pub fn select_time_unsafe_raw(mut self, raw_sql_segment: UnsafeRawSqlSegment) -> Self {
        self.query_options
            .raw_projections
            .push(RawProjection::new("time", raw_sql_segment));
        self
    }

    pub fn group_by_time(self) -> Self {
        self.group_by("time")
    }

    pub fn group_by_time_as(self, alias: impl Into<String>) -> Self {
        let alias = alias.into();
        let mut request = self.group_by("time");
        request.query = request
            .query
            .project_expr(alias, Expr::column("time"));
        request
    }

    pub fn group_by_time_with_function(
        self,
        alias: impl Into<String>,
        function: AggregateFunction,
    ) -> Self {
        self.group_by("time")
            .aggregate_with_function("time", alias, function)
    }

    pub fn count_time(self) -> Self {
        self.count_time_as("time_count")
    }

    pub fn count_time_as(self, alias: impl Into<String>) -> Self {
        self.aggregate_count_field("time", alias)
    }

    pub fn sum_time(self) -> Self {
        self.sum_time_as("sum_time")
    }

    pub fn sum_time_as(self, alias: impl Into<String>) -> Self {
        self.aggregate_sum("time", alias)
    }

    pub fn avg_time(self) -> Self {
        self.avg_time_as("avg_time")
    }

    pub fn avg_time_as(self, alias: impl Into<String>) -> Self {
        self.aggregate_avg("time", alias)
    }

    pub fn min_time(self) -> Self {
        self.min_time_as("min_time")
    }

    pub fn min_time_as(self, alias: impl Into<String>) -> Self {
        self.aggregate_min("time", alias)
    }

    pub fn max_time(self) -> Self {
        self.max_time_as("max_time")
    }

    pub fn max_time_as(self, alias: impl Into<String>) -> Self {
        self.aggregate_max("time", alias)
    }

    pub fn unselect_time(mut self) -> Self {
        self.query.projection.retain(|field| field != "time");
        self.query_options.raw_projections.retain(|projection| projection.property_name != "time");
        self
    }


    pub fn with_time(
        mut self,
        operator: FieldOperator,
        values: impl IntoIterator<Item = impl Into<teaql_core::Value>>,
    ) -> Self {
        self.query = self.query.and_filter(field_operator_expr(
            "time",
            operator,
            values.into_iter().map(Into::into).collect(),
        ));
        self
    }

    pub fn create_time_criteria(
        operator: FieldOperator,
        values: impl IntoIterator<Item = impl Into<teaql_core::Value>>,
    ) -> Expr {
        field_operator_expr(
            "time",
            operator,
            values.into_iter().map(Into::into).collect(),
        )
    }

    pub fn with_time_is(mut self, value: impl Into<teaql_core::Value>) -> Self {
        self.query = self.query.and_filter(Expr::eq("time", value));
        self
    }



    pub fn with_time_is_not(mut self, value: impl Into<teaql_core::Value>) -> Self {
        self.query = self.query.and_filter(Expr::ne("time", value));
        self
    }

    pub fn with_time_greater_than(mut self, value: impl Into<teaql_core::Value>) -> Self {
        self.query = self.query.and_filter(Expr::gt("time", value));
        self
    }

    pub fn with_time_greater_than_or_equal_to(mut self, value: impl Into<teaql_core::Value>) -> Self {
        self.query = self.query.and_filter(Expr::gte("time", value));
        self
    }

    pub fn with_time_less_than(mut self, value: impl Into<teaql_core::Value>) -> Self {
        self.query = self.query.and_filter(Expr::lt("time", value));
        self
    }

    pub fn with_time_less_than_or_equal_to(mut self, value: impl Into<teaql_core::Value>) -> Self {
        self.query = self.query.and_filter(Expr::lte("time", value));
        self
    }

    pub fn with_time_between(
        mut self,
        lower: impl Into<teaql_core::Value>,
        upper: impl Into<teaql_core::Value>,
    ) -> Self {
        self.query = self.query.and_filter(Expr::between("time", lower, upper));
        self
    }

    pub fn with_time_between_range<T>(mut self, range: DateRange<T>) -> Self
    where
        T: Into<teaql_core::Value>,
    {
        self.query = self.query.and_filter(Expr::between(
            "time",
            range.start,
            range.end,
        ));
        self
    }

    pub fn with_time_in(
        mut self,
        values: impl IntoIterator<Item = impl Into<teaql_core::Value>>,
    ) -> Self {
        self.query = self.query.and_filter(Expr::in_list(
            "time",
            values.into_iter().map(Into::into),
        ));
        self
    }

    pub fn with_time_not_in(
        mut self,
        values: impl IntoIterator<Item = impl Into<teaql_core::Value>>,
    ) -> Self {
        self.query = self.query.and_filter(Expr::not_in_list(
            "time",
            values.into_iter().map(Into::into),
        ));
        self
    }

    pub fn with_time_before(mut self, value: impl Into<teaql_core::Value>) -> Self {
        self.query = self.query.and_filter(Expr::lt("time", value));
        self
    }

    pub fn with_time_after(mut self, value: impl Into<teaql_core::Value>) -> Self {
        self.query = self.query.and_filter(Expr::gt("time", value));
        self
    }

    pub fn with_time_is_unknown(mut self) -> Self {
        self.query = self.query.and_filter(Expr::is_null("time"));
        self
    }



    pub fn with_time_is_known(mut self) -> Self {
        self.query = self.query.and_filter(Expr::is_not_null("time"));
        self
    }


    pub fn order_by_time_asc(mut self) -> Self {
        self.query = self.query.order_asc("time");
        self
    }

    pub fn order_by_time_desc(mut self) -> Self {
        self.query = self.query.order_desc("time");
        self
    }

    pub fn order_by_time_asc_using_gbk(mut self) -> Self {
        self.query = self.query.order_gbk_asc("time");
        self
    }

    pub fn order_by_time_desc_using_gbk(mut self) -> Self {
        self.query = self.query.order_gbk_desc("time");
        self
    }


    pub fn select_optional_time(mut self) -> Self {
        self.query = self.query.project("optional_time");
        self
    }

    pub fn project_optional_time(self) -> Self {
        self.select_optional_time()
    }

    pub fn select_optional_time_raw(self, raw_sql_segment: impl Into<String>) -> Self {
        self.select_optional_time_unsafe_raw(UnsafeRawSqlSegment::trusted(raw_sql_segment))
    }

    pub fn select_optional_time_unsafe_raw(mut self, raw_sql_segment: UnsafeRawSqlSegment) -> Self {
        self.query_options
            .raw_projections
            .push(RawProjection::new("optional_time", raw_sql_segment));
        self
    }

    pub fn group_by_optional_time(self) -> Self {
        self.group_by("optional_time")
    }

    pub fn group_by_optional_time_as(self, alias: impl Into<String>) -> Self {
        let alias = alias.into();
        let mut request = self.group_by("optional_time");
        request.query = request
            .query
            .project_expr(alias, Expr::column("optional_time"));
        request
    }

    pub fn group_by_optional_time_with_function(
        self,
        alias: impl Into<String>,
        function: AggregateFunction,
    ) -> Self {
        self.group_by("optional_time")
            .aggregate_with_function("optional_time", alias, function)
    }

    pub fn count_optional_time(self) -> Self {
        self.count_optional_time_as("optional_time_count")
    }

    pub fn count_optional_time_as(self, alias: impl Into<String>) -> Self {
        self.aggregate_count_field("optional_time", alias)
    }

    pub fn sum_optional_time(self) -> Self {
        self.sum_optional_time_as("sum_optional_time")
    }

    pub fn sum_optional_time_as(self, alias: impl Into<String>) -> Self {
        self.aggregate_sum("optional_time", alias)
    }

    pub fn avg_optional_time(self) -> Self {
        self.avg_optional_time_as("avg_optional_time")
    }

    pub fn avg_optional_time_as(self, alias: impl Into<String>) -> Self {
        self.aggregate_avg("optional_time", alias)
    }

    pub fn min_optional_time(self) -> Self {
        self.min_optional_time_as("min_optional_time")
    }

    pub fn min_optional_time_as(self, alias: impl Into<String>) -> Self {
        self.aggregate_min("optional_time", alias)
    }

    pub fn max_optional_time(self) -> Self {
        self.max_optional_time_as("max_optional_time")
    }

    pub fn max_optional_time_as(self, alias: impl Into<String>) -> Self {
        self.aggregate_max("optional_time", alias)
    }

    pub fn unselect_optional_time(mut self) -> Self {
        self.query.projection.retain(|field| field != "optional_time");
        self.query_options.raw_projections.retain(|projection| projection.property_name != "optional_time");
        self
    }


    pub fn with_optional_time(
        mut self,
        operator: FieldOperator,
        values: impl IntoIterator<Item = impl Into<teaql_core::Value>>,
    ) -> Self {
        self.query = self.query.and_filter(field_operator_expr(
            "optional_time",
            operator,
            values.into_iter().map(Into::into).collect(),
        ));
        self
    }

    pub fn create_optional_time_criteria(
        operator: FieldOperator,
        values: impl IntoIterator<Item = impl Into<teaql_core::Value>>,
    ) -> Expr {
        field_operator_expr(
            "optional_time",
            operator,
            values.into_iter().map(Into::into).collect(),
        )
    }

    pub fn with_optional_time_is(mut self, value: impl Into<teaql_core::Value>) -> Self {
        self.query = self.query.and_filter(Expr::eq("optional_time", value));
        self
    }



    pub fn with_optional_time_is_not(mut self, value: impl Into<teaql_core::Value>) -> Self {
        self.query = self.query.and_filter(Expr::ne("optional_time", value));
        self
    }

    pub fn with_optional_time_greater_than(mut self, value: impl Into<teaql_core::Value>) -> Self {
        self.query = self.query.and_filter(Expr::gt("optional_time", value));
        self
    }

    pub fn with_optional_time_greater_than_or_equal_to(mut self, value: impl Into<teaql_core::Value>) -> Self {
        self.query = self.query.and_filter(Expr::gte("optional_time", value));
        self
    }

    pub fn with_optional_time_less_than(mut self, value: impl Into<teaql_core::Value>) -> Self {
        self.query = self.query.and_filter(Expr::lt("optional_time", value));
        self
    }

    pub fn with_optional_time_less_than_or_equal_to(mut self, value: impl Into<teaql_core::Value>) -> Self {
        self.query = self.query.and_filter(Expr::lte("optional_time", value));
        self
    }

    pub fn with_optional_time_between(
        mut self,
        lower: impl Into<teaql_core::Value>,
        upper: impl Into<teaql_core::Value>,
    ) -> Self {
        self.query = self.query.and_filter(Expr::between("optional_time", lower, upper));
        self
    }

    pub fn with_optional_time_between_range<T>(mut self, range: DateRange<T>) -> Self
    where
        T: Into<teaql_core::Value>,
    {
        self.query = self.query.and_filter(Expr::between(
            "optional_time",
            range.start,
            range.end,
        ));
        self
    }

    pub fn with_optional_time_in(
        mut self,
        values: impl IntoIterator<Item = impl Into<teaql_core::Value>>,
    ) -> Self {
        self.query = self.query.and_filter(Expr::in_list(
            "optional_time",
            values.into_iter().map(Into::into),
        ));
        self
    }

    pub fn with_optional_time_not_in(
        mut self,
        values: impl IntoIterator<Item = impl Into<teaql_core::Value>>,
    ) -> Self {
        self.query = self.query.and_filter(Expr::not_in_list(
            "optional_time",
            values.into_iter().map(Into::into),
        ));
        self
    }

    pub fn with_optional_time_before(mut self, value: impl Into<teaql_core::Value>) -> Self {
        self.query = self.query.and_filter(Expr::lt("optional_time", value));
        self
    }

    pub fn with_optional_time_after(mut self, value: impl Into<teaql_core::Value>) -> Self {
        self.query = self.query.and_filter(Expr::gt("optional_time", value));
        self
    }

    pub fn with_optional_time_is_unknown(mut self) -> Self {
        self.query = self.query.and_filter(Expr::is_null("optional_time"));
        self
    }



    pub fn with_optional_time_is_known(mut self) -> Self {
        self.query = self.query.and_filter(Expr::is_not_null("optional_time"));
        self
    }


    pub fn order_by_optional_time_asc(mut self) -> Self {
        self.query = self.query.order_asc("optional_time");
        self
    }

    pub fn order_by_optional_time_desc(mut self) -> Self {
        self.query = self.query.order_desc("optional_time");
        self
    }

    pub fn order_by_optional_time_asc_using_gbk(mut self) -> Self {
        self.query = self.query.order_gbk_asc("optional_time");
        self
    }

    pub fn order_by_optional_time_desc_using_gbk(mut self) -> Self {
        self.query = self.query.order_gbk_desc("optional_time");
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


    pub fn select_optional_payload(mut self) -> Self {
        self.query = self.query.project("optional_payload");
        self
    }

    pub fn project_optional_payload(self) -> Self {
        self.select_optional_payload()
    }

    pub fn select_optional_payload_raw(self, raw_sql_segment: impl Into<String>) -> Self {
        self.select_optional_payload_unsafe_raw(UnsafeRawSqlSegment::trusted(raw_sql_segment))
    }

    pub fn select_optional_payload_unsafe_raw(mut self, raw_sql_segment: UnsafeRawSqlSegment) -> Self {
        self.query_options
            .raw_projections
            .push(RawProjection::new("optional_payload", raw_sql_segment));
        self
    }

    pub fn group_by_optional_payload(self) -> Self {
        self.group_by("optional_payload")
    }

    pub fn group_by_optional_payload_as(self, alias: impl Into<String>) -> Self {
        let alias = alias.into();
        let mut request = self.group_by("optional_payload");
        request.query = request
            .query
            .project_expr(alias, Expr::column("optional_payload"));
        request
    }

    pub fn group_by_optional_payload_with_function(
        self,
        alias: impl Into<String>,
        function: AggregateFunction,
    ) -> Self {
        self.group_by("optional_payload")
            .aggregate_with_function("optional_payload", alias, function)
    }

    pub fn count_optional_payload(self) -> Self {
        self.count_optional_payload_as("optional_payload_count")
    }

    pub fn count_optional_payload_as(self, alias: impl Into<String>) -> Self {
        self.aggregate_count_field("optional_payload", alias)
    }

    pub fn sum_optional_payload(self) -> Self {
        self.sum_optional_payload_as("sum_optional_payload")
    }

    pub fn sum_optional_payload_as(self, alias: impl Into<String>) -> Self {
        self.aggregate_sum("optional_payload", alias)
    }

    pub fn avg_optional_payload(self) -> Self {
        self.avg_optional_payload_as("avg_optional_payload")
    }

    pub fn avg_optional_payload_as(self, alias: impl Into<String>) -> Self {
        self.aggregate_avg("optional_payload", alias)
    }

    pub fn min_optional_payload(self) -> Self {
        self.min_optional_payload_as("min_optional_payload")
    }

    pub fn min_optional_payload_as(self, alias: impl Into<String>) -> Self {
        self.aggregate_min("optional_payload", alias)
    }

    pub fn max_optional_payload(self) -> Self {
        self.max_optional_payload_as("max_optional_payload")
    }

    pub fn max_optional_payload_as(self, alias: impl Into<String>) -> Self {
        self.aggregate_max("optional_payload", alias)
    }

    pub fn unselect_optional_payload(mut self) -> Self {
        self.query.projection.retain(|field| field != "optional_payload");
        self.query_options.raw_projections.retain(|projection| projection.property_name != "optional_payload");
        self
    }


    pub fn with_optional_payload(
        mut self,
        operator: FieldOperator,
        values: impl IntoIterator<Item = impl Into<teaql_core::Value>>,
    ) -> Self {
        self.query = self.query.and_filter(field_operator_expr(
            "optional_payload",
            operator,
            values.into_iter().map(Into::into).collect(),
        ));
        self
    }

    pub fn create_optional_payload_criteria(
        operator: FieldOperator,
        values: impl IntoIterator<Item = impl Into<teaql_core::Value>>,
    ) -> Expr {
        field_operator_expr(
            "optional_payload",
            operator,
            values.into_iter().map(Into::into).collect(),
        )
    }

    pub fn with_optional_payload_is(mut self, value: impl Into<teaql_core::Value>) -> Self {
        self.query = self.query.and_filter(Expr::eq("optional_payload", value));
        self
    }



    pub fn with_optional_payload_is_not(mut self, value: impl Into<teaql_core::Value>) -> Self {
        self.query = self.query.and_filter(Expr::ne("optional_payload", value));
        self
    }

    pub fn with_optional_payload_greater_than(mut self, value: impl Into<teaql_core::Value>) -> Self {
        self.query = self.query.and_filter(Expr::gt("optional_payload", value));
        self
    }

    pub fn with_optional_payload_greater_than_or_equal_to(mut self, value: impl Into<teaql_core::Value>) -> Self {
        self.query = self.query.and_filter(Expr::gte("optional_payload", value));
        self
    }

    pub fn with_optional_payload_less_than(mut self, value: impl Into<teaql_core::Value>) -> Self {
        self.query = self.query.and_filter(Expr::lt("optional_payload", value));
        self
    }

    pub fn with_optional_payload_less_than_or_equal_to(mut self, value: impl Into<teaql_core::Value>) -> Self {
        self.query = self.query.and_filter(Expr::lte("optional_payload", value));
        self
    }

    pub fn with_optional_payload_between(
        mut self,
        lower: impl Into<teaql_core::Value>,
        upper: impl Into<teaql_core::Value>,
    ) -> Self {
        self.query = self.query.and_filter(Expr::between("optional_payload", lower, upper));
        self
    }

    pub fn with_optional_payload_between_range<T>(mut self, range: DateRange<T>) -> Self
    where
        T: Into<teaql_core::Value>,
    {
        self.query = self.query.and_filter(Expr::between(
            "optional_payload",
            range.start,
            range.end,
        ));
        self
    }

    pub fn with_optional_payload_in(
        mut self,
        values: impl IntoIterator<Item = impl Into<teaql_core::Value>>,
    ) -> Self {
        self.query = self.query.and_filter(Expr::in_list(
            "optional_payload",
            values.into_iter().map(Into::into),
        ));
        self
    }

    pub fn with_optional_payload_not_in(
        mut self,
        values: impl IntoIterator<Item = impl Into<teaql_core::Value>>,
    ) -> Self {
        self.query = self.query.and_filter(Expr::not_in_list(
            "optional_payload",
            values.into_iter().map(Into::into),
        ));
        self
    }

    pub fn with_optional_payload_before(mut self, value: impl Into<teaql_core::Value>) -> Self {
        self.query = self.query.and_filter(Expr::lt("optional_payload", value));
        self
    }

    pub fn with_optional_payload_after(mut self, value: impl Into<teaql_core::Value>) -> Self {
        self.query = self.query.and_filter(Expr::gt("optional_payload", value));
        self
    }

    pub fn with_optional_payload_is_unknown(mut self) -> Self {
        self.query = self.query.and_filter(Expr::is_null("optional_payload"));
        self
    }



    pub fn with_optional_payload_is_known(mut self) -> Self {
        self.query = self.query.and_filter(Expr::is_not_null("optional_payload"));
        self
    }


    pub fn order_by_optional_payload_asc(mut self) -> Self {
        self.query = self.query.order_asc("optional_payload");
        self
    }

    pub fn order_by_optional_payload_desc(mut self) -> Self {
        self.query = self.query.order_desc("optional_payload");
        self
    }

    pub fn order_by_optional_payload_asc_using_gbk(mut self) -> Self {
        self.query = self.query.order_gbk_asc("optional_payload");
        self
    }

    pub fn order_by_optional_payload_desc_using_gbk(mut self) -> Self {
        self.query = self.query.order_gbk_desc("optional_payload");
        self
    }


    pub fn select_timestamp(mut self) -> Self {
        self.query = self.query.project("timestamp");
        self
    }

    pub fn project_timestamp(self) -> Self {
        self.select_timestamp()
    }

    pub fn select_timestamp_raw(self, raw_sql_segment: impl Into<String>) -> Self {
        self.select_timestamp_unsafe_raw(UnsafeRawSqlSegment::trusted(raw_sql_segment))
    }

    pub fn select_timestamp_unsafe_raw(mut self, raw_sql_segment: UnsafeRawSqlSegment) -> Self {
        self.query_options
            .raw_projections
            .push(RawProjection::new("timestamp", raw_sql_segment));
        self
    }

    pub fn group_by_timestamp(self) -> Self {
        self.group_by("timestamp")
    }

    pub fn group_by_timestamp_as(self, alias: impl Into<String>) -> Self {
        let alias = alias.into();
        let mut request = self.group_by("timestamp");
        request.query = request
            .query
            .project_expr(alias, Expr::column("timestamp"));
        request
    }

    pub fn group_by_timestamp_with_function(
        self,
        alias: impl Into<String>,
        function: AggregateFunction,
    ) -> Self {
        self.group_by("timestamp")
            .aggregate_with_function("timestamp", alias, function)
    }

    pub fn count_timestamp(self) -> Self {
        self.count_timestamp_as("timestamp_count")
    }

    pub fn count_timestamp_as(self, alias: impl Into<String>) -> Self {
        self.aggregate_count_field("timestamp", alias)
    }

    pub fn sum_timestamp(self) -> Self {
        self.sum_timestamp_as("sum_timestamp")
    }

    pub fn sum_timestamp_as(self, alias: impl Into<String>) -> Self {
        self.aggregate_sum("timestamp", alias)
    }

    pub fn avg_timestamp(self) -> Self {
        self.avg_timestamp_as("avg_timestamp")
    }

    pub fn avg_timestamp_as(self, alias: impl Into<String>) -> Self {
        self.aggregate_avg("timestamp", alias)
    }

    pub fn min_timestamp(self) -> Self {
        self.min_timestamp_as("min_timestamp")
    }

    pub fn min_timestamp_as(self, alias: impl Into<String>) -> Self {
        self.aggregate_min("timestamp", alias)
    }

    pub fn max_timestamp(self) -> Self {
        self.max_timestamp_as("max_timestamp")
    }

    pub fn max_timestamp_as(self, alias: impl Into<String>) -> Self {
        self.aggregate_max("timestamp", alias)
    }

    pub fn unselect_timestamp(mut self) -> Self {
        self.query.projection.retain(|field| field != "timestamp");
        self.query_options.raw_projections.retain(|projection| projection.property_name != "timestamp");
        self
    }


    pub fn with_timestamp(
        mut self,
        operator: FieldOperator,
        values: impl IntoIterator<Item = impl Into<teaql_core::Value>>,
    ) -> Self {
        self.query = self.query.and_filter(field_operator_expr(
            "timestamp",
            operator,
            values.into_iter().map(Into::into).collect(),
        ));
        self
    }

    pub fn create_timestamp_criteria(
        operator: FieldOperator,
        values: impl IntoIterator<Item = impl Into<teaql_core::Value>>,
    ) -> Expr {
        field_operator_expr(
            "timestamp",
            operator,
            values.into_iter().map(Into::into).collect(),
        )
    }

    pub fn with_timestamp_is(mut self, value: impl Into<teaql_core::Value>) -> Self {
        self.query = self.query.and_filter(Expr::eq("timestamp", value));
        self
    }



    pub fn with_timestamp_is_not(mut self, value: impl Into<teaql_core::Value>) -> Self {
        self.query = self.query.and_filter(Expr::ne("timestamp", value));
        self
    }

    pub fn with_timestamp_greater_than(mut self, value: impl Into<teaql_core::Value>) -> Self {
        self.query = self.query.and_filter(Expr::gt("timestamp", value));
        self
    }

    pub fn with_timestamp_greater_than_or_equal_to(mut self, value: impl Into<teaql_core::Value>) -> Self {
        self.query = self.query.and_filter(Expr::gte("timestamp", value));
        self
    }

    pub fn with_timestamp_less_than(mut self, value: impl Into<teaql_core::Value>) -> Self {
        self.query = self.query.and_filter(Expr::lt("timestamp", value));
        self
    }

    pub fn with_timestamp_less_than_or_equal_to(mut self, value: impl Into<teaql_core::Value>) -> Self {
        self.query = self.query.and_filter(Expr::lte("timestamp", value));
        self
    }

    pub fn with_timestamp_between(
        mut self,
        lower: impl Into<teaql_core::Value>,
        upper: impl Into<teaql_core::Value>,
    ) -> Self {
        self.query = self.query.and_filter(Expr::between("timestamp", lower, upper));
        self
    }

    pub fn with_timestamp_between_range<T>(mut self, range: DateRange<T>) -> Self
    where
        T: Into<teaql_core::Value>,
    {
        self.query = self.query.and_filter(Expr::between(
            "timestamp",
            range.start,
            range.end,
        ));
        self
    }

    pub fn with_timestamp_in(
        mut self,
        values: impl IntoIterator<Item = impl Into<teaql_core::Value>>,
    ) -> Self {
        self.query = self.query.and_filter(Expr::in_list(
            "timestamp",
            values.into_iter().map(Into::into),
        ));
        self
    }

    pub fn with_timestamp_not_in(
        mut self,
        values: impl IntoIterator<Item = impl Into<teaql_core::Value>>,
    ) -> Self {
        self.query = self.query.and_filter(Expr::not_in_list(
            "timestamp",
            values.into_iter().map(Into::into),
        ));
        self
    }

    pub fn with_timestamp_before(mut self, value: impl Into<teaql_core::Value>) -> Self {
        self.query = self.query.and_filter(Expr::lt("timestamp", value));
        self
    }

    pub fn with_timestamp_after(mut self, value: impl Into<teaql_core::Value>) -> Self {
        self.query = self.query.and_filter(Expr::gt("timestamp", value));
        self
    }

    pub fn with_timestamp_is_unknown(mut self) -> Self {
        self.query = self.query.and_filter(Expr::is_null("timestamp"));
        self
    }



    pub fn with_timestamp_is_known(mut self) -> Self {
        self.query = self.query.and_filter(Expr::is_not_null("timestamp"));
        self
    }


    pub fn order_by_timestamp_asc(mut self) -> Self {
        self.query = self.query.order_asc("timestamp");
        self
    }

    pub fn order_by_timestamp_desc(mut self) -> Self {
        self.query = self.query.order_desc("timestamp");
        self
    }

    pub fn order_by_timestamp_asc_using_gbk(mut self) -> Self {
        self.query = self.query.order_gbk_asc("timestamp");
        self
    }

    pub fn order_by_timestamp_desc_using_gbk(mut self) -> Self {
        self.query = self.query.order_gbk_desc("timestamp");
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
    pub fn with_text_is_string_create_function_now(self) -> Self {
        self.with_text_is("string(createFunction=now")
    }



    pub fn with_text_is_not_string_create_function_now(self) -> Self {
        self.with_text_is_not("string(createFunction=now")
    }


    pub fn with_text_is_update_function_now(self) -> Self {
        self.with_text_is("updateFunction=now)")
    }



    pub fn with_text_is_not_update_function_now(self) -> Self {
        self.with_text_is_not("updateFunction=now)")
    }



    pub fn with_optional_text_is_string_create_function_now(self) -> Self {
        self.with_optional_text_is("string(createFunction=now")
    }



    pub fn with_optional_text_is_not_string_create_function_now(self) -> Self {
        self.with_optional_text_is_not("string(createFunction=now")
    }


    pub fn with_optional_text_is_update_function_now(self) -> Self {
        self.with_optional_text_is("updateFunction=now)")
    }



    pub fn with_optional_text_is_not_update_function_now(self) -> Self {
        self.with_optional_text_is_not("updateFunction=now)")
    }



    pub fn with_code_is_uni_str_create_function_now(self) -> Self {
        self.with_code_is("uniStr(createFunction=now")
    }



    pub fn with_code_is_not_uni_str_create_function_now(self) -> Self {
        self.with_code_is_not("uniStr(createFunction=now")
    }


    pub fn with_code_is_update_function_now(self) -> Self {
        self.with_code_is("updateFunction=now)")
    }



    pub fn with_code_is_not_update_function_now(self) -> Self {
        self.with_code_is_not("updateFunction=now)")
    }



    pub fn with_optional_code_is_uni_str_create_function_now(self) -> Self {
        self.with_optional_code_is("uniStr(createFunction=now")
    }



    pub fn with_optional_code_is_not_uni_str_create_function_now(self) -> Self {
        self.with_optional_code_is_not("uniStr(createFunction=now")
    }


    pub fn with_optional_code_is_update_function_now(self) -> Self {
        self.with_optional_code_is("updateFunction=now)")
    }



    pub fn with_optional_code_is_not_update_function_now(self) -> Self {
        self.with_optional_code_is_not("updateFunction=now)")
    }



    pub fn with_integer_is_integer_create_function_now(self) -> Self {
        self.with_integer_is("integer(createFunction=now")
    }



    pub fn with_integer_is_not_integer_create_function_now(self) -> Self {
        self.with_integer_is_not("integer(createFunction=now")
    }


    pub fn with_integer_is_update_function_now(self) -> Self {
        self.with_integer_is("updateFunction=now)")
    }



    pub fn with_integer_is_not_update_function_now(self) -> Self {
        self.with_integer_is_not("updateFunction=now)")
    }



    pub fn with_optional_integer_is_integer_create_function_now(self) -> Self {
        self.with_optional_integer_is("integer(createFunction=now")
    }



    pub fn with_optional_integer_is_not_integer_create_function_now(self) -> Self {
        self.with_optional_integer_is_not("integer(createFunction=now")
    }


    pub fn with_optional_integer_is_update_function_now(self) -> Self {
        self.with_optional_integer_is("updateFunction=now)")
    }



    pub fn with_optional_integer_is_not_update_function_now(self) -> Self {
        self.with_optional_integer_is_not("updateFunction=now)")
    }



    pub fn with_long_is_long_create_function_now(self) -> Self {
        self.with_long_is("long(createFunction=now")
    }



    pub fn with_long_is_not_long_create_function_now(self) -> Self {
        self.with_long_is_not("long(createFunction=now")
    }


    pub fn with_long_is_update_function_now(self) -> Self {
        self.with_long_is("updateFunction=now)")
    }



    pub fn with_long_is_not_update_function_now(self) -> Self {
        self.with_long_is_not("updateFunction=now)")
    }



    pub fn with_optional_long_is_long_create_function_now(self) -> Self {
        self.with_optional_long_is("long(createFunction=now")
    }



    pub fn with_optional_long_is_not_long_create_function_now(self) -> Self {
        self.with_optional_long_is_not("long(createFunction=now")
    }


    pub fn with_optional_long_is_update_function_now(self) -> Self {
        self.with_optional_long_is("updateFunction=now)")
    }



    pub fn with_optional_long_is_not_update_function_now(self) -> Self {
        self.with_optional_long_is_not("updateFunction=now)")
    }



    pub fn with_decimal_is_number_create_function_now(self) -> Self {
        self.with_decimal_is("number(createFunction=now")
    }



    pub fn with_decimal_is_not_number_create_function_now(self) -> Self {
        self.with_decimal_is_not("number(createFunction=now")
    }


    pub fn with_decimal_is_update_function_now(self) -> Self {
        self.with_decimal_is("updateFunction=now)")
    }



    pub fn with_decimal_is_not_update_function_now(self) -> Self {
        self.with_decimal_is_not("updateFunction=now)")
    }



    pub fn with_optional_decimal_is_number_create_function_now(self) -> Self {
        self.with_optional_decimal_is("number(createFunction=now")
    }



    pub fn with_optional_decimal_is_not_number_create_function_now(self) -> Self {
        self.with_optional_decimal_is_not("number(createFunction=now")
    }


    pub fn with_optional_decimal_is_update_function_now(self) -> Self {
        self.with_optional_decimal_is("updateFunction=now)")
    }



    pub fn with_optional_decimal_is_not_update_function_now(self) -> Self {
        self.with_optional_decimal_is_not("updateFunction=now)")
    }



    pub fn with_float_is_float_create_function_now(self) -> Self {
        self.with_float_is("float(createFunction=now")
    }



    pub fn with_float_is_not_float_create_function_now(self) -> Self {
        self.with_float_is_not("float(createFunction=now")
    }


    pub fn with_float_is_update_function_now(self) -> Self {
        self.with_float_is("updateFunction=now)")
    }



    pub fn with_float_is_not_update_function_now(self) -> Self {
        self.with_float_is_not("updateFunction=now)")
    }



    pub fn with_optional_float_is_float_create_function_now(self) -> Self {
        self.with_optional_float_is("float(createFunction=now")
    }



    pub fn with_optional_float_is_not_float_create_function_now(self) -> Self {
        self.with_optional_float_is_not("float(createFunction=now")
    }


    pub fn with_optional_float_is_update_function_now(self) -> Self {
        self.with_optional_float_is("updateFunction=now)")
    }



    pub fn with_optional_float_is_not_update_function_now(self) -> Self {
        self.with_optional_float_is_not("updateFunction=now)")
    }



    pub fn with_double_is_double_create_function_now(self) -> Self {
        self.with_double_is("double(createFunction=now")
    }



    pub fn with_double_is_not_double_create_function_now(self) -> Self {
        self.with_double_is_not("double(createFunction=now")
    }


    pub fn with_double_is_update_function_now(self) -> Self {
        self.with_double_is("updateFunction=now)")
    }



    pub fn with_double_is_not_update_function_now(self) -> Self {
        self.with_double_is_not("updateFunction=now)")
    }



    pub fn with_optional_double_is_double_create_function_now(self) -> Self {
        self.with_optional_double_is("double(createFunction=now")
    }



    pub fn with_optional_double_is_not_double_create_function_now(self) -> Self {
        self.with_optional_double_is_not("double(createFunction=now")
    }


    pub fn with_optional_double_is_update_function_now(self) -> Self {
        self.with_optional_double_is("updateFunction=now)")
    }



    pub fn with_optional_double_is_not_update_function_now(self) -> Self {
        self.with_optional_double_is_not("updateFunction=now)")
    }



    pub fn with_active_is_bool_create_function_now(self) -> Self {
        self.with_active_is("bool(createFunction=now")
    }



    pub fn with_active_is_not_bool_create_function_now(self) -> Self {
        self.with_active_is_not("bool(createFunction=now")
    }


    pub fn with_active_is_update_function_now(self) -> Self {
        self.with_active_is("updateFunction=now)")
    }



    pub fn with_active_is_not_update_function_now(self) -> Self {
        self.with_active_is_not("updateFunction=now)")
    }



    pub fn with_optional_active_is_bool_create_function_now(self) -> Self {
        self.with_optional_active_is("bool(createFunction=now")
    }



    pub fn with_optional_active_is_not_bool_create_function_now(self) -> Self {
        self.with_optional_active_is_not("bool(createFunction=now")
    }


    pub fn with_optional_active_is_update_function_now(self) -> Self {
        self.with_optional_active_is("updateFunction=now)")
    }



    pub fn with_optional_active_is_not_update_function_now(self) -> Self {
        self.with_optional_active_is_not("updateFunction=now)")
    }



    pub fn with_date_is_date_create_function_now(self) -> Self {
        self.with_date_is("date(createFunction=now")
    }



    pub fn with_date_is_not_date_create_function_now(self) -> Self {
        self.with_date_is_not("date(createFunction=now")
    }


    pub fn with_date_is_update_function_now(self) -> Self {
        self.with_date_is("updateFunction=now)")
    }



    pub fn with_date_is_not_update_function_now(self) -> Self {
        self.with_date_is_not("updateFunction=now)")
    }



    pub fn with_optional_date_is_date_create_function_now(self) -> Self {
        self.with_optional_date_is("date(createFunction=now")
    }



    pub fn with_optional_date_is_not_date_create_function_now(self) -> Self {
        self.with_optional_date_is_not("date(createFunction=now")
    }


    pub fn with_optional_date_is_update_function_now(self) -> Self {
        self.with_optional_date_is("updateFunction=now)")
    }



    pub fn with_optional_date_is_not_update_function_now(self) -> Self {
        self.with_optional_date_is_not("updateFunction=now)")
    }



    pub fn with_time_is_time_create_function_now(self) -> Self {
        self.with_time_is("time(createFunction=now")
    }



    pub fn with_time_is_not_time_create_function_now(self) -> Self {
        self.with_time_is_not("time(createFunction=now")
    }


    pub fn with_time_is_update_function_now(self) -> Self {
        self.with_time_is("updateFunction=now)")
    }



    pub fn with_time_is_not_update_function_now(self) -> Self {
        self.with_time_is_not("updateFunction=now)")
    }



    pub fn with_optional_time_is_time_create_function_now(self) -> Self {
        self.with_optional_time_is("time(createFunction=now")
    }



    pub fn with_optional_time_is_not_time_create_function_now(self) -> Self {
        self.with_optional_time_is_not("time(createFunction=now")
    }


    pub fn with_optional_time_is_update_function_now(self) -> Self {
        self.with_optional_time_is("updateFunction=now)")
    }



    pub fn with_optional_time_is_not_update_function_now(self) -> Self {
        self.with_optional_time_is_not("updateFunction=now)")
    }



    pub fn with_payload_is_json_me_create_function_now(self) -> Self {
        self.with_payload_is("jsonMe(createFunction=now")
    }



    pub fn with_payload_is_not_json_me_create_function_now(self) -> Self {
        self.with_payload_is_not("jsonMe(createFunction=now")
    }


    pub fn with_payload_is_update_function_now(self) -> Self {
        self.with_payload_is("updateFunction=now)")
    }



    pub fn with_payload_is_not_update_function_now(self) -> Self {
        self.with_payload_is_not("updateFunction=now)")
    }



    pub fn with_optional_payload_is_json_me_create_function_now(self) -> Self {
        self.with_optional_payload_is("jsonMe(createFunction=now")
    }



    pub fn with_optional_payload_is_not_json_me_create_function_now(self) -> Self {
        self.with_optional_payload_is_not("jsonMe(createFunction=now")
    }


    pub fn with_optional_payload_is_update_function_now(self) -> Self {
        self.with_optional_payload_is("updateFunction=now)")
    }



    pub fn with_optional_payload_is_not_update_function_now(self) -> Self {
        self.with_optional_payload_is_not("updateFunction=now)")
    }



    pub fn with_timestamp_is_date_time_create_function_now(self) -> Self {
        self.with_timestamp_is("dateTime(createFunction=now")
    }



    pub fn with_timestamp_is_not_date_time_create_function_now(self) -> Self {
        self.with_timestamp_is_not("dateTime(createFunction=now")
    }


    pub fn with_timestamp_is_update_function_now(self) -> Self {
        self.with_timestamp_is("updateFunction=now)")
    }



    pub fn with_timestamp_is_not_update_function_now(self) -> Self {
        self.with_timestamp_is_not("updateFunction=now)")
    }




}

impl<R> Default for NoteRequest<R> {
    fn default() -> Self {
        Self::new()
    }
}

impl<R> From< NoteRequest<R> > for SelectQuery {
    fn from(request: NoteRequest<R>) -> Self {
        QuerySelection::from(request).into_query()
    }
}

impl<R> From< NoteRequest<R> > for QuerySelection {
    fn from(request: NoteRequest<R>) -> Self {
        Self {
            query: request.query,
            relation_selections: request.relation_selections,
            relation_filters: request.relation_filters,
            child_enhancements: request.child_enhancements,
            query_options: request.query_options,
        }
    }
}


impl<'a, C> crate::request_support::AuditedSave<'a, C> for teaql_core::Audited<crate::Note> 
where C: crate::TeaqlRuntime + ?Sized + 'a
{
    type Error = teaql_runtime::RuntimeError;
    type Entity = crate::Note;
    fn save(self, context: &'a C) -> std::pin::Pin<Box<dyn std::future::Future<Output = Result<Self::Entity, Self::Error>> + Send + '_>> {
        Box::pin(async move {
            context.save_audited_entity(self).await
        })
    }
}

impl<R: teaql_core::Entity> crate::PurposedQuery<NoteRequest<R>> {
    pub fn comment(mut self, comment: impl Into<String>) -> Self {
        self.inner.query_options.comment = Some(comment.into());
        self
    }

    pub fn new_entity<C>(&self, context: &C) -> crate::Note
    where
        C: crate::TeaqlRuntime + ?Sized,
    {
        self.require_comment();
        let mut entity = crate::Note::runtime_new(context.user_context().entity_runtime_state());
        if let Ok(id) = context.user_context().next_id(crate::Note::ENTITY_NAME) {
            entity.update_id(id);
        }
        teaql_core::Entity::mark_as_new(&mut entity);
        entity
    }

    fn into_inner_with_trace(mut self) -> NoteRequest<R> {
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
