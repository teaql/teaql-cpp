#!/usr/bin/env python3
"""Compare actual generated container identities, descriptors and fixed layouts."""
from pathlib import Path
from xml.sax.saxutils import escape
import hashlib

import audit_generated_privacy_contracts_5_0_6 as compiled


def scenarios():
    values = [
        ('control', '17'),
        ('entity', '17, entity="Other", audit_value_max_len=21'),
        ('table', '17, table="other_table", audit_value_max_len=21'),
        ('data_service', '17, data_service="other", audit_value_max_len=21'),
        ('reverse_relation', '17, reverse_relation(name="others", target="Child", many), audit_value_max_len=21'),
        ('combined', '17, entity="Other", table="other_table", data_service="other", '
         'reverse_relation(name="others", target="Child", local_key="id", foreign_key="item", many), '
         'audit_mask_fields="name", audit_value_max_len=21'),
        ('duplicate_relation', '17, reverse_relation(name="others", target="Child", many), '
         'reverse_relation(name="others", target="Item", local_key="version", foreign_key="id"), '
         'audit_value_max_len=21'),
        ('unknown_relation_flag', '17, reverse_relation(name="others", unknown, target="Child", many), '
         'audit_value_max_len=21'),
        ('unknown_relation_assignment', '17, reverse_relation(name="others", unknown=0, target="Child"), '
         'audit_value_max_len=21'),
        ('relation_defaults', '17, reverse_relation(name="others", target="Child"), audit_value_max_len=21'),
        ('embedded_null', r'17, entity="Other\0雪", table="other\0table", data_service="other\0service", '
         r'reverse_relation(name="others\0name", target="Child\0target", local_key="id\0local", '
         r'foreign_key="id\0foreign", many), audit_mask_fields="name\0suffix", audit_value_max_len=21'),
    ]
    result = []
    for name, value in values:
        attribute = escape(value, {'"': '&quot;'})
        model = ('<root name="cpp_privacy_contracts" data_service="sqlite">'
                 '<platform name="string()"/>'
                 '<item platform="platform(contextRoot=true)" name="string(column=display_name)" '
                 'note="string()?" _table="business_items" _audit_mask_fields="name,platform" '
                 f'_audit_value_max_len="{attribute}"/>'
                 '<child _parent="item" secret="string(column=password)" '
                 '_table="business_children" _audit_mask_fields="secret"/></root>')
        result.append((name, model))
    return result


RUST_MAIN = '''
use serde_json::{json, Value};
use teaql_core::TeaqlEntity;
use teaql_data_service::SqlIntentRedactions;
use teaql_runtime::{EntityKey, EntityRuntimeState};
fn observe<T: TeaqlEntity>(key: EntityKey) -> Value {
    let d=T::entity_descriptor();
    let layout=T::field_layout().unwrap().unwrap();
    let relations=d.relations.iter().map(|r|json!([
        r.name,r.target_entity,r.local_key,r.foreign_key,r.many,r.attach,r.delete_missing,
        layout.is_relation(&r.name),layout.index(&r.name)])).collect::<Vec<_>>();
    json!({"entity":d.name,"table":d.table_name,"service":d.data_service,
        "identity":<T as TeaqlEntity>::ENTITY_NAME,"key_entity":key.entity,
        "layout_entity":layout.entity(),"revision":layout.revision(),"count":layout.field_count(),
        "relations":relations,"masks":d.audit_mask_fields,"declared":d.audit_mask_fields_declared,
        "policies":d.properties.iter().map(|p|json!([p.name,p.column_name,
            format!("{:?}",SqlIntentRedactions::field_policy(&d,&p.name))])).collect::<Vec<_>>(),
        "limit":d.audit_value_max_len.map(|n|n.to_string())})
}
fn main() {
    let records=vec![
        observe::<Platform>(Platform::runtime_new(EntityRuntimeState::default()).entity_key()),
        observe::<Item>(Item::runtime_new(EntityRuntimeState::default()).entity_key()),
        observe::<Child>(Child::runtime_new(EntityRuntimeState::default()).entity_key())];
    println!("{}",json!(records));
}
'''

