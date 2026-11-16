#!/usr/bin/env python3
"""Admit only qualified source-service epochs while preserving historical checks."""

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
    "service": ("compiler_source_service_current_5_0_6.json.gz", "18ded1436e0c6a07ee66bffef8ddb08234f76d1eccaa3ade132b87049979689c"),
    "plain": ("compiler_source_service_plain_jobs_5_0_6.json.gz", "f2ebbb7f95e9b9eeaa862ee3d8e37c113324faf6c581135a3ae93be8a99e3aec"),
    "sdk": ("compiler_source_service_sdks_current_5_0_6.json.gz", "e9fafcf61cf5c794902740efe606e585a62ee8b8e3275f520665b7bd5c50f625"),
    "preserved": ("compiler_source_service_preserved_contracts_5_0_6.json.gz", "f00f4a24681acd2c6341e0773d6f6014a3c7a8f7538c20354c4f3bb5d876677c"),
    "source": ("compiler_source_service_transition_5_0_6.json.gz", "fc92dbdedef6cf137d0a9e331ee92f50415a54a0be8b52ad19a8a8d37a9ca337"),
    "predicates": ("compiler_source_service_predicate_transition_5_0_6.json.gz", "990a05c8f8f21a0e5abf19d682c7140f359e1ac542d6d0a8acf159e5f4d58827"),
    "forward": ("compiler_source_service_forward_transition_5_0_6.json.gz", "2dbee597a05e78499ae2aa20a0b94981713d39b3c111048352bd687542994955"),
    "epoch_bridge": ("compiler_source_service_epoch_bridge_transition_5_0_6.json.gz", "e44f15d7a3e2b4b9a31c6779aec9fafe9673c79692f067421a13f878a3b861a6"),
    "corpus_forward": ("compiler_source_service_corpus_forward_transition_5_0_6.json.gz", "43b45221806f52f74b915b6c8ba854c249397a69eebaf0c64dbf7134c93ebb7d"),
    "service_controls": ("compiler_source_service_controls_current_5_0_6.json.gz", "f5bbd674698528f58f4b4861d38d3ed5c58c2e65f51ea03169b11ace6b37207d"),
    "service_controls_optimized": ("compiler_source_service_controls_current_optimized_5_0_6.json.gz", "929efebe7d4aec268ad50cfa6df10473a2141baa2f81ffa2f7f06163a1dd923c"),
    "sdk_controls": ("compiler_source_service_sdk_controls_5_0_6.json.gz", "bd1cb99dc0672e2f319709545b29400bc4bc8805825a05b5730e1e66d1f65a5e"),
    "sdk_controls_optimized": ("compiler_source_service_sdk_controls_optimized_5_0_6.json.gz", "72df49064bea9f9c4bf0b4b7051ae693e687149dd9820f92582d81ff632ecce1"),
    "preserved_controls": ("compiler_source_service_preserved_controls_5_0_6.json.gz", "9c2f089dabe18fdaa80854f17b5cd2adbaa4d1a0878cd693ca914ae05b8e0510"),
    "preserved_controls_optimized": ("compiler_source_service_preserved_controls_optimized_5_0_6.json.gz", "e0e33e166063bc1c57d1304005a2d705f78417af5b603d19f7ca681de5628b00"),
}


def require(value, message):
    if not value:
        raise RuntimeError("Source service admission: " + message)


def sha(data):
    return hashlib.sha256(data).hexdigest()


def bound(key):
    name, expected = RECORDS[key]
    data = (EVIDENCE / name).read_bytes()
    require(sha(data) == expected, "immutable receipt: " + name)
    return json.loads(gzip.decompress(data))


def path_for(name):
    repository, relative = name.split(":", 1)
    require(repository in ("runtime", "producer"), "known source repository")
    return (ROOT if repository == "runtime" else PRODUCER) / relative


