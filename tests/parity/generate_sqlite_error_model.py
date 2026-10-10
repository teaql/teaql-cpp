#!/usr/bin/env python3
"""Record actual locked Rust driver/provider model calls without normalizing outputs."""
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
    inputs = []
    codes = [-2147483648, -2, -1, *range(27), 257, 517, 1555, 1811, 2147483647]
    messages = [None, "", "native failure", "quote\"\\\n\x00 / 雪 / 🧊"]
    for code, message in itertools.product(codes, messages):
        inputs.append({"kind": "native", "code": code, "message": message})
    for code, offset, text in itertools.product([1, 257, -2147483648],
                                              [-1, 0, 11, 2147483647],
                                              ["", "SELECT missing FROM data", "雪\"\\\n\x00🧊"]):
        inputs.append({"kind": "input", "code": code, "message": text,
                       "sql": "SQL:" + text, "offset": offset})
    sizes = ["0", "1", "31", "4294967295"]
    for given, expected in itertools.product(sizes, sizes):
        inputs.append({"kind": "count", "given": given, "expected": expected})
    inputs.extend([{"kind": "multiple"}, {"kind": "returned"}])
    inputs.extend({"kind": "index", "index": index} for index in sizes)
    for index, name, kind in itertools.product(sizes, ["", "column", "雪\"\\\n\x00🧊"], range(5)):
        inputs.append({"kind": "type", "index": index, "name": name, "type": kind})
    inputs.extend({"outer": "unsupported", "text": text} for text in ["", "list", "object", "unknown"])
    for outer, text in itertools.product(["column", "bind", "lock", "compile", "shape"],
                                        ["", "field", "雪\"\\\n\x00🧊", "long:" + "x" * 256]):
        inputs.append({"outer": outer, "text": text})
    invalid_utf8 = [b"\xff", b"\xc2", b"\xe1\x80", b"\xf0\x90\x80", b"\xed\xa0\x80",
                    b"\xe0\x9f\x80", b"a\xffb", "雪".encode() + b"\xf0\x90\x80a"]
    inputs.extend({"kind":"utf8","index":index,"bytes":list(value)}
                  for index,value in itertools.product(["0","1","31"],invalid_utf8))
    inputs.extend({"kind":"nul","bytes":list(value)} for value in
                  [b"\x00", b"a\x00b", b"\xff\x00\x80", b"a\x00b\x00", b"long:"+b"x"*256+b"\x00tail"])
    inputs.extend({"kind":"path","bytes":list(value)} for value in
                  [b"", b".", b"./a", b"a/./b", b"/a", b"a\x00b", b"a\xffb", b"\xe1\x80", "雪/á".encode()])
    def facade_input(value):
        result = dict(value)
        result["variant"] = result["kind"]
        result["kind"] = "simple"
        if "changed" in result:
            result["changed"] = facade_input(result["changed"])
        return result
    with gzip.open(Path(__file__).with_name("sqlite_simple_error_cases.json.gz"),"rt") as source:
        inputs.extend(facade_input(case["input"]) for case in json.load(source)["cases"])
    chains = [[""], ["failure"], ["root", "middle", "leaf"], ["quote\"\\\n\x00雪🧊", "nested"], ["long:" + "x"*256]]
    for index,kind,messages in itertools.product(["0","1","31"],range(5),chains):
        inputs.append({"kind":"boxed","category":0,"index":index,"type":kind,"messages":messages})
    inputs.extend({"kind":"boxed","category":category,"messages":messages}
                  for category,messages in itertools.product([1,2],chains))
    # ffi::Error has public independent fields; exercise states which Error::new
    # cannot construct, then observe the original enum accessors and diagnostics.
    for kind,category,code in itertools.product(["native","input"],range(24),[1555,-2147483648]):
        value={"kind":kind,"category":category,"code":code,"message":None}
        if kind == "input":
            value.update(message="native payload",sql="SELECT payload",offset=-1)
        changed=dict(value,category=(category+1)%24)
        value["changed"]=changed
        inputs.append(value)
    results = observe(args.rust, inputs)
    target = Path(__file__).with_name("sqlite_error_model_cases.json.gz")
    if target.exists():
        with gzip.open(target,"rt") as old_file:
            previous = json.load(old_file)["cases"]
        prefix = [{"input":value,"expected":result} for value,result in zip(inputs,results)][:len(previous)]
        if prefix != previous:
            raise RuntimeError("Existing original error observations changed; inspect before replacing them")
    corpus = {"baseline": json.loads((root / "upstream.lock.json").read_text())["commit"],
              "note": "Actual rusqlite 0.40.1 driver models and original provider From/Debug/Error; no normalized observations. All 25 enabled tags and independently constructed native category/extended-code fields are observed; whole provider API/lifecycle/composition acceptance remains separate.",
              "cases": [{"input": value, "expected": result} for value, result in zip(inputs, results)]}
    with target.open("wb") as out:
        with gzip.GzipFile(fileobj=out, mode="wb", mtime=0) as compressed:
            compressed.write((json.dumps(corpus, ensure_ascii=True, separators=(",", ":")) + "\n").encode())
    print(f"Recorded {len(inputs)} actual SQLite driver/provider error observations")


if __name__ == "__main__":
    main()
