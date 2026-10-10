"""Extract AES-256, 96-bit-IV, 128-bit-tag cases from the official NIST archive."""
import argparse
import hashlib
import json
from pathlib import Path
import zipfile

URL = "https://csrc.nist.gov/CSRC/media/Projects/Cryptographic-Algorithm-Validation-Program/documents/mac/gcmtestvectors.zip"
SHA256 = "f9fc479e134cde2980b3bb7cddbcb567b2cd96fd753835243ed067699f26a023"


def records(source):
    config, record = {}, {}
    for line in source.splitlines() + [""]:
        line = line.strip()
        if not line:
            if record:
                if config.get("IVlen") == "96" and config.get("Taglen") == "128":
                    yield record
                record = {}
        elif line.startswith("["):
            name, value = line[1:-1].split(" = ")
            config[name] = value
        elif line == "FAIL":
            record["FAIL"] = True
        elif " = " in line:
            name, value = line.split(" = ", 1)
            record[name] = value
        elif line.endswith(" ="):
            record[line[:-2]] = ""


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("archive", type=Path)
    args = parser.parse_args()
    data = args.archive.read_bytes()
    if hashlib.sha256(data).hexdigest() != SHA256:
        raise RuntimeError("NIST archive checksum differs")
    cases = []
    with zipfile.ZipFile(args.archive) as archive:
        for name in ["gcmEncryptExtIV256.rsp", "gcmDecrypt256.rsp"]:
            for case in records(archive.read(name).decode()):
                cases.append(dict(source=name, **case))
    output = dict(source=URL, sha256=SHA256,
                  note="Unmodified official CAVP data selected only by AES-256, IVlen=96 and Taglen=128. These informal tests are not CAVP validation.",
                  cases=cases)
    Path(__file__).with_name("nist_gcm_cases.json").write_text(json.dumps(output, indent=2) + "\n")
    print(f"Recorded {len(cases)} official NIST GCM cases")


if __name__ == "__main__":
    main()
