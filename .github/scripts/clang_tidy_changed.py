"""Run the repository clang-tidy policy on the C++ files changed by a PR."""

import argparse
from concurrent.futures import ThreadPoolExecutor
import json
import os
from pathlib import Path
import re
import shlex
import subprocess

UNITS = {".c", ".cc", ".cpp", ".cxx", ".c++", ".C", ".ixx", ".cppm"}
HEADERS = {".h", ".hh", ".hpp", ".hxx", ".h++", ".inl", ".ipp", ".tpp", ".H"}
SPLIT_MAKE = re.compile(r"(?:\\[^\n]|[^\s\\])+")

# P0 runs while the bundled third-party files still use their original paths.
# Keep these paths aligned with maps/moves.tsv in the cleanup tool.
LEGACY_VENDOR = tuple(map(Path, (
    "src/simplnx/3rdParty/canvas_ity",
    "src/Plugins/NXImageProcessing/src/NXImageProcessing/oless",
    "src/Plugins/NXImageProcessing/src/NXImageProcessing/utils/nifti1.h",
    "src/Plugins/OrientationAnalysis/src/OrientationAnalysis/utilities/inicpp.h",
    "src/Plugins/SimplnxCore/src/SimplnxCore/utils/nanoflann.hpp",
    "src/simplnx/Utilities/RTree.hpp",
    "src/simplnx/Utilities/MD5.hpp",
    "src/simplnx/Utilities/MD5.cpp",
)))


def read_changed(root, base):
    names = subprocess.check_output(["git", "diff", "--name-only", "--diff-filter=ACMR", "-z",
                                     f"{base}...HEAD", "--"])
    roots = {"src", "test", "wrapping", "utilities", "cmake"}
    changed = set()
    for name in names.split(b"\0"):
        path = Path(os.fsdecode(name))
        if (path.parts and path.parts[0] in roots and path.suffix in UNITS | HEADERS
                and not any(part.lower() == "3rdparty" for part in path.parts)
                and not any(path.is_relative_to(vendor) for vendor in LEGACY_VENDOR)):
            resolved = (root / path).resolve()
            if resolved.is_relative_to(root) and resolved.is_file():
                changed.add(resolved)
    return changed


def depend(entry):
    arguments = iter(entry.get("arguments") or shlex.split(entry["command"]))
    command = [next(arguments)]
    for arg in arguments:
        if arg in ("-o", "-MF", "-MT", "-MQ", "-MJ", "--serialize-diagnostics"):
            next(arguments)
        elif arg not in ("-c", "-MD", "-MMD", "-MP") and not arg.startswith(("-MF", "-MT", "-MQ", "-MJ")):
            command.append(arg)
    directory = Path(entry["directory"])
    output = subprocess.check_output([*command, "-M", "-MT", "clang-tidy", "-MF", "-"],
                                     cwd=directory, text=True)
    if not output.startswith("clang-tidy:"):
        raise ValueError(f"Unexpected dependency output for {entry['file']}")
    names = SPLIT_MAKE.findall(output.removeprefix("clang-tidy:").replace("\\\n", " "))
    return {(directory / re.sub(r"\\(.)", r"\1", name).replace("$$", "$")).resolve() for name in names}


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--database", type=Path, required=True)
    parser.add_argument("--base", required=True)
    args = parser.parse_args()
    root = Path.cwd()
    version = subprocess.check_output(["clang-tidy", "--version"], text=True)
    if not re.search(r"\bversion 19\.1\.1\b(?!\.)", version):
        raise SystemExit(f"Expected clang-tidy 19.1.1, got {version.strip()}")
    changed = read_changed(root, args.base)
    if not changed:
        print("No changed C++ files.")
        return 0
    database = json.loads((args.database / "compile_commands.json").read_text())
    headers = {path for path in changed if path.suffix in HEADERS}
    matches = changed - headers
    covered = set()
    units = {}
    workers = min(8, os.cpu_count() or 1)
    with ThreadPoolExecutor(max_workers=workers) as pool:
        deps = pool.map(depend, database) if headers else (set() for _ in database)
        for command, includes in zip(database, deps):
            unit = (Path(command["directory"]) / command["file"]).resolve()
            affected = includes & headers
            if unit in matches or affected:
                relevant = affected | ({unit} & matches)
                units.setdefault(unit, set()).update(relevant)
                covered.update(relevant)
        if missing := changed - covered:
            # Expose files outside the Linux preset without inventing compilation flags.
            for path in sorted(missing):
                print(f"::notice file={path.relative_to(root)}::No compile command or includer in the Linux preset")
        print(f"Checking {len(covered)} changed files via {len(units)} translation units.")
        if not units:
            return 0
        tidy = ["clang-tidy", f"-p={args.database}", "--use-color=false"]

        def check(unit):
            line_filter = json.dumps([{"name": str(path)} for path in sorted(units[unit])])
            return subprocess.run([*tidy, f"--line-filter={line_filter}", str(unit)],
                                  capture_output=True, text=True)

        results = pool.map(check, sorted(units))
        failed = False
        for result in results:
            print(result.stdout + result.stderr, end="")
            failed |= bool(result.returncode)
    return int(failed)


if __name__ == "__main__":
    raise SystemExit(main())
