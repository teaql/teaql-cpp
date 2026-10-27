use serde_json::{Value, json};
use std::io::{self, BufRead};
use syn::parse::Parser;
mod original {
    include!("../../.reference/teaql-rs/teaql-macros/src/attr.rs");
}
fn expr_kind(value: &syn::Expr) -> &'static str {
    match value {
        syn::Expr::Array(..) => "Array",
        syn::Expr::Assign(..) => "Assign",
        syn::Expr::Async(..) => "Async",
        syn::Expr::Await(..) => "Await",
        syn::Expr::Binary(..) => "Binary",
        syn::Expr::Block(..) => "Block",
        syn::Expr::Break(..) => "Break",
        syn::Expr::Call(..) => "Call",
        syn::Expr::Cast(..) => "Cast",
        syn::Expr::Closure(..) => "Closure",
        syn::Expr::Const(..) => "Const",
        syn::Expr::Continue(..) => "Continue",
        syn::Expr::Field(..) => "Field",
        syn::Expr::ForLoop(..) => "ForLoop",
        syn::Expr::Group(..) => "Group",
        syn::Expr::If(..) => "If",
        syn::Expr::Index(..) => "Index",
        syn::Expr::Infer(..) => "Infer",
        syn::Expr::Let(..) => "Let",
        syn::Expr::Lit(..) => "Lit",
        syn::Expr::Loop(..) => "Loop",
        syn::Expr::Macro(..) => "Macro",
        syn::Expr::Match(..) => "Match",
        syn::Expr::MethodCall(..) => "MethodCall",
        syn::Expr::Paren(..) => "Paren",
        syn::Expr::Path(..) => "Path",
        syn::Expr::Range(..) => "Range",
        syn::Expr::RawAddr(..) => "RawAddr",
        syn::Expr::Reference(..) => "Reference",
        syn::Expr::Repeat(..) => "Repeat",
        syn::Expr::Return(..) => "Return",
        syn::Expr::Struct(..) => "Struct",
        syn::Expr::Try(..) => "Try",
        syn::Expr::TryBlock(..) => "TryBlock",
        syn::Expr::Tuple(..) => "Tuple",
        syn::Expr::Unary(..) => "Unary",
        syn::Expr::Unsafe(..) => "Unsafe",
        syn::Expr::Verbatim(..) => "Verbatim",
        syn::Expr::While(..) => "While",
        syn::Expr::Yield(..) => "Yield",
        _ => "Unknown",
    }
}
fn type_kind(value: &syn::Type) -> &'static str {
    match value {
        syn::Type::Array(..) => "Array",
        syn::Type::BareFn(..) => "BareFn",
        syn::Type::Group(..) => "Group",
        syn::Type::ImplTrait(..) => "ImplTrait",
        syn::Type::Infer(..) => "Infer",
        syn::Type::Macro(..) => "Macro",
        syn::Type::Never(..) => "Never",
        syn::Type::Paren(..) => "Paren",
        syn::Type::Path(..) => "Path",
        syn::Type::Ptr(..) => "Ptr",
        syn::Type::Reference(..) => "Reference",
        syn::Type::Slice(..) => "Slice",
        syn::Type::TraitObject(..) => "TraitObject",
        syn::Type::Tuple(..) => "Tuple",
        syn::Type::Verbatim(..) => "Verbatim",
        _ => "Unknown",
    }
}
fn pat_kind(value: &syn::Pat) -> &'static str {
    match value {
        syn::Pat::Const(..) => "Const",
        syn::Pat::Ident(..) => "Ident",
        syn::Pat::Lit(..) => "Lit",
        syn::Pat::Macro(..) => "Macro",
        syn::Pat::Or(..) => "Or",
        syn::Pat::Paren(..) => "Paren",
        syn::Pat::Path(..) => "Path",
        syn::Pat::Range(..) => "Range",
        syn::Pat::Reference(..) => "Reference",
        syn::Pat::Rest(..) => "Rest",
        syn::Pat::Slice(..) => "Slice",
        syn::Pat::Struct(..) => "Struct",
        syn::Pat::Tuple(..) => "Tuple",
        syn::Pat::TupleStruct(..) => "TupleStruct",
        syn::Pat::Type(..) => "Type",
        syn::Pat::Verbatim(..) => "Verbatim",
        syn::Pat::Wild(..) => "Wild",
        _ => "Unknown",
    }
}
fn stmt_kind(value: &syn::Stmt) -> &'static str {
    match value {
        syn::Stmt::Local(..) => "Local",
        syn::Stmt::Item(..) => "Item",
        syn::Stmt::Expr(..) => "Expr",
        syn::Stmt::Macro(..) => "Macro",
        _ => "Unknown",
    }
}
fn item_kind(value: &syn::Item) -> &'static str {
    match value {
        syn::Item::Const(..) => "Const",
        syn::Item::Enum(..) => "Enum",
        syn::Item::ExternCrate(..) => "ExternCrate",
        syn::Item::Fn(..) => "Fn",
        syn::Item::ForeignMod(..) => "ForeignMod",
        syn::Item::Impl(..) => "Impl",
        syn::Item::Macro(..) => "Macro",
        syn::Item::Mod(..) => "Mod",
        syn::Item::Static(..) => "Static",
        syn::Item::Struct(..) => "Struct",
        syn::Item::Trait(..) => "Trait",
        syn::Item::TraitAlias(..) => "TraitAlias",
        syn::Item::Type(..) => "Type",
        syn::Item::Union(..) => "Union",
        syn::Item::Use(..) => "Use",
        syn::Item::Verbatim(..) => "Verbatim",
        _ => "Unknown",
    }
}

