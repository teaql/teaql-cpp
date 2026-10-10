use serde_json::{json, Value as Json};
use std::io::{self, BufRead};
use teaql_core::*;

fn formatted<T: std::fmt::Debug>(v: &T) -> Json {
    json!({"compact":format!("{v:?}"),"pretty":format!("{v:#?}"),
        "lower_hex":format!("{v:x?}"),"upper_hex":format!("{v:X?}"),
        "pretty_lower_hex":format!("{v:#x?}"),"pretty_upper_hex":format!("{v:#X?}"),
        "width_precision":format!("{v:>40.2?}"),"fancy":format!("{v:🧊^+18.3?}"),
        "zero_pad":format!("{v:+012?}")})
}

struct OwnedEntity { values: MutationValues }
impl TeaqlEntity for OwnedEntity {
    const ENTITY_NAME: &'static str = "Projection";
    fn entity_descriptor() -> EntityDescriptor { EntityDescriptor::new(Self::ENTITY_NAME) }
}
impl Entity for OwnedEntity {
    fn into_values(self) -> MutationValues { self.values }
    fn from_compact_row(row: CompactRow) -> Result<Self,EntityError> { Ok(Self {values:row.into_map().into()}) }
}

struct BindingEntity { data: BaseEntityData, calls: Vec<String>, mode: u64 }
impl TeaqlEntity for BindingEntity {
    const ENTITY_NAME: &'static str = "Binding";
    fn entity_descriptor() -> EntityDescriptor { EntityDescriptor::new(Self::ENTITY_NAME) }
}
impl Entity for BindingEntity {
    fn into_values(self) -> MutationValues { self.data.to_values_map().into() }
    fn from_compact_row(_: CompactRow) -> Result<Self,EntityError> { unimplemented!() }
}
impl BaseEntity for BindingEntity {
    fn base(&self) -> &BaseEntityData { &self.data }
    fn base_mut(&mut self) -> &mut BaseEntityData { &mut self.data }
    fn put_dynamic(&mut self, key: impl Into<String>, value: impl Into<Value>) -> Option<Value> {
        let key=key.into();
        self.calls.push(key.clone());
        match self.mode {
            2=>None,
            1=>self.data.put_dynamic("redirectedStyle",value),
            _=>self.data.put_dynamic(key,value),
        }
    }
}

macro_rules! decoded {
    ($t:ty, $input:expr) => {{
    match serde_json::from_value::<$t>($input) {
        Ok(value) => {
            let encoded=serde_json::to_value(&value).unwrap();
            let roundtrip=serde_json::from_value::<$t>(encoded.clone());
            json!({"accepted":true,"debug":formatted(&value),"serialized":encoded,
                "clone_equal":value==value.clone(),"roundtrip_accepted":roundtrip.is_ok(),
                "roundtrip_equal":roundtrip.as_ref().ok().map(|v|v==&value)})
        },
        Err(_) => json!({"accepted":false}),
    }
    }};
}