def validate(records):
    from verify_compiler_source_service_5_0_6 import validate as validate_service
    from verify_cpp_compiler_job_5_0_6 import validate as validate_plain
    from reproduce_compiler_projection_sdks_5_0_6 import validate as validate_sdks
    from preserve_compiler_projection_contracts_5_0_6 import validate as validate_preserved

    validate_service(records["service"])
    validate_plain(records["plain"])
    validate_sdks(records["sdk"])
    validate_preserved(records["preserved"])
    require(records["sdk"]["observed_before"] == records["sdk"]["after"], "all current SDK manifests/code/guidance unchanged")
    for kind, count in (("service", 14), ("sdk", 8), ("preserved", 8)):
        for mode in (0, 1):
            control = records[kind + "_controls" + ("_optimized" if mode else "")]
            require(control["complete"] is True and type(control["optimized"]) is int and control["optimized"] == mode
                    and control["execution_sha256"] == RECORDS[kind][1]
                    and len(control["controls"]) == count and all(row["rejected"] is True for row in control["controls"]),
                    "actual complete ordinary/optimized controls")
    transition = records["source"]
    require(transition["complete"] is True and len(transition["paths"]) == 14, "fourteen executed implementation/source paths")
    for name, row in transition["paths"].items():
        require(registered_epoch_matches(path_for(name), row["after_sha256"]) and row["after_sha256"] == sha(row["after_source"].encode()), "actual qualified source")
        require(records["service"]["input_hashes_after"][name] == row["after_sha256"], "actual service execution source binding")
        if row["before_source"] is not None:
            require(sha(row["before_source"].encode()) == row["before_sha256"], "source preimage")
    service = transition["paths"]["producer:generator/src/main/java/com/skynet/codegenerator/DomainGenerationService.java"]
    restored = service["after_source"]
    start = restored.index("    private final Map<String, SourceGenerator> sourceGenerators;")
    end = restored.index("    public List<GeneratedFile> generateDomainFiles(Domain domain, String scope)")
    restored = restored[:start] + "\n" + restored[end:]
    for value in ("import java.util.Map;\n", "import com.skynet.codegenerator.meta.BaseDomainManager;\n",
                  '        if (sourceGenerators.containsKey(scope)) {\n            throw new IllegalArgumentException("Configured compiler generation requires original source files");\n        }\n'):
        require(restored.count(value) == 1, "one exact additive service branch/import")
        restored = restored.replace(value, "", 1)
    require(restored == service["before_source"], "all original default dispatch bodies unchanged")
    controller = transition["paths"]["producer:generator/src/main/java/com/skynet/codegenerator/GenerateController.java"]
    restored = controller["after_source"]
    start = restored.index("    public GenerateController() {")
    end = restored.index("    public static void main(", start)
    restored = restored[:start] + restored[end:]
    restored = restored.replace("    private final DomainGenerationService domainGenerationService;",
                                "    private final DomainGenerationService domainGenerationService = new DomainGenerationService();", 1)
    restored = restored.replace("domainGenerationService.generateSourceFiles(domain, generateContext)",
                                "generateDomainFiles(domain, generateContext.getScope())", 1)
    require(restored == controller["before_source"], "all other controller/validation/ZIP bodies unchanged")
    predicates = records["predicates"]
    require(predicates["complete"] is True and set(predicates["paths"]) == {
        "scripts/verify_compiler_projection_generation_5_0_6.py", "scripts/compiler_projection_source_admission_5_0_6.py"},
        "two explicit historical reader boundaries")
    for name, row in predicates["paths"].items():
        source = row["before_source"]
        for edit in row["edits"]:
            require(source.count(edit["before"]) == 1, "one exact predicate/import edit")
            source = source.replace(edit["before"], edit["after"], 1)
        require(source == row["after_source"] and sha(source.encode()) == row["after_sha256"]
                and sha(row["before_source"].encode()) == row["before_sha256"]
                and registered_epoch_matches(ROOT / name, row["after_sha256"]),
                "all remaining historical reader bodies and controls preserved")
    forward = records["forward"]
    require(forward["complete"] is True and set(forward["paths"]) == {
        "scripts/compiler_projection_source_admission_5_0_6.py",
        "scripts/nominal_type_source_admission_5_0_6.py",
        "scripts/projected_type_source_admission_5_0_6.py"}, "three explicit source-version forwarding boundaries")
    for name, row in forward["paths"].items():
        source = row["before_source"]
        for edit in row["edits"]:
            require(source.count(edit["before"]) == 1, "one exact forwarding predicate/import edit")
            source = source.replace(edit["before"], edit["after"], 1)
        require(source == row["after_source"] and sha(source.encode()) == row["after_sha256"]
                and sha(row["before_source"].encode()) == row["before_sha256"]
                and registered_epoch_matches(ROOT / name, row["after_sha256"]),
                "all other forwarding helper bodies and controls preserved")
        if name in predicates["paths"]:
            require(predicates["paths"][name]["after_sha256"] == row["before_sha256"],
                    "consecutive helper transitions share an exact source epoch")
    bridge = records["epoch_bridge"]
    name = "scripts/compiler_projection_source_admission_5_0_6.py"
    require(bridge["complete"] is True and set(bridge["paths"]) == {name}, "one pure source-epoch graph bridge")
    row = bridge["paths"][name]
    source = row["before_source"]
    for edit in row["edits"]:
        require(source.count(edit["before"]) == 1, "one exact graph predicate/import edit")
        source = source.replace(edit["before"], edit["after"], 1)
    require(source == row["after_source"] and sha(source.encode()) == row["after_sha256"]
            and sha(row["before_source"].encode()) == row["before_sha256"]
            and forward["paths"][name]["after_sha256"] == row["before_sha256"]
            and registered_epoch_matches(ROOT / name, row["after_sha256"]), "qualified graph composition preserves all other helper bodies")
    corpus = records["corpus_forward"]
    name = "scripts/nominal_type_source_admission_5_0_6.py"
    require(corpus["complete"] is True and set(corpus["paths"]) == {name}, "one bounded generated-caller source delegation")
    row = corpus["paths"][name]
    source = row["before_source"]
    for edit in row["edits"]:
        require(source.count(edit["before"]) == 1, "one exact generated-caller forwarding edit")
        source = source.replace(edit["before"], edit["after"], 1)
    require(source == row["after_source"] and sha(source.encode()) == row["after_sha256"]
            and sha(row["before_source"].encode()) == row["before_sha256"]
            and forward["paths"][name]["after_sha256"] == row["before_sha256"]
            and registered_epoch_matches(ROOT / name, row["after_sha256"]), "original corpus-specific before-only admission remains intact")


