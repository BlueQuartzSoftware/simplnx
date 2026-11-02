# ImageProcessing OOC Baseline Environment

- Capture started: `2026-08-18T14:55:00-0400`
- Campaign baseline: pre-optimization, 32M target voxels, Z=128, warmup=1, repeats=3.
- Task 1 infrastructure commits (recorded explicitly; no ImageProcessing algorithm optimization):
  - simplnx `49dd11bf4b6e22031f4e5d211ed3c062c30ac761` — `TEST: Relocate watershed OOC warning tests`
  - SimplnxOoc `aa3c57b543672a22bd366bc0ed81e11eb61b30e3` — `FIX: Register OOC benchmark backend`

## Repository identities and initial status

| Repository | Commit | Branch / status |
|---|---|---|
| DREAM3D-NX | `4671f5ddd6f88fd4d2fe48cbef29b37aecda1a7e` | `ooc-itk-filters`; clean |
| simplnx | `49dd11bf4b6e22031f4e5d211ed3c062c30ac761` | `ooc-itk-filters...jessica/ooc-itk-filters [ahead 1]`; clean |
| SimplnxOoc | `aa3c57b543672a22bd366bc0ed81e11eb61b30e3` | `ooc-itk-filters...origin/ooc-itk-filters [ahead 1]`; clean |

## Machine and toolchain

- macOS 26.5 (25F71)
- CPU: Apple M1 Max; logical CPUs: 10
- Memory: 34,359,738,368 bytes (32 GiB)
- Workspace filesystem: `/dev/disk3s1`, 926 GiB total, 513 GiB used, 386 GiB available (58%)
- Compiler: Apple clang 21.0.0 (clang-2100.1.1.101), `/usr/bin/c++`
- SDK command path: `/Applications/Xcode.app/Contents/Developer/Platforms/MacOSX.platform/Developer/SDKs/MacOSX.sdk`
- Cached SDK: `/Applications/Xcode.app/Contents/Developer/Platforms/MacOSX.platform/Developer/SDKs/MacOSX26.5.sdk`

## Relevant CMake cache identity

| Cache | Build type | `DREAM3DNX_USE_OOC` | `SIMPLNX_TEST_ALGORITHM_PATH` | `SIMPLNXOOC_SOURCE_DIR` |
|---|---|---:|---:|---|
| `build/InCore-Release` | `Release` | `OFF` | `0` | `/Users/joeykleingers/Workspace/SimplnxOoc/.codex/worktrees/ooc-itk-filters` |
| `build/OOC-Release` | `Release` | `ON` | `1` | `/Users/joeykleingers/Workspace/SimplnxOoc/.codex/worktrees/ooc-itk-filters` |

## Exact benchmark commands

```bash
build/InCore-Release/Bin/ImageProcessingBenchmark --mode incore \
  --target-voxels 33554432 --z-slices 128 --warmup 1 --repeats 3 \
  --filters all --require-new-result \
  --out /Users/joeykleingers/Workspace/simplnx/.codex/worktrees/ooc-itk-filters/src/Plugins/ImageProcessing/benchmark/results/2026-08-18-pre-optimization-incore-32M.csv

build/OOC-Release/Bin/ImageProcessingBenchmark --mode ooc \
  --target-voxels 33554432 --z-slices 128 --warmup 1 --repeats 3 \
  --filters all --require-new-result \
  --out /Users/joeykleingers/Workspace/simplnx/.codex/worktrees/ooc-itk-filters/src/Plugins/ImageProcessing/benchmark/results/2026-08-18-pre-optimization-ooc-32M.csv

build/OOC-Release/Bin/ImageProcessingBenchmark \
  --merge /Users/joeykleingers/Workspace/simplnx/.codex/worktrees/ooc-itk-filters/src/Plugins/ImageProcessing/benchmark/results/2026-08-18-pre-optimization-incore-32M.csv \
  /Users/joeykleingers/Workspace/simplnx/.codex/worktrees/ooc-itk-filters/src/Plugins/ImageProcessing/benchmark/results/2026-08-18-pre-optimization-ooc-32M.csv \
  --out /Users/joeykleingers/Workspace/simplnx/.codex/worktrees/ooc-itk-filters/src/Plugins/ImageProcessing/benchmark/results/2026-08-18-pre-optimization-combined-32M.csv
```

## Execution and validation

- In-core sweep started: `2026-08-18T14:55:45-0400`.
- Terminal lifecycle correction: transcript truncation did not terminate either process; both continued and wrote their CSV at normal completion.
- In-core and OOC CSVs each contain 84 lines (header plus 83 rows). The merged CSV also contains 84 lines.
- Final combined validation: `missing_new_incore=0`, `missing_new_ooc=0`, `itk_incore_ms=n/a=6`; controller identity comparison of filter/type/dimensions/voxels was empty.
- Merge command exited 0.
