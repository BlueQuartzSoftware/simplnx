# Retroactive V&V: ReadAngDataFilter

*Report status:* **DRAFT**. Generated from git-history and source-tree inspection. Developer must confirm or correct the Oracle class, Algorithm Relationship, and the V&V status entries.

## Metadata

| Field | Value |
|---|---|
| SIMPLNX UUID | `5b062816-79ac-47ce-93cb-e7966896bcbd` |
| SIMPLNX ClassName | `ReadAngDataFilter` |
| SIMPLNX Human Name | Read EDAX EBSD Data (.ang) |
| SIMPL UUID | `b8e128a8-c2a3-5e6c-a7ad-e4fb864e5d40` (from `simpl_conversion/6_5/ReadAngDataFilter.json`) |
| SIMPL ClassName | `ReadAngData` (per the SIMPL conversion JSON) |
| SIMPL Human Name | Import EDAX EBSD Data (.ang) (per the SIMPL conversion JSON) |
| Plugin | OrientationAnalysis |

### Source files scanned

- `src/Plugins/OrientationAnalysis/src/OrientationAnalysis/Filters/ReadAngDataFilter.{hpp,cpp}`
- `src/Plugins/OrientationAnalysis/src/OrientationAnalysis/Filters/Algorithms/ReadAngData.{hpp,cpp}`
- `src/Plugins/OrientationAnalysis/test/ReadAngDataTest.cpp`
- `src/Plugins/OrientationAnalysis/test/simpl_conversion/6_5/ReadAngDataFilter.json`
- `src/Plugins/OrientationAnalysis/test/simpl_conversion/6_4/ReadAngDataFilter.json`
- `src/Plugins/OrientationAnalysis/docs/ReadAngDataFilter.md`
- `src/Plugins/OrientationAnalysis/pipelines/EBSD_File_Processing/Read_EDAX_Ang_File.d3dpipeline`
- `src/Plugins/OrientationAnalysis/src/OrientationAnalysis/utilities/EbsdReaderUtilities.hpp` (touched by PR #1586 for phase-info display)

## Algorithm Relationship

- **Tentative classification:** **Port** — the SIMPLNX filter is a thin wrapper around the canonical EbsdLib `ebsdlib::AngReader` (formerly `AngReader` before the EbsdLib 2.0.0 namespace migration in PR #1472). It does not implement any `.ang` parsing math itself; it delegates to EbsdLib's reader and then copies/repacks the parsed buffers into the SIMPLNX DataStructure (Image Geometry + Cell Attribute Matrix + Cell Ensemble Attribute Matrix).
- **Evidence:** `Algorithms/ReadAngData.cpp` is 192 lines and is essentially: open reader, call `readFile()`, call `loadMaterialInfo()` (copies phase metadata into the ensemble AM), call `copyRawEbsdData()` (copies Phi1/Phi/Phi2/IQ/CI/SEM/Fit/X/Y/PhaseData into the cell AM). All file-format intelligence lives in `EbsdLib/IO/TSL/AngReader.h`.
- **Action required:** Confirm by inspecting the SIMPL `ReadAngData` source (UUID `b8e128a8-c2a3-5e6c-a7ad-e4fb864e5d40`) and verifying that the SIMPL implementation also delegates to the same EbsdLib reader. If yes, the only meaningful differences between SIMPL and SIMPLNX are (a) the destination data-structure shape and (b) any per-array semantics (phase clamping, error mapping). The legacy comparison (`compare-legacy-dream3d`, Step 0 e) should be straightforward.

## PRs inspected (since 2025-10-01)

> Pruned: pure-style/repo-wide refactor PRs (#1457 static-inline cleanup, #1501 Vec3 unification, #1524 filter-tag rename, #1538 zlib tar extraction) are listed at the bottom of this section but not detailed individually — they did not change the behavior of this filter.

### PR #1438 — *"ENH: Microtexture related filter cleanup"* — merged 2025-10-25

- **Files in this filter:** algorithm header (`Algorithms/ReadAngData.hpp`), filter (`ReadAngDataFilter.cpp`)
- **Diff size:** 2 files, very small (one-line include-syntax changes)
- **Change nature:** Pure include-syntax fix per the PR's `STY: Use proper include syntax for EbsdLib since it is an external library` bullet — switched `#include "EbsdLib/IO/TSL/AngReader.h"` to `#include <EbsdLib/IO/TSL/AngReader.h>` in both files. No algorithmic change to ReadAng. The bulk of #1438 is for sibling Microtexture filters (CAxisSegmentFeatures, ComputeAvgOrientations, etc.).
- **V&V content:** None for ReadAng. Listed for completeness because the policy's pruning rules call for explicit inspection of #1438.

### PR #1462 — *"VV: Read EDAX EBSD Data (.ang)"* — merged 2025-11-11

- **Files in this filter:** test (`ReadAngDataTest.cpp`) +64/-6, plus `test/CMakeLists.txt` (replaced `6_6_read_ang_data.tar.gz` with `read_ang_test.tar.gz`)
- **Change nature:** **This is the explicit V&V PR for this filter.** Joey Kleingers replaced the legacy `6_6_read_ang_data` exemplar (which had been generated from DREAM3D 6.6) with a new `read_ang_test` archive generated from NX itself. The PR also added two new error-path test cases:
  - `OrientationAnalysis::ReadAngData: Invalid Phase` — exercises the `-150` error code (phase-info / loadMaterialInfo failure path).
  - `OrientationAnalysis::ReadAngData: Invalid Columns & Rows` — exercises the `-600` error code (header dimension parse failure).
- The original "Valid Execution" test was renamed to "Exemplary Test" and re-pointed at the new exemplar (`read_ang_test/read_ang_test.dream3d`, `read_ang_test/read_ang_test.ang`).
- **V&V content:** **High.** This PR is the closest thing in the entire history to a per-filter V&V deliverable. **However**, by replacing the 6.6 exemplar with an NX-generated exemplar, it consciously breaks the "compare to legacy DREAM3D" oracle and replaces it with a "regression-test against past NX" oracle (Class 5, regression / golden-master). That choice is defensible for a thin wrapper around EbsdLib, but the policy maintainer should confirm.
- **Provenance question:** the new `read_ang_test.tar.gz` (SHA512 `de7cd89d925da01f291f44686964ec89d469659d0005219f9869afe26b8f62af278461ac3f5deb3afe7f3e65ec074ab3a1357d77a1a5f92eb3a1ea8cc5e4b236`) was uploaded with this PR; engineer must inspect the archive to confirm whether a ReadMe / pipeline-of-record was bundled.

### PR #1472 — *"ENH: Update to EbsdLib 2.0.0 API"* — merged 2025-11-24 *(broad refactor, exception flagged because EbsdLib 2.0.0 may have changed AngReader semantics)*

- **Files in this filter:** algorithm (.hpp, .cpp), filter (.cpp)
- **Diff size:** Mechanical namespace migration (~30 lines across the three files)
- **Change nature:** Pure rename. Every reference to `AngReader`, `AngPhase`, `EbsdLib::AngFile::*`, `EbsdLib::Ang::*`, and `EbsdLib::CrystalStructure::*` was rewritten to its `ebsdlib::*` namespace-qualified equivalent. **No change to call sequence, parsing logic, error codes, array layout, or per-record semantics.** The downstream EbsdLib 2.0.0 may have implementation changes inside `AngReader` itself, but those are outside SIMPLNX and would not appear in this diff.
- **V&V content:** None directly, but **a Deviation entry candidate**: if EbsdLib 2.0.0 changed any per-record parsing behavior (number-format strictness, treatment of malformed lines, endianness handling for any binary `.ang` variant) relative to the EbsdLib version used by DREAM3D 6.5.172, the difference will surface as a Deviation between SIMPLNX (using EbsdLib 2.0.0+) and legacy. Engineer should diff the EbsdLib `AngReader.{cpp,h}` between the legacy DREAM3D pinned version and 2.0.0 to enumerate possibilities.

### PR #1476 — *"BUG/ENH: Fix Backwards Pipeline Compatibility and Add Testing"* — merged 2026-01-06

- **Files in this filter:** filter (`ReadAngDataFilter.cpp`), 1 line change
- **Diff size:** 1 file, +1/-1
- **Change nature:** SIMPL-conversion fix. The `FromSIMPLJson` converter for the `DataContainerName` parameter was switched from `DataContainerCreationFilterParameterConverter` to `DCPathBuilderFilterParameterConverter`. This makes the legacy SIMPL `DataContainerName` map correctly into the NX `output_image_geometry_path` parameter (which is a full DataPath, not just a name).
- **V&V content:** **Pipeline-conversion correctness only.** Does not affect the actual `.ang` parsing or per-record behavior of the filter when run from a native NX pipeline.

### PR #1566 — *"BUG: Fix parameter linking in WritePoleFigure Filter"* — merged 2026-03-23

- **Files in this filter:** docs only (`docs/ReadAngDataFilter.md`, +17 lines)
- **Change nature:** Documentation addition. Added a "Note on .ang file Data Ordering" block listing the per-line column order (phi1, Phi, phi2, x, y, IQ, CI, phase, SEM, Fit). Despite the PR title referring to a different filter, this commit also bundles a doc improvement for ReadAng.
- **V&V content:** Documentation currency only. The 10-column order it documents matches what `Algorithms/ReadAngData.cpp` actually reads from the EbsdLib reader (Phi1, Phi, Phi2 are pulled separately and re-packed into a 3-component EulerAngles array; the rest map 1:1).

### PR #1586 — *"BUG: Fix display of Phase information Ang,Ctf,GrainMapper readers"* — merged 2026-04-14

- **Files in this filter:** filter (`ReadAngDataFilter.cpp`) +4 lines, algorithm (`Algorithms/ReadAngData.cpp`) -1 line, plus a non-trivial change to `utilities/EbsdReaderUtilities.hpp` (shared by Ang, Ctf, GrainMapper readers)
- **Diff size:** small for ReadAng itself; the substantive change is in the shared utility
- **Change nature:** **Material bug fix** in two areas:
  1. Added a preflight error (-19501) to `ReadAngDataFilter::preflightImpl` that fires when the input `.ang` file is missing the `GRID` header key — previously a malformed-header file could pass preflight and fail (or worse, succeed silently with garbage geometry) at execute time.
  2. Fixed a phase-information display ordering bug in `EbsdReaderUtilities.hpp::GeneratePreflightPhaseInformation()` — phases now appear in the order declared in the file rather than re-sorted by Laue index. This is a UI/preflight concern; it does not change the persisted DataStructure values, only what the user sees in the preflight panel.
- **V&V content:** **Medium.** The new GRID-header check is a correctness fix that hardens the filter against a class of malformed inputs. **This is a Deviation entry candidate** vs. SIMPL 6.5.172, which presumably does not have the GRID-header guard.

### PR #1588 — *"ENH: SIMPL Backwards Compatibility Test Redesign"* — merged 2026-04-22

- **Files in this filter:** two new fixture files
  - `test/simpl_conversion/6_4/ReadAngDataFilter.json`
  - `test/simpl_conversion/6_5/ReadAngDataFilter.json`
- **Note (source-of-truth correction):** The brief expected this PR to also add a "SIMPL Backwards Compatibility" `TEST_CASE` to `ReadAngDataTest.cpp`. **It did not** — `ReadAngDataTest.cpp` was not modified by #1588, and `grep` for "Backwards Compatibility" inside that test file returns no hits. The two SIMPL conversion JSONs were added, but the per-filter `DYNAMIC_SECTION` test stub is missing for ReadAng (compare with sibling filters that did get one). This is a gap.
- **Change nature:** **Test fixture addition only.** The 6.4 and 6.5 JSONs supply hand-crafted SIMPL pipeline fragments whose conversion to SIMPLNX Arguments can be exercised by the redesigned-fixture test framework. The fact that the JSONs exist suggests the new framework can find and consume them automatically, even without an explicit per-filter TEST_CASE; engineer should confirm this against the framework.
- **V&V content:** **Pipeline-conversion correctness only** — once wired up, this verifies that opening a legacy SIMPL `.json` pipeline that referenced the old `ReadAngData` filter (UUID `b8e128a8-c2a3-5e6c-a7ad-e4fb864e5d40`) produces a SIMPLNX `ReadAngDataFilter` instance with parameters mapped correctly. It does **not** verify that the filter's per-pixel output matches legacy.

### Pruned PRs (touched the file but not behaviorally relevant to this filter)

| PR | Subject | Why pruned |
|---|---|---|
| #1457 | Clean up 'static inline' from filter headers | Style |
| #1501 | Combine Matrix3x1, Point3D, Vec3 into a Vec3<T> | Refactor (1 line removed in ReadAngData.cpp) |
| #1524 | Fixed filter tags to consistently use the full filter name | Test cosmetic |
| #1538 | Replace cmake subprocess tar.gz extraction with zlib | Test infrastructure (TestFileSentinel signature change) |

## Test coverage detected

`ReadAngDataTest.cpp` contains 3 `TEST_CASE`s (108 lines total, all exemplar-driven):

1. `OrientationAnalysis::ReadAngData: Exemplary Test` — happy-path. Reads `read_ang_test.ang`, compares the resulting cell attribute matrix against the bundled exemplar `read_ang_test.dream3d` via `CompareExemplarToGeneratedData`. *(Renamed and re-pointed in PR #1462.)*
2. `OrientationAnalysis::ReadAngData: Invalid Phase` — error path. Feeds `ang_unit_test_invalid_phase.ang` and asserts `executeResult.result.errors()[0].code == -150`. *(Added by PR #1462.)*
3. `OrientationAnalysis::ReadAngData: Invalid Columns & Rows` — error path. Feeds `ang_unit_test_invalid_cols_rows.ang` and asserts `executeResult.result.errors()[0].code == -600`. *(Added by PR #1462.)*

There is **no** SIMPL backwards-compatibility `TEST_CASE` in `ReadAngDataTest.cpp` despite the JSONs added by PR #1588 — see the PR #1588 sub-section for the source-of-truth correction.

There is also **no** unit test that:
- Tests the new GRID-header preflight guard added in PR #1586 (no `Missing GRID` test case)
- Tests phase-information display ordering (the other half of PR #1586)
- Tests a multi-phase `.ang` file independently of the single happy-path exemplar
- Hand-crafts a minimal `.ang` file with known content and verifies the produced DataStructure matches by-hand expected values (Class 1 oracle)

## Exemplar archive

- **Archive name:** `read_ang_test.tar.gz`
- **SHA512:** `de7cd89d925da01f291f44686964ec89d469659d0005219f9869afe26b8f62af278461ac3f5deb3afe7f3e65ec074ab3a1357d77a1a5f92eb3a1ea8cc5e4b236`
- **Referenced in:** `src/Plugins/OrientationAnalysis/test/CMakeLists.txt` (line 152)
- **Provenance (per PR #1462 description):** "Update unit test to rely on data generated from NX instead of 6.6." So the exemplar `.dream3d` was produced by NX itself, not by legacy DREAM3D. **This is a regression / golden-master oracle, not a legacy-comparison oracle.**
- **Contents (inferred):** at minimum
  - `read_ang_test.ang` — happy-path input file
  - `read_ang_test.dream3d` — exemplar output (cell + ensemble AM)
  - `ang_unit_test_invalid_phase.ang` — malformed phase block
  - `ang_unit_test_invalid_cols_rows.ang` — malformed dimensions
- **Action required:** Download the archive locally and inspect for:
  - an inner `ReadMe.md` (provenance per Step 0)
  - the NX pipeline that generated `read_ang_test.dream3d` (so it can be re-run to regenerate the exemplar)
  - the original source of `read_ang_test.ang` (was it a real EDAX scan, or hand-crafted from a 6.6 exemplar?)
  - both of the malformed inputs — confirm they were created by hand and document what was changed relative to a valid file

## Oracle classification (tentative)

I/O filters are an unusual oracle case. The brief proposed Class 2 (Reference-implementation) as the natural oracle, with Class 4 (Invariant) as a companion and Class 1 (Analytical / hand-crafted) as a defensible alternative. After inspecting the source, the recommendation is:

- **Recommended primary class: 2 (Reference-implementation oracle).** The reference implementation is **EbsdLib's `ebsdlib::AngReader` itself**, plus EDAX OIM Analysis as a second independent reference. Because SIMPLNX delegates all parsing to EbsdLib, the question "is the SIMPLNX output correct?" reduces to "is EbsdLib's parse correct, and did SIMPLNX repack the buffers without mutation?" The Class-2 oracle for the second half of that question can be constructed by:
  1. Running EbsdLib's `AngReader` standalone on the same `.ang` file
  2. Pulling the per-array buffers via `getPointerByName(...)`
  3. Comparing element-by-element to the SIMPLNX cell AM (with one documented exception: the phase-clamp `if(phasePtr[i] < 1) phasePtr[i] = 1;` in `copyRawEbsdData` deliberately deviates from raw EbsdLib output — that clamp is a SIMPLNX-side correction for invalid phase indices and should be a documented Deviation).
- **Recommended companion class: 4 (Invariant-based).** Natural invariants for any successful `.ang` parse:
  - `cellAM.tupleCount == imageGeom.dims[0] * imageGeom.dims[1] * imageGeom.dims[2]`
  - `cellAM.dims == {nRows × nCols × 1}` (Z is always 1 for a single-slice `.ang`)
  - All Phi1/Phi/Phi2 values fall in `[0, 2π)` (radians, per EDAX TSL convention)
  - All Phase indices fall in `[1, nPhases]` (after the SIMPLNX clamp; raw could include 0 or negative)
  - `crystalStructures[0] == UnknownCrystalStructure` and `materialNames[0] == "Invalid Phase"` (per `loadMaterialInfo`)
  - `latticeConstants` row 0 is all zeros (per `loadMaterialInfo`)
  - `xPosition.min() >= 0`, `yPosition.min() >= 0`, monotone in scan order
- **Defensible alternative: 1 (Analytical / hand-crafted).** A 2×2 (or 3×3) `.ang` file with hand-chosen Euler angles, IQ, CI, and a single phase has an exactly-predictable DataStructure. This is the strongest oracle for an I/O filter and is cheap to author.
- **Action required:** Developer to choose between Class 2 (delegate to EbsdLib as reference), Class 1 (hand-crafted minimal `.ang`), or both. Class 4 is essentially free to add as a few `REQUIRE(...)` lines in the existing exemplar test and should be added regardless.

## V&V status so far

| Item | Status | Notes |
|---|---|---|
| Algorithm review (`review-algorithm` skill) | Not visible from PR history | No PR explicitly performs the line-by-line review of `Algorithms/ReadAngData.cpp`. The algorithm is small (192 lines) so the review is low cost. |
| Code path coverage (algorithmic) | Partial | Happy path + 2 error paths are covered. The new GRID-header guard added in PR #1586 has **no test**. Multi-phase files, hex-grid rejection, and `phasePtr[i] < 1` clamp paths are not explicitly exercised. |
| Code path coverage (SIMPL conversion) | Fixtures present, test stub missing | PR #1588 added the 6.4 and 6.5 conversion JSONs but did **not** add a per-filter `DYNAMIC_SECTION` SIMPL-Backwards test to `ReadAngDataTest.cpp`. Engineer must confirm whether the redesigned framework auto-discovers the JSONs without per-filter glue. |
| Exemplar data in Data_Archive | **Yes** | `read_ang_test.tar.gz` referenced in test/CMakeLists.txt. Generated from NX (not legacy), per PR #1462 description. |
| Exemplar provenance documented | Unknown | TBD by inspecting archive contents. The PR description says "data generated from NX" but does not commit the generating pipeline alongside the test code. |
| Oracle class recorded | **No** | This document is the first to propose one. |
| Toy data / independent expected output (Step 0 c) | No | No hand-crafted minimal `.ang` exists. |
| Reference-implementation comparison (vs. EbsdLib standalone or OIM Analysis) | No | No script or test compares SIMPLNX output to a standalone EbsdLib AngReader run or to EDAX OIM Analysis. |
| Legacy comparison report (Step 0 e) | **No, and explicitly side-stepped** | PR #1462 *replaced* the 6.6 exemplar with an NX-generated one. The legacy comparison was never run; running it now is still valuable for the Deviation list. |
| Deviation entries (`ReadAngData-D<N>`) | None | Not yet written. PR #1586's GRID-header guard and PR #1462's exemplar replacement are both candidates. |
| Documentation currency | Probably current | Updated by PR #1566 (column-order block). Needs accuracy audit per `review-filter-docs`. |
| Verification archive (OneDrive) | No | Not yet created. |

## Gaps to close (to meet Step 0 / Legacy Comparison policy)

These map to Step 0 a–e of the V&V policy:

1. **(Step 0 a — algorithm review.)** Run `review-algorithm` against the small (192-line) `Algorithms/ReadAngData.cpp`. Pay particular attention to:
   - The `if(phasePtr[i] < 1) phasePtr[i] = 1;` clamp — is it correct, or should invalid phases be flagged as a separate "bad" sentinel? This is a behavior choice that diverges from raw EDAX semantics.
   - Lack of cancel-check between large array copies (`std::copy(fComp0, fComp0 + totalCells, ...)`) — for large scans this is a multi-second un-cancellable region.
   - No bounds check on `getPointerByName(...)` return values (assumed non-null based on `readFile()` success).

2. **(Step 0 b — confirm the oracle.)** Pick Class 2 (delegate to EbsdLib AngReader as reference) **and** Class 4 (invariants), with optional Class 1 (hand-crafted toy `.ang`) for the strongest possible coverage. Defend in the V&V archive ReadMe.

3. **(Step 0 c — toy / independent expected.)** Write a 2×2 `.ang` file with known Euler angles, IQ, CI, phase indices, and one phase. Hand-derive the expected DataStructure (12 voxels' worth) and add it as a unit test. This becomes the Class-1 oracle of record.

4. **(Step 0 d — invariant assertions.)** Add the seven invariants listed in the Oracle section as `REQUIRE(...)` lines in the existing "Exemplary Test", or as a new dedicated `[invariants]`-tagged test case. Cost is ~30 lines.

5. **(Step 0 e — legacy comparison.)** Use `compare-legacy-dream3d` to diff SIMPLNX vs. DREAM3D 6.5.172 on the **same** `.ang` file. Expected outcomes:
   - Likely-identical: Phi1, Phi, Phi2, X, Y, IQ, CI, SEM, Fit (all of these are direct EbsdLib-buffer copies in both versions).
   - Possibly-different: Phase array (depending on whether legacy also clamps `< 1` to `1`).
   - Possibly-different: Cell Ensemble Attribute Matrix layout (especially if 6.5.172 wrote the phase-info arrays in a different order than NX does after the PR #1586 fix — see Deviation D2 below).
   - Possibly-different: error behavior on the malformed-cols/rows and missing-GRID inputs (legacy may not have the GRID guard added in PR #1586).

6. **(SIMPL conversion test wire-up.)** Confirm whether the PR #1588 redesigned framework auto-discovers `simpl_conversion/6_5/ReadAngDataFilter.json` and `simpl_conversion/6_4/ReadAngDataFilter.json` without a per-filter `TEST_CASE`. If not, add a `DYNAMIC_SECTION` test like the sibling filters that already have one (e.g., `CAxisSegmentFeaturesTest.cpp` after #1588).

7. **(GRID-header test.)** Add a small fourth test case that feeds an `.ang` file missing the `GRID` header key and asserts `errors()[0].code == -19501`. Currently the PR #1586 fix has no test guard.

8. **(Inspect `read_ang_test.tar.gz` and document provenance.)** Determine how the exemplar `.dream3d` was generated, whether a generating pipeline `.d3dpipeline` was committed (none is visible alongside the test code), and what the original source of the `.ang` file was. Write an Oracle Provenance block for the archive ReadMe.

9. **(Produce the Algorithm Relationship one-liner.)** Tentative: *"Port — thin SIMPLNX wrapper around EbsdLib's `ebsdlib::AngReader`, with one corrective behavior addition (PR #1586 GRID-header preflight guard) and one documented behavioral choice (phase-index clamp `< 1 → 1`)."*

10. **(Archive everything.)** Per `archive-filter-verification` for the OneDrive folder.

## Recommended Deviation entries (proposed, pending legacy comparison)

> **Deviation ID:** `ReadAngData-D1`
> **Filter UUID:** `5b062816-79ac-47ce-93cb-e7966896bcbd`
> **Symptom:** SIMPLNX rejects an `.ang` file that is missing the `GRID` header key during preflight (error code `-19501`); legacy DREAM3D 6.5.172 likely accepts the file and either fails later with a less informative error or silently produces a degenerate Image Geometry.
> **Root cause:** Improvement in SIMPLNX — PR #1586 added an explicit preflight guard to surface the malformed-header condition early and informatively.
> **Affected users:** Anyone whose pipeline ingests `.ang` files from third-party converters that may omit the `GRID` field. NX users get a clear error; legacy users may have been silently consuming bad data.
> **Recommendation:** Trust SIMPLNX. Optionally backport the GRID-header check to a 6.5.172 patch release if user demand exists.
> **Status:** Proposed — pending verification that 6.5.172 lacks the guard.

> **Deviation ID:** `ReadAngData-D2`
> **Filter UUID:** `5b062816-79ac-47ce-93cb-e7966896bcbd`
> **Symptom:** The Cell Ensemble Attribute Matrix's phase-information arrays (CrystalStructures, MaterialNames, LatticeConstants) appear in a different order in the preflight phase-display panel between SIMPLNX (post-PR #1586) and legacy.
> **Root cause:** PR #1586 fixed a display ordering bug in `EbsdReaderUtilities.hpp::GeneratePreflightPhaseInformation()`. SIMPLNX now displays phases in the order declared in the `.ang` file (correct); legacy DREAM3D 6.5.172 may display them re-sorted by Laue index (incorrect).
> **Affected users:** Anyone visually inspecting the phase panel during preflight; downstream serialized order in the ensemble AM should be unaffected (storage is keyed by phase index, not by display order).
> **Recommendation:** Trust SIMPLNX. The preflight display value is a UI concern, but verify by inspection that the storage order in the ensemble AM is identical between SIMPLNX and legacy.
> **Status:** Proposed — pending verification of legacy display behavior and confirmation that no on-disk ordering changed.

> **Deviation ID:** `ReadAngData-D3`
> **Filter UUID:** `5b062816-79ac-47ce-93cb-e7966896bcbd`
> **Symptom:** Voxels whose raw EDAX phase index is `< 1` (e.g., 0 or negative — typically used by EDAX as an "indexed but no phase" marker) are written into the SIMPLNX `Phases` array with value `1`, not their raw value.
> **Root cause:** Deliberate SIMPLNX-side correction in `Algorithms/ReadAngData.cpp` lines 122–127: `if(phasePtr[i] < 1) { phasePtr[i] = 1; } targetArray[i] = phasePtr[i];`. This treats invalid-phase voxels as belonging to the first phase rather than as bad voxels.
> **Affected users:** Anyone whose downstream analysis needs to identify and mask out voxels that EDAX flagged as not having a valid phase. With this clamp, those voxels are indistinguishable from valid phase-1 voxels in the ensemble AM.
> **Recommendation:** **Decision required.** Either (a) confirm legacy DREAM3D does the same clamp (then no Deviation; document as expected behavior), or (b) replace the clamp with a sentinel value (e.g., 0) and add a "BadPhaseMask" output array for downstream filters to use. Option (b) is the more correct behavior but is a breaking change for downstream pipelines.
> **Status:** Proposed — pending verification of legacy clamp behavior.

> **Deviation ID:** `ReadAngData-D4` *(provisional, pending EbsdLib version diff)*
> **Filter UUID:** `5b062816-79ac-47ce-93cb-e7966896bcbd`
> **Symptom:** Per-record values may differ between SIMPLNX (using EbsdLib 2.0.0+) and legacy (using whatever version DREAM3D 6.5.172 pinned), in cases where EbsdLib 2.0.0 changed parsing strictness, malformed-line handling, or numeric edge-case behavior.
> **Root cause:** PR #1472 migrated SIMPLNX to EbsdLib 2.0.0. The diff visible in SIMPLNX is pure renames, but the upstream EbsdLib 2.0.0 may carry parsing fixes.
> **Affected users:** Anyone consuming `.ang` files with malformed lines, mixed encodings, or unusual numeric formatting (e.g., locale-dependent decimal separators).
> **Recommendation:** Diff EbsdLib `AngReader.{cpp,h}` between the legacy DREAM3D pinned version and 2.0.0. Enumerate any behavioral changes and either promote them to dedicated Deviation entries or close this entry as "no upstream behavioral change found."
> **Status:** Proposed — requires upstream-EbsdLib diff to confirm.
