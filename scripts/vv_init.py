#!/usr/bin/env python3
"""Scaffold V&V working files for a DREAM3D-NX filter.

Creates the following from the v2 templates under docs/vv_templates/:

  src/Plugins/<PluginName>/vv/<FilterName>.md
  src/Plugins/<PluginName>/vv/deviations/<FilterName>.md

Provenance sidecars are created per exemplar archive on demand and are not
scaffolded by this script.

Usage:
  python scripts/vv_init.py <FilterName> [--plugin <PluginName>] [--force]

If --plugin is omitted, the script searches src/Plugins/*/src/*/Filters for
the matching <FilterName>.cpp and uses that plugin. The SIMPLNX UUID is
extracted from the .cpp file when present.
"""

from __future__ import annotations

import argparse
import re
import sys
from datetime import date
from pathlib import Path


REPO_ROOT = Path(__file__).resolve().parent.parent
TEMPLATE_DIR = REPO_ROOT / "docs" / "vv_templates"
PLUGINS_DIR = REPO_ROOT / "src" / "Plugins"


def find_filter_source(filter_name: str, plugin_hint: str | None) -> tuple[Path, Path]:
  """Return (plugin_dir, filter_cpp_path) for the named filter."""
  candidates: list[Path] = []
  if plugin_hint:
    plugin_dir = PLUGINS_DIR / plugin_hint
    if not plugin_dir.is_dir():
      sys.exit(f"Plugin directory not found: {plugin_dir}")
    candidates = list(plugin_dir.rglob(f"Filters/{filter_name}.cpp"))
  else:
    candidates = list(PLUGINS_DIR.glob(f"*/src/*/Filters/{filter_name}.cpp"))

  if not candidates:
    sys.exit(
      f"Could not locate source file for {filter_name}.cpp under "
      f"{PLUGINS_DIR}. Pass --plugin <PluginName> if the filter is in an "
      f"unusual location."
    )
  if len(candidates) > 1:
    paths = "\n  ".join(str(p) for p in candidates)
    sys.exit(f"Multiple candidates for {filter_name}.cpp:\n  {paths}\nPass --plugin to disambiguate.")

  filter_cpp = candidates[0]
  # plugin_dir is src/Plugins/<PluginName>/ — three parents up from src/<PluginName>/Filters/<File>.cpp
  plugin_dir = filter_cpp.parents[3]
  return plugin_dir, filter_cpp


def extract_uuid(filter_cpp: Path) -> str:
  """Pull the SIMPLNX UUID out of the filter .cpp by regex. Returns '<uuid>' if not found."""
  text = filter_cpp.read_text(encoding="utf-8", errors="replace")
  # Match the canonical Uuid::FromString("...") literal used by simplnx filters.
  match = re.search(r'SIMPLNX_DEF_FILTER_TRAITS\(.*, .*, "([0-9a-fA-F\-]{36})"\);', text)
  if match:
    return match.group(1)
  return "<uuid>"


def render(template_path: Path, replacements: dict[str, str]) -> str:
  text = template_path.read_text(encoding="utf-8")
  for key, value in replacements.items():
    text = text.replace(key, value)
  return text


def write_if_absent(target: Path, content: str, force: bool) -> None:
  if target.exists() and not force:
    print(f"  exists, skipped: {target.relative_to(REPO_ROOT)}")
    return
  target.parent.mkdir(parents=True, exist_ok=True)
  target.write_text(content, encoding="utf-8")
  verb = "overwrote" if target.exists() and force else "wrote"
  print(f"  {verb}: {target.relative_to(REPO_ROOT)}")


def main() -> None:
  parser = argparse.ArgumentParser(description="Scaffold V&V working files for a filter.")
  parser.add_argument("filter_name", help="Filter class name, e.g., ComputeGroupingDensityFilter")
  parser.add_argument("--plugin", help="Plugin name (auto-detected from source layout if omitted)")
  parser.add_argument("--force", action="store_true", help="Overwrite existing files")
  args = parser.parse_args()

  plugin_dir, filter_cpp = find_filter_source(args.filter_name, args.plugin)
  plugin_name = plugin_dir.name
  uuid_value = extract_uuid(filter_cpp.with_suffix('.hpp'))
  today = date.today().isoformat()

  report_template = TEMPLATE_DIR / "report_template.md"
  deviation_template = TEMPLATE_DIR / "deviation_template.md"
  for path in (report_template, deviation_template):
    if not path.is_file():
      sys.exit(f"Missing template: {path}")

  replacements = {
    "<FilterName>": args.filter_name,
    "<PluginName>": plugin_name,
    "<uuid>": uuid_value,
  }

  report_target = plugin_dir / "vv" / f"{args.filter_name}.md"
  deviation_target = plugin_dir / "vv" / "deviations" / f"{args.filter_name}.md"

  print(f"Filter:  {args.filter_name}")
  print(f"Plugin:  {plugin_name}")
  print(f"UUID:    {uuid_value}")
  print(f"Date:    {today}")
  print(f"Source:  {filter_cpp.relative_to(REPO_ROOT)}")
  print()
  print("Writing:")
  write_if_absent(report_target, render(report_template, replacements), args.force)
  write_if_absent(deviation_target, render(deviation_template, replacements), args.force)

  # Ensure the provenance dir exists so the engineer has a place to land sidecars.
  provenance_dir = plugin_dir / "vv" / "provenance"
  provenance_dir.mkdir(parents=True, exist_ok=True)

  print()
  print("Next steps:")
  print(f"  1. Open {report_target.relative_to(REPO_ROOT)} and fill in the header.")
  print(f"  2. Open {TEMPLATE_DIR.relative_to(REPO_ROOT) / 'report_gates.md'} alongside it.")
  print(f"  3. Decide the oracle class (the one ordering rule).")
  print(f"  4. As exemplar archives are touched, copy provenance_template.md into")
  print(f"     {provenance_dir.relative_to(REPO_ROOT)}/<archive>.md")


if __name__ == "__main__":
  main()
