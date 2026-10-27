#!/usr/bin/env python3
"""Regenerate C++ contract SDKs with strict body and file-set equality.

Normally every file must remain byte-identical. Explicit recording modes allow
only generated ASSIST.md and shared support.hpp updates. Every other file stays
identical. Original Rust and historical C++ stay untouched.
"""
import argparse
import difflib
import hashlib
import json
from pathlib import Path
import re
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[1]
PRODUCER = ROOT.parent / 'teaql_code_gen_cpp'
FAMILIES = {'scalar_candidate_contracts': 'CandidateMetadataProbe',
            'type_configuration_contracts': 'TypeConfigurationProbe',
            'sample_type_configuration_contracts': 'TypeConfigurationProbe',
            'constant_metadata_contracts': 'ConstantMetadataProbe',
            'constant_topology_contracts': 'ConstantMetadataProbe',
            'opaque_type_configuration_contracts': 'TypeConfigurationProbe'}
METADATA_HELPER_INPUTS = tuple(
    f'generator/src/main/java/com/skynet/codegenerator/cpp/{name}.java'
    for name in ('CppFieldLayoutMetadata', 'CppRustLiteralMetadata',
                 'CppRustXidMetadata', 'CppJsonInputMetadata'))


def metadata_manifest_paths(root):
    paths = [root / 'examples/generated/manifest.json']
    paths.extend(sorted((root / 'tests').glob('generated_*/manifest.json')))
    for family in FAMILIES:
        paths.extend(sorted((root / 'tests/generator/parity' / family)
                            .glob('*/cpp/PROVENANCE.json')))
    paths.extend(root / 'tests/generator/parity' / family / 'cpp/PROVENANCE.json'
                 for family in ('query_alias_contracts', 'relation_identity_contracts',
                                'expression_root_contracts', 'hydration_allocation_contracts',
                                'graph_serialization_contracts'))
    return paths


