#!/usr/bin/env python3
"""Record reproducible parsing observations from the locked Rust oracle."""
import argparse
import json
import random
import subprocess
from pathlib import Path

from compare_values import observe


def inputs():
    cases = []

    def add(text, method, radix=10):
        cases.append(dict(kind="parse", text=text, method=method, radix=radix))

    literals = ["", "+", "-", ".", "+.", "-.", "_", "_1", "._1", "1_", "1__2",
                "1._2", "1.2.3", "1 2", " 1", "1 ", "0", "-0", "-0.000", "+001.00",
                ".5", "-.5", "1.", "1e2", "1E-2", "1e+29", "1e-29", "1e--2",
                "NaN", "inf", "\u00e9", "1\0", "79228162514264337593543950335",
                "79228162514264337593543950336", "79228162514264337593543950335.5",
                "7922816251426433759354395033.56"]
    for text in literals:
        for method in ("normal", "exact", "radix"):
            add(text, method)
    for prefix in ("0.", "1.", "1844674407370955162.", "7922816251426433759354395033."):
        for length in (0, 1, 2, 17, 18, 19, 27, 28, 29, 30, 60):
            for suffix in ("", "0", "5", "9", "_", "__5", "x", ".", "e1"):
                for method in ("normal", "exact", "radix"):
                    add(prefix + "0" * length + suffix, method)
    alphabet = "0123456789abcdefghijklmnopqrstuvwxyz"
    for radix in (0, 1, *range(2, 37), 37, 2**32 - 1):
        for text in literals + ["a", "A", "z", "Z", "f.F", "101.101", "19", "9.9"]:
            add(text, "radix", radix)
        if 2 <= radix <= 36:
            high = alphabet[radix - 1]
            for length in (19, 20, 23, 24, 28, 29, 31, 32, 61, 95, 96, 97, 98):
                for text in (high * length, "0." + high * length,
                             "1." + high * length + "_5", "0" * length + "x"):
                    add(text, "radix", radix)
    rng = random.Random(505)
    boundaries = ["18446744073709551615", "1844674407370955162",
                  "79228162514264337593543950335", "7922816251426433759354395033"]
    for whole in boundaries:
        for fraction in ("0", "4", "5", "6", "9", "50", "99", "99999"):
            for sign in ("", "-", "+"):
                for method in ("normal", "exact", "radix"):
                    add(sign + whole + "." + fraction, method)
    for _ in range(1000):
        whole = "".join(rng.choice("0123456789") for _ in range(rng.randint(0, 35)))
        fraction = "".join(rng.choice("0123456789") for _ in range(rng.randint(0, 60)))
        text = whole + "." + fraction
        if rng.choice((True, False)):
            text = "_".join(text)
        text = rng.choice(("", "-", "+")) + text + rng.choice(("", "_", "x", ".", "e1"))
        for method in ("normal", "exact", "radix"):
            add(text, method)
    for _ in range(600):
        radix = rng.randint(2, 36)
        text = "".join(rng.choice(alphabet[:radix]) for _ in range(rng.randint(1, 105)))
        position = rng.randrange(len(text) + 1)
        if rng.choice((True, False)):
            text = text[:position] + "." + text[position:]
        if rng.choice((True, False)):
            text = "-" + text
        add(text, "radix", radix)
    # Preserve the first occurrence; corpus counts stay stable across runs.
    return list({json.dumps(case, sort_keys=True): case for case in cases}.values())


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--rust", required=True)
    args = parser.parse_args()
    root = Path(__file__).resolve().parents[2]
    subprocess.run(["python3", str(root / "scripts/verify-upstream.py")], check=True)
    lock = json.loads((root / "upstream.lock.json").read_text())
    cases = inputs()
    results = observe(args.rust, cases)
    corpus = dict(baseline=lock["commit"],
                  dependency="rust_decimal 1.42.1; tests/parity/rust-values/Cargo.lock",
                  note="Original exact/radix/FromStr calls, including errors and panic boundaries.",
                  cases=[dict(input=case, expected=result) for case, result in zip(cases, results)])
    destination = Path(__file__).with_name("decimal_parsing_cases.json")
    destination.write_text(json.dumps(corpus, indent=2, ensure_ascii=True) + "\n")
    print(f"Recorded {len(cases)} Decimal parsing observations")


if __name__ == "__main__":
    main()
