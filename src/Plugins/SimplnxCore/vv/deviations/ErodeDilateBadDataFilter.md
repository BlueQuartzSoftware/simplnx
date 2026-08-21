# Deviations from DREAM3D 6.5.171: ErodeDilateBadDataFilter

This file lists every documented behavioral difference between this SIMPLNX filter and its DREAM3D 6.5.171 equivalent.

Entries are referenced by stable ID (`ErodeDilateBadDataFilter-D<N>`) from the V&V report and from public migration guidance. The ID is stable across renames; the Filter UUID field is the permanent cross-reference anchor.

> **ID note:** `ErodeDilateBadDataFilter-D1` was cited as `ErodeDilateBadDataFilter-B1` in earlier revisions of this file and in the PR #1687 review thread, under a local `-B<N>` convention for SIMPLNX-side bugs. That convention is used nowhere else in the repository, so the entry has been renumbered to the project-standard `-D<N>` form (matching `CAxisSegmentFeaturesFilter-D1`, which documents an equivalent SIMPLNX-side bug found during its own V&V cycle). No external references to the old ID exist.

## Headline: legacy A/B comparison performed — one SIMPLNX-side bug, fixed

The gap recorded in the previous revision of this file ("no legacy comparison has been performed") is closed. A genuine DREAM3D 6.5.171 `PipelineRunner` was run against a hand-verified input twin of the C++ test fixture across **all 28 combinations** of operation (Erode/Dilate) × direction combination (7 valid combos) × iteration count (1, 2), and the legacy `ErodeDilateBadData.{h,cpp}` source was diffed line-by-line against the SIMPLNX algorithm. Both the binary and the source live in a sibling checkout on the authoring engineer's machine and are not committed to this repository. See `ErodeDilateBadDataFilter-D1` below and the V&V report's Oracle and Bug Fixes sections.

---

## ErodeDilateBadDataFilter-D1

