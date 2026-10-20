
//! Generated TeaQL domain crate for `cpp-expression-roots-core`.
//!
//! **Before writing queries**, read the `AGENTS.md` at the workspace root.
//! It contains the entity list and the exact `cargo teaql` commands to fetch API prompts.
//!
//! The generated library is not the API-discovery surface. Read the generated
//! application's `AGENTS.md`, then request model-aware object/field Assist.
//! A registry dependency does not require vendoring or browsing generated
//! domain-library source to learn method names. If Assist lacks an operation,
//! report `MISSING_ASSIST` for that path.

pub mod e;
pub mod q;
pub mod request_support;
pub mod runtime;
pub mod sample_data;
pub mod platform;
pub mod class;
pub mod operator;
pub mod template;
pub mod namespace;
pub mod plain_record;

pub use teaql_core;
pub use teaql_runtime::LedgerEntity;
pub use e::*;
pub use q::*;
pub use request_support::*;
pub use runtime::*;
pub use sample_data::*;
pub use platform::*;
pub use class::*;
pub use operator::*;
pub use template::*;
pub use namespace::*;
pub use plain_record::*;