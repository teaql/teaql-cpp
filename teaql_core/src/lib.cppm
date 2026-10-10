// Publish existing global declarations; implementation bodies remain canonical.
module;
#include <teaql/core.hpp>
#include <teaql/core/detail/hmac_sha256.hpp>
#include <teaql/core/detail/aes_gcm.hpp>
#include <teaql/core/detail/query_debug.hpp>
#include <teaql/core/system_time.hpp>
#include <teaql/core/system_time_debug.hpp>
export module teaql_core;

export import teaql_memory;

export namespace teaql {
using ::teaql::ACTION_LIST_KEY;
using ::teaql::Aggregate;
using ::teaql::AggregateFunction;
using ::teaql::AggregationCacheOptions;
using ::teaql::AnyRef;
using ::teaql::Audited;
using ::teaql::BUSINESS_ID_V1_DOMAIN_SIZE;
using ::teaql::BaseEntity;
using ::teaql::BaseEntityData;
using ::teaql::BaseEntityDefaults;
using ::teaql::BatchInsertCommand;
using ::teaql::BatchUpdateCommand;
using ::teaql::BinaryOp;
using ::teaql::BoxedRelations;
using ::teaql::BusinessIdAllocation;
using ::teaql::BusinessIdAllocator;
using ::teaql::BusinessIdDefinition;
using ::teaql::BusinessIdError;
using ::teaql::BusinessIdErrorCode;
using ::teaql::BusinessIdGenerationRequest;
using ::teaql::BusinessIdPlan;
using ::teaql::BusinessIdProfile;
using ::teaql::BusinessIdScope;
using ::teaql::BusinessIdSlot;
using ::teaql::BusinessIdValue;
using ::teaql::ColumnLayout;
using ::teaql::CompactRow;
using ::teaql::ContinuousPageFetchOptions;
using ::teaql::CoreEntityGraphBuilder;
using ::teaql::DEFAULT_BUSINESS_ID_DIGITS;
using ::teaql::DEFAULT_BUSINESS_ID_PROFILE;
using ::teaql::DataType;
using ::teaql::Date;
using ::teaql::DateTime;
using ::teaql::DebugAlignment;
using ::teaql::DebugNumbers;
using ::teaql::DebugOptions;
using ::teaql::DebugWriter;
using ::teaql::Decimal;
using ::teaql::DecimalAlignment;
using ::teaql::DecimalError;
using ::teaql::DecimalFormat;
using ::teaql::DecimalInlineString;
using ::teaql::DecimalJsonError;
using ::teaql::DecimalNotation;
using ::teaql::DefaultDebugElement;
using ::teaql::DeleteCommand;
using ::teaql::Entity;
using ::teaql::EntityDefaults;
using ::teaql::EntityDescriptor;
using ::teaql::EntityDescriptorStore;
using ::teaql::EntityError;
using ::teaql::EntityGraph;
using ::teaql::EntityGraphNode;
using ::teaql::EntityGraphOperation;
using ::teaql::EntitySnapshot;
using ::teaql::Expr;
using ::teaql::ExprFunction;
using ::teaql::FieldSet;
using ::teaql::GeneratedValues;
using ::teaql::IdSetPaginationOptions;
using ::teaql::IdentifiableEntity;
using ::teaql::InsertCommand;
using ::teaql::Int128;
using ::teaql::Json;
using ::teaql::JsonParseError;
using ::teaql::LEGACY_DAILY_SEQUENCE_PROFILE;
using ::teaql::ModelDecodeError;
using ::teaql::ModelResult;
using ::teaql::MutationKind;
using ::teaql::MutationValues;
using ::teaql::NamedExpr;
using ::teaql::ObjectGroupBy;
using ::teaql::OrderBy;
using ::teaql::PARTITION_RANK_PROPERTY;
using ::teaql::PropertyDescriptor;
using ::teaql::RawSqlProjection;
using ::teaql::Record;
using ::teaql::RecoverCommand;
using ::teaql::RelationAggregate;
using ::teaql::RelationDescriptor;
using ::teaql::RelationLoad;
using ::teaql::Result;
using ::teaql::RoundingStrategy;
using ::teaql::STYLE_KEY;
using ::teaql::SafeExpression;
using ::teaql::SelectQuery;
using ::teaql::Slice;
using ::teaql::SmartList;
using ::teaql::SortDirection;
using ::teaql::StreamConfig;
using ::teaql::SubQueryExpr;
using ::teaql::TeaqlBoxedRelations;
using ::teaql::TeaqlEmpty;
using ::teaql::TeaqlEntity;
using ::teaql::Timestamp;
using ::teaql::TraceChain;
using ::teaql::TraceKind;
using ::teaql::TraceNode;
using ::teaql::TypedNull;
using ::teaql::UInt128;
using ::teaql::UnpackedDecimal;
using ::teaql::UpdateCommand;
using ::teaql::SystemDuration;
using ::teaql::SystemTimeError;
using ::teaql::SystemTimeDifference;
using ::teaql::SystemTime;
using ::teaql::UNIX_EPOCH;
using ::teaql::Value;
using ::teaql::VersionedEntity;
using ::teaql::WEB_RESPONSE_VERSION;
using ::teaql::WebAction;
using ::teaql::WebResponse;
using ::teaql::WebStyle;
using ::teaql::XlsBlock;
using ::teaql::XlsBlockBuildContext;
using ::teaql::XlsPage;
using ::teaql::XlsWorkbook;
using ::teaql::business_id_error_code_name;
using ::teaql::clone_record;
using ::teaql::compact_row_to_json_value;
using ::teaql::debug_string;
using ::teaql::default_table_name;
using ::teaql::record_to_json_value;
using ::teaql::register_entities;
using ::teaql::teaql_is_empty;
using ::teaql::value_from_utc;
using ::teaql::write_debug;
}