CPP_MAIN = '''
#include "cpp_privacy_contracts.hpp"
#include <teaql/data_service/sql_log.hpp>
#include <iostream>
using namespace teaql;
template <Entity T> Json observe(UserContext& user_context) {
    auto r=user_context.memory_resource();
    auto d=T::entity_descriptor(r);
    const auto& layout=T::field_layout(r);
    auto& entity=user_context.make<T>();
    auto key=entity.entity_key();
    Json::Object item(r);
    item.emplace("entity",Json(r,d.name));
    item.emplace("table",Json(r,d.table_name));
    item.emplace("service",d.data_service ? Json(r,*d.data_service) : Json(r));
    item.emplace("identity",Json(r,T::ENTITY_NAME));
    item.emplace("key_entity",Json(r,key.entity));
    item.emplace("layout_entity",Json(r,layout.entity()));
    item.emplace("revision",Json(r,layout.revision()));
    item.emplace("count",Json(r,static_cast<std::uint64_t>(layout.field_count())));
    item.emplace("declared",Json(r,d.audit_mask_fields_declared));
    item.emplace("limit",d.audit_value_max_len ? Json(r,std::to_string(*d.audit_value_max_len)) : Json(r));
    Json::Array relations(r),masks(r),policies(r);
    for (const auto& rel : d.relations) {
        Json::Array row(r);
        row.emplace_back(rel.name);row.emplace_back(rel.target_entity);
        row.emplace_back(rel.local_key);row.emplace_back(rel.foreign_key);
        row.emplace_back(rel.many);row.emplace_back(rel.attach);row.emplace_back(rel.delete_missing);
        row.emplace_back(layout.is_relation(rel.name));
        auto index=layout.index(rel.name);
        row.emplace_back(index ? Json(r,static_cast<std::uint64_t>(*index)) : Json(r));
        relations.emplace_back(std::move(row));
    }
    for (const auto& mask : d.audit_mask_fields) { masks.emplace_back(mask); }
    for (const auto& p : d.properties) {
        auto policy=SqlIntentRedactions::field_policy(d,p.name);
        const char* name=policy==SqlParameterLogPolicy::Plain ? "Plain" :
            policy==SqlParameterLogPolicy::Masked ? "Masked" :
            policy==SqlParameterLogPolicy::Credential ? "Credential" : "Unknown";
        Json::Array row(r);
        row.emplace_back(p.name);row.emplace_back(p.column_name);row.emplace_back(name);
        policies.emplace_back(std::move(row));
    }
    item.emplace("relations",Json(r,std::move(relations)));
    item.emplace("masks",Json(r,std::move(masks)));
    item.emplace("policies",Json(r,std::move(policies)));
    return Json(r,std::move(item));
}
int main() {
    std::pmr::set_default_resource(std::pmr::null_memory_resource());
    GlobalContext global;
    auto user_context=global.make_user_context();
    auto r=user_context->memory_resource();
    Json::Array output(r);
    output.emplace_back(observe<cpp_privacy_contracts::Platform>(*user_context));
    output.emplace_back(observe<cpp_privacy_contracts::Item>(*user_context));
    output.emplace_back(observe<cpp_privacy_contracts::Child>(*user_context));
    std::cout << Json(r,std::move(output)).dump(r) << '\\n';
}
'''


if __name__ == '__main__':
    compiled.main(scenarios=scenarios(), rust_main=RUST_MAIN, cpp_main=CPP_MAIN,
                  scope='Complete unchanged original entities and generated C++ facade; actual descriptor, runtime key and fixed-layout identity/relation observations.',
                  extra_provenance={'driver_sha256': hashlib.sha256(Path(__file__).read_bytes()).hexdigest()})
