"""Observe development checks when copied to an owned sitecustomize.py.

This records calls and subprocess boundaries without replacing gate results.
The collector must verify that each selected entry produced an observation.
"""

import ast
import atexit
from collections import Counter
import dis
import hashlib
import json
import marshal
import os
from pathlib import Path
import sys
import threading


ROOT = os.environ["CODEX_TEAQL_CHECK_OBSERVER_ROOT"]
OWNED = os.environ["CODEX_TEAQL_CHECK_OBSERVER_OWNED"]
DESTINATION = Path(os.environ["CODEX_TEAQL_CHECK_OBSERVER_LOGS"])
PREFIXES = (ROOT + "/scripts/", ROOT + "/tests/", OWNED + "/")
INITIAL_ARGV = list(sys.argv)
INITIAL_PARENT = os.getppid()
INITIAL_PROFILE = sys.getprofile()
INITIAL_TRACE = sys.gettrace()
CALL_MODE = os.environ.get("CODEX_TEAQL_CHECK_OBSERVER_CALL_MODE", "calls")
if CALL_MODE not in ("calls", "audit_only"):
    raise RuntimeError("Unknown observer call mode")
FILES = {}
CODES = {}
COMPILES = Counter()
EXECS = Counter()
DYNAMIC_FILENAMES = set()
DYNAMIC_ASSERTION_FILES = set()
NON_ASSERTION_FILES = set()
SPAWNS = []
ERRORS = []
AUDIT_STATE = threading.local()
FINISHING = False


def relevant(filename):
    return (filename.startswith(PREFIXES) or filename in DYNAMIC_FILENAMES
            or filename in ("<string>", "<stdin>"))


def file_info(filename):
    if filename not in FILES:
        path = Path(filename)
        value = {"path": filename, "sha256": None, "assertion_lines": []}
        if path.is_file() and path.suffix == ".py":
            source = path.read_bytes()
            value["sha256"] = hashlib.sha256(source).hexdigest()
            value["assertion_lines"] = sorted(
                node.lineno for node in ast.walk(ast.parse(source))
                if isinstance(node, ast.Assert))
        FILES[filename] = value
    return FILES[filename]


def code_info(code):
    if code not in CODES:
        source = file_info(code.co_filename)
        CODES[code] = {
            "path": code.co_filename, "name": code.co_name,
            "first_line": code.co_firstlineno,
            "code_sha256": hashlib.sha256(marshal.dumps(code)).hexdigest(),
            "assertion_opcodes": sum(
                instruction.opname == "LOAD_ASSERTION_ERROR"
                for instruction in dis.get_instructions(code)),
            "source_assertions": len(source["assertion_lines"]),
            "calls": 0, "assertion_line_hits": Counter(),
        }
    return CODES[code]


def trace(frame, event, argument):
    filename = frame.f_code.co_filename
    if filename in NON_ASSERTION_FILES or not relevant(filename):
        return None
    try:
        info = file_info(filename)
        if event == "call":
            code = code_info(frame.f_code)
            if not (info["assertion_lines"] or code["assertion_opcodes"]
                    or filename in DYNAMIC_ASSERTION_FILES):
                NON_ASSERTION_FILES.add(filename)
                return None
            code["calls"] += 1
        elif event == "line" and frame.f_lineno in info["assertion_lines"]:
            code_info(frame.f_code)["assertion_line_hits"][frame.f_lineno] += 1
    except Exception as error:
        ERRORS.append(type(error).__name__ + ": " + str(error))
        return None
    return trace