export namespace teaql::detail {
using ::teaql::detail::BoolVector;
using ::teaql::detail::CopyableExpressionList;
using ::teaql::detail::DecimalArithmeticAccess;
using ::teaql::detail::EntityIdMapper;
using ::teaql::detail::ExpressionEvaluator;
using ::teaql::detail::ExpressionMapper;
using ::teaql::detail::ExpressionRoot;
using ::teaql::detail::InvokeResult;
using ::teaql::detail::ListGetMapper;
using ::teaql::detail::ListValue;
using ::teaql::detail::ModelReader;
using ::teaql::detail::OptionalValue;
using ::teaql::detail::consume_record;
using ::teaql::detail::debug_float32;
using ::teaql::detail::debug_optional_integer;
using ::teaql::detail::debug_text;
using ::teaql::detail::debug_query;
using ::teaql::detail::debug_expr;
using ::teaql::detail::CryptoBlock;
using ::teaql::detail::CryptoKey;
using ::teaql::detail::CryptoNonce;
using ::teaql::detail::CryptoBytes;
using ::teaql::detail::CryptoView;
using ::teaql::detail::wipe_bytes;
using ::teaql::detail::Aes256;
using ::teaql::detail::aes256_gcm_encrypt;
using ::teaql::detail::aes256_gcm_decrypt;
using ::teaql::detail::HmacSha256;
using ::teaql::detail::hmac_sha256;
using ::teaql::detail::hkdf_sha256_32;
using ::teaql::detail::Sha256;
using ::teaql::detail::debug_value;
using ::teaql::detail::decode_model;
using ::teaql::detail::decoded;
using ::teaql::detail::expression_result;
using ::teaql::detail::invoke_in;
using ::teaql::detail::list_id_key;
using ::teaql::detail::model_array;
using ::teaql::detail::model_integer;
using ::teaql::detail::model_json;
using ::teaql::detail::model_object;
using ::teaql::detail::model_optional;
using ::teaql::detail::model_optional_string;
using ::teaql::detail::model_put;
using ::teaql::detail::model_string;
using ::teaql::detail::mutation_debug_element;
using ::teaql::detail::trim;
using ::teaql::detail::trim_start;
}

