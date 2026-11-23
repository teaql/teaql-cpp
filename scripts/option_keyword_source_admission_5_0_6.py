#!/usr/bin/env python3
"""Admit the user's reserved-name policy without rewriting prior compiler evidence."""

import argparse
import ast
import copy
from functools import cache
import gzip
import hashlib
import json
from pathlib import Path
import re

ROOT = Path(__file__).resolve().parents[1]
PRODUCER = ROOT.parent / "teaql_code_gen_cpp"
RECORD = ROOT / "docs/parity/option_keyword_sources_5_0_6.json.gz"
RECORD_SHA = "14d13ef85b02670b20083b834f43a3ef5ef0e790a087f9a9d64708400bab8b69"
EXECUTION = ROOT / "docs/parity/option_keyword_java_5_0_6.json.gz"
EXECUTION_SHA = "4f90469f4eb1698440bba3732dfe63cbcb4497b76e2b536db8e44e397da055bf"
SELF = "runtime:scripts/option_keyword_source_admission_5_0_6.py"
PREVIOUS = "runtime:scripts/generation_decision_source_admission_5_0_6.py"
READER = "runtime:scripts/verify_generation_decisions_5_0_6.py"
OFFLINE = "runtime:scripts/compiler_offline_source_admission_5_0_6.py"
DRIVER = "runtime:scripts/verify_option_keyword_policy_5_0_6.py"
MANIFEST = "runtime:docs/compatibility_exceptions_5_0_6.json"
GOVERNANCE = "runtime:docs/governance_qualification_updates_5_0_6.json"
POLICY = "runtime:docs/option_keyword_policy_5_0_6.json"
PRODUCER_DOC = "producer:docs/cpp_compiler_host.md"


def require(condition, message):
    if not condition:
        raise ValueError("Option keyword source admission: " + message)


def sha(data):
    return hashlib.sha256(data).hexdigest()


def read(path):
    return json.loads(gzip.decompress(Path(path).read_bytes()))


def source_path(name):
    scope, relative = name.split(":", 1)
    require(scope in ("runtime", "producer") and relative and not Path(relative).is_absolute()
            and ".." not in Path(relative).parts, "canonical source identity")
    return (ROOT if scope == "runtime" else PRODUCER) / relative


def function_changes(before, after, expected):
    trees = [ast.parse(value) for value in (before, after)]
    functions = [{node.name: ast.dump(node) for node in tree.body if isinstance(node, ast.FunctionDef)}
                 for tree in trees]
    require(functions[0].keys() == functions[1].keys()
            and {key for key in functions[0] if functions[0][key] != functions[1][key]} == set(expected),
            "exact metadata/current-source admission functions")
    for tree in trees:
        for node in tree.body:
            if isinstance(node, ast.FunctionDef) and node.name in expected:
                node.body = [ast.Pass()]
    require(ast.dump(trees[0]) == ast.dump(trees[1]), "all prior execution/comparison/control bodies remain exact")


def expected_policy():
    return dict(schema=1, runtime_commit="fe82a18982415f1746df9a2cf7c42f5e9abdc6d3",
        generator_commit="45e1572c4956ca963410d8ea436ead3c8857087a", decision="reserve_option_model_identifiers",
        decided_on="2026-11-20", keywords=["option", "Option"], applies_to=["entity_name", "field_name"],
        entity_rule_id="KSML-KEYWORD-001", field_rule_id="KSML-KEYWORD-002",
        rule_source="generator/src/main/java/com/skynet/codegenerator/evaluation/ReservedKeywordRule.java",
        replacement_examples=["option_record", "payment_option", "option_code"], preserves_optional_types=True,
        failure_stage="model_validation", compiler_error_adapter_required=False,
        supersedes="docs/compatibility_exceptions_5_0_6.json#option_model",
        documentation="docs/option_reserved_keyword_policy_5_0_6.md",
        execution_receipt="docs/parity/option_keyword_java_5_0_6.json.gz", execution_sha256=EXECUTION_SHA,
        whole_goal_complete=False)


def validate_dispositions(rows):
    before, after = [json.loads(rows[MANIFEST][key]) for key in ("before_source", "after_source")]
    expected = copy.deepcopy(before)
    require(expected["option_model"]["status"] == "approved", "preserved prior explicit approval")
    expected["option_model"].update(status="superseded", superseded_on="2026-11-20",
        superseded_by="docs/option_keyword_policy_5_0_6.json",
        resolution="Reserve option and Option as model identifiers through the shared keyword rule; reject before compiler invocation.")
    require(expected == after, "only Option's current disposition changes; other exceptions/evidence stay exact")
    before, after = [json.loads(rows[GOVERNANCE][key]) for key in ("before_source", "after_source")]
    expected = copy.deepcopy(before)
    prior = expected["approved_compatibility_decisions"].pop("option_model")
    require(prior == dict(approved_on="2026-11-20", contract="docs/option_model_compatibility_exception_5_0_6.md"),
            "exact superseded governance approval")
    expected["model_naming_decisions"] = {"option_model": dict(decided_on="2026-11-20",
        status="reserved_keyword", contract="docs/option_reserved_keyword_policy_5_0_6.md")}
    require(expected == after, "GP11/GP16 and every other governance disposition stay exact")
    policy = json.loads(rows[POLICY]["after_source"])
    require(policy == expected_policy() and type(policy["schema"]) is int
            and policy["preserves_optional_types"] is True and policy["compiler_error_adapter_required"] is False
            and policy["whole_goal_complete"] is False, "exact user-directed model keyword policy")


