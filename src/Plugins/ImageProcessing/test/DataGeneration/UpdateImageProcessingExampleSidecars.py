#!/usr/bin/env python3
"""Apply paper-input target metadata to ImageProcessing YAML and Markdown sidecars."""

from __future__ import annotations

import argparse
import json
from pathlib import Path
import re


BEGIN_MARKER = "<!-- INPUT-DATA-STRATEGY:BEGIN -->"
END_MARKER = "<!-- INPUT-DATA-STRATEGY:END -->"


def input_strategy(target: dict, seed: int) -> dict:
    strategy = {
        "strategy": target["strategy"],
        "asset_keys": target["asset_keys"],
        "paper_doi": target.get("paper_doi", ""),
        "paper_url": target.get("paper_url", ""),
        "reference_visual": target["reference_visual"],
        "target_traits": target["target_traits"],
        "known_differences": target["known_differences"],
        "archive_provenance_file": "Provenance.json",
    }
    if "synthetic" in target["strategy"]:
        strategy["generator"] = {
            "path": "Generators/GenerateImageProcessingExampleData.py",
            "seed": seed,
            "metrics_file": "PaperInputMetrics.json",
            "target_contract": "Generators/PaperInputTargets.json",
        }
    if target["strategy"] == "original-paper-linked":
        strategy.update(
            {
                "dataset_doi": target.get("dataset_doi", ""),
                "source_url": target["source_url"],
                "source_sha512": target["source_sha512"],
            }
        )
    return strategy


def update_dataset_metadata(citation: dict, target: dict) -> None:
    dataset = citation["dataset"]
    strategy = target["strategy"]
    if strategy == "original-paper-linked":
        if "nist" in target["source_url"].lower() or "s3.amazonaws.com/nist" in target["source_url"].lower():
            license_value = "NIST public data terms; review the dataset record for third-party restrictions."
        else:
            license_value = "MIT license in the paper-linked GitHub repository."
        dataset.update(
            {
                "availability": "Original paper-linked input data is publicly available and a measured-derived input is included in this archive.",
                "url": f"https://doi.org/{target['dataset_doi']}" if target.get("dataset_doi") else target["source_url"],
                "doi": target.get("dataset_doi", ""),
                "license": license_value,
                "included": True,
                "reproduction_notes": target["known_differences"],
            }
        )
    elif "synthetic" in strategy:
        dataset.update(
            {
                "availability": "The original paper input is unavailable or not redistributable. A deterministic paper-inspired synthetic input is included.",
                "included": False,
                "reproduction_notes": target["known_differences"],
            }
        )


def markdown_strategy_block(strategy: dict) -> str:
    lines = [
        BEGIN_MARKER,
        "### Input source strategy",
        "",
        f"Strategy: `{strategy['strategy']}`.",
        "",
        f"Paper visual target: {strategy['reference_visual']}",
        "",
        "Target traits:",
        "",
    ]
    lines.extend(f"- {trait}" for trait in strategy["target_traits"])
    if "generator" in strategy:
        generator = strategy["generator"]
        lines.extend(
            [
                "",
                f"Generator: `{generator['path']}` with fixed seed `{generator['seed']}`.",
                "",
                f"Generated metrics: `{generator['metrics_file']}`. Target contract: `{generator['target_contract']}`.",
            ]
        )
    if strategy.get("source_url"):
        lines.extend(
            [
                "",
                f"Original source: [{strategy['source_url']}]({strategy['source_url']}).",
                "",
                f"Original source SHA-512: `{strategy['source_sha512']}`.",
            ]
        )
    lines.extend(
        [
            "",
            f"Known differences: {strategy['known_differences']}",
            "",
            "See `Data/ImageProcessing_Examples/Provenance.json` for per-file transformations, checksums, dimensions, spacing, and licensing.",
            END_MARKER,
        ]
    )
    return "\n".join(lines)


def update_markdown(path: Path, strategy: dict, citation: dict) -> None:
    text = path.read_text(encoding="utf-8")
    block = markdown_strategy_block(strategy)
    if BEGIN_MARKER in text:
        text = re.sub(re.escape(BEGIN_MARKER) + r".*?" + re.escape(END_MARKER), block, text, flags=re.DOTALL)
    else:
        anchor = "The included files are compact test fixtures. See `Data/ImageProcessing_Examples/Provenance.json` for source, transformation, checksum, dimensions, spacing, and data classification."
        if anchor not in text:
            anchor = "## Data flow"
            text = text.replace(anchor, block + "\n\n" + anchor, 1)
        else:
            text = text.replace(anchor, block, 1)

    section_start = text.index("## Scientific basis and annotated references")
    guidance_start = text.index("## Guidance for an LLM or MCP assistant")
    section = text[section_start:guidance_start]
    reference_match = re.search(r"(?m)^1\. ", section)
    if reference_match is None:
        raise RuntimeError(f"{path}: cannot find first scientific reference")
    references = section[reference_match.start() :].rstrip()
    dataset = citation["dataset"]
    preface = [
        "## Scientific basis and annotated references",
        "",
        f"Workflow reproduction status: `{dataset['reproduction_status']}`.",
        "",
        dataset["reproduction_notes"],
        "",
        f"Paper dataset availability: {dataset['availability']}",
        "",
        f"Dataset license: {dataset['license']}",
        "",
        f"Original or paper-linked dataset included in the example archive: `{'yes' if dataset['included'] else 'no'}`.",
    ]
    if "synthetic" in strategy["strategy"]:
        preface.extend(["", "Paper-inspired synthetic fallback included in the example archive: `yes`."])
    if dataset.get("url"):
        preface.extend(["", f"Dataset record: [{dataset['url']}]({dataset['url']})"])
    preface.extend(["", references, "", ""])
    text = text[:section_start] + "\n".join(preface) + text[guidance_start:]
    path.write_text(text, encoding="utf-8")


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--suite-dir", type=Path, required=True)
    parser.add_argument("--targets", type=Path, required=True)
    args = parser.parse_args()

    targets = json.loads(args.targets.read_text(encoding="utf-8"))
    for pipeline_name, target in targets["pipelines"].items():
        example_dir = args.suite_dir / pipeline_name
        yaml_path = example_dir / f"{pipeline_name}.yaml"
        markdown_path = example_dir / f"{pipeline_name}.md"
        metadata = json.loads(yaml_path.read_text(encoding="utf-8"))
        strategy = input_strategy(target, targets["seed"])
        metadata["input_data_strategy"] = strategy
        metadata["last_verified"] = "2026-09-01"
        update_dataset_metadata(metadata["citations"][0], target)
        yaml_path.write_text(json.dumps(metadata, indent=2, ensure_ascii=False) + "\n", encoding="utf-8")
        update_markdown(markdown_path, strategy, metadata["citations"][0])


if __name__ == "__main__":
    main()
