"""Observe unchanged original public round-trip reference APIs and SPI order."""
import argparse
import copy
import datetime
import itertools
import json
from pathlib import Path
import subprocess
import struct
from compare_values import observe

ACK = "I_UNDERSTAND_THIS_EXPOSES_INTERNAL_ENTITY_IDS_FOR_LOCAL_DEBUGGING_ONLY"
NOW = int(datetime.datetime(2026, 9, 27, tzinfo=datetime.timezone.utc).timestamp())
GOLDEN = "tqr1.k2.MzMzMzMzMzMzMzMzmhCET15vBdSc1BbFJcrvMMnRP1wAyjunShYlFPnDZL7Gk5V1LuptWyrbiQ8LQSDHFUmN3o6vb-MEgXZuh3N-YkeO55CYLeVgiyBBYslzuSu-fvWC2KMZcnZ5D8BRmWpGYA2Ok5sWe1Fg1aIyadTjVoKB-H2vPW_0okj-"


def case(**fields):
    out = dict(op="roundtrip", profile=1, ack=None, governed=False,
        service="order-service", environment="test-a", current="k2",
        keys=[["k1", "11"*32], ["k2", "22"*32]], key="22"*32,
        config_error="none", issue_error="none", lookup_error="none", lookup="k1",
        principal=["oidc", "alice", "Platform", "7"],
        scope=["doc-100", "edit-order", "Order", "100", "9"],
        identity=["OrderItem", "42", "3"],
        consume_principal=["oidc", "alice", "Platform", "7"],
        consume_scope=["doc-100", "edit-order", "Order", "100", "9"], expected_entity="OrderItem",
        lifetime=["600", 0], time=[str(NOW), 0], consume_time=[str(NOW), 0], nonce="33"*12,
        issue_allowed=True, consume_allowed=True, context=False, installed=True, principal_installed=True,
        raw=False, reference={"token":GOLDEN}, wire=GOLDEN)
    out.update(fields)
    return copy.deepcopy(out)


def inputs():
    cases=[]
    for profile,ack,governed in itertools.product(range(3),[None,"",ACK," "+ACK,ACK+" ",ACK.lower(),"near"],[False,True]):
        cases.append(case(op="config",profile=profile,ack=ack,governed=governed))
    texts=["", " ", " leading", "trailing ", "\u2003", "a\0b", "中😀", "a"*4096, "a"*4097]
    for name in ["service","environment"]:
        for value in texts:
            for ack in [None,ACK]: cases.append(case(op="config",ack=ack,**{name:value}))
    for model,field,indices in [("principal","principal",range(3)),("scope","scope",range(3)),("identity","identity",[0])]:
        for index,value in itertools.product(indices,texts):
            value_case=case(op="model",model=model);value_case[field][index]=value;cases.append(value_case)
    for identity in ["0","1",str(2**64-1)]:
        for field,index in [("principal",3),("scope",3),("identity",1)]:
            c=case(op="model",model=field);c[field][index]=identity;cases.append(c)
    for number in [str(-2**63),"-1","0",str(2**63-1)]:
        for field,index in [("scope",4),("identity",2)]:
            c=case(op="model",model=field);c[field][index]=number;cases.append(c)
    for name,size in itertools.product(["", "a", "k2", "a"*64,"a"*65,"-", "_", "a.b", "a+b","中"," k2"],[0,31,32,33]):
        cases.append(case(op="model",model="key",current=name,key="22"*size))
    for current,lookup in itertools.product(["k1","k2","missing","bad.key"],["k1","k2","missing"]):
        cases.append(case(op="provider",current=current,lookup=lookup))
    for keys in [[],[["k2","22"*32],["k2","11"*32]],[["bad.key","22"*32]],[["k2","22"*31]]]:
        cases.append(case(op="provider",keys=keys))
    for mode,wire in itertools.product([False,True],[None,False,1,1.0,"", "tqr1.", "tqr1.invalid", "tqr2.x", {},
            {"id":42,"version":3},{"id":42,"version":-1},{"id":0,"version":0},{"id":-1,"version":3},
            {"id":42.0,"version":3},{"id":42,"version":3.0},{"id":42,"version":2**63},
            {"id":2**64-1,"version":2**63-1},{"id":42,"version":3,"extra":False},{"id":"42","version":3}]):
        cases.append(case(op="wire",raw=mode,wire=wire))
    for ack,context,installed,trusted,op in itertools.product([None,ACK],[False,True],[False,True],[False,True],
                                                              ["roundtrip","mode","notice","serialize","deserialize"]):
        for raw in [False,True]:
            cases.append(case(op=op,ack=ack,context=context,installed=installed,principal_installed=trusted,
                reference={"id":"42","version":"3"} if raw else {"token":GOLDEN},
                wire={"id":42,"version":3} if raw else GOLDEN))
    for ack,issue,consume in itertools.product([None,ACK],[False,True],[False,True]):
        cases.append(case(ack=ack,issue_allowed=issue,consume_allowed=consume))
    for name in ["config_error","issue_error","lookup_error"]:
        for error in ["invalid","expired","scope","auth","config"]:
            for ack in [None,ACK]:cases.append(case(ack=ack,**{name:error}))
    for name,indices,values in [("principal",range(3),["", " bob", "x", "bob"]),
                                ("scope",range(3),["", " x", "x"]),
                                ("identity",[0],["", " Item", "Item", "OrderItem"])]:
        for index,value,ack in itertools.product(indices,values,[None,ACK]):
            c=case(ack=ack);c[name][index]=value;cases.append(c)
    for name,index,values in [("principal",3,["0","8",str(2**64-1)]),
                             ("scope",3,["0","101",str(2**64-1)]),
                             ("scope",4,["-1","0","10",str(2**63-1)]),
                             ("identity",1,["0","1",str(2**64-1)]),
                             ("identity",2,["-1","0",str(2**63-1)])]:
        for value,ack in itertools.product(values,[None,ACK]):
            c=case(ack=ack);c[name][index]=value;cases.append(c)
    for name,indices in [("consume_principal",range(4)),("consume_scope",range(5))]:
        for index,ack in itertools.product(indices,[None,ACK]):
            c=case(ack=ack);c[name][index]="8" if (name=="consume_principal" and index==3) or (name=="consume_scope" and index>=3) else "other";cases.append(c)
    for lifetime,ack in itertools.product([["0",0],["0",1],["0",999999999],["0",1000000000],
            ["1",0],["600",0],["9223372036854775",807000000],["9223372036854775",807000001],
            [str(2**64-1),0],[str(2**64-1),1000000000]],[None,ACK]):
        cases.append(case(ack=ack,lifetime=lifetime))
    for delta,nanos in itertools.product([-61,-60,-59,0,599,600,601],[0,999999999]):
        cases.append(case(op="consume",consume_time=[str(NOW+delta),nanos]))
    for initial,nanos in itertools.product([0,500000000,999999999],[0,500000000,999999999]):
        cases.append(case(time=[str(NOW),initial],lifetime=["1",nanos],consume_time=[str(NOW+1),0]))
    cases.append(case(time=[str(NOW+59),1500000000],lifetime=["1",999999999],consume_time=[str(NOW+59),0]))
    for expected,ack in itertools.product(["", " Other", "Other", "OrderItem"],[None,ACK]):
        cases.append(case(ack=ack,expected_entity=expected))
    for token in ["", "tqr1.", "tqr1.k2.", "tqr1.k2.A", "tqr1.k2.AAAA", "tqr1.k3.AAAA", "tqr1.bad.key.x", "tqr2.k2.x", GOLDEN+"=", GOLDEN+" ", " "+GOLDEN, GOLDEN[:-1], GOLDEN+".x", "tqr1."+"a"*24000]:
        cases.append(case(op="consume",reference={"token":token}))
    for pos in range(8,len(GOLDEN)):
        replacement="A" if GOLDEN[pos]!="A" else "B"
        cases.append(case(op="consume",reference={"token":GOLDEN[:pos]+replacement+GOLDEN[pos+1:]}))
    for current in ["k1","k2"]:cases.append(case(current=current))
    cases.append(case(op="consume",keys=[["k2","22"*32]],reference={"token":GOLDEN.replace(".k2.",".k1.")}))
    for service,environment in [("other-service","test-a"),("order-service","prod")]:
        cases.append(case(op="consume",service=service,environment=environment))
    for raw,ack in itertools.product([{"id":"0","version":"3"},{"id":"42","version":"-1"},
            {"id":"42","version":"3"},{"id":"1","version":"3"}],[None,ACK]):
        cases.append(case(op="consume",ack=ack,reference=raw))
    return cases


