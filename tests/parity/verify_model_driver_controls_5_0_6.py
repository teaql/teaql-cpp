#!/usr/bin/env python3
"""Exercise compiled-model, source-artifact and bounded entity-library obligations."""
import json
from pathlib import Path
import sys

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / 'scripts'))
from model_driver_provenance_5_0_6 import validate
from verify_generated_driver_controls_5_0_6 import main


def extra_controls(reject, original, registry):
    def artifact_source(record, contract):
        prefix = contract['artifact_fixtures'][0] + '/lib/src/'
        path = next(name for name in record['source_files'] if name.startswith(prefix))
        record['source_files'][path] = '0' * 64

    reject('changed_artifact_only_source', artifact_source)

    def compiled_artifact(record, contract):
        contract['generated_fixtures'].append(contract['artifact_fixtures'].pop())

    reject('artifact_falsely_claimed_as_compiled_package', compiled_artifact)
    reject('missing_required_artifact_producer',
           lambda record, contract: contract['artifact_fixtures'].pop(0))
    reject('missing_required_plural_producer',
           lambda record, contract: contract['artifact_fixtures'].pop(1))

    def library_metadata(record, contract, remove):
        metadata = json.loads(record['project_metadata'])
        project = next(p for p in metadata['packages'] if p['name'] == contract['package'])
        target = next(t for t in project['targets'] if t['name'] == contract['library_target']['name'])
        if remove:
            project['targets'].remove(target)
        else:
            target['src_path'] = '/tmp/substituted_entity_library.rs'
        record['project_metadata'] = json.dumps(metadata)
        record['steps'][record['metadata_step']]['stdout'] = record['project_metadata']

    reject('missing_entity_library_target',
           lambda record, contract: library_metadata(record, contract, True))
    reject('substituted_entity_library_source',
           lambda record, contract: library_metadata(record, contract, False))


if __name__ == '__main__':
    main(validator=validate, extra_controls=extra_controls)
