# Retroactive V&V: ComputeCAxisLocationsFilter

*Report status:* **DRAFT**. Generated from git-history and source-tree inspection. Developer must confirm or correct the Oracle class, Algorithm Relationship, and the V&V status entries.

## Metadata

| Field | Value |
|---|---|
| SIMPLNX UUID | `a51c257a-ddc1-499a-9b21-f2d25a19d098` |
| SIMPLNX ClassName | `ComputeCAxisLocationsFilter` |
| SIMPLNX Human Name | Compute C-Axis Locations |
| SIMPL UUID | `68ae7b7e-b9f7-5799-9f82-ce21d0ccd55e` |
| SIMPL ClassName | `FindCAxisLocations` *(per `simpl_conversion/6_4/ComputeCAxisLocationsFilter.json` — `Filter_Name: "FindCAxisLocations"`)* |
| SIMPL Human Name | Find C-Axis Locations *(legacy human label)* |
| Plugin | OrientationAnalysis |

### Source files scanned

- `src/Plugins/OrientationAnalysis/src/OrientationAnalysis/Filters/ComputeCAxisLocationsFilter.{hpp,cpp}`
- `src/Plugins/OrientationAnalysis/src/OrientationAnalysis/Filters/Algorithms/ComputeCAxisLocations.{hpp,cpp}`
- `src/Plugins/OrientationAnalysis/test/ComputeCAxisLocationsTest.cpp`
- `src/Plugins/OrientationAnalysis/test/simpl_conversion/6_5/ComputeCAxisLocationsFilter.json`
- `src/Plugins/OrientationAnalysis/test/simpl_conversion/6_4/ComputeCAxisLocationsFilter.json`
- `src/Plugins/OrientationAnalysis/docs/ComputeCAxisLocationsFilter.md`
- `src/Plugins/OrientationAnalysis/test/CMakeLists.txt` (line 140 — `caxis_data.tar.gz`)
- `src/Plugins/OrientationAnalysis/src/OrientationAnalysis/OrientationAnalysisLegacyUUIDMapping.hpp` (line 130)

## Algorithm Relationship

