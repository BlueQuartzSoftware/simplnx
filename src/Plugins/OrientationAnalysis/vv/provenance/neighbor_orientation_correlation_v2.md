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
  **the exemplar comparison never compared a single array at any commit in its history.** Bisect proof (2026-07-07): the comparison loop was introduced already mapping to `Exemplar Data` with the silent `continue` (commit `d199bc749`, 2022-07-24, pre-simplnx plugin repo), and the archive's SHA512 (`1223674…`) is unchanged from its first `download_test_data()` registration (commit `e34baf1f2`, 2022-12-02) through retirement — its container was always `DataContainer`. Hollow from birth.
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

  > **⚠ Evidence archival (open action):** `reference_noc.py`, `check_derivations.py`, and
  > `DERIVATIONS.md` live only in the engineer's local `VV_Work/` folder and are **not** committed
  > to the repository, so the Class 2 reference oracle is not reproducible from the repo. They must be
  > uploaded to the OneDrive verification archive (per the archive-filter-verification workflow) and
  > this note replaced with the archive link before final sign-off. Mitigation: every fixture's
  > expected value is *also* hand-derivable (Class 1) and encoded inline in the test, so the fixtures
  > remain auditable even while the reference scripts are unarchived.
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

- **Reviewer:** Michael Jackson (technical authority)
- **Date:** 2026-07-16
