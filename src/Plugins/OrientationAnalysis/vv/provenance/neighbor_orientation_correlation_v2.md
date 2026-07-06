# Exemplar Archive Provenance: neighbor_orientation_correlation_v2.tar.gz — RETIRED

**Status: RETIRED 2026-07-06 (same day as creation, never consumed by a merged commit).**
No test consumes this archive; there is no `download_test_data()` entry for it. This sidecar
is retained as the forensic record of why the filter now has **no exemplar archive at all**.

---

## Why the filter has no exemplar archive

The V&V policy forbids circular oracles. Both generations of this filter's exemplar failed
that bar:

- **v1 (`neighbor_orientation_correlation.tar.gz`, retired 2026-07-06):** its data container
  was named `DataContainer` while the test looked up `Exemplar Data/...`, so every array
  lookup returned null and the comparison loop's silent `continue` skipped everything —
  **the exemplar comparison never compared a single array** (hollow pass since 2022-08).
- **v2 (this archive):** created during the 2026-07-06 V&V cycle from the post-fix SIMPLNX
  output as a regression pin. That is a circular oracle by construction; it was retired the
  same day, before the V&V branch merged, and the `Small IN100 Pipeline` test was rewritten
  to be archive-free.

## What replaced it

- **Correctness (non-circular):** 11 inline oracle fixtures in
  `test/NeighborOrientationCorrelationTest.cpp` (`Oracle F01`–`F11`, `Class 4` invariants).
  Toy data is built programmatically; expected values are hand-derived (Class 1, co-axial
  z-rotation regime where every misorientation convention reduces to `|Δθ|`) and
  cross-checked by a NumPy reference implementation (Class 2, `reference_noc.py`,
  NumPy 2.4.2, 33 assertions in `check_derivations.py`) — both derived independently of the
  SIMPLNX and legacy implementations.
- **Production scale:** the `Small IN100 Pipeline` test snapshots the full cell data before
  the filter runs and asserts the Class 4 invariants at 4.4M cells (high-confidence cells
  bit-identical across every array; every modified cell was low-confidence; at least one
  cell modified). Invariants derive from the filter's specification, not from any
  implementation's output.

## Archive identity (historical)

| Field | Value |
|---|---|
| **Archive** | `neighbor_orientation_correlation_v2.tar.gz` |
| **SHA512** | `1596d028af1e885005eda9d07e118fc4a03afb4cf30064095052805ea0c098759e4968ceac2f82ac58eae1c9c92a367eae93b4bcd332cebf1622d06ea968e9e7` |
| **Used by tests** | none (retired) |
| **Generated / retired by** | Michael Jackson, 2026-07-06 |

## Second-engineer oracle review

- **Reviewer:** skipped in-session (single engineer)
- **Skip reason:** review requested at PR review of the `vv/NeighborOrientationCorrelation` branch (PR #1655).