def audit(event, arguments):
    if getattr(AUDIT_STATE, "active", False) or FINISHING:
        return
    AUDIT_STATE.active = True
    try:
        if event == "subprocess.Popen":
            executable, command, cwd, environment = arguments
            selected = os.environ if environment is None else environment
            values = list(command) if isinstance(command, (list, tuple)) else [command]
            rendered = []
            for index, value in enumerate(values):
                if index and values[index - 1] == "-c":
                    rendered.append({"inline_source_sha256": hashlib.sha256(
                        str(value).encode()).hexdigest(), "characters": len(str(value))})
                else:
                    rendered.append(os.fsdecode(value))
            SPAWNS.append({"executable": os.fsdecode(executable), "command": rendered,
                           "cwd": None if cwd is None else os.fsdecode(cwd),
                           "python_optimize": selected.get("PYTHONOPTIMIZE"),
                           "observer_in_pythonpath": OWNED in str(selected.get("PYTHONPATH") or "")})
        elif event == "compile":
            source, filename = arguments
            frame = sys._getframe().f_back
            caller = frame.f_code.co_filename if frame is not None else ""
            if isinstance(filename, str) and (relevant(filename) or relevant(caller)):
                DYNAMIC_FILENAMES.add(filename)
                if isinstance(source, str):
                    source = source.encode()
                if isinstance(source, bytes):
                    COMPILES[(filename, hashlib.sha256(source).hexdigest(), "bytes")] += 1
                elif isinstance(source, ast.AST):
                    digest = hashlib.sha256(ast.dump(source).encode()).hexdigest()
                    COMPILES[(filename, digest, "ast_dump")] += 1
                try:
                    tree = source if isinstance(source, ast.AST) else ast.parse(source)
                except (SyntaxError, ValueError, TypeError):
                    tree = None
                if tree is not None and any(isinstance(node, ast.Assert) for node in ast.walk(tree)):
                    DYNAMIC_ASSERTION_FILES.add(filename)
                    NON_ASSERTION_FILES.discard(filename)
        elif event == "exec":
            code = arguments[0]
            frame = sys._getframe().f_back
            caller = frame.f_code.co_filename if frame is not None else ""
            if relevant(code.co_filename) or relevant(caller):
                DYNAMIC_FILENAMES.add(code.co_filename)
                EXECS[(code.co_filename, hashlib.sha256(marshal.dumps(code)).hexdigest())] += 1
    except Exception as error:
        ERRORS.append(type(error).__name__ + ": " + str(error))
    finally:
        AUDIT_STATE.active = False


def finish():
    global FINISHING
    FINISHING = True
    profile_retained = sys.getprofile() is INITIAL_PROFILE
    trace_retained = sys.gettrace() is (trace if CALL_MODE == "calls" else INITIAL_TRACE)
    if CALL_MODE == "calls" and trace_retained:
        sys.settrace(INITIAL_TRACE)
    rows = []
    for value in CODES.values():
        if value["calls"]:
            row = dict(value)
            row["assertion_line_hits"] = [
                {"line": line, "count": count}
                for line, count in sorted(value["assertion_line_hits"].items())]
            rows.append(row)
    value = {"pid": os.getpid(), "parent_pid": INITIAL_PARENT,
             "initial_argv": INITIAL_ARGV, "optimized": sys.flags.optimize,
             "executable": sys.executable, "profile_retained": profile_retained,
             "trace_retained": trace_retained, "profile_mode": "not_installed",
             "call_mode": CALL_MODE,
             "observer_sha256": hashlib.sha256(Path(__file__).read_bytes()).hexdigest(),
             "files": sorted(FILES.values(), key=lambda row: row["path"]),
             "called_assertion_file_code": sorted(rows, key=lambda row: (
                 row["path"], row["first_line"], row["name"], row["code_sha256"])),
             "compiles": [{"path": path, "source_sha256": digest, "kind": kind, "count": count}
                          for (path, digest, kind), count in sorted(COMPILES.items())],
             "execs": [{"path": path, "code_sha256": digest, "count": count}
                       for (path, digest), count in sorted(EXECS.items())],
             "subprocesses": SPAWNS, "observer_errors": ERRORS}
    (DESTINATION / (str(os.getpid()) + ".json")).write_text(json.dumps(value, sort_keys=True))


atexit.register(finish)
sys.addaudithook(audit)
if CALL_MODE == "calls":
    sys.settrace(trace)
