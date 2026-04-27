# Retroactive V&V: ComputeFeatureReferenceCAxisMisorientationsFilter

*Report status:* **DRAFT**. Generated from git-history and source-tree inspection. Developer must confirm or correct the Oracle class, Algorithm Relationship, and the V&V status entries.

## Metadata

| Field | Value |
|---|---|
| SIMPLNX UUID | `16c487d2-8f99-4fb5-a4df-d3f70a8e6b25` |
| SIMPLNX ClassName | `ComputeFeatureReferenceCAxisMisorientationsFilter` |
| SIMPLNX Human Name | Compute Feature Reference C-Axis Misalignments |
| SIMPL UUID (legacy) | `1a0848da-2edd-52c0-b111-62a4dc6d2886` |
| SIMPL ClassName | `FindFeatureReferenceCAxisMisorientations` |
| SIMPL Human Name | Compute Feature Reference C Axis Misorientations |
| Plugin | OrientationAnalysis |

### Source files scanned

- `src/Plugins/OrientationAnalysis/src/OrientationAnalysis/Filters/ComputeFeatureReferenceCAxisMisorientationsFilter.{hpp,cpp}`
- `src/Plugins/OrientationAnalysis/src/OrientationAnalysis/Filters/Algorithms/ComputeFeatureReferenceCAxisMisorientations.{hpp,cpp}`
- `src/Plugins/OrientationAnalysis/test/ComputeFeatureReferenceCAxisMisorientationsTest.cpp`
- `src/Plugins/OrientationAnalysis/test/simpl_conversion/6_5/ComputeFeatureReferenceCAxisMisorientationsFilter.json`
- `src/Plugins/OrientationAnalysis/test/simpl_conversion/6_4/ComputeFeatureReferenceCAxisMisorientationsFilter.json`
- `src/Plugins/OrientationAnalysis/docs/ComputeFeatureReferenceCAxisMisorientationsFilter.md`
- `src/Plugins/OrientationAnalysis/test/CMakeLists.txt` (test data registration)
- `src/Plugins/OrientationAnalysis/pipelines/EBSD_File_Processing/EBSD_Hexagonal_Data_Analysis.d3dpipeline` (referenced in docs as example pipeline)

## Algorithm Relationship

