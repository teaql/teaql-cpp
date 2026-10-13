#pragma once
#include <teaql/core/business_id.hpp>
#include <teaql/core/request_intent.hpp>
#include <teaql/core/business_id_debug.hpp>
#include <teaql/core/chrono_value.hpp>
#include <teaql/core/debug.hpp>
#include <teaql/core/decimal_debug.hpp>
#include <teaql/core/decimal_format.hpp>
#include <teaql/core/dynamic_search.hpp>
#include <teaql/core/dynamic_properties.hpp>
#include <teaql/core/dynamic_fields.hpp>
#include <teaql/core/dynamic_fields_debug.hpp>
#include <teaql/core/dynamic_search_debug.hpp>
#include <teaql/core/entity.hpp>
#include <teaql/core/entity_debug.hpp>
#include <teaql/core/entity_graph.hpp>
#include <teaql/core/entity_graph_debug.hpp>
#include <teaql/core/eval.hpp>
#include <teaql/core/expr.hpp>
#include <teaql/core/list.hpp>
#include <teaql/core/load_state.hpp>
#include <teaql/core/meta.hpp>
#include <teaql/core/meta_debug.hpp>
#include <teaql/core/mutation.hpp>
#include <teaql/core/mutation_debug.hpp>
#include <teaql/core/presentation_debug.hpp>
#include <teaql/core/query.hpp>
#include <teaql/core/query_debug.hpp>
#include <teaql/core/request.hpp>
#include <teaql/core/request_debug.hpp>
#include <teaql/core/safe_expression.hpp>
#include <teaql/core/serde_utils.hpp>
#include <teaql/core/time.hpp>
#include <teaql/core/trace.hpp>
#include <teaql/core/value.hpp>
#include <teaql/core/web.hpp>
#include <teaql/core/xls.hpp>

// Match the Rust crate boundary without merging its graph builder with runtime.
namespace teaql::core {
using ::teaql::RequestKind;
using ::teaql::RequestIntentError;
using ::teaql::QueryIntent;
using ::teaql::MutationIntent;
using ::teaql::FieldLayout;
using ::teaql::RelationShapeCache;
using ::teaql::LoadedSnapshot;
using ::teaql::SnapshotPool;
using ::teaql::DynamicPropertyDefinitions;
using ::teaql::DynamicFieldError;
using ::teaql::DynamicFieldSelection;
using ::teaql::DynamicFieldDefinitions;
using ::teaql::DynamicFieldState;
using ::teaql::DynamicFieldMutation;
using ::teaql::DynamicFieldValue;
using ::teaql::DynamicFieldMergeShapes;
using ::teaql::DynamicFieldValues;

using ::teaql::value_from_utc;
using ::teaql::DebugAlignment;
using ::teaql::DebugNumbers;
using ::teaql::DebugOptions;
using ::teaql::DebugWriter;
using ::teaql::DecimalInlineString;
using ::teaql::debug_string;
using ::teaql::write_debug;
using ::teaql::dynamic_search::write_debug;
using ::teaql::Audited;
using ::teaql::BaseEntity;
using ::teaql::BaseEntityData;
using ::teaql::Entity;
using ::teaql::EntityDescriptorStore;
using ::teaql::EntityError;
using ::teaql::EntityJsonTraversal;
using ::teaql::IdentifiableEntity;
using ::teaql::TeaqlBoxedRelations;
using ::teaql::TeaqlEntity;
using ::teaql::VersionedEntity;
using ::teaql::BinaryOp;
using ::teaql::Expr;
using ::teaql::ExprFunction;
using ::teaql::SmartList;
using ::teaql::EntityDescriptor;
using ::teaql::PropertyDescriptor;
using ::teaql::RelationDescriptor;
using ::teaql::BatchInsertCommand;
using ::teaql::BatchUpdateCommand;
using ::teaql::DeleteCommand;
using ::teaql::EntitySnapshot;
using ::teaql::GeneratedValues;
using ::teaql::InsertCommand;
using ::teaql::MutationKind;
using ::teaql::MutationValues;
using ::teaql::RecoverCommand;
using ::teaql::UpdateCommand;
using ::teaql::default_table_name;
using ::teaql::Aggregate;
using ::teaql::AggregateFunction;
using ::teaql::AggregationCacheOptions;
using ::teaql::CompactRow;
using ::teaql::CompactRowLayout;
using ::teaql::ContinuousPageFetchOptions;
using ::teaql::IdSetPaginationOptions;
using ::teaql::NamedExpr;
using ::teaql::ObjectGroupBy;
using ::teaql::OrderBy;
using ::teaql::PARTITION_RANK_PROPERTY;
using ::teaql::RawSqlProjection;
using ::teaql::Record;
using ::teaql::RelationAggregate;
using ::teaql::RelationLoad;
using ::teaql::SelectQuery;
using ::teaql::Slice;
using ::teaql::SortDirection;
using ::teaql::StreamConfig;
using ::teaql::compact_row_to_json_value;
using ::teaql::record_to_json_value;
using ::teaql::SafeExpression;
using ::teaql::TeaqlEmpty;
using ::teaql::TraceKind;
using ::teaql::TraceNode;
using ::teaql::DataType;
using ::teaql::Decimal;
using ::teaql::Value;
using ::teaql::ACTION_LIST_KEY;
using ::teaql::STYLE_KEY;
using ::teaql::WEB_RESPONSE_VERSION;
using ::teaql::WebAction;
using ::teaql::WebResponse;
using ::teaql::WebStyle;
using ::teaql::XlsBlock;
using ::teaql::XlsBlockBuildContext;
using ::teaql::XlsPage;
using ::teaql::XlsWorkbook;
namespace dynamic_search = ::teaql::dynamic_search;
namespace eval = ::teaql::eval;
namespace request = ::teaql::request;
namespace serde_utils = ::teaql::serde_utils;
}