def digest(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def without_wire_functions(text):
    """Remove only the two newly generated metadata functions, preserving all old bodies."""
    pattern = r'inline\s+GeneratedWireFieldMappings&\s+generatedWireField(?:Mappings|Aliases)\s*\('
    count = 0
    while match := re.search(pattern, text):
        opening = text.index('{', match.end())
        depth = 0
        for index in range(opening, len(text)):
            if text[index] == '{':
                depth += 1
            elif text[index] == '}':
                depth -= 1
                if depth == 0:
                    text = text[:match.start()] + text[index + 1:].lstrip('\r\n')
                    count += 1
                    break
        else:
            raise RuntimeError('Incomplete generated wire metadata function')
    if count != 2:
        raise RuntimeError('Expected exactly two new wire metadata functions')
    return text


def without_request_state_adapters(text):
    """Remove only five exact new delegates; preserve every existing request body."""
    names = ('relationSelections', 'relationFilters', 'childEnhancements', 'queryOptions', 'intoQuery')
    counts = {}
    for name in names:
        qualifier = r'&&' if name == 'intoQuery' else r'const'
        result = 'SelectQuery' if name == 'intoQuery' else 'auto'
        pattern = (rf'{result}\s+{name}\(\)\s*{qualifier}\s*requires\s*\(!Purposed\)\s*'
                   r'\{\s*([^{}]*)\}\s*')
        expected = ('returnstd::move(query_).intoQuery();' if name == 'intoQuery'
                    else f'returnquery_.{name}();')
        def remove(match):
            if re.sub(r'\s+', '', match[1]) != expected:
                raise RuntimeError(f'Non-delegate request state body: {name}')
            return ''
        text, counts[name] = re.subn(pattern, remove, text)
    if not counts[names[0]] or len(set(counts.values())) != 1:
        raise RuntimeError(f'Incomplete request state adapter set: {counts}')
    return text, sum(counts.values())


def without_runtime_configuration(text, environment, has_business_ids):
    """Remove only reviewed source-shaped adapters; leave filename/setup bodies intact.

    This is a change-scope check, not execution acceptance. The environment
    constant comes from the unchanged original generated Rust artifact.
    """
    prefix = '''using ServiceRuntime = UserContext;
using ServiceRuntimeError = GeneratedServiceRuntimeError<SqliteDriverError>;
using DataServiceConnection = Ref<SqliteConnection>;
inline constexpr std::string_view DATABASE_URL_ENV = "ENVIRONMENT";
struct ServiceRuntimeConfig : GeneratedServiceRuntimeConfig {
using GeneratedServiceRuntimeConfig::GeneratedServiceRuntimeConfig;
std::string_view databaseUrl() const { return database_url; }
static Result<ServiceRuntimeConfig, ServiceRuntimeError> fromEnv(UserContext& user_context) {
auto r = user_context.memory_resource(); auto value = read_generated_runtime_config(r, DATABASE_URL_ENV);
using Output = Result<ServiceRuntimeConfig, ServiceRuntimeError>;
if (!value) { return Output::err(r, ServiceRuntimeError::from_preparation(r, value.error())); }
return Output::ok(r, ServiceRuntimeConfig(std::allocator_arg, Allocator(r), value.value())); }
};'''.replace('ENVIRONMENT', environment)
    entry = '''inline Ref<DataServiceExecutor> serviceRuntimeFromConnection(UserContext& user_context, DataServiceConnection connection) {
auto& module = moduleWithBehaviorsAndCheckers(user_context);
return assemble_generated_sqlite_runtime<DataServiceExecutor, HAS_BUSINESS_IDS>(user_context, module, connection); }
inline Result<Ref<DataServiceExecutor>, ServiceRuntimeError> serviceRuntime(UserContext& user_context, const ServiceRuntimeConfig& config) {
auto r = user_context.memory_resource(); using Output = Result<Ref<DataServiceExecutor>, ServiceRuntimeError>;
if (config.memory_resource() != r) { throw std::invalid_argument("Generated runtime configuration requires an explicit destination copy"); }
auto connection = open_generated_sqlite_runtime_connection(user_context, config.databaseUrl());
if (!connection) { return Output::err(r, ServiceRuntimeError::from_preparation(r, connection.error())); }
return Output::ok(r, serviceRuntimeFromConnection(user_context, connection.value())); }
inline Result<Ref<DataServiceExecutor>, ServiceRuntimeError> serviceRuntimeFromEnv(UserContext& user_context) {
auto r = user_context.memory_resource(); auto config = ServiceRuntimeConfig::fromEnv(user_context);
if (!config) { return Result<Ref<DataServiceExecutor>, ServiceRuntimeError>::err(r, std::move(config).error()); }
return serviceRuntime(user_context, config.value()); }'''.replace('HAS_BUSINESS_IDS', str(has_business_ids).lower())
    for start_name, end_name, expected in (
            ('using ServiceRuntime = UserContext;', 'inline GeneratedWireFieldMappings&', prefix),
            ('inline Ref<DataServiceExecutor> serviceRuntimeFromConnection',
             'inline Ref<DataServiceExecutor> configureRuntime', entry)):
        if text.count(start_name) != 1:
            raise RuntimeError(f'Missing or duplicated configuration adapter: {start_name}')
        start = text.index(start_name)
        end = text.index(end_name, start)
        if re.sub(r'\s+', '', text[start:end]) != re.sub(r'\s+', '', expected):
            raise RuntimeError(f'Unexpected configuration adapter implementation: {start_name}')
        text = text[:start] + text[end:]
    return text


def restore_runtime_binding(text, previous):
    """Allow exactly one reviewed filename delegate replacement, preserving all other code."""
    pattern = (r'inline\s+Ref<DataServiceExecutor>\s+configureRuntime\(UserContext&\s+user_context,\s*'
               r'std::string_view\s+filename\s*=\s*":memory:"\)\s*\{([^{}]*)\}')
    old = re.search(pattern, previous)
    new = re.search(pattern, text)
    if not old or not new or len(re.findall(pattern, text)) != 1:
        raise RuntimeError('Missing or duplicated filename runtime adapter')
    business_ids = 'make<SqliteBusinessIdAllocator>' in old[1]
    expected = '''auto& module = moduleWithBehaviorsAndCheckers(user_context);
module.apply_to(user_context); auto& metadata = module.metadata;
user_context.set_metadata(user_context, metadata);
auto connection = user_context.make_resource_ref<SqliteConnection>(filename);
return bind_generated_sqlite_runtime<DataServiceExecutor, HAS_BUSINESS_IDS>(user_context, metadata, connection);'''.replace(
        'HAS_BUSINESS_IDS', str(business_ids).lower())
    if re.sub(r'\s+', '', new[1]) != re.sub(r'\s+', '', expected):
        raise RuntimeError('Filename runtime adapter changed its order or shared delegation')
    return text[:new.start()] + old[0] + text[new.end():]


def restore_registry_schema_adapters(text, previous):
    """Review the complete new dispatch/registry block and two binding substitutions."""
    module = re.search(r'inline\s+RuntimeModule&\s+module\(UserContext&\s+user_context\)\s*\{([^{}]*)\}', previous)
    if not module:
        raise RuntimeError('Missing existing canonical model registration')
    models = re.findall(r'module\.entity<(\w+)>\(\);', module[1])
    if not models:
        raise RuntimeError('No original registered models')
    expected = '''using DataServiceDialect = SqliteDialect;
using DataServiceMutationExecutor = SqliteMutationExecutor;
using DataServiceMutationError = MutationExecutorError;
using DataServiceIdGenerator = SqliteIdSpaceGenerator;
class LocalSchemaProvider final : public GeneratedLocalSchemaProvider {
public: LocalSchemaProvider(std::allocator_arg_t, Allocator a) : GeneratedLocalSchemaProvider(std::allocator_arg, a, lookup) {}
LocalSchemaProvider(std::allocator_arg_t, Allocator a, const LocalSchemaProvider& other) : GeneratedLocalSchemaProvider(std::allocator_arg, a, other) {}
private: static std::optional<EntityDescriptor> lookup(Memory r, std::string_view name) {
'''
    for model in models:
        expected += f'if (name == {model}::ENTITY_NAME) {{ return {model}::entity_descriptor(r); }}\n'
    expected += '''return std::nullopt; } };
inline InMemoryEntityRegistry& repositoryRegistry(UserContext& user_context) { auto& registry = user_context.make<InMemoryEntityRegistry>();
'''
    for model in models:
        expected += f'registry.register_entity({model}::ENTITY_NAME);\n'
    expected += '''return registry; }
inline InMemoryEntityDataServiceBehaviorRegistry& behaviorRegistry(UserContext& user_context) { auto& registry = user_context.make<InMemoryEntityDataServiceBehaviorRegistry>();
'''
    for model in models:
        expected += f'{{ auto& behavior = user_context.make<{model}Behavior>(); registry.register_behavior({model}::ENTITY_NAME, user_context, behavior); }}\n'
    expected += '''return registry; }
inline InMemoryCheckerRegistry& checkerRegistry(UserContext& user_context) { auto& registry = user_context.make<InMemoryCheckerRegistry>();
'''
    for model in models:
        expected += f'{{ auto& checker = user_context.make<{model}Checker>(); auto& adapter = user_context.make<TypedEntityChecker<{model}, {model}Checker>>(user_context, checker); registry.register_checker(user_context, adapter); }}\n'
    expected += 'return registry; }'
    start = text.index('using DataServiceDialect =')
    end = text.index('inline GeneratedWireFieldMappings&', start)
    if re.sub(r'\s+', '', text[start:end]) != re.sub(r'\s+', '', expected):
        raise RuntimeError('Unexpected generated registry/schema mechanics or model coverage')
    text = text[:start] + text[end:]
    pattern = r'auto&\s+schema_provider\s*=\s*user_context\.make<LocalSchemaProvider>\(\);\s*'
    text, count = re.subn(pattern, '', text)
    if count != 2:
        raise RuntimeError('Expected distinct local schemas at both runtime assembly entries')
    text, count = re.subn(r'\(\s*user_context,\s*module,\s*connection,\s*schema_provider\s*\)',
                          '(user_context, module, connection)', text)
    if count != 1:
        raise RuntimeError('Unexpected source-shaped assembly binding')
    text, count = re.subn(r'\(\s*user_context,\s*schema_provider,\s*connection\s*\)',
                          '(user_context, metadata, connection)', text)
    if count != 1:
        raise RuntimeError('Unexpected filename schema binding')
    return text


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--check', action='store_true')
    parser.add_argument('--allow-sample-operand-change', action='store_true',
                        help='Record only source-qualified JSON sample delegates to the shared runtime')
    parser.add_argument('--allow-business-id-lookup-change', action='store_true',
                        help='Permit only model-derived typed Q lookup delegates and exact guidance')
    parser.add_argument('--allow-result-marker-change', action='store_true',
                        help='Permit only unconstrained result markers, Entity execution guards and exact guidance')
    parser.add_argument('--allow-expression-entry-change', action='store_true',
                        help='Permit only static API entry metadata in existing E factory calls')
    parser.add_argument('--allow-scalar-diagnostic-change', action='store_true',
                        help='Permit only exact scalar keyword Q/E forwarding aliases and their guide')
    parser.add_argument('--allow-ledger-composition-change', action='store_true',
                        help='Permit only the exact Context-owned ledger-composition delegate and guide')
    parser.add_argument('--allow-relation-identity-change', action='store_true',
                        help='Permit only canonical relationship literals, source descriptor order and exact Q/E/Assist delegates')
    parser.add_argument('--allow-default-alias-change', action='store_true',
                        help='Permit only original result aliases, query/descriptor identity separation and exact Assist literals')
    parser.add_argument('--allow-assist-change', action='store_true',
                        help='Record genuinely generated ASSIST.md changes; require every other byte identical')
    parser.add_argument('--allow-support-change', action='store_true',
                        help='Record only shared support.hpp adapters and Assist changes; all other code stays identical')
    parser.add_argument('--allow-runtime-configuration-change', action='store_true',
                        help='Record only configuration/env/connection adapters, four support includes and Assist; preserve all old bodies')
    parser.add_argument('--allow-runtime-binding-change', action='store_true',
                        help='Record only the filename adapter shared-binding replacement; preserve install/open ordering and all other bodies')
    parser.add_argument('--allow-registry-schema-change', action='store_true',
                        help='Record exact model registry/schema adapters, aliases, one include and two schema bindings; preserve all other code')
    parser.add_argument('--allow-business-allocator-alias-change', action='store_true',
                        help='Record the conditional Business-ID allocator alias and model-specific Assist only')
    parser.add_argument('--allow-timestamp-default-change', action='store_true',
                        help='Record only required Timestamp initializer replacements verified through pinned formatting')
    parser.add_argument('--allow-wire-mapping-change', action='store_true',
                        help='Record only two new runtime mapping functions, their include and generated guidance')
    parser.add_argument('--allow-request-state-change', action='store_true',
                        help='Record only five exact request state delegates and generated guidance')
    parser.add_argument('--allow-core-query-view-change', action='store_true',
                        help='Record only the exact coreQuery delegate and generated guidance')
    parser.add_argument('--allow-matching-statistics-input-change', action='store_true',
                        help='Record only exact matching/roll-up/field-statistic delegates and Assist')
    parser.add_argument('--allow-relation-analysis-input-change', action='store_true',
                        help='Record only exact facet/group/relation aggregate input delegates and Assist')
    parser.add_argument('--allow-relation-selection-delegate-change', action='store_true',
                        help='Record only exact typed selector delegation replacements and Assist')
    parser.add_argument('--allow-relation-selection-input-change', action='store_true',
                        help='Record only six exact runtime selector input delegates per relation and Assist')
    parser.add_argument('--allow-enhancement-input-change', action='store_true',
                        help='Record only exact enhancement value overloads and public conversion replacements')
    parser.add_argument('--allow-type-match-range-change', action='store_true',
                        help='Record only exact constrained matchTypes delegates and Assist changes')
    parser.add_argument('--allow-membership-range-change', action='store_true',
                        help='Record only constrained IN/NOT IN range delegates and guidance')
    parser.add_argument('--allow-scalar-comparison-change', action='store_true',
                        help='Record only constrained scalar comparison delegates and guidance')
    parser.add_argument('--allow-sound-value-change', action='store_true',
                        help='Record only constrained sound-like Value delegates and guidance')
    parser.add_argument('--allow-query-branch-change', action='store_true',
                        help='Record only new source-selected numeric/reference query delegates')
    parser.add_argument('--report', type=Path,
                        help='Write a new reproduction report without replacing historical evidence')
    args = parser.parse_args()
    if args.check and (args.allow_sample_operand_change or args.allow_business_id_lookup_change or args.allow_result_marker_change or args.allow_expression_entry_change or args.allow_scalar_diagnostic_change or args.allow_ledger_composition_change or args.allow_relation_identity_change or args.allow_default_alias_change or args.allow_assist_change or args.allow_support_change or args.allow_runtime_configuration_change or args.allow_runtime_binding_change or args.allow_registry_schema_change or args.allow_business_allocator_alias_change
                       or args.allow_timestamp_default_change or args.allow_wire_mapping_change
                       or args.allow_request_state_change or args.allow_core_query_view_change
                       or args.allow_membership_range_change or args.allow_matching_statistics_input_change or args.allow_relation_analysis_input_change or args.allow_relation_selection_input_change or args.allow_relation_selection_delegate_change or args.allow_enhancement_input_change or args.allow_type_match_range_change or args.allow_scalar_comparison_change
                       or args.allow_sound_value_change or args.allow_query_branch_change or args.report):
        parser.error('--check cannot record instruction changes or reports')
    staged, records = [], []
    with tempfile.TemporaryDirectory(prefix='teaql_cpp_contract_reproduction_') as temporary:
        work = Path(temporary)
        classpath = work / 'classpath.txt'
        subprocess.run(['mvn', '-q', '-pl', 'generator', 'dependency:build-classpath',
                        f'-Dmdep.outputFile={classpath}'], cwd=PRODUCER, check=True)
        cp = ':'.join(str(PRODUCER / p) for p in ('generator/target/classes', 'model/target/classes'))
        cp += ':' + classpath.read_text().strip()
        subprocess.run(['javac', '-cp', cp, '-d', str(work),
                        *(str(ROOT / 'tests/generator' / (name + '.java'))
                          for name in sorted(set(FAMILIES.values()) | {'QueryAliasMetadataProbe', 'RelationIdentityMetadataProbe', 'ScalarDiagnosticMetadataProbe', 'BusinessIdLookupMetadataProbe'}))], check=True)
        for family, probe in FAMILIES.items():
            for case in sorted((ROOT / 'tests/generator/parity' / family).iterdir()):
                if not case.is_dir():
                    continue
                destination = case / 'cpp'
                provenance = json.loads((destination / 'PROVENANCE.json').read_text())
                retained = {p.relative_to(destination).as_posix(): digest(p)
                            for p in destination.rglob('*')
                            if p.is_file() and p.name != 'PROVENANCE.json'}
                if retained != provenance['files']:
                    raise RuntimeError(f'Retained SDK bytes differ from provenance: {case}')
                if digest(case / 'model.xml') != provenance['model_sha256']:
                    raise RuntimeError(f'Model input changed: {case}')
                output = work / family / case.name
                result = subprocess.run(['java', '-cp', str(work) + ':' + cp, probe, 'cpp',
                                         str(case / 'model.xml'), str(output)],
                                        text=True, capture_output=True)
                if result.returncode:
                    raise RuntimeError(f'{family}/{case.name}: generation failed: {result.stderr}')
                headers = sorted(str(p) for p in output.rglob('*.hpp'))
                if headers:
                    subprocess.run([str(ROOT / '.tools/clang-format/bin/clang-format'),
                                    f'--style=file:{ROOT / ".clang-format"}', '--Werror', '-i', *headers],
                                   check=True)
                files = {p.relative_to(output).as_posix(): digest(p)
                         for p in output.rglob('*') if p.is_file()}
                changed = sorted(set(files) ^ set(provenance['files']) |
                                 {p for p in files.keys() & provenance['files'].keys()
                                  if files[p] != provenance['files'][p]})
                allowed = {'ASSIST.md'} if args.allow_assist_change or args.allow_support_change else set()
                sample_operand_delegates = 0
                if args.allow_sample_operand_change and changed:
                    from sample_operand_qualification import verify_file
                    for name in changed:
                        models_path = name.replace('/sample_data.hpp', '/models.hpp')
                        sample_operand_delegates += verify_file(ROOT, name,
                            (destination / name).read_text(), (output / name).read_text(),
                            (destination / models_path).read_text())
                        allowed.add(name)
                business_id_lookup_changes = 0
                if args.allow_business_id_lookup_change and changed:
                    from business_id_lookup_qualification import metadata as lookup_metadata, verify_file as verify_lookup
                    lookups = lookup_metadata(case / 'model.xml', work / 'lookup_metadata.json', cp, work)
                    for name in changed:
                        business_id_lookup_changes += verify_lookup(ROOT, name, (destination / name).read_text(), (output / name).read_text(), lookups)
                        allowed.add(name)
                result_marker_changes = 0
                if args.allow_result_marker_change and changed:
                    from query_result_marker_qualification import verify_file as verify_marker
                    for name in changed:
                        result_marker_changes += verify_marker(ROOT, name, (destination / name).read_text(), (output / name).read_text())
                        allowed.add(name)
                expression_entry_changes = 0
                if args.allow_expression_entry_change and changed:
                    from expression_entry_qualification import verify_file as verify_entry
                    for name in changed:
                        expression_entry_changes += verify_entry(ROOT, name, (destination / name).read_text(), (output / name).read_text())
                        allowed.add(name)
                alias_changes = 0
                scalar_diagnostic_changes = 0
                if args.allow_scalar_diagnostic_change and changed:
                    from scalar_diagnostic_qualification import metadata as scalar_metadata, verify_file as verify_scalar
                    scalars = scalar_metadata(case / 'model.xml', work / 'scalar_metadata.json', cp, work)
                    for name in changed:
                        scalar_diagnostic_changes += verify_scalar(ROOT, name, (destination / name).read_text(), (output / name).read_text(), scalars)
                        allowed.add(name)
                ledger_composition_changes = 0
                if args.allow_ledger_composition_change and changed:
                    from ledger_composition_qualification import verify_file as verify_composition_file
                    for name in changed:
                        ledger_composition_changes += verify_composition_file(ROOT, name, (destination / name).read_text(), (output / name).read_text())
                        allowed.add(name)
                relation_identity_changes = 0
                if args.allow_relation_identity_change and changed:
                    from relation_identity_qualification import metadata as relation_metadata, verify_file as verify_relation_file
                    relations = relation_metadata(case / 'model.xml', work / 'relation_metadata.json', cp, work)
                    for name in changed:
                        relation_identity_changes += verify_relation_file(ROOT, name, (destination / name).read_text(),
                                                                         (output / name).read_text(), relations)
                        allowed.add(name)
                if args.allow_default_alias_change and changed:
                    if any(name != 'ASSIST.md' and not name.endswith(('/queries.hpp', '/models.hpp')) for name in changed):
                        raise RuntimeError(f'Alias repair changed unrelated files: {changed}')
                    from query_alias_qualification import metadata, verify_file
                    aliases = metadata(case / 'model.xml', work / 'alias_metadata.json', cp, work)
                    for name in changed:
                        alias_changes += verify_file(ROOT, name, (destination / name).read_text(),
                                                     (output / name).read_text(), aliases)
                        allowed.add(name)
                request_state_additions = 0
                membership_additions = 0
                scalar_comparison_additions = 0
                sound_value_additions = 0
                query_branch_additions = 0
                runtime_configuration_additions = 0
                runtime_binding_replacements = 0
                registry_schema_additions = 0
                business_allocator_alias_additions = 0
                if args.allow_business_allocator_alias_change:
                    for path in changed:
                        if path == 'ASSIST.md':
                            # Require the domain's already generated Business-ID
                            # binding instead of admitting unrelated guide edits.
                            runtime_paths = [name for name in files if name.endswith('/runtime.hpp')]
                            if len(runtime_paths) != 1 or not re.search(
                                    r'assemble_generated_sqlite_runtime<DataServiceExecutor,\s*true>',
                                    (destination / runtime_paths[0]).read_text()):
                                raise RuntimeError('Business-ID guidance added to an ordinary domain')
                            addition = ('DataServiceBusinessIdAllocator names the SQLite-backed allocation/schema implementation for this modeled Business-ID domain; ordinary domains omit the alias. Register runtime infrastructure before installing the application-owned key provider. Sample generation leaves Business-ID values to configured Checker/Fix; a missing key can fail a governed sample save while earlier commits remain.\n')
                            guide = (output / path).read_text()
                            if guide.count(addition) != 1 or guide.replace(addition, '') != (destination / path).read_text():
                                raise RuntimeError('Business-ID alias guidance changed unrelated instructions')
                            allowed.add(path)
                        elif path.startswith('include/') and path.endswith('/runtime.hpp'):
                            previous = (destination / path).read_text()
                            generated = (output / path).read_text()
                            alias = 'using DataServiceBusinessIdAllocator = SqliteBusinessIdAllocator;\n'
                            if generated.count(alias) != 1 or alias in previous or not re.search(
                                    r'assemble_generated_sqlite_runtime<DataServiceExecutor,\s*true>', previous):
                                raise RuntimeError('Invalid conditional Business-ID allocator alias')
                            restored = generated.replace(alias, '')
                            formatted = subprocess.run(
                                [str(ROOT / '.tools/clang-format/bin/clang-format'),
                                 f'--style=file:{ROOT / ".clang-format"}', '--Werror',
                                 '--assume-filename=' + path], input=restored, text=True,
                                capture_output=True, check=True).stdout
                            if formatted != previous:
                                raise RuntimeError('Allocator alias changed an existing generated body')
                            allowed.add(path)
                            business_allocator_alias_additions += 1
                if args.allow_registry_schema_change:
                    allowed.add('ASSIST.md')
                    for path in changed:
                        if not path.startswith('include/'):
                            continue
                        previous = (destination / path).read_text()
                        restored = (output / path).read_text()
                        if path.endswith('/runtime.hpp'):
                            restored = restore_registry_schema_adapters(restored, previous)
                            registry_schema_additions += 8
                        elif path.endswith('/support.hpp'):
                            include = '#include <teaql/runtime/generated_schema_provider.hpp>\n'
                            if restored.count(include) != 1 or include in previous:
                                raise RuntimeError('Unexpected schema support include')
                            restored = restored.replace(include, '')
                        else:
                            continue
                        formatted = subprocess.run(
                            [str(ROOT / '.tools/clang-format/bin/clang-format'),
                             f'--style=file:{ROOT / ".clang-format"}', '--Werror',
                             '--assume-filename=' + path], input=restored, text=True,
                            capture_output=True, check=True).stdout
                        if formatted != previous:
                            raise RuntimeError(f'Registry/schema update changed unrelated code: {family}/{case.name}/{path}')
                        allowed.add(path)
                if args.allow_runtime_binding_change:
                    for path in changed:
                        if not path.startswith('include/') or not path.endswith('/runtime.hpp'):
                            continue
                        previous = (destination / path).read_text()
                        restored = restore_runtime_binding((output / path).read_text(), previous)
                        formatted = subprocess.run(
                            [str(ROOT / '.tools/clang-format/bin/clang-format'),
                             f'--style=file:{ROOT / ".clang-format"}', '--Werror',
                             '--assume-filename=' + path], input=restored, text=True,
                            capture_output=True, check=True).stdout
                        if formatted != previous:
                            raise RuntimeError(f'Runtime binding changed unrelated bodies: {family}/{case.name}/{path}')
                        allowed.add(path)
                        runtime_binding_replacements += 1
                if args.allow_runtime_configuration_change:
                    allowed.add('ASSIST.md')
                    environment = None
                    if any(path.endswith('/runtime.hpp') for path in changed):
                        original_runtime = case / 'rust/lib/src/runtime.rs'
                        environment = re.search(r'pub const DATABASE_URL_ENV: &str = "([^"]+)";',
                                                original_runtime.read_text())
                        if not environment:
                            raise RuntimeError(f'Missing original environment contract: {original_runtime}')
                    for path in changed:
                        if not path.startswith('include/'):
                            continue
                        previous = (destination / path).read_text()
                        retained = (output / path).read_text()
                        if path.endswith('/support.hpp'):
                            for header in ('teaql/runtime/generated_runtime_config.hpp',
                                           'teaql/runtime/generated_service_runtime_error.hpp',
                                           'teaql/provider_sqlite/runtime_assembly.hpp',
                                           'teaql/provider_sqlite/driver_error.hpp'):
                                addition = '#include <' + header + '>\n'
                                if retained.count(addition) != 1 or addition in previous:
                                    raise RuntimeError(f'Invalid configuration include: {path}: {header}')
                                retained = retained.replace(addition, '')
                        elif path.endswith('/runtime.hpp'):
                            retained = without_runtime_configuration(
                                retained, environment[1], 'make<SqliteBusinessIdAllocator>' in previous)
                            runtime_configuration_additions += 8
                        else:
                            continue
                        formatted = subprocess.run(
                            [str(ROOT / '.tools/clang-format/bin/clang-format'),
                             f'--style=file:{ROOT / ".clang-format"}', '--Werror',
                             '--assume-filename=' + path], input=retained, text=True,
                            capture_output=True, check=True).stdout
                        if formatted != previous:
                            raise RuntimeError(f'Configuration update changed existing bodies: {family}/{case.name}/{path}')
                        allowed.add(path)
                if args.allow_query_branch_change:
                    allowed.add('ASSIST.md')
                    pattern = (r'(?:template\s*<request::EntityReference\s+Reference>\s*)?'
                               r'(\w+Request)\s*(&&|&)\s*'
                               r'((?:select\w+(?:AsWithFunction|WithFunction)|standardDeviation\w+|squareRootOfPopulationStandardDeviation\w+|sampleVariance\w+|samplePopulationVariance\w+|filterBy\w+))'
                               r'\(([^()]*)\)\s*(&&|&)\s*\{([^{}]*(?:\{[^{}]*\}[^{}]*)?)\}\s*')
                    for path in changed:
                        if not path.startswith('include/') or not path.endswith('/queries.hpp'):
                            continue
                        previous=(destination/path).read_text()
                        normalized_previous=re.sub(r'\s+','',previous)
                        count=[0]
                        def remove_new_branch(match):
                            owner,returns,name,args,category,body=match.groups()
                            signature=re.sub(r'\s+','',f'{owner}{returns}{name}({args}){category}'+'{')
                            if signature in normalized_previous:
                                return match[0]
                            assert returns==category
                            body=re.sub(r'\s+','',body)
                            # Constant requests retain their existing selection-mutation
                            # wrapper. Admit only that exact wrapper around a new delegate.
                            if category == '&' and body.startswith('query_.mutate_selection('):
                                wrapped = re.fullmatch(
                                    r'query_\.mutate_selection\(\[&\]\(SelectQuery&selection\)'
                                    r'\{([^{}]*)\}\);return\*this;', body)
                                assert wrapped, body
                                body = wrapped[1] + 'return*this;'
                            if category=='&&':
                                arguments='std::forward<Reference>(value)' if name.startswith('filterBy') else ','.join(p.strip().split()[-1] for p in args.split(',') if p.strip())
                                assert body==f'this->{name}({arguments});returnstd::move(*this);',body
                            elif name.startswith('filterBy'):
                                assert re.fullmatch(r'query_\.andFilter\(Expr::eq\(memory_resource\(\),"(?:[^"\\]|\\.)*",request::entity_id_value\(memory_resource\(\),std::forward<Reference>\(value\)\)\)\);return\*this;',body),body
                            else:
                                assert re.fullmatch(r'query_\.(?:selectWithFunction|aggregateStddev|aggregateStddevPop|aggregateVarSamp|aggregateVarPop)\([^;]+\);return\*this;',body),body
                            count[0]+=1
                            return ''
                        retained=re.sub(pattern,remove_new_branch,(output/path).read_text())
                        if not count[0] or count[0]%2:
                            raise RuntimeError(f'Incomplete new branch pairs: {path}: {count[0]}')
                        formatted=subprocess.run([str(ROOT/'.tools/clang-format/bin/clang-format'),f'--style=file:{ROOT/".clang-format"}','--Werror','--assume-filename='+path],input=retained,text=True,capture_output=True,check=True).stdout
                        if formatted!=previous:
                            difference = ''.join(difflib.unified_diff(
                                previous.splitlines(keepends=True),
                                formatted.splitlines(keepends=True),
                                fromfile='retained/' + path,
                                tofile='new-with-added-delegates-removed/' + path))
                            raise RuntimeError(
                                f'Branch update changed an existing body: {family}/{case.name}/{path}\n'
                                + difference)
                        allowed.add(path);query_branch_additions+=count[0]
                if args.allow_sound_value_change:
                    allowed.add('ASSIST.md')
                    pattern = (r'template\s*<class\s+Input>\s*requires\s*std::constructible_from<Value,\s*Memory,\s*const Input&>\s*'
                               r'\w+Request\s*(&&|&)\s*(\w+Sound\w*Like)\(const Input&\s*value\)\s*(&&|&)\s*\{([^{}]*)\}\s*')
                    for path in changed:
                        if not path.startswith('include/') or not path.endswith('/queries.hpp'):
                            continue
                        def remove_sound(match):
                            name,category,body=match[2],match[3],re.sub(r'\s+','',match[4])
                            assert match[1]==category
                            if category=='&&':
                                assert body==f'this->{name}(value);returnstd::move(*this);'
                            else:
                                assert re.fullmatch(r'query_\.andFilter\(Expr::sound_like\(memory_resource\(\),"(?:[^"\\]|\\.)*",value\)\);return\*this;',body),body
                            return ''
                        retained,count=re.subn(pattern,remove_sound,(output/path).read_text())
                        if not count or count%2:
                            raise RuntimeError(f'Incomplete sound Value pairs: {path}')
                        formatted=subprocess.run([str(ROOT/'.tools/clang-format/bin/clang-format'),
                            f'--style=file:{ROOT/".clang-format"}','--Werror','--assume-filename='+path],
                            input=retained,text=True,capture_output=True,check=True).stdout
                        if formatted!=(destination/path).read_text():
                            raise RuntimeError(f'Sound update changed previous bodies: {family}/{case.name}/{path}')
                        allowed.add(path);sound_value_additions+=count
                if args.allow_scalar_comparison_change:
                    allowed.add('ASSIST.md')
                    single = (r'template\s*<class\s+Input>\s*requires\s*std::constructible_from<Value,\s*Memory,\s*const Input&>\s*'
                              r'\w+Request\s*(&&|&)\s*(\w+)\(const Input&\s*value\)\s*(&&|&)\s*\{([^{}]*)\}\s*')
                    between = (r'template\s*<class\s+Lower,\s*class\s+Upper>\s*requires\s*'
                               r'std::constructible_from<Value,\s*Memory,\s*const Lower&>\s*&&\s*'
                               r'std::constructible_from<Value,\s*Memory,\s*const Upper&>\s*'
                               r'\w+Request\s*(&&|&)\s*(\w+Between)\(const Lower&\s*lower,\s*const Upper&\s*upper\)\s*(&&|&)\s*\{([^{}]*)\}\s*')
                    for path in changed:
                        if not path.startswith('include/') or not path.endswith('/queries.hpp'):
                            continue
                        def remove_comparison(match, two=False):
                            name, category, body = match[2], match[3], re.sub(r'\s+', '', match[4])
                            assert match[1] == category
                            arguments = 'lower,upper' if two else 'value'
                            if category == '&&':
                                assert body == f'this->{name}({arguments});returnstd::move(*this);'
                            else:
                                operations = [('GreaterThanOrEqualTo','gte'),('LessThanOrEqualTo','lte'),
                                              ('GreaterThan','gt'),('LessThan','lt'),('IsNot','ne'),('Is','eq'),('Between','between')]
                                operation = next(op for suffix,op in operations if name.endswith(suffix))
                                assert re.fullmatch(r'query_\.andFilter\(Expr::'+operation+r'\(memory_resource\(\),"(?:[^"\\]|\\.)*",'
                                                    +arguments+r'\)\);return\*this;',body),body
                            return ''
                        retained = (output / path).read_text()
                        retained, count_one = re.subn(single, remove_comparison, retained)
                        retained, count_two = re.subn(between, lambda m: remove_comparison(m, True), retained)
                        if not count_one or (count_one+count_two)%2:
                            raise RuntimeError(f'Incomplete comparison delegate pairs: {path}')
                        formatted = subprocess.run(
                            [str(ROOT / '.tools/clang-format/bin/clang-format'),
                             f'--style=file:{ROOT / ".clang-format"}', '--Werror', '--assume-filename='+path],
                            input=retained,text=True,capture_output=True,check=True).stdout
                        if formatted != (destination / path).read_text():
                            raise RuntimeError(f'Comparison update changed existing bodies: {family}/{case.name}/{path}')
                        allowed.add(path)
                        scalar_comparison_additions += count_one+count_two
                matching_statistics_additions = 0
                if args.allow_matching_statistics_input_change:
                    allowed.add('ASSIST.md')
                    template = (r'(?:template\s*<class\s+ChildRequest>\s*requires\s*requires\s*'
                                r'\(const\s+ChildRequest&\s+child\)\s*\{\s*child\.runtimeQuery\(\)\.selection\(\);\s*\}\s*)?')
                    method = (r'(?:with\w+Matching|without\w+Matching|rollUpTo\w+With|'
                              r'(?:sum|avg|min|max|standardDeviation|squareRootOfPopulationStandardDeviation|sampleVariance|samplePopulationVariance)\w+As)')
                    pattern = (template + r'(\w+Request)\s*(&&|&)\s*(' + method + r')\s*\(\s*'
                               r'((?:std::string_view\s+alias,\s*)?const\s+'
                               r'(ChildRequest|SelectQuery|request::QuerySelection)&\s*child)\)\s*(&&|&)\s*\{([^{}]*)\}\s*')
                    quoted = r'"[A-Za-z_][A-Za-z_0-9]*"'
                    for path in changed:
                        if not path.startswith('include/') or not path.endswith('/queries.hpp'):
                            continue
                        pairs = {}
                        def remove_matching_statistics(match):
                            request, returns, name, parameters, kind, category, raw_body = match.groups()
                            body = re.sub(r'\s+', '', raw_body)
                            assert returns == category
                            child = 'child.runtimeQuery()' if kind=='ChildRequest' else 'child'
                            if category == '&&':
                                arguments = ('alias,' if name.endswith('As') else '') + 'child'
                                assert body == f'this->{name}({arguments});returnstd::move(*this);', body
                            elif name.endswith('As'):
                                expected = (r'query_\.aggregateRelationField\(' + quoted + r',alias,' + re.escape(child)
                                            + ',' + quoted + ',' + quoted
                                            + r',AggregateFunction::(?:Sum|Avg|Min|Max|Stddev|StddevPop|VarSamp|VarPop)\);return\*this;')
                                assert re.fullmatch(expected,body),body
                            else:
                                head = (r'query_\.filterRelation\((' + quoted + '),(' + quoted + '),'
                                        r'\w+::entity_descriptor\(memory_resource\(\)\),'
                                        + re.escape(child) + r',(true|false),(' + quoted + r')\);')
                                call = re.match(head,body)
                                assert call,body
                                assert call[3] == ('true' if name.startswith('without') else 'false'),body
                                tail = body[call.end():]
                                if name.startswith('rollUpTo'):
                                    assert tail == f'query_.groupByObject({call[4]},{call[1]},{child});return*this;',tail
                                else:
                                    assert tail == 'return*this;',tail
                            if kind=='ChildRequest':
                                assert 'child.runtimeQuery().selection();' in match[0]
                            pairs.setdefault((request,name),[]).append((kind,category))
                            return ''
                        retained, added = re.subn(pattern, remove_matching_statistics, (output/path).read_text())
                        expected=sorted((kind,category) for kind in ['ChildRequest','SelectQuery','request::QuerySelection'] for category in ['&','&&'])
                        assert pairs and all(sorted(entries)==expected for entries in pairs.values()),pairs
                        formatted = subprocess.run(
                            [str(ROOT/'.tools/clang-format/bin/clang-format'),
                             f'--style=file:{ROOT/".clang-format"}', '--Werror', '--assume-filename='+path],
                            input=retained, text=True, capture_output=True, check=True).stdout
                        if formatted != (destination/path).read_text():
                            raise RuntimeError(f'Matching/statistic update changed old bodies: {family}/{case.name}/{path}')
                        allowed.add(path)
                        matching_statistics_additions += added
                analysis_input_additions = 0
                if args.allow_relation_analysis_input_change:
                    allowed.add('ASSIST.md')
                    template = (r'(?:template\s*<class\s+ChildRequest>\s*requires\s*requires\s*'
                                r'\(const\s+ChildRequest&\s+child\)\s*\{\s*child\.runtimeQuery\(\)\.selection\(\);\s*\}\s*)?')
                    method = r'(?:facetBy\w+As(?:WithOptions)?|groupBy\w+With(?:DetailsFrom|Details)?|count\w+With|statsFrom\w+)'
                    pattern = (template + r'(\w+Request)\s*(&&|&)\s*(' + method + r')\('
                               r'((?:std::string_view\s+(?:facet_name|alias),\s*)?const\s+'
                               r'(ChildRequest|SelectQuery|request::QuerySelection)&\s*child'
                               r'(?:,\s*bool\s+include_all_facets)?)\)\s*(&&|&)\s*\{([^{}]*)\}\s*')
                    for path in changed:
                        if not path.startswith('include/') or not path.endswith('/queries.hpp'):
                            continue
                        pairs = {}
                        def remove_analysis_input(match):
                            request, returns, name, parameters, kind, category = match.groups()[:6]
                            body = re.sub(r'\s+', '', match[7])
                            assert returns == category
                            arguments = (('facet_name,' if name.startswith('facetBy') else
                                          'alias,' if name.startswith('count') or name.startswith('statsFrom') and name.endswith('As') else '')
                                         + 'child' + (',include_all_facets' if name.endswith('WithOptions') else ''))
                            if category == '&&':
                                assert body == f'this->{name}({arguments});returnstd::move(*this);', body
                            else:
                                call = re.fullmatch(r'query_\.(facetBy|groupByObject|aggregateRelation)\((.*)\);return\*this;',body)
                                assert call, body
                                operands = call[2].split(',')
                                assert re.fullmatch(r'"[A-Za-z_][A-Za-z_0-9]*"',operands[0])
                                child = 'child.runtimeQuery()' if kind=='ChildRequest' else 'child'
                                if call[1]=='facetBy':
                                    assert name.startswith('facetBy')
                                    assert operands[1:]==(['facet_name',child,'include_all_facets'] if name.endswith('WithOptions') else ['facet_name',child])
                                elif call[1]=='groupByObject':
                                    assert name.startswith('groupBy') and re.fullmatch(r'"[A-Za-z_][A-Za-z_0-9]*"',operands[1])
                                    assert operands[2:]==[child]
                                else:
                                    assert operands[1:]==[('alias' if arguments.startswith('alias,') else '"refinements"'),child,'true' if name.startswith('count') else 'false']
                            if kind=='ChildRequest':
                                assert 'child.runtimeQuery().selection();' in match[0]
                            pairs.setdefault((request,name),[]).append((kind,category))
                            return ''
                        retained, added = re.subn(pattern, remove_analysis_input, (output/path).read_text())
                        expected=sorted((kind,category) for kind in ['ChildRequest','SelectQuery','request::QuerySelection'] for category in ['&','&&'])
                        assert pairs and all(sorted(entries)==expected for entries in pairs.values()), pairs
                        formatted = subprocess.run(
                            [str(ROOT/'.tools/clang-format/bin/clang-format'),
                             f'--style=file:{ROOT/".clang-format"}', '--Werror', '--assume-filename='+path],
                            input=retained, text=True, capture_output=True, check=True).stdout
                        if formatted != (destination/path).read_text():
                            raise RuntimeError(f'Analysis input update changed old bodies: {family}/{case.name}/{path}')
                        allowed.add(path)
                        analysis_input_additions += added
                relation_delegate_replacements = 0
                if args.allow_relation_selection_delegate_change:
                    allowed.add('ASSIST.md')
                    typed = (r'template\s*<bool\s+P,\s*Entity\s+ChildResult>\s*requires\s*\(!P\)\s*'
                             r'\w+Request\s*&\s*select\w+With\(const\s+\w+Request<P,\s*ChildResult>&\s*child\)'
                             r'\s*&\s*\{([^{}]*)\}')
                    for path in changed:
                        if not path.startswith('include/') or not path.endswith('/queries.hpp'):
                            continue
                        def restore_typed_selector(match):
                            body = re.sub(r'\s+', '', match[1])
                            selected = re.fullmatch(r'query_\.selectRelation\(("[A-Za-z_][A-Za-z_0-9]*"),'
                                                    r'child\.runtimeQuery\(\)\);return\*this;', body)
                            assert selected, body
                            relation = selected[1]
                            return (match[0][:match[0].index('{')] + '{ query_.requireCompatible(child.query_); '
                                    'query_.mutate_selection([&](SelectQuery& selection) { selection.relation_query('
                                    + relation + ', child.query()); }); return *this; }')
                        retained, replaced = re.subn(typed, restore_typed_selector, (output/path).read_text())
                        if not replaced:
                            raise RuntimeError(f'No typed selector delegates changed: {path}')
                        formatted = subprocess.run(
                            [str(ROOT/'.tools/clang-format/bin/clang-format'),
                             f'--style=file:{ROOT/".clang-format"}', '--Werror', '--assume-filename='+path],
                            input=retained, text=True, capture_output=True, check=True).stdout
                        if formatted != (destination/path).read_text():
                            raise RuntimeError(f'Typed selector delegation changed unrelated bodies: {path}')
                        allowed.add(path)
                        relation_delegate_replacements += replaced
                relation_input_additions = 0
                if args.allow_relation_selection_input_change:
                    allowed.add('ASSIST.md')
                    generic = (r'template\s*<class\s+ChildRequest>\s*requires\s*requires\s*'
                               r'\(const\s+ChildRequest&\s+child\)\s*\{\s*child\.runtimeQuery\(\)\.selection\(\);\s*\}\s*'
                               r'(\w+Request)\s*(&&|&)\s*(select\w+With)\(const\s+ChildRequest&\s+child\)\s*(&&|&)\s*\{([^{}]*)\}\s*')
                    direct = (r'(\w+Request)\s*(&&|&)\s*(select\w+With)\(const\s+'
                              r'(?:SelectQuery|request::QuerySelection)&\s*child\)\s*(&&|&)\s*\{([^{}]*)\}\s*')
                    for path in changed:
                        if not path.startswith('include/') or not path.endswith('/queries.hpp'):
                            continue
                        pairs = {}
                        def remove_relation_input(match, wrapped):
                            request, returns, method, category = match[1], match[2], match[3], match[4]
                            body = re.sub(r'\s+', '', match[5])
                            assert returns == category
                            if category == '&&':
                                assert body == f'this->{method}(child);returnstd::move(*this);', body
                            else:
                                operand = r'child\.runtimeQuery\(\)' if wrapped else 'child'
                                assert re.fullmatch(r'query_\.selectRelation\("[A-Za-z_][A-Za-z_0-9]*",'
                                                    + operand + r'\);return\*this;', body), body
                            key = (request, method)
                            pairs.setdefault(key, []).append((wrapped, category))
                            return ''
                        retained, generic_count = re.subn(generic, lambda m: remove_relation_input(m, True),
                                                         (output/path).read_text())
                        retained, direct_count = re.subn(direct, lambda m: remove_relation_input(m, False), retained)
                        for key, entries in pairs.items():
                            assert sorted(entries) == sorted([(True, '&'), (True, '&&'),
                                                             (False, '&'), (False, '&&'),
                                                             (False, '&'), (False, '&&')]), (key, entries)
                        if not pairs or direct_count != 2*generic_count:
                            raise RuntimeError(f'Incomplete selector input delegates: {path}')
                        formatted = subprocess.run(
                            [str(ROOT/'.tools/clang-format/bin/clang-format'),
                             f'--style=file:{ROOT/".clang-format"}', '--Werror', '--assume-filename='+path],
                            input=retained, text=True, capture_output=True, check=True).stdout
                        if formatted != (destination/path).read_text():
                            raise RuntimeError(f'Selector input update changed old bodies: {family}/{case.name}/{path}')
                        allowed.add(path)
                        relation_input_additions += generic_count + direct_count
                enhancement_additions = 0
                enhancement_replacements = 0
                if args.allow_enhancement_input_change:
                    allowed.add('ASSIST.md')
                    direct = (r'\w+Request\s*(&&|&)\s*enhanceChild\(const\s+'
                              r'(?:SelectQuery|request::QuerySelection)&\s*child\)\s*(&&|&)\s*\{([^{}]*)\}\s*')
                    wrapped = (r'template\s*<class\s+ChildRequest>\s*requires\s*requires\s*'
                               r'\(const\s+ChildRequest&\s+child\)\s*\{\s*child\.runtimeQuery\(\)\.selection\(\);\s*\}\s*'
                               r'\w+Request\s*(&&|&)\s*enhanceChild\(const\s+ChildRequest&\s+child\)\s*(&&|&)\s*\{([^{}]*)\}\s*')
                    for path in changed:
                        if not path.startswith('include/') or not path.endswith('/queries.hpp'):
                            continue
                        def remove_direct(match):
                            returns, category, body = match[1], match[2], re.sub(r'\s+', '', match[3])
                            assert returns == category
                            expected = ('this->enhanceChild(child);returnstd::move(*this);'
                                        if category == '&&' else 'query_.enhanceChild(child);return*this;')
                            assert body == expected, body
                            return ''
                        retained, added = re.subn(direct, remove_direct, (output/path).read_text())
                        def restore_wrapped(match):
                            returns, category, body = match[1], match[2], re.sub(r'\s+', '', match[3])
                            assert returns == category
                            expected = ('this->enhanceChild(child);returnstd::move(*this);'
                                        if category == '&&' else 'query_.enhanceChild(child.runtimeQuery());return*this;')
                            assert body == expected, body
                            return match[0].replace('child.runtimeQuery().selection();','child.query_.selection();').replace('child.runtimeQuery()','child.query_')
                        retained, replaced = re.subn(wrapped, restore_wrapped, retained)
                        if not added or added != 2*replaced:
                            raise RuntimeError(f'Incomplete enhancement overloads: {path}: {added}/{replaced}')
                        formatted = subprocess.run(
                            [str(ROOT/'.tools/clang-format/bin/clang-format'),
                             f'--style=file:{ROOT/".clang-format"}', '--Werror', '--assume-filename='+path],
                            input=retained, text=True, capture_output=True, check=True).stdout
                        if formatted != (destination/path).read_text():
                            raise RuntimeError(f'Enhancement update changed other bodies: {family}/{case.name}/{path}')
                        allowed.add(path)
                        enhancement_additions += added
                        enhancement_replacements += replaced
                type_match_additions = 0
                if args.allow_type_match_range_change:
                    allowed.add('ASSIST.md')
                    pattern = (r'template\s*<std::ranges::input_range\s+Inputs>\s*requires\s*'
                               r'std::constructible_from<Value,\s*Memory,\s*std::ranges::range_reference_t<Inputs>>\s*'
                               r'\w+Request\s*(&&|&)\s*matchTypes\(Inputs&&\s*inputs\)\s*(&&|&)\s*\{([^{}]*)\}\s*')
                    for path in changed:
                        if not path.startswith('include/') or not path.endswith('/queries.hpp'):
                            continue
                        def remove_type_match(match):
                            returns, category, body = match[1], match[2], re.sub(r'\s+', '', match[3])
                            assert returns == category
                            expected = ('this->matchTypes(std::forward<Inputs>(inputs));returnstd::move(*this);'
                                        if category == '&&' else
                                        'query_.matchTypes(std::forward<Inputs>(inputs));return*this;')
                            assert body == expected, body
                            return ''
                        retained, additions = re.subn(pattern, remove_type_match, (output/path).read_text())
                        if not additions or additions % 2:
                            raise RuntimeError(f'Incomplete type-match delegates: {path}: {additions}')
                        formatted = subprocess.run(
                            [str(ROOT/'.tools/clang-format/bin/clang-format'),
                             f'--style=file:{ROOT/".clang-format"}', '--Werror', '--assume-filename='+path],
                            input=retained, text=True, capture_output=True, check=True).stdout
                        if formatted != (destination/path).read_text():
                            raise RuntimeError(f'Type matching changed existing bodies: {family}/{case.name}/{path}')
                        allowed.add(path)
                        type_match_additions += additions
                if args.allow_membership_range_change:
                    allowed.add('ASSIST.md')
                    pattern = (r'template\s*<std::ranges::input_range\s+Inputs>\s*requires\s*'
                               r'std::constructible_from<Value,\s*Memory,\s*std::ranges::range_reference_t<Inputs>>\s*'
                               r'\w+Request\s*(&&|&)\s*(\w+(?:NotIn|In))\(Inputs&&\s*inputs\)\s*(&&|&)\s*\{([^{}]*)\}\s*')
                    for path in changed:
                        if not path.startswith('include/') or not path.endswith('/queries.hpp'):
                            continue
                        def remove_membership(match):
                            name, category, body = match[2], match[3], re.sub(r'\s+', '', match[4])
                            assert match[1] == category
                            if category == '&&':
                                assert body == f'this->{name}(std::forward<Inputs>(inputs));returnstd::move(*this);'
                            else:
                                op = 'NotIn' if name.endswith('NotIn') else 'In'
                                assert re.fullmatch(r'query_\.filterField\("(?:[^"\\]|\\.)*",request::FieldOperator::'
                                                    + op + r',std::forward<Inputs>\(inputs\)\);return\*this;', body), body
                            return ''
                        retained_body, additions = re.subn(pattern, remove_membership, (output / path).read_text())
                        if not additions or additions % 4:
                            raise RuntimeError(f'Incomplete constrained membership delegates: {path}: {additions}')
                        formatted = subprocess.run(
                            [str(ROOT / '.tools/clang-format/bin/clang-format'),
                             f'--style=file:{ROOT / ".clang-format"}', '--Werror',
                             '--assume-filename=' + path], input=retained_body, text=True,
                            capture_output=True, check=True).stdout
                        if formatted != (destination / path).read_text():
                            raise RuntimeError(f'Membership update changed existing bodies: {family}/{case.name}/{path}')
                        allowed.add(path)
                        membership_additions += additions
                core_query_additions = 0
                if args.allow_core_query_view_change:
                    allowed.add('ASSIST.md')
                    for path in changed:
                        if not path.startswith('include/') or not path.endswith('/queries.hpp'):
                            continue
                        body = (output / path).read_text()
                        pattern = r'SelectQuery\s+coreQuery\(\)\s*const\s*requires\s*\(!Purposed\)\s*\{\s*return\s+query_\.coreQuery\(\);\s*\}\s*'
                        retained_body, additions = re.subn(pattern, '', body)
                        if not additions:
                            raise RuntimeError(f'Missing exact coreQuery delegates: {path}')
                        formatted = subprocess.run(
                            [str(ROOT / '.tools/clang-format/bin/clang-format'),
                             f'--style=file:{ROOT / ".clang-format"}', '--Werror',
                             '--assume-filename=' + path], input=retained_body, text=True,
                            capture_output=True, check=True).stdout
                        if formatted != (destination / path).read_text():
                            raise RuntimeError(f'Core view update changed existing bodies: {family}/{case.name}/{path}')
                        allowed.add(path)
                        core_query_additions += additions
                if args.allow_request_state_change:
                    allowed.add('ASSIST.md')
                    for path in changed:
                        if not path.startswith('include/') or not path.endswith('/queries.hpp'):
                            continue
                        retained_body, additions = without_request_state_adapters((output / path).read_text())
                        formatted = subprocess.run(
                            [str(ROOT / '.tools/clang-format/bin/clang-format'),
                             f'--style=file:{ROOT / ".clang-format"}', '--Werror',
                             '--assume-filename=' + path], input=retained_body, text=True,
                            capture_output=True, check=True).stdout
                        if formatted != (destination / path).read_text():
                            raise RuntimeError(f'Request state update changed an existing body: {family}/{case.name}/{path}')
                        allowed.add(path)
                        request_state_additions += additions
                if args.allow_support_change:
                    allowed.update(p for p in files if p.startswith('include/') and p.endswith('/support.hpp'))
                if args.allow_wire_mapping_change:
                    allowed.add('ASSIST.md')
                    for path in changed:
                        if not path.startswith('include/'):
                            continue
                        if path.endswith('/runtime.hpp'):
                            previous = (destination / path).read_text()
                            retained_body = without_wire_functions((output / path).read_text())
                        elif path.endswith('/support.hpp'):
                            previous = (destination / path).read_text()
                            retained_body = (output / path).read_text().replace(
                                '#include <teaql/runtime/generated_wire_fields.hpp>\n', '')
                        else:
                            continue
                        formatted = subprocess.run(
                            [str(ROOT / '.tools/clang-format/bin/clang-format'),
                             f'--style=file:{ROOT / ".clang-format"}', '--Werror',
                             '--assume-filename=' + path], input=retained_body, text=True,
                            capture_output=True, check=True).stdout
                        if formatted != previous:
                            raise RuntimeError(f'Wire metadata update changed an existing body: {family}/{case.name}/{path}')
                        allowed.add(path)
                constructor_changes = 0
                if args.allow_timestamp_default_change:
                    for path in changed:
                        if not path.startswith('include/') or not path.endswith('/models.hpp'):
                            continue
                        expected, replacements = re.subn(
                            r'([A-Za-z_][A-Za-z_0-9]*_)\(generated_detail::default_value<Timestamp>\(a\.resource\(\)\)\)',
                            r'\1(Timestamp::now())', (destination / path).read_text())
                        formatted = subprocess.run(
                            [str(ROOT / '.tools/clang-format/bin/clang-format'),
                             f'--style=file:{ROOT / ".clang-format"}', '--Werror',
                             '--assume-filename=' + path], input=expected, text=True,
                            capture_output=True, check=True).stdout
                        if not replacements or formatted.encode() != (output / path).read_bytes():
                            raise RuntimeError(f'Non-initializer model change: {family}/{case.name}/{path}')
                        allowed.add(path)
                        constructor_changes += replacements
                if changed and (not set(changed) <= allowed or files.keys() != provenance['files'].keys()):
                    raise RuntimeError(f'Instruction-only reproduction changed SDK bytes: '
                                       f'{family}/{case.name}: {changed}')
                for path in changed:
                    staged.append((destination / path, (output / path).read_bytes()))
                provenance['files'] = files
                for path in provenance['generator_implementation_sha256']:
                    provenance['generator_implementation_sha256'][path] = digest(PRODUCER / path)
                # Audit limits are interpreted through these shared helpers even
                # when a fixture omits the attribute. Retain their actual producer
                # identity without permitting changes to generated SDK bodies.
                for path in METADATA_HELPER_INPUTS:
                    provenance['generator_implementation_sha256'][path] = digest(PRODUCER / path)
                generated = json.dumps(provenance, indent=2) + '\n'
                if args.check and (destination / 'PROVENANCE.json').read_text() != generated:
                    raise RuntimeError(f'Stale producer provenance: {family}/{case.name}')
                staged.append((destination / 'PROVENANCE.json', generated.encode()))
                records.append(dict(family=family, case=case.name, files=len(files),
                                    sample_operand_delegates=sample_operand_delegates,
                                    source_alias_identity_edits=alias_changes,
                                    expression_entry_metadata_additions=expression_entry_changes,
                                    result_marker_changed_files=result_marker_changes,
                                    business_id_lookup_edits=business_id_lookup_changes,
                                    scalar_diagnostic_edits=scalar_diagnostic_changes,
                                    relation_identity_edits=relation_identity_changes,
                                    ledger_composition_edits=ledger_composition_changes,
                                    regenerated_files_identical=not changed,
                                    regenerated_code_identical=all(p == 'ASSIST.md' for p in changed),
                                    changed_instruction_files=[p for p in changed if p == 'ASSIST.md'],
                                    changed_generated_files=changed,
                                    request_state_adapter_additions=request_state_additions,
                                    core_query_adapter_additions=core_query_additions,
                                    membership_range_adapter_additions=membership_additions,
                                    type_match_range_adapter_additions=type_match_additions,
                                    matching_statistics_input_adapter_additions=matching_statistics_additions,
                                    relation_analysis_input_adapter_additions=analysis_input_additions,
                                    relation_selection_input_adapter_additions=relation_input_additions,
                                    typed_selector_delegate_replacements=relation_delegate_replacements,
                                    enhancement_input_adapter_additions=enhancement_additions,
                                    enhancement_public_access_replacements=enhancement_replacements,
                                    scalar_comparison_adapter_additions=scalar_comparison_additions,
                                    sound_value_adapter_additions=sound_value_additions,
                                    query_branch_adapter_additions=query_branch_additions,
                                    runtime_configuration_adapter_additions=runtime_configuration_additions,
                                    runtime_binding_adapter_replacements=runtime_binding_replacements,
                                    registry_schema_adapter_additions=registry_schema_additions,
                                    business_allocator_alias_additions=business_allocator_alias_additions,
                                    timestamp_initializer_replacements=constructor_changes))
        if not args.check:
            for path, generated in staged:
                path.write_bytes(generated)
        print(f'Reproduced {len(records)} complete C++ contract SDKs; '
              f'{sum(not row["regenerated_code_identical"] for row in records)} generated code updates; '
              f'{sum(bool(row["changed_instruction_files"]) for row in records)} generated Assist updates')
        if args.report:
            args.report.write_text(
                json.dumps(dict(schema=1, cases=records, rust_artifacts_changed=False,
                                historical_cpp_artifacts_changed=False), indent=2) + '\n')


if __name__ == '__main__':
    main()
