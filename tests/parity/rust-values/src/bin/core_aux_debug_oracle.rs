#![allow(deprecated)]
use chrono::NaiveDate;
use rust_decimal::{Error, RoundingStrategy};
use serde_json::{json, Value as Json};
use std::io::{self, BufRead};
use teaql_core::dynamic_search::*;
use teaql_core::business_id::*;
use teaql_core::*;

fn formatted<T: std::fmt::Debug>(value: &T) -> Json {
    json!({"compact":format!("{value:?}"),"pretty":format!("{value:#?}"),
        "lower_hex":format!("{value:x?}"),"upper_hex":format!("{value:X?}"),
        "pretty_lower_hex":format!("{value:#x?}"),"pretty_upper_hex":format!("{value:#X?}"),
        "width_precision":format!("{value:>40.2?}"),"fancy":format!("{value:🧊^+18.3?}"),
        "zero_pad":format!("{value:+012?}")})
}

fn evaluate(input: Json) -> Json {
    let flags = input["flags"].as_u64().unwrap();
    let text = input["text"].as_str().unwrap();
    let number = input["number"].as_u64().unwrap();
    let codes = [BusinessIdErrorCode::InvalidDefinition, BusinessIdErrorCode::InvalidFormat,
        BusinessIdErrorCode::Immutable, BusinessIdErrorCode::Exhausted,
        BusinessIdErrorCode::Allocation, BusinessIdErrorCode::KeyNotFound, BusinessIdErrorCode::Encoding];
    let error = BusinessIdError::new(codes[flags as usize % codes.len()], text);
    let mut definition = if flags & 1 == 0 {
        BusinessIdDefinition::daily_sequence(text, "prefix", "namespace")
    } else {
        BusinessIdDefinition::legacy_daily_sequence(text, "prefix", "namespace")
    };
    definition.split_by_date = flags & 2 != 0;
    definition.preserve_digits = input["digits"].as_u64().unwrap() as u8;
    definition.separator = text.to_owned();
    let scope = BusinessIdScope { domain_root_key: text.into(), aggregate_type: "Aggregate".into(),
        namespace: "namespace".into(), period_key: text.into() };
    let request = BusinessIdGenerationRequest { definition: &definition, domain_root_key: text,
        aggregate_type: "Aggregate", business_date: NaiveDate::parse_from_str(input["date"].as_str().unwrap(), "%Y-%m-%d").unwrap() };
    let plan = BusinessIdPlan { scope: scope.clone(), prefix: text.into(),
        date_text: if flags & 4 == 0 {None} else {Some(text.into())}, preserve_digits: definition.preserve_digits,
        separator: text.into(), initial_sequence: flags, max_sequence: number };
    let allocation = BusinessIdAllocation { scope: scope.clone(), sequence: number };
    let id = BusinessIdValue(text.into());
    let model = SearchModel { fields: std::collections::BTreeMap::from([(text.into(), "string".into())]),
        relations: std::collections::BTreeMap::from([("child".into(),text.into())]) };
    let warning = DynamicSearchWarning {code:"WarningCode",entity:text.into(),clause:"filter",field_path:text.into()};
    let filter = DynamicSearchFilter {field_path:text.into(),operator:"$eq".into(),value:input["value"].clone()};
    let order = DynamicSearchOrder {field_path:text.into(),descending:flags & 1 != 0};
    let normalized = NormalizedDynamicSearch {
        filters: if flags & 8 == 0 {vec![]} else {vec![filter.clone(),filter.clone()]},
        orders: vec![order.clone()], warnings: vec![warning.clone()] };
    let result = DynamicSearchResult {query:SelectQuery::new(text).comment(text),warnings:vec![warning.clone()]};
    let search_error = DynamicSearchError("Invalid trusted search setup");
    let words = input["words"].as_array().unwrap();
    // The result type is public through unpack(), but has no crate-root reexport.
    let mut unpacked = Decimal::ZERO.unpack();
    unpacked.negative = flags & 1 != 0;
    unpacked.scale = input["scale"].as_u64().unwrap() as u32;
    unpacked.lo = words[0].as_u64().unwrap() as u32;
    unpacked.mid = words[1].as_u64().unwrap() as u32;
    unpacked.hi = words[2].as_u64().unwrap() as u32;
    let errors = [Error::ErrorString(text.into()),Error::ExceedsMaximumPossibleValue,Error::LessThanMinimumPossibleValue,
        Error::Underflow,Error::ScaleExceedsMaximumPrecision(unpacked.scale),Error::ConversionTo(text.into())];
    let rounding = [RoundingStrategy::MidpointNearestEven,RoundingStrategy::MidpointAwayFromZero,
        RoundingStrategy::MidpointTowardZero,RoundingStrategy::ToZero,RoundingStrategy::AwayFromZero,
        RoundingStrategy::ToNegativeInfinity,RoundingStrategy::ToPositiveInfinity,RoundingStrategy::BankersRounding,
        RoundingStrategy::RoundHalfUp,RoundingStrategy::RoundHalfDown,RoundingStrategy::RoundDown,RoundingStrategy::RoundUp];
    json!({"codes":codes.iter().map(formatted).collect::<Vec<_>>(),"error":formatted(&error),
        "definition":formatted(&definition),"scope":formatted(&scope),"request":formatted(&request),
        "plan":formatted(&plan),"allocation":formatted(&allocation),"id":formatted(&id),
        "model":formatted(&model),"warning":formatted(&warning),"filter":formatted(&filter),
        "order":formatted(&order),"normalized":formatted(&normalized),"result":formatted(&result),
        "search_error":formatted(&search_error),"unpacked":formatted(&unpacked),
        "decimal_errors":errors.iter().map(formatted).collect::<Vec<_>>(),
        "decimal_error_sources":errors.iter().map(|e| std::error::Error::source(e).is_none()).collect::<Vec<_>>(),
        "decimal_error_displays":errors.iter().map(ToString::to_string).collect::<Vec<_>>(),
        "decimal_error_clones":errors.iter().map(|e| *e == e.clone()).collect::<Vec<_>>(),
        "decimal_error_formats":errors.iter().map(|e| json!([
            format!("{e}"), format!("{e:>40.2}"), format!("{e:🧊^+18.3}"),
            format!("{e:+012}"), format!("{e:_<12.3}"), format!("{e:_>12.3}"),
            format!("{e:_^12.3}"), format!("{e:.0}"), format!("{e:.1}")
        ])).collect::<Vec<_>>(),
        "rounding":rounding.iter().map(formatted).collect::<Vec<_>>(),"warning_json":warning,
        "clone_equal":[error==error.clone(),definition==definition.clone(),scope==scope.clone(),
            request==request.clone(),plan==plan.clone(),allocation==allocation.clone(),id==id.clone(),
            warning==warning.clone(),filter==filter.clone(),order==order.clone(),search_error==search_error.clone(),unpacked==unpacked.clone()]})
}

fn main() {
    for line in io::stdin().lock().lines() {
        println!("{}",evaluate(serde_json::from_str(&line.unwrap()).unwrap()));
    }
}
