#!/usr/bin/env python3
"""Carry an AP02 partial-effect source case through generated SQLite Save and sinks."""

import argparse
import gzip
import hashlib
import json
from pathlib import Path
import sys
from xml.sax.saxutils import escape

import verify_generated_audit_sql_sinks_5_0_6 as sink


ROOT = Path(__file__).resolve().parents[1]
CORPUS = (ROOT.parent / "teaql_code_gen_cpp/generator/src/test/resources/"
          "cpp_cross_context_composition_metadata_5_0_6.json.gz")
SOURCE_RECEIPT = ROOT / "docs/parity/audit_cross_context_composition_maven_5_0_6.json.gz"
CASE = "cross_malformed_match_arm_audit_value_max_len"
EXPECTED_LIMIT = "13"
EXPECTED_MASKED = "PR***...***RY"


def require(condition, message):
    if not condition:
        raise RuntimeError(message)


def digest(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def checked_model(case_name, mask_declaration, source_cases):
    require(case_name == CASE and mask_declaration == "named"
            and source_cases.resolve() == CORPUS.resolve(),
            "Exact partial-effect model selection")
    corpus = json.loads(gzip.decompress(CORPUS.read_bytes()))
    require(corpus["source_receipt"] == str(SOURCE_RECEIPT.relative_to(ROOT))
            and corpus["source_receipt_sha256"] == digest(SOURCE_RECEIPT),
            "Locked Rust metadata receipt")
    case = next(row for row in corpus["cases"] if row["name"] == CASE)
    require(case["expected"] == {
        "declared": False, "entity": "Probe", "indexed": False,
        "limit": EXPECTED_LIMIT, "masks": [], "parsed": True,
        "relations": [], "service": None, "table": "probe_data",
    }, "Original partial metadata effects")
    require(case["input"].startswith('entity="Before", audit_value_max_len=13, '
                                      'audit_value_max_len=match value ')
            and ', entity="After", table="after_table", ' in case["input"],
            "Malformed middle expression and ignored later settings")
    old = 'advisor="specialist(optional=true)" _audit_mask_fields="name" />'
    baseline = sink.BASE_MODEL.read_text()
    require(baseline.count(old) == 1, "One School audit declaration")
    new = ('advisor="specialist(optional=true)" _audit_mask_fields="name"\n'
           '            _audit_value_max_len="'
           + escape(case["input"], {'"': '&quot;'}) + '" />')
    return corpus, case, baseline.replace(old, new)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--record", required=True, type=Path)
    parser.add_argument("--sink-record", required=True, type=Path)
    args = parser.parse_args()
    require(not args.record.exists() and not args.sink_record.exists(),
            "Fresh original/native execution receipts")
    sink.SOURCE_CASE_EXPECTATIONS[CASE] = (
        EXPECTED_LIMIT, EXPECTED_MASKED, True)
    sink.checked_model = checked_model
    sys.argv = [str(sink.__file__), "--source-case", CASE,
                "--source-corpus", str(CORPUS), "--record", str(args.record),
                "--sink-record", str(args.sink_record)]
    result = sink.main()
    if result != 0:
        return result
    record = json.loads(gzip.decompress(args.record.read_bytes()))
    require(record["complete"] is True and record["source_case"]["name"] == CASE,
            "Complete selected generated execution")
    record["cross_context_wrapper_sha256"] = digest(Path(__file__))
    record["cross_context_source_receipt_sha256"] = digest(SOURCE_RECEIPT)
    args.record.write_bytes(gzip.compress(
        (json.dumps(record, indent=2) + "\n").encode(), mtime=0))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
