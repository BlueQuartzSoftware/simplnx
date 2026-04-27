# Retroactive V&V: RotateSampleRefFrameFilter

*Report status:* **DRAFT**. Generated from git-history and source-tree inspection. Developer must confirm or correct the Oracle class, Algorithm Relationship, and the V&V status entries.

## Metadata

| Field | Value |
|---|---|
| SIMPLNX UUID | `d2451dc1-a5a1-4ac2-a64d-7991669dcffc` |
| SIMPLNX ClassName | `RotateSampleRefFrameFilter` |
| SIMPLNX Human Name | Rotate Sample Reference Frame |
| SIMPL UUID | *(TBD — confirm in legacy SIMPL repo)* |
| SIMPL ClassName | `RotateSampleRefFrame` (legacy SIMPL Sampling/OrientationAnalysis filter — name preserved) |
| SIMPL Human Name | *(TBD — confirm in legacy SIMPL repo)* |
| Plugin | SimplnxCore |

### Source files scanned

- `src/Plugins/SimplnxCore/src/SimplnxCore/Filters/RotateSampleRefFrameFilter.{hpp,cpp}`
- `src/Plugins/SimplnxCore/src/SimplnxCore/Filters/Algorithms/RotateSampleRefFrame.{hpp,cpp}`
- `src/Plugins/SimplnxCore/test/RotateSampleRefFrameTest.cpp`
- `src/Plugins/SimplnxCore/test/simpl_conversion/6_5/RotateSampleRefFrameFilter.json`
- `src/Plugins/SimplnxCore/test/simpl_conversion/6_4/RotateSampleRefFrameFilter.json`
- `src/Plugins/SimplnxCore/docs/RotateSampleRefFrameFilter.md`
- Underlying utility: `src/simplnx/Utilities/ImageRotationUtilities.*` (where the actual transformation/resampling math lives — `CreateRotationArgs`, `GenerateRotationTransformationMatrix`, `RotateImageGeometryWithNearestNeighbor`)

## Algorithm Relationship

