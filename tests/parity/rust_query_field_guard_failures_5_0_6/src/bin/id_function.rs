use cpp_query_branches_core::Q;
use teaql_core::{AggregateFunction,request::FieldOperator};
fn main(){let _=Q::entries().select_id_with_function(AggregateFunction::Count);}
