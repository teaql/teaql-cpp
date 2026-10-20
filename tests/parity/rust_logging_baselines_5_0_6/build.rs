use std::{env,fs,path::PathBuf};
fn main(){
    let root=PathBuf::from(env::var("CARGO_MANIFEST_DIR").unwrap()).join("../../../.reference/teaql-rs/teaql-runtime/src");
    let out=PathBuf::from(env::var("OUT_DIR").unwrap());
    for (file,start,end,target,prefix,suffix) in [
        ("mutation_policy.rs","tokio::task_local! {","\nimpl UserContext {","governance_scope.rs","",""),
        ("context.rs","    pub fn send_event(","    pub async fn get_in_store(","context_event.rs","impl UserContext {\n","}\n")]{
        let path=root.join(file);println!("cargo:rerun-if-changed={}",path.display());
        let text=fs::read_to_string(path).unwrap();let from=text.find(start).expect("locked source start marker");let to=from+text[from..].find(end).expect("locked source end marker");
        fs::write(out.join(target),format!("{prefix}{}{suffix}",&text[from..to])).unwrap();
    }
}
