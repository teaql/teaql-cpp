#!/usr/bin/env python3
"""Reproduce immutable i18n facts, without invoking the application generator."""
import argparse
import json
from pathlib import Path
import subprocess


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--unicode-oracle", required=True,
                        help="Compiled unicode_language_table Rust observation tool")
    parser.add_argument("--check", action="store_true")
    args = parser.parse_args()
    root = Path(__file__).resolve().parents[1]
    lock = json.loads((root / "upstream.lock.json").read_text())
    source = subprocess.check_output(
        ["git", "-C", str(root / ".reference/teaql-rs"), "show",
         f"{lock['commit']}:teaql-runtime/src/builtin-i18n-v1.json"], text=True)
    catalog = json.loads(source)
    entries = [(locale, namespace == "vocabulary", key, text)
               for locale, entry in catalog["locales"].items()
               for namespace in ("messages", "vocabulary")
               for key, text in entry[namespace].items()]
    lines = ["// Immutable translations from Rust v5.0.5 builtin-i18n-v1.json.",
             "#pragma once", "#include <array>", "#include <string_view>",
             "namespace teaql::detail {",
             "struct BuiltinI18nEntry { std::string_view locale; bool vocabulary; std::string_view key,text; };",
             f"inline constexpr std::array<BuiltinI18nEntry,{len(entries)}> builtin_i18n_entries{{{{"]
    for locale, vocabulary, key, text in entries:
        values = [json.dumps(locale, ensure_ascii=True), str(vocabulary).lower(),
                  json.dumps(key, ensure_ascii=True), json.dumps(text, ensure_ascii=True)]
        lines.append("    {" + ", ".join(values) + "},")
    lines.extend(["}};", "}"])
    facts = {
        "teaql_runtime/include/teaql/runtime/detail/builtin_i18n_data.hpp": "\n".join(lines) + "\n",
        "teaql_core/include/teaql/core/detail/language_case_tables.hpp": subprocess.check_output(
            [str(Path(args.unicode_oracle).resolve())], text=True),
    }
    for name, content in facts.items():
        target = root / name
        if args.check:
            if not target.exists() or target.read_text() != content:
                raise SystemExit(f"Immutable i18n facts differ: {name}")
        else:
            target.write_text(content)
    print(f"{'Checked' if args.check else 'Extracted'} {len(entries)} builtin translations and Unicode facts")


if __name__ == "__main__":
    main()