def validate(record):
    import verify_option_keyword_policy_5_0_6 as gate
    from compiled_column_source_admission_5_0_6 import current_input_matches as column_matches

    require(sha(EXECUTION.read_bytes()) == EXECUTION_SHA, "immutable actual 393-test Java execution")
    execution = read(EXECUTION)
    gate.validate(execution)
    expected = {PREVIOUS, READER, OFFLINE, DRIVER, MANIFEST, GOVERNANCE, POLICY, PRODUCER_DOC,
                "producer:" + gate.MODEL, *("producer:" + name for name in execution["source_images"])}
    require(record.get("complete") is True and record.get("whole_goal_complete") is False
            and set(record["paths"]) == expected and record["execution_sha256"] == EXECUTION_SHA,
            "exact central keyword/factory contracts and scoped policy/source continuations")
    rows = record["paths"]
    new = {DRIVER, POLICY, "producer:" + gate.MODEL}
    for name, row in rows.items():
        require(row["after_sha256"] == sha(row["after_source"].encode())
                and (row["after_sha256"] == sha(source_path(name).read_bytes())
                     or column_matches(source_path(name), row["after_sha256"])), "exact current source image: " + name)
        if name in new:
            require(row["before_source"] is None and row["before_sha256"] is None, "genuinely new policy/fixture/driver")
        else:
            require(row["before_sha256"] == sha(row["before_source"].encode()), "full source preimage")
        if name in {"producer:" + key for key in execution["source_images"]}:
            require(row == execution["source_images"][name.split(":", 1)[1]], "actual freshly compiled Java transition")
        elif name in {PREVIOUS, READER, OFFLINE, MANIFEST, GOVERNANCE, PRODUCER_DOC}:
            require(row["before_sha256"] == execution["inputs_after"][name], "actual preceding Java execution frame")
        elif name in {DRIVER, "producer:" + gate.MODEL}:
            require(row["after_sha256"] == execution["inputs_after"][name], "actual executed model/driver")
    row = rows["producer:" + gate.RULE]
    old = '"macro", "union"\n'
    replacement = ('"macro", "union",\n'
                   "            // Generated entities must not shadow the Rust prelude's Option<T>.\n"
                   '            "option", "Option"\n')
    require(row["before_source"].count(old) == 1 and row["before_source"].replace(old, replacement, 1) == row["after_source"],
            "only two names are added to the existing shared Rust keyword list")
    row = rows["producer:" + gate.FACTORY]
    imports = ("import com.skynet.codegenerator.evaluation.KsmlEvaluationGate;\n"
               "import com.skynet.codegenerator.evaluation.KsmlEvaluationContext;\n"
               "import com.skynet.codegenerator.evaluation.KsmlEvaluationResponse;\n"
               "import com.skynet.codegenerator.evaluation.KsmlValidationException;\n"
               "import com.skynet.codegenerator.evaluation.ReservedKeywordRule;\n")
    wrapped = ('    return new DomainGenerationService(Map.of(CppCoreGenerator.SCOPE, source -> {\n'
               '      var model = new KsmlEvaluationContext(source);\n'
               '      var evaluation = new KsmlEvaluationResponse();\n'
               '      new ReservedKeywordRule().evaluate(model, evaluation);\n'
               '      evaluation.getErrors().forEach(model::enrichSourceLocation);\n'
               '      if (KsmlEvaluationGate.blocks(evaluation)) {\n'
               '        throw new KsmlValidationException(evaluation);\n'
               '      }\n'
               '      return compiler.generate(source);\n'
               '    }));\n')
    require(row["after_source"].count(imports) == row["after_source"].count(wrapped) == 1
            and row["after_source"].replace(imports, "", 1).replace(wrapped,
                "    return new DomainGenerationService(Map.of(CppCoreGenerator.SCOPE, compiler));\n", 1) == row["before_source"],
            "factory only delegates shared keyword validation before the unchanged compiler adapter")
    function_changes(rows[READER]["before_source"], rows[READER]["after_source"], {"validate_exceptions", "validate_java", "main"})
    function_changes(rows[PREVIOUS]["before_source"], rows[PREVIOUS]["after_source"], {"validate"})
    function_changes(rows[OFFLINE]["before_source"], rows[OFFLINE]["after_source"], {"source_epochs", "added_input_hashes"})
    validate_dispositions(rows)
    binding = 'RECORD_SHA = "' + RECORD_SHA + '"\n'
    current = Path(__file__).read_text()
    if current.replace(binding, 'RECORD_SHA = "__RECORD_SHA256__"\n', 1) != record["helper_after_source_normalized"]:
        from compiled_column_source_admission_5_0_6 import preceding_source
        current = preceding_source(SELF, current)
    require(current.count(binding) == 1 and current.replace(binding, 'RECORD_SHA = "__RECORD_SHA256__"\n', 1)
            == record["helper_after_source_normalized"], "complete source helper image; only its own immutable receipt hash is normalized")


