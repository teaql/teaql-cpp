//! Adapted source execution only: original model/checker method bodies are retained.
use cpp_inheritance_business_ids_core::teaql_core::{CompactRow, Entity, Value};
use cpp_inheritance_business_ids_core::*;
use serde_json::json;

fn main() {
    let _ = Q::documents_with_children();
    assert!(DocumentNumber::new(" \t\u{3000}\u{a0}").is_err());
    let first = DocumentNumber::new(" \t\u{3000}DOC-20261014-ABC123\u{a0} ").unwrap();
    assert_eq!(first.as_str(), "DOC-20261014-ABC123");
    let second = DocumentNumber::new("replacement").unwrap();
    let mut results = vec![];
    macro_rules! check_model {
        ($model:ty, $expression:ident) => {{
            let row = CompactRow::from_map([
                ("id".to_owned(), Value::U64(7)),
                ("version".to_owned(), Value::I64(1)),
            ].into());
            let mut model = <$model>::from_compact_row(row).unwrap();
            assert!(model.document_number().as_str().is_empty());
            assert!(matches!(model.eval_document_number(),
                cpp_inheritance_business_ids_core::teaql_core::eval::EvalResult::NotLoaded { .. }));
            let typed: DocumentNumber = model.document_number();
            assert!(typed.as_str().is_empty());
            model.update_document_number(first.clone());
            assert_eq!(model.document_number(), first);
            assert_eq!(model.changed_document_number().unwrap().try_text(), Some(first.as_str()));
            assert_eq!(E::$expression(&model).get_document_number().unwrap(), first);
            let copied = model.clone();
            model.update_document_number(second.clone());
            // Runtime state is shared by clones in the original source.
            assert_eq!(copied.document_number(), second);
            assert_eq!(model.document_number(), second);
            results.push(json!({"entity": stringify!($model), "shared_type": true,
                "initial_not_loaded": true, "value": first.as_str(),
                "after_shared_update": copied.document_number().as_str()}));
        }};
    }
    check_model!(Document, document);
    check_model!(IntermediateRecord, intermediate_record);
    check_model!(Invoice, invoice);
    check_model!(Receipt, receipt);
    check_model!(SpecialInvoice, special_invoice);
    println!(
        "{}",
        json!({"adapted_source_only": true, "models": results})
    );
}
