//! Compile unchanged original entities; expose only their private runtime constructors.
#[path="../../generator/parity/inheritance_variants_rust/lib/src/tenant/entity.rs"]
mod tenant;
pub use tenant::Tenant;
pub fn new_tenant(root: teaql_runtime::EntityRuntimeState) -> Tenant { Tenant::runtime_new(root) }
#[path="../../generator/parity/inheritance_variants_rust/lib/src/document/entity.rs"]
mod document;
pub use document::Document;
pub fn new_document(root: teaql_runtime::EntityRuntimeState) -> Document { Document::runtime_new(root) }
#[path="../../generator/parity/inheritance_variants_rust/lib/src/intermediate_record/entity.rs"]
mod intermediate_record;
pub use intermediate_record::IntermediateRecord;
pub fn new_intermediate_record(root: teaql_runtime::EntityRuntimeState) -> IntermediateRecord { IntermediateRecord::runtime_new(root) }
#[path="../../generator/parity/inheritance_variants_rust/lib/src/invoice/entity.rs"]
mod invoice;
pub use invoice::Invoice;
pub fn new_invoice(root: teaql_runtime::EntityRuntimeState) -> Invoice { Invoice::runtime_new(root) }
#[path="../../generator/parity/inheritance_variants_rust/lib/src/receipt/entity.rs"]
mod receipt;
pub use receipt::Receipt;
pub fn new_receipt(root: teaql_runtime::EntityRuntimeState) -> Receipt { Receipt::runtime_new(root) }
#[path="../../generator/parity/inheritance_variants_rust/lib/src/special_invoice/entity.rs"]
mod special_invoice;
pub use special_invoice::SpecialInvoice;
pub fn new_special_invoice(root: teaql_runtime::EntityRuntimeState) -> SpecialInvoice { SpecialInvoice::runtime_new(root) }
