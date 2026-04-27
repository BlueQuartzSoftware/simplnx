# Retroactive V&V: RotateEulerRefFrameFilter

*Report status:* **DRAFT**. Generated from git-history and source-tree inspection. Developer must confirm or correct the Oracle class, Algorithm Relationship, and the V&V status entries.

## Metadata

| Field | Value |
|---|---|
| SIMPLNX UUID | `0458edcd-3655-4465-adc8-b036d76138b5` |
| SIMPLNX ClassName | `RotateEulerRefFrameFilter` |
| SIMPLNX Human Name | Rotate Euler Reference Frame |
| SIMPL UUID | *(TBD — confirm in legacy SIMPL repo)* |
| SIMPL ClassName | `RotateEulerRefFrame` (assumed; confirm in legacy SIMPL repo) |
| SIMPL Human Name | *(TBD — confirm in legacy SIMPL repo)* |
| Plugin | OrientationAnalysis |

### Source files scanned

- `src/Plugins/OrientationAnalysis/src/OrientationAnalysis/Filters/RotateEulerRefFrameFilter.{hpp,cpp}`
- `src/Plugins/OrientationAnalysis/src/OrientationAnalysis/Filters/Algorithms/RotateEulerRefFrame.{hpp,cpp}`
- `src/Plugins/OrientationAnalysis/test/RotateEulerRefFrameTest.cpp`
- `src/Plugins/OrientationAnalysis/test/simpl_conversion/6_5/RotateEulerRefFrameFilter.json`
- `src/Plugins/OrientationAnalysis/test/simpl_conversion/6_4/RotateEulerRefFrameFilter.json`
- `src/Plugins/OrientationAnalysis/docs/RotateEulerRefFrameFilter.md`

## Algorithm Relationship

- **Tentative classification:** **Port** — the SIMPLNX filter appears to be a direct translation of the legacy SIMPL implementation; the SIMPLNX UUID is preserved and a `FromSIMPLJson()` converter is in place that maps the legacy `RotationAxis` + `RotationAngle` into the unified `rotation_axis_angle` 4-tuple.
- **Evidence:** No rewrite signal in the PR history. The Algorithm/Filter split is in place (Algorithm/`RotateEulerRefFrame.{hpp,cpp}` is *not* in `Algorithms/not_used/`, so the Algorithm-class extraction has already been performed). The math (axis-angle → orientation matrix → Euler) follows the standard Bunge convention conversion using EbsdLib transformation primitives.
- **Action required:** Confirm by reading the corresponding SIMPL filter source and running `compare-legacy-dream3d` step (e) against a shared toy dataset. **Pay particular attention to the precision change introduced by PR #1472 (EbsdLib 2.0)** — see Deviation `RotateEulerRefFrame-D1` below.

## PRs inspected (since 2025-10-01)

