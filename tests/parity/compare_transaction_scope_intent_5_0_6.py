#!/usr/bin/env python3
"""Compare explicit TransactionScope intent, SQLite effects and safe diagnostics."""

import argparse
import gzip
import hashlib
import itertools
import json
import os
from pathlib import Path
import subprocess
import sys


ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / "scripts"))
from verify_constant_closure_5_0_6 import registry_identities

CORPUS = Path(__file__).with_name("transaction_scope_intent_5_0_6_cases.json.gz")
FRAMING_CORPUS = Path(__file__).with_name("transaction_scope_intent_framing_5_0_6_cases.json.gz")
PROJECT = Path("tests/parity/rust_document_services_reference_5_0_6")
DRIVER = PROJECT / "src/bin/transaction_scope_intent_5_0_6.rs"
SOURCES = (
    "teaql-runtime/src/context/transaction.rs",
    "teaql-runtime/src/context/logging.rs",
    "teaql-data-service/src/lib.rs",
    "teaql-sql/src/executor.rs",
    "teaql-provider-sqlite/src/transaction.rs",
)


def digest(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def encoded(value):
    return json.dumps(value, ensure_ascii=True, sort_keys=True, separators=(",", ":"))


def cases():
    base = dict(comment="Inspect transaction rows", purpose="Verify transaction visibility",
                reason="Apply the reviewed transaction change", version=3, body_fail=False,
                select_logs=True, mutation_logs=True)
    flows = (("query",), ("mutate",), ("query", "mutate"),
             ("mutate", "query"), ("query", "mutate", "query"))
    for actions, body_fail, select, mutation in itertools.product(
            flows, (False, True), (False, True), (False, True)):
        yield dict(base, actions=list(actions), body_fail=body_fail,
                   select_logs=select, mutation_logs=mutation)
    # Reject missing intent before the relevant physical operation. When a read
    # precedes rejection, its successful result and diagnostics must survive.
    for field, blank, logs in itertools.product(
            ("comment", "purpose", "reason"), ("", " \u2003\t"), (False, True)):
        value = dict(base, actions=["query", "mutate", "query"],
                     select_logs=logs, mutation_logs=logs)
        value[field] = blank
        yield value
    for actions, version, logs in itertools.product(
            (("mutate", "query"), ("query", "mutate", "query")), (2, 4), (False, True)):
        yield dict(base, actions=list(actions), version=version,
                   select_logs=logs, mutation_logs=logs)
    for logs, body_fail in itertools.product((False, True), (False, True)):
        yield dict(base, actions=["query", "mutate", "query"],
                   comment="Inspect \u96ea " + "c" * 160,
                   purpose="Verify \u96ea " + "p" * 160,
                   reason="Review \u96ea " + "r" * 160,
                   select_logs=logs, mutation_logs=logs, body_fail=body_fail)


def provenance():
    baseline = json.loads((ROOT / "upstream.lock.json").read_text())["commit"]
    actual = subprocess.check_output(
        ["git", "-C", str(ROOT / ".reference/teaql-rs"), "rev-parse", "HEAD"],
        text=True).strip()
    if actual != baseline:
        raise RuntimeError("Transaction oracle reference differs from the active lock")
    if subprocess.check_output(
            ["git", "-C", str(ROOT / ".reference/teaql-rs"),
             "status", "--porcelain", "--untracked-files=no"], text=True).strip():
        raise RuntimeError("The frozen transaction oracle checkout has tracked changes")
    for source in SOURCES:
        original = subprocess.check_output(
            ["git", "-C", str(ROOT / ".reference/teaql-rs"),
             "show", f"{baseline}:{source}"])
        if hashlib.sha256(original).hexdigest() != digest(ROOT / ".reference/teaql-rs" / source):
            raise RuntimeError(f"Original transaction source changed: {source}")
    if not registry_identities(ROOT / PROJECT / "Cargo.lock") <= registry_identities(
            ROOT / ".reference/teaql-rs/Cargo.lock"):
        raise RuntimeError("Transaction oracle dependency identities differ from the reference")
    return dict(baseline=baseline, observer=digest(ROOT / DRIVER),
                manifest=digest(ROOT / PROJECT / "Cargo.toml"),
                lock=digest(ROOT / PROJECT / "Cargo.lock"),
                sources={path:digest(ROOT / ".reference/teaql-rs" / path) for path in SOURCES})


def framing_cases():
    for separator in ("\u0085", "\u2028", "\u2029", "\u0085\u2028\u2029"):
        value = next(cases())
        value.update(actions=["query", "mutate", "query"],
                     comment="Inspect" + separator + "rows",
                     purpose="Verify" + separator + "visibility",
                     reason="Review" + separator + "change",
                     select_logs=True, mutation_logs=True)
        yield value


def observe(executable, inputs):
    env = dict(os.environ, TEAQL_LOG_ENDPOINT="off")
    process = subprocess.run([str(Path(executable).resolve())],
                             input="".join(encoded(value) + "\n" for value in inputs),
                             text=True, capture_output=True, env=env, timeout=180)
    if process.returncode:
        raise RuntimeError(f"Transaction observer failed ({process.returncode}): {process.stderr}")
    # JSON Lines uses LF. Other Unicode line separators can occur inside JSON
    # strings and must remain part of the single observation that contains them.
    lines = process.stdout.split("\n")
    if lines and lines[-1] == "":
        lines.pop()
    values = [json.loads(line) for line in lines]
    if len(values) != len(inputs):
        raise RuntimeError("Transaction observer result cardinality differs")
    for value in values:
        if set(value) != {"outcome", "steps", "persisted", "logs"}:
            raise RuntimeError("Transaction observer result shape differs")
        if not all(isinstance(value[key], list) for key in ("steps", "persisted", "logs")):
            raise RuntimeError("Transaction observer collections are malformed")
    return values


def check_corpus(path, inputs, origin, args):
    original = observe(args.rust, inputs) if args.rust else None
    if args.record:
        corpus = dict(provenance=origin,
                      projection="All steps, snapshots, persisted rows and stable safe-log fields; wall-clock times excluded",
                      cases=[dict(input=input, expected=value) for input, value in zip(inputs, original)])
        if path.exists():
            with gzip.open(path, "rt") as file:
                if encoded(json.load(file)) != encoded(corpus):
                    raise RuntimeError("Refusing to overwrite a different transaction corpus")
        else:
            path.write_bytes(gzip.compress((json.dumps(corpus, indent=2) + "\n").encode(), mtime=0))
    else:
        with gzip.open(path, "rt") as file:
            corpus = json.load(file)
        if encoded(corpus["provenance"]) != encoded(origin):
            raise RuntimeError("Transaction corpus provenance differs")
        if encoded([value["input"] for value in corpus["cases"]]) != encoded(inputs):
            raise RuntimeError("Transaction corpus input matrix differs")
    expected = [value["expected"] for value in corpus["cases"]]
    for label, results in (("Rust", original),
                           ("C++", observe(args.cpp, inputs) if args.cpp else None)):
        if results is None:
            continue
        for index, (want, actual) in enumerate(zip(expected, results)):
            if encoded(want) != encoded(actual):
                print(json.dumps(dict(index=index, input=inputs[index], expected=want, actual=actual), indent=2))
                raise SystemExit(f"{label} transaction intent observation differs")


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--cpp")
    parser.add_argument("--rust")
    parser.add_argument("--record", action="store_true")
    args = parser.parse_args()
    if not args.cpp and not args.rust:
        parser.error("An actual observer is required")
    if args.record and not args.rust:
        parser.error("--record requires the actual original Rust observer")
    origin = provenance()
    matrices = ((CORPUS, list(cases())), (FRAMING_CORPUS, list(framing_cases())))
    for path, inputs in matrices:
        check_corpus(path, inputs, origin, args)
    print(f"TransactionScope explicit intent: {sum(len(inputs) for _, inputs in matrices)} complete observations match")


if __name__ == "__main__":
    main()
