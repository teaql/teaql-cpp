"""Record original public entity codec/Context behavior and malformed payloads."""
import argparse
import copy
import datetime
import itertools
import json
from pathlib import Path
import struct
import subprocess
from compare_values import observe

NOW = int(datetime.datetime(2026, 9, 26, 12, tzinfo=datetime.timezone.utc).timestamp())
KEYS = [[2, "22" * 32]]
GOLDEN = "tqr1.AAAAAjMzMzMzMzMzMzMzM3bKiZgRSQQhfIj2cBXRDZIloUGHWLBp8QrXL_aejwIXPFtvV_E71O7wbOXy3cvYo_SwxvuS-89x572T9CO_pDAY4tbjWCNv"


def case(**fields):
    value = dict(op="roundtrip", active=2, keys=KEYS, decode_active=2, decode_keys=KEYS,
                 now=[str(NOW), 0], consume_now=[str(NOW), 0], nonce="33" * 12,
                 entity="OrderItem", id="42", version="7", purpose="edit-order",
                 expected_entity="OrderItem", expected_purpose="edit-order", lifetime=["3600", 0],
                 context=False, installed=True)
    value.update(fields)
    return copy.deepcopy(value)


def payload(entity=b"OrderItem", identity=42, version=7, issued=NOW, expires=NOW+3600, purpose=b"edit-order"):
    def text(data):
        return struct.pack(">H", len(data)) + data
    return text(entity) + struct.pack(">Qqqq", identity, version, issued, expires) + text(purpose)


