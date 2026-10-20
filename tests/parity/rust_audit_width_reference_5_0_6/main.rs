//! Observe the actual host parser and the same usize quoting used by derive_impl.
use quote::quote;
use serde_json::{Value, json};
use std::io::{self, BufRead};

#[path = "../../../.reference/teaql-rs/teaql-macros/src/attr.rs"]
mod original;

fn main() {
    for line in io::stdin().lock().lines() {
        let value: String = serde_json::from_str(&line.unwrap()).unwrap();
        let text = format!("#[teaql(audit_value_max_len={value})] struct Probe {{}}");
        let input: syn::DeriveInput = syn::parse_str(&text).unwrap();
        let attrs = original::parse_container_attrs(&input.attrs, "Probe");
        let limit = attrs.audit_value_max_len;
        let literal = limit.map(|number| quote!(#number).to_string());
        let output: Value = json!({"limit": limit.map(|n| n.to_string()), "literal": literal});
        println!("{output}");
    }
}
