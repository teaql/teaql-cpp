use crate::fixture_value::value;
use serde_json::Value as J;
use teaql_core::*;
use teaql_sql::*;
pub struct FixtureDialect {
    pub numbered: bool,
}
impl SqlDialect for FixtureDialect {
    fn kind(&self) -> DatabaseKind {
        DatabaseKind::Sqlite
    }
    fn quote_ident(&self, s: &str) -> String {
        quote_identifier_if_needed(s, '"')
    }
    fn placeholder(&self, i: usize) -> String {
        if self.numbered {
            format!("?{i}")
        } else {
            "?".into()
        }
    }
}
fn items<'a>(j: &'a J, key: &str) -> &'a [J] {
    j[key].as_array().map(Vec::as_slice).unwrap_or(&[])
}
pub fn uint(j: &J, key: &str) -> Option<u32> {
    j[key].as_u64().map(|v| v as u32)
}
fn datatype(i: u64) -> DataType {
    [
        DataType::Bool,
        DataType::I64,
        DataType::U64,
        DataType::F64,
        DataType::Decimal,
        DataType::Text,
        DataType::LargeText,
        DataType::Json,
        DataType::Date,
        DataType::Timestamp,
    ][i as usize]
}
pub fn entity(j: &J) -> EntityDescriptor {
    let mut e = EntityDescriptor::new(j["name"].as_str().unwrap());
    if let Some(v) = j["table"].as_str() {
        e.table_name = v.into();
    }
    e.audit_mask_fields_declared = j["declared"].as_bool().unwrap_or(false);
    e.audit_mask_fields = items(j, "masked")
        .iter()
        .map(|v| v.as_str().unwrap().into())
        .collect();
    for p in items(j, "properties") {
        let mut prop = PropertyDescriptor::new(
            p["name"].as_str().unwrap(),
            datatype(p["type"].as_u64().unwrap()),
        );
        if let Some(v) = p["column"].as_str() {
            prop.column_name = v.into();
        }
        prop.nullable = p["nullable"].as_bool().unwrap_or(true);
        prop.is_id = p["id"].as_bool().unwrap_or(false);
        prop.is_version = p["version"].as_bool().unwrap_or(false);
        prop.max_length = uint(p, "length");
        prop.numeric_precision = uint(p, "precision");
        prop.numeric_scale = uint(p, "scale");
        e.properties.push(prop);
    }
    e
}
fn op(i: u64) -> BinaryOp {
    [
        BinaryOp::Eq,
        BinaryOp::Ne,
        BinaryOp::Gt,
        BinaryOp::Gte,
        BinaryOp::Lt,
        BinaryOp::Lte,
        BinaryOp::Like,
        BinaryOp::NotLike,
        BinaryOp::In,
        BinaryOp::NotIn,
        BinaryOp::InLarge,
        BinaryOp::NotInLarge,
    ][i as usize]
}
pub fn expression(j: &J) -> Expr {
    let args = || items(j, "args").iter().map(expression).collect::<Vec<_>>();
    match j["kind"].as_str().unwrap() {
        "column" => Expr::column(j["name"].as_str().unwrap()),
        "value" => Expr::Value(value(&j["value"])),
        "binary" => Expr::Binary {
            left: Box::new(expression(&j["left"])),
            op: op(j["op"].as_u64().unwrap()),
            right: Box::new(expression(&j["right"])),
        },
        "function" => Expr::Function {
            function: [
                ExprFunction::Soundex,
                ExprFunction::Gbk,
                ExprFunction::Count,
                ExprFunction::Sum,
                ExprFunction::Avg,
                ExprFunction::Min,
                ExprFunction::Max,
                ExprFunction::Stddev,
                ExprFunction::StddevPop,
                ExprFunction::VarSamp,
                ExprFunction::VarPop,
                ExprFunction::BitAnd,
                ExprFunction::BitOr,
                ExprFunction::BitXor,
            ][j["function"].as_u64().unwrap() as usize],
            args: args(),
        },
        "subquery" => Expr::SubQuery {
            left: Box::new(expression(&j["left"])),
            op: op(j["op"].as_u64().unwrap()),
            entity: entity(&j["entity"]),
            query: Box::new(query(&j["query"])),
        },
        "between" => Expr::Between {
            expr: Box::new(expression(&j["args"][0])),
            lower: Box::new(expression(&j["args"][1])),
            upper: Box::new(expression(&j["args"][2])),
        },
        "null" => Expr::IsNull(Box::new(expression(&j["args"][0]))),
        "not_null" => Expr::IsNotNull(Box::new(expression(&j["args"][0]))),
        "not" => Expr::Not(Box::new(expression(&j["args"][0]))),
        "and" => Expr::And(args()),
        "or" => Expr::Or(args()),
        _ => panic!("unknown expression"),
    }
}
pub fn query(j: &J) -> SelectQuery {
    let mut q = SelectQuery::new("FixtureQuery");
    q.projection = items(j, "projection")
        .iter()
        .map(|v| v.as_str().unwrap().into())
        .collect();
    q.group_by = items(j, "group")
        .iter()
        .map(|v| v.as_str().unwrap().into())
        .collect();
    q.expr_projection = items(j, "expr_projection")
        .iter()
        .map(|p| NamedExpr::new(p["alias"].as_str().unwrap(), expression(&p["expr"])))
        .collect();
    let raw = |key| {
        items(j, key)
            .iter()
            .map(|p| RawSqlProjection {
                property_name: p["alias"].as_str().unwrap().into(),
                raw_sql_segment: p["sql"].as_str().unwrap().into(),
            })
            .collect()
    };
    q.raw_projections = raw("raw_projection");
    q.dynamic_properties = raw("dynamic");
    q.raw_sql_search_criteria = items(j, "raw_criteria")
        .iter()
        .map(|v| v.as_str().unwrap().into())
        .collect();
    q.aggregates = items(j, "aggregates")
        .iter()
        .map(|a| Aggregate {
            function: [
                AggregateFunction::Count,
                AggregateFunction::Sum,
                AggregateFunction::Avg,
                AggregateFunction::Min,
                AggregateFunction::Max,
                AggregateFunction::Stddev,
                AggregateFunction::StddevPop,
                AggregateFunction::VarSamp,
                AggregateFunction::VarPop,
                AggregateFunction::BitAnd,
                AggregateFunction::BitOr,
                AggregateFunction::BitXor,
            ][a["function"].as_u64().unwrap() as usize],
            field: a["field"].as_str().unwrap().into(),
            alias: a["alias"].as_str().unwrap().into(),
        })
        .collect();
    q.order_by = items(j, "order")
        .iter()
        .map(|o| OrderBy {
            field: o["field"].as_str().unwrap_or("").into(),
            expr: o.get("expr").map(expression),
            direction: if o["direction"] == 0 {
                SortDirection::Asc
            } else {
                SortDirection::Desc
            },
        })
        .collect();
    q.filter = j.get("filter").map(expression);
    q.having = j.get("having").map(expression);
    q.search_with_text = j["search"].as_str().map(str::to_owned);
    q.partition_by = j["partition"].as_str().map(str::to_owned);
    q.raw_sql = j["raw_sql"].as_str().map(str::to_owned);
    q.comment = j["comment"].as_str().map(str::to_owned);
    q.slice = j.get("slice").map(|s| Slice {
        offset: s["offset"].as_u64().unwrap(),
        limit: s["limit"].as_u64(),
    });
    q
}
