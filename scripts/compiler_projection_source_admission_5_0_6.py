"""Admit only the tested compiler-resolver source and complete SDK epochs."""

import argparse
import copy
from functools import cache
import gzip
import hashlib
import json
from pathlib import Path
import sys

ROOT = Path(__file__).resolve().parents[1]
PRODUCER = ROOT.parent / "teaql_code_gen_cpp"
EVIDENCE = ROOT / "docs/parity"
RECORDS = {
    "generation": ("compiler_projection_generation_5_0_6.json.gz", "148f8020b032d0a7a2bc421333d725a3d2128f928391a27f53079e668f16880a"),
    "sdk": ("compiler_projection_sdks_5_0_6.json.gz", "c6ab0166121cc6adb483178ff16194a0dcd587313fbaa506ecb1d41e8ca572e3"),
    "preserved": ("compiler_projection_preserved_contracts_5_0_6.json.gz", "086c6774a2128e475a341adeb31b26c5280ea68fc650fcfbf17a17ae88b1d660"),
    "source": ("compiler_projection_source_transition_5_0_6.json.gz", "77eab8ca41092e1828ec38c69366d78a45b38074b7472a6742c88b034255a814"),
    "admission": ("compiler_projection_admission_transition_5_0_6.json.gz", "e5e2e89fb3e3b3385b2df98f878a415f43529656843b9e0e9e9448d10099af0a"),
    "forward": ("compiler_projection_forward_guard_transition_5_0_6.json.gz", "72f6fc1374f62236bfba56d1a17d8c81807a338aed0c158066a0b8b5d0406927"),
    "forward_tree": ("compiler_projection_forward_tree_transition_5_0_6.json.gz", "8786cdb0a2e1a7d6b272ed33125cd006bf4d2293e36e65bf402cf85a48239606"),
    "terminal": ("compiler_projection_terminal_guard_transition_5_0_6.json.gz", "79ee008c4cd760cf046577a66f7cf7df8ff512e57706f1dbb0bc8436220ef126"),
    "generation_controls": ("compiler_projection_generation_controls_5_0_6.json.gz", "35192c66aa8b954d70a634da6b61b36e71a6706e63b919ce7ce0b521276f18b1"),
    "generation_controls_optimized": ("compiler_projection_generation_controls_optimized_5_0_6.json.gz", "381629fa72e7f740802dbe50190b0243e36fdd422276e630ca3cb7aea338385d"),
    "sdk_controls": ("compiler_projection_sdk_controls_5_0_6.json.gz", "9427a6b57488fb6a056cf25211d713cc4d263badd66049afa175f08b1b7df489"),
    "sdk_controls_optimized": ("compiler_projection_sdk_controls_optimized_5_0_6.json.gz", "cb48456c2cd40a26cbcb2857cb5572ccd6f5c2be4a9eb276c9c27748ed55a3bb"),
    "preserved_controls": ("compiler_projection_preserved_controls_5_0_6.json.gz", "e570f91e70219de532b823cefbd5cd15d11af65a1a9b6044fdf493a52e61e029"),
    "preserved_controls_optimized": ("compiler_projection_preserved_controls_optimized_5_0_6.json.gz", "30fc4446ee9d4cfc2b5753f7ba0c6e8b4b2a03327e125a9970343c36558e861c"),
}


def require(value, message):
    if not value:
        raise RuntimeError("Compiler resolver source admission: " + message)


def sha(data):
    return hashlib.sha256(data).hexdigest()


def read(path):
    return json.loads(gzip.decompress(path.read_bytes()))


def bound(key):
    name, expected = RECORDS[key]
    data = (EVIDENCE / name).read_bytes()
    require(sha(data) == expected, "immutable evidence: " + name)
    return json.loads(gzip.decompress(data))


