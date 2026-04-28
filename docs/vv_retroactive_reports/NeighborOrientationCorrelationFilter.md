# Retroactive V&V: NeighborOrientationCorrelationFilter

*Report status:* **DRAFT**. Generated from git-history and source-tree inspection. Developer must confirm or correct the Oracle class, Algorithm Relationship, and the V&V status entries.

## Metadata

| Field | Value |
|---|---|
| SIMPLNX UUID | `4625c192-7e46-4333-a294-67a2eb64cb37` |
| SIMPLNX ClassName | `NeighborOrientationCorrelationFilter` |
| SIMPLNX Human Name | Neighbor Orientation Correlation |
| SIMPL UUID | *(TBD — confirm in legacy SIMPL repo; the SIMPLNX UUID was probably preserved from SIMPL but should be checked)* |
| SIMPL ClassName | `NeighborOrientationCorrelation` *(presumed; confirm in legacy SIMPL repo)* |
| SIMPL Human Name | Neighbor Orientation Correlation *(presumed; confirm in legacy SIMPL repo)* |
| Plugin | OrientationAnalysis |

### Source files scanned

- `src/Plugins/OrientationAnalysis/src/OrientationAnalysis/Filters/NeighborOrientationCorrelationFilter.{hpp,cpp}`
- `src/Plugins/OrientationAnalysis/src/OrientationAnalysis/Filters/Algorithms/NeighborOrientationCorrelation.{hpp,cpp}`
- `src/Plugins/OrientationAnalysis/test/NeighborOrientationCorrelationTest.cpp`
- `src/Plugins/OrientationAnalysis/test/simpl_conversion/6_5/NeighborOrientationCorrelationFilter.json`
- `src/Plugins/OrientationAnalysis/test/simpl_conversion/6_4/NeighborOrientationCorrelationFilter.json`
- `src/Plugins/OrientationAnalysis/docs/NeighborOrientationCorrelationFilter.md`

## Algorithm Relationship

- **Tentative classification:** **Port** — direct translation of the legacy SIMPL `NeighborOrientationCorrelation` filter. The algorithmic structure (start at level 6, decrement to user `Level`, count well-correlated face neighbors per low-confidence voxel, replace via `copyTuple` from "best" neighbor) is the historical EBSD cleanup pass. The UUID appears to have been preserved (developer should verify against legacy SIMPL).
- **Evidence:** No PR in the inspection window rewrites the algorithm core. PR #1523 mechanically refactors the inline 6-face neighbor index/bounds logic to call `simplnx/Utilities/NeighborUtilities.hpp` helpers (`initializeFaceNeighborOffsets`, `computeValidFaceNeighbors`, `faceNeighborInternalIdx`); the math is unchanged. PR #1472 swaps `QuatF` / `OrientationD` symbols for the `ebsdlib::QuatD` / `ebsdlib::AxisAngleDType` API but the call to `LaueOps::calculateMisorientation` is preserved. PR #1535 strips redundant null/type/component checks now covered by the parameter system. PR #1590 tightens neighbor types to `VoxelNeighbors<Image3D>::k_FaceNeighborCount` and removes redundant repeated calls to `computeValidFaceNeighbors` per inner iteration.
- **Action required:** Confirm SIMPL ancestry by reading the corresponding SIMPL filter source and running `compare-legacy-dream3d` step (e) against a shared toy dataset. There is one structural difference vs. legacy worth flagging: the `> currentLevel` strict-inequality threshold and how "best" neighbor ties are broken (see "Recommended Deviation entries" below).

## PRs inspected (since 2025-10-01)

