#!/usr/bin/env python3

import json
import os
from pathlib import Path
import subprocess
import sys
import tempfile

import imageprocessing  # Registers ImageProcessing filters before pipeline loading.
import simplnx as nx
import simplnx_utilities


PLUGIN_DIR = Path(__file__).resolve().parent.parent
PIPELINE_DIR = PLUGIN_DIR / "pipelines"


def collect_runtime_paths(value):
    paths = []
    if isinstance(value, dict):
        for child in value.values():
            paths.extend(collect_runtime_paths(child))
    elif isinstance(value, list):
        for child in value:
            paths.extend(collect_runtime_paths(child))
    elif isinstance(value, str) and value.startswith("Data/"):
        paths.append(value)
    return paths


def verify_generated_structure(pipeline_json, code, pipeline_path):
    search_start = 0
    for step_index, step in enumerate(pipeline_json["pipeline"]):
        filter_name = step["filter"]["name"].split("::")[-1]
        marker = f"{filter_name}.execute("
        position = code.find(marker, search_start)
        if position < 0:
            raise RuntimeError(f"{pipeline_path}: generated code is missing step {step_index} ({filter_name})")
        search_start = position + len(marker)

    missing_paths = sorted({path for path in collect_runtime_paths(pipeline_json) if path not in code})
    if missing_paths:
        raise RuntimeError(f"{pipeline_path}: generated code is missing runtime paths: {missing_paths}")


def main():
    specification = json.loads((PIPELINE_DIR / "PythonGeneration.yaml").read_text(encoding="utf-8"))
    if specification["converter_api"] != "simplnx_utilities.generate_python_pipeline":
        raise RuntimeError("PythonGeneration.yaml names an unsupported converter API")
    if specification["output_lifetime"] != "temporary":
        raise RuntimeError("PythonGeneration.yaml must require temporary generated programs")

    pipeline_paths = sorted(PIPELINE_DIR.glob("([0-9][0-9]) */*.d3dpipeline"))
    if len(pipeline_paths) != 16:
        raise RuntimeError(f"Expected 16 example pipelines, found {len(pipeline_paths)}")

    environment = os.environ.copy()
    with tempfile.TemporaryDirectory(prefix="imageprocessing-python-") as temporary_directory:
        temporary_path = Path(temporary_directory)
        for pipeline_path in pipeline_paths:
            pipeline_json = json.loads(pipeline_path.read_text(encoding="utf-8"))
            pipeline = nx.Pipeline.from_file(pipeline_path)
            code = simplnx_utilities.generate_python_pipeline(pipeline)
            compile(code, str(pipeline_path), "exec")
            verify_generated_structure(pipeline_json, code, pipeline_path)

            generated_path = temporary_path / f"{pipeline_path.stem}.py"
            generated_path.write_text(code, encoding="utf-8")
            result = subprocess.run(
                [sys.executable, str(generated_path)],
                cwd=Path.cwd(),
                env=environment,
                capture_output=True,
                text=True,
                timeout=1500,
                check=False,
            )
            if result.returncode != 0:
                raise RuntimeError(
                    f"Generated program failed for {pipeline_path}\n"
                    f"Exit code: {result.returncode}\n"
                    f"Standard output:\n{result.stdout}\n"
                    f"Standard error:\n{result.stderr}"
                )
            print(f"PASS: {pipeline_path.name}")


if __name__ == "__main__":
    main()