def main():
    parser=argparse.ArgumentParser();parser.add_argument("--rust",required=True);args=parser.parse_args()
    root=Path(__file__).resolve().parents[2]
    subprocess.run(["python3",str(root/"scripts/verify_upstream.py")],check=True)
    inputs_=inputs()
    old=observe(args.rust,[case(op="issue",current="k1",nonce="44"*12)])[0]["reference"]
    inputs_.extend([case(op="consume",reference=old),
                    case(op="consume",reference=old,keys=[["k2","22"*32]])])
    # Mutate only fixture data decrypted from the unchanged public golden token.
    # Reauthenticate with the already-locked Rust primitives; C++ consumes these
    # tokens through its real parser without a duplicate parser in the fixture.
    plaintext=bytes.fromhex(observe(args.rust,[dict(op="fixture_decrypt",token=GOLDEN)])[0]["plaintext"])
    malformed=[plaintext[:n] for n in range(len(plaintext))]+[plaintext+b"x"]
    for offset,length,value in [(0,1,b"\xff"),(34,1,b"\xff"),
                               (43,8,struct.pack(">Q",0)),(51,8,struct.pack(">q",-1)),
                               (83,8,struct.pack(">q",-1)),(91,8,struct.pack(">q",NOW+61)),
                               (99,8,struct.pack(">q",NOW)),(91,8,struct.pack(">q",-2**63)),
                               (99,8,struct.pack(">q",2**63-1))]:
        malformed.append(plaintext[:offset]+value+plaintext[offset+length:])
    both=bytearray(plaintext);both[91:99]=struct.pack(">q",-2**63);both[99:107]=struct.pack(">q",2**63-1);malformed.append(bytes(both))
    tokens=observe(args.rust,[dict(op="fixture_encrypt",plaintext=p.hex())for p in malformed])
    inputs_.extend(case(op="consume",reference=t["reference"])for t in tokens)
    outputs=observe(args.rust,inputs_)
    corpus=dict(baseline=json.loads((root/"upstream.lock.json").read_text())["commit"],
        note="Direct unchanged public models, key provider, wire codec, context-bound runtime and UserContext calls. SPI fixtures record complete authorization arguments plus key/clock/nonce order. Mutable clock and authorization fixtures preserve return-time rechecks without copied orchestration. Governed ignores environment acknowledgement; process-environment configuration records original raw notices. Crypto wire tokens are compared exactly. Authenticated malformed payloads mutate fixture bytes decrypted from the original golden token and are reauthenticated with existing Rust primitives; no codec parsing is copied.",
        cases=[dict(input=i,expected=o)for i,o in zip(inputs_,outputs)])
    Path(__file__).with_name("round_trip_reference_cases.json").write_text(json.dumps(corpus,indent=2)+"\n")
    print(f"Recorded {len(inputs_)} original round-trip reference observations")


if __name__=="__main__":main()