def admission_edits():
    imported = "    from compiler_projection_source_admission_5_0_6 import current_input_matches\n\n"
    return {
        "scripts/reproduce_const_function_sdks_5_0_6.py": [
            ("def validate(record):\n", "def validate(record):\n" + imported),
            ('all(digest(directory / key) == value for key, value in after["files"].items())',
             'all(current_input_matches(directory / key, value) for key, value in after["files"].items())'),
        ],
        "scripts/verify_const_function_sql_5_0_6.py": [
            ("def validate(record):\n", "def validate(record):\n" + imported),
            ('all(Path(name).is_file() and digest(Path(name)) == value\n                for name, value in record["input_hashes_after"].items())',
             'all(current_input_matches(Path(name), value)\n                for name, value in record["input_hashes_after"].items())'),
        ],
        "scripts/const_function_source_admission_5_0_6.py": [
            ("def validate(records):\n", "def validate(records):\n    from compiler_projection_source_admission_5_0_6 import current_input_matches as compiler_input_matches\n\n"),
            ('all(Path(name).is_file() and sha(Path(name).read_bytes()) == value\n                for name, value in java["sources"].items())',
             'all(compiler_input_matches(Path(name), value)\n                for name, value in java["sources"].items())'),
            ('sha((ROOT / "scripts" / path).read_bytes()) == control["current_helper_sha256"]',
             'compiler_input_matches(ROOT / "scripts" / path, control["current_helper_sha256"])'),
            ("def current_input_matches(path, expected):\n", "def current_input_matches(path, expected):\n    from compiler_projection_source_admission_5_0_6 import current_input_matches as compiler_input_matches\n\n"),
            ('return path.is_file() and sha(path.read_bytes()) == after', 'return compiler_input_matches(path, after)'),
            ('return path.is_file() and sha(path.read_bytes()) == expected', 'return compiler_input_matches(path, expected)'),
        ],
        "tests/parity/verify_ap02_reverse_relation_sql_receipt_5_0_6.py": [
            ("def validate(record):\n", "def validate(record):\n    from compiler_projection_source_admission_5_0_6 import java_inventory_matches\n\n"),
            ('java_sources <= record["input_hashes_before"].keys()',
             'java_inventory_matches(java_sources, record["input_hashes_before"].keys())'),
        ],
    }


