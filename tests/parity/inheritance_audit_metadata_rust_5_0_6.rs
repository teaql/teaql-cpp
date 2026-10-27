use cpp_inheritance_graph_core::{Academy, ResearchAcademy, School};
use serde_json::{Value as Json, json};
use teaql_core::{EntityDescriptor, Record, TeaqlEntity, Value};
use teaql_data_service::SqlIntentRedactions;
use teaql_runtime::RawAuditEvent;

fn describe(descriptor: EntityDescriptor) -> Json {
    json!({
        "entity": descriptor.name,
        "table": descriptor.table_name,
        "service": descriptor.data_service,
        "limit": descriptor.audit_value_max_len.map(|value| value.to_string()),
        "masks": descriptor.audit_mask_fields,
        "declared": descriptor.audit_mask_fields_declared,
        "name_sql_policy": format!("{:?}", SqlIntentRedactions::field_policy(&descriptor, "name")),
        "password_sql_policy": format!("{:?}", SqlIntentRedactions::field_policy(&descriptor, "password")),
    })
}

fn main() {
    let school = School::entity_descriptor();
    let values = Record::from([(
        "name".to_owned(),
        Value::Text("PRIVATE-SCHOOL-CANARY".to_owned()),
    )]);
    let event = RawAuditEvent::created("School", values);
    let safe = event.build_safe_event(&school.audit_mask_fields, school.audit_value_max_len);
    println!(
        "{}",
        json!({
            "descriptors": [
                describe(school),
                describe(Academy::entity_descriptor()),
                describe(ResearchAcademy::entity_descriptor()),
            ],
            "safe_audit": safe.fields.iter().map(|field| json!({
                "name": field.name,
                "value": field.value,
                "masked": field.masked,
                "truncated": field.truncated,
                "raw_length": field.raw_length,
                "output_length": field.output_length,
                "mask_reason": field.mask_reason,
                "truncate_reason": field.truncate_reason,
            })).collect::<Vec<_>>(),
        })
    );
}
