# Retroactive V&V: CAxisSegmentFeaturesFilter

*Report status:* **DRAFT**. Generated from git-history and source-tree inspection. Developer must confirm or correct the Oracle class, Algorithm Relationship, and the V&V status entries.

## Metadata

| Field | Value |
|---|---|
| SIMPLNX UUID | `9fe07e17-aef1-4bf1-834c-d3a73dafc27d` |
| SIMPLNX ClassName | `CAxisSegmentFeaturesFilter` |
| SIMPLNX Human Name | Segment Features (C-Axis Misalignment) |
| SIMPL UUID | `bff6be19-1219-5876-8838-1574ad29d965` |
| SIMPL ClassName | *(TBD — confirm in legacy SIMPL repo)* |
| SIMPL Human Name | *(TBD — confirm in legacy SIMPL repo)* |
| Plugin | OrientationAnalysis |

### Source files scanned

- `src/Plugins/OrientationAnalysis/src/OrientationAnalysis/Filters/CAxisSegmentFeaturesFilter.{hpp,cpp}`
- `src/Plugins/OrientationAnalysis/src/OrientationAnalysis/Filters/Algorithms/CAxisSegmentFeatures.{hpp,cpp}`
- `src/Plugins/OrientationAnalysis/test/CAxisSegmentFeaturesTest.cpp`
- `src/Plugins/OrientationAnalysis/test/simpl_conversion/6_5/CAxisSegmentFeaturesFilter.json`
- `src/Plugins/OrientationAnalysis/test/simpl_conversion/6_4/CAxisSegmentFeaturesFilter.json`
- `src/Plugins/OrientationAnalysis/docs/CAxisSegmentFeaturesFilter.md`

## Algorithm Relationship

- **Tentative classification:** **Port** — the SIMPLNX filter appears to be a direct translation of the legacy SIMPL implementation; the UUID was preserved.
- **Evidence:** No rewrite signal in PR history. Standard feature-level segmentation with per-voxel C-axis misorientation tolerance.
- **Action required:** Confirm by reading the corresponding SIMPL filter source and running `compare-legacy-dream3d` step (e) against a shared toy dataset.

## PRs inspected (since 2025-10-01)