| Field | Value |
|---|---|
| **Deviation ID** | `ErodeDilateBadDataFilter-D1` (formerly cited as `-B1`) |
| **Filter UUID** | `7f2f7378-580e-4337-8c04-a29e7883db0b` |
| **Status** | active (SIMPLNX bug **resolved** in PR #1687; documented for users of prior SIMPLNX releases) |

**Symptom:** In SIMPLNX releases prior to PR #1687, the *X Direction*, *Y Direction*, and *Z Direction* parameters had **no effect on the output**. They were parsed correctly from filter args into `ErodeDilateBadDataInputValues` (`ErodeDilateBadDataFilter.cpp:151-153`), but every face neighbor remained eligible — subject only to the geometry boundary — regardless of the flags. Disabling a direction silently produced the all-directions-on result. DREAM3D 6.5.171 honors the flags correctly, so any run with fewer than all three directions enabled diverges from legacy. This is also what produced the previous V&V pass's observation that "all 7 direction-combination fixtures encode byte-identical expected output": the fixture was not under-discriminating, the algorithm was ignoring direction entirely.

**Root cause:** Bug (SIMPLNX). `adjustValidNeighbors` — the helper intended to mask face-neighbor validity by direction — was defined in `Algorithms/ErodeDilateBadData.cpp` but **never called** from `operator()()`. Confirmed by grepping the translation unit for `XDirOn` / `YDirOn` / `ZDirOn` / `adjustValidNeighbors(`: only the function definition matched, with no call site. Legacy achieves the same gating by ORing the direction flag into each per-face boundary check (`|| !m_ZDirOn` and siblings), so the legacy behavior was never in question.

*Branch-history note:* an earlier commit on the fix branch, `7e543f701` "Fixed XYZ direction off bug", *had* added a call to `adjustValidNeighbors`, but passed it the face-index-order array and bitwise-ANDed the index constants `0..5` against the direction booleans — which corrupts the iteration order rather than gating validity — and additionally gated `+X` by `zDir` and `+Z` by `xDir` (swapped axes). Since `2&1=0` and `3&1=1`, the iterated face list collapsed to `{-Z,-Y}` for *every* flag combination, including all-on. That call was later removed, leaving direction fully inert until it was reimplemented.

**Fix:** `Algorithms/ErodeDilateBadData.cpp:64-80` — `adjustValidNeighbors` now takes the per-voxel `isValidFaceNeighbor` boolean array (the actual validity gate consumed by the vote/mark loop) and ANDs each of the six entries against the correct axis flag, using the named `VoxelNeighbors<Image3D>` constants rather than raw indices. It is called at `:162-163`, immediately after `computeValidFaceNeighbors`, for every bad-data voxel.

**Verification:**

- All 28 `k_ExemplarFeatureIds*` / `k_ExemplarData*` constants in `ErodeDilateBadDataTest.cpp` were regenerated from genuine DREAM3D 6.5.171 binary output. They are legacy output, not a hand derivation — a Class 2 oracle. (They were previously byte-identical across all 7 direction combinations for a given operation and iteration count, which is what masked the bug.)
- 28/28 combinations (7 directions × 2 operations × 2 iteration counts) match legacy exactly, on both `FeatureIds` and the `Misc` tracer array — see the V&V report's Oracle section for the run details.
- `(Erode) Expanded` / `(Dilate) Expanded` (28 parameterized runs total, 2078 assertions combined) pass in both in-core and OOC builds.
- Per-direction structural coverage confirmed by temporary hit-count instrumentation — see "Per-direction code-path coverage" below.

**Affected users:** Anyone who ran `ErodeDilateBadData` on a SIMPLNX build predating PR #1687 with fewer than all three directions enabled. Their output silently matched the all-directions-on result, eroding or dilating across axes they had explicitly disabled. Users who left all three directions on (the default) are **unaffected** — that path was always correct, and the archive-based `(Erode)` regression test passes on affected releases for exactly that reason.

**Recommendation:** Trust SIMPLNX at or after PR #1687, which agrees with 6.5.171 across all 28 parameter combinations. Results from affected builds that used a restricted direction set should be regenerated.

---

## Non-deviations (confirmed correct — do not "fix")

### Dilate tie-break: last-bad-neighbor-wins is correct, not a bug

**Investigated and ruled out.** When a good voxel has two or more bad face-neighbors, `neighbors[neighborPoint] = voxelIndex` unconditionally overwrites on each bad neighbor visited, so whichever bad voxel is scanned *last* (highest flat index, in z/y/x order) wins. Since every bad voxel shares `FeatureId == 0`, this choice is invisible to a `FeatureIds`-only comparison — it shows up only in the `Misc` tracer array, which is why it was flagged as unverified in the prior pass and initially suspected as a bug in this one.

A "first bad neighbor wins" fix (skip the overwrite if `neighbors[neighborPoint]` is already set, plus resetting `neighbors` to `-1` at the top of each iteration) was implemented and *appeared* correct until checked against real DREAM3D 6.5.171 output: `PipelineRunner` running Dilate / XYZ / 1 iteration against the matching legacy input produced `Misc` values matching the **original, unmodified** last-write-wins SIMPLNX behavior, not the "first-wins" rewrite (diverging at 3 of 32 indices: 9, 15, 30). The change was reverted in full — both the per-iteration reset and the overwrite guard. Unconditional last-write-wins, with `neighbors` initialized once before the iteration loop rather than per-iteration, is confirmed legacy-faithful.

This resolves the prior V&V pass's "second-engineer review pending: erode/dilate tie-break order" item — verified against actual legacy binary output, not source reading alone.

### Erode tie-break: first-processed-neighbor-wins is correct

Vote-count-based, using `[-Z,-Y,-X,+X,+Y,+Z]` scan order; a later neighbor's vote must strictly exceed the current maximum to replace the leader. Matches legacy source line-for-line (identical vote and comparison logic) and matches legacy binary output for all 28 tested combinations. Not a deviation.

### Direction masking of the Erode `featureCount` reset loop

SIMPLNX reuses the direction-masked `isValidFaceNeighbor` array in the post-vote reset loop; legacy resets over *boundary-valid* neighbors and ignores the direction flags there. Simulating both variants across all 28 combinations gives identical output, and the equivalence is general rather than incidental: the reset set is a superset of the increment set in both variants, so `featureCount` returns to all-zeros after every bad voxel either way. Not a deviation.

### Deferring the FeatureIds transfer to a second pass

Legacy interleaves the `FeatureIds` transfer with the other cell arrays in a single pass, mutating `m_FeatureIds` as it goes. SIMPLNX transfers the other arrays first (in parallel) and `FeatureIds` afterward, serially. Equivalent: erode only maps 0→>0 and dilate only >0→0, `neighbors[]` always points at a voxel whose relevant polarity is preserved by the operation, and each index is written at most once per pass — so neither transfer predicate can ever observe a changed value. Not a deviation.

### Legacy tie-break language says "chosen randomly"; SIMPLNX is deterministic

The SIMPLNX filter markdown (`docs/ErodeDilateBadDataFilter.md`), carried over from legacy documentation, stated that erode ties are broken "randomly." Both the legacy *source* (`ErodeDilateBadData.cpp`, `Source/Plugins/Processing/ProcessingFilters/`) and the legacy *binary* output are fully deterministic — the same first-processed-wins scan order as SIMPLNX, with no RNG anywhere in the algorithm. "Randomly" is inaccurate documentation language, not a behavioral characteristic; SIMPLNX's determinism is not a deviation.

**The user-facing doc has been corrected** to describe the actual behavior: the six face neighbors are visited in the fixed order `[-Z,-Y,-X,+X,+Y,+Z]` and a later neighbor must have a strictly greater count to displace the leader, so the earliest tied neighbor in that scan order wins. The same edit documents the two preflight errors (`-14601` no directions enabled, `-14602` zero-length geometry dimension) and the effect of the direction restrictions.

### Cancel check present in SIMPLNX, absent in legacy

`operator()` reads `m_ShouldCancel` once per Z-slice inside the iteration loop and returns immediately if set. Legacy's equivalent loop has no cancel check at all. Additive capability with no effect on a run to completion — SIMPLNX is ahead of legacy here, not behind. Not a deviation. (This path is Path 11 in the V&V report's coverage table and is the one enumerated path no test exercises.)

