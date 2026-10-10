#!/usr/bin/env python3
"""Record real Rust byte-validation, NUL ownership and Unix path observations."""
import argparse
import gzip
import itertools
import json
from pathlib import Path
from compare_values import observe


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--rust", required=True)
    args = parser.parse_args()
    root = Path(__file__).resolve().parents[2]
    samples = {b"", b"ascii", "雪🧊á\n\x00".encode()}
    samples.update(bytes([first]) for first in range(256))
    boundaries = [0, 127, 128, 143, 144, 159, 160, 191, 192, 255]
    samples.update(bytes([first, second]) for first, second in itertools.product(range(256), boundaries))
    samples.update(bytes([first, *rest]) for first, rest in itertools.product(
        [0xe0, 0xe2, 0xed, 0xef], itertools.product(boundaries, repeat=2)))
    samples.update(bytes([first, *rest]) for first, rest in itertools.product(
        [0xf0, 0xf1, 0xf4, 0xf5], itertools.product([0, 127, 128, 143, 144, 191, 192, 255], repeat=3)))
    seeds = [b"\xc2", b"\xe0\xa0", b"\xf0\x90\x80", b"\xed\xa0\x80", b"\xc0\x80", b"\xff"]
    for prefix, sequence, suffix in itertools.product([b"", b"prefix", "雪".encode()], seeds,
                                                       [b"", b"a", b"\x00", b"\x80"]):
        samples.add(prefix + sequence + suffix)
    inputs = [{"bytes":list(value), "other":list(value)} for value in sorted(samples)]
    paths = [b"", b".", b"./", b"./a", b"a", b"a/", b"a//", b"a/.", b"a/./b", b"a/b",
             b"a/../b", b"../a", b"/", b"//", b"///a", b"/a", b"/./a", b"/a/..", b"a\x00b",
             b"a\xffb", "雪/a".encode(), "a/á".encode()]
    inputs.extend({"bytes":list(a), "other":list(b)} for a, b in itertools.product(paths, repeat=2))
    expected = observe(args.rust, inputs)
    corpus = {"baseline":json.loads((root / "upstream.lock.json").read_text())["commit"],
              "profile":"Unix native path bytes; portable UTF-8 and NUL payloads",
              "note":"Actual standard-library calls; no copied validation/path algorithm or normalized diagnostics.",
              "cases":[{"input":value,"expected":result} for value,result in zip(inputs,expected)]}
    target = Path(__file__).with_name("sqlite_native_diagnostics_cases.json.gz")
    with target.open("wb") as out:
        with gzip.GzipFile(fileobj=out, mode="wb", mtime=0) as compressed:
            compressed.write((json.dumps(corpus,ensure_ascii=True,separators=(",", ":"))+"\n").encode())
    print(f"Recorded {len(inputs)} actual native diagnostic observations")


if __name__ == "__main__":
    main()
