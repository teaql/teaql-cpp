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
if not ("running 1 test" in result.stdout):
    raise AssertionError
for output in (result.stdout, result.stderr):
    if not ("CLIENT_SECRET_FIELD_PATH_91" not in output):
        raise AssertionError
    if not ("SECRET_VALUE_99" not in output):
        raise AssertionError
if not ("<omitted>" in result.stderr):
    raise AssertionError
print("Original dynamic-search subprocess warning privacy passed")
