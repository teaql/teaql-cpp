use cpp_query_branches_core::Q;
use teaql_core::{AggregateFunction,request::FieldOperator};
fn main(){let _=Q::entries().with_active(FieldOperator::Equal, vec![true]);}