> Pruned: pure-style/repo-wide refactor PRs (#1457 static-inline cleanup, #1439 NeighborList tuple support, #1538 zlib tar.gz extraction, #1543 doc pipeline references) are listed at the bottom of this section but not detailed individually — they did not change behavior of this filter.

### PR #1438 — *"ENH: Microtexture related filter cleanup"* — merged 2025-10-25

- **Files in this filter:** algorithm (.cpp)
- **Diff size:** 1 file, +1 / -1 line
- **Change nature:** Pure include-syntax fix — `#include "EbsdLib/LaueOps/LaueOps.h"` → `#include <EbsdLib/LaueOps/LaueOps.h>` to use external-library include syntax. No behavioral change in *this* filter, but the umbrella PR carried multiple correctness fixes to other microtexture/orientation filters (CAxisSegmentFeatures, ComputeAvgOrientations, FindFeatureReferenceCAxisOrientation, ConvertOrientations, etc.).
- **V&V content:** Flagged by the policy maintainer as MTR-V&V-related work (parent PR is the umbrella for early MTR fixes). For *this* filter the diff is cosmetic; the umbrella PR's other fixes are not relevant here.

### PR #1472 — *"ENH: Update to EbsdLib 2.0.0 API"* — merged 2025-11-24 *(broad refactor, exception flagged because this filter delegates orientation math to EbsdLib)*

- **Files in this filter:** algorithm (.cpp)
- **Diff size:** 1 file, +7 / -7 lines
- **Change nature:** API migration. Replaces `QuatF` (single-precision) with `ebsdlib::QuatD` (double-precision) for `quat1`, `quat2`, and replaces `OrientationD` with `ebsdlib::AxisAngleDType` for the misorientation result returned by `LaueOps::calculateMisorientation`. The numerical inputs to `LaueOps` are now constructed as doubles instead of floats — the underlying quaternion array (`quats`, a Float32Array) is widened to double at the call site. Functionally this should *increase* precision of the misorientation calculation but may produce small bit-level differences vs. legacy SIMPL output that still uses the old `QuatF` path.
- **V&V content:** Promoted because this filter is an EbsdLib-delegating filter (per the pruning rules, #1472 is promoted by default for such filters). The float-vs-double precision shift is a candidate Deviation entry.

### PR #1523 — *"ENH: Factor out the 6-face neighbor code that is systemic through out the code base"* — merged 2026-02-05

- **Files in this filter:** algorithm (.cpp), **+71 / -141 lines (net -70)**
- **Change nature:** Mechanical refactor. The hand-rolled 6-face neighbor offset table (`neighpoints[6]`) and the inline boundary-check chain (`if(j == 0 && plane == 0) good = false; ...`) are replaced by calls to the new shared helpers in `simplnx/Utilities/NeighborUtilities.hpp`: `initializeFaceNeighborOffsets`, `computeValidFaceNeighbors`, and the `faceNeighborInternalIdx` ordering. Local variable names also become more descriptive (`i` → `voxelIndex`, `j` → `faceIndexJ`, `column/row/plane` → `xIdx/yIdx/zIdx`, `int64_t` → `int64`, `int32_t` → `int32`, etc.). The two-level loop structure (per-voxel face × per-face co-neighbor) and the misorientation comparisons inside it are otherwise the same.
- **V&V content:** No new tests. **Behavioral risk:** the helpers must produce the *same* offset ordering and the *same* validity flags as the inline code did. The diff also momentarily introduces a cosmetic redundancy where `isValidFaceNeighbor` is recomputed inside the inner loop (later cleaned up by PR #1590). This PR is the kind of cross-filter sweep that benefits from a regression-vs-legacy comparison since the algorithmic intent is preserved but the index plumbing is now shared with several other filters.

### PR #1535 — *"ENH: Remove redundant preflight checks that are already done in the parameter"* — merged 2026-02-18 *(broad refactor, exception flagged because the change to this filter is non-trivial — 78 lines removed from preflight)*

- **Files in this filter:** filter (.cpp), **+2 / -76 lines**
- **Change nature:** Strips ~76 lines of preflight code that were duplicating validation already performed by the typed `ArraySelectionParameter` (existence checks, DataType checks, component-count checks for ConfidenceIndex/CellPhases/CrystalStructures/Quats). Switches the `imageGeomPtr` null-check + manual error to a direct `getDataRefAs<ImageGeom>`. Net behavior is intended to be identical because the parameter system enforces the same invariants — but the explicit error codes (`k_MissingGeomError = -580090`, `k_MissingInputArray = -580091`, `k_IncorrectInputArray = -580092`) are gone, replaced by the parameter system's built-in error reporting.
- **V&V content:** None added. **Behavioral risk:** very low for valid pipelines. For invalid pipelines, error codes/messages users see will be different from before — worth noting if any downstream tooling parses error codes.

### PR #1538 — *"ENH: Replace cmake subprocess tar.gz extraction with zlib in unit tests"* — merged 2026-02-23

- **Files in this filter:** test (.cpp), +2 / -3 lines
- **Change nature:** Test infrastructure — switches the test to use the new `TestFileSentinel` mechanism for fetching `neighbor_orientation_correlation.tar.gz` and `Small_IN100_dream3d_v3.tar.gz`. No algorithmic change.
- **V&V content:** None.

### PR #1588 — *"ENH: SIMPL Backwards Compatibility Test Redesign"* — merged 2026-04-22

- **Files in this filter:** test (.cpp) +49 lines, plus two new fixture files
  - `test/simpl_conversion/6_4/NeighborOrientationCorrelationFilter.json` (1332 bytes)
  - `test/simpl_conversion/6_5/NeighborOrientationCorrelationFilter.json` (1389 bytes)
- **Change nature:** **Test addition.** Added a per-filter SIMPL→SIMPLNX backwards-compatibility test (`OrientationAnalysis::NeighborOrientationCorrelationFilter: SIMPL Backwards Compatibility`) that exercises both the SIMPL 6.4 (Filter_Name fallback) and 6.5 (UUID-mapped) pipeline conversion paths via `DYNAMIC_SECTION`. Asserts each typed parameter value (`MinConfidence == 2.5f`, `MisorientationTolerance == 2.5f`, `Level == 5`, etc.) and that the resulting filter has the right UUID.
- **V&V content:** **Pipeline-conversion correctness only** — verifies that loading a legacy SIMPL pipeline produces a SIMPLNX filter instance with the right parameter values. Does **not** verify that the filter's *output* matches legacy. That step is still missing.

### PR #1590 — *"ENH: Standardize 2D Image Handling"* — merged 2026-04-23 *(broad refactor, exception flagged because this filter uses the NeighborUtilities helpers that were re-typed by this PR)*

- **Files in this filter:** algorithm (.cpp), +5 / -6 lines
- **Change nature:** Type tightening. Switches the local arrays to use `VoxelNeighbors<Image3D>::k_FaceNeighborCount` (a `constexpr FaceNeighborType`) for their size template parameter, makes the offset and idx arrays `const`/`constexpr`, and removes a redundant call to `computeValidFaceNeighbors` inside the inner loop (the validity flags do not change between iterations). The inline `k_FaceNeighborCount` reference (introduced by PR #1523) is replaced with the dimensionality-aware `VoxelNeighbors<Image3D>::k_FaceNeighborCount`. **Important:** this filter is hard-wired to `Image3D`, even though the broader PR theme was to make image handling work in 2D. So if a user runs this filter on an image whose Z dimension is 1, the 6-face neighbor logic will still be applied — the two would-be Z-neighbors will simply be flagged invalid by `computeValidFaceNeighbors`. This is the historical SIMPL behavior and is presumably correct, but worth confirming as part of the legacy comparison.
- **V&V content:** No new tests added for *this* filter (the PR added `NeighborUtilities` pure-function tests at the utility layer instead).

### Pruned PRs (touched the file but not behaviorally relevant to this filter)

| PR | Subject | Why pruned |
|---|---|---|
| #1439 | Multi-Dimensional Tuple Support for StringArray and NeighborList | Test file — touched 1 line, API plumbing |
| #1457 | Clean up 'static inline' from filter headers | Pure style |
| #1513 | DOC: Update documentation for various filters | Comment-only changes in algorithm .cpp |
| #1543 | Update pipeline references in each of the documentation files | Doc-only, 1 line |

## Test coverage detected

`NeighborOrientationCorrelationTest.cpp` contains 2 `TEST_CASE`s:

1. `OrientationAnalysis::NeighborOrientationCorrelationFilter: Small IN100 Pipeline` — Full Small IN100 reconstruction prologue (MultiThreshold → ConvertOrientations → AlignSectionsMisorientation → IdentifySample → AlignSectionsFeatureCentroid → BadDataNeighborOrientationCheck) feeds into a single invocation of NeighborOrientationCorrelationFilter (`MinConfidence=0.2`, `MisorientationTolerance=5.0°`, `Level=2`). Every cell-data array in the result is then compared element-wise (typed `CompareDataArrays<T>`) against the corresponding array in the exemplar `neighbor_orientation_correlation.dream3d`.
2. `OrientationAnalysis::NeighborOrientationCorrelationFilter: SIMPL Backwards Compatibility` — SIMPL 6.4 + 6.5 conversion paths via `DYNAMIC_SECTION` *(added by PR #1588)*. Pipeline-conversion only — does not execute the filter or compare outputs.

There is **no** small/synthetic test case (no hand-built fixture asserting a specific voxel was/was-not corrected); coverage is only the full-pipeline end-to-end against the exemplar.

## Exemplar archive

- **Archive name:** `neighbor_orientation_correlation.tar.gz`
- **SHA512:** `122367452174ade2f24dde7a4610bddc4f147a223722d9b30c1df9eaa2cd2bf25e1c7957aba83f3f9de79b4eadd79339b848f9530d1ebf44c69244ea5442cf85`
- **Companion archive (input data):** `Small_IN100_dream3d_v3.tar.gz` (referenced by the same test)
- **Referenced in:** `src/Plugins/OrientationAnalysis/test/CMakeLists.txt` (line 150)
- **Provenance:** *(TBD — engineer must inspect the archive to determine how the exemplar was generated and whether an Oracle Provenance block exists in any ReadMe inside it.)*
- **Action required:** Download the archive locally and inspect for: an inner `ReadMe.md`, the input `.dream3d` files used to generate the exemplars, the pipeline files that produced the exemplars, and any provenance notes. Promote this content into the verification archive ReadMe per Step 0's Oracle Provenance policy.

## Oracle classification (tentative)

- **Recommended class:** **4 (Invariant-based)**, with a **3 (Paper-based)** companion if a named reference for this specific cleanup pass can be identified, and **1 (Analytical)** for tiny hand-built fixtures.
- **Rationale:**
  - **Invariants (Class 4) suitable for assertion in unit tests:**
    - **Idempotence on a uniform-orientation volume:** if every voxel has the same quaternion within tolerance, no voxel should be modified across any number of iterations. (The algorithm only modifies voxels whose `confidenceIndex < MinConfidence`, so the fixture must also have all CI ≥ MinConfidence — *or* alternatively, set MinConfidence high but then every "best neighbor" will agree, so the replacement is a no-op semantically.)
    - **Monotone level decrement:** the iteration runs from `currentLevel = 6` down to `> Level` (strict). On each pass, voxels that were corrected at a higher (more permissive) level cannot be un-corrected at a lower level. This is a structural invariant of the loop, not the math.
    - **Bounded change set per pass:** only voxels with `confidenceIndex < MinConfidence` can be touched. Voxels above the threshold must remain bit-identical across iterations. (Easy `REQUIRE` assertion in a small-fixture test.)
    - **Phase preservation:** when `cellPhases[voxelIndex] != cellPhases[neighborPoint]` or either is 0, the misorientation comparison is skipped. So a voxel surrounded by neighbors of a different phase should never be corrected by them. (Strong, easy-to-encode assertion.)
    - **No-correction when no neighbor passes:** if every face-neighbor of a low-CI voxel is invalid (boundary) or different-phase, `bestNeighbor` stays −1 and `copyTuple` is a no-op. (Boundary voxel test.)
  - **Paper companion (Class 3):** EBSD orientation-cleanup of this kind is described in textbook references (Wright/Adams, Schwartz/Kumar/Adams *Electron Backscatter Diffraction in Materials Science*) and is implemented in TSL OIM Analysis as "Neighbor Orientation Correlation Cleanup". The legacy DREAM3D source is the canonical reference for the specific implementation. Recommend developer cite the original SIMPL implementation header and any Rowenhorst/Wright reference.
  - **Analytical (Class 1) micro-fixtures:** a 3×3×3 volume with one bad-CI voxel in the center surrounded by 6 same-phase same-orientation neighbors should produce a single replacement at level 6 (and below). Hand-derivable.
- **Action required:** Developer to confirm a paper reference and to defend or replace the Class-4 recommendation. The current full-pipeline-vs-exemplar test should be supplemented with at least 2–3 small synthetic Class-4 invariant assertions.

## V&V status so far

| Item | Status | Notes |
|---|---|---|
| Algorithm review (`review-algorithm` skill) | Not visible from PR history | No PR explicitly performs the line-by-line review. PR #1523 (refactor) and #1590 (type tightening) did make the algorithm file cleaner, but neither was an audit. |
| Code path coverage (algorithmic) | **Weak** | Only 1 end-to-end pipeline test against an exemplar. No small synthetic fixtures, no boundary-voxel test, no different-phase test, no Level-vs-currentLevel sweep. |
| Code path coverage (SIMPL conversion) | Good | PR #1588 added SIMPL 6.4 + 6.5 conversion test. |
| Exemplar data in Data_Archive | **Yes** | `neighbor_orientation_correlation.tar.gz` referenced in test/CMakeLists.txt. |
| Exemplar provenance documented | Unknown | TBD by inspecting archive contents. |
| Oracle class recorded | **No** | This document is the first to propose one. |
| Toy data / independent expected output (Step 0 c) | No | No script or hand-derivation on file. |
| Legacy comparison report (Step 0 e) | No | `compare-legacy-dream3d` has not been run. The PR #1472 EbsdLib float→double migration is the most likely source of small numerical drift vs. SIMPL 6.5.172 and is the minimum justification for running the comparison. |
| Deviation entries (`NeighborOrientationCorrelation-D<N>`) | None | Not yet written. See "Recommended Deviation entries" below for pre-flagged candidates. |
| Documentation currency | Mostly current | Last touched by PR #1543 (pipeline-reference doc cleanup) and PR #1513 (algorithm comment doc). The user-facing description in `docs/NeighborOrientationCorrelationFilter.md` still describes the algorithm correctly given the post-#1535/#1523/#1590 implementation. Does **not** mention that the level decrement is strict (`> Level`, not `>= Level`) — worth a clarification. |
| Verification archive (OneDrive) | No | Not yet created. |

## Gaps to close (to meet Step 0 / Legacy Comparison policy)

1. **Confirm the oracle.** Class 4 (invariant-based) is the recommended starting point; check for a paper or legacy SIMPL doc reference and upgrade to Class 3 if one exists.
2. **Add small synthetic Class-4 assertions to the test file.** Today the only algorithmic test is the full Small IN100 end-to-end. Add at minimum:
   - Idempotence on a uniform-orientation volume.
   - "High-CI voxels are bit-identical after the filter" assertion on the existing pipeline (cheap to add, strong invariant).
   - A 3×3×3 single-bad-voxel hand-derivable test.
   - A two-phase test verifying that a low-CI voxel surrounded by different-phase neighbors is not modified.
3. **Inspect `neighbor_orientation_correlation.tar.gz` and document provenance.** Determine how the exemplar was generated (likely a SIMPL pipeline given the `Exemplar Data / CellData` group naming convention used in the test), what input data was used, and write an Oracle Provenance block for the archive ReadMe.
4. **Run the legacy comparison.** Use `compare-legacy-dream3d` to diff SIMPLNX vs. DREAM3D 6.5.172 on the same Small IN100 input. Expected discrepancies, if any, are most likely to surface as small floating-point drift in the misorientation values driven by the PR #1472 float→double widening (see Deviation D1 below). Possibly also small differences if any rare voxel happens to sit exactly on the `> currentLevel` ↔ `>= currentLevel` boundary (Deviation D2 below).
5. **Produce the Algorithm Relationship one-liner.** Tentative: *"Port — direct translation of the SIMPL `NeighborOrientationCorrelation` filter; misorientation math now goes through `ebsdlib::QuatD` (double precision) instead of `QuatF`. No algorithmic semantic changes since 2025-10-01, only refactors (PR #1523, #1535, #1590) and an EbsdLib API migration (PR #1472)."*
6. **Document the strict-level-inequality semantic in the user-facing markdown** (`docs/NeighborOrientationCorrelationFilter.md`) — the loop is `currentLevel > Level`, so a user setting `Level = 6` gets *zero* iterations, not one.
7. **Archive everything** per `archive-filter-verification` for the OneDrive folder.

## Recommended Deviation entries (proposed, pending legacy comparison)

> **Deviation ID:** `NeighborOrientationCorrelation-D1`
> **Filter UUID:** `4625c192-7e46-4333-a294-67a2eb64cb37`
> **Symptom:** SIMPLNX may produce slightly different replacement decisions on borderline voxels compared to SIMPL 6.5.172.
> **Root cause:** PR #1472 (Update to EbsdLib 2.0.0 API) widened the misorientation calculation inputs from `QuatF` (single precision) to `ebsdlib::QuatD` (double precision). The legacy SIMPL implementation uses single precision. For voxels whose neighbor misorientation is very near the user-supplied `MisorientationTolerance`, the SIMPLNX double-precision value may fall on the opposite side of the threshold from the legacy single-precision value, changing whether the voxel is counted into `neighborSimCount` / `neighborDiffCount` and ultimately whether/which neighbor becomes `bestNeighbor`.
> **Affected users:** Anyone bit-comparing SIMPLNX cleanup output to SIMPL 6.5.172 cleanup output on the same dataset; anyone with a `MisorientationTolerance` very close to a populated misorientation peak in their data.
> **Recommendation:** Trust SIMPLNX. Double-precision misorientation is the more accurate calculation. Document this in the filter docs as a known small-magnitude difference vs. legacy.
> **Status:** Proposed — pending verification via `compare-legacy-dream3d`.

> **Deviation ID:** `NeighborOrientationCorrelation-D2`
> **Filter UUID:** `4625c192-7e46-4333-a294-67a2eb64cb37`
> **Symptom:** "Best neighbor" tie-breaking on equal `neighborSimCount` is not specified.
> **Root cause:** In the inner loop of the algorithm, `bestNeighbor[voxelIndex]` is updated only when `neighborSimCount[faceIndex] > best` (strict greater-than). This means the *first* face neighbor (in the order returned by `faceNeighborInternalIdx`) that achieves the maximum sim-count wins. PR #1523 routed the face-iteration order through the new `NeighborUtilities` helpers; if the helper's iteration order differs from the legacy hard-coded order (`-dims[0]*dims[1]`, `-dims[0]`, `-1`, `+1`, `+dims[0]`, `+dims[0]*dims[1]`), then on tied voxels SIMPLNX will pick a *different* neighbor than legacy — same misorientation distance, different tuple values copied. **This is not a bug**, but it can produce a downstream bit-difference on otherwise-equivalent inputs.
> **Affected users:** Anyone bit-comparing SIMPLNX vs. legacy on a dataset that contains tied-best-neighbor voxels (likely rare in noisy real data, possibly common in synthetic fixtures).
> **Recommendation:** Verify that `NeighborUtilities::faceNeighborInternalIdx` produces the same face order as legacy SIMPL's hand-coded loop. If yes, no Deviation needed (delete this entry). If no, document the new order as the canonical SIMPLNX behavior.
> **Status:** Proposed — pending source-side verification of `NeighborUtilities::faceNeighborInternalIdx` ordering vs. legacy.

> **Deviation ID:** `NeighborOrientationCorrelation-D3`
> **Filter UUID:** `4625c192-7e46-4333-a294-67a2eb64cb37`
> **Symptom:** Strict-vs-inclusive level-loop boundary: the user-facing parameter is "Cleanup Level" but the loop is `for(currentLevel = 6; currentLevel > Level; currentLevel--)`. A user setting `Level = 6` runs zero iterations; `Level = 5` runs exactly one iteration at level 6; etc.
> **Root cause:** Loop semantics — strict `>` rather than `>=`. This matches the legacy SIMPL behavior (presumed), but the user-facing documentation describes the parameter as "Minimum number of neighbor Cells that must have orientations within above tolerance to allow Cell to be changed", which a user might naturally read as inclusive.
> **Affected users:** Anyone setting `Level == 6` and expecting one cleanup pass.
> **Recommendation:** Likely no algorithmic change; clarify in the filter doc that `Level = N` means the loop terminates *after* `currentLevel = N + 1` and that `Level = 6` is therefore a no-op. Confirm SIMPL legacy uses the same strict inequality.
> **Status:** Proposed — pending legacy comparison and developer review.

> **Deviation ID:** `NeighborOrientationCorrelation-D4`
> **Filter UUID:** `4625c192-7e46-4333-a294-67a2eb64cb37`
> **Symptom:** Mixed-phase neighbor handling — when `cellPhases[a] != cellPhases[b]` or either is 0, the misorientation is *not* computed and `axisAngle[3]` retains its sentinel value `std::numeric_limits<double>::max()`.
> **Root cause:** In the first comparison (voxel vs. neighbor), `axisAngle[3] > misorientationToleranceR` will then be true and the voxel-vs-neighbor pair is counted into `neighborDiffCount`. In the second comparison (neighbor vs. co-neighbor), `axisAngle[3] < misorientationToleranceR` will then be false and the pair is *not* counted into `neighborSimCount`. The asymmetry is intentional (mixed-phase pairs should look "different"), but it is not documented and not directly tested.
> **Affected users:** Multi-phase EBSD datasets where cleanup is run across phase boundaries.
> **Recommendation:** Add a multi-phase Class-4 invariant test (per Gap #2 above) and document the mixed-phase semantics in the user-facing markdown.
> **Status:** Proposed — pending developer review of legacy SIMPL behavior.

> **Deviation ID:** `NeighborOrientationCorrelation-D5`
> **Filter UUID:** `4625c192-7e46-4333-a294-67a2eb64cb37`
> **Symptom:** Per-Laue-group dispatch: the algorithm uses `crystalStructures[cellPhases[voxelIndex]]` to index into `LaueOps::GetAllOrientationOps()` and call `calculateMisorientation`. Voxels of an unsupported / `UnknownCrystalStructure` Laue class will dispatch to whatever ops object lives at that index in `GetAllOrientationOps()`.
> **Root cause:** No defensive check on `laueClass` before the `orientationOps[laueClass]->calculateMisorientation` call. Behavior on out-of-range Laue indices is delegated entirely to EbsdLib.
> **Affected users:** Anyone with a `CrystalStructures` ensemble array containing values outside the legitimate Laue-class range.
> **Recommendation:** Either add a preflight check that all `crystalStructures` values are in the supported range, or document that out-of-range Laue indices produce undefined behavior. (Likely identical to legacy SIMPL, so probably document rather than change.)
> **Status:** Proposed — pending developer review.