@cache
def sources():
    require(sha(RECORD.read_bytes()) == RECORD_SHA, "immutable exact keyword policy continuation")
    record = read(RECORD)
    validate(record)
    return record["paths"]


def source_rows():
    return {**sources(), SELF: dict(before_source=None, before_sha256=None,
        after_source=Path(__file__).read_text(), after_sha256=sha(Path(__file__).read_bytes()))}


def added_input_hashes():
    return {name: row["after_sha256"] for name, row in source_rows().items() if row["before_source"] is None}


def current_input_matches(path, expected):
    from compiled_column_source_admission_5_0_6 import current_input_matches as column_matches

    path = Path(path).resolve()
    if type(expected) is not str or re.fullmatch(r"[0-9a-f]{64}", expected) is None or not path.is_file():
        return False
    scope = "runtime:" if path.is_relative_to(ROOT) else "producer:" if path.is_relative_to(PRODUCER) else None
    if scope is None:
        return False
    base = ROOT if scope == "runtime:" else PRODUCER
    row = source_rows().get(scope + path.relative_to(base).as_posix())
    return (row is not None and expected in (row["before_sha256"], row["after_sha256"])) or column_matches(path, expected)


def preceding_source(name, current):
    row = sources().get(name)
    if row is not None and current != row["after_source"]:
        from compiled_column_source_admission_5_0_6 import preceding_source as column_preimage
        current = column_preimage(name, current)
    require(name == PREVIOUS and row is not None and current == row["after_source"], "one exact prior source-admission image")
    return row["before_source"]


def validate_current_disposition(contract):
    require(contract == json.loads(sources()[MANIFEST]["after_source"]), "current reserved-name policy and unchanged other exceptions")


def historical_exception_contract(contract):
    validate_current_disposition(contract)
    value = copy.deepcopy(contract)
    value["option_model"] = json.loads(sources()[MANIFEST]["before_source"])["option_model"]
    return value


def controls(record):
    changes = [
        ("numeric_complete", lambda x: x.update(complete=1)),
        ("goal_promoted", lambda x: x.update(whole_goal_complete=True)),
        ("missing_source", lambda x: x["paths"].pop(READER)),
        ("wrong_execution", lambda x: x.update(execution_sha256="0" * 64)),
        ("changed_factory", lambda x: x["paths"]["producer:generator/src/main/java/com/skynet/codegenerator/cpp/CppCompilerServiceConfiguration.java"].update(after_source="return compiler;")),
        ("changed_preimage", lambda x: x["paths"][PREVIOUS].update(before_sha256="0" * 64)),
        ("changed_helper", lambda x: x.update(helper_after_source_normalized="pass\n")),
        ("changed_policy", lambda x: x["paths"][POLICY].update(after_source="{}")),
        ("revived_exception", lambda x: x["paths"][MANIFEST].update(after_source=x["paths"][MANIFEST]["before_source"])),
        ("governance_promoted", lambda x: x["paths"][GOVERNANCE].update(after_source="{}")),
    ]
    results = []
    for name, change in changes:
        altered = copy.deepcopy(record)
        change(altered)
        try:
            validate(altered)
        except (ValueError, KeyError, TypeError, IndexError) as error:
            results.append(dict(name=name, rejected=True, reason=str(error)))
        else:
            raise ValueError("Invalid policy/source continuation admitted: " + name)
    for name, change in (
        ("current_approval_revived", lambda x: x["option_model"].update(status="approved")),
        ("current_scope_widened", lambda x: x["option_model"]["case_names"].append("all_types")),
    ):
        altered = json.loads(record["paths"][MANIFEST]["after_source"])
        change(altered)
        try:
            validate_current_disposition(altered)
        except (ValueError, KeyError, TypeError) as error:
            results.append(dict(name=name, rejected=True, reason=str(error)))
        else:
            raise ValueError("Invalid current policy admitted: " + name)
    return results


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--record", type=Path)
    args = parser.parse_args()
    if args.record and args.record.exists():
        parser.error("Preserve prior source qualification records")
    sources()
    results = controls(read(RECORD))
    if args.record:
        args.record.write_bytes(gzip.compress(json.dumps(dict(complete=True, whole_goal_complete=False,
            source_receipt_sha256=RECORD_SHA, execution_sha256=EXECUTION_SHA, results=results), sort_keys=True).encode(), mtime=0))
    print("Shared Option keyword policy and exact source continuations qualified; 12 corruption controls rejected")


if __name__ == "__main__":
    main()
