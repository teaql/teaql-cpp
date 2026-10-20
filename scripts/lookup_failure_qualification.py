"""Bound the generated lookup-result repair to its exact shared runtime bodies."""
import gzip
import hashlib
import json
from pathlib import Path
from relation_identity_qualification import format_cpp

RUNTIME = 'teaql_runtime/include/teaql/runtime/entity_query.hpp'
RESULTS = 'teaql_runtime/include/teaql/runtime/detail/query_results.hpp'
RETURNS = {
    'executeForList': 'SmartList<R>', 'executeForOne': 'std::optional<R>',
    'executeForCount': 'std::uint64_t', 'executeForExists': 'bool',
    'executeForPage': 'SmartList<R>', 'executeForRows': 'SmartList<CompactRow>',
    'executeForStream': 'TypedEntityQueryStream<R, typename E::Error>',
}
ADDITION = '// Repository lookup is part of generated execution. Preserve its Graph result\n// boundary instead of throwing before callers receive their operation handle.\ntemplate <class T, class Error>\nclass GeneratedQueryLookupFailure {\npublic:\n    using allocator_type = Allocator;\n    using Output = Result<T, DataServiceError<Error>>;\n\n    GeneratedQueryLookupFailure(std::allocator_arg_t, Allocator a, const ContextError& error)\n        : result_(Output::err(a.resource(),\n                              DataServiceError<Error>::runtime(\n                                  a.resource(),\n                                  RuntimeError::graph(a.resource(), error.to_string(a.resource()))))) {}\n\n    std::optional<Output> poll(WakeSignal&) {\n        return std::move(result_);\n    }\n\n    void finish_and_wait() noexcept {}\n\n    void cancel_and_wait() noexcept {}\n\nprivate:\n    Output result_;\n};\n\ntemplate <class T, class Error>\nauto& generated_query_lookup_failure(Memory r, const ContextError& error) {\n    using State = GeneratedQueryLookupFailure<T, Error>;\n    return make_operation<typename State::Output, State>(r, error);\n}\n\n'


def upgrade_entity_query(root, previous):
    old = ('        if (!service) {\n'
           '            throw std::runtime_error(\n'
           '                std::string(service.error().to_string(user_context.memory_resource())));\n'
           '        }')
    assert previous.count(old) == 7
    for name, value in RETURNS.items():
        start = previous.index('    auto& ' + name + '(')
        end = previous.index('\n    }', start)
        body = previous[start:end]
        assert body.count(old) == 1, name
        replacement = ('        if (!service) {\n'
                       '            return detail::generated_query_lookup_failure<' + value + ', typename E::Error>(\n'
                       '                user_context.memory_resource(), service.error());\n'
                       '        }')
        previous = previous[:start] + body.replace(old, replacement) + previous[end:]
    return format_cpp(root, RUNTIME, previous)


def verify(root):
    checkpoint = root / 'docs/parity/entry_lookup_before_5_0_6.json.gz'
    before = json.loads(gzip.decompress(checkpoint.read_bytes()))
    for name in (RUNTIME, RESULTS):
        assert hashlib.sha256(before[name]['text'].encode()).hexdigest() == before[name]['sha256']
    expected = upgrade_entity_query(root, before[RUNTIME]['text'])
    assert (root / RUNTIME).read_text() == expected, 'Unreviewed execution-header change'
    prefix = 'namespace teaql::detail {\n'
    previous = before[RESULTS]['text']
    assert previous.count(prefix) == 1
    expected_results = format_cpp(root, RESULTS, previous.replace(prefix, prefix + ADDITION))
    assert (root / RESULTS).read_text() == expected_results, 'Unreviewed result-adapter change'
    return before[RUNTIME]['text']
