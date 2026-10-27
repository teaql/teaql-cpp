#!/usr/bin/env python3
"""Guard the proposed quoted-column compatibility boundary with live-source receipts.

The locked Rust generator compiles the two models and derives additional
attributes from their column text. C++ deliberately refuses those inputs. This
verifier checks the observed difference and an ordinary generated model; it does
not approve the exception or establish complete G14 parity.
"""

import argparse
import copy
import gzip
import hashlib
import json
from pathlib import Path
import subprocess
import sys

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / "scripts"))
from compare_values import same
import verify_property_shape_metadata_5_0_6 as property_observer

DIAGNOSTIC = ROOT / "docs/parity/field_attribute_current_property_exception_5_0_6.json.gz"
VALID = ROOT / "docs/parity/field_attribute_current_valid_control_5_0_6.json.gz"
MODELS = ROOT / "tests/parity/field_attribute_models_5_0_6.json.gz"
VALID_MODELS = ROOT / "tests/parity/property_shape_models_5_0_6.json.gz"
REFUSAL = "java.lang.IllegalArgumentException: Unescaped quote in original Rust column literal"


def read(path):
    data = path.read_bytes()
    return json.loads(gzip.decompress(data) if path.suffix == ".gz" else data)


def digest(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def require(condition, description):
    if not condition:
        raise ValueError(description)


def property_for(entity, name):
    values = [value for value in entity["properties"] if value["name"] == name]
    require(len(values) == 1, "exact property identity: " + name)
    return values[0]


def validate_common(receipt, corpus, corpus_path, selected_names):
    require(receipt.get("complete") is True, "complete receipt")
    require(receipt.get("owned_directory_removed") is True, "owned cleanup")
    require(type(receipt.get("owned_bytes_removed")) is int
            and receipt["owned_bytes_removed"] > 0, "measured disposable output")
    require(receipt.get("inputs_unchanged") is True, "stable input epoch")
    require(receipt.get("input_hashes_before") == receipt.get("input_hashes_after"),
            "identical before/after input hashes")
    require(receipt.get("runtime_commit") == corpus["runtime_commit"]
            == read(ROOT / "upstream.lock.json")["commit"], "locked runtime")
    reference = subprocess.check_output(
        ["git", "-C", str(ROOT / ".reference/teaql_code_gen"), "rev-parse", "HEAD"],
        text=True).strip()
    require(receipt.get("generator_commit") == corpus["generator_commit"] == reference,
            "locked generator")

    hashes = receipt["input_hashes_before"]
    require(str(corpus_path) in hashes, "selected complete model corpus")
    require(str(ROOT / "scripts/verify_audit_metadata_compiler_5_0_6.py") in hashes,
            "compiler verifier source")
    require(str(ROOT.parent / "teaql_code_gen_cpp/generator/src/main/java"
                "/com/skynet/codegenerator/cpp/CppJsonInputMetadata.java") in hashes,
            "current C++ refusal source")
    for path, wanted in hashes.items():
        source = Path(path)
        require(source.is_file() and digest(source) == wanted, "current input identity: " + path)

    wrapper = receipt["observer_wrapper"]
    wrapper_path = ROOT / "scripts/verify_property_shape_metadata_5_0_6.py"
    require(wrapper["source"] == wrapper_path.read_text()
            and wrapper["sha256"] == digest(wrapper_path), "current property observer")
    require(wrapper["compiler_sha256"] == digest(
        ROOT / "scripts/verify_audit_metadata_compiler_5_0_6.py"), "current compiler controller")
    require(wrapper["rust_main"] == property_observer.RUST_MAIN
            and wrapper["cpp_main"] == property_observer.CPP_MAIN,
            "property descriptor observer bodies")

    sources = [next((case for case in corpus["cases"] if case["name"] == name), None)
               for name in selected_names]
    require(all(source is not None for source in sources), "selected corpus names")
    cases = receipt["cases"]
    require(type(cases) is list and len(cases) == len(selected_names), "complete selected case set")
    require([case["name"] for case in cases] == selected_names, "ordered selected case names")
    for index, (case, source) in enumerate(zip(cases, sources)):
        require(source["expected"] == {"parsed": True}, "original accepted model")
        require(case["input"] == source["input"] and case["model"]
                == source["model"].replace("{namespace}", f"cpp_audit_case_{index}"),
                "unchanged complete model")
        require([row["entity"] for row in case["original"]]
                == ["Platform", "Item", "Child"], "executed original entity set")
        for name, file in case["files"].items():
            require(hashlib.sha256(file["text"].encode()).hexdigest() == file["sha256"],
                    "retained generated file: " + name)
        for entity in ("platform", "item", "child"):
            require(f"original/lib/src/{entity}/entity.rs" in case["files"],
                    "complete original generated entity: " + entity)
        require("harness/main.rs" in case["files"], "executed observer source")

    labels = [step["label"] for step in receipt["steps"]]
    require(len(labels) == len(set(labels)), "unique execution steps")
    require(all(type(step["exit_code"]) is int for step in receipt["steps"]),
            "integer process exit codes")
    return {step["label"]: step for step in receipt["steps"]}


def validate_exception(receipt, corpus):
    names = ["column_attribute_alias", "column_attribute_shape"]
    steps = validate_common(receipt, corpus, MODELS, names)
    require(receipt.get("diagnostic_cpp_refusal") is True
            and receipt.get("semantic_accepted") is False,
            "explicit non-parity diagnostic status")
    require(receipt.get("expected_cpp_refusal") == REFUSAL.split(": ", 1)[1],
            "exact C++ refusal contract")
    require([case["name"] for case in corpus["cases"]] == names,
            "two source-confirmed injection shapes")
    common = {"upstream", "original_classpath", "original_java_compile",
              "cpp_classpath", "cpp_java_compile"}
    expected_steps = common | {
        name + suffix for name in names for suffix in
        ("_original_generate", "_cpp_generate", "_original_build", "_original_run")}
    require(set(steps) == expected_steps, "complete original/refusal commands")
    require(all(step["exit_code"] == 0 for label, step in steps.items()
                if not label.endswith("_cpp_generate")), "successful original execution")
    for index, case in enumerate(receipt["cases"]):
        name = names[index]
        refusal = case["cpp_refusal"]
        require(type(refusal["exit_code"]) is int and refusal["exit_code"] != 0,
                "C++ generation refusal: " + name)
        require(refusal["exit_code"] == steps[name + "_cpp_generate"]["exit_code"]
                and refusal["stderr"] == steps[name + "_cpp_generate"]["stderr"]
                and REFUSAL in refusal["stderr"], "actual exact refusal: " + name)
        expected_length = 100 if index == 0 else 7
        for entity in case["original"][1:]:
            value = property_for(entity, "name")
            require(value["column"] == "chosen"
                    and type(value["max_length"]) is int
                    and value["max_length"] == expected_length,
                    "original derived column and length: " + name + "/" + entity["entity"])
        secret = property_for(case["original"][2], "secret")
        require(secret["column"] == "secret" and secret["max_length"] == 100,
                "unaffected inherited field")


def validate_valid(receipt, corpus):
    steps = validate_common(receipt, corpus, VALID_MODELS, ["property_shape_lengths"])
    require(receipt.get("diagnostic_cpp_refusal") is None
            and receipt.get("semantic_accepted") is None,
            "ordinary generated-model control")
    require("property_shape_lengths" in [case["name"] for case in corpus["cases"]],
            "selected valid property shape")
    required = {"upstream", "original_classpath", "original_java_compile",
                "cpp_classpath", "cpp_java_compile", "property_shape_lengths_original_generate",
                "property_shape_lengths_cpp_generate", "property_shape_lengths_cpp_format",
                "property_shape_lengths_original_build", "property_shape_lengths_original_run",
                "observer_format", "native_build", "native_run"}
    require(set(steps) == required and all(row["exit_code"] == 0 for row in steps.values()),
            "successful original and C++ build/execution")
    require(receipt.get("differences") == [] and receipt["cases"][0]["match"] is True
            and same(receipt["cases"][0]["original"], receipt["cases"][0]["cpp"]),
            "exact ordinary property descriptors")


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--record", type=Path)
    parser.add_argument("--diagnostic", type=Path, default=DIAGNOSTIC,
                        help="Original/refusal receipt; defaults to the current source epoch")
    parser.add_argument("--valid", type=Path, default=VALID,
                        help="Current ordinary generated-model control receipt")
    args = parser.parse_args()
    if args.record and args.record.exists():
        parser.error("Preserve previous control receipts")
    exception, valid = read(args.diagnostic), read(args.valid)
    exception_models, valid_models = read(MODELS), read(VALID_MODELS)
    validate_exception(exception, exception_models)
    validate_valid(valid, valid_models)
    controls = [
        ("numeric_complete", 0, lambda x: x.update(complete=1)),
        ("accepted_exception", 0, lambda x: x.update(semantic_accepted=True)),
        ("missing_case", 0, lambda x: x["cases"].pop()),
        ("wrong_original_column", 0, lambda x: property_for(x["cases"][0]["original"][1], "name").update(column="first")),
        ("wrong_original_length", 0, lambda x: property_for(x["cases"][1]["original"][1], "name").update(max_length=7.0)),
        ("wrong_child_length", 0, lambda x: property_for(x["cases"][1]["original"][2], "name").update(max_length=100)),
        ("wrong_refusal", 0, lambda x: x["cases"][0]["cpp_refusal"].update(stderr="unrelated error")),
        ("successful_cpp_injection", 0, lambda x: x["cases"][0]["cpp_refusal"].update(exit_code=0)),
        ("stale_source", 0, lambda x: x["input_hashes_before"].update({next(iter(x["input_hashes_before"])): "0" * 64})),
        ("changed_generated_source", 0, lambda x: x["cases"][0]["files"]["harness/main.rs"].update(text="corruption")),
        ("lost_cleanup", 0, lambda x: x.update(owned_directory_removed=False)),
        ("valid_difference", 1, lambda x: x.update(differences=["invented"])),
        ("valid_observer_mismatch", 1, lambda x: x["cases"][0].update(match=1)),
    ]
    outcomes = []
    for name, side, mutation in controls:
        changed = copy.deepcopy(exception if side == 0 else valid)
        mutation(changed)
        try:
            (validate_exception(changed, exception_models) if side == 0
             else validate_valid(changed, valid_models))
        except (ValueError, KeyError, TypeError, IndexError) as error:
            outcomes.append({"name": name, "rejected": True, "reason": str(error)})
        else:
            raise AssertionError("Compatibility boundary admitted " + name)
    if args.record:
        record = {
            "complete": True, "approved": False, "whole_goal_complete": False,
            "scope": "Current original descriptor/refusal execution and valid C++ control, plus in-memory verifier corruptions; no new runtime execution or compatibility approval.",
            "inputs": {str(path.resolve().relative_to(ROOT)): digest(path) for path in
                       (args.diagnostic, args.valid, MODELS, VALID_MODELS, Path(__file__))},
            "results": outcomes,
        }
        args.record.write_bytes(gzip.compress((json.dumps(record, indent=2) + "\n").encode(), mtime=0))
    print(f"Field attribute candidate exception: 2 original/refusal cases, "
          f"1 valid generated control, {len(outcomes)}/{len(controls)} corruptions rejected")


if __name__ == "__main__":
    main()