fn evaluate(input: Json) -> Json {
    if input["kind"]=="owning_ranges" {
        let value=input["value"].clone();
        let make=||OwnedEntity {values:MutationValues::from(std::collections::BTreeMap::from([("payload".into(),Value::Json(value.clone()))]))};
        let result=if input["mode"]=="move_iterator" {
            let named=[make(),make()].into_iter();
            WebResponse::from_entities(named)
        } else {
            let named=(0..2).map(|_|make());
            WebResponse::from_entities(named)
        };
        return json!({"json":result.to_json_value(),"debug":formatted(&result)});
    }

    if input["kind"]=="trim" {
        let value=input["value"].clone();
        let required=teaql_core::serde_utils::trimmed_string::deserialize(value.clone());
        let optional=teaql_core::serde_utils::trimmed_opt_string::deserialize(value.clone());
        let mut output=json!({"required_accepted":required.is_ok(),"optional_accepted":optional.is_ok(),
                              "required":required.ok(),"optional":optional.ok().flatten(),"original":value});
        if let Some(text)=value.as_str() {
            output["serialized_required"]=teaql_core::serde_utils::trimmed_string::serialize(text,serde_json::value::Serializer).unwrap();
            output["serialized_optional"]=teaql_core::serde_utils::trimmed_opt_string::serialize(&Some(text.to_owned()),serde_json::value::Serializer).unwrap();
        } else if value.is_null() {
            output["serialized_optional"]=teaql_core::serde_utils::trimmed_opt_string::serialize(&None,serde_json::value::Serializer).unwrap();
        }
        return output;
    }

    match input["kind"].as_str().unwrap() {
        "bindings"=>{
            let mut entity=BindingEntity {data:BaseEntityData::default(),calls:vec![],mode:input["mode"].as_u64().unwrap()};
            let mut base=BaseEntityData::default();
            let mut record=Record::new();
            if input["present"].as_bool().unwrap() {
                for key in [STYLE_KEY,ACTION_LIST_KEY] {
                    entity.data.put_dynamic(key,Value::Json(input["value"].clone()));
                    base.put_dynamic(key,Value::Json(input["value"].clone()));
                    record.insert(key.into(),Value::Json(input["value"].clone()));
                }
            }
            let style=WebStyle::with_font_color("private\n雪").class_names("class");
            let action=WebAction::view_web_action();
            let mut states=vec![];
            for _ in 0..2 {
                style.bind_entity(&mut entity); style.bind_base(&mut base); style.bind_record(&mut record);
                action.bind_entity(&mut entity); action.bind_base(&mut base); action.bind_record(&mut record);
                states.push(json!({"entity":record_to_json_value(&entity.data.to_values_map()),"calls":entity.calls,
                    "base":record_to_json_value(&base.to_values_map()),"record":record_to_json_value(&record)}));
            }
            json!({"states":states})
        },
        "ranges"=>{
            let entity=||OwnedEntity {values:std::collections::BTreeMap::from([
                ("payload".into(),Value::Json(input["value"].clone()))]).into()};
            let response=WebResponse::from_entities([entity(),entity()]);
            let records=std::collections::LinkedList::from([
                std::collections::BTreeMap::from([("value".into(),Value::I64(1))]),
                std::collections::BTreeMap::from([("value".into(),Value::Json(input["value"].clone()))])]);
            let records=WebResponse::from_records(records);
            let list=SmartList::new(vec![entity(),entity()]).with_total_count(99);
            let list=WebResponse::from_smart_list(list);
            json!({"entities":formatted(&response),"entities_json":response.to_json_value(),
                "records":formatted(&records),"records_json":records.to_json_value(),
                "list":formatted(&list),"list_json":list.to_json_value()})
        },
        "style"=>decoded!(WebStyle,input["value"].clone()),
        "action"=>decoded!(WebAction,input["value"].clone()),
        "response"=>decoded!(WebResponse,input["value"].clone()),
        "block"=>decoded!(XlsBlock,input["value"].clone()),
        "context"=>decoded!(XlsBlockBuildContext,input["value"].clone()),
        "page"=>decoded!(XlsPage,input["value"].clone()),
        "workbook"=>decoded!(XlsWorkbook,input["value"].clone()),
        "construct"=>{
            let n=input["flags"].as_u64().unwrap();
            let value=input["value"].clone();
            let style=WebStyle::new().background_color("").font_color("private\n雪").class_names("class");
            let action=WebAction::new().key("").name("name").level("level").execute("execute")
                .target("target").component("component").warning_message("warning")
                .role_for_list("role").request_url("/URL").optional_warning_message(if n&1!=0{None}else{Some("")});
            let response=WebResponse::success().with_data(vec![value.clone()]).with_record_count(u64::MAX)
                .with_facets(std::collections::BTreeMap::from([("facet".into(),value.clone())]));
            let context=XlsBlockBuildContext::new("page\n雪",if n&1!=0{-2}else{2},if n&2!=0{-3}else{3});
            let block=context.to_block(value.clone()).region(-3,-2,4,5).span(2,3)
                .add_property("private",value.clone()).style(XlsBlock::new("style",0,0,Json::Null));
            let page=XlsPage::new("page\n雪").add_block(block.clone()).add_block(block.clone());
            let workbook=XlsWorkbook::new().add_page(page.clone()).add_page(page.clone());
            json!({"style":formatted(&style),"action":formatted(&action),"response":formatted(&response),
                "context":formatted(&context),"next":formatted(&context.next()),
                "next_line":formatted(&context.next_line()),"new_line":formatted(&context.new_line()),
                "block":formatted(&block),"page":formatted(&page),"workbook":formatted(&workbook),
                "style_json":style.to_json_value(),"action_json":action.to_json_value(),
                "response_json":response.to_json_value(),"block_json":block.to_json_value(),
                "workbook_json":workbook.to_json_value(),
                "block_found":page.block_at(-3,-2).map(formatted),"page_found":workbook.page("page\n雪").map(formatted),
                "missing_block":page.block_at(99,99).is_none(),"missing_page":workbook.page("missing").is_none(),
                "default_style":formatted(&WebStyle::default()),"default_action":formatted(&WebAction::default()),
                "default_page":formatted(&XlsPage::default()),"default_workbook":formatted(&XlsWorkbook::default())})
        },
        _=>unreachable!(),
    }
}

fn main() {
    for line in io::stdin().lock().lines() {
        println!("{}",evaluate(serde_json::from_str(&line.unwrap()).unwrap()));
    }
}