@cache
def evidence():
    records = {key: bound(key) for key in RECORDS}
    validate(records)
    return records


@cache
def source_epochs():
    result = {}
    for key in ("source", "predicates", "forward", "epoch_bridge", "corpus_forward"):
        for name, row in bound(key)["paths"].items():
            path = path_for(name) if key == "source" else ROOT / name
            result.setdefault(path, []).append(row)
    return result


def registered_epoch_matches(path, expected):
    from compiler_model_configuration_source_admission_5_0_6 import registered_epoch_matches as model_epoch_matches

    """Follow only hash-bound source edges, without recursively validating evidence."""
    if not path.is_file():
        return False
    observed = sha(path.read_bytes())
    pending, visited = [expected], set()
    while pending:
        candidate = pending.pop()
        if candidate == observed or model_epoch_matches(path, candidate):
            return True
        if candidate in visited:
            continue
        visited.add(candidate)
        pending.extend(row["after_sha256"] for row in source_epochs().get(path, [])
                       if row["before_sha256"] == candidate)
    return False


def current_input_matches(path, expected):
    from compiler_model_configuration_source_admission_5_0_6 import current_input_matches as model_input_matches

    if model_input_matches(path, expected):
        return True
    if not path.is_file():
        return False
    current = sha(path.read_bytes())
    if current == expected:
        return True
    if path not in source_epochs() or not registered_epoch_matches(path, expected):
        return False
    evidence()
    return True


