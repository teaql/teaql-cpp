use std::io::{self, BufRead};
use serde_json::{json, Value as Json};
use teaql_core::*;
fn evaluate(input: Json) -> Json {
    let kind=input["kind"].as_str().unwrap();
    let value=input["value"].clone();
    macro_rules! decode { ($t:ty) => { match serde_json::from_value::<$t>(value) {
        Ok(v)=>json!({"ok":serde_json::to_value(v).unwrap()}), Err(_)=>json!({"error":true})
    } }; }
    match kind {
        "action_factory" => {
            let action=match value.as_str().unwrap() {
                "view"=>WebAction::view_web_action(),
                "sub_list"=>WebAction::view_sub_list_action("名字","list","children"),
                "component"=>WebAction::simple_component_action("名字","table"),
                "modify"=>WebAction::modify_web_action("名字","/url"),
                "modify_warning"=>WebAction::modify_web_action_with_warning("名字","/url",Some(" warning ")),
                "modify_empty_warning"=>WebAction::modify_web_action_with_warning("名字","/url",Some("")),
                "default_modify"=>WebAction::default_modify_web_action(),
                "delete"=>WebAction::delete_web_action(),
                "delete_warning"=>WebAction::delete_web_action_with_warning("/url"," warning "),
                "audit"=>WebAction::audit_web_action("/url"," warning "),
                "discard"=>WebAction::discard_web_action("/url"," warning "),
                "goto"=>WebAction::goto_action("名字","target","/url"),
                "switch"=>WebAction::switch_view_action("名字","target"),
                "add_new"=>WebAction::add_new_web_action("名字"),
                "upload"=>WebAction::batch_upload_web_action(),
                "common"=>return json!({"ok":WebAction::common_web_actions()}),
                _=>panic!("unknown action factory"),
            };json!({"ok":action})
        },
        "response_factory" => {
            let result=match value.as_str().unwrap() {
                "success"=>WebResponse::success(),
                "fail"=>WebResponse::fail(" failed "),
                "empty"=>WebResponse::empty_list(" none "),
                "push"=>WebResponse::success().with_record_count(99).push_json(json!({"data":[1,2]})),
                _=>panic!("unknown response factory"),
            };json!({"ok":result})
        },
        "navigation" => {
            let x=value["x"].as_i64().unwrap() as i32;let y=value["y"].as_i64().unwrap() as i32;
            let context=XlsBlockBuildContext::new("page",x,y);
            let block=context.to_block(json!({"value":"data"})).span(value["width"].as_i64().unwrap() as i32,value["height"].as_i64().unwrap() as i32);
            json!({"ok":{"context":context,"next":context.next(),"newLine":context.new_line(),"nextLine":context.next_line(),"block":block,"width":block.width(),"height":block.height(),"contains":block.contains(x,y)}})
        },
        "style"=>decode!(WebStyle), "action"=>decode!(WebAction), "response"=>decode!(WebResponse),
        "block"=>decode!(XlsBlock), "context"=>decode!(XlsBlockBuildContext),
        "page"=>decode!(XlsPage), "workbook"=>decode!(XlsWorkbook),
        _=>panic!("unknown fixture kind"),
    }
}
fn main() { for line in io::stdin().lock().lines() {
    println!("{}",evaluate(serde_json::from_str(&line.unwrap()).unwrap()));
} }