export namespace teaql::dynamic_search {
using ::teaql::dynamic_search::DynamicSearchError;
using ::teaql::dynamic_search::DynamicSearchFilter;
using ::teaql::dynamic_search::DynamicSearchOrder;
using ::teaql::dynamic_search::DynamicSearchResult;
using ::teaql::dynamic_search::DynamicSearchWarning;
using ::teaql::dynamic_search::FilterBinding;
using ::teaql::dynamic_search::NormalizedDynamicSearch;
using ::teaql::dynamic_search::OrderBinding;
using ::teaql::dynamic_search::SearchModel;
using ::teaql::dynamic_search::SearchModels;
using ::teaql::dynamic_search::SearchResult;
using ::teaql::dynamic_search::WarningSink;
using ::teaql::dynamic_search::merge_dynamic_search;
using ::teaql::dynamic_search::normalize_dynamic_search;
using ::teaql::dynamic_search::write_debug;
}

export namespace teaql::core {
using ::teaql::core::ACTION_LIST_KEY;
using ::teaql::core::Aggregate;
using ::teaql::core::AggregateFunction;
using ::teaql::core::AggregationCacheOptions;
using ::teaql::core::Audited;
using ::teaql::core::BaseEntity;
using ::teaql::core::BaseEntityData;
using ::teaql::core::BatchInsertCommand;
using ::teaql::core::BatchUpdateCommand;
using ::teaql::core::BinaryOp;
using ::teaql::core::CompactRow;
using ::teaql::core::ContinuousPageFetchOptions;
using ::teaql::core::DataType;
using ::teaql::core::DebugAlignment;
using ::teaql::core::DebugNumbers;
using ::teaql::core::DebugOptions;
using ::teaql::core::DebugWriter;
using ::teaql::core::Decimal;
using ::teaql::core::DecimalInlineString;
using ::teaql::core::DeleteCommand;
using ::teaql::core::Entity;
using ::teaql::core::EntityDescriptor;
using ::teaql::core::EntityDescriptorStore;
using ::teaql::core::EntityError;
using ::teaql::core::EntityGraph;
using ::teaql::core::EntityGraphBuilder;
using ::teaql::core::EntityGraphNode;
using ::teaql::core::EntityGraphOperation;
using ::teaql::core::EntitySnapshot;
using ::teaql::core::Expr;
using ::teaql::core::ExprFunction;
using ::teaql::core::GeneratedValues;
using ::teaql::core::IdSetPaginationOptions;
using ::teaql::core::IdentifiableEntity;
using ::teaql::core::InsertCommand;
using ::teaql::core::MutationKind;
using ::teaql::core::MutationValues;
using ::teaql::core::NamedExpr;
using ::teaql::core::ObjectGroupBy;
using ::teaql::core::OrderBy;
using ::teaql::core::PARTITION_RANK_PROPERTY;
using ::teaql::core::PropertyDescriptor;
using ::teaql::core::RawSqlProjection;
using ::teaql::core::Record;
using ::teaql::core::RecoverCommand;
using ::teaql::core::RelationAggregate;
using ::teaql::core::RelationDescriptor;
using ::teaql::core::RelationLoad;
using ::teaql::core::STYLE_KEY;
using ::teaql::core::SafeExpression;
using ::teaql::core::SelectQuery;
using ::teaql::core::Slice;
using ::teaql::core::SmartList;
using ::teaql::core::SortDirection;
using ::teaql::core::StreamConfig;
using ::teaql::core::TeaqlBoxedRelations;
using ::teaql::core::TeaqlEmpty;
using ::teaql::core::TeaqlEntity;
using ::teaql::core::TraceKind;
using ::teaql::core::TraceNode;
using ::teaql::core::UpdateCommand;
using ::teaql::core::Value;
using ::teaql::core::VersionedEntity;
using ::teaql::core::WEB_RESPONSE_VERSION;
using ::teaql::core::WebAction;
using ::teaql::core::WebResponse;
using ::teaql::core::WebStyle;
using ::teaql::core::XlsBlock;
using ::teaql::core::XlsBlockBuildContext;
using ::teaql::core::XlsPage;
using ::teaql::core::XlsWorkbook;
using ::teaql::core::compact_row_to_json_value;
using ::teaql::core::debug_string;
using ::teaql::core::default_table_name;
using ::teaql::core::record_to_json_value;
using ::teaql::core::value_from_utc;
using ::teaql::core::write_debug;
}

