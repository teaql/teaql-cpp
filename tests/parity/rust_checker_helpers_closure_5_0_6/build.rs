use std::{env, fs, path::PathBuf};

fn main() {
    let source = PathBuf::from(env::var("CARGO_MANIFEST_DIR").unwrap())
        .join("../rust_field_layout/src/main.rs");
    println!("cargo:rerun-if-changed={}", source.display());
    let text = fs::read_to_string(source).unwrap();
    let begin = text.find("fn checker_helper_probe(").unwrap();
    let end = text[begin..].find("fn mutation_envelope_probe(").unwrap() + begin;
    let comment = text.find("fn comment_probe(").unwrap();
    let methods = format!("{}\n{}", &text[begin..end], &text[comment..]);
    fs::write(PathBuf::from(env::var("OUT_DIR").unwrap()).join("original_observers.rs"), methods).unwrap();
}