def validate(records):
    from compiler_source_service_source_admission_5_0_6 import registered_epoch_matches as service_epoch_matches

    from preserve_compiler_projection_contracts_5_0_6 import validate as validate_preserved
    from reproduce_compiler_projection_sdks_5_0_6 import validate as validate_sdks
    from verify_compiler_projection_generation_5_0_6 import validate as validate_generation

    validate_generation(records["generation"])
    validate_sdks(records["sdk"])
    validate_preserved(records["preserved"])
    require(records["generation"]["java_test_count"] == 236 and records["generation"]["check_mips"] is True,
            "all current Java contracts and actual target guard")
    source = records["source"]
    require(source["complete"] is True and len(source["paths"]) == 7
            and source["generation_sha256"] == RECORDS["generation"][1], "exact tested producer and recipe transition")
    for name, row in source["paths"].items():
        root = PRODUCER if name.startswith("producer:") else ROOT
        path = root / name.split(":", 1)[1]
        require(sha(path.read_bytes()) == row["after_sha256"] == sha(row["after_source"].encode()), "actual tested source bytes")
        if row["before_source"] is not None:
            require(sha(row["before_source"].encode()) == row["before_sha256"], "immutable source preimage")
    transition = records["admission"]
    edits = admission_edits()
    require(transition["complete"] is True and set(transition["paths"]) == set(edits), "four explicit predicate transitions")
    for name, row in transition["paths"].items():
        restored = row["before_source"]
        for before, after in edits[name]:
            require(restored.count(before) == 1, "one exact historical predicate/import")
            restored = restored.replace(before, after, 1)
        require(restored == row["after_source"] and sha(restored.encode()) == row["after_sha256"]
                and sha(row["before_source"].encode()) == row["before_sha256"]
                and registered_epoch_matches(ROOT / name, sha(restored.encode())), "all other historical bodies and controls preserved")
    for kind, count in (("generation", 16), ("sdk", 8), ("preserved", 8)):
        for optimized in (0, 1):
            control = records[kind + "_controls" + ("_optimized" if optimized else "")]
            require(control["complete"] is True and type(control["optimized"]) is int and control["optimized"] == optimized
                    and len(control["controls"]) == count and all(item["rejected"] is True for item in control["controls"])
                    and control["execution_sha256"] == RECORDS[kind][1], "actual ordinary/optimized artifact controls")
    forward = records["forward"]
    require(forward["complete"] is True and forward["path"] == "scripts/nominal_type_source_admission_5_0_6.py"
            and forward["before_source"].count(forward["edit"]["before"]) == 1
            and forward["before_source"].replace(forward["edit"]["before"], forward["edit"]["after"], 1) == forward["after_source"]
            and sha(forward["before_source"].encode()) == forward["before_sha256"]
            and sha(forward["after_source"].encode()) == forward["after_sha256"]
            and registered_epoch_matches(ROOT / forward["path"], forward["after_sha256"]), "one exact forward guard delegation")
    tree = records["forward_tree"]
    require(tree["complete"] is True and len(tree["paths"]) == 7, "seven remaining physical forward guards")
    for name, row in tree["paths"].items():
        expected = row["before_source"]
        for edit in row["edits"]:
            require(expected.count(edit["before"]) == 1, "exact physical guard predicate")
            expected = expected.replace(edit["before"], edit["after"], 1)
        require(expected == row["after_source"] and sha(expected.encode()) == row["after_sha256"]
                and sha(row["before_source"].encode()) == row["before_sha256"]
                and service_epoch_matches(ROOT / name, sha(expected.encode())), "all other forward helper bodies remain unchanged")
    terminal = records["terminal"]
    expected = terminal["before_source"]
    for edit in terminal["edits"]:
        require(expected.count(edit["before"]) == 1, "one exact terminal source-version boundary")
        expected = expected.replace(edit["before"], edit["after"], 1)
    require(terminal["complete"] is True and terminal["path"] == forward["path"]
            and terminal["before_sha256"] == forward["after_sha256"]
            and expected == terminal["after_source"] and sha(expected.encode()) == terminal["after_sha256"]
            and service_epoch_matches(ROOT / terminal["path"], sha(expected.encode())), "direct latest source-version delegation only")


@cache
def evidence():
    records = {key: bound(key) for key in RECORDS}
    validate(records)
    return records


@cache
def artifact_epochs():
    source, admission, sdk = (bound(key) for key in ("source", "admission", "sdk"))
    result = {}
    def add(path, before, after):
        result.setdefault(path, []).append((before, after))
    for name, row in source["paths"].items():
        if row["before_sha256"] is not None:
            root = PRODUCER if name.startswith("producer:") else ROOT
            add(root / name.split(":", 1)[1], row["before_sha256"], row["after_sha256"])
    for name, row in admission["paths"].items():
        add(ROOT / name, row["before_sha256"], row["after_sha256"])
    forward = bound("forward")
    add(ROOT / forward["path"], forward["before_sha256"], forward["after_sha256"])
    for name, row in bound("forward_tree")["paths"].items():
        add(ROOT / name, row["before_sha256"], row["after_sha256"])
    terminal = bound("terminal")
    add(ROOT / terminal["path"], terminal["before_sha256"], terminal["after_sha256"])
    for old, new in zip(sdk["before"], sdk["after"], strict=True):
        add(ROOT / old["manifest"], old["files"][Path(old["manifest"]).name], new["files"][Path(new["manifest"]).name])
    return result


def registered_epoch_matches(path, expected):
    from compiler_source_service_source_admission_5_0_6 import registered_epoch_matches as service_epoch_matches

    if not path.is_file():
        return False
    observed = sha(path.read_bytes())
    pending, visited = [expected], set()
    while pending:
        candidate = pending.pop()
        if candidate == observed or service_epoch_matches(path, candidate):
            return True
        if candidate in visited:
            continue
        visited.add(candidate)
        pending.extend(after for before, after in artifact_epochs().get(path, []) if before == candidate)
    return False


