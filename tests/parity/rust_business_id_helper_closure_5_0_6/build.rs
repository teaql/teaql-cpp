use std::{env, fs, path::PathBuf};

fn main() {
    let source = PathBuf::from(env::var("CARGO_MANIFEST_DIR").unwrap())
        .join("../rust_field_layout/src/main.rs");
    println!("cargo:rerun-if-changed={}", source.display());
    let text = fs::read_to_string(source).unwrap();
    let begin = text.find("fn business_id_probe(").unwrap();
    let end = text[begin..].find("fn generated_relations_probe(").unwrap() + begin;
    let methods = &text[begin..end];
    fs::write(PathBuf::from(env::var("OUT_DIR").unwrap()).join("original_observers.rs"), methods).unwrap();
}
