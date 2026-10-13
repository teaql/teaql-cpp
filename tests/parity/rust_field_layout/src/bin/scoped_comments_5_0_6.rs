//! Compare original stack and persistent comment scopes directly.
use serde_json::{Value as Json, json};
use std::io::{self, BufRead};
use std::sync::Arc;
use teaql_core::{TraceKind, TraceNode};
use teaql_runtime::{ScopedCommentNode, TraceScopeToken};

fn stack(nodes: &[TraceNode], parent: Option<&ScopedCommentNode<'_>>) -> Vec<TraceNode> {
    if let Some((first, rest)) = nodes.split_first() {
        let node = ScopedCommentNode {
            parent,
            track: first.clone(),
        };
        if rest.is_empty() {
            node.to_trace_chain()
        } else {
            stack(rest, Some(&node))
        }
    } else {
        Vec::new()
    }
}

fn record(nodes: Vec<TraceNode>) -> Json {
    json!(
        nodes
            .into_iter()
            .map(|node| json!({"kind":format!("{:?}",node.kind),
        "entity":node.entity_type,"id":node.entity_id,"comment":node.comment}))
            .collect::<Vec<_>>()
    )
}

fn main() {
    let kinds = [
        TraceKind::Operation,
        TraceKind::Request,
        TraceKind::Relation,
        TraceKind::Entity,
        TraceKind::Provider,
        TraceKind::Sql,
        TraceKind::Comment,
        TraceKind::Purpose,
        TraceKind::AuditReason,
    ];
    for line in io::stdin().lock().lines() {
        let input: Json = serde_json::from_str(&line.unwrap()).unwrap();
        let nodes: Vec<_> = input
            .as_array()
            .unwrap()
            .iter()
            .enumerate()
            .map(|(i, value)| {
                TraceNode::typed(
                    kinds[i % kinds.len()].clone(),
                    format!("Entity{i}"),
                    if i % 2 == 0 { Some(i as u64) } else { None },
                    value.as_str().unwrap(),
                )
            })
            .collect();
        let mut parent = None;
        for (i, node) in nodes.iter().enumerate() {
            parent = Some(Arc::new(TraceScopeToken {
                parent,
                track: node.clone(),
                node_index: i as u64,
            }));
        }
        let persistent = parent
            .as_ref()
            .map(|node| node.recover_trace_chain())
            .unwrap_or_default();
        println!(
            "{}",
            json!({"stack":record(stack(&nodes,None)),"persistent":record(persistent)})
        );
    }
}
