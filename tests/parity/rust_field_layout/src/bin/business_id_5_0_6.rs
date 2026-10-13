use std::io::{self,BufRead};
use serde_json::{json,Value as Json};
use teaql_core::business_id::*;
fn scope(input:&Json)->BusinessIdScope {
    BusinessIdScope { domain_root_key:input[0].as_str().unwrap().into(),aggregate_type:input[1].as_str().unwrap().into(),namespace:input[2].as_str().unwrap().into(),period_key:input[3].as_str().unwrap().into() }
}
fn error(e:BusinessIdError)->Json { json!({"code":format!("{:?}",e.code),"message":e.message,"display":e.to_string(),"source_none":std::error::Error::source(&e).is_none()}) }

fn change_definition(value: &mut BusinessIdDefinition, field: i64) {
    match field {
        0 => value.field_name.push('!'), 1 => value.profile.push('!'),
        2 => value.prefix.push('!'), 3 => value.namespace.push('!'),
        4 => value.split_by_date = !value.split_by_date,
        5 => value.preserve_digits += 1, 6 => value.separator.push('!'), _ => {},
    }
}
fn change_scope(value: &mut BusinessIdScope, field: i64) {
    match field {
        0 => value.domain_root_key.push('!'), 1 => value.aggregate_type.push('!'),
        2 => value.namespace.push('!'), 3 => value.period_key.push('!'), _ => {},
    }
}
fn compare<T: Clone + PartialEq>(original: &T, changed: &T) -> Json {
    json!({"clone_equal": *original == original.clone(), "changed_equal": original == changed,
        "reverse_equal": changed == original})
}
fn equality(input: &Json) -> Json {
    let field = input["field"].as_i64().unwrap();
    let definition = BusinessIdDefinition::daily_sequence("field", "prefix", "namespace");
    let scope = scope(&json!(["root", "Aggregate", "namespace", "period"]));
    match input["model"].as_str().unwrap() {
        "definition" => {
            let mut changed = definition.clone(); change_definition(&mut changed, field);
            compare(&definition, &changed)
        },
        "scope" => { let mut changed = scope.clone(); change_scope(&mut changed, field); compare(&scope, &changed) },
        "request" => {
            let mut changed_definition = definition.clone();
            if field < 7 { change_definition(&mut changed_definition, field); }
            let date = chrono::NaiveDate::from_ymd_opt(2024, 2, 29).unwrap();
            let original = BusinessIdGenerationRequest { definition: &definition,
                domain_root_key: "root", aggregate_type: "Aggregate", business_date: date };
            let changed = BusinessIdGenerationRequest { definition: &changed_definition,
                domain_root_key: if field == 7 { "root!" } else { "root" },
                aggregate_type: if field == 8 { "Aggregate!" } else { "Aggregate" },
                business_date: if field == 9 { chrono::NaiveDate::from_ymd_opt(2024, 3, 1).unwrap() } else { date } };
            compare(&original, &changed)
        },
        "plan" => {
            let original = BusinessIdPlan { scope, prefix: "prefix".into(), date_text: None,
                preserve_digits: 6, separator: "-".into(), initial_sequence: 1, max_sequence: 99 };
            let mut changed = original.clone();
            if field < 4 { change_scope(&mut changed.scope, field); } else {
                match field { 4 => changed.prefix.push('!'), 5 => changed.date_text = Some(String::new()),
                    6 => changed.preserve_digits += 1, 7 => changed.separator.push('!'),
                    8 => changed.initial_sequence += 1, 9 => changed.max_sequence += 1, _ => {} }
            }
            compare(&original, &changed)
        },
        "allocation" => {
            let original = BusinessIdAllocation { scope, sequence: 7 };
            let mut changed = original.clone();
            if field < 4 { change_scope(&mut changed.scope, field); }
            else if field == 4 { changed.sequence += 1; }
            compare(&original, &changed)
        },
        "value" => { let original = BusinessIdValue("value".into()); let mut changed = original.clone();
            if field == 0 { changed.0.push('!'); } compare(&original, &changed) },
        "error" => { let original = BusinessIdError::new(BusinessIdErrorCode::Encoding, "message");
            let mut changed = original.clone();
            if field == 0 { changed.code = BusinessIdErrorCode::Allocation; }
            else if field == 1 { changed.message.push('!'); } compare(&original, &changed) },
        "code" => { let codes = [BusinessIdErrorCode::InvalidDefinition, BusinessIdErrorCode::InvalidFormat,
                BusinessIdErrorCode::Immutable, BusinessIdErrorCode::Exhausted, BusinessIdErrorCode::Allocation,
                BusinessIdErrorCode::KeyNotFound, BusinessIdErrorCode::Encoding];
            json!({"equal": codes[input["left"].as_u64().unwrap() as usize] == codes[input["right"].as_u64().unwrap() as usize]}) },
        _ => panic!("unknown business ID equality fixture"),
    }
}

fn evaluate(input:Json)->Json {
    match input["kind"].as_str().unwrap() {
        "equality" => equality(&input),
        "definition"=>{
            let mut definition=if input["legacy"]==true { BusinessIdDefinition::legacy_daily_sequence("business_id","订单","orders") } else { BusinessIdDefinition::daily_sequence("business_id","订单","orders") };
            if let Some(profile)=input["profile"].as_str() {definition.profile=profile.into();}
            if let Some(digits)=input["digits"].as_u64() {definition.preserve_digits=u8::try_from(digits).unwrap();}
            let range=match definition.max_sequence() {Ok(n)=>json!({"ok":n.to_string()}),Err(e)=>error(e)};
            json!({"field_name":definition.field_name,"profile":definition.profile,"prefix":definition.prefix,"namespace":definition.namespace,"split_by_date":definition.split_by_date,"preserve_digits":definition.preserve_digits,"separator":definition.separator,"range":range})
        },
        "scope"=>json!({"canonical":scope(&input["fields"]).canonical_key()}),
        "range"=>{
            let plan=BusinessIdPlan {scope:scope(&json!(["","","",""])),prefix:"".into(),date_text:None,preserve_digits:0,separator:"".into(),initial_sequence:input["initial"].as_str().unwrap().parse().unwrap(),max_sequence:input["maximum"].as_str().unwrap().parse().unwrap()};
            match plan.validate_allocation_range() {Ok(())=>json!({"ok":true}),Err(e)=>error(e)}
        },
        "error"=>{
            let codes=[BusinessIdErrorCode::InvalidDefinition,BusinessIdErrorCode::InvalidFormat,BusinessIdErrorCode::Immutable,BusinessIdErrorCode::Exhausted,BusinessIdErrorCode::Allocation,BusinessIdErrorCode::KeyNotFound,BusinessIdErrorCode::Encoding];
            error(BusinessIdError::new(codes[input["code"].as_u64().unwrap() as usize],input["message"].as_str().unwrap()))
        },
        _=>panic!("unknown business ID fixture"),
    }
}
fn main() { for line in io::stdin().lock().lines() { println!("{}",evaluate(serde_json::from_str(&line.unwrap()).unwrap())); } }