namespace teaql::core::time {
using ::teaql::Timestamp;
}

namespace teaql::core::business_id {
using ::teaql::DEFAULT_BUSINESS_ID_PROFILE;
using ::teaql::LEGACY_DAILY_SEQUENCE_PROFILE;
using ::teaql::DEFAULT_BUSINESS_ID_DIGITS;
using ::teaql::BUSINESS_ID_V1_DOMAIN_SIZE;
using ::teaql::BusinessIdErrorCode;
using ::teaql::BusinessIdError;
using ::teaql::BusinessIdDefinition;
using ::teaql::BusinessIdScope;
using ::teaql::BusinessIdGenerationRequest;
using ::teaql::BusinessIdPlan;
using ::teaql::BusinessIdAllocation;
using ::teaql::BusinessIdValue;
using ::teaql::BusinessIdAllocator;
using ::teaql::BusinessIdProfile;
using ::teaql::BusinessIdSlot;
}

namespace teaql::dynamic_properties {
using ::teaql::DynamicPropertyDefinitions;
}

namespace teaql::core::dynamic_properties {
using ::teaql::DynamicPropertyDefinitions;
}

namespace teaql::dynamic_fields {
using ::teaql::DynamicFieldError;
using ::teaql::DynamicFieldSelection;
using ::teaql::DynamicFieldDefinitions;
using ::teaql::DynamicFieldState;
using ::teaql::DynamicFieldMutation;
using ::teaql::DynamicFieldValue;
using ::teaql::DynamicFieldMergeShapes;
using ::teaql::DynamicFieldValues;
}

namespace teaql::core::dynamic_fields {
using ::teaql::DynamicFieldError;
using ::teaql::DynamicFieldSelection;
using ::teaql::DynamicFieldDefinitions;
using ::teaql::DynamicFieldState;
using ::teaql::DynamicFieldMutation;
using ::teaql::DynamicFieldValue;
using ::teaql::DynamicFieldMergeShapes;
using ::teaql::DynamicFieldValues;
}
