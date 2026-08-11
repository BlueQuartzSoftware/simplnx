# Deviations from DREAM3D 6.5.171: ErodeDilateBadDataFilter

Entries use stable IDs (`ErodeDilateBadDataFilter-D<N>` for legacy deviations, `ErodeDilateBadDataFilter-B<N>` for SIMPLNX-side bugs).

---

## Headline: Legacy A/B comparison performed — one confirmed SIMPLNX-side bug, fixed

The gap recorded in the previous revision of this file ("no legacy comparison has been performed") is closed. Legacy source was located on this machine (`C:\Users\holym\BlueQuartz\Builds\DREAM3D\DREAM3D-6.5.171-Win64` binary, plus `ErodeDilateBadData.{h,cpp}` source in a sibling `DREAM3D` checkout — not committed to this repository, but usable for direct comparison) and a genuine DREAM3D 6.5.171 pipeline (`PipelineRunner.exe`) was run against a hand-verified input twin of the C++ test fixture, across **all 28 combinations** of operation (Erode/Dilate) × direction combination (7 valid combos) × iteration count (1, 2). See [`ErodeDilateBadDataFilter-B1`](#erodedilatebaddatafilter-b1-direction-parameters-had-no-effect-fixed) below and the V&V report's Oracle/Bug Fixes sections for full detail.

## ErodeDilateBadDataFilter-B1: Direction parameters had no effect (fixed)

**Type:** SIMPLNX-side bug (not a legacy deviation — legacy behaves correctly here; SIMPLNX did not).

**Symptom:** `XDirOn`/`YDirOn`/`ZDirOn` were parsed correctly from filter args into `ErodeDilateBadDataInputValues` (`ErodeDilateBadDataFilter.cpp:151-153`), but had **zero effect** on the algorithm. Every face neighbor was eligible (subject only to geometry boundary) regardless of the Direction parameters. This is exactly what produced the previous V&V pass's observation that "all 7 direction-combination fixtures ... encode byte-identical expected output" — the fixture wasn't under-discriminating, the *algorithm* was ignoring direction entirely.

**Root cause:** `adjustValidNeighbors` — the helper clearly intended to mask face-neighbor validity by direction — was defined in `Algorithms/ErodeDilateBadData.cpp` but **never called** anywhere in `operator()()`. Confirmed by grepping the compiled `.cpp` for the literal strings `XDirOn`/`YDirOn`/`ZDirOn`/`adjustValidNeighbors(`: only the function *definition* matched, no call site. (A branch-history note: an earlier commit on this working branch, `7e543f701` "Fixed XYZ direction off bug", *had* added a call to `adjustValidNeighbors`, but passed it the face-index-order array and bitwise-ANDed index constants `0..5` against the direction booleans — which corrupts the iteration order rather than gating validity, and additionally had `+X` gated by `zDir` and `+Z` gated by `xDir` [swapped axes]. That call was later removed in an uncommitted edit, leaving direction fully inert — the state this pass found and fixed from scratch.)

**Fix:** `Algorithms/ErodeDilateBadData.cpp:64-80` — `adjustValidNeighbors` now takes the per-voxel `isValidFaceNeighbor` boolean array (the actual validity gate consumed by the vote/mark loop) and ANDs each of the 6 entries against the correct axis flag, using the named `VoxelNeighbors<Image3D>` constants rather than raw indices. It is now called at `:162-163`, immediately after `computeValidFaceNeighbors`, for every bad-data voxel.

**Verification:**
- All 28 `k_ExemplarFeatureIds*`/`k_ExemplarData*` (Misc) constants in `ErodeDilateBadDataTest.cpp` were rewritten to be direction-discriminating (previously byte-identical across all 7 combos for a given operation/iteration count) and hand-traced against the fixture geometry.
- Independently corroborated against genuine DREAM3D 6.5.171 output: 28/28 combinations (7 directions × 2 operations × 2 iteration counts) match exactly, both `FeatureIds` and `Misc` — see V&V report Oracle section for the run details.
- `(Erode) Expanded` / `(Dilate) Expanded` (28 GENERATE runs total): pass.

## Non-deviations (confirmed correct — do not "fix")

### Dilate tie-break: last-bad-neighbor-wins is correct, not a bug

**Investigated and ruled out this pass.** When a good voxel has two or more bad face-neighbors, `neighbors[neighborPoint] = voxelIndex` unconditionally overwrites on each bad neighbor visited, so whichever bad voxel is scanned *last* (highest flat index, in z/y/x order) wins. Since every bad voxel shares `FeatureId == 0`, this choice is invisible to a `FeatureIds`-only comparison — it only shows up in the `Misc` tracer array, which is exactly why it was flagged as unverified in the prior pass and why this pass initially suspected it as a bug.

A "first bad neighbor wins" fix (skip the overwrite if `neighbors[neighborPoint]` is already set, plus resetting `neighbors` to `-1` at the top of each iteration) was implemented and *appeared* correct until checked against real DREAM3D 6.5.171 output: `PipelineRunner.exe` running Dilate/XYZ/1-iteration against the matching legacy input file produced `Misc` values matching the **original, unmodified** last-write-wins SIMPLNX behavior, not the "first-wins" rewrite (diverged at 3 of 32 indices: 9, 15, 30). The "first-wins" change was reverted in full (both the per-iteration reset and the overwrite guard); unconditional last-write-wins, with `neighbors` initialized once before the iteration loop (not per-iteration), is confirmed legacy-faithful.

This resolves the prior V&V pass's "second-engineer review pending: erode/dilate tie-break order" item — no further review needed; verified against the actual legacy binary output, not just source reading.

### Erode tie-break: first-processed-neighbor-wins is correct

Vote-count-based, using `[-Z,-Y,-X,+X,+Y,+Z]` scan order; a later neighbor's vote must strictly exceed the current max to replace the leader. Matches legacy source line-for-line (identical vote/comparison logic) and matches legacy binary output for all tested combinations. Not a deviation.

### Legacy tie-break language says "chosen randomly"; SIMPLNX is deterministic

The SIMPLNX filter markdown (`docs/ErodeDilateBadDataFilter.md`), carried over from legacy documentation, states that erode ties are broken "randomly." Both the legacy *source* (`ErodeDilateBadData.cpp`, `Source/Plugins/Processing/ProcessingFilters/`) and the legacy *binary* output are fully deterministic — same first-processed-wins scan order as SIMPLNX, no RNG involved anywhere in the algorithm. "Randomly" in the legacy docs is inaccurate documentation language, not a behavioral characteristic; SIMPLNX's determinism is not a deviation. (Legacy source is now available for direct comparison — this was previously only inferable.)

## Per-direction code-path coverage

Previously an open question ("could not distinguish correct per-direction gating from a no-op gate" — see prior V&V pass). Resolved this pass by two independent means:
1. **Behavioral:** exemplar data now differs by direction combination (see B1 above), and matches legacy per-combination.
2. **Structural:** `Algorithms/ErodeDilateBadData.cpp` was temporarily instrumented with per-face-direction (`-Z/-Y/-X/+X/+Y/+Z`) hit counters and run through the full `(Erode) Expanded` + `(Dilate) Expanded` GENERATE sweep. All 6 directions were hit — both at the "loop reached" level and at the "vote/mark condition fired" level, in both the vote/mark loop and the Erode-only cleanup loop. Instrumentation was removed afterward (not shipped).

## What would need to happen to extend this further

Everything listed in the prior revision of this file has been done: legacy binary obtained and run, SIMPLNX output compared element-wise against legacy output, and the neighbor-selection/tie-break logic diffed directly against legacy source. The zero-dimensions preflight path (`-14602`), also previously listed here as uncovered, is now reached and correctly asserted by the `No Dimensions` test (fixed on this branch — see V&V report Code path coverage). Remaining follow-up (not gating, see V&V report):
1. The legacy A/B comparison in this pass was a manual/one-time verification (pipeline JSONs run through `PipelineRunner.exe`, output diffed via `h5py`), not wired into automated CI. Consider checking in the legacy `.dream3d` input/output pairs as an exemplar archive and adding an automated Class 2 comparison test (matching the pattern used by `FillBadDataFilter`'s `FillBadData_SmallIN100` test), so this verification re-runs on every CI build instead of relying on this document.