def inputs():
    cases = []
    for active, size in itertools.product([0, 1, 2, 2**32-1], [0, 1, 16, 31, 32, 33, 64]):
        cases.append(case(op="new", active=active, keys=[[2, "22" * size]]))
    for keys in [[], [[2, "11"*32], [2, "22"*32]], [[1, "11"*32], [2, "22"*32]],
                 [[2, "11"*31], [2, "22"*32]], [[2**32-1, "ff"*32]]]:
        cases.append(case(op="new", keys=keys))
    for context, installed, identity, version in itertools.product([False, True], [False, True],
            ["0", "1", "42", str(2**64-1)], [str(-2**63), "-1", "0", "7", str(2**63-1)]):
        cases.append(case(context=context, installed=installed, id=identity, version=version))
    for entity, purpose in itertools.product(["", " ", "\u2003", " OrderItem ", "OrderItem", "中😀\0"],
                                            ["", " ", "edit-order", "purpose\0😀"]):
        cases.append(case(entity=entity, purpose=purpose, expected_entity=entity, expected_purpose=purpose))
    for lifetime in [["0", 0], ["0", 1], ["0", 999999999], ["1", 0], ["1", 1],
                     ["3600", 0], [str(2**64-1), 0], ["9223372036854775", 807000000],
                     ["9223372036854775", 807000001], ["9223372036854776", 0],
                     ["0", 1000000000], ["1", 2**32-1], [str(2**64-1), 1000000000]]:
        cases.append(case(lifetime=lifetime))
    for nanos, duration_nanos, offset, consume_nanos in itertools.product([0, 1, 500000000, 999999999],
            [0, 1, 500000000, 999999999], [-1, 0, 1, 2], [0, 999999999]):
        cases.append(case(now=[str(NOW), nanos], lifetime=["1", duration_nanos], consume_now=[str(NOW+offset), consume_nanos]))
    # Chrono leap-second addition remains distinct from ordinary fractional carry.
    for seconds, nanos in itertools.product([0, 1, 60], [0, 1, 999999999]):
        cases.append(case(now=[str(NOW+59), 1500000000], consume_now=[str(NOW+59), 0], lifetime=[str(seconds), nanos]))
    for consume, nanos, entity, purpose in itertools.product([NOW-61, NOW-60, NOW-59, NOW, NOW+3599, NOW+3600],
                [0, 1], ["OrderItem", "Other"], ["edit-order", "other"]):
        cases.append(case(op="decode", token=GOLDEN, consume_now=[str(consume), nanos], expected_entity=entity, expected_purpose=purpose))
    for keys, active in [([[1,"11"*32],[2,"22"*32]],2), ([[1,"11"*32]],1), ([[2,"11"*32]],2),
                        ([[2,"22"*32],[2,"11"*32]],2), ([[2,"11"*32],[2,"22"*32]],2)]:
        cases.append(case(decode_keys=keys, decode_active=active))
    for token in ["", "tqr1.", "tqr1.A", "tqr1.AA", "tqr1.AAA", "tqr1.AAAA", "tqr2."+GOLDEN[5:],
                  " "+GOLDEN, GOLDEN+" ", GOLDEN+"=", GOLDEN+"\n", GOLDEN[:-1], GOLDEN+"A"]:
        cases.append(case(op="decode",token=token))
    for i in range(5, len(GOLDEN)):
        replacement = "A" if GOLDEN[i] != "A" else "B"
        cases.append(case(op="decode", token=GOLDEN[:i]+replacement+GOLDEN[i+1:]))
    good = payload()
    for key_version in [0, 2**32-1]:
        version_keys = [[key_version, "22"*32]]
        cases.append(case(active=key_version, keys=version_keys, decode_active=key_version, decode_keys=version_keys))
    cases.append(case(now=["-1", 999999999], consume_now=["-1", 999999999], lifetime=["1", 1]))
    malformed = [good[:i] for i in range(len(good))] + [good+b"x", payload(identity=0), payload(entity=b""),
        payload(entity=b"\xff"), payload(entity=b"\xc0\x80"), payload(purpose=b"\xed\xa0\x80"),
        payload(issued=2**63-1), payload(expires=-2**63), payload(issued=NOW+61), payload(expires=NOW),
        payload(entity=b" ", expires=NOW+100), payload(version=-2**63), payload(identity=2**64-1),
        payload(issued=NOW+60), payload(expires=NOW-1)]
    for plain in malformed:
        cases.append(case(op="forged", plaintext=plain.hex(), encryption_key="22"*32, header_version=2))
    # Known version relabeling with identical retained key material follows the
    # source envelope; the numeric header itself is not additional AAD.
    cases.append(case(op="forged", plaintext=good.hex(), encryption_key="22"*32, header_version=1,
                      decode_keys=[[1,"22"*32],[2,"22"*32]],decode_active=2))
    cases.append(case(op="forged", plaintext=payload(entity=b" ").hex(), encryption_key="22"*32,
                      header_version=2, expected_entity=" "))
    for field in ["entity", "purpose"]:
        for length in [65535, 65536]:
            fields = {field:"a"*length, "expected_"+field:"a"*length}
            cases.append(case(**fields))
    return cases


def main():
    parser=argparse.ArgumentParser();parser.add_argument("--rust",required=True);args=parser.parse_args()
    root=Path(__file__).resolve().parents[2]
    subprocess.run(["python3",str(root/"scripts/verify-upstream.py")],check=True)
    cases=inputs();outputs=observe(args.rust,cases)
    corpus=dict(baseline=json.loads((root/"upstream.lock.json").read_text())["commit"],
        note="Unchanged original public AeadEntityReferenceCodec and UserContext calls. Fixed key/clock/nonce fixtures retain exact wire tokens. Authenticated malformed plaintext uses only the already locked original aes-gcm primitive, never copied codec parsing. DateTime addition panics are normalized as panic=true, not converted to token errors. NIST vectors are a separate acceptance corpus.",
        cases=[dict(input=i,expected=o)for i,o in zip(cases,outputs)])
    Path(__file__).with_name("entity_reference_cases.json").write_text(json.dumps(corpus,indent=2)+"\n")
    print(f"Recorded {len(cases)} entity-reference observations")


if __name__=="__main__":main()
