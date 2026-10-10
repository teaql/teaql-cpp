//! Actual pinned runtime graph model derives, independently of save execution.
use serde_json::{json, Value as J};
use std::{collections::BTreeMap, io::{self, BufRead}, sync::Arc};
use teaql_core::{EntitySnapshot, MutationValues, TraceKind, TraceNode, Value};
use teaql_runtime::{EntityValues, GraphMutationBatch, GraphMutationKind, GraphMutationPlan, GraphMutationPlanItem, GraphNode, GraphOperation, GraphTransactionBoundary, ScopedCommentNode, TraceScopeToken};
#[path = "../fixture_value.rs"] mod fixture_value;
fn formatted<T: std::fmt::Debug>(value: &T) -> J {
    json!({"compact":format!("{value:?}"),"pretty":format!("{value:#?}"),"lower_hex":format!("{value:x?}"),"upper_hex":format!("{value:X?}"),"pretty_lower_hex":format!("{value:#x?}"),"pretty_upper_hex":format!("{value:#X?}"),"width_precision":format!("{value:>40.2?}"),"fancy":format!("{value:🧊^+18.3?}"),"zero_pad":format!("{value:+012?}")})
}
fn record(input: &J) -> BTreeMap<String, Value> {
    input.as_object().unwrap().iter().map(|(key,value)|(key.clone(),fixture_value::value(value))).collect()
}
fn node(input: &J) -> GraphNode {
    let mut result = GraphNode::new(input["entity"].as_str().unwrap()).operation([GraphOperation::Upsert,GraphOperation::Create,GraphOperation::Reference,GraphOperation::Remove][input["operation"].as_u64().unwrap() as usize]);
    for (key,value) in record(&input["values"]) { result = result.value(key,value); }
    if let Some(comment) = input["comment"].as_str() { result.set_comment(comment); }
    result.dirty_fields = input["dirty"].as_array().map(|values|values.iter().map(|value|value.as_str().unwrap().to_owned()).collect());
    result.original_values = if input["old"].is_null() { None } else { Some(EntitySnapshot::from(record(&input["old"]))) };
    for (name,children) in input["relations"].as_object().unwrap() { result = result.relations(name,children.as_array().unwrap().iter().map(node)); }
    result
}
fn evaluate(input: J) -> J {
    let original = node(&input["node"]);
    let copied = original.clone();
    let mut changed = copied.clone();
    changed.set_comment("changed field");
    let values: EntityValues = record(&input["node"]["values"]).into();
    let mutation: MutationValues = values.clone().into();
    let roundtrip: EntityValues = mutation.clone().into();
    let mut tokens = Vec::new();
    for (index,comment) in input["trace"].as_array().unwrap().iter().enumerate() {
        tokens.push(Arc::new(TraceScopeToken { parent: tokens.last().cloned(), track: TraceNode::typed(TraceKind::Entity,"Parent",Some(index as u64),comment.as_str().unwrap()), node_index:index as u64 }));
    }
    let mut scopes = Vec::new();
    for (index,comment) in input["trace"].as_array().unwrap().iter().enumerate() {
        scopes.push(ScopedCommentNode { parent: None, track: TraceNode::typed(TraceKind::Entity,"Parent",Some(index as u64),comment.as_str().unwrap()) });
    }
    // Construct borrowed scope nodes recursively for the exact source Debug.
    fn scoped(index: usize, values: &[ScopedCommentNode<'_>], parent: Option<&ScopedCommentNode<'_>>) -> J {
        if index == values.len() { return parent.map(formatted).unwrap_or(J::Null); }
        let current = ScopedCommentNode { parent, track: values[index].track.clone() };
        scoped(index+1,values,Some(&current))
    }
    let kind = [GraphMutationKind::Create,GraphMutationKind::Update,GraphMutationKind::Delete,GraphMutationKind::Reference][input["kind"].as_u64().unwrap() as usize];
    let item = GraphMutationPlanItem { entity: original.entity.clone(), kind, values: mutation, update_fields: vec!["label".into(),"name🧊".into()], item_index: u64::MAX, scope_token: tokens.last().cloned(), old_values: original.original_values.clone() };
    let batch = GraphMutationBatch { entity: item.entity.clone(), kind, update_fields:item.update_fields.clone(), items: vec![item.clone()] };
    let mut plan = GraphMutationPlan::default();
    plan.planned_root = Some(original.clone()); plan.items.push(item.clone()); plan.batches.push(batch.clone()); plan.next_item_index = u64::MAX;
    if input["visited"] == true { plan.visited_nodes.insert(("Parent🧊".into(),"u:1".into())); }
    let mut different = plan.clone(); different.next_item_index = 0;
    let operation = [GraphOperation::Upsert,GraphOperation::Create,GraphOperation::Reference,GraphOperation::Remove][input["node"]["operation"].as_u64().unwrap() as usize];
    let boundary = [GraphTransactionBoundary::Started,GraphTransactionBoundary::AlreadyActive,GraphTransactionBoundary::Unsupported][input["boundary"].as_u64().unwrap() as usize];
    json!({"values":formatted(&values),"values_roundtrip_equal":values==roundtrip,"node":formatted(&original),"node_clone":formatted(&copied),"node_clone_equal":original==copied,"node_changed_equal":original==changed,"operation":formatted(&operation),"kind":formatted(&kind),"kind_for_update":[formatted(&GraphMutationKind::for_update(false)),formatted(&GraphMutationKind::for_update(true))],"boundary":formatted(&boundary),"token":tokens.last().map(|value|formatted(value.as_ref())),"scope":scoped(0,&scopes,None),"item":formatted(&item),"item_clone_equal":item==item.clone(),"batch":formatted(&batch),"batch_clone_equal":batch==batch.clone(),"plan":formatted(&plan),"plan_clone":formatted(&plan.clone()),"plan_clone_equal":plan==plan.clone(),"plan_changed_equal":plan==different,"default_plan":formatted(&GraphMutationPlan::default()),"default_values":formatted(&EntityValues::default())})
}
fn main() { for line in io::stdin().lock().lines() { println!("{}",evaluate(serde_json::from_str(&line.unwrap()).unwrap())); } }
