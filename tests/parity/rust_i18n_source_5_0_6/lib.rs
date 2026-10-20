//! Original Language/I18n source modules and their unchanged four tests.
//! Check/model/error dependencies are the actual locked runtime types.
pub use teaql_runtime::{CheckResult, CheckRule, ObjectLocation, RuntimeError};
#[path="../../../.reference/teaql-rs/teaql-runtime/src/language.rs"]
pub mod language;
#[path="../../../.reference/teaql-rs/teaql-runtime/src/i18n.rs"]
pub mod i18n;
pub use language::{Language, Locale};
pub use i18n::I18nCatalog;
