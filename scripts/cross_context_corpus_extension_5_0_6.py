"""Qualify the exact added corpus declaration without relabeling old execution."""

import gzip
import hashlib
import json
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
PRODUCER = ROOT.parent / "teaql_code_gen_cpp"
SOURCE = ROOT / "scripts/verify_audit_metadata_source_5_0_6.py"
TRANSITION = ROOT / "docs/parity/cross_context_corpus_extension_current_5_0_6.json.gz"
TRANSITION_SHA256 = "360609279e5e32f420547511a965636073a0314b685804793f69859325a49da2"
BEFORE_SHA256 = "1445e9e3f7aa227e69bf0dbd37f8291ee27eaf643b97e54a25400e291afdea9d"
AFTER_SHA256 = "362393bcb94db766521bfae3d3b743c8821b19d5588366b1aa56e97d6236ff8e"
CORPUS = PRODUCER / "generator/src/test/resources/cpp_reverse_relation_type_composition_metadata_5_0_6.json.gz"
CORPUS_SHA256 = "fe6b8727f16b3ec10c2c99032faf3977466c3f8dcac9ed0b656bf446d3d17d10"
ADDITION = '    (PRODUCER / "generator/src/test/resources/cpp_reverse_relation_type_composition_metadata_5_0_6.json.gz", 2495),\n'


def require(condition, message):
    if not condition:
        raise RuntimeError("Corpus declaration qualification: " + message)


def sha(value):
    return hashlib.sha256(value).hexdigest()


def verify_extension(record, current_source, corpus_bytes):
    require(record["complete"] is True and record["whole_goal_complete"] is False
            and record["kind"] == "one_corpus_prepend", "bounded source extension")
    require(record["runtime_commit"] == "fe82a18982415f1746df9a2cf7c42f5e9abdc6d3",
            "locked runtime")
    require(record["generator_commit"] == "45e1572c4956ca963410d8ea436ead3c8857087a",
            "locked generator")
    before, after = record["source_before"], record["source_after"]
    require(sha(before.encode()) == BEFORE_SHA256 and sha(after.encode()) == AFTER_SHA256
            and current_source == after, "exact prior and current source bytes")
    require(after.count(ADDITION) == 1 and "CORPORA = (\n" + ADDITION in after
            and after.replace(ADDITION, "", 1) == before,
            "one prepended declaration; every other byte preserved")
    require(record["added_corpus_sha256"] == sha(corpus_bytes) == CORPUS_SHA256,
            "separately qualified added corpus bytes")
    corpus = json.loads(gzip.decompress(corpus_bytes))
    require(corpus["runtime_commit"] == record["runtime_commit"]
            and corpus["generator_commit"] == record["generator_commit"]
            and type(corpus["unique_inputs"]) is int and corpus["unique_inputs"] == 2495
            and len(corpus["cases"]) == 2495 and corpus["source_case_count"] == 4000,
            "exact added source corpus")
    require(corpus["source_receipt_sha256"]
            == "f73631da9535f1a6c6359a5d5d6d817be5a4a1d2dfa8dffa9e71bfc017486bd2",
            "original added-corpus execution identity")


def source_input_matches(path, expected):
    if path != SOURCE:
        return sha(path.read_bytes()) == expected
    if expected != BEFORE_SHA256:
        return False
    data = TRANSITION.read_bytes()
    require(sha(data) == TRANSITION_SHA256, "immutable source admission")
    verify_extension(json.loads(gzip.decompress(data)), SOURCE.read_text(), CORPUS.read_bytes())
    return True
