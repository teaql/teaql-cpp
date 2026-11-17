#!/usr/bin/env python3
"""Compile the remaining scalar families and execute their generated consumers."""
import argparse
import base64
import json
import os
from pathlib import Path
import shutil
import sys
import tempfile
from verify_audit_metadata_source_5_0_6 import identities
from verify_compiler_generated_business_5_0_6 import execution_inputs, manifest, native_inputs, read, references, require, run_owned, sha, write
from verify_compiler_governed_execution_5_0_6 import prepare
from verify_compiler_host_assembly_5_0_6 import HOST, WORKER, package
from verify_compiler_projection_generation_5_0_6 import hashes, native_command
from verify_compiler_scalar_consumers_5_0_6 import CLOCK, WALL, inputs as checker_inputs
from verify_compiler_scalar_governed_5_0_6 import inputs as graph_inputs, typed as existing_typed
from verify_compiler_source_service_5_0_6 import artifact_sha
ROOT = Path(__file__).resolve().parents[1]
SOURCES = {name: (ROOT / ('tests/parity/compiler_scalar_' + stem + '_probe.cpp'), ROOT / ('tests/parity/compiler_scalar_' + stem + '_probe.rs')) for (name, stem) in (('checker', 'consumers'), ('reads', 'storage'), ('graph', 'governed'))}
sys.path.insert(0, str(ROOT / 'tests/parity'))
from compare_values import json_lines, same

def cases():
    result = []
    for (kind, expression, carrier, rust, values, datatype) in (('text', 'string', 'String', 'alloc::string::String', ['edge', 'changed'], 'Text'), ('bool', 'bool', 'bool', 'bool', [True, False], 'Bool'), ('i64', 'integer', 'std::int64_t', 'i64', [4294967297, -4294967297], 'I64'), ('f32', 'float', 'float', 'f32', [1.5, -3.25], 'F64'), ('f64', 'double', 'double', 'f64', [2.5, -3.75], 'F64'), ('decimal', 'number', 'Decimal', 'rust_decimal::decimal::Decimal', ['123.45', '-9.125'], 'Decimal')):
        for optional in (False,) if kind == 'decimal' else (False, True):
            name = kind + ('_optional' if optional else '_required')
            model = '<root name="compiled_scalar_storage" org="teaql" data_service="sqlite"><gateway name="string()"/><reading gateway="gateway(contextRoot=true)" value="' + expression + '()' + ('?' if optional else '') + '"/></root>'
            result.append(dict(name=name, kind=kind, optional=optional, values=values, model=model, carrier='std::optional<' + carrier + '>' if optional else carrier, rust_type='core::option::Option<' + rust + '>' if optional else rust, data_type=datatype))
    return result

def typed(case, value):
    if value is None:
        return dict(kind='null')
    if case['kind'] in ('i64', 'f32', 'f64'):
        return dict(kind='i64' if case['kind'] == 'i64' else 'f64', value=str(value))
    return existing_typed(case, value)

def inputs(name, case):
    if name in ('checker', 'graph'):
        result = (checker_inputs if name == 'checker' else graph_inputs)(case)
        for value in result:
            if name == 'graph' or value['kind'] == 'checker':
                value['typed'] = typed(case, value['value'])
                value['next_typed'] = typed(case, value['next_value'])
        return result
    declaration = ' DECIMAL' if case['kind'] == 'decimal' else ''
    schema = 'CREATE TABLE gateway_data(id INTEGER PRIMARY KEY,name TEXT,version INTEGER);CREATE TABLE reading_data(id INTEGER PRIMARY KEY,value' + declaration + ",gateway INTEGER,version INTEGER);INSERT INTO gateway_data VALUES(1,'edge',1);"
    values = [*case['values'], None] if case['optional'] else case['values']

    def literal(value):
        if value is None:
            return 'NULL'
        if type(value) is bool:
            return str(int(value))
        if case['kind'] in ('text', 'decimal'):
            return "'" + value.replace("'", "''") + "'"
        return str(value)
    data = schema + 'INSERT INTO reading_data VALUES' + ','.join(('(' + str(index + 7) + ',' + literal(value) + ',1,1)' for (index, value) in enumerate(values))) + ';'
    return [dict(sql=data), dict(sql=schema)]