> Pruned: pure-style/repo-wide refactor PRs (#1457 static-inline cleanup did not touch this filter; #1524 test-tag fix; #1538 zlib extraction) are listed at the bottom of this section but not detailed individually — they did not change behavior of this filter.

### PR #1472 — *"ENH: Update to EbsdLib 2.0.0 API"* — merged 2025-11-24

- **Files in this filter:** algorithm (.cpp) only
- **Diff size:** 1 file, +11 / -13 lines
- **Change nature:** **Substantive — API migration with implicit precision change.** The algorithm was rewritten from the EbsdLib 1.x `OrientationTransformation::ax2om<OrientationF, OrientationF>` / `eu2om<OrientationF, OrientationF>` / `om2eu<OrientationF, OrientationF>` chain (single-precision `OrientationF`) to EbsdLib 2.0's `ebsdlib::AxisAngleDType` / `ebsdlib::OrientationMatrixDType` / `ebsdlib::EulerDType` types. The intermediate `OrientationUtilities::OrientationMatrixToGMatrix` call was replaced with `om.toEigenGMatrix()`. The `Matrix3fR` (float) intermediate was changed to `Matrix3dR` (double). The constant `nx::core::numbers::pi / 180.0F` lost its `F` suffix and is now `180.0` (double). Net effect: the per-voxel arithmetic is now done in **double precision** before being demoted back to the `float32` storage array.
- **V&V content:** **None visible in the PR.** No new test, no exemplar update, no documentation note. The behavior of the filter has changed at the bit level — values may differ in the last few ULPs of the float32 result. Whether the existing `0.0001` tolerance in the unit test is wide enough to absorb the change is the only check that has happened.
- **Action required:** Run `compare-legacy-dream3d` to quantify the magnitude of the precision shift on a representative dataset. If diffs exceed downstream tolerances (e.g. for IPF coloring), file as Deviation `RotateEulerRefFrame-D1`.

### PR #1501 — *"COMP: Combine Matrix3x1, Point3D, Vec3 into a Vec3<T> in Array.hpp"* — merged 2026-02-23

- **Files in this filter:** algorithm (.cpp) only
- **Diff size:** 1 file, +5 / -5 lines
- **Change nature:** **Refactor (broad refactor; flagged because it touched this filter's only source file).** Replaced the `std::vector<float>` axis storage with `nx::core::FloatVec3`, replaced `MatrixMath::Normalize3x1(axis.data())` with `axis = axis.normalize()`, and changed the Impl class member from `std::vector<float> m_AxisAngle` to `FloatVec3 m_AxisAngle`. `MatrixMath` source file was removed plugin-wide.
- **V&V content:** None. Pure type refactor; the underlying normalize math should be equivalent.
- **Risk:** Low. Worth a quick spot-check that `FloatVec3::normalize()` produces the same result as `MatrixMath::Normalize3x1` on a non-unit axis like `(1,1,1)` (the value the unit test uses).

### PR #1547 — *"DOC: Fix filter documentation and documentation related code bugs"* — merged 2026-03-10

- **Files in this filter:** docs (.md), +1 / -1 lines
- **Change nature:** Documentation hygiene — corrected the "Group (Subgroup)" line from `Orientation Analysis (Conversion)` to `Processing (Conversion)` to match the filter's `defaultTags()` (which lists `"Processing", "Conversion", ...`).
- **V&V content:** Doc currency improvement. Not algorithmic.

### PR #1588 — *"ENH: SIMPL Backwards Compatibility Test Redesign"* — merged 2026-04-22

- **Files in this filter:** test (.cpp) +42 lines, plus two new fixture files
  - `test/simpl_conversion/6_4/RotateEulerRefFrameFilter.json` (~23 lines)
  - `test/simpl_conversion/6_5/RotateEulerRefFrameFilter.json` (~24 lines)
- **Change nature:** **Test addition.** Added a per-filter SIMPL→SIMPLNX backwards-compatibility test that exercises both SIMPL 6.4 (Filter_Name fallback) and 6.5 (UUID-mapped) pipeline conversion paths via `DYNAMIC_SECTION`. Test name: `"OrientationAnalysis::RotateEulerRefFrameFilter: SIMPL Backwards Compatibility"`.
- **V&V content:** **Pipeline-conversion correctness only** — the test verifies that opening a legacy SIMPL pipeline in DREAM3DNX produces a filter instance with the correct `EulerAnglesArrayPath` value. The test explicitly notes that the complex `FloatVec3p1FilterParameterConverter` round-trip is "verified by successful pipeline loading" rather than by an explicit value check on the 4-tuple. It does **not** verify that the filter's *output* matches legacy. That latter step is still missing.

### Pruned PRs (touched the file but not behaviorally relevant to this filter)

| PR | Subject | Why pruned |
|---|---|---|
| #1524 | Fixed filter tags to consistently use the full filter name | Test cosmetic — changed the Catch2 tag string only |
| #1538 | Replace cmake subprocess tar.gz extraction with zlib in unit tests | Test infrastructure — `TestFileSentinel` API tweak only |

## Test coverage detected

`RotateEulerRefFrameTest.cpp` contains 2 `TEST_CASE`s:

1. `OrientationAnalysis::RotateEulerRefFrame` — Loads a 480 000-tuple `EulerAngles.csv` and a precomputed `EulersRotated.csv` from the `ASCIIData` archive, runs the filter with axis `(1,1,1)` and angle `30°`, and asserts every component matches the precomputed array within `1e-4`. **This is exemplar-based verification using a CSV oracle**, not a pipeline/`.dream3d` exemplar.
2. `OrientationAnalysis::RotateEulerRefFrameFilter: SIMPL Backwards Compatibility` — SIMPL 6.4 + 6.5 conversion paths via `DYNAMIC_SECTION` *(added by PR #1588)*. Conversion-only.

Test 1 exercises a single (axis, angle) configuration: `(1, 1, 1)` axis, `30°` angle. There is no explicit coverage of:
- Identity rotation (axis arbitrary, angle = 0°) → output should equal input.
- Rotation followed by inverse rotation → output should equal input within tolerance.
- 360° rotation → output should equal input.
- Pure axis rotations (e.g. (0,0,1) at 90°) where the result is hand-derivable.
- Edge cases (gimbal lock orientations, Euler triple at the wraparound).

## Exemplar archive

- **Archive name:** `ASCIIData.tar.gz`
- **SHA512:** `70388864301ca1ea7fce7b1666d3abf682eee68c7d8b7a9bf532df7aff11e7ea9de7dc2dc80e33f0e363cbad023b663bff97df4be362f0312d311e9d5bedf370`
- **Referenced in:** `src/Plugins/SimplnxCore/CMakeLists.txt` (line 517) — *not* in the OrientationAnalysis test CMakeLists; the OrientationAnalysis test relies on the SimplnxCore download having already happened.
- **Files this filter consumes from the archive:** `ASCIIData/EulerAngles.csv` (input, 480 000 × 3 float32) and `ASCIIData/EulersRotated.csv` (oracle, 480 000 × 3 float32).
- **Provenance:** *(TBD — engineer must inspect the archive to determine how `EulersRotated.csv` was generated and whether an Oracle Provenance block exists in any ReadMe inside it.)* The shared `ASCIIData` archive is also used by `ComputeAvgOrientationsTest`, `ComputeIPFColorsTest`, and others, so any provenance update needs a coordinated audit.
- **Action required:** Download the archive locally and inspect for: an inner `ReadMe.md`, the SIMPL or SIMPLNX pipeline that produced `EulersRotated.csv`, and the (axis, angle) values used. **Critical question:** was `EulersRotated.csv` generated by legacy SIMPL `RotateEulerRefFrame`, by an earlier SIMPLNX commit, or by an independent oracle (Mathematica/Python/paper-based hand calculation)? The answer determines the Oracle class.

## Oracle classification (tentative)

- **Recommended class:** **3 (Paper-based)** as the strongest defensible oracle.
- **Rationale:** The math here is fully published. The Bunge Euler-angle convention (Z-X-Z, intrinsic, passive) is canonically defined; the canonical reference for the conversion math (Euler ↔ orientation matrix ↔ axis-angle, with all sign and order conventions pinned down) is **Rowenhorst et al. (2015), "Consistent representations of and conversions between 3D rotations," *Modelling and Simulation in Materials Science and Engineering*, 23(8):083501**. The filter does:
  1. Convert the user-supplied axis-angle to an orientation matrix `R`.
  2. For each voxel: convert the stored Euler triple to an orientation matrix `g`, compute `g_new = (g * R).colwise().normalized()`, convert `g_new` back to an Euler triple, and write it back.
- **Companion oracles:**
  - **Class 1 (Analytical):** Identity case — for `R = I` (any axis, angle = 0° or 360°), the output Euler triples must equal the input Euler triples within float32 round-trip tolerance.
  - **Class 4 (Invariant):** Applying `R` then `R⁻¹` (i.e., the same axis with negated angle) must return the original orientation. The orientation magnitude (axis-angle θ of the misorientation between input and output) is preserved if `R` is a pure rotation. The disorientation between input voxel `i` and output voxel `i` must equal the rotation angle `w` for every voxel (modulo crystal symmetry, which this filter does *not* apply — it operates on raw orientations).
- **Action required:** Developer to confirm the Bunge convention assumption is consistent with what EbsdLib 2.0's `EulerDType` actually implements (the EbsdLib documentation should be cited). Developer to defend or replace the Class-3 recommendation. The existing CSV-oracle test should be promoted to a Class-3 reference if the CSV's provenance can be tied back to Rowenhorst-style hand calculations or to a known-good SIMPL run.

## V&V status so far

| Item | Status | Notes |
|---|---|---|
| Algorithm review (`review-algorithm` skill) | Not visible from PR history | No PR explicitly performs the line-by-line review. The PR #1472 EbsdLib migration would have benefited from one. |
| Code path coverage (algorithmic) | **Weak** | Only one (axis, angle) configuration is tested. No identity case, no inverse-pair case, no symmetry/wraparound coverage. |
| Code path coverage (SIMPL conversion) | Good | PR #1588 added SIMPL 6.4 + 6.5 conversion test, though the 4-tuple value itself is not asserted (only the array path is). |
| Exemplar data in Data_Archive | **Yes (shared)** | `ASCIIData.tar.gz` referenced in `src/Plugins/SimplnxCore/CMakeLists.txt`. Shared with multiple tests. |
| Exemplar provenance documented | Unknown | TBD by inspecting archive contents. |
| Oracle class recorded | **No** | This document is the first to propose one. |
| Toy data / independent expected output (Step 0 c) | **Partial** | The CSV oracle (`EulersRotated.csv`) is independent expected output, but its provenance is not documented in-tree. |
| Legacy comparison report (Step 0 e) | No | `compare-legacy-dream3d` has not been run. **Strongly recommended** because PR #1472 introduced an implicit precision shift from float to double in the per-voxel arithmetic. |
| Deviation entries (`RotateEulerRefFrame-D<N>`) | None | Not yet written. PR #1472's float→double precision shift is the strongest Deviation candidate. |
| Documentation currency | Probably current | Updated by PR #1547 (Group/Subgroup correction). The body description has not been audited against the current implementation. Needs `review-filter-docs`. |
| Verification archive (OneDrive) | No | Not yet created. |

## Gaps to close (to meet Step 0 / Legacy Comparison policy)

1. **Confirm the oracle.** Class 3 (paper-based, Rowenhorst 2015) is the recommended starting point. Verify that the Bunge convention assumption matches what EbsdLib 2.0's `EulerDType`/`AxisAngleDType`/`OrientationMatrixDType` actually implement. Verify that the multiplication order in `(g * R)` corresponds to the documented "passive rotation of the reference frame" semantics in the user-facing doc.
2. **Add invariant tests.** Promote the existing single-configuration test to a parameterized test that covers, at minimum, (a) the identity case (`angle = 0`), (b) the inverse-pair case (apply R then R⁻¹), and (c) one analytically-derivable rotation (e.g. `(0,0,1)` axis, `90°` angle on a known Euler triple). These should be Class-1/Class-4 assertions written directly in C++, independent of any CSV file.
3. **Inspect `ASCIIData.tar.gz` and document provenance for `EulersRotated.csv`.** Determine how the oracle was generated, whether a SIMPL pipeline produced it, and the exact (axis, angle) used. Write an Oracle Provenance block for the archive ReadMe. Coordinate with the other ASCIIData-consuming tests.
4. **Run the legacy comparison.** Use `compare-legacy-dream3d` to diff SIMPLNX vs. DREAM3D 6.5.171/172 on the same input. The expected outcome includes at minimum one Deviation entry: `RotateEulerRefFrame-D1` for the float→double precision shift introduced in PR #1472, if the legacy version is float and the magnitude of the diff is non-trivial.
5. **Spot-check the FloatVec3 normalize change from PR #1501.** Confirm that `nx::core::FloatVec3::normalize()` produces the same result as `MatrixMath::Normalize3x1` for the unit test's `(1, 1, 1)` axis. Should be identical bit-for-bit, but worth one assertion.
6. **Produce the Algorithm Relationship one-liner.** Tentative: *"Port — direct translation of the SIMPL `RotateEulerRefFrame` filter; no algorithmic correctness changes since import, but the EbsdLib 2.0 migration in PR #1472 implicitly promoted the per-voxel arithmetic from float to double precision."*
7. **Archive everything** per `archive-filter-verification` for the OneDrive folder.

## Recommended Deviation entries (proposed, pending legacy comparison)

> **Deviation ID:** `RotateEulerRefFrame-D1`
> **Filter UUID:** `0458edcd-3655-4465-adc8-b036d76138b5`
> **Symptom:** SIMPLNX produces Euler triples that differ from SIMPL 6.5.172 in the last few ULPs of the float32 result. Exact magnitude is TBD pending `compare-legacy-dream3d`.
> **Root cause:** PR #1472 (EbsdLib 2.0.0 API migration) replaced the EbsdLib 1.x `OrientationTransformation::ax2om<OrientationF, OrientationF>` / `eu2om` / `om2eu` chain (single-precision `OrientationF`) with EbsdLib 2.0's `ebsdlib::AxisAngleDType` / `ebsdlib::OrientationMatrixDType` / `ebsdlib::EulerDType` (double-precision). The intermediate `Matrix3fR` was likewise widened to `Matrix3dR`, and the radians-conversion constant lost its `F` suffix. The per-voxel arithmetic is now done in double precision before being demoted back to the `float32` output array.
> **Affected users:** Anyone whose downstream analysis is sensitive to the last few ULPs of the rotated Euler angle (most won't be; IPF coloring near color-region boundaries is the most likely place to see a visible diff). Anyone bit-exact reproducing legacy results.
> **Recommendation:** Trust SIMPLNX. The double-precision intermediate is mathematically more accurate; the legacy float-only chain accumulated more rounding error per voxel. Document the change. No code action recommended for SIMPLNX.
> **Status:** Proposed — pending verification that 6.5.172 actually produces a measurable diff (run the comparison).

> **Deviation ID:** `RotateEulerRefFrame-D2` *(speculative — only file if the diff in D1 turns out to be larger than expected)*
> **Filter UUID:** `0458edcd-3655-4465-adc8-b036d76138b5`
> **Symptom:** Order of operations or normalization differs between SIMPL and SIMPLNX in a way not explained by precision alone.
> **Root cause hypotheses to investigate:** PR #1472 also removed the explicit `OrientationUtilities::OrientationMatrixToGMatrix(om)` step (now folded into `om.toEigenGMatrix()`); if the two helpers transpose differently, the resulting `g_new = (g * R).colwise().normalized()` is computed against a transposed matrix and the output Eulers will be wrong, not just rounded. Also worth checking: PR #1501's switch from `MatrixMath::Normalize3x1` to `FloatVec3::normalize()` for the input axis.
> **Recommendation:** **Do not file this deviation unless D1's investigation reveals diffs that cannot be explained by precision. Treat as a debugging hypothesis.**
> **Status:** Speculative — placeholder.
