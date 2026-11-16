#!/usr/bin/env python3
"""Reproduce the complete 319+87 default SDK contracts before admitting new source."""

import argparse
import copy
import gzip
import hashlib
import json
from pathlib import Path
import signal
import sys
import tempfile

from run_native_integration_epoch_hardened_5_0_6 import capture_inputs, references
from verify_primitive_const_governed_execution_5_0_6 import run_owned

ROOT = Path(__file__).resolve().parents[1]
PRODUCER = ROOT.parent / "teaql_code_gen_cpp"
EVIDENCE = ROOT / "docs/parity"
SOURCES = (
    ("ap02_const_function_preserved_sdks_repaired_5_0_6.json.gz",
     "2fb8876f4d32efb94e8e1ff69801c59b3b4e8ff81b44d4b882d48c1bd1f6024e", "files", "reused_cpp_exit_code"),
    ("ap02_const_function_repaired_generated_5_0_6.json.gz",
     "e6d6873b686877e83290892d7a99606bc2ca8388bece23edf29e2aae28a444e5", "cpp_files", "cpp_exit_code"),
)
GENERATION = EVIDENCE / "compiler_projection_generation_5_0_6.json.gz"
BATCH = r'''
import com.fasterxml.jackson.databind.*;
import com.skynet.codegenerator.cpp.CppCoreGenerator;
import java.nio.charset.StandardCharsets;
import java.nio.file.*;
import java.security.MessageDigest;
import java.util.*;
import java.util.stream.Stream;

public final class PreserveDefaultSdks {
  public static void main(String[] args) throws Exception {
    ObjectMapper json = new ObjectMapper();
    Path owned = Path.of(args[1]), output = owned.resolve("generated"), model = owned.resolve("model.xml");
    int count = 0;
    for (String line : Files.readAllLines(Path.of(args[0]), StandardCharsets.UTF_8)) {
      JsonNode input = json.readTree(line);
      if (Files.exists(output)) {
        try (Stream<Path> paths = Files.walk(output)) {
          for (Path path : paths.sorted(Comparator.reverseOrder()).toList()) { Files.delete(path); }
        }
      }
      Files.writeString(model, input.get("model").textValue(), StandardCharsets.UTF_8);
      CppCoreGenerator.main(new String[] {model.toString(), output.toString()});
      Map<String, String> files = new TreeMap<>();
      try (Stream<Path> paths = Files.walk(output)) {
        for (Path path : paths.filter(Files::isRegularFile).toList()) {
          files.put(output.relativize(path).toString(), HexFormat.of().formatHex(
              MessageDigest.getInstance("SHA-256").digest(Files.readAllBytes(path))));
        }
      }
      Map<String, String> expected = json.convertValue(input.get("files"),
          new com.fasterxml.jackson.core.type.TypeReference<Map<String, String>>() {});
      Map<String, Object> result = new LinkedHashMap<>();
      result.put("id", input.get("id").textValue());
      result.put("match", files.equals(expected));
      result.put("files", files);
      if (!files.equals(expected)) {
        Set<String> names = new TreeSet<>(files.keySet()); names.addAll(expected.keySet());
        String changed = names.stream().filter(p -> !Objects.equals(files.get(p), expected.get(p))).findFirst().orElseThrow();
        result.put("first_changed_file", changed);
        if (Files.isRegularFile(output.resolve(changed))) {
          result.put("changed_source", Files.readString(output.resolve(changed)));
        }
      }
      System.out.println("CPP_SDK_FRAME " + json.writeValueAsString(result));
      if (!files.equals(expected)) { throw new IllegalStateException("SDK bytes changed: " + input.get("id")); }
      count++;
    }
    System.out.println("CPP_SDK_COUNT " + count);
  }
}
'''


def require(value, message):
    if not value:
        raise RuntimeError("Default SDK preservation: " + message)


def sha(data):
    return hashlib.sha256(data).hexdigest()


def read(path):
    return json.loads(gzip.decompress(path.read_bytes()))


def write(path, value):
    path.write_bytes(gzip.compress(json.dumps(value, sort_keys=True).encode(), mtime=0))


def cases():
    result = []
    for name, expected, files, outcome in SOURCES:
        require(sha((EVIDENCE / name).read_bytes()) == expected, "immutable complete preceding SDK corpus")
        source = read(EVIDENCE / name)
        require(source["complete"] is True, "completed prior compiler epoch")
        for index, row in enumerate(source["cases"]):
            result.append(dict(id=name + ":" + str(index), source_record=name, source_index=index,
                               name=row["name"], model=row["model"], files=row[files],
                               original_exit_code=row["original_exit_code"], cpp_exit_code=row[outcome]))
    require(len(result) == 406, "all 319 preceding and 87 installed function contracts")
    return result


