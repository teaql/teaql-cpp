//! Actual generated combined scalars, model input boundaries and alias geometry.
use cpp_sample_runtime_core::{SampleRecord,SampleLink};
use cpp_member_aliases_core::Record;
use serde_json::{json,Value as J};
use std::{io::{self,BufRead},sync::Arc};
use teaql_core::{Entity,TeaqlEntity,DataType};
use teaql_runtime::{UserContext,InMemoryMetadataStore};
fn run(input:&J)->J {
    match input["case"].as_str().unwrap() {
        "combined" => {
            let context=cpp_sample_runtime_core::module().into_context();
            let value=json!({"id":1,"version":7,"optional_count":null,"quantity":0,"active":false,
                "height":3.5,"amount":"123.450","day":"2024-02-29","recorded_at":1700000000123_i64,
                "payload":{"literal":"_comment"},"clock_text":"","_name":"derived","_nil":null,"_count":0});
            let row=context.decode_json_entity::<SampleRecord>(&value).unwrap();
            let out=row.clone().into_json();
            let again=context.decode_json_entity::<SampleRecord>(&out).unwrap().into_json();
            json!({"json":out,"roundtrip":again,"null_loaded":row.is_field_loaded("optional_count"),
                "omitted_loaded":row.is_field_loaded("optional_active"),"quantity":row.quantity(),"active":row.active(),
                "height":row.height(),"amount":row.amount().to_string(),"day":row.day().to_string(),
                "timestamp":row.recorded_at().0,"clock":row.clock_text(),"dirty":row.dirty_fields(),
                "new":row.is_new(),"deleted":row.is_marked_as_delete(),"dynamic_pending":row.has_pending_dynamic_mutations()})
        },
        "rejections" => {
            let context=cpp_sample_runtime_core::module().into_context();
            let mut errors=Vec::new();
            for value in [json!({"unknown":"PRIVATE_LITERAL"}),json!({"label":42}),json!({"active":null}),
                json!({"quantity":1.5}),json!({"id":-1}),json!({"day":"PRIVATE_LITERAL"}),
                json!({"_original_values":{}}),json!({"#label":"PRIVATE_LITERAL"}),json!({"platform":{"id":1,"name":"PRIVATE_LITERAL"}})] {
                errors.push(match context.decode_json_entity::<SampleRecord>(&value){Ok(_)=>panic!("invalid input accepted"),Err(e)=>e.to_string()});
            }
            let missing=UserContext::new();
            errors.push(match missing.decode_json_entity::<SampleRecord>(&json!({"id":1})){Ok(_)=>panic!("uninstalled model accepted"),Err(e)=>e.to_string()});
            let mut descriptor=SampleRecord::entity_descriptor();descriptor.properties[0].data_type=DataType::Text;
            let wrong=UserContext::new().with_metadata(InMemoryMetadataStore::new().with_entity(descriptor));
            errors.push(match wrong.decode_json_entity::<SampleRecord>(&json!({"id":"PRIVATE_LITERAL"})){Ok(_)=>panic!("wrong model accepted"),Err(e)=>e.to_string()});
            json!({"errors":errors})
        },
        "target" => {
            let mut descriptor=SampleLink::entity_descriptor();descriptor.properties[0].data_type=DataType::Text;
            let mut context=cpp_sample_runtime_core::module().into_context();
            context.set_metadata(InMemoryMetadataStore::new().with_entity(SampleRecord::entity_descriptor()).with_entity(descriptor));
            let value=json!({"id":1,"first_links":[]});
            let incompatible=match context.decode_json_entity::<SampleRecord>(&value){Ok(_)=>panic!("wrong target accepted"),Err(e)=>e.to_string()};
            let missing=UserContext::new().with_metadata(InMemoryMetadataStore::new()
                .with_entity(SampleRecord::entity_descriptor()).with_entity(SampleLink::entity_descriptor()));
            let unregistered=match missing.decode_json_entity::<SampleRecord>(&value){Ok(_)=>panic!("missing decoder accepted"),Err(e)=>e.to_string()};
            json!({"incompatible":incompatible,"unregistered":unregistered})
        },
        "aliases" => {
            let context=cpp_member_aliases_core::module().into_context();
            let n=input["count"].as_u64().unwrap() as usize;
            let values:Vec<J>=(0..n).map(|i|if i%2==0 {json!({"id":1,"version":7,"type":null})}else{json!({"legacy_type":"private","version":7,"id":2})}).collect();
            let mut values=values;values.push(json!({"id":3,"version":7}));
            let rows=context.decode_json_entities::<Record>(&json!(values)).unwrap();
            let first=rows[0].loaded_state_snapshot().unwrap();
            let same=rows[..n].iter().all(|r|Arc::ptr_eq(&first,&r.loaded_state_snapshot().unwrap()));
            let distinct=!Arc::ptr_eq(&first,&rows[n].loaded_state_snapshot().unwrap());
            let duplicate=match context.decode_json_entity::<Record>(&json!({"type":"A","legacy_type":"B"})){Ok(_)=>panic!("duplicate alias accepted"),Err(e)=>e.to_string()};
            json!({"count":n,"same":same,"distinct":distinct,"first_loaded":rows[0].is_field_loaded("type"),
                "other_loaded":rows[n].is_field_loaded("type"),"first":rows[0].clone().into_json(),
                "last":rows[n-1].clone().into_json(),"other":rows[n].clone().into_json(),"duplicate":duplicate})
        }, _=>panic!("unknown fixture"),
    }
}
fn main(){for line in io::stdin().lock().lines(){let input:J=serde_json::from_str(&line.unwrap()).unwrap();println!("{}",run(&input));}}
