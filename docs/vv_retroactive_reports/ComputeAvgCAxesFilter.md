# Retroactive V&V: ComputeAvgCAxesFilter

*Report status:* **DRAFT**. Generated from git-history and source-tree inspection. Developer must confirm or correct the Oracle class, Algorithm Relationship, and the V&V status entries.

## Metadata

| Field | Value |
|---|---|
| SIMPLNX UUID | `453cdb58-7bbb-4576-ad5e-f75a1c54d348` |
| SIMPLNX ClassName | `ComputeAvgCAxesFilter` |
| SIMPLNX Human Name | Compute Average C-Axis Orientations |
| SIMPL UUID | *(TBD — confirm in legacy SIMPL repo)* |
| SIMPL ClassName | `FindAvgCAxes` (visible from the 6.4 conversion fixture: `"Filter_Name": "FindAvgCAxes"`) |
| SIMPL Human Name | "Find Avg C-Axes" *(probable, from filter-name renaming PR #956)* |
| Plugin | OrientationAnalysis |

### Source files scanned

- `src/Plugins/OrientationAnalysis/src/OrientationAnalysis/Filters/ComputeAvgCAxesFilter.{hpp,cpp}`
- `src/Plugins/OrientationAnalysis/src/OrientationAnalysis/Filters/Algorithms/ComputeAvgCAxes.{hpp,cpp}`
- `src/Plugins/OrientationAnalysis/test/ComputeAvgCAxesTest.cpp`
- `src/Plugins/OrientationAnalysis/test/simpl_conversion/6_5/ComputeAvgCAxesFilter.json`
- `src/Plugins/OrientationAnalysis/test/simpl_conversion/6_4/ComputeAvgCAxesFilter.json`
- `src/Plugins/OrientationAnalysis/docs/ComputeAvgCAxesFilter.md`
- `src/Plugins/OrientationAnalysis/test/CMakeLists.txt` (test-data registration only)

## Algorithm Relationship

- **Tentative classification:** **Port** — the SIMPLNX filter is a translation of the legacy SIMPL `FindAvgCAxes`, but it has been **non-trivially modified** (PR #1438 included a documented behavioural rewrite of the inner accumulation loop and switched from `float` to `double` accumulation; PR #1472 reworked the EbsdLib API call style and removed the explicit `OrientationMatrixToGMatrixTranspose` helper in favour of `oMatrix.transpose() * cAxis`).
- **Evidence:** The 6.4 SIMPL fixture uses `Filter_Name: "FindAvgCAxes"` and the 6.5 fixture maps a SIMPL UUID to the SIMPLNX UUID — both are evidence of a name-change-aware port. The numerical semantics are nominally the same (passive→active rotation, accumulate, normalize) but the running-average trick used to decide the antipodal flip and the data-precision (now `double`) are visibly different from the original.
- **Action required:** Confirm by reading the corresponding SIMPL `FindAvgCAxes` source and running `compare-legacy-dream3d` step (e) against a shared toy dataset. The PR #1438 rewrite + PR #1472 EbsdLib refactor are exactly the sort of changes that can introduce small numerical Deviations vs. legacy.

## PRs inspected (since 2025-10-01)

> Pruned: pure-style/repo-wide refactor PRs (#1457 static-inline header cleanup, #1501 Vec3 unification, #1538 zlib tar.gz extraction in tests, #1547 doc/tag fix) are listed at the bottom of this section but not detailed individually — they did not change behaviour of this filter. PR #1472 (EbsdLib 2.0 bump) and PR #1438 (microtexture cleanup) are both broad refactors but are **promoted here per the standard rules** (#1472 because this filter delegates to EbsdLib for orientation conversion; #1438 because it explicitly touched this filter's algorithm with a comment trail).

### PR #1438 — *"ENH: Microtexture related filter cleanup"* — merged 2025-10-25 (broad refactor, exception flagged because the PR explicitly touched the inner loop of this filter — commit body: "Compute Average C-Axis was updated and documented with comments")

- **Files in this filter:** algorithm (.cpp), filter (.cpp)
- **Diff size:** algorithm `+89 / -76`; filter `+2 / -3`
- **Change nature:** **Material rewrite of the inner loop + behaviour change.**
  - **Float → double precision:** All accumulation now uses `Eigen::Vector3d` and `OrientationD` / `QuatD` / `Matrix3dR` — the previous version used `float` throughout. This will cause small but real numerical differences vs. legacy.
  - **Counter increment moved before the accumulation:** Previously `counter[featureIds[i]]++` happened *after* the running-average comparison; in the new code the counter is incremented *before* the running-average is computed. This subtly changes the sequence of running averages used for the antipodal-flip decision (the very first hex voxel of a feature now divides by 1 instead of 0; the previous version divided by 0 on the first cell, which would have produced NaN in `curCAxis` — re-read the legacy code to confirm).
  - **Error-code re-numbering:** `-6402` → `-76402`, `-6403` → `-76403`, and the preflight warning `-6401` was deleted entirely and replaced with a non-error `preflightUpdatedValues` "Crystal Symmetry Warning:" entry — so a pipeline that previously expected a preflight warning will no longer get one.
  - **Comments added throughout** the algorithm (per commit message "documented with comments").
- **V&V content:** This PR is the strongest candidate for a Deviation entry vs. legacy. The float→double change is a **silent numerical-precision change**; the counter-reorder is a **silent algorithmic change**; the error-code change is a **silent API-contract change**. None of these are tested explicitly — the existing exemplar test (`7_2_AvgCAxis.tar.gz`) was presumably regenerated to match the new output.

### PR #1472 — *"ENH: Update to EbsdLib 2.0.0 API"* — merged 2025-11-24 (broad refactor, exception flagged because this filter delegates orientation math to EbsdLib)

- **Files in this filter:** algorithm (.cpp)
- **Diff size:** `+9 / -12`
- **Change nature:** API rename (`EbsdLib::` → `ebsdlib::` namespace) plus a meaningful refactor: the explicit `OrientationMatrixToGMatrixTranspose(oMatrix)` helper call was removed and the transpose is now applied inline as `oMatrix.transpose() * cAxis`. The matrix construction also changed from `OrientationTransformation::qu2om<QuatD,OrientationD>(...)` to `ebsdlib::QuaternionDType(...).toOrientationMatrix()`. Numerically these should be equivalent **provided** the new `toOrientationMatrix()` produces the same passive-rotation matrix convention as the legacy `qu2om`. There is no test asserting that.
- **V&V content:** Inspect the EbsdLib 2.0 implementation of `QuaternionDType::toOrientationMatrix()` to confirm convention parity. If conventions differ, this is a Deviation source.

### PR #1476 — *"BUG/ENH: Fix Backwards Pipeline Compatibility and Add Testing"* — merged 2026-01-06

- **Files in this filter:** filter (.cpp), `+1 / -1`
- **Diff size:** trivially small
- **Change nature:** Single-line `FromSIMPLJson` correction: `DataArrayNameFilterParameterConverter` → `DataArrayCreationToDataObjectNameFilterParameterConverter` for the `AvgCAxes` output. This is a **SIMPL-pipeline-conversion bug fix**, not an algorithm change.
- **V&V content:** This change is what the SIMPL conversion test in PR #1588 ultimately validates.

### PR #1547 — *"DOC: Fix filter documentation and documentation related code bugs"* — merged 2026-03-10

- **Files in this filter:** docs (.md), `+1 / -1` (subgroup typo "Crystallographic" → "Crystallography")
- **Change nature:** Pure documentation typo correction.
- **V&V content:** None.

### PR #1582 — *"ENH: Add missing cancel checks to lots of filters"* — merged 2026-04-08 (broad refactor, listed for completeness because it modified this algorithm)

- **Files in this filter:** algorithm (.cpp), `+10 / -0`
- **Change nature:** Added two `if(m_ShouldCancel) return {};` guards — one in the per-cell loop, one in the per-feature normalisation loop. No algorithmic effect on completed runs.
- **V&V content:** Improves cancel responsiveness only.

### PR #1588 — *"ENH: SIMPL Backwards Compatibility Test Redesign"* — merged 2026-04-22

- **Files in this filter:** test (.cpp) `+49 lines`, plus two new fixture files
  - `test/simpl_conversion/6_4/ComputeAvgCAxesFilter.json` (743 bytes; uses legacy `Filter_Name: "FindAvgCAxes"`)
  - `test/simpl_conversion/6_5/ComputeAvgCAxesFilter.json` (800 bytes; UUID-mapped)
- **Change nature:** **Test addition.** Added a per-filter SIMPL→SIMPLNX backwards-compatibility test exercising both the SIMPL 6.4 (Filter_Name fallback) and 6.5 (UUID-mapped) conversion paths via `DYNAMIC_SECTION`. Test name: `"OrientationAnalysis::ComputeAvgCAxesFilter: SIMPL Backwards Compatibility"`.
- **V&V content:** **Pipeline-conversion correctness only** — the test verifies that opening a legacy SIMPL pipeline produces a filter instance with the right parameter values. It does **not** verify that the filter's *output* matches legacy.

### Pruned PRs (touched the file but not behaviorally relevant to this filter)

| PR | Subject | Why pruned |
|---|---|---|
| #1439 | Multi-Dimensional Tuple Support for StringArray and NeighborList | API change, no per-filter behavior change |
| #1457 | Clean up 'static inline' from filter headers | Style only (header cosmetic) |
| #1501 | Combine Matrix3x1, Point3D, Vec3 into a Vec3<T> | Removed one stale include line |
| #1538 | Replace cmake subprocess tar.gz extraction with zlib | Test infrastructure (sentinel call signature) |

## Test coverage detected

`ComputeAvgCAxesTest.cpp` contains 3 `TEST_CASE`s:

1. `OrientationAnalysis::ComputeAvgCAxesFilter: Valid Filter Execution` — single positive-path execution against the `7_2_AvgCAxis.tar.gz` exemplar, comparing computed `Computed ParentAvgCAxes [NX]` against exemplar `ParentAvgCAxes [NX]` with `CompareFloatArraysWithNans<float32>` at tolerance `5.0E-7f`.
2. `OrientationAnalysis::ComputeAvgCAxesFilter: Invalid Filter Execution` — uses the older `caxis_data.tar.gz` exemplar, mutates `crystalStructs[1] = 1` (Cubic_High), and asserts that execute returns INVALID. Verifies the all-non-hex error path (`-76402`).
3. `OrientationAnalysis::ComputeAvgCAxesFilter: SIMPL Backwards Compatibility` — SIMPL 6.4 + 6.5 conversion paths via `DYNAMIC_SECTION` *(added by PR #1588)*. Conversion-only.

**Coverage gaps observed:**
- No test for the **mixed-phase warning path** (some hex + some non-hex phases → `-76403` warning, NaN values for non-hex feature tuples). The algorithm code-path exists but is not exercised.
- No test for the **`counter[i] == 0` rescue path** (hex feature with all voxels masked → output set to (0,0,1)). Reachable in principle but the test data does not appear to construct it.
- No invariant assertion — the test relies entirely on bit-exact (within 5e-7) comparison to a pre-baked exemplar. There is no oracle-style check that output vectors are unit-norm, that NaN voxels propagate to NaN feature output, or that c-axis equivalence (c ≡ −c) is handled consistently.

## Exemplar archive

There are **two** archives referenced by this filter's tests:

| Archive | SHA512 | Used by |
|---|---|---|
| `7_2_AvgCAxis.tar.gz` | `054d1fbb92baacfa79f0bd326ae32b5bfc67d0935413ea8f194980527ff9694bd21a49f73e43d59b731a567ea89190523176bf5c98e8585e7b206265d5b05143` | Valid-execution test (primary) |
| `caxis_data.tar.gz` | `56468d3f248661c0d739d9acd5a1554abc700bf136586f698a313804536916850b731603d42a0b93aae47faf2f7ee49d4181b1c3e833f054df6f5c70b5e041dc` | Invalid-execution test only (provides input data; `7_0_find_caxis_data.dream3d`) |

- **Referenced in:** `src/Plugins/OrientationAnalysis/test/CMakeLists.txt` lines 131 and 140
- **Provenance:** *(TBD — engineer must inspect both archives to determine how the exemplars were generated and whether an Oracle Provenance block exists in any ReadMe inside them.)* The `7_2_` prefix on the primary archive suggests it was regenerated post-PR-#1438 (which changed the algorithm to double precision); the `7_0_` filename inside `caxis_data` suggests legacy DREAM3D-style provenance.
- **Action required:** Download both archives, inspect for ReadMe / pipeline / input-data, and decide whether `7_2_AvgCAxis.tar.gz` is itself a circular oracle (regenerated from the very SIMPLNX code under test) — if so, it cannot serve as an independent oracle and a new dataset must be derived from legacy.

## Oracle classification (tentative)

- **Recommended class:** **3 (Paper-based)** — Rowenhorst et al. 2015 ("Consistent representations of and conversions between 3D rotations", *MSMSE* 23 083501) is the standard reference for the quaternion-to-orientation-matrix conversion used here, and Bunge's "Texture Analysis in Materials Science" is the standard reference for the c-axis-from-orientation derivation. The algorithm is: rotate `[0,0,1]` by the per-voxel quaternion (passive convention, then transpose to active), accumulate, renormalise.
- **Companion class:** **4 (Invariant-based)** — strong invariants exist:
  - Output must be unit-norm (`||avgCAxes[i]|| == 1`) for any feature with at least one hex voxel and at least one non-zero contribution.
  - For a feature whose voxels all share the same quaternion `q`, `avgCAxis(feature) == rotate([0,0,1], q)` exactly.
  - For non-hex feature voxels: output must be NaN.
  - For hex features with `counter == 0` (all voxels masked or invalid): output must be exactly `(0,0,1)` — see line 162-164 of the algorithm.
- **Companion class:** **1 (Analytical)** — A single-voxel hex feature with quaternion `(1,0,0,0)` (identity) must produce `avgCAxis == (0,0,1)` to within float epsilon.
- **Antipodal-flip note (likely Deviation candidate vs. SIMPL):** Because hex symmetry treats `c` and `−c` as equivalent, the algorithm uses a running-average dot-product test (`if w < 0 then c1 *= -1`) to keep contributions co-aligned. The reordering of `counter[currentFeatureId]++` in PR #1438 changed *when* this dot-product test is computed for the first voxel of each feature. Re-read the SIMPL legacy to confirm whether the legacy version had the increment before or after the dot-product test — this is the most likely source of small per-feature sign flips between SIMPLNX and legacy.
- **Action required:** Developer to defend or replace the Class 3 / 4 / 1 stack, and to add a paper reference into the algorithm header.

## V&V status so far

| Item | Status | Notes |
|---|---|---|
| Algorithm review (`review-algorithm` skill) | Partial | PR #1438 commit message says "Compute Average C-Axis was updated and documented with comments" — comments are present in the code, but no formal review checklist has been recorded. |
| Code path coverage (algorithmic) | Weak | Only 1 positive-path test (single exemplar) and 1 negative-path test (all-non-hex). Mixed-phase warning path and `counter==0` rescue path are uncovered. |
| Code path coverage (SIMPL conversion) | Good | PR #1588 added SIMPL 6.4 + 6.5 conversion test; PR #1476 fixed the actual conversion mapping. |
| Exemplar data in Data_Archive | **Yes (×2)** | `7_2_AvgCAxis.tar.gz` (primary) and `caxis_data.tar.gz` (invalid-test input). |
| Exemplar provenance documented | Unknown | TBD by inspecting archive contents. The `7_2_` prefix suggests post-PR-#1438 regeneration, which would make it a circular oracle. |
| Oracle class recorded | **No** | This document is the first to propose one. |
| Toy data / independent expected output (Step 0 c) | No | No script or hand-derivation on file. Identity-quaternion check (Class 1) would be ~5 lines. |
| Legacy comparison report (Step 0 e) | No | `compare-legacy-dream3d` has not been run. PR #1438's float→double change and counter-reorder are guaranteed to produce small Deviations. |
| Deviation entries (`ComputeAvgCAxes-D<N>`) | None | Not yet written. PR #1438 generates ≥3 candidate entries (precision, counter ordering, error-code/preflight-warning surface). |
| Documentation currency | Probably current | Updated by PR #1547 (subgroup typo). The user-facing docs do not mention the antipodal-flip handling, the double-precision accumulation, or the `(0,0,1)` rescue value. |
| Verification archive (OneDrive) | No | Not yet created. |

## Gaps to close (to meet Step 0 / Legacy Comparison policy)

1. **Confirm the oracle.** Check the algorithm header for a paper reference; if Rowenhorst 2015 / Bunge can be cited, lock in Class 3 + Class 4 + Class 1 stack.
2. **Add the missing invariant assertions** to the existing test: unit-norm of every non-NaN output, NaN propagation for non-hex features, exact `(0,0,1)` for the all-masked rescue case, and a Class-1 identity-quaternion analytical check.
3. **Add coverage for the mixed-phase warning path.** Mutate `crystalStructs[2]` in a multi-phase exemplar so one phase is hex and another is cubic, assert warning `-76403` is emitted and that the cubic feature's tuple is NaN.
4. **Inspect both archives and document provenance.** Critical question: was `7_2_AvgCAxis.tar.gz` regenerated from the post-PR-#1438 SIMPLNX output? If yes, the test is circular and must be supplemented with either (a) a hand-derived analytical exemplar or (b) a legacy-DREAM3D-derived independent exemplar.
5. **Run the legacy comparison.** Use `compare-legacy-dream3d` against DREAM3D 6.5.172 `FindAvgCAxes`. Expected outcome: at least one numerical Deviation due to the float→double change (PR #1438), possibly a sign-flip Deviation on first-voxel-of-feature due to the counter-reorder (PR #1438), and a preflight-warning-surface Deviation (PR #1438 dropped the `-6401` warning).
6. **Confirm the EbsdLib 2.0 convention parity.** Verify that `ebsdlib::QuaternionDType::toOrientationMatrix()` produces the same passive-rotation matrix as the legacy `OrientationTransformation::qu2om<QuatD,OrientationD>(...)` (PR #1472). If not, that is an additional Deviation source.
7. **Produce the Algorithm Relationship one-liner.** Tentative: *"Port — translation of SIMPL `FindAvgCAxes` with three intentional changes applied during PR #1438 (float→double precision, counter-increment reordering, error-code re-numbering / preflight-warning removal) and an EbsdLib API refactor in PR #1472."*
8. **Archive everything** per `archive-filter-verification` for the OneDrive folder.

## Recommended Deviation entries (proposed, pending legacy comparison)

> **Deviation ID:** `ComputeAvgCAxes-D1`
> **Filter UUID:** `453cdb58-7bbb-4576-ad5e-f75a1c54d348`
> **Symptom:** SIMPLNX output average c-axis vectors differ from SIMPL `FindAvgCAxes` output in the 6th–7th decimal place across most features.
> **Root cause:** PR #1438 changed the accumulation precision from `float`/`Eigen::Vector3f` to `double`/`Eigen::Vector3d` (and `OrientationF` → `OrientationD`, `QuatF` → `QuatD`, `Matrix3fR` → `Matrix3dR`). Final output is still cast to `float32` for storage, but intermediate sums are now in double precision.
> **Affected users:** Anyone bit-comparing average c-axis vectors against legacy DREAM3D output. Magnitudes and inter-feature relationships are unaffected.
> **Recommendation:** Trust SIMPLNX. Double-precision accumulation is the standard for averaging directional data. Document the precision change in the user-facing docs.
> **Status:** Proposed — pending verification that 6.5.172 actually exhibits float-precision accumulation (read legacy source).

> **Deviation ID:** `ComputeAvgCAxes-D2`
> **Filter UUID:** `453cdb58-7bbb-4576-ad5e-f75a1c54d348`
> **Symptom:** For features whose first hex voxel happens to lie on the antipodal half of the c-axis hemisphere, the average c-axis sign may flip vs. legacy.
> **Root cause:** PR #1438 moved `counter[currentFeatureId]++` to *before* the running-average antipodal-flip test. In the old code, `counter` was 0 for the first voxel and so the per-feature running average `curCAxis = avgCAxes[..]/counter[..]` divided by zero on cell #1, producing `inf`/`NaN` in `curCAxis` and (depending on what `CosBetweenVectors(c1, NaN-vec)` returned) potentially **always passing** the `w < 0` test, i.e. always flipping the first voxel. The new code increments `counter` to 1 first, so `curCAxis = avgCAxes[..]/1 = (0,0,0)` on cell #1, `CosBetweenVectors(c1, zero-vec) = 0`, and the flip is **never** applied to the first voxel.
> **Affected users:** Any user comparing per-feature c-axis sign against legacy. Magnitude is unchanged; only the sign of the antipodal hemisphere choice can differ. Because `c ≡ −c` for hex symmetry, downstream texture analyses are typically unaffected — but per-feature scalar comparisons are.
> **Recommendation:** Trust SIMPLNX. The legacy behaviour was a divide-by-zero artefact. Verify by reading SIMPL `FindAvgCAxes` source.
> **Status:** Proposed — needs legacy source confirmation. **High-priority** Deviation candidate because the symptom is silent (sign flip without magnitude change).

> **Deviation ID:** `ComputeAvgCAxes-D3`
> **Filter UUID:** `453cdb58-7bbb-4576-ad5e-f75a1c54d348`
> **Symptom:** A pipeline that previously produced a preflight warning (`-6401` "Finding the average c-axes requires Hexagonal-Low 6/m or Hexagonal-High 6/mmm…") now produces no preflight warning; the message has been moved to a non-error `preflightUpdatedValues` info entry titled "Crystal Symmetry Warning:". Error codes `-6402`/`-6403` were also re-numbered to `-76402`/`-76403`.
> **Root cause:** PR #1438 reclassified the message from a `Result<>` warning to a `preflightUpdatedValues` entry, and re-numbered the error codes.
> **Affected users:** Anyone parsing pipeline logs by error code, or whose CI greps for the preflight warning text.
> **Recommendation:** Trust SIMPLNX (the new behaviour is more user-friendly). Note the error-code change in release notes.
> **Status:** Proposed — confirmed by source diff.

> **Deviation ID:** `ComputeAvgCAxes-D4` *(speculative)*
> **Filter UUID:** `453cdb58-7bbb-4576-ad5e-f75a1c54d348`
> **Symptom:** Output average c-axis vectors may differ from legacy if the EbsdLib 2.0 `QuaternionDType::toOrientationMatrix()` uses a different rotation convention than the legacy `OrientationTransformation::qu2om<QuatD,OrientationD>(...)`.
> **Root cause:** PR #1472 swapped the EbsdLib API call style. Functional equivalence has not been independently verified.
> **Affected users:** All users.
> **Recommendation:** Read EbsdLib 2.0 source to confirm convention parity. If parity holds, withdraw this Deviation.
> **Status:** Proposed — needs EbsdLib 2.0 source inspection. May not be a real Deviation.