def rust_observer(name, case):
    source = SOURCES[name][1].read_text()
    if not case['optional'] or name == 'reads':
        return (source, None)
    before = 'entity.update_value(desired);'
    after = 'entity.update_value(desired.map(teaql_core::Value::from).unwrap_or(teaql_core::Value::Null));'
    count = 2 if name == 'checker' else 3
    require(source.count(before) == count, 'exact optional observer setter arguments')
    return (source.replace(before, after), dict(before=before, after=after, replacements=count))

def execute(path, selection):
    require(not path.exists() and shutil.disk_usage(ROOT).free > 6 * 1024 ** 3 and CLOCK.is_file(), 'fresh family record, owned space and existing clock')
    selected = cases() if selection is None else [case for name in selection for case in cases() if case['name'] == name]
    require(selected and (selection is None or len(selected) == len(set(selection)) == len(selection)), 'explicit supported unique model selection')
    record = dict(complete=False, whole_goal_complete=False, driver_source=Path(__file__).read_text(), observer_sources={name: dict(cpp=cpp.read_text(), rust=rust.read_text()) for (name, (cpp, rust)) in SOURCES.items()}, host_source=HOST.read_text(), worker_source=WORKER.read_text(), preparation_steps=[], steps=[], cases=[], selected_cases=[case['name'] for case in selected], inputs_before=execution_inputs(), references_before=references(), native_before=native_inputs(), clock_before=sha(CLOCK.read_bytes()))
    owned = None

    def run(label, command, *, env=None, payload=None):
        print(label, 'running', flush=True)
        result = run_owned(command, cwd=ROOT, env=env, input=payload, text=True, capture_output=True, timeout=1200)
        record['steps'].append(dict(label=label, command=list(map(str, command)), input=payload, exit_code=result.returncode, stdout=result.stdout, stderr=result.stderr))
        write(path, record)
        print(label, 'exit', result.returncode, flush=True)
        require(type(result.returncode) is int and result.returncode == 0, 'actual scalar family command failed: ' + label)
        return result.stdout
    try:
        with tempfile.TemporaryDirectory(prefix='teaql_compiler_scalar_family_') as directory:
            owned = Path(directory)
            record['owned_directory'] = directory
            try:
                (capability, classpath) = prepare(owned, record)
                config = record['capabilities']
                config['schema'] = 'teaql.rust_configuration.job.v4'
                run('host_javac', ['javac', '--release', '17', '-cp', classpath, '-d', owned / 'cpp_classes', HOST])
                config['artifacts'] = {name: artifact_sha(Path(name)) for name in config['artifacts']}
                capability.write_text(json.dumps(config, sort_keys=True))
                env = dict(os.environ, CARGO_INCREMENTAL='0', CARGO_PROFILE_DEV_DEBUG='0', CARGO_TARGET_DIR=str(owned / 'consumer_target'), CARGO_TARGET_X86_64_UNKNOWN_LINUX_GNU_LINKER=config['linker']['path'], TEAQL_LOG_ENDPOINT='off', PYTHONDONTWRITEBYTECODE='1')
                record['environment_overrides'] = {name: env[name] for name in ('CARGO_INCREMENTAL', 'CARGO_PROFILE_DEV_DEBUG', 'CARGO_TARGET_DIR', 'CARGO_TARGET_X86_64_UNKNOWN_LINUX_GNU_LINKER', 'TEAQL_LOG_ENDPOINT')}
                fixed = dict(env, LD_PRELOAD=str(CLOCK), TEAQL_TEST_SAMPLE_WALL_MILLIS=str(WALL))
                record['process_clock'] = dict(path=str(CLOCK), sha256=record['clock_before'], environment={key: fixed[key] for key in ('LD_PRELOAD', 'TEAQL_TEST_SAMPLE_WALL_MILLIS')})
                for case in selected:
                    name = case['name']
                    current = owned / name
                    current.mkdir()
                    model = current / 'model.json'
                    model.write_text(json.dumps(package(case['model']), sort_keys=True))
                    row = dict(case, source_package=package(case['model']), consumers={})
                    record['cases'].append(row)
                    run(name + '_host', [config['cpp_generator']['command'][0], '-cp', classpath, 'CppCompilerHostProbe', model, current / 'cpp', Path(sys.executable).resolve(), WORKER, capability, owned / 'service_jobs', current / 'job.json.gz', '--controller'], env=env)
                    row['job'] = read(current / 'job.json.gz')
                    require(row['job']['complete'] is True and row['job']['outcome'] == 'resolved', 'complete original compiler and current generated SDK')
                    model.write_bytes(base64.b64decode(row['job']['source_package_bytes_base64'], validate=True))
                    run(name + '_original_generate', [*config['reference_generator']['command'], model, current / 'original', current / 'inventory.json', '--source-package', '--semantic-storage'], env=env)
                    row.update(original_sources=hashes(current / 'original'), cpp_sources=hashes(current / 'cpp'), inventory=json.loads((current / 'inventory.json').read_bytes()))
                    require(row['original_sources'] == row['job']['original_sources_before'] and row['cpp_sources'] == row['job']['cpp_sources'], 'whole independent original and actual published SDK identities')
                    bins = []
                    for consumer in SOURCES:
                        (source, adaptation) = rust_observer(consumer, case)
                        observer = current / (consumer + '.rs')
                        observer.write_text(source)
                        row['consumers'][consumer] = dict(inputs=inputs(consumer, case), rust_source=source, rust_argument_adapter=adaptation)
                        bins.append('[[bin]]\nname="' + consumer + '"\npath=' + json.dumps(str(observer)) + '\n')
                    base = manifest(current / 'original', 'domain', current / 'checker.rs')
                    (header, tail) = base.split('[[bin]]\n', 1)
                    (_, dependencies) = tail.split('[dependencies]\n', 1)
                    text = header + ''.join(bins) + '[dependencies]\n' + dependencies
                    harness = current / 'harness'
                    harness.mkdir()
                    (harness / 'Cargo.toml').write_text(text)
                    shutil.copyfile(ROOT / '.reference/teaql-rs/Cargo.lock', harness / 'Cargo.lock')
                    run(name + '_original_resolve', ['cargo', 'metadata', '--offline', '--format-version', '1', '--manifest-path', harness / 'Cargo.toml'], env=env)
                    require(identities(harness / 'Cargo.lock') <= identities(ROOT / '.reference/teaql-rs/Cargo.lock'), 'exact reference dependency identities')
                    run(name + '_original_build', ['cargo', 'build', '--offline', '--locked', '--bins', '--manifest-path', harness / 'Cargo.toml', '-j', '2'], env=env)
                    for (consumer, (cpp, _)) in SOURCES.items():
                        command = native_command(current / 'cpp/include', cpp, owned / ('native_' + consumer))
                        command[1:1] = ['-I' + str(ROOT / 'tests/parity'), '-DTEAQL_EXPECTED_VALUE_TYPE=' + case['carrier']]
                        run(name + '_' + consumer + '_native_build', command, env=env)
                        values = row['consumers'][consumer]
                        payload = ''.join((json.dumps(value) + '\n' for value in values['inputs']))
                        values['original'] = json_lines(run(name + '_' + consumer + '_original_run', [owned / 'consumer_target/debug' / consumer], env=fixed, payload=payload))
                        values['cpp'] = json_lines(run(name + '_' + consumer + '_native_run', [owned / ('native_' + consumer)], env=fixed, payload=payload))
                    row.update(consumer_manifest=text, consumer_lock=(harness / 'Cargo.lock').read_text(), original_sources_after=hashes(current / 'original'), cpp_sources_after=hashes(current / 'cpp'))
                record['complete'] = True
            finally:
                record['owned_bytes_removed'] = sum((p.stat().st_size for p in owned.rglob('*') if p.is_file()))
    except BaseException as error:
        record.update(complete=False, error=repr(error))
        raise
    finally:
        record.update(owned_directory_removed=owned is not None and (not owned.exists()), inputs_after=execution_inputs(), references_after=references(), native_after=native_inputs(), clock_after=sha(CLOCK.read_bytes()))
        write(path, record)
    require(record['inputs_before'] == record['inputs_after'] and record['references_before'] == record['references_after'] and (record['native_before'] == record['native_after']) and (record['clock_before'] == record['clock_after']), 'frozen complete scalar family execution inputs')
    print('Scalar families collected; semantic comparison remains a separate exit', flush=True)

def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--execute', action='store_true', required=True)
    parser.add_argument('--record', type=Path, required=True)
    parser.add_argument('--case', action='append')
    args = parser.parse_args()
    execute(args.record, args.case)
if __name__ == '__main__':
    main()