fn witness(input: &Value) -> Value {
    if input.is_null() {
        return Value::Null;
    }
    let code = input["code"].as_str().unwrap();
    let value = match input["kind"].as_str().unwrap() {
        "Expr" => syn::parse_str::<syn::Expr>(code).map(|v| expr_kind(&v).to_owned()),
        "Type" => syn::parse_str::<syn::Type>(code).map(|v| type_kind(&v).to_owned()),
        "Pat" if input["variant"] == "Type" => syn::parse_str::<syn::Expr>(&format!("|{code}| x"))
            .map(|v| {
                let syn::Expr::Closure(v) = v else {
                    panic!("Expected a typed closure witness");
                };
                pat_kind(v.inputs.first().unwrap()).to_owned()
            }),
        "Pat" => syn::Pat::parse_multi_with_leading_vert
            .parse_str(code)
            .map(|v| pat_kind(&v).to_owned()),
        "Stmt" => syn::parse_str::<syn::Block>(&format!("{{ {code} }}"))
            .map(|v| stmt_kind(v.stmts.first().unwrap()).to_owned()),
        "Item" => syn::parse_str::<syn::Item>(code).map(|v| item_kind(&v).to_owned()),
        _ => panic!("Unknown witness category"),
    };
    match value {
        Ok(v) => json!({"variant":v}),
        Err(e) => json!({"error":e.to_string()}),
    }
}

fn metadata(input: &Value) -> Value {
    let text = match input.as_str() {
        Some(value) => format!("#[teaql(audit_value_max_len={value})] struct Probe {{}}"),
        None => "struct Probe {}".into(),
    };
    match syn::parse_str::<syn::DeriveInput>(&text) {
        Ok(input) => match std::panic::catch_unwind(std::panic::AssertUnwindSafe(|| {
            original::parse_container_attrs(&input.attrs, "Probe")
        })) {
            Ok(attrs) => json!({"parsed":true,"entity":attrs.entity_name,"table":attrs.table_name,
                "service":attrs.data_service,"indexed":attrs.indexed_layout,
                "limit":attrs.audit_value_max_len.map(|n|n.to_string()),
                "masks":attrs.audit_mask_fields,"declared":attrs.audit_mask_fields_declared,
                "relations":attrs.reverse_relations.iter().map(|r| json!({
                    "name":r.name,"target":r.target,"local":r.local_key.as_deref().unwrap_or("id"),
                    "foreign":r.foreign_key.as_deref().unwrap_or("id"),"many":r.many,
                    "typed":r.json_type.is_some()})).collect::<Vec<_>>() }),
            Err(error) => {
                json!({"parsed":false,"stage":"attribute","error":error.downcast_ref::<String>()
                .map(|s|s.as_str()).or_else(||error.downcast_ref::<&str>().copied()).unwrap_or("attribute panic")})
            }
        },
        Err(error) => json!({"parsed":false,"stage":"lexical","error":error.to_string()}),
    }
}
fn main() {
    let default_hook = std::panic::take_hook();
    std::panic::set_hook(Box::new(move |info| {
        let value = info
            .payload()
            .downcast_ref::<String>()
            .map(String::as_str)
            .or_else(|| info.payload().downcast_ref::<&str>().copied());
        if !value.is_some_and(|v| v == "expected string literal" || v == "expected bool literal") {
            default_hook(info);
        }
    }));
    for line in io::stdin().lock().lines() {
        let input: Value = serde_json::from_str(&line.unwrap()).unwrap();
        println!(
            "{}",
            json!({"metadata":metadata(&input["input"]),"witness":witness(&input["witness"])})
        );
    }
}
