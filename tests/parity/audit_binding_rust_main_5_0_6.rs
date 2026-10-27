use serde_json::json;
use teaql_core::{TeaqlEntity, Value};
use teaql_data_service::{SqlIntentRedactions, SqlLogContext};

fn main() {
    let mut records = Vec::new();
    for descriptor in [
        Platform::entity_descriptor(),
        Item::entity_descriptor(),
        Child::entity_descriptor(),
    ] {
        let mut policies = Vec::new();
        let mut binding_redactions = Vec::new();
        for property in &descriptor.properties {
            let policy = SqlIntentRedactions::field_policy(&descriptor, &property.name);
            policies.push(json!([
                property.name,
                property.column_name,
                format!("{policy:?}")
            ]));

            let mut context = SqlLogContext {
                generated_sql: true,
                ..Default::default()
            };
            context.parameter_policies.push(policy);
            let canary = format!("TEAQL-PROBE-{}", property.name);
            let redactions =
                SqlIntentRedactions::from_bindings(&context, &[Value::Text(canary)], "SELECT ?");
            let mut safe = Vec::new();
            redactions.extend_secrets(false, &mut safe);
            let mut debug = Vec::new();
            redactions.extend_secrets(true, &mut debug);
            binding_redactions.push(json!([property.name, safe, debug]));
        }
        records.push(json!({
            "entity": descriptor.name,
            "table": descriptor.table_name,
            "service": descriptor.data_service,
            "masks": descriptor.audit_mask_fields,
            "declared": descriptor.audit_mask_fields_declared,
            "limit": descriptor.audit_value_max_len.map(|value| value.to_string()),
            "policies": policies,
            "binding_redactions": binding_redactions,
        }));
    }
    println!("{}", json!(records));
}
