"""Prepare an explicit derived-fixture name/slot bridge without editing originals.

This is an adapted compilation experiment, not untouched producer acceptance.
The original twenty-error record remains authoritative for original artifacts.
"""
import hashlib
import json
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
ORIGINAL = ROOT / 'tests/generator/parity/inheritance_business_ids_rust/lib'
DESTINATION = ROOT / 'build_inherited_business_binding_bridge_5_0_6/lib'
CHILDREN = {'intermediate_record': 'IntermediateRecord', 'invoice': 'Invoice',
            'receipt': 'Receipt', 'special_invoice': 'SpecialInvoice'}


def digest(value):
    return hashlib.sha256(value).hexdigest()


def main():
    producer = json.loads((ORIGINAL.parent / 'PROVENANCE.json').read_text())
    runtime = json.loads((ROOT / 'upstream.lock.json').read_text())
    generator = json.loads((ROOT.parent / 'teaql_code_gen_cpp/cpp_generator.lock.json').read_text())
    if producer['runtime_commit'] != runtime['commit']:
        raise RuntimeError('Original runtime provenance differs from active source')
    if producer['generator_commit'] != generator['generator_baseline_commit']:
        raise RuntimeError('Original producer provenance differs from active source')
    parent = (ORIGINAL / 'src/document/entity.rs').read_text()
    start = parent.index('pub(crate) struct DocumentDocumentNumberBusinessIdSlot')
    slot = parent[start:]
    if slot.count('impl teaql_core::business_id::BusinessIdSlot') != 1:
        raise RuntimeError('Original parent slot shape changed')
    records = {}
    for path in ORIGINAL.rglob('*'):
        if not path.is_file():
            continue
        relative = path.relative_to(ORIGINAL)
        original = path.read_bytes()
        adapted = original
        prefix, suffix = b'', b''
        visibility_bridge = False
        if relative == Path('src/document/entity.rs'):
            declaration = b'pub struct DocumentNumber(String);'
            if original.count(declaration) != 1:
                raise RuntimeError('Original shared identifier declaration changed')
            adapted = original.replace(declaration,
                                       b'pub struct DocumentNumber(pub(crate) String);', 1)
            visibility_bridge = True
        for folder, entity in CHILDREN.items():
            if relative == Path(f'src/{folder}/entity.rs'):
                prefix = b'use crate::document::DocumentNumber;\n'
                # Bind the exact parent slot body to the flattened child carrier.
                # Method bodies/field names remain byte-identical; only the tuple
                # carrier type in its declaration changes, as the model requires.
                declaration = "pub(crate) &'a mut Document,"
                if slot.count(declaration) != 1:
                    raise RuntimeError('Original tuple carrier declaration changed')
                suffix = ('\n' + slot.replace(declaration,
                          f"pub(crate) &'a mut {entity},", 1)).encode()
        destination = DESTINATION / relative
        destination.parent.mkdir(parents=True, exist_ok=True)
        destination.write_bytes(prefix + adapted + suffix)
        if destination.read_bytes() != prefix + adapted + suffix:
            raise RuntimeError('Derived fixture differs from its explicit bridge specification')
        records[str(relative)] = dict(original_sha256=digest(original),
                                      adapted_sha256=digest(destination.read_bytes()),
                                      prefixed_bytes=len(prefix), appended_bytes=len(suffix),
                                      crate_private_tuple_visibility_bridge=visibility_bridge)
    report = dict(schema=1, runtime_commit=runtime['commit'],
                  generator_commit=producer['generator_commit'],
                  model=producer['model'], model_sha256=producer['model_sha256'],
                  original_files_unchanged=True, untouched_producer_acceptance=False,
                  qualification_status='prepared; compilation and behavior not yet qualified',
                  adaptation='Four missing shared-type imports; original parent BusinessIdSlot '
                             'body rebound to each flattened child carrier; the shared tuple '
                             'field is crate-private in the derived fixture so existing descendant '
                             'setters can access it. No method/checker/runtime algorithm replacement. '
                             'This is explicitly adapted compilation, not original artifact acceptance.',
                  slot_body_sha256=digest(slot.encode()), files=records)
    (DESTINATION.parent / 'provenance.json').write_text(json.dumps(report, indent=2) + '\n')
    print(f'Prepared {len(records)} original files with four explicit child binding bridges')


if __name__ == '__main__':
    main()
