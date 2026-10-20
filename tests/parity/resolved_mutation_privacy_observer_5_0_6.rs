// Trusted fixture assembly and observations; no runtime algorithm is replaced.
fn apply_privacy<'a,E>(mut service:EntityDataService<'a,E>, mode:Option<&str>, replace:bool)
    ->EntityDataService<'a,E>
where E:teaql_data_service::QueryExecutor+teaql_data_service::MutationExecutor+Send+Sync {
    if let Some(mode)=mode.filter(|mode| *mode!="none") {
        let mut redactions=teaql_data_service::SqlIntentRedactions::default();
        if mode=="id" {
            redactions.capture_target_id(&teaql_core::Value::U64(7));
        } else if mode!="empty" {
            let source=teaql_data_service::SqlLogContext {
                parameter_policies:vec![match mode {
                    "masked"=>teaql_data_service::SqlParameterLogPolicy::Masked,
                    "credential"=>teaql_data_service::SqlParameterLogPolicy::Credential,
                    _=>teaql_data_service::SqlParameterLogPolicy::Unknown,
                }],..Default::default()
            };
            redactions=teaql_data_service::SqlIntentRedactions::from_bindings(
                &source,&[teaql_core::Value::Text("PRIVATE-OPERAND".into())],"");
        }
        service.query_log_intent=Some(std::sync::Mutex::new(redactions.clone()));
        service.data_service.mutation_privacy=Some(std::sync::Arc::new(redactions));
    }
    if replace {
        service=service.with_mutation_intent(teaql_core::MutationIntent::new("replacement event reason").unwrap());
    }
    service
}

fn privacy_event_out(event:&crate::RawAuditEvent)->serde_json::Value {
    let safe=event.build_safe_event(&[],None);
    serde_json::json!({
        "privacy":event.diagnostic_redactions.as_ref().map(|source|source.is_empty()),
        "fields":safe.fields.iter().map(|field|serde_json::json!({
            "name":field.name,"value":field.value,"masked":field.masked,"truncated":field.truncated,
            "raw_length":field.raw_length,"output_length":field.output_length,
            "mask_reason":field.mask_reason,"truncate_reason":field.truncate_reason,
        })).collect::<Vec<_>>(),
        "trace":trace_out(&safe.trace_chain),
    })
}
