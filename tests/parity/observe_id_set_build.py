#!/usr/bin/env python3
"""Record original Rust registry/mutex observations for valid lifetime scripts."""
import argparse
import json
import random
from pathlib import Path
from compare_values import observe


def case(seed):
    rng = random.Random(seed)
    keys = ["", "query", "query:0", "query::0", "查询/边缘", "a\0b"]
    handles, tasks, held = {}, {}, {}
    steps = []

    def emit(op, **values):
        steps.append(dict(op=op, **values))
        h, t = values.get("handle"), values.get("task")
        if op == "borrow":
            handles[h] = values["key"]
        elif op == "lock":
            tasks[t] = dict(handle=h, key=handles[h], state="pending", held=False)
        elif op == "poll":
            task = tasks[t]
            if task["state"] == "pending" and task["key"] not in held:
                task.update(state="ready", held=True)
                held[task["key"]] = t
        elif op == "cancel" and tasks[t]["state"] == "pending":
            tasks[t]["state"] = "cancelled"
        elif op == "release" and tasks[t]["held"]:
            tasks[t]["held"] = False
            del held[tasks[t]["key"]]
        elif op == "reset":
            del handles[h]

    # Every case begins with contention and a wake transferred on cancellation.
    key = keys[seed % len(keys)]
    for h in range(3):
        emit("borrow", handle=h, key=key)
    emit("same", handle=0, other=1)
    for t in range(3):
        emit("lock", handle=t, task=t)
        emit("poll", task=t)
    emit("release", task=0)
    emit("cancel", task=1)
    emit("poll", task=2)
    emit("release", task=2)

    for _ in range(35):
        choices = []
        if handles and len(tasks) < 12:
            choices.append("lock")
        if tasks:
            choices.extend(["cancel", "release"])
            if any(t["state"] != "cancelled" for t in tasks.values()):
                choices.append("poll")
        free = [h for h in range(6) if h not in handles]
        resettable = [h for h in handles if not any(
            t["handle"] == h and (t["held"] or t["state"] == "pending")
            for t in tasks.values())]
        if free:
            choices.append("borrow")
        if resettable:
            choices.append("reset")
        if len(handles) > 1:
            choices.append("same")
        op = rng.choice(choices)
        if op == "borrow":
            emit(op, handle=rng.choice(free), key=rng.choice(keys))
        elif op == "lock":
            emit(op, handle=rng.choice(list(handles)), task=len(tasks))
        elif op == "reset":
            emit(op, handle=rng.choice(resettable))
        elif op == "same":
            a, b = rng.sample(list(handles), 2)
            emit(op, handle=a, other=b)
        elif op == "poll":
            emit(op, task=rng.choice([id for id, t in tasks.items() if t["state"] != "cancelled"]))
        else:
            emit(op, task=rng.choice(list(tasks)))
    for id in tasks:
        emit("release", task=id)
        emit("cancel", task=id)
    for h in list(handles):
        emit("reset", handle=h)
    emit("borrow", handle=0, key=key)
    emit("borrow", handle=1, key=key)
    emit("same", handle=0, other=1)
    emit("lock", handle=0, task=len(tasks))
    emit("poll", task=len(tasks)-1)
    return {"steps": steps}


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--rust", required=True)
    args = parser.parse_args()
    root = Path(__file__).resolve().parents[2]
    inputs = [case(seed) for seed in range(144)]
    outputs = observe(args.rust, inputs)
    corpus = {"baseline": json.loads((root / "upstream.lock.json").read_text())["commit"],
              "cases": [dict(input=i, expected=o) for i, o in zip(inputs, outputs)]}
    Path(__file__).with_name("id_set_build_cases.json").write_text(json.dumps(corpus, ensure_ascii=True, separators=(",", ":")) + "\n")
    print(f"Recorded {len(inputs)} ID set build coordination scripts")


if __name__ == "__main__":
    main()