- **Tentative classification:** **Port + Bug Fix**. The SIMPLNX filter is a direct translation of the legacy SIMPL `FindFeatureReferenceCAxisMisorientations` — the legacy UUID `1a0848da-2edd-52c0-b111-62a4dc6d2886` is preserved in a comment at the bottom of the .hpp, and the SIMPL conversion fixture maps each parameter 1-for-1. PR #1438 then performed an explicit code review (per the commit message `REV: Compute Feature Reference CAxis Misorientations has been reviewed.`) that included a documented numerical fix (use doubles to accumulate the StdDev so output agrees with DREAM3D > 6.5.172) and a parameter-struct rename (`*ArrayName` → `*ArrayPath`). The current docs note: *"Results from this filter can differ from its original version in DREAM3D 6.6 by around 0.0001."*
- **Note vs. brief:** The brief envisioned two reference modes (average c-axis vs. arbitrary reference voxel). The SIMPLNX implementation **only supports the average c-axis** mode — it consumes a pre-computed `AvgCAxes` feature-data array (typically produced by `ComputeAvgCAxesFilter`). There is no "reference voxel" code path here; that capability — if it exists at all — would belong to a sibling filter (e.g., `ComputeFeatureReferenceMisorientationsFilter`, which PR #1504 modified for that exact purpose). Brief corrected.
- **Action required:** Confirm by running `compare-legacy-dream3d` against SIMPL 6.5.172 on a hex EBSD toy dataset; the expected delta is the ≤ 0.0001 noted in the docs.

## PRs inspected (since 2025-10-01)

> Pruned: pure-style/repo-wide refactor PRs that only touched mechanical text in this filter (#1457 static-inline cleanup, #1538 zlib extraction, and #1439 multi-dimensional tuple support which only touched a `ShapeType` alias here) are listed at the bottom of this section but not detailed individually — they did not change the behavior of this filter.

### PR #1438 — *"ENH: Microtexture related filter cleanup"* — merged 2025-10-25

- **Files in this filter:** algorithm (.hpp +18, .cpp +135 lines reworked), filter (.cpp +8)
- **Diff size:** 3 files, +88 / −73
- **Change nature:** **Material — code review + documented bug fix.** The commit body explicitly states `BUG: FindFeatureReferenceCAxisOrientation - Use doubles to accumulate the StdDev values. Output values now agree with DREAM.3D > 6.5.172` and `REV: Compute Feature Reference CAxis Misorientations has been reviewed.` Specific changes visible in the diff:
  - StdDev accumulator changed from `float32` to `double` (`std::vector<double> stdevs`).
  - Per-feature average misorientation now divided by `counts[featureId]` (an explicit per-feature counter), not implicitly averaged via a stale `featAvgCAxisMis` slot.
  - Output arrays explicitly zero-filled before the loop (`featAvgCAxisMis.fill(0.0f)`, `featStdevCAxisMis.fill(0.0f)`).
  - Loop nest order changed from `col → row → plane` to the natural `plane → row → col` (correctness-preserving but matches index formula `plane * xPoints * yPoints + row * xPoints + col`).
  - Removed an obsolete 32-bit guard (`totalPoints > std::numeric_limits<uint32>::max()` check).
  - Parameter struct field rename: `FeatureAvgCAxisMisorientationsArrayName` → `FeatureAvgCAxisMisorientationsArrayPath` (and the same for the Stdev/Reference fields). Type stayed `DataPath`; only the field name was made consistent with the rest of the codebase.
  - Typo fix in two error/warning strings: "mis orientation" → "misorientation".
  - Section comments added throughout the algorithm.
- **V&V content:** **High.** This is the closest thing to per-filter V&V work in the entire history. Numerical-correctness fix + documented agreement claim with legacy DREAM3D (`> 6.5.172`). The docs were updated to reflect this in the same series (note the user-facing line in the doc: *"This version uses double precision in part of its calculation to improve agreement and accuracy between platforms (notably ARM)."*). However: no new unit-test was added with this fix, so the "now agrees with DREAM3D" claim is **not regression-locked** in the repository.

### PR #1472 — *"ENH: Update to EbsdLib 2.0.0 API"* — merged 2025-11-24 (broad refactor, exception flagged because this filter delegates orientation math to EbsdLib)

- **Files in this filter:** algorithm (.cpp) only
- **Diff size:** small, +5 / −5
- **Change nature:** API rename + math-call rewrite. The orientation-matrix construction changed from
  - `OrientationTransformation::qu2om<QuatD, OrientationD>(...)` followed by `OrientationMatrixToGMatrixTranspose(...)` and a left-multiply
  to
  - `ebsdlib::QuaternionDType(...).toOrientationMatrix()` followed by an explicit `oMatrix.transpose() * cAxis`.
  The `EbsdLib::CrystalStructure::Hexagonal_*` enum was also renamed to lowercase `ebsdlib::CrystalStructure::Hexagonal_*`.
- **V&V content:** **Medium-risk surface area.** The new code path *should* be mathematically identical (transpose-of-orientation-matrix times c-axis is the rotated c-axis), but the swap is non-trivial and unit-tested only via the existing exemplar comparison in `caxis_data.tar.gz`. Promoted (not pruned) per the audit policy: this filter delegates the load-bearing math to EbsdLib so the bump is materially relevant to its output.

### PR #1547 — *"DOC: Fix filter documentation and documentation related code bugs"* — merged 2026-03-10

- **Files in this filter:** docs (.md), 1 line changed
- **Change nature:** Pure doc-text edit (+1 / −1).
- **V&V content:** None.

### PR #1582 — *"ENH: Add missing cancel checks to lots of filters"* — merged 2026-04-08 (broad refactor, exception flagged because the additions land inside this filter's compute loops)

- **Files in this filter:** algorithm (.cpp) only
- **Diff size:** +20 / −0
- **Change nature:** Inserts `if(m_ShouldCancel) return {};` guards in four loops: the outer `plane` loop in the per-cell pass, the per-feature average loop, the per-cell stdev-accumulation loop, and the final per-feature stdev finalization loop.
- **V&V content:** Cancel-responsiveness only. No algorithmic change. Behavior on completed (non-cancelled) runs is identical.

### PR #1588 — *"ENH: SIMPL Backwards Compatibility Test Redesign"* — merged 2026-04-22

- **Files in this filter:** test (.cpp) +52 lines, plus two new fixture files
  - `test/simpl_conversion/6_4/ComputeFeatureReferenceCAxisMisorientationsFilter.json` (~1.0 KB)
  - `test/simpl_conversion/6_5/ComputeFeatureReferenceCAxisMisorientationsFilter.json` (~1.0 KB; legacy UUID `1a0848da-...`, legacy class name `FindFeatureReferenceCAxisMisorientations`)
- **Change nature:** **Test addition.** Adds a per-filter SIMPL→SIMPLNX backwards-compatibility test that exercises both SIMPL 6.4 (`Filter_Name` fallback) and 6.5 (UUID-mapped) pipeline conversion paths via `DYNAMIC_SECTION`. Test name: `"OrientationAnalysis::ComputeFeatureReferenceCAxisMisorientationsFilter: SIMPL Backwards Compatibility"`. Verifies all 8 parameter keys round-trip correctly.
- **V&V content:** **Pipeline-conversion correctness only** — verifies that opening a legacy SIMPL pipeline in DREAM3DNX produces a filter instance with the right parameter values. It does **not** verify that the filter's *output* matches legacy. That latter step is still missing.

### Pruned PRs (touched the file but not behaviorally relevant to this filter)

| PR | Subject | Why pruned |
|---|---|---|
| #1439 | Multi-Dimensional Tuple Support for StringArray and NeighborList | The only touch point in this filter was a 1-line `ShapeType` alias; no algorithmic change. |
| #1457 | Clean up 'static inline' from filter headers | Pure style. |
| #1538 | Replace cmake subprocess tar.gz extraction with zlib in unit tests | Test infrastructure (the `TestFileSentinel` constructor lost an unused first argument). |

## Test coverage detected

`ComputeFeatureReferenceCAxisMisorientationsTest.cpp` contains **3 `TEST_CASE`s**:

1. `OrientationAnalysis::ComputeFeatureReferenceCAxisMisorientationsFilter: Valid Filter Execution` — loads `7_0_find_caxis_data.dream3d` from `caxis_data.tar.gz`, runs the filter, and compares all three output arrays (FeatureRefCAxisMis cell array, FeatureAvgCAxisMis feature array, FeatureStdevCAxisMis feature array) against exemplars using `UnitTest::CompareFloatArraysWithNans<float32>` with `UnitTest::EPSILON`.
2. `OrientationAnalysis::ComputeFeatureReferenceCAxisMisorientationsFilter: InValid Filter Execution` — overwrites `crystalStructs[1] = 1` (Cubic_High) so no hex phase exists, expects `executeImpl` to return error `-9802`.
3. `OrientationAnalysis::ComputeFeatureReferenceCAxisMisorientationsFilter: SIMPL Backwards Compatibility` — SIMPL 6.4 + 6.5 conversion paths via `DYNAMIC_SECTION` *(added by PR #1588)*.

There is **only one happy-path algorithmic test** — it covers the all-hex case but does not exercise the *mixed-phase* warning path (`-9803` "non-hexagonal phases will be skipped") or the boundary `cellPhase == 0 / cellFeatureId == 0` skip path. The per-voxel `if(w > 90.0) w = 180.0 - w` antipodal-fold branch is also exercised only implicitly via the exemplar.

## Exemplar archive

- **Archive name:** `caxis_data.tar.gz`
- **SHA512:** `56468d3f248661c0d739d9acd5a1554abc700bf136586f698a313804536916850b731603d42a0b93aae47faf2f7ee49d4181b1c3e833f054df6f5c70b5e041dc`
- **Referenced in:** `src/Plugins/OrientationAnalysis/test/CMakeLists.txt` line 140
- **Inner exemplar file used:** `caxis_data/7_0_find_caxis_data.dream3d`
- **Shared with:** Other CAxis filters (e.g., `CAxisSegmentFeaturesFilter` references the same `caxis_data.tar.gz`).
- **Provenance:** *(TBD — engineer must inspect the archive to determine how the exemplar was generated. The `7_0_` prefix is unusual for a current-NX archive and suggests it was regenerated by a recent NX pipeline rather than carried forward from legacy DREAM3D.)*
- **Action required:** Download the archive locally and inspect for: an inner `ReadMe.md`, the `.d3dpipeline` files used to generate the exemplars, and provenance notes. Promote into the verification archive ReadMe per Step 0's Oracle Provenance policy. Critically, given the docs claim *"Results from this filter can differ from its original version in DREAM3D 6.6 by around 0.0001"*, confirm whether the exemplar was generated by the *new* (post-PR-#1438) NX algorithm or by legacy DREAM3D 6.6 — this changes whether the unit test is regression-locked to the *fixed* behavior or to the *legacy* behavior.

## Oracle classification (tentative)

The brief recommended **Class 3 (Paper-based, Rowenhorst et al. 2015) + Class 4 (Invariant)**. After source inspection:

- **Recommended class:** **4 (Invariant-based)** as the primary classification, with **3 (Paper-based)** as a companion if a citation is added to the algorithm header. There is no Rowenhorst-style "axis–angle of disorientation" computation here — the algorithm is a *single-axis* angular metric `angle(c_voxel, c_feature_avg)` with antipodal folding, which is much simpler than a full Rodrigues/quaternion disorientation calculation.
- **Defensible invariants for a Class-4 oracle:**
  1. **Range:** every output value `cellRefCAxisMis[i]` is in `[0°, 90°]` (the `if(w > 90.0) w = 180.0 - w` clamp guarantees this; combined with the `std::clamp(w, -1.0, 1.0)` before `acos`, the result is also free of NaN).
  2. **Non-negativity:** every output is `≥ 0` (it's `acos` of a clamped cosine, possibly folded).
  3. **Skip semantics:** for any cell where `cellFeatureId == 0` OR `cellPhase == 0` OR the cell's phase is not Hexagonal_High/Hexagonal_Low, `cellRefCAxisMis[i] == 0.0f` exactly. Verifiable post-condition.
  4. **Per-feature mean:** `featAvgCAxisMis[fId]` equals the arithmetic mean of `cellRefCAxisMis[i]` over all hex cells with `featureIds[i] == fId` and `cellPhases[i] > 0`. (Because the per-cell value is itself zero for skipped cells, but those cells are also excluded from `counts[fId]`, so the average is over hex-only cells.)
  5. **Per-feature stdev:** `featStdevCAxisMis[fId]` equals the population standard deviation (divisor = `counts[fId]`, not `counts[fId]-1`) of those same per-cell values.
  6. **Identity case:** if every hex voxel inside a feature has the same orientation as the feature's `AvgCAxes` direction, every per-voxel value in that feature is exactly `0.0` and so are the feature average and stdev.
  7. **Antipodal symmetry:** the output is invariant under sign-flip of the per-feature `AvgCAxes` row (because the `> 90°` fold cancels the sign). This is the hex-c-axis equivalence and is the load-bearing physics assumption.
  8. **Units:** all outputs are in **degrees** (the algorithm multiplies by `Constants::k_180OverPiD`). The .cpp tooltip on the parameter explicitly says "Misorientation angle (in degrees)".
- **Class-1 (Analytical) corner case for cross-check:** A single feature with one cell whose quaternion is identity (`{1,0,0,0}`) and an `AvgCAxes` of `{0,0,1}` should produce `cellRefCAxisMis[0] == 0.0`. (The orientation matrix for the identity quaternion is `I`, transpose is `I`, and `I·{0,0,1} = {0,0,1}`, dot with `{0,0,1}` = 1, `acos(1) = 0`.)
- **Brief corrections:**
  - The brief described the algorithm as `acos(|reference_caxis · voxel_caxis|)`. The actual code is `acos(clamp(cos, -1, 1))` followed by `if w > 90° then w = 180 − w`. Mathematically equivalent in exact arithmetic, but **numerically distinct near the 90° boundary** — `acos(|x|)` is well-conditioned near `|x|=0`, whereas `acos(x)` followed by a fold can pick up a sign-flip of ~1 ULP. Worth noting in the V&V record.
  - The brief envisioned an `ω` ambiguity between `acos(dot)` and `acos(|dot|)` modes. Source uses the fold form unconditionally; there is no parameter to switch.
  - The brief mentioned a "reference voxel" mode. **Does not exist in this filter.**
- **Action required:** Developer to defend or replace the Class-4 recommendation. If a paper reference (e.g., a microtexture-region book chapter or a hex-misorientation tutorial) can be added to the algorithm header, upgrade to Class 3 + Class 4.

## V&V status so far

| Item | Status | Notes |
|---|---|---|
| Algorithm review (`review-algorithm` skill) | **Done in PR #1438** | Commit body explicitly says `REV: Compute Feature Reference CAxis Misorientations has been reviewed.` Code is well-commented as a result. |
| Code path coverage (algorithmic) | **Partial** | One happy-path exemplar test + one error path. Mixed-phase warning path (`-9803`), cell-with-feature-id-0 skip path, and boundary fold (w near 90°) are not isolated tests. |
| Code path coverage (SIMPL conversion) | Good | PR #1588 added SIMPL 6.4 + 6.5 conversion test. |
| Exemplar data in Data_Archive | **Yes** | `caxis_data.tar.gz` referenced in test/CMakeLists.txt. |
| Exemplar provenance documented | Unknown | TBD by inspecting archive contents. The `7_0_find_caxis_data.dream3d` filename suggests an NX-generated exemplar but is not confirmed. |
| Oracle class recorded | **No** | This document is the first to propose one. |
| Toy data / independent expected output (Step 0 c) | No | No script or hand-derivation on file. |
| Legacy comparison report (Step 0 e) | **Partial — claimed in commit message but not archived** | PR #1438 asserts *"Output values now agree with DREAM.3D > 6.5.172"* but no diff report or run script is checked in. |
| Deviation entries (`ComputeFeatureReferenceCAxisMisorientations-D<N>`) | None | Not yet written. PR #1438's stdev-precision fix is a strong Deviation candidate. |
| Documentation currency | Probably current | Updated by PR #1547 and includes the helpful sentence about the ~0.0001 numerical difference vs. DREAM3D 6.6. Needs accuracy audit per `review-filter-docs`. |
| Verification archive (OneDrive) | No | Not yet created. |

## Gaps to close (to meet Step 0 / Legacy Comparison policy)

1. **Confirm the oracle.** Class 4 (invariant-based) with the eight invariants enumerated above; promote to Class 3 + Class 4 if a paper reference for hex c-axis-distribution analysis can be cited in the algorithm header.
2. **Add the missing algorithmic tests.** At minimum: (a) a mixed-phase test that triggers the `-9803` warning and verifies hex-cells are computed and non-hex-cells get `0.0`; (b) a `featureId == 0` / `cellPhase == 0` skip test that asserts those output cells are exactly `0.0f`; (c) a hand-constructed identity-orientation test (Class 1) that asserts a single voxel of identity orientation and `AvgCAxes={0,0,1}` produces output `0.0`.
3. **Encode the invariants explicitly in the Valid Filter Execution test.** Add `REQUIRE(min(refMisArray) >= 0.0f)` and `REQUIRE(max(refMisArray) <= 90.0f + UnitTest::EPSILON)` so the Class-4 range invariant becomes a regression check, not just a property of the exemplar.
4. **Inspect `caxis_data.tar.gz` and document provenance.** Determine which pipeline produced `7_0_find_caxis_data.dream3d` and whether the exemplar was generated by the post-PR-#1438 NX algorithm or by legacy DREAM3D 6.6. Record the answer in an Oracle Provenance block in the archive ReadMe. (This is the load-bearing question for whether the unit test locks the *new* or *old* behavior.)
5. **Run the legacy comparison and archive it.** PR #1438's commit asserts agreement with DREAM3D > 6.5.172 within ~0.0001; that claim should be reproduced via `compare-legacy-dream3d` and the resulting diff archived alongside the verification folder. Expected outcome: at least one Deviation entry capturing the stdev-precision fix.
6. **Produce the Algorithm Relationship one-liner.** Tentative: *"Port + Bug Fix — direct translation of SIMPL `FindFeatureReferenceCAxisMisorientations`; PR #1438 changed StdDev accumulation to double-precision (now agrees with DREAM3D > 6.5.172 to ~0.0001) and renamed the parameter-struct fields; PR #1472 rewrote the orientation-matrix call to the EbsdLib 2.0.0 API."*
7. **Archive everything** per `archive-filter-verification` for the OneDrive folder.

## Recommended Deviation entries (proposed, pending legacy comparison)

> **Deviation ID:** `ComputeFeatureReferenceCAxisMisorientations-D1`
> **Filter UUID:** `16c487d2-8f99-4fb5-a4df-d3f70a8e6b25`
> **Symptom:** SIMPLNX `featStdevCAxisMis` differs from SIMPL 6.5.172 output by amounts up to ~0.0001 (per the user-facing doc); larger deviations may appear on ARM platforms.
> **Root cause:** Legacy DREAM3D ≤ 6.5.172 accumulated the squared deviations for population standard deviation in `float32`. SIMPLNX (post-PR-#1438) accumulates in `double` and only narrows when writing the final `float32` array. Since stdev is a sum-of-squares whose dynamic range is order-of-magnitude wider than the inputs, single-precision accumulation accrues catastrophic cancellation on large feature-cell counts.
> **Affected users:** Anyone comparing exact stdev values against legacy outputs; users on ARM where the legacy single-precision rounding mode differs.
> **Recommendation:** **Trust SIMPLNX.** Legacy was less accurate. A back-port to legacy DREAM3D is mechanically simple (change one accumulator type) but probably not worth the effort.
> **Status:** Proposed — pending legacy `compare-legacy-dream3d` run that quantifies the actual delta.

> **Deviation ID:** `ComputeFeatureReferenceCAxisMisorientations-D2` *(potential — needs verification)*
> **Filter UUID:** `16c487d2-8f99-4fb5-a4df-d3f70a8e6b25`
> **Symptom:** Per-voxel angles in the open neighborhood of `w = 90°` may differ between SIMPLNX and legacy DREAM3D by ~1 ULP.
> **Root cause:** Both versions compute `acos(clamp(dot, -1, 1))` and then fold via `if(w > 90°) w = 180 − w`. The fold is sensitive to whether `dot` was rounded above or below the value that would produce `acos = 90°`. The mathematically cleaner form `acos(|dot|)` would avoid the fold but is not what either version uses.
> **Affected users:** Edge-case bit-exact regressions; visualization will be unaffected.
> **Recommendation:** Document as expected. If bit-exactness matters, refactor both legacy and SIMPLNX to `acos(min(|dot|, 1.0))`.
> **Status:** Proposed — only confirm if legacy comparison reveals 90°-boundary deltas; otherwise withdraw.

> **Deviation ID:** `ComputeFeatureReferenceCAxisMisorientations-D3` *(potential — needs verification of EbsdLib 2.0.0 math equivalence)*
> **Filter UUID:** `16c487d2-8f99-4fb5-a4df-d3f70a8e6b25`
> **Symptom:** Output values may shift by ULP-level amounts between SIMPLNX commits before and after PR #1472.
> **Root cause:** PR #1472 swapped from `OrientationTransformation::qu2om<QuatD,OrientationD>` + `OrientationMatrixToGMatrixTranspose` + left-multiply to `ebsdlib::QuaternionDType.toOrientationMatrix()` + `oMatrix.transpose() * cAxis`. The two are mathematically equivalent but reorder floating-point operations, which can produce ULP-level differences.
> **Affected users:** Anyone bisecting unit-test exemplar updates across the EbsdLib 2.0.0 bump.
> **Recommendation:** Verify by re-running the `caxis_data.tar.gz` exemplar at the PR #1472 boundary and recording any delta. If the delta is below `UnitTest::EPSILON`, no action; otherwise note it here.
> **Status:** Proposed — withdraw if the exemplar test passes unchanged at the PR #1472 boundary.