- **Tentative classification:** **Port** — the SIMPLNX filter appears to be a direct translation of the legacy SIMPL `FindCAxisLocations` filter; the legacy SIMPL UUID `{68ae7b7e-b9f7-5799-9f82-ce21d0ccd55e}` is registered in `OrientationAnalysisLegacyUUIDMapping.hpp` to map onto the new `ComputeCAxisLocationsFilter` UUID. The algorithm is short (~100 lines): per voxel, build the orientation matrix from the quaternion, transpose it, multiply by the crystal-frame c-axis `(0,0,1)`, normalize, and force `c1.z >= 0` via antipodal flip.
- **Evidence:** No rewrite signal in PR history. Material PRs since 2025-10-01 are: an EbsdLib-API rename pass (#1472) that swapped `OrientationTransformation::qu2om(...)` + `OrientationMatrixToGMatrixTranspose(...)` for the more direct `Quaternion::toOrientationMatrix().transpose()` formulation, and a one-line cancel-check insertion (#1582). Neither changed the math.
- **Action required:** Confirm by reading the corresponding SIMPL `FindCAxisLocations` source and running `compare-legacy-dream3d` against a shared Hex toy dataset. PR #1472 changed the *expression* of qu→om; verify it remains numerically identical (same EbsdLib convention) on the legacy comparison.

## PRs inspected (since 2025-10-01)

> Pruned: pure-style/repo-wide refactor PRs (#1457 static-inline, #1538 zlib extraction, #1547 doc subgroup typo) and the Multi-Dimensional NeighborList API PR (#1439) are listed at the bottom of this section but not detailed individually — they did not change behavior of this filter.

### PR #1438 — *"ENH: Microtexture related filter cleanup"* — merged 2025-10-25

- **Files in this filter:** algorithm (.cpp) — include-style only
- **Diff size:** 1 file, +1 / -1 line (changed `#include "EbsdLib/LaueOps/LaueOps.h"` to `#include <EbsdLib/LaueOps/LaueOps.h>`)
- **Change nature:** Pure include-syntax cleanup as part of a much larger microtexture-pipeline correctness pass. **None of the algorithmic bug-fixes itemized in PR #1438's commit body touched `ComputeCAxisLocations`** — the c-axis-touching items in that PR were on related filters (`CAxisSegmentFeatures`, `FindFeatureReferenceCAxisOrientation`, `ComputeFeatureNeighborCAxisAlignments`, `ComputeAvgOrientations`).
- **V&V content:** Flagged by the policy maintainer as MTR-V&V-related work in the broader sense, but for *this particular filter* the diff is cosmetic. The c-axis algorithm itself was not corrected here.

### PR #1472 — *"ENH: Update to EbsdLib 2.0.0 API"* — merged 2025-11-24 *(broad refactor, exception flagged because this filter delegates directly to EbsdLib for its core math: Quaternion → OrientationMatrix conversion, LaueOps namespace, CrystalStructure enum)*

- **Files in this filter:** algorithm (.cpp)
- **Diff size:** 1 file, +8 / -7 lines
- **Change nature:** EbsdLib 2.0.0 API rename pass. Renamed `EbsdLib::CrystalStructure::*` to `ebsdlib::CrystalStructure::*`, renamed `LaueOps::Pointer` to `ebsdlib::LaueOps::Pointer`, and **most importantly replaced** `OrientationF oMatrix = OrientationTransformation::qu2om<QuatF, OrientationF>({q0,q1,q2,q3})` followed by `OrientationMatrixToGMatrixTranspose(oMatrix) * cAxis` with `ebsdlib::OrientationMatrixFType oMatrix = ebsdlib::QuaternionFType(q0,q1,q2,q3).toOrientationMatrix()` followed by `oMatrix.transpose() * cAxis`. The intent is mathematically equivalent (same quaternion-to-rotation-matrix and same transpose), but the underlying EbsdLib code paths are different and the storage type changed (`OrientationF` → `Eigen::Matrix3f`-backed `OrientationMatrixFType`).
- **V&V content:** **Medium-High risk.** Two concerns to verify in the legacy comparison: (1) the new `Quaternion::toOrientationMatrix()` must use the same convention (passive vs active, and same component order `(w,x,y,z)` vs `(x,y,z,w)`) as the old `OrientationTransformation::qu2om`; (2) the old `OrientationMatrixToGMatrixTranspose` helper may have done more than a plain `.transpose()` (e.g. row-vs-column-major reinterpretation). If either differs, this PR silently changed the c-axis output.

### PR #1582 — *"ENH: Add missing cancel checks to lots of filters"* — merged 2026-04-08 *(broad refactor, exception flagged because the change adds an early-return inside the per-voxel loop, which is a control-flow change)*

- **Files in this filter:** algorithm (.cpp)
- **Diff size:** 1 file, +5 / -0 lines
- **Change nature:** Inserted `if(m_ShouldCancel) { return {}; }` at the top of the per-voxel for-loop. **Note the early-return returns an empty `Result<>` instead of an error**, which means a cancelled run leaves the output `CAxisLocation` array partially populated (zeros from `CreateArrayAction` for voxels not yet visited). Downstream filters that consume cancelled output would see garbage.
- **V&V content:** None. No test was added.

### PR #1588 — *"ENH: SIMPL Backwards Compatibility Test Redesign"* — merged 2026-04-22

- **Files in this filter:** test (.cpp) +45 lines, plus two new fixture files
  - `test/simpl_conversion/6_4/ComputeCAxisLocationsFilter.json` (482 bytes — uses `Filter_Name: "FindCAxisLocations"` only, exercises the 6.4 name-based fallback)
  - `test/simpl_conversion/6_5/ComputeCAxisLocationsFilter.json` (539 bytes — uses `Filter_Uuid: "{68ae7b7e-b9f7-5799-9f82-ce21d0ccd55e}"`, exercises the 6.5 UUID-mapped path)
- **Change nature:** **Test addition.** Added a per-filter SIMPL→SIMPLNX backwards-compatibility test that exercises both paths via `DYNAMIC_SECTION`. Test name: `"OrientationAnalysis::ComputeCAxisLocationsFilter: SIMPL Backwards Compatibility"`. Asserts the converted filter has the right UUID and that `k_QuatsArrayPath_Key` and `k_CAxisLocationsArrayName_Key` carry the expected exemplar values.
- **V&V content:** **Pipeline-conversion correctness only** — verifies that opening a legacy SIMPL pipeline in DREAM3DNX produces a filter instance with the right parameter values. It does **not** verify that the filter's *output* matches legacy. That latter step is still missing. Also note: only two of the four parameters are checked (`CellPhasesArrayPath` and `CrystalStructuresArrayPath` are not asserted in the conversion test).

### Pruned PRs (touched the file but not behaviorally relevant to this filter)

| PR | Subject | Why pruned |
|---|---|---|
| #1439 | Multi-Dimensional Tuple Support for StringArray and NeighborList | API change, only adjusted preflight ShapeType usage in this filter |
| #1457 | Clean up 'static inline' from filter headers | Style — removed `inline` from `static inline constexpr StringLiteral` |
| #1538 | Replace cmake subprocess tar.gz extraction with zlib | Test infrastructure (TestFileSentinel signature change) |
| #1547 | Fix filter documentation and documentation related code bugs | Subgroup typo: "Crystallographic" → "Crystallography" in docs (1 char) |

## Test coverage detected

`ComputeCAxisLocationsTest.cpp` contains 3 `TEST_CASE`s:

1. `OrientationAnalysis::ComputeCAxisLocationsFilter: Valid Filter Execution` — runs the filter on a Hex exemplar (`7_0_find_caxis_data.dream3d`) and compares the computed `NX_CAxisLocations` against exemplar `CAxisLocations` via `UnitTest::CompareFloatArraysWithNans<float32>` with `UnitTest::EPSILON`.
2. `OrientationAnalysis::ComputeCAxisLocationsFilter: InValid Filter Execution` — mutates `crystalStructs[1] = 1` (Cubic_High) so that no phase is hexagonal, then expects execute to fail with error `-3522`.
3. `OrientationAnalysis::ComputeCAxisLocationsFilter: SIMPL Backwards Compatibility` — SIMPL 6.4 + 6.5 conversion paths via `DYNAMIC_SECTION` *(added by PR #1588)*.

Tests 1–2 exercise: the all-hex success path, and the no-hex error path. **Coverage gap:** the *mixed* phase case (some hex, some non-hex — the path that emits warning `-3523` and writes `NaN` for non-hex voxels) is **not** tested. Test 1's exemplar comparison uses `CompareFloatArraysWithNans` which suggests NaNs are expected somewhere, so mixed-phase input may be implicit in the exemplar; needs confirmation by inspecting `7_0_find_caxis_data.dream3d`.

## Exemplar archive

- **Archive name:** `caxis_data.tar.gz`
- **SHA512:** `56468d3f248661c0d739d9acd5a1554abc700bf136586f698a313804536916850b731603d42a0b93aae47faf2f7ee49d4181b1c3e833f054df6f5c70b5e041dc`
- **Referenced in:** `src/Plugins/OrientationAnalysis/test/CMakeLists.txt` line 140 (the same archive serves `CAxisSegmentFeaturesTest` and other Hex-related tests)
- **Inner exemplar file:** `caxis_data/7_0_find_caxis_data.dream3d` (note `7_0_` prefix, not `6_6_`/`6_5_`)
- **Exemplar array path:** `k_CellAttributeMatrix / "CAxisLocations"`
- **Generated array path:** `k_CellAttributeMatrix / "NX_CAxisLocations"`
- **Provenance:** *(TBD — engineer must inspect the archive to determine how `CAxisLocations` was generated, whether it came from a SIMPL `FindCAxisLocations` run, a SIMPLNX self-roundtrip, or a hand-derived oracle.)*
- **Action required:** Download the archive locally and inspect for: an inner `ReadMe.md`, the input `.dream3d` files used to generate the exemplars, the pipeline files that produced the exemplars, and any provenance notes. Promote this content into the verification archive ReadMe per Step 0's Oracle Provenance policy. **Critical:** if the exemplar `CAxisLocations` was generated by SIMPLNX itself (not by legacy DREAM3D), the test is currently a regression check, not a verification.

## Oracle classification (tentative)

- **Recommended class:** **3 (Paper-based)** — Rowenhorst, D., Rollett, A.D., Rohrer, G.S., Groeber, M., Jackson, M., Konijnenberg, P.J., De Graef, M., *"Consistent representations of and conversions between 3D rotations"*, Modelling Simul. Mater. Sci. Eng. **23** (2015) 083501. The algorithm is the per-voxel application of the standard quaternion-to-rotation-matrix formula followed by a transpose-then-rotate of the crystal `[0,0,1]` direction; this is exactly the construction described in §3 of the paper for moving a crystal-frame vector into the sample frame. The c-axis is the special case `[0,0,1]` because hexagonal symmetry leaves it invariant under the proper rotation point group `622` / `6/mmm`.
- **Class 4 (Invariant) companion:** several invariants are easy to assert: (a) every output 3-vector is unit norm (`|c1| == 1` to float tolerance); (b) for an identity quaternion `(1,0,0,0)`, `c1 = (0,0,1)` exactly; (c) the antipodal-flip convention is enforced — every output has `c1.z >= 0`; (d) for a 180-deg rotation about Z, `c1` is unchanged (the c-axis is the rotation axis); (e) for any non-hex phase, output is `(NaN, NaN, NaN)`. These should be encoded as test assertions on a small synthetic dataset.
- **Class 1 (Analytical):** all special-orientation closed forms are hand-computable from the quaternion-to-matrix formula. Examples to write into the test: identity → `(0,0,1)`; 90-deg rotation about X → `(0, -1, 0)` then antipodal-flipped to `(0, 1, 0)` *(verify the convention)*; 90-deg rotation about Y → `(1, 0, 0)`; 90-deg rotation about Z → `(0, 0, 1)`.
- **Rationale:** The math is small, well-known, and entirely closed-form. Class 3 is defensible because the `qu2om` formula is the one published in Rowenhorst 2015 (and that paper is *the* citation already used elsewhere in OrientationAnalysis). Class 4 is the natural companion check. Class 1 spot-checks belong in the unit test regardless.
- **Action required:** Developer to defend or replace. If a closer paper reference is preferred (e.g. Bunge's *Texture Analysis in Materials Science* for the c-axis pole-figure convention), substitute. The `cAxisLocation` is also called the *c-axis pole figure direction* in texture-analysis literature; the antipodal flip (`c1.z < 0` → multiply by -1) is the upper-hemisphere convention used in pole figures.

## V&V status so far

| Item | Status | Notes |
|---|---|---|
| Algorithm review (`review-algorithm` skill) | Not visible from PR history | No PR explicitly performs the line-by-line review. |
| Code path coverage (algorithmic) | **Partial** | All-hex success and no-hex error paths covered. **Mixed-phase NaN-emitting path is not explicitly tested** (may be implicit in the exemplar). Cancel-check path (PR #1582) untested. |
| Code path coverage (SIMPL conversion) | Good | PR #1588 added SIMPL 6.4 + 6.5 conversion test; only 2 of 4 parameters asserted. |
| Exemplar data in Data_Archive | **Yes** | `caxis_data.tar.gz` referenced in test/CMakeLists.txt line 140. |
| Exemplar provenance documented | Unknown | TBD by inspecting archive contents. Critical question: was `CAxisLocations` generated by legacy SIMPL `FindCAxisLocations` or by SIMPLNX itself? |
| Oracle class recorded | **No** | This document is the first to propose one. |
| Toy data / independent expected output (Step 0 c) | No | No script or hand-derivation on file. Identity-quaternion / 90-deg-rotation Class-1 spot checks are missing. |
| Legacy comparison report (Step 0 e) | No | `compare-legacy-dream3d` has not been run. PR #1472's qu2om-formulation rewrite makes this comparison especially valuable. |
| Deviation entries (`ComputeCAxisLocations-D<N>`) | None | Not yet written. Three candidates flagged below. |
| Documentation currency | Probably current | Updated by PR #1547 (subgroup typo). The doc correctly explains the no-symmetry-needed argument for hex but does not explain the antipodal-flip convention or the NaN treatment of non-hex voxels — both behaviors a user needs to know. |
| Verification archive (OneDrive) | No | Not yet created. |

## Gaps to close (to meet Step 0 / Legacy Comparison policy)

1. **Confirm the oracle.** Class 3 (Rowenhorst 2015) is the recommended starting point; defend or replace. Add a Class 1 hand-computed special-orientation block to the unit test and Class 4 invariant assertions (unit norm, `z >= 0`, NaN for non-hex).
2. **Inspect `caxis_data.tar.gz` and document provenance.** Determine whether the `CAxisLocations` exemplar came from legacy DREAM3D or from a SIMPLNX self-roundtrip. If the latter, the current test is a regression check only; promote it to verification by re-generating the exemplar from a trusted source.
3. **Run the legacy comparison.** Use `compare-legacy-dream3d` to diff SIMPLNX vs. DREAM3D 6.5.172 on the same Hex toy data. **Specifically validate that PR #1472's API rename did not change the qu→om convention.** A small dataset of ~10 hand-picked quaternions with paper-derived expected c-axes is the right input.
4. **Add a mixed-phase test.** Construct an input where phase 1 is hexagonal and phase 2 is cubic, verify warning `-3523` is emitted, and verify cubic-phase voxels receive `(NaN, NaN, NaN)`.
5. **Add a cancel-check test.** Trigger the cancel atomic mid-execute and verify that the early-return behavior is what's actually wanted. The current code returns `Result<>{}` (success) on cancel, leaving the output array partially zeroed — this is likely a bug. See Deviation D3 below.
6. **Document the conventions.** The filter doc should explain (a) the antipodal flip into the upper hemisphere `(z >= 0)`, (b) the NaN for non-hex voxels, (c) the active-vs-passive quaternion convention used. Step the doc up via `review-filter-docs`.
7. **Produce the Algorithm Relationship one-liner.** Tentative: *"Port — direct translation of the SIMPL `FindCAxisLocations` filter; one EbsdLib API rewrite (PR #1472) restated the qu→om math through `Quaternion::toOrientationMatrix()` rather than `OrientationTransformation::qu2om(...)`; one cancel-check insertion (PR #1582)."*
8. **Archive everything** per `archive-filter-verification` for the OneDrive folder.

## Recommended Deviation entries (proposed, pending legacy comparison)

> **Deviation ID:** `ComputeCAxisLocations-D1`
> **Filter UUID:** `a51c257a-ddc1-499a-9b21-f2d25a19d098`
> **Symptom:** SIMPLNX writes `(NaN, NaN, NaN)` to the `CAxisLocation` 3-component output for any voxel whose phase is not Hexagonal_High or Hexagonal_Low. SIMPL 6.5.172 may write something else (zeros? uninitialized memory? raw quaternion-rotated vector ignoring phase?).
> **Root cause:** Different convention for "what to write when the input phase is wrong for this algorithm." SIMPLNX's choice (NaN sentinel + warning `-3523`) is the safer one because downstream consumers can detect it; SIMPL's behavior must be confirmed.
> **Affected users:** Anyone running this filter on multi-phase data (Hex + non-Hex), and anyone whose downstream analysis (e.g. pole-figure plotting) reads from non-Hex voxel locations.
> **Recommendation:** Trust SIMPLNX. Document the NaN convention in the filter docs. Engineer to confirm legacy behavior before finalizing.
> **Status:** Proposed — pending verification of legacy behavior.

> **Deviation ID:** `ComputeCAxisLocations-D2`
> **Filter UUID:** `a51c257a-ddc1-499a-9b21-f2d25a19d098`
> **Symptom:** The output `CAxisLocation` 3-vector is always forced into the upper hemisphere (`z >= 0`) by an antipodal flip (`c1 *= -1.0f` when `c1[2] < 0`). Crystallographically `c` and `-c` are equivalent for hexagonal pole-figure analysis, so this is correct, but **a downstream user comparing two adjacent voxels with the same true c-axis but on opposite sides of the `z = 0` plane might see them flip sign** in legacy code that did not enforce the hemisphere convention.
> **Root cause:** SIMPLNX (and presumably SIMPL) enforces the upper-hemisphere convention. Need to confirm legacy did the same.
> **Affected users:** Anyone visualizing the raw `CAxisLocation` vector field (rather than a pole-figure projection); anyone who expects continuous c-axis vectors across a feature boundary.
> **Recommendation:** Trust SIMPLNX. Document the upper-hemisphere convention in the filter docs. If legacy also flips, this is not actually a deviation — convert to a Documentation note.
> **Status:** Proposed — pending verification of legacy behavior.

> **Deviation ID:** `ComputeCAxisLocations-D3`
> **Filter UUID:** `a51c257a-ddc1-499a-9b21-f2d25a19d098`
> **Symptom:** When the user cancels mid-execute, the filter returns `Result<>{}` (success) with the output `CAxisLocation` array left partially populated (zero-initialized for voxels not yet visited from `CreateArrayAction`).
> **Root cause:** PR #1582 inserted `if(m_ShouldCancel) { return {}; }` without converting to a `MakeWarningVoidResult`/`MakeErrorResult` or marking the output array as invalid. Downstream filters that read `CAxisLocation` see (0, 0, 0) for unfinished voxels — which would be misinterpreted as a unit Z c-axis.
> **Affected users:** Anyone who cancels a long-running pipeline mid-c-axis-computation and then inspects the partial output (e.g. via the GUI's intermediate-results view), or anyone whose pipeline catches the cancel and then proceeds to a downstream filter.
> **Recommendation:** Change the cancel return to a warning `Result<>` and/or zero-fill with NaN instead of letting the `CreateArrayAction`'s default zero-fill stand. This deviation is **not** vs. legacy — legacy did not have a cancel check at all — but it's a correctness issue worth tracking.
> **Status:** Proposed — internal correctness issue, not a legacy-comparison item.

> **Deviation ID:** `ComputeCAxisLocations-D4` *(potential, requires PR #1472 legacy comparison)*
> **Filter UUID:** `a51c257a-ddc1-499a-9b21-f2d25a19d098`
> **Symptom:** PR #1472 replaced `OrientationTransformation::qu2om<QuatF, OrientationF>(...)` followed by `OrientationMatrixToGMatrixTranspose(oMatrix) * cAxis` with `ebsdlib::QuaternionFType(...).toOrientationMatrix().transpose() * cAxis`. If the new EbsdLib 2.0.0 `Quaternion::toOrientationMatrix()` uses a different convention (e.g. row-major vs column-major storage, active vs passive rotation, different quaternion component order) than the old `qu2om`, the c-axis output silently changed at PR #1472.
> **Root cause:** Possible API-rename masking a behavioral change in EbsdLib 2.0.0.
> **Affected users:** Anyone running this filter on output from SIMPLNX prior to PR #1472 vs after, with strict bit-for-bit reproducibility expectations.
> **Recommendation:** Run the legacy comparison `compare-legacy-dream3d` and diff against pre-#1472 SIMPLNX output on the same Hex input. If outputs match to float tolerance, close this entry as "no deviation." If they differ, escalate.
> **Status:** Proposed — requires comparison run before promotion or dismissal.
