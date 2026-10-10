"""Port the original subprocess privacy check for default dynamic warnings."""
import argparse
import subprocess

parser = argparse.ArgumentParser()
parser.add_argument("--cpp", required=True)
parser.add_argument("--cpp-emulator", default="")
args = parser.parse_args()
emulator = args.cpp_emulator.split(";") if args.cpp_emulator else []
result = subprocess.run([*emulator, args.cpp, "--canary-child"], text=True,
                        capture_output=True, check=True)
assert "running 1 test" in result.stdout
for output in (result.stdout, result.stderr):
    assert "CLIENT_SECRET_FIELD_PATH_91" not in output
    assert "SECRET_VALUE_99" not in output
assert "<omitted>" in result.stderr
print("Original dynamic-search subprocess warning privacy passed")