def current_input_matches(path, expected):
    from compiler_source_service_source_admission_5_0_6 import current_input_matches as service_input_matches

    if path.is_file() and sha(path.read_bytes()) == expected:
        return True
    if service_input_matches(path, expected):
        return True
    if path in artifact_epochs() and registered_epoch_matches(path, expected):
        evidence()
        return True
    return False


def java_inventory_matches(current, historical):
    unknown = set(current) - set(historical)
    if not unknown:
        return True
    source = bound("source")
    additions = {str(PRODUCER / name.split(":", 1)[1]): row["after_sha256"]
                 for name, row in source["paths"].items()
                 if name.startswith("producer:") and "/src/main/java/" in name and row["before_sha256"] is None}
    if not unknown <= additions.keys():
        from compiler_source_service_source_admission_5_0_6 import java_inventory_matches as service_inventory_matches
        if not service_inventory_matches(unknown, additions.keys()):
            return False
    evidence()
    return all(Path(name).is_file() and sha(Path(name).read_bytes()) == additions[name]
               for name in unknown if name in additions)


def controls():
    source = evidence()
    tests = (
        ("generation", "incomplete_generation", lambda r: r.update(complete=False)),
        ("generation", "invented_rust_execution", lambda r: r.update(original_rust_compiler_processes=4)),
        ("generation", "missing_native_compile", lambda r: r["steps"].pop(next(i for i, s in enumerate(r["steps"]) if s["label"] == "complete_native_compile"))),
        ("sdk", "missing_retained_sdk", lambda r: r["after"].pop()),
        ("sdk", "missing_resolver_fingerprint", lambda r: r["after"][0]["value"]["generator_implementation_sha256"].pop("generator/src/main/java/com/skynet/codegenerator/cpp/CppProjectionResolver.java")),
        ("preserved", "missing_old_contract", lambda r: r["cases"].pop()),
        ("preserved", "changed_old_sdk", lambda r: r["cases"][0]["files"].update(invented="0" * 64)),
        ("source", "changed_producer_source", lambda r: r["paths"]["producer:generator/src/main/java/com/skynet/codegenerator/cpp/CppCoreGenerator.java"].update(after_source="invalid")),
        ("admission", "lost_historical_body", lambda r: next(iter(r["paths"].values())).update(after_source="pass\n")),
        ("generation_controls_optimized", "lost_optimized_control", lambda r: r["controls"].pop()),
    )
    rejected = []
    for key, name, mutate in tests:
        changed = dict(source)
        changed[key] = copy.deepcopy(source[key])
        mutate(changed[key])
        try:
            validate(changed)
        except RuntimeError as error:
            rejected.append(dict(case=name, rejected=True, error=str(error)))
        else:
            raise RuntimeError("Accepted compiler source corruption: " + name)
    for path, edges in artifact_epochs().items():
        for index, (before, after) in enumerate(edges):
            require(current_input_matches(path, before) and current_input_matches(path, after)
                    and not current_input_matches(path, "0" * 64), "each exact previous/current input admits, unknown input rejects")
            rejected.append(dict(case="unknown_epoch:" + str(path.relative_to(PRODUCER if path.is_relative_to(PRODUCER) else ROOT)) + ":" + str(index), rejected=True))
    require(not java_inventory_matches({"/unknown/Unapproved.java"}, set()), "unknown Java inventory member rejected")
    rejected.append(dict(case="unknown_java_inventory", rejected=True))
    return rejected


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--controls", type=Path)
    args = parser.parse_args()
    evidence()
    rejected = controls()
    result = dict(complete=True, whole_goal_complete=False, optimized=sys.flags.optimize,
                  source_sha256=sha(Path(__file__).read_bytes()), valid_record_accepted=True, controls=rejected,
                  receipts={key: value[1] for key, value in RECORDS.items()})
    if args.controls:
        require(not args.controls.exists(), "preserve source controls")
        args.controls.write_bytes(gzip.compress(json.dumps(result, sort_keys=True).encode(), mtime=0))
    print("Compiler resolver source admission:", len(rejected), "corruptions/unknown epochs rejected; optimized=", sys.flags.optimize)


if __name__ == "__main__":
    main()