> Pruned: pure-style/repo-wide refactor PRs (#1457 static-inline, #1472 EbsdLib bump, #1501 Vec3 unification, #1535 preflight cleanup, #1582 cancel-check sweep) and pure test-infrastructure PRs (#1524 tag fix, #1538 zlib extraction) are listed at the bottom of this section but not detailed individually — they did not change behavior of this filter.

### PR #1373 — *"ENH: Add option to use 26 neighbor kernel for SegmentFeatures class"* — merged 2025-10-21

- **Files in this filter:** filter (.hpp, .cpp), algorithm (.hpp, .cpp), docs (.md)
- **Diff size:** 5 files, +28 / -9 lines
- **Change nature:** **Feature addition.** Added a parameter to choose the 6-face neighbor kernel or a 26-connected (full-cube) neighbor kernel. Touched the filter parameter list, the algorithm's neighbor iteration, and the user-facing doc.
- **V&V content:** PR description claims doc, unit test, and pipelines all updated for the new option. Test file confirms two new code paths now exist (`:All` and `:MaskAll`).

### PR #1438 — *"ENH: Microtexture related filter cleanup"* — merged 2025-10-25

- **Files in this filter:** algorithm (.cpp), filter (.cpp)
- **Diff size:** 2 files, +9 / -6 lines
- **Change nature:** Small cleanup as part of a larger microtexture-pipeline correctness pass.
- **V&V content:** Flagged by the policy maintainer as early MTR-V&V-related work (multiple small fixes that together made microtexture pipelines run correctly). The diff does not show new tests or new exemplar data, but the corrections it carried are the kind of thing that ought to be captured as Deviation entries vs. legacy if the legacy versions are still wrong.

### PR #1466 — *"BUG: Fixes issue when segmenting features with a +1 feature count"* — merged 2025-11-14

- **Files in this filter:** test (.cpp) — **+278 lines on the test file**
- **Diff size:** Test file substantially expanded
- **Change nature:** **Material bug fix + verification.** PR description: *"The previous algorithm would always be 1 too high in the number of tuples in the feature attribute matrix versus what was actually found."* Test file expansion of 278 lines suggests the fix was paired with new test coverage proving the correct count.
- **V&V content:** **High** — this is the closest thing to per-filter V&V work in the entire history. The bug it fixed is *exactly* a candidate for a Deviation entry vs. SIMPL 6.5.172 if the legacy version still has the off-by-one.
- **Note:** This PR is **NOT in the user's pruned legend** but it appears in the full git log. Worth promoting to the table. (Side note: the user listed #1490 in the pruned table for this filter — that's a style PR; #1466 is the substantive one.)

### PR #1490 — *"STY: Fix warnings about unintended slicing of object"* — merged 2026-01-09

- **Files in this filter:** algorithm (.cpp) only
- **Diff size:** +9 / -8 lines
- **Change nature:** Pure style / compiler-warning fix (`ResultVoid` / `Result<void>` slicing). No behavioral change intended.
- **V&V content:** None.

### PR #1571 — *"DOC: Add standardized ChoicesParameter descriptions to filter docs"* — merged 2026-03-30

- **Files in this filter:** docs (.md), +7 lines
- **Change nature:** Documentation hygiene — added a standardized `### Parameter Label` subsection for the ChoicesParameter (likely the 6-vs-26 neighbor toggle).
- **V&V content:** Doc currency improvement. Not algorithmic.

### PR #1588 — *"ENH: SIMPL Backwards Compatibility Test Redesign"* — merged 2026-04-22

- **Files in this filter:** test (.cpp) +52 lines, plus two new fixture files
  - `test/simpl_conversion/6_4/CAxisSegmentFeaturesFilter.json` (1146 bytes)
  - `test/simpl_conversion/6_5/CAxisSegmentFeaturesFilter.json` (1203 bytes)
- **Change nature:** **Test addition.** Added a per-filter SIMPL→SIMPLNX backwards-compatibility test that exercises both SIMPL 6.4 (Filter_Name fallback) and 6.5 (UUID-mapped) pipeline conversion paths via `DYNAMIC_SECTION`. Test name: `"OrientationAnalysis::CAxisSegmentFeaturesFilter: SIMPL Backwards Compatibility"`.
- **V&V content:** **Pipeline-conversion correctness only** — the test verifies that opening a legacy SIMPL pipeline in DREAM3DNX produces a filter instance with the right parameter values. It does **not** verify that the filter's *output* matches legacy. That latter step is still missing.

### Pruned PRs (touched the file but not behaviorally relevant to this filter)

| PR | Subject | Why pruned |
|---|---|---|
| #1439 | Multi-Dimensional Tuple Support for StringArray and NeighborList | API change, no per-filter behavior change |
| #1457 | Clean up 'static inline' from filter headers | Style |
| #1472 | Update to EbsdLib 2.0.0 API | Library bump, no algorithmic change |
| #1501 | Combine Matrix3x1, Point3D, Vec3 into a Vec3<T> | Refactor |
| #1524 | Fixed filter tags to consistently use the full filter name | Test cosmetic |
| #1535 | Remove redundant preflight checks | Cleanup |
| #1538 | Replace cmake subprocess tar.gz extraction with zlib | Test infrastructure |
| #1582 | Add missing cancel checks to lots of filters | Sweep |

## Test coverage detected

`CAxisSegmentFeaturesTest.cpp` contains 5 `TEST_CASE`s:

1. `CAxisSegmentFeatures:Face` — 6-face neighbor kernel, no mask
2. `CAxisSegmentFeatures:All` — 26-neighbor kernel, no mask *(added by PR #1373)*
3. `CAxisSegmentFeatures:MaskFace` — 6-face kernel, with mask
4. `CAxisSegmentFeatures:MaskAll` — 26-neighbor kernel, with mask *(added by PR #1373)*
5. `CAxisSegmentFeaturesFilter: SIMPL Backwards Compatibility` — SIMPL 6.4 + 6.5 conversion paths via DYNAMIC_SECTION *(added by PR #1588)*

Tests 1–4 exercise the algorithmic 2×2 cross-product (kernel × mask). Test 5 is conversion-only.

## Exemplar archive

- **Archive name:** `caxis_data.tar.gz`
- **SHA512:** `56468d3f248661c0d739d9acd5a1554abc700bf136586f698a313804536916850b731603d42a0b93aae47faf2f7ee49d4181b1c3e833f054df6f5c70b5e041dc`
- **Referenced in:** `src/Plugins/OrientationAnalysis/test/CMakeLists.txt`
- **Provenance:** *(TBD — engineer must inspect the archive to determine how the exemplar was generated and whether an Oracle Provenance block exists in any ReadMe inside it.)*
- **Action required:** Download the archive locally and inspect for: an inner `ReadMe.md`, the input `.dream3d` files used to generate the exemplars, the pipeline files that produced the exemplars, and any provenance notes. Promote this content into the verification archive ReadMe per Step 0's Oracle Provenance policy.

## Oracle classification (tentative)

- **Recommended class:** **4 (Invariant-based)**, with optional **3 (Paper-based)** if a named reference for C-axis segmentation is available.
- **Rationale:** Segmentation has natural invariants — FeatureIds start at 1, are contiguous, `count(features) ≤ count(voxels)`, and every voxel pair grouped into the same feature has C-axis misorientation below the user-selected tolerance. Note: PR #1466 was specifically about the `count(features)` invariant being off-by-one. These can be encoded as test assertions. If a paper exists describing the algorithm, Class 3 is stronger.
- **Action required:** Developer to confirm whether a paper reference exists in the algorithm header or legacy DREAM3D docs. Developer to defend or replace the Class-4 recommendation.

## V&V status so far

| Item | Status | Notes |
|---|---|---|
| Algorithm review (`review-algorithm` skill) | Not visible from PR history | No PR explicitly performs the line-by-line review. |
| Code path coverage (algorithmic) | Good | 4 tests cover the kernel × mask cross-product. |
| Code path coverage (SIMPL conversion) | Good | PR #1588 added SIMPL 6.4 + 6.5 conversion test. |
| Exemplar data in Data_Archive | **Yes** | `caxis_data.tar.gz` referenced in test/CMakeLists.txt. |
| Exemplar provenance documented | Unknown | TBD by inspecting archive contents. |
| Oracle class recorded | **No** | This document is the first to propose one. |
| Toy data / independent expected output (Step 0 c) | No | No script or hand-derivation on file. |
| Legacy comparison report (Step 0 e) | No | `compare-legacy-dream3d` has not been run. |
| Deviation entries (`CAxisSegmentFeatures-D<N>`) | None | Not yet written. PR #1466's off-by-one bug fix is a strong Deviation candidate if SIMPL 6.5.172 still has the bug. |
| Documentation currency | Probably current | Updated by PRs #1373 (26-neighbor option) and #1571 (ChoicesParameter description). Needs accuracy audit per `review-filter-docs`. |
| Verification archive (OneDrive) | No | Not yet created. |

## Gaps to close (to meet Step 0 / Legacy Comparison policy)

1. **Confirm the oracle.** Class 4 (invariant-based) is the recommended starting point; check for a paper reference and upgrade to Class 3 if one exists.
2. **Defend or rewrite the existing tests as oracle-class assertions.** The current 4 algorithmic tests likely already check feature counts and IDs implicitly via exemplar comparison; promote those checks into explicit Class-4 invariant assertions (`REQUIRE(feature_ids.min() == 1)`, etc.) so the test code becomes the oracle of record.
3. **Inspect `caxis_data.tar.gz` and document provenance.** Determine how the exemplars were generated, whether a SIMPL pipeline produced them, and what the input data was. Write an Oracle Provenance block for the archive ReadMe.
4. **Run the legacy comparison.** Use `compare-legacy-dream3d` to diff SIMPLNX vs. DREAM3D 6.5.172 on the same toy data. The expected outcome includes at minimum one Deviation entry: `CAxisSegmentFeatures-D1` for the off-by-one feature count bug fixed in PR #1466 (assuming legacy still has it).
5. **Produce the Algorithm Relationship one-liner.** Tentative: *"Port — direct translation of the SIMPL `<TBD>` filter; one bug fix applied in PR #1466 (feature-count off-by-one)."*
6. **Archive everything** per `archive-filter-verification` for the OneDrive folder.

## Recommended Deviation entries (proposed, pending legacy comparison)

> **Deviation ID:** `CAxisSegmentFeatures-D1`
> **Filter UUID:** `9fe07e17-aef1-4bf1-834c-d3a73dafc27d`
> **Symptom:** SIMPLNX produces N features on a given input; SIMPL 6.5.172 produces N+1 (one phantom feature with empty attribute-matrix tuple).
> **Root cause:** Bug in 6.5.172 — the legacy algorithm allocated one extra tuple in the feature attribute matrix that did not correspond to a found feature. Fixed in SIMPLNX by PR #1466.
> **Affected users:** Anyone whose downstream analysis iterates feature attribute matrices and assumes every row corresponds to a real feature; anyone comparing feature counts across versions.
> **Recommendation:** Trust SIMPLNX. Legacy was wrong. A quick patch to 6.5.172's algorithm to remove the +1 allocation is the minimal legacy fix; engineer to evaluate whether legacy patch is needed for users who must reproduce exact legacy counts.
> **Status:** Proposed — pending verification that 6.5.172 actually exhibits the bug (run the comparison).