## Per-direction code-path coverage

Previously an open question ("could not distinguish correct per-direction gating from a no-op gate" — previously unresolved). Now resolved by two independent means:

1. **Behavioral:** expected data now differs by direction combination (see D1 above) and matches legacy per-combination, 28/28.
2. **Structural:** `Algorithms/ErodeDilateBadData.cpp` was temporarily instrumented with per-face-direction (`-Z/-Y/-X/+X/+Y/+Z`) hit counters and run through the full `(Erode) Expanded` + `(Dilate) Expanded` sweep. All six directions were hit, both at the "loop reached" level and at the "vote/mark condition fired" level, in both the vote/mark loop and the Erode-only cleanup loop. Instrumentation was removed afterward and is not shipped.

## What would need to happen to extend this further

Everything listed in the prior revision of this file has been done: legacy binary obtained and run, SIMPLNX output compared element-wise against legacy output, and the neighbor-selection and tie-break logic diffed directly against legacy source. The zero-dimensions preflight path (`-14602`), also previously listed here as uncovered, is now reached and asserted by the `No Dimensions` test.

Remaining follow-up (not gating — see the V&V report's V&V phase row):

1. **Automate the 28-combination A/B run.** It was a manual, one-time verification (pipeline JSONs through `PipelineRunner`, output diffed with `h5py`), not wired into CI. The expected values are now compiled into the test as constants, so the *comparison* does re-run in CI — but a future change to the fixture would require redoing the legacy run by hand. Consider checking in the legacy `.dream3d` input/output pairs as an exemplar archive with a provenance sidecar, matching the pattern used by `FillBadDataFilter`'s `FillBadData_SmallIN100` test.
2. **Cover the cancel path (Path 11).** Requires cancel-signal injection; no test currently sets `m_ShouldCancel` and asserts early termination.
3. **Add a production-scale Dilate test.** `6_6_erode_dilate_test.tar.gz` already contains an unused `Exemplar Bad Data Dilate` container, so this costs no new test data. See `../provenance/6_6_erode_dilate_test.md`.
