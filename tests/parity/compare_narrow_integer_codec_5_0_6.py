#!/usr/bin/env python3
"""Compare narrow scalar decoding with complete original derive observations."""

import argparse
import gzip
import hashlib
import json
from pathlib import Path
import sys

from compare_values import observe, same

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / "scripts"))
from verify_narrow_integer_codec_5_0_6 import cases

CORPUS = Path(__file__).with_name("narrow_integer_codec_5_0_6_cases.json.gz")
CORPUS_SHA = "d461c0af524aad841c5e9b45bbab307e8b2c68aab986bf9fa78c3897d7dbbfaa"


def corpus():
    data = CORPUS.read_bytes()
    if hashlib.sha256(data).hexdigest() != CORPUS_SHA:
        raise RuntimeError("Narrow integer corpus bytes differ")
    result = json.loads(gzip.decompress(data))
    baseline = json.loads((ROOT / "upstream.lock.json").read_bytes())["commit"]
    if result["baseline"] != baseline or len(result["cases"]) != 376:
        raise RuntimeError("Complete locked narrow integer corpus required")
    if not same([row["input"] for row in result["cases"]], cases()):
        raise RuntimeError("Complete narrow integer input membership differs")
    if hashlib.sha256((ROOT / result["source"]).read_bytes()).hexdigest() != result["source_sha256"]:
        raise RuntimeError("Original narrow integer derive fixture differs")
    receipt = ROOT / "docs/parity/narrow_integer_codec_current_5_0_6.json.gz"
    if hashlib.sha256(receipt.read_bytes()).hexdigest() != result["execution_receipt_sha256"]:
        raise RuntimeError("Actual original narrow integer execution differs")
    return result


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--cpp", required=True)
    parser.add_argument("--cpp-emulator", default="")
    args = parser.parse_args()
    rows = corpus()["cases"]
    actual = observe(args.cpp, [row["input"] for row in rows],
                     emulator=[part for part in args.cpp_emulator.split(";") if part])
    if not same(actual, [row["expected"] for row in rows]):
        failures = [index for index, (left, right) in enumerate(zip(rows, actual, strict=True))
                    if not same(left["expected"], right)]
        raise RuntimeError("Narrow integer value/error mismatches: " + str(failures[:10]))
    print("Compared all 376 original narrow integer values and exact errors")


if __name__ == "__main__":
    main()
