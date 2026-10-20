#!/usr/bin/env python3
"""Execute injected reverse-relation JSON through complete original/C++ models."""
from pathlib import Path
import hashlib
from xml.sax.saxutils import escape

import audit_generated_privacy_contracts_5_0_6 as compiled
import audit_container_metadata_contracts_5_0_6 as descriptors


def scenarios():
    base = descriptors.scenarios()[0][1]
    values = [
        ('untyped', 'reverse_relation(name="others", target="Child", many)'),
        ('typed_many', 'reverse_relation(name="others", target="Child", many, json_type="crate::Child")'),
        ('typed_one', 'reverse_relation(name="others", target="Child", json_type="crate::Child")'),
        ('typed_qualified', 'reverse_relation(name="others", target="Child", many, json_type="crate::Child /* type trivia */")'),
        ('typed_distinct_target', 'reverse_relation(name="others", target="Item", many, json_type="crate::Child")'),
        ('typed_distinct_lookup', 'reverse_relation(name="others", target="Child", many, json_type="Item")'),
        ('typed_overridden_identity', 'entity="Other", reverse_relation(name="others", target="Child", many, json_type="crate::Child")'),
        ('typed_duplicate', 'reverse_relation(name="others", target="Child", many, json_type="crate::Child"), '
         'reverse_relation(name="others", target="Child", json_type="crate::Child")'),
        ('typed_self', 'reverse_relation(name="others", target="Child", many, json_type="Self")'),
        ('unqualified_foreign', 'reverse_relation(name="others", target="Child", many, json_type="Child")'),
    ]
    return [(name, base.replace('_audit_value_max_len="17"', '_audit_value_max_len="'
             + escape('17, ' + value + ', audit_value_max_len=21', {'"': '&quot;'}) + '"'))
            for name, value in values]


RUST_VIEWS = '''
fn views() -> Vec<Value> {
    use teaql_core::{Entity,CompactRow,RelationShapeCache,SmartList,Value as Field};
    use teaql_runtime::EntityGraphBuilder;
    use std::collections::BTreeMap;
    let mut output=Vec::new();
    for mode in 0..6 {
        let root=EntityRuntimeState::default();
        let mut row=CompactRow::from_map(BTreeMap::from([
            ("id".into(),Field::U64(1)),("version".into(),Field::I64(2)),
            ("name".into(),Field::Text("parent".into()))]));
        if mode!=1 { row.mark_relation_loaded("others",&mut RelationShapeCache::default()); }
        let entity=Item::from_compact_row_with_context(row,&root).unwrap();
        let child=Child::from_compact_row(CompactRow::from_map(BTreeMap::from([
            ("id".into(),Field::U64(3)),("version".into(),Field::I64(4)),
            ("name".into(),Field::Text("child".into())),
            ("secret".into(),Field::Text("payload".into()))]))).unwrap();
        let mut graph=EntityGraphBuilder::default();
        let owner=<Item as TeaqlEntity>::ENTITY_NAME;
        if mode==1 || mode==2 || mode==3 {
            let mut list=SmartList::new(if mode==2 {vec![]} else {vec![child]});
            list.is_loaded=true;
            graph.install_relation_list(owner,1,"others",list);
        } else if mode==4 {
            graph.install_relation_option::<Child>(owner,1,"others",None);
        } else if mode==5 {
            graph.install_relation_option(owner,1,"others",Some(child));
        }
        root.freeze_graph(graph).unwrap();
        output.push(entity.into_json());
    }
    output
}
'''

CPP_VIEWS = '''
static Json views(UserContext& user_context) {
    using namespace cpp_privacy_contracts;
    auto r=user_context.memory_resource();
    Json::Array output(r);
    for (int mode=0;mode<6;++mode) {
        EntityRuntimeState root(std::allocator_arg,Allocator(r));
        Record fields(r);
        fields.emplace("id",Value(r,std::uint64_t(1)));
        fields.emplace("version",Value(r,std::int64_t(2)));
        fields.emplace("name",Value(r,"parent"));
        CompactRow row(std::allocator_arg,Allocator(r),std::move(fields));
        RelationShapeCache shapes(std::allocator_arg,Allocator(r));
        if (mode!=1) { row.mark_relation_loaded("others",shapes); }
        auto entity=Item::from_compact_row_with_context(r,std::move(row),AnyRef(root));
        if (!entity) { throw std::runtime_error("Parent decoding failed"); }
        Record child_fields(r);
        child_fields.emplace("id",Value(r,std::uint64_t(3)));
        child_fields.emplace("version",Value(r,std::int64_t(4)));
        child_fields.emplace("name",Value(r,"child"));
        child_fields.emplace("secret",Value(r,"payload"));
        auto child=Child::from_compact_row(r,CompactRow(std::allocator_arg,Allocator(r),std::move(child_fields)));
        if (!child) { throw std::runtime_error("Child decoding failed"); }
        EntityGraphBuilder graph(std::allocator_arg,Allocator(r));
        if (mode==1 || mode==2 || mode==3) {
            SmartList<Child> list(std::allocator_arg,Allocator(r));
            if (mode!=2) { list.data.emplace_back(child.value()); }
            list.loaded();
            graph.install_relation_list(Item::ENTITY_NAME,1,"others",std::move(list));
        } else if (mode==4 || mode==5) {
            std::optional<Child> target;
            if (mode==5) { target.emplace(std::allocator_arg,Allocator(r),child.value()); }
            graph.install_relation_option<Child>(Item::ENTITY_NAME,1,"others",std::move(target));
        }
        if (!root.freeze_graph(std::move(graph))) { throw std::runtime_error("Graph freeze failed"); }
        output.emplace_back(std::move(entity).value().into_json(r));
    }
    return Json(r,std::move(output));
}
'''

RUST_MAIN = descriptors.RUST_MAIN.replace('fn main() {', RUST_VIEWS + '\nfn main() {').replace(
    'json!(records)', 'json!({"descriptors":records,"views":views()})')
CPP_MAIN = descriptors.CPP_MAIN.replace('int main() {', CPP_VIEWS + '\nint main() {').replace(
    'std::cout << Json(r,std::move(output)).dump(r)',
    'Json::Object result(r); result.emplace("descriptors",Json(r,std::move(output))); '
    'result.emplace("views",views(*user_context)); std::cout << Json(r,std::move(result)).dump(r)')


if __name__ == '__main__':
    compiled.main(scenarios=scenarios(), rust_main=RUST_MAIN, cpp_main=CPP_MAIN,
                  scope='Complete unchanged original entities and C++ facade; typed reverse JSON over absent, unmarked, empty, nonempty, NULL and present graph views; no implicit SQL.',
                  expected_rejections={'unqualified_foreign': (
                      r'cannot find type `Child` in this scope',
                      r'json_type does not resolve in the original entity module')},
                  extra_provenance={'driver_sha256': hashlib.sha256(Path(__file__).read_bytes()).hexdigest()})