- **Tentative classification:** **Port** of the legacy SIMPL `RotateSampleRefFrame` filter, with the simplnx implementation **delegating the math to a shared `ImageRotationUtilities` library** (used by other resample/transform filters in simplnx). The UUID is *not* a SIMPL-preserved UUID (it is the new simplnx UUID), but the `LegacyUUIDMapping` and `FromSIMPLJson` plumbing make it the canonical successor to the SIMPL filter. As of January 2026 (PR #1301) the executeImpl body was extracted into the standard Algorithm class pattern; the actual rotation/resampling math is unchanged.
- **Evidence:**
  - `executeImpl()` is now a thin shim that builds a `RotateSampleRefFrameInputValues` struct and calls `RotateSampleRefFrame{...}()`.
  - The Algorithm operator()() delegates to `ImageRotationUtilities::CreateRotationArgs`, `GenerateRotationTransformationMatrix`, and the type-dispatched `RotateImageGeometryWithNearestNeighbor` worker — i.e. nearest-neighbor resampling driven by a 4×4 transformation matrix.
  - `FromSIMPLJson()` exists and maps SIMPL keys (`RotationRepresentationChoice`, `RotationAngle`, `RotationAxis`, `RotationTable`, `CellAttributeMatrixPath`) onto simplnx parameter keys.
  - The user-facing doc carries a notable warning: *"As of July 2023, this filter is only verified to work with a rotation angle of 90 or 180 degrees, a rotation axis of (010) || (100) || (001) The origin must also be (0, 0, 0)."* — this is a strong V&V signal that arbitrary-angle behavior has never been formally validated.
- **Action required:** Confirm by reading the corresponding SIMPL filter source (DREAM3D 6.5.171 `RotateSampleRefFrame`) and running `compare-legacy-dream3d` on a 90°-around-Z toy dataset. Also clarify whether `ImageRotationUtilities` was ported from SIMPL or is a simplnx invention — that determines whether arbitrary-angle output is "Port" or "New" behavior.

## PRs inspected (since 2025-10-01)

> Pruned legend at the bottom of this section. The promoted PRs below are the ones that materially affected this filter's behavior, tests, or organization.

### PR #1301 — *"ENH: Move Execution to Algorithm Classes"* — merged 2026-01-08 *(broad refactor, exception flagged because this PR was the actual Algorithm-class extraction event for this filter — the central goal of issue #1284)*

- **Files in this filter:** filter (.hpp -6, .cpp -96/+ refactor), **NEW** algorithm (.hpp +57, .cpp +107)
- **Diff size:** 4 files, +187 / -79 lines
- **Change nature:** **Structural extraction.** This PR created `Algorithms/RotateSampleRefFrame.{hpp,cpp}` and moved the body of `executeImpl()` into the new Algorithm class operator()(). The filter `executeImpl()` was reduced to a thin shim that constructs a `RotateSampleRefFrameInputValues` struct and invokes the Algorithm. PR description: *"Fixes 3 from: #1284"* — i.e. this is exactly the issue-#1284 work item the project guidelines describe.
- **V&V content:** **Structural / no algorithmic change** — the rotation math itself is untouched. However, this is the materially important "lifecycle" event for this filter under issue #1284, so it must be called out.

### PR #1465 — *"TEST: Update RotateSampleRefFrame unit test to also test 45 & 180 in all directions"* — merged 2025-11-10

- **Files in this filter:** test (.cpp) **+38 / -132**, test/CMakeLists.txt +1
- **Diff size:** 2 files, +39 / -132 lines (net deletion — the prior test was 132 lines longer; test was rewritten to use Catch2 `GENERATE` to parameterize across angle × axis × keep-origin combinations)
- **Change nature:** **Material test expansion + new exemplar archive.** The unit test was rewritten to drive 18 `GENERATE` cases (3 angles × 3 axes × 2 keep-origin states). The companion archive `Rotate_Sample_Ref_Frame_Test_v2.tar.gz` was added in `test/CMakeLists.txt`. PR notes: *"Rotate_Sample_Ref_Frame_Test_v2.tar.gz needs to be removed from the DataArchive after this PR gets approved and merged in."*
- **V&V content:** **High** — this is the only PR in the window that actually expanded what the filter is verified against. The test now compares against exemplars for 90°/180° **and 45°** rotations around X/Y/Z, including the keep-origin variant. The 45° case directly contradicts the user-facing doc warning that only 90/180 are verified — engineer should reconcile the two.

### PR #1476 — *"BUG/ENH: Fix Backwards Pipeline Compatibility and Add Testing"* — merged 2026-01-06

- **Files in this filter:** filter (.cpp) **+12 / -2**
- **Diff size:** 1 file, +14 / -2 lines
- **Change nature:** **`FromSIMPLJson` correctness fix.** The `FromSIMPLJson()` body was refactored to handle the case where SIMPL 6.5 pipelines do not contain the `RotationRepresentationChoice` or `RotationTable` keys (those were added later); the converter now skips them gracefully when absent and only includes them in the result when valid.
- **V&V content:** **Pipeline-conversion correctness only** — does not change the algorithm. Required for legacy 6.5 pipelines that pre-date the choice/table fields to round-trip into simplnx without spurious errors.

### PR #1535 — *"ENH: Remove redundant preflight checks that are already done in the parameter"* — merged 2026-02-18 *(broad refactor, exception flagged because this PR removed substantive preflight logic from this filter)*

- **Files in this filter:** filter (.cpp) **+3 / -7**
- **Diff size:** 1 file, +3 / -7 lines
- **Change nature:** **Preflight cleanup.** Removed redundant checks that the GeometrySelectionParameter and DataGroupCreationParameter already enforce. Net effect: the preflight is shorter; behavior is unchanged because the parameter classes already validate the same things.
- **V&V content:** None for the algorithm. Worth noting that this is a code-hygiene change; the validation guarantees are now tied to the parameter framework rather than the filter, so any future change to those parameter classes must be regression-tested against this filter.

### PR #1571 — *"DOC: Add standardized ChoicesParameter descriptions to filter docs"* — merged 2026-03-30

- **Files in this filter:** docs (.md) **+7 lines**
- **Change nature:** Documentation hygiene — added a standardized `### Rotation Representation` subsection enumerating the **Axis Angle [0]** vs **Rotation Matrix [1]** choices.
- **V&V content:** Doc currency improvement. Not algorithmic. *Note:* The pre-existing July 2023 warning about only 90/180° rotations being verified was **not** updated by this PR even though the test suite (PR #1465, three months earlier) had already added 45° cases — recommend reconciling.

### PR #1588 — *"ENH: SIMPL Backwards Compatibility Test Redesign"* — merged 2026-04-22

- **Files in this filter:** test (.cpp) **+48 lines**, plus two new fixture files
  - `test/simpl_conversion/6_4/RotateSampleRefFrameFilter.json` (470 bytes, 21 lines)
  - `test/simpl_conversion/6_5/RotateSampleRefFrameFilter.json` (679 bytes, 33 lines)
- **Diff size:** 3 files, +102 lines
- **Change nature:** **Test addition.** Added a per-filter SIMPL→SIMPLNX backwards-compatibility test that exercises both SIMPL 6.4 (Filter_Name fallback) and 6.5 (UUID-mapped) pipeline conversion paths via `DYNAMIC_SECTION`. Test name: `"SimplnxCore::RotateSampleRefFrameFilter: SIMPL Backwards Compatibility"`. The 6.5 fixture exercises the parameter rebuilt by PR #1476 (the `RotationRepresentationChoice` / `RotationTable` graceful-skip).
- **V&V content:** **Pipeline-conversion correctness only** — verifies that opening a legacy SIMPL pipeline in DREAM3DNX produces a filter instance with the right parameter values (specifically `RotationRepresentation == 0` and `SelectedImageGeometryPath == "DataContainer"`). Does **not** verify that the filter's *output* matches legacy.

### Pruned PRs (touched the file but not behaviorally relevant to this filter)

| PR | Subject | Why pruned |
|---|---|---|
| #1439 | Multi-Dimensional Tuple Support for StringArray and NeighborList | API change, single-line touch in this filter (+1 / -1) |
| #1457 | Clean up 'static inline' from filter headers | Pure style — header constexpr cleanup |
| #1538 | Replace cmake subprocess tar.gz extraction with zlib | Test infrastructure (+1 / -2 in test file, mechanical) |

## Test coverage detected

`RotateSampleRefFrameTest.cpp` contains **2 `TEST_CASE`s**:

1. `SimplnxCore::RotateSampleRefFrame` — runs **18 parameterized sections** via `GENERATE` over (angle ∈ {45°, 90°, 180°}) × (axis ∈ {<100>, <010>, <001>}) × (keepOrigin ∈ {false, true}). For each parameter combination the test runs the filter **twice** — first via `RotationRepresentation::AxisAngle`, then via `RotationRepresentation::RotationMatrix` (with the matrix derived from the same axis/angle via Eigen). Each run is compared against the corresponding exemplar geometry/CellData in `Rotate_Sample_Ref_Frame_Test_v3.dream3d`. Exemplar geometries are named like `180_100`, `90_010_KeepOrigin`, `45_001`, etc.
2. `SimplnxCore::RotateSampleRefFrameFilter: SIMPL Backwards Compatibility` — SIMPL 6.4 + 6.5 conversion paths via `DYNAMIC_SECTION` *(added by PR #1588)*.

Test 1 effectively gives **36 algorithmic checks** (18 sections × 2 representations) plus the `CompareImageGeometry` / `CompareExemplarToGenerateAttributeMatrix` invariants per check. Test 2 is conversion-only.

## Exemplar archive

- **Active archive name:** `Rotate_Sample_Ref_Frame_Test_v3.tar.gz`
- **SHA512 (v3):** `197f79f26df77c5511637b38c2c25ce615e19a6053d6630093c77e22dadc49a05d048500b98e207f30116d86019e686d7e8db8b455d1e8a3224c175c9aae0bdc`
- **Predecessor still referenced in CMakeLists.txt:** `Rotate_Sample_Ref_Frame_Test_v2.tar.gz` (SHA512 `b7c1a0dca46e133233ef931c2b99879da87ef0facdff77458fdeeb5db2dabd9a477e6699fd006c3676789ca052b0c74e52f7d1f36648f934841672620ec4ce4e`) — both v2 and v3 download lines are present; v3 is the one the test consumes.
- **Referenced in:** `src/Plugins/SimplnxCore/test/CMakeLists.txt` (lines 274-275)
- **Versioning history:** `Rotate_Sample_Ref_Frame_Test.tar.gz` → `_v2` (PR #1465 added 45°/keep-origin exemplars) → `_v3` (probably an exemplar regeneration after a downstream change; **needs engineer to confirm** which PR produced v3 — git log on test/CMakeLists.txt did not surface the v3 bump in the file-scoped log above; it may have been added with the test rewrite in PR #1465 or in a later un-noticed PR).
- **Provenance:** *(TBD — engineer must inspect the archive to determine how the exemplar was generated and whether an Oracle Provenance block exists in any ReadMe inside it.)*
- **Action required:** Download both v2 and v3 archives. Confirm whether v3 is still the in-use exemplar set; remove v2 from CMakeLists.txt if no other test depends on it; document how the exemplars were generated (legacy SIMPL pipeline? DREAM3DNX self-pipeline? hand-derived?). Promote into the verification archive ReadMe per Step 0's Oracle Provenance policy.

## Oracle classification (tentative)

- **Recommended class — split:**
  - **Class 1 (Analytical) for axis-aligned 90°/180°/270° rotations.** Each voxel maps deterministically to a known new index (e.g. for 90° about Z: `(i,j,k) → (Ny-1-j, i, k)`). No interpolation occurs; the rotation matrix has only 0s and ±1s; output is **bit-exact** in every cell type (uint8…float64) and equality of CellData arrays can be asserted directly. The current `Rotate_Sample_Ref_Frame_Test_v3` exemplars for the 90/180 sections already make this oracle achievable — the comparison threshold can be tightened to exact equality for these cases.
  - **Class 4 (Invariant-based) for arbitrary rotations (e.g. the 45° sections).** Bit-exactness is impossible because nearest-neighbor resampling onto a new grid produces a sampling lattice that differs from the input lattice. Useful invariants:
    - Total voxel count of the bounding-box output equals `outputDims[0]*[1]*[2]` from `CreateRotationArgs` (preflight already computes this).
    - For a discrete labeled field (e.g. FeatureIds), the **set of labels present** is preserved (no spurious labels appear; labels that exist in the input must still exist in the output unless they fall entirely outside the rotated bbox crop).
    - **Round-trip identity**: rotating by R then by R⁻¹ should return a CellData array within nearest-neighbor sampling tolerance (some boundary voxels may differ due to two consecutive resamplings).
    - **Origin behavior**: when `KeepInputGeometryOrigin=true`, output `origin == input origin`; when `false`, output origin equals input origin shifted by `outputXMin/YMin/ZMin` from `RotateArgs`.
  - **Class 5 (Expert-visual) only as fallback** for arbitrary rotations if invariants do not achieve confidence.
- **Rationale:** This is exactly the kind of filter the V&V policy anticipated when it split Class 1 from Class 4. The dual classification is honest: 90/180 cases can be checked to machine precision; arbitrary cases cannot, and pretending otherwise produces brittle tests.
- **Action required:** Developer to defend or replace this dual-class recommendation. If accepted, restructure the test assertions so the 90/180 sections use exact equality and the 45 section uses Class-4 invariant assertions instead of (or in addition to) exemplar-comparison.

## V&V status so far

| Item | Status | Notes |
|---|---|---|
| Algorithm review (`review-algorithm` skill) | Not visible from PR history | The Algorithm class is small (~100 lines) and delegates the math to `ImageRotationUtilities`. A separate review of `ImageRotationUtilities` (the actual rotation math) is more important than reviewing this thin wrapper. |
| Code path coverage (algorithmic) | **Excellent** | 18 parameter combinations × 2 representations = 36 checks per run. Exercises X/Y/Z axes, 45°/90°/180° angles, and keep-origin on/off. |
| Code path coverage (SIMPL conversion) | Good | PR #1588 added SIMPL 6.4 + 6.5 conversion test. PR #1476 fixed graceful handling of missing 6.5 keys. |
| Exemplar data in Data_Archive | **Yes (v3)** | `Rotate_Sample_Ref_Frame_Test_v3.tar.gz`. v2 still listed in CMakeLists.txt — needs cleanup decision. |
| Exemplar provenance documented | **Unknown** | TBD by inspecting archive contents. |
| Oracle class recorded | **No** | This document is the first to propose one. Recommend dual Class-1 + Class-4 split. |
| Toy data / independent expected output (Step 0 c) | Partially | The 90/180 cases admit hand-derivation (each voxel maps to a known index). Has not been formalized as in-test assertions; current tests only do exemplar-comparison. |
| Legacy comparison report (Step 0 e) | **No** | `compare-legacy-dream3d` has not been run. Particularly useful for confirming `ImageRotationUtilities` matches the legacy SIMPL implementation for axis-aligned 90/180. |
| Deviation entries (`RotateSampleRefFrame-D<N>`) | None | Not yet written. The doc warning about only-90/180-being-verified is the seed of a Deviation conversation. |
| Documentation currency | **Stale** | The doc still carries the *"As of July 2023, this filter is only verified to work with a rotation angle of 90 or 180 degrees…"* warning even though PR #1465 (Nov 2025) added 45° tests. Either remove the warning, or document why 45° tests are present despite the verification gap. |
| Verification archive (OneDrive) | No | Not yet created. |

## Gaps to close (to meet Step 0 / Legacy Comparison policy)

1. **Confirm the dual oracle classification.** Class 1 for 90/180/270; Class 4 for arbitrary rotations. Document which test cases fall under which class and tighten the comparison thresholds for the Class-1 cases to exact equality.
2. **Reconcile the doc warning vs. the test suite.** The user-facing doc claims only 90/180 are verified; the unit test asserts 45° matches an exemplar. Either:
   - The 45° exemplars are trustworthy → update the doc to remove the warning and state the new verification scope; **or**
   - The 45° exemplars are self-consistent but not validated against an independent source → keep the warning, and add a comment in the test acknowledging the 45° cases verify *self-consistency* (algorithm produces the same output as last time) rather than *correctness*.
3. **Add explicit Class-1 invariant assertions** for the 90/180/270 cases. For 90° about Z, assert: `outputDims = (Ny, Nx, Nz)`; `outputArray[Ny-1-j, i, k] == inputArray[i, j, k]` for each voxel. This makes the test code itself the oracle of record for these cases.
4. **Add Class-4 invariant assertions** for the 45° cases (label-set preservation, voxel-count match, round-trip R∘R⁻¹ within tolerance).
5. **Inspect the exemplar archives (v2 and v3)** and document provenance. Determine which PR created the v3 archive. Decide whether to remove the v2 download line from CMakeLists.txt (per PR #1465's note: *"Rotate_Sample_Ref_Frame_Test_v2.tar.gz needs to be removed from the DataArchive after this PR gets approved and merged in"* — that cleanup may not have happened).
6. **Run the legacy comparison.** Use `compare-legacy-dream3d` to diff SIMPLNX vs. DREAM3D 6.5.171 on a 90°-around-Z case (Class-1, where bit-exactness is expected) and on a 45°-around-Z case (Class-4, expect interpolation differences if the legacy and simplnx resamplers differ). Both will produce useful Deviation entries.
7. **Review `ImageRotationUtilities`** in a separate, dedicated `review-algorithm` pass. The actual rotation math lives there, not in this filter. Most of the V&V risk is in that utility.
8. **Produce the Algorithm Relationship one-liner.** Tentative: *"Port — direct translation of the SIMPL `RotateSampleRefFrame` filter, with the rotation/resampling math factored into the shared `ImageRotationUtilities` library; executeImpl extracted to an Algorithm class in PR #1301 per issue #1284."*
9. **Archive everything** per `archive-filter-verification` for the OneDrive folder.

## Recommended Deviation entries (proposed, pending legacy comparison)

> **Deviation ID:** `RotateSampleRefFrame-D1`
> **Filter UUID:** `d2451dc1-a5a1-4ac2-a64d-7991669dcffc`
> **Symptom:** *(pending)* — running `compare-legacy-dream3d` on a non-axis-aligned rotation (e.g. 45° about Z) is expected to show per-voxel differences between SIMPLNX and DREAM3D 6.5.171 due to potentially different nearest-neighbor sampling rules at half-cell boundaries.
> **Root cause:** *(pending verification)* — depends on whether the legacy SIMPL `RotateSampleRefFrame` and simplnx `ImageRotationUtilities` use identical rounding/snap rules when picking a source voxel for each output voxel.
> **Affected users:** Anyone reproducing arbitrary-angle rotations for cross-version comparison studies. Axis-aligned (90/180) cases should be unaffected.
> **Recommendation:** *(pending verification)* — if SIMPLNX is the more rigorous resampler, document the difference and trust SIMPLNX. If the difference is a regression, fix simplnx.
> **Status:** Proposed — pending legacy comparison run.

> **Deviation ID:** `RotateSampleRefFrame-D2`
> **Filter UUID:** `d2451dc1-a5a1-4ac2-a64d-7991669dcffc`
> **Symptom:** SIMPL 6.5 pipelines that pre-date the `RotationRepresentationChoice` and `RotationTable` parameter additions used to fail conversion; PR #1476 added graceful skip-when-absent handling so they now load successfully and default to AxisAngle representation with the identity matrix.
> **Root cause:** Conversion code originally required keys that were optional in 6.5.
> **Affected users:** Anyone opening pre-6.5.172 pipelines that contain `RotateSampleRefFrame`.
> **Recommendation:** Trust SIMPLNX. The change is a strict improvement: pipelines that previously failed now load with reasonable defaults.
> **Status:** Closed by PR #1476 — record as a documented behavioral improvement.

> **Deviation ID:** `RotateSampleRefFrame-D3` *(documentation-class deviation, not algorithmic)*
> **Filter UUID:** `d2451dc1-a5a1-4ac2-a64d-7991669dcffc`
> **Symptom:** User-facing documentation states the filter is *"only verified to work with a rotation angle of 90 or 180 degrees, a rotation axis of (010) || (100) || (001)"* (warning dated July 2023). However, the unit test added in PR #1465 (Nov 2025) asserts correct behavior for 45° rotations against pre-generated exemplars.
> **Root cause:** Doc was not updated when the test suite was expanded.
> **Affected users:** Anyone reading the doc to decide whether to use the filter for non-axis-aligned rotations.
> **Recommendation:** Either update the doc to state the new verification scope (45°/90°/180° about X/Y/Z, with the v3 exemplar set) or — if the 45° exemplars are merely self-regression rather than independent validation — keep the warning and add a clarifying note in the test file.
> **Status:** Proposed — requires engineer judgement on the exemplar provenance.
