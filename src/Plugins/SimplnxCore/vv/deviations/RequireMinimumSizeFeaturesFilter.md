# Deviations from DREAM3D 6.5.171: RequireMinimumSizeFeaturesFilter

| Field | Value |
|-------|-------|
| SIMPLNX UUID | `074472d3-ba8d-4a1a-99f2-2d56a0d082a0` |
| SIMPL UUID   | `53ac1638-8934-57b8-b8e5-4b91cdda23ec` (`MinSize`) |
| Comparison run | 2026-08-18 |
| Full comparison details | `ww_work/RequireMinimumSizeFeatures/ReadMe.md` + `results_compare.txt` + `results_oracle_check.txt` (OneDrive; never committed) |

## No deviations observed

The binary A/B against DREAM3D 6.5.171 `PipelineRunner` produced **bit-identical output
on every tested configuration**: 8 parameter combinations × 4 arrays = **32/32 matches**,
with no element-level difference in the cell `FeatureIds`, the index-encoded
`CopiedScalar` companion array, or the compacted feature-level `NumElements` and
`Phases` arrays (tuple counts included).

Both binaries were run against the same legacy-format inputs — the 6×6×6 and 5×1×1 grids
the SIMPLNX unit tests build in
`src/Plugins/SimplnxCore/test/RequireMinimumSizeFeaturesTest.cpp` — rather than against
separately invented A/B data. (The test file also carries a third grid, the 9×1×1
vote-counter-reset strip, added after the A/B run to kill a mutation the other two were
blind to; it was not part of the A/B and is not claimed as legacy evidence.)

| Combination | MinAllowedFeatureSize | ApplyToSinglePhase | Comparison | Result |
|-------------|----------------------|--------------------|------------|--------|
| `cube6_min3_sp0` | 3 | no       | 6.5.171 vs NX | Bit-identical (216 cells × 2 arrays, 6-tuple feature arrays) |
| `cube6_min3_sp1` | 3 | yes (1)  | 6.5.171 vs NX | Bit-identical — both a no-op (nothing crosses the threshold in phase 1) |
| `cube6_min4_sp0` | 4 | no       | 6.5.171 vs NX | Bit-identical (12 cells reassigned, 7→4 feature tuples) |
| `cube6_min4_sp1` | 4 | yes (1)  | 6.5.171 vs NX | Bit-identical (11 cells reassigned, 7→5 feature tuples) |
| `cube6_min5_sp0` | 5 | no       | 6.5.171 vs NX | Bit-identical (16 cells reassigned, 7→3 feature tuples) |
| `cube6_min5_sp1` | 5 | yes (1)  | 6.5.171 vs NX | Bit-identical (16 cells reassigned, 7→4 feature tuples) |
| `strip5_min2_sp0`| 2 | no       | 6.5.171 vs NX | Bit-identical — both a no-op (feature 2 sits exactly at the threshold) |
| `strip5_min3_sp0`| 3 | no       | 6.5.171 vs NX | Bit-identical (2 cells reassigned, 3→2 feature tuples) |

Agreement alone is not evidence of correctness, so each binary was additionally checked
against the hand-derived oracle independently for the three combinations the unit tests
assert (`cube6_min4_sp0`, `cube6_min4_sp1`, `strip5_min3_sp0`): **24/24 arrays match the
oracle in both binaries.**

Two behaviours were singled out because they are the places a port most plausibly drifts,
and both were confirmed *by execution* rather than only by source reading:

* **At-threshold survival.** A feature holding exactly `MinAllowedFeatureSize` cells
  survives in both versions. Legacy `MinSize.cpp:452` (all-phase) / `:463`
  (single-phase) and SIMPLNX
  `Algorithms/RequireMinimumSizeFeatures.cpp:297,308` both keep on `NumCells >= min`, so
  removal is strictly below the threshold. The `cube6_min4_*` and `strip5_min2_sp0`
  combinations exercise it from both sides.
* **Vote tie-break.** At the fixture's deliberate 2-vs-2 tie (cell `(2,5,5)`, index 212 —
  feature 1 at `-Z` and `-X`, feature 2 at `-Y` and `+X`), both binaries wrote
  `CopiedScalar = 10211`, i.e. both selected the `-X` neighbour at index 211. That
  confirms the strictly-greater-than vote test (`if(current > most)` at
  `MinSize.cpp:358-364`; `if(currentVoteCount > maxVoteCount)` at
  `RequireMinimumSizeFeatures.cpp:231`) in both code lines. A non-strict test would have
  produced `10213` in either binary.

The NX implementation is a direct port. The port-time differences that do exist —
`NeighborUtilities` helpers in place of inline offset/boundary arithmetic, `std::fill`
in place of a per-neighbour vote-counter reset, FeatureIds transferred last instead of
interleaved, and copy-forward compaction in place of tuple erasure — are all internal
mechanics and the output is invariant to them. See the report's *Algorithm Relationship*
section for the line-by-line accounting.

**No surgical patch to the legacy line is required**, because no legacy bug was confirmed.

## Note: a shared robustness gap is not a deviation

`RequireMinimumSizeFeatures` indexes `activeObjects` and `voteCounter` with unvalidated
FeatureIds and has no no-progress guard on its coarsening loop, where the sibling
`RequireMinNumNeighborsFilter` gained explicit `-55567` / `-55572` errors during its own
V&V. DREAM3D 6.5.171 `MinSize.cpp` contains the identical unguarded code, so the two
versions behave the same and there is nothing here for a migrating user to be warned
about. It is recorded as a hardening follow-up in the V&V report, not as a deviation
entry.