def execute(path):
    require(not path.exists(), "preserve prior receipts")
    source = read(GENERATION)
    require(source["complete"] is True, "actual current explicit resolver generation")
    planned = cases()
    record = dict(complete=False, whole_goal_complete=False, actual_cpp_compiler_processes=0,
                  reused_cpp_compiler_outcomes=406, planned_cases=planned,
                  generation_sha256=sha(GENERATION.read_bytes()), driver_source=Path(__file__).read_text(),
                  batch_source=BATCH, input_hashes_before=capture_inputs(), references_before=references(), steps=[])

    def run(label, command):
        outcome = run_owned(command, cwd=ROOT, text=True, capture_output=True, timeout=900)
        record["steps"].append(dict(label=label, command=command, exit_code=outcome.returncode,
                                    stdout=outcome.stdout, stderr=outcome.stderr))
        write(path, record)
        print(label, "exit", outcome.returncode, flush=True)
        require(outcome.returncode == 0, "fresh Java generation failed; inspect retained result frame")
        return outcome

    try:
        with tempfile.TemporaryDirectory(prefix="teaql_preserve_compiler_projection_") as directory:
            owned = Path(directory)
            record["owned_directory"] = directory
            try:
                batch = owned / "PreserveDefaultSdks.java"
                batch.write_text(BATCH)
                model_input = owned / "cases.jsonl"
                model_input.write_text("".join(json.dumps(row, sort_keys=True) + "\n" for row in planned))
                command = next(s["command"] for s in source["steps"] if s["label"] == "cpp_javac")
                command = [s.replace(source["owned_directory"], directory) for s in command] + [str(batch)]
                run("current_javac", command)
                cp = next(s["command"][2] for s in source["steps"] if s["label"] == "complete_cpp_generate")
                cp = cp.replace(source["owned_directory"], directory)
                result = run("complete_sdk_batch", ["java", "-cp", cp, "PreserveDefaultSdks", str(model_input), directory])
                rows = [json.loads(line.removeprefix("CPP_SDK_FRAME ")) for line in result.stdout.splitlines()
                        if line.startswith("CPP_SDK_FRAME ")]
                record["cases"] = rows
                require(len(rows) == 406 and "CPP_SDK_COUNT 406" in result.stdout, "all actual current SDK generations completed")
            finally:
                record["owned_bytes_removed"] = sum(p.stat().st_size for p in owned.rglob("*") if p.is_file())
        record.update(complete=True, owned_directory_removed=not Path(record["owned_directory"]).exists(),
                      input_hashes_after=capture_inputs(), references_after=references())
        validate(record)
    except BaseException as error:
        record.update(complete=False, error=repr(error))
        raise
    finally:
        if "owned_directory" in record:
            record["owned_directory_removed"] = not Path(record["owned_directory"]).exists()
        write(path, record)


def validate(record):
    require(record["complete"] is True and record["whole_goal_complete"] is False
            and type(record["actual_cpp_compiler_processes"]) is int and record["actual_cpp_compiler_processes"] == 0
            and record["reused_cpp_compiler_outcomes"] == 406, "honest source-only compiler outcome reuse")
    require(record["planned_cases"] == cases() and len(record["cases"]) == 406,
            "complete locked SDK/compiler membership")
    require(record["generation_sha256"] == sha(GENERATION.read_bytes())
            and record["driver_source"] == Path(__file__).read_text() and record["batch_source"] == BATCH,
            "exact generation source and batch recipe")
    for actual, prior in zip(record["cases"], record["planned_cases"], strict=True):
        require(actual["id"] == prior["id"] and actual["match"] is True and actual["files"] == prior["files"],
                "all complete SDK bytes unchanged; actual old compiler result may be reused")
    require(record["owned_directory_removed"] is True
            and record["input_hashes_before"] == record["input_hashes_after"]
            and record["references_before"] == record["references_after"], "closed source frame and owned cleanup")
    require(len(record["steps"]) == 2 and [s["label"] for s in record["steps"]] == ["current_javac", "complete_sdk_batch"]
            and all(type(s["exit_code"]) is int and s["exit_code"] == 0 for s in record["steps"]),
            "actual current compiler and complete generation batch")
    frames = [json.loads(line.removeprefix("CPP_SDK_FRAME ")) for line in record["steps"][-1]["stdout"].splitlines()
              if line.startswith("CPP_SDK_FRAME ")]
    require(frames == record["cases"] and "CPP_SDK_COUNT 406" in record["steps"][-1]["stdout"], "actual batch frames and terminal count")


def controls(record):
    rejected = []
    for name in ("incomplete", "missing_sdk", "wrong_sdk_bytes", "invented_compilation", "missing_process", "boolean_exit", "missing_batch_frame", "cleanup"):
        changed = copy.deepcopy(record)
        if name == "incomplete": changed["complete"] = False
        elif name == "missing_sdk": changed["cases"].pop()
        elif name == "wrong_sdk_bytes": changed["cases"][0]["files"][next(iter(changed["cases"][0]["files"]))] = "0" * 64
        elif name == "invented_compilation": changed["actual_cpp_compiler_processes"] = 406
        elif name == "missing_process": changed["steps"].pop()
        elif name == "boolean_exit": changed["steps"][0]["exit_code"] = False
        elif name == "missing_batch_frame": changed["steps"][-1]["stdout"] = changed["steps"][-1]["stdout"].replace("CPP_SDK_FRAME ", "MISSING ", 1)
        elif name == "cleanup": changed["owned_directory_removed"] = False
        try:
            validate(changed)
        except RuntimeError as error:
            rejected.append(dict(case=name, rejected=True, error=str(error)))
        else:
            raise RuntimeError("Accepted default SDK corruption: " + name)
    return rejected


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    action = parser.add_mutually_exclusive_group(required=True)
    action.add_argument("--execute", action="store_true")
    action.add_argument("--check", action="store_true")
    parser.add_argument("--record", type=Path, required=True)
    parser.add_argument("--controls", type=Path)
    args = parser.parse_args()
    if args.execute:
        def stop(signum, frame):
            raise KeyboardInterrupt("Reap only this SDK preservation child")
        signal.signal(signal.SIGTERM, stop)
        execute(args.record.resolve())
    else:
        record = read(args.record)
        validate(record)
        rejected = controls(record)
        if args.controls:
            require(not args.controls.exists(), "preserve prior controls")
            write(args.controls, dict(complete=True, whole_goal_complete=False, optimized=sys.flags.optimize,
                  execution_sha256=sha(args.record.read_bytes()), controls=rejected))
        print("Default SDK preservation: 406 complete SDKs;", len(rejected), "corruptions rejected; optimized=", sys.flags.optimize)


if __name__ == "__main__":
    main()