def java_inventory_matches(current, historical):
    unknown = set(current) - set(historical)
    if not unknown:
        return True
    additions = {str(path): row for path, rows in source_epochs().items() for row in rows
                 if path.is_relative_to(PRODUCER / "generator/src/main/java") and row["before_source"] is None}
    if not unknown <= additions.keys():
        from compiler_model_configuration_source_admission_5_0_6 import java_inventory_matches as model_inventory_matches
        if not model_inventory_matches(unknown, additions.keys()):
            return False
    evidence()
    return all(sha(Path(name).read_bytes()) == additions[name]["after_sha256"] for name in unknown if name in additions)


def controls():
    valid = evidence()
    tests = (
        ("service", "incomplete_service", lambda r: r.update(complete=False)),
        ("service", "missing_actual_controller", lambda r: r["cases"].pop()),
        ("plain", "missing_plain_model", lambda r: r["cases"].pop()),
        ("sdk", "missing_retained_sdk", lambda r: r["after"].pop()),
        ("sdk", "changed_current_manifest", lambda r: r["observed_before"][0]["files"].update(invented="0" * 64)),
        ("preserved", "missing_default_contract", lambda r: r["cases"].pop()),
        ("source", "changed_production_source", lambda r: next(iter(r["paths"].values())).update(after_source="invalid")),
        ("predicates", "lost_historical_body", lambda r: next(iter(r["paths"].values())).update(after_source="pass\n")),
        ("forward", "lost_forwarding_body", lambda r: next(iter(r["paths"].values())).update(after_source="pass\n")),
        ("epoch_bridge", "lost_graph_bridge_body", lambda r: next(iter(r["paths"].values())).update(after_source="pass\n")),
        ("corpus_forward", "lost_corpus_forwarding_body", lambda r: next(iter(r["paths"].values())).update(after_source="pass\n")),
        ("service_controls_optimized", "missing_optimized_control", lambda r: r["controls"].pop()),
    )
    rejected = []
    for key, name, mutate in tests:
        changed = dict(valid)
        changed[key] = copy.deepcopy(valid[key])
        mutate(changed[key])
        try:
            validate(changed)
        except RuntimeError as error:
            rejected.append(dict(case=name, rejected=True, error=str(error)))
        else:
            raise RuntimeError("Source service corruption accepted: " + name)
    for path, rows in source_epochs().items():
        for row in rows:
            require(current_input_matches(path, row["after_sha256"]) and not current_input_matches(path, "0" * 64),
                    "exact qualified input and unknown epoch rejection")
            if row["before_sha256"] is not None:
                require(current_input_matches(path, row["before_sha256"]), "qualified prior source epoch")
            rejected.append(dict(case="source_epoch:" + str(path), after_sha256=row["after_sha256"], rejected=True))
    require(not java_inventory_matches({"/unqualified/Generator.java"}, set()), "unknown Java source rejected")
    rejected.append(dict(case="unknown_java_addition", rejected=True))
    return dict(complete=True, optimized=sys.flags.optimize, controls=rejected,
                valid_record_accepted=True, current_helper_sha256=sha(Path(__file__).read_bytes()))


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--output", type=Path, required=True)
    args = parser.parse_args()
    require(not args.output.exists(), "preserve existing controls")
    result = controls()
    args.output.write_bytes(gzip.compress(json.dumps(result, sort_keys=True).encode(), mtime=0))
    print(json.dumps(dict(complete=True, optimized=result["optimized"], controls=len(result["controls"]))))


if __name__ == "__main__":
    main()
