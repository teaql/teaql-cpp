use cpp_business_ids_core::{E, Receipt};
use teaql_core::{CompactRow, Entity};
fn main() {
    let receipt=Receipt::from_compact_row(CompactRow::from_map(Default::default())).unwrap();
    E::receipt(&receipt).get_receipt_number().or_default_if_null();
}
