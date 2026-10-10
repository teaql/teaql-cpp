//! Real SQLite and unchanged locked example models; only business observations are normalized.
#[path = "../../../../../.reference/teaql-rs/examples/src/lib.rs"]
mod original;
use original::{Order,OrderLine,Product,sqlite_context};
use teaql_core::{Entity,Expr,SelectQuery,SmartList};
use teaql_runtime::{AuditedSaveExt,EntityKey,PurposedSelectQuery};
use teaql_provider_sqlite::SqliteMutationExecutor;
use serde_json::{Value as J,json};
use std::io::{self,BufRead};

fn observe(orders:&SmartList<Order>)->J {
    json!(orders.as_slice().iter().map(|order|json!({
        "id":order.id,"version":order.version,"name":order.name,"lines_loaded":order.lines.is_loaded,
        "lines":order.lines.as_slice().iter().map(|line|json!({"id":line.id,"order_id":line.order_id,
            "name":line.name,"product_id":line.product_id,"product":line.product.as_ref().map(|p|json!({"id":p.id,"name":p.name}))})).collect::<Vec<_>>()
    })).collect::<Vec<_>>())
}
async fn run(input:J)->J {
    let connection=rusqlite::Connection::open_in_memory().unwrap();
    let context=sqlite_context(SqliteMutationExecutor::from_connection(connection));
    context.ensure_schema().await.unwrap();
    let repository=context.entity_data_service::<teaql_sql::SqlDataServiceExecutor<
        teaql_provider_sqlite::SqliteDialect,SqliteMutationExecutor,teaql_runtime::InMemoryMetadataStore>>("Order").unwrap();
    let rows=if input["mode"]=="schema" {
        for (version,name,reason,deleted) in [
            (1,"draft","Create the example order",false),
            (1,"submitted","Submit the example order",false),
            (2,"submitted","Delete the example order",true)] {
            let order=Order{root:Default::default(),id:1,version,name:name.into(),lines:SmartList::default()};
            if deleted {order.root.mark_as_delete(EntityKey::new("Order",1_u64));}
            order.audit_as(reason).save(&context).await.unwrap();
        }
        let query=PurposedSelectQuery::new(repository.select().project("id").project("version").project("name").filter(Expr::eq("id",1_u64)),"Inspect the soft-deleted example order");
        observe(&repository.fetch_entities::<Order>(&query).await.unwrap())
    } else {
        for (id,name) in [(100,"keyboard"),(101,"mouse")] {
            Product{root:Default::default(),id,name:name.into()}.audit_as("Seed the product before creating an order line that references it").save(&context).await.unwrap();
        }
        Order{root:Default::default(),id:1,version:1,name:"graph-order".into(),lines:SmartList::from(vec![
            OrderLine{root:Default::default(),id:10,order_id:1,name:"first-line".into(),product_id:100,product:None},
            OrderLine{root:Default::default(),id:11,order_id:1,name:"second-line".into(),product_id:101,product:None}
        ])}.audit_as("Create the example order graph").save(&context).await.unwrap();
        let query=PurposedSelectQuery::new(SelectQuery::new("Order").project("name").relation_query("lines",SelectQuery::new("OrderLine").relation("product")).order_asc("id"),"Display the saved example order");
        observe(&repository.fetch_enhanced_entities::<Order>(&query).await.unwrap())
    };
    if input["logs"]==true {
        json!(context.sql_logs().iter().map(|e|json!({"operation":format!("{:?}",e.operation),"purpose":e.purpose,"audit":e.audit_reason})).collect::<Vec<_>>())
    } else { rows }
}
fn main(){
    unsafe { std::env::set_var("TEAQL_LOG_ENDPOINT","off"); }
    let runtime=tokio::runtime::Builder::new_current_thread().enable_all().build().unwrap();
    for line in io::stdin().lock().lines(){
        println!("{}",runtime.block_on(run(serde_json::from_str(&line.unwrap()).unwrap())));
    }
}
