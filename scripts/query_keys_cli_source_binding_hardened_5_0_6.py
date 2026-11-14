#!/usr/bin/env python3
"""Admit the source-qualified width control through the original key validator."""

import copy
from pathlib import Path
import sys
from unittest.mock import patch

import query_keys_cli_source_binding_5_0_6 as historical


ROOT = historical.ROOT
REGISTRY = historical.REGISTRY
read = historical.read
read_ctest = historical.read_ctest
digest = historical.digest
require = historical.require
HISTORICAL_WIDTH = ROOT / "tests/parity/verify_query_key_width_cli_5_0_6.py"
CURRENT_WIDTH = ROOT / "tests/parity/verify_query_key_width_cli_hardened_5_0_6.py"
HISTORICAL_WIDTH_SHA256 = "d935e02b2bc78ec7023afd7fbe36c17d0631a13bfb960a9886e1ed14de38d09f"
CURRENT_WIDTH_SHA256 = "a48781fc80c217d7fff5467156878c1863875cfb6434af35e66cb38dd8605175"


class Validator(historical.Validator):
    def validate_commands(self, tests, build):
        require(digest(HISTORICAL_WIDTH) == HISTORICAL_WIDTH_SHA256
                and digest(CURRENT_WIDTH) == CURRENT_WIDTH_SHA256,
                "qualified width-control source identity")
        candidates = [row for row in tests if row["name"] == "parity_query_key_width_cli"]
        require(len(candidates) == 1, "one qualified width-control registration")
        command = candidates[0]["command"]
        require(len(command) == 2
                and Path(command[0]).resolve() == Path(sys.executable).resolve()
                and command[1] == str(CURRENT_WIDTH),
                "qualified current width-control command")
        translated = copy.deepcopy(tests)
        selected = next(row for row in translated if row["name"] == "parity_query_key_width_cli")
        selected["command"][1] = str(HISTORICAL_WIDTH)
        return super().validate_commands(translated, build)


def main():
    with patch.object(historical, "Validator", Validator):
        return historical.main()


if __name__ == "__main__":
    main()
