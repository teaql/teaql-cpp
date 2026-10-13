#!/usr/bin/env python3
"""Compare public SmartList calls with separately replayed Rust 5.0.6 evidence."""

from compare_values import main


if __name__ == "__main__":
    raise SystemExit(main("smart_list_api_5_0_6_cases.json.gz", "SmartList API 5.0.6"))
