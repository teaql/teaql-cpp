#!/usr/bin/env python3
"""Compile original/generated C++ property descriptors for complete model callers."""
import gzip
import hashlib
import json
from pathlib import Path
import sys

import verify_audit_metadata_compiler_5_0_6 as compiler

ROOT = Path(__file__).resolve().parents[1]
RUST_MAIN = r'''
use serde_json::json;
use teaql_core::TeaqlEntity;
fn main() {
    let rows = [Platform::entity_descriptor(), Item::entity_descriptor(), Child::entity_descriptor()]
        .into_iter().map(|d| json!({"entity":d.name, "properties":d.properties.into_iter().map(|p|
            json!({"name":p.name,"column":p.column_name,"type":p.data_type as u8,
                "nullable":p.nullable,"id":p.is_id,"version":p.is_version,
                "max_length":p.max_length,"precision":p.numeric_precision,"scale":p.numeric_scale}))
            .collect::<Vec<_>>()})).collect::<Vec<_>>();
    println!("{}", json!(rows));
}
'''
CPP_MAIN = r'''
static Json observe(Memory r, EntityDescriptor descriptor) {
    Json::Object output(r);
    output.emplace("entity", Json(r, descriptor.name));
    Json::Array properties(r);
    auto number = [&](const std::optional<std::uint32_t>& value) {
        return value ? Json(r, static_cast<std::uint64_t>(*value)) : Json(r);
    };
    for (const auto& property : descriptor.properties) {
        Json::Object value(r);
        value.emplace("name", Json(r, property.name));
        value.emplace("column", Json(r, property.column_name));
        value.emplace("type", Json(r, static_cast<std::uint64_t>(property.data_type)));
        value.emplace("nullable", Json(r, property.nullable));
        value.emplace("id", Json(r, property.is_id));
        value.emplace("version", Json(r, property.is_version));
        value.emplace("max_length", number(property.max_length));
        value.emplace("precision", number(property.numeric_precision));
        value.emplace("scale", number(property.numeric_scale));
        properties.emplace_back(std::move(value));
    }
    output.emplace("properties", Json(r, std::move(properties)));
    return Json(r, std::move(output));
}
int main() {}
'''


def main():
    compiler.RUST_MAIN = RUST_MAIN
    compiler.CPP_MAIN = CPP_MAIN
    output = Path(sys.argv[sys.argv.index('--record') + 1])
    assert not output.exists()
    try:
        return compiler.main()
    finally:
        if output.exists():
            record = json.loads(gzip.decompress(output.read_bytes()))
            if not record.get('diagnostic_cpp_refusal'):
                record['scope'] = ('Complete original/C++ generated property descriptor callers: names, '
                    'columns, types, nullability, ID/version and length/precision/scale. '
                    'No SQL execution or complete AP02/G14 acceptance.')
            record['observer_wrapper'] = dict(source=Path(__file__).read_text(),
                sha256=hashlib.sha256(Path(__file__).read_bytes()).hexdigest(),
                compiler_sha256=hashlib.sha256(Path(compiler.__file__).read_bytes()).hexdigest(),
                rust_main=RUST_MAIN, cpp_main=CPP_MAIN)
            output.write_bytes(gzip.compress(json.dumps(record).encode(), mtime=0))


if __name__ == '__main__':
    raise SystemExit(main())