export namespace teaql::eval {
using ::teaql::eval::EvalResult;
using ::teaql::eval::LoadState;
using ::teaql::eval::MissingPath;
using ::teaql::eval::write_debug;
}

export namespace teaql::request {
using ::teaql::request::COUNT_ALIAS;
using ::teaql::request::DateRange;
using ::teaql::request::EntityReference;
using ::teaql::request::FacetRequest;
using ::teaql::request::FieldOperator;
using ::teaql::request::ObjectGroupBy;
using ::teaql::request::QueryOptions;
using ::teaql::request::QuerySelection;
using ::teaql::request::RawDynamicProperty;
using ::teaql::request::RawProjection;
using ::teaql::request::RelationAggregate;
using ::teaql::request::RelationFilter;
using ::teaql::request::RelationSelection;
using ::teaql::request::TYPE_FIELD;
using ::teaql::request::TYPE_GROUP_FIELD;
using ::teaql::request::UnsafeRawSqlSegment;
using ::teaql::request::apply_relation_selections;
using ::teaql::request::apply_runtime_metadata;
using ::teaql::request::attach_facets;
using ::teaql::request::dynamic_json_filter_expr;
using ::teaql::request::dynamic_json_operator;
using ::teaql::request::dynamic_json_u64_field;
using ::teaql::request::dynamic_json_value_to_teaql_value;
using ::teaql::request::dynamic_json_values;
using ::teaql::request::entity_id_value;
using ::teaql::request::field_operator_column_expr;
using ::teaql::request::field_operator_expr;
using ::teaql::request::merge_outer_filter_into_facet_aggregates;
using ::teaql::request::remove_default_live_filter;
using ::teaql::request::remove_filter_expr;
using ::teaql::request::required_text;
using ::teaql::request::required_value;
using ::teaql::request::runtime_relation_aggregates;
using ::teaql::request::write_debug;
}

export namespace teaql::request::debug_detail {
using ::teaql::request::debug_detail::range_debug_element;
}

export namespace teaql::serde_utils {
using ::teaql::serde_utils::Error;
}

export namespace teaql::serde_utils::trimmed_string {
using ::teaql::serde_utils::trimmed_string::deserialize;
using ::teaql::serde_utils::trimmed_string::serialize;
}

export namespace teaql::serde_utils::trimmed_opt_string {
using ::teaql::serde_utils::trimmed_opt_string::deserialize;
using ::teaql::serde_utils::trimmed_opt_string::serialize;
}

export namespace teaql::core::time {
using ::teaql::core::time::Timestamp;
}

export namespace teaql::core::business_id {
using ::teaql::core::business_id::BUSINESS_ID_V1_DOMAIN_SIZE;
using ::teaql::core::business_id::BusinessIdAllocation;
using ::teaql::core::business_id::BusinessIdAllocator;
using ::teaql::core::business_id::BusinessIdDefinition;
using ::teaql::core::business_id::BusinessIdError;
using ::teaql::core::business_id::BusinessIdErrorCode;
using ::teaql::core::business_id::BusinessIdGenerationRequest;
using ::teaql::core::business_id::BusinessIdPlan;
using ::teaql::core::business_id::BusinessIdProfile;
using ::teaql::core::business_id::BusinessIdScope;
using ::teaql::core::business_id::BusinessIdSlot;
using ::teaql::core::business_id::BusinessIdValue;
using ::teaql::core::business_id::DEFAULT_BUSINESS_ID_DIGITS;
using ::teaql::core::business_id::DEFAULT_BUSINESS_ID_PROFILE;
using ::teaql::core::business_id::LEGACY_DAILY_SEQUENCE_PROFILE;
}

export namespace teaql::core {
namespace dynamic_search = ::teaql::dynamic_search;
namespace eval = ::teaql::eval;
namespace request = ::teaql::request;
namespace serde_utils = ::teaql::serde_utils;
}
