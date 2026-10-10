"""Record original Context locking calls, normalizing only opaque owner identity."""
import argparse
import itertools
import json
from pathlib import Path
import subprocess
from compare_values import observe

def step(op, user=0, key="q", timeout=0, expiry=0, provider=0, millis=0):
    return dict(op=op,user=user,key=key,timeout=timeout,expiry=expiry,provider=provider,millis=millis)

def inputs():
    cases=[]
    for key, first, second, expiry in itertools.product(["", "q", "long key / nested", "锁\0key"],range(3),range(3),[0,2**64-1]):
        cases.append(dict(steps=[step("local",first,key,expiry=expiry),step("local",second,key),
            step("unlock_local",second,key),step("local",first,key),step("unlock_local",first,key),step("local",second,key)]))
    for key, first, second, provider, timeout, expiry in itertools.product(
            ["", "q", "unicode 锁\0key"],range(2),range(2),range(2),[0,37],[0,41,2**64-1]):
        cases.append(dict(steps=[step("remote",first,key,timeout,expiry),step("unlock_remote",second,key),
            step("provider",first,provider=provider),step("provider",second,provider=provider),
            step("remote",first,key,timeout,expiry),step("identifiers",first),step("remote",first,key,timeout,expiry),
            step("remote",second,key,timeout,expiry),step("unlock_remote",second,key),step("unlock_remote",first,key),
            step("remote",second,key,timeout,expiry)]))
    for provider in range(2):
        cases.append(dict(steps=[step("provider",provider=provider),step("remote"),step("close"),step("new"),
            step("provider",provider=provider),step("unlock_remote"),step("remote"),step("provider",provider=1-provider),step("remote")]))
    cases.append(dict(steps=[step("local"),step("close"),step("new"),step("unlock_local"),step("local")]))
    cases.append(dict(steps=[step("local",expiry=10),step("sleep",millis=40),step("local",1),step("unlock_local"),step("local")]))
    cases.append(dict(steps=[step("local",expiry=10),step("local",1,timeout=1000),step("unlock_local"),step("local")]))
    cases.append(dict(steps=[step("local",expiry=1),step("local",expiry=0),step("sleep",millis=20),step("local",1)]))
    return cases

def main():
    parser=argparse.ArgumentParser();parser.add_argument("--rust",required=True);args=parser.parse_args()
    root=Path(__file__).resolve().parents[2]
    subprocess.run(["python3",str(root/"scripts/verify_upstream.py")],check=True)
    cases=inputs();outputs=observe(args.rust,cases)
    lock=json.loads((root/"upstream.lock.json").read_text())
    corpus=dict(baseline=lock["commit"],note="Direct unchanged public UserContext local/remote methods. Provider fixtures only. Opaque owner tokens are normalized by first-seen identity across providers, preserving equality, replacement and recreation. Rust local keys receive a unique script prefix to isolate process-static entries; C++ uses an explicit Global-owned application directory.",cases=[dict(input=i,expected=o)for i,o in zip(cases,outputs)])
    Path(__file__).with_name("locking_cases.json").write_text(json.dumps(corpus,indent=2)+"\n")
    print(f"Recorded {len(cases)} Context locking scripts")
if __name__=="__main__":main()
