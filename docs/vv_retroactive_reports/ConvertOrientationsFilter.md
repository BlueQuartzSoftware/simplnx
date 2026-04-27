# Retroactive V&V: ConvertOrientationsFilter

*Report status:* **DRAFT**. Generated from git-history and source-tree inspection. Developer must confirm or correct the Oracle class, Algorithm Relationship, and the V&V status entries.

## Metadata

| Field | Value |
|---|---|
| SIMPLNX UUID | `501e54e6-a66f-4eeb-ae37-00e649c00d4b` |
| SIMPLNX ClassName | `ConvertOrientationsFilter` |
| SIMPLNX Human Name | Convert Orientation Representation |
| SIMPL UUID | *(TBD — confirm in legacy SIMPL repo)* |
| SIMPL ClassName | *(TBD — confirm in legacy SIMPL repo; PR #1588 fixtures show `Filter_Name = "ConvertOrientations"`)* |
| SIMPL Human Name | *(TBD — confirm in legacy SIMPL repo)* |
| Plugin | OrientationAnalysis |

### Source files scanned

- `src/Plugins/OrientationAnalysis/src/OrientationAnalysis/Filters/ConvertOrientationsFilter.{hpp,cpp}`
- `src/Plugins/OrientationAnalysis/src/OrientationAnalysis/Filters/Algorithms/ConvertOrientations.{hpp,cpp}`
- `src/Plugins/OrientationAnalysis/test/ConvertOrientationsTest.cpp`
- `src/Plugins/OrientationAnalysis/test/simpl_conversion/6_5/ConvertOrientationsFilter.json`
- `src/Plugins/OrientationAnalysis/test/simpl_conversion/6_4/ConvertOrientationsFilter.json`
- `src/Plugins/OrientationAnalysis/docs/ConvertOrientationsFilter.md`

## Algorithm Relationship

- **Tentative classification:** **Port** — the SIMPLNX filter was originally a direct translation of the legacy SIMPL `ConvertOrientations` filter; the dispatch table was preserved (8 representations × 8 representations = 56 conversion paths). The UUID is preserved.
- **Caveat — significant internal rewrite:** Even though the user-visible behavior is preserved, the *backbone* of the implementation has been re-platformed twice during the audit window:
  1. PR #1468 extracted the inline filter implementation into a separate `ConvertOrientations` algorithm class.
  2. PR #1472 replaced the dispatch over `OrientationTransformation::eu2om<>()` (and 55 sibling free function templates) with an `OC_TBB_IMPL(...)` macro-generated `XxxConvertor` class hierarchy that calls **type-erased EbsdLib representation classes** (`ebsdlib::EulerFType`, `ebsdlib::QuaternionFType`, etc.) using member methods like `inputInstance.toEuler()`. The math now lives entirely inside EbsdLib 2.0.0; this filter is now a thin orchestrator.
- **Action required:** Confirm by running `compare-legacy-dream3d` against DREAM3D 6.5.172 on a shared toy dataset. Because EbsdLib still implements Rowenhorst et al. (2015), no numerical change is *expected*; an empirical confirmation is still required.

## PRs inspected (since 2025-10-01)

> Pruned: pure-style/repo-wide refactor PRs (#1457 static-inline, #1491 clang-format toggle, #1524 tag fix, #1543 doc-pipeline references, #1547 sphinx warnings) are listed at the bottom of this section but not detailed individually — they did not change behavior of this filter.

### PR #1438 — *"ENH: Microtexture related filter cleanup"* — merged 2025-10-25

- **Files in this filter:** filter (.cpp) only — `+4 / -4`
- **Diff size:** Small, but algorithmically meaningful.
- **Change nature:** **Bug fix** — buried inside a multi-bullet microtexture-cleanup omnibus PR was the bullet *"BUG: ConvertOrientations - make sure the quaternion is properly formed before conversion."* The diff:
  - Zero-initialized the `std::array<T, 4> input` buffer (previously uninitialized).
  - Wrapped the input quaternion with `.getPositiveOrientation()` so that `qu2*` conversions always operate on the canonical sign-positive scalar form.
- **V&V content:** **Material.** This is the kind of fix that produces a numerical Deviation vs. legacy SIMPL 6.5.172 if the legacy version still hands raw, possibly negative-scalar quaternions to the conversion routines. Expected symptom: any `Quaternion → *` path on inputs with negative scalar components would have been flipping sign of axis-angle / Rodrigues / Euler output in legacy.

### PR #1468 — *"ENH: ConvertOrientationsFilter uses an Algorithm Class."* — merged 2025-11-11

- **Files in this filter:** new algorithm files created (`Algorithms/ConvertOrientations.{hpp,cpp}`, +60/+677), filter slimmed (+15/-8), test (+43)
- **Diff size:** 5 files, +788 / -8
- **Change nature:** **Refactor (algorithm extraction).** Per the project's CLAUDE.md guideline, the existing inline `executeImpl()` was extracted into a stand-alone `ConvertOrientations` algorithm class with `ConvertOrientationsInputValues` POD. No algorithmic change intended — the same 56-path dispatch was carried over verbatim.
- **V&V content:** Architecture cleanup, no expected output change. New behavior surface: the algorithm now lives in its own translation unit and can be unit-tested independently.

### PR #1472 — *"ENH: Update to EbsdLib 2.0.0 API"* — merged 2025-11-24 *(broad refactor, exception flagged because all orientation math lives in EbsdLib for this filter)*

- **Files in this filter:** algorithm (.cpp) `+387/-822`, algorithm (.hpp) `+12/-?`, filter (.cpp) `-696` (most logic moved to algorithm), test (.cpp) `+47/-?`
- **Diff size:** Massive — `+387 / -1190` net for this filter alone.
- **Change nature:** **Library re-platforming with structural rewrite of the dispatch.** Concretely:
  - **Before:** `OrientationTransformation::eu2om<InputType, OutputType>` (and 55 siblings) — free function templates inside EbsdLib's `OrientationTransformation` namespace, called from a giant `if/else if` ladder. Quaternion-side variants used `ToQuaternion` / `FromQuaternion` adapters.
  - **After:** A macro `OC_TBB_IMPL(TO_REP)` synthesises eight templated `XxxConvertor<T,K,InputType,OutputType>` classes; each iterates tuples and calls the *member method* `inputInstance.to##TO_REP()` on a type-erased EbsdLib representation object (`ebsdlib::EulerFType`, `ebsdlib::OrientationMatrixFType`, `ebsdlib::QuaternionFType`, `ebsdlib::AxisAngleFType`, `ebsdlib::RodriguesFType`, `ebsdlib::HomochoricFType`, `ebsdlib::CubochoricFType`, `ebsdlib::StereographicFType`). Dispatch is now an outer `if(OutputType == ...)` with an inner `switch(InputType)`.
  - **Quaternion handling:** the explicit `Quaternion<T>::Order::VectorScalar` argument that previously threaded through every Quaternion path is gone — quaternion order is now an internal property of `ebsdlib::QuaternionFType`. **The `getPositiveOrientation()` correction added in PR #1438 only existed in the old `FromQuaternion` adapter; it must be confirmed that the equivalent canonicalisation is now performed by `ebsdlib::QuaternionFType::toXxx()` itself.**
  - **Validity-check side:** the per-representation `XxxCheck<T>` structs were rewritten — `OrientationMatrixCheck` now calls `oaType.isValid()` (member method on `ebsdlib::OrientationMatrix<T>`) rather than the free function `OrientationTransformation::om_check()`. The non-OM/non-Euler check structs are now no-ops (they were no-ops before too).
- **V&V content:** **Potentially material.** Although the underlying math should remain Rowenhorst et al. (2015), this is exactly the class of change where small precision drift, sign convention drift, or a regression in the Quaternion canonicalisation could slip in. Deviation candidate.
- **Test impact:** the existing `Valid filter execution` test (96 input × output combinations checked against hard-coded `k_InitValues`) survives PR #1472 with **no change to the expected values** — strong evidence that round-trip results are bit-equivalent on the chosen test point. But the test uses *one* input tuple at a single, deliberately-non-degenerate orientation; near-pole / near-singularity inputs are not tested.

### PR #1535 — *"ENH: Remove redundant preflight checks"* — merged 2026-02-18

- **Files in this filter:** filter (.cpp), `-21` (pure removal)
- **Change nature:** Removed preflight checks that were already done by the parameter validators (per project CLAUDE.md guideline). No behavior change.
- **V&V content:** None.

### PR #1571 — *"DOC: Add standardized ChoicesParameter descriptions to filter docs"* — merged 2026-03-30

- **Files in this filter:** docs (.md), `+26`
- **Change nature:** Documentation hygiene — added the two `### Input Orientation Type` / `### Output Orientation Type` subsections enumerating all 8 representation choices with brief descriptions.
- **V&V content:** Doc currency improvement. Not algorithmic.

### PR #1588 — *"ENH: SIMPL Backwards Compatibility Test Redesign"* — merged 2026-04-22

- **Files in this filter:** test (.cpp) `+47`, plus two new fixture files
  - `test/simpl_conversion/6_4/ConvertOrientationsFilter.json` (~20 lines)
  - `test/simpl_conversion/6_5/ConvertOrientationsFilter.json` (~21 lines)
- **Change nature:** **Test addition.** Added a per-filter SIMPL→SIMPLNX backwards-compatibility test that exercises both SIMPL 6.4 (`Filter_Name` fallback) and 6.5 (UUID-mapped) pipeline conversion paths via `DYNAMIC_SECTION`. Test name: `OrientationAnalysis::ConvertOrientationsFilter: SIMPL Backwards Compatibility`.
- **V&V content:** **Pipeline-conversion correctness only.** The test verifies that opening a legacy SIMPL pipeline in DREAM3DNX produces a `ConvertOrientationsFilter` instance with the right parameter values (input/output type choices, input array path, output array name). It does **not** verify that the filter's *output* matches legacy.

### Pruned PRs (touched the file but not behaviorally relevant to this filter)

| PR | Subject | Why pruned |
|---|---|---|
| #1301 | Add missing algorithm classes to some filters | Cleanup/style follow-up to PR #1468 (this filter already had its algorithm class; the touch was minor) |
| #1457 | Clean up 'static inline' from filter headers | Style |
| #1491 | Use more distinct syntax for clang-format toggle | Style |
| #1524 | Filter tag full-name consistency | Test cosmetic |
| #1543 | Update pipeline references in doc | Doc cosmetic |
| #1547 | Fix filter documentation and code defaultTags() bugs | Doc + tag cleanup |

## Test coverage detected

`ConvertOrientationsTest.cpp` contains 4 `TEST_CASE`s:

1. `OrientationAnalysis::ConvertOrientations: Invalid preflight` — exercises three preflight failure paths: missing input array, out-of-range input type index, out-of-range output type index.
2. `OrientationAnalysis::ConvertOrientations: Valid filter execution` — **the workhorse test.** Iterates the full 7×7 cross-product of `{Euler, OrientationMatrix, Quaternion, AxisAngle, Rodrigues, Homochoric, Cubochoric}` (skipping equal-pairs), seeded from a single hard-coded golden orientation `k_InitValues` (identical Euler ↔ OM ↔ Qu ↔ Ax ↔ Ro ↔ Ho ↔ Cu values pre-computed externally). Compares output to expected with absolute tolerance `< 0.0001`. **Stereographic is excluded** from this cross-product (loop runs 0..6, not 0..7).
3. `OrientationAnalysis::ConvertOrientations: Equal Representations` — `GENERATE_COPY` over all `OCType::GetOrientationTypes()` enum values; verifies preflight rejects same-input-as-output with `k_MatchingTypesError`.
4. `OrientationAnalysis::ConvertOrientationsFilter: SIMPL Backwards Compatibility` — SIMPL 6.4 + 6.5 conversion paths via `DYNAMIC_SECTION` *(added by PR #1588)*.

**Coverage gaps observed:**
- Only **one** input orientation is exercised per representation (a generic non-degenerate one). No coverage of identity/zero rotation, gimbal-lock pole (Φ ≈ 0 or π), 180° flips, or near-singular Rodrigues vectors.
- Stereographic projection (`st`) is not tested in the cross-product loop despite being a fully-supported representation.
- No round-trip test (`X → Y → X` should equal the original to within machine epsilon) — the test uses externally pre-computed golden values for each representation rather than algorithmically chained conversions.
- No multi-tuple test — `tupleShape = {1}` for the cross-product test.

## Exemplar archive

- **Archive name(s) referenced in `src/Plugins/OrientationAnalysis/test/CMakeLists.txt`:** `convert_orientations_to_vertex_geometry.tar.gz` (this is the data for the *sibling* filter `ConvertOrientationsToVertexGeometry`, not for `ConvertOrientationsFilter`).
- **Provenance for `ConvertOrientationsFilter`:** **None — there is no exemplar `.dream3d` archive for this filter.** All numeric verification is performed against hard-coded `k_InitValues` literals embedded in `ConvertOrientationsTest.cpp`.
- **Action required:** Decide whether to add an exemplar archive (e.g., a small representative input orientation array with all 8 representations pre-computed by an independent tool such as MTEX or a hand-derived Python script per the Class-3 paper) or to formalize the existing in-source `k_InitValues` as the oracle of record by documenting their provenance (source tool, derivation, paper section).

## Oracle classification (tentative)

- **Recommended class:** **Class 3 (Paper-based)** — *Rowenhorst, Rollett, Rohrer, Groeber, Jackson, Konijnenberg, De Graef (2015), "Consistent representations of and conversions between 3D rotations", Modelling Simul. Mater. Sci. Eng. 23 083501.*
- **Companion classes:**
  - **Class 4 (Invariant-based) — strongly recommended addition.** Round-trip identity (`X → Y → X` == X within machine epsilon for every supported pair); norm preservation (Quaternion / Stereographic / Homochoric output norm == 1); determinant preservation (Orientation Matrix det == +1); axis-vector unit-norm (Axis-Angle, Rodrigues axis component).
  - **Class 1 (Analytical) — easy to add.** Hand-verifiable special cases: Euler `(0,0,0)` → identity quaternion `(0,0,0,1)`; Euler `(0,0,0)` → identity orientation matrix `[[1,0,0],[0,1,0],[0,0,1]]`; Euler `(π,0,0)` and `(0,π,0)` → corresponding 180° axis-angle pairs.
- **Rationale:** All eight representations and every pairwise conversion in this filter are direct calls into EbsdLib's `OrientationTransformation` namespace (now type-erased through `ebsdlib::XxxFType::toYyy()` member methods after PR #1472), which directly implements the algorithms in Rowenhorst et al. 2015. That paper provides canonical test data in its appendices and is THE definitive reference. The current in-source `k_InitValues` literals appear to be one such pre-computed canonical orientation set, but the test does not cite the source.
- **Action required:** Developer to confirm Rowenhorst 2015 is the actual implementation source for EbsdLib (high confidence — Mike Jackson is a co-author of that paper). Defend or replace the Class-3 + Class-4 + Class-1 stack.

## V&V status so far

| Item | Status | Notes |
|---|---|---|
| Algorithm review (`review-algorithm` skill) | Not visible from PR history | No PR explicitly performs the line-by-line review of the algorithm class. |
| Code path coverage (algorithmic) | **Partial** | 7×7 = 42 of the 56 representation pairs covered (excludes Stereographic from the cross-product). Single tuple, single seed orientation, no degenerate / near-singular inputs. |
| Code path coverage (SIMPL conversion) | Good | PR #1588 added SIMPL 6.4 + 6.5 conversion test. |
| Exemplar data in Data_Archive | **No** | No `.tar.gz` referenced for this filter; oracle is in-source literals. |
| Exemplar provenance documented | **No** | The `k_InitValues` literals have no comment citing their source. |
| Oracle class recorded | **No** | This document is the first to propose one. |
| Toy data / independent expected output (Step 0 c) | **In-source only** | `k_InitValues` is effectively the toy data, but no script / hand-derivation is on file. |
| Legacy comparison report (Step 0 e) | **No** | `compare-legacy-dream3d` has not been run. PRs #1438, #1468, and especially #1472 each warrant one. |
| Deviation entries (`ConvertOrientations-D<N>`) | **None** | Not yet written. PR #1438's quaternion-canonicalisation fix and PR #1472's library-backbone swap are both Deviation candidates. |
| Documentation currency | Probably current | Updated by PRs #1543, #1547, #1571. Needs accuracy audit per `review-filter-docs` — particularly the *Quaternion: Scalar part must be positive and have unit norm* claim, which is enforced internally by `getPositiveOrientation()`. |
| Verification archive (OneDrive) | No | Not yet created. |

## Gaps to close (to meet Step 0 / Legacy Comparison policy)

1. **Confirm the oracle.** Class 3 (Rowenhorst 2015) + Class 4 (round-trip / invariant) + Class 1 (analytical specials) is the recommended stack. Confirm the paper reference is in the EbsdLib source or in the legacy DREAM3D doc.
2. **Add a round-trip invariant test.** The current test compares each pair against a pre-computed golden tuple but does not assert `X → Y → X == X`. An algorithmic round-trip test gives O(N²) coverage instead of O(N) for the same effort.
3. **Cover the missing Stereographic conversions.** The `Valid filter execution` cross-product loops to index 6, missing all 14 conversions involving Stereographic (7 to + 7 from).
4. **Add degenerate-input tests.** Identity rotation, Φ ≈ 0 (gimbal-lock), Φ = π, 180° axis-angle, near-zero Rodrigues, near-pole Stereographic. PR #1438's quaternion fix specifically targets a non-canonical-quaternion regression that the existing test would not have caught.
5. **Document the provenance of `k_InitValues`.** Add an in-source comment or inline reference (paper appendix? MTEX dump? independent Python derivation?) so the values become a defensible Class-3 oracle.
6. **Run the legacy comparison.** Use `compare-legacy-dream3d` to diff SIMPLNX vs. DREAM3D 6.5.172 on a small EBSD scan with all 56 conversion paths exercised. Three independent reasons motivate this run: (a) the PR #1438 quaternion fix may not be present in legacy; (b) PR #1472 swapped the EbsdLib API surface — verify no precision drift; (c) confirm the Stereographic implementations agree (legacy may not have implemented Stereographic the same way EbsdLib 2.0.0 does, or at all).
7. **Produce the Algorithm Relationship one-liner.** Tentative: *"Port — 56-path representation conversion table preserved from SIMPL `ConvertOrientations`. Internal backbone re-platformed twice during the audit window: PR #1468 extracted to algorithm class; PR #1472 swapped from `OrientationTransformation::xx2yy<>` template free functions to type-erased `ebsdlib::XxxFType::toYyy()` member methods. PR #1438 added quaternion canonicalisation."*
8. **Archive everything** per `archive-filter-verification` for the OneDrive folder.

## Recommended Deviation entries (proposed, pending legacy comparison)

> **Deviation ID:** `ConvertOrientations-D1`
> **Filter UUID:** `501e54e6-a66f-4eeb-ae37-00e649c00d4b`
> **Symptom:** For Quaternion → {Euler, OrientationMatrix, AxisAngle, Rodrigues, Homochoric, Cubochoric, Stereographic} conversions, SIMPLNX produces a sign-canonical (scalar-positive) intermediate quaternion before invoking the conversion math; SIMPL 6.5.172 may not. Result: for any input quaternion with a negative scalar component, the legacy filter may emit an output with a flipped sign convention (still a symmetrically equivalent rotation, but not bit-equal).
> **Root cause:** Bug fix in SIMPLNX — PR #1438 added `inputQuat = QuaterionType(...).getPositiveOrientation();` before calling the `From-Quaternion` transform. The legacy code path passed the raw input quaternion verbatim.
> **Affected users:** Anyone whose downstream analysis is sign-sensitive on the quaternion-derived axis or w component, or who runs strict bit-for-bit cross-version comparison.
> **Recommendation:** Trust SIMPLNX. Both representations are mathematically equivalent rotations, but the canonical sign-positive form is the convention recommended by Rowenhorst et al. 2015.
> **Status:** Proposed — pending verification that 6.5.172 actually exhibits the negative-scalar-pass-through.

> **Deviation ID:** `ConvertOrientations-D2`
> **Filter UUID:** `501e54e6-a66f-4eeb-ae37-00e649c00d4b`
> **Symptom:** Possible precision drift in any of the 56 conversion paths after SIMPLNX adopted EbsdLib 2.0.0 (PR #1472), which replaced `OrientationTransformation::eu2om<>` (free template functions) with `ebsdlib::EulerFType::toOrientationMatrix()` (type-erased member methods).
> **Root cause:** The math in EbsdLib 2.0.0 should be identical to the prior `OrientationTransformation` namespace (both implement Rowenhorst 2015), but the implementation surface was wholly rewritten. Any accumulator-order, fma-vs-mul-add, or intermediate-storage precision change in EbsdLib 2.0.0 will surface here.
> **Affected users:** Anyone running strict bit-equal comparison across the EbsdLib 1.x → 2.0.0 transition (i.e., any pre-PR-#1472 SIMPLNX build vs. current).
> **Recommendation:** Run `compare-legacy-dream3d` and compare SIMPLNX(current) vs. SIMPLNX(pre-#1472) AND vs. DREAM3D 6.5.172 on the same input. Likely outcome: agreement to within `1e-6` for `float32`. If drift exceeds the unit-test tolerance (`1e-4`), investigate.
> **Status:** Proposed — pending the comparison run. The existing unit test passes against the same `k_InitValues` post-#1472, which is suggestive evidence of no observable drift on at least the one tested orientation.

> **Deviation ID:** `ConvertOrientations-D3` *(speculative, pending Stereographic-aware comparison)*
> **Filter UUID:** `501e54e6-a66f-4eeb-ae37-00e649c00d4b`
> **Symptom:** Conversions involving the Stereographic projection representation may differ from legacy SIMPL, or may not be implemented in legacy at all. The SIMPLNX unit test excludes Stereographic from its 7×7 cross-product loop.
> **Root cause:** Unknown — needs investigation. Stereographic is index 7 in the SIMPLNX representation enum; may be a SIMPLNX/EbsdLib 2.0.0 addition.
> **Affected users:** Anyone relying on Stereographic conversions.
> **Recommendation:** Confirm whether legacy SIMPL ever supported Stereographic. If yes, run legacy comparison on the 14 Stereographic paths. If no, document Stereographic as a SIMPLNX-only feature in the user-facing doc.
> **Status:** Proposed — pending investigation of legacy support.
