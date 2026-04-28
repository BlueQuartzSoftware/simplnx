# Retroactive V&V: WritePoleFigureFilter

*Report status:* **DRAFT**. Generated from git-history and source-tree inspection. Developer must confirm or correct the Oracle class, Algorithm Relationship, and the V&V status entries.

## Metadata

| Field | Value |
|---|---|
| SIMPLNX UUID | `00cbb97e-a5c2-43e6-9a35-17a0f9ce26ed` |
| SIMPLNX ClassName | `WritePoleFigureFilter` |
| SIMPLNX Human Name | Generate and Write Pole Figure Images (Group: IO / Output) |
| SIMPL UUID (legacy) | `a10bb78e-fcff-553d-97d6-830a43c85385` *(noted in header comment)* |
| SIMPL ClassName | *(TBD — confirm in legacy SIMPL repo, almost certainly `WritePoleFigure`)* |
| SIMPL Human Name | *(TBD — confirm in legacy SIMPL repo)* |
| Plugin | OrientationAnalysis |

### Source files scanned

- `src/Plugins/OrientationAnalysis/src/OrientationAnalysis/Filters/WritePoleFigureFilter.{hpp,cpp}` (371 lines .cpp)
- `src/Plugins/OrientationAnalysis/src/OrientationAnalysis/Filters/Algorithms/WritePoleFigure.{hpp,cpp}` (796 lines .cpp post-#1587)
- `src/Plugins/OrientationAnalysis/test/WritePoleFigureTest.cpp`
- `src/Plugins/OrientationAnalysis/test/simpl_conversion/6_5/WritePoleFigureFilter.json`
- `src/Plugins/OrientationAnalysis/test/simpl_conversion/6_4/WritePoleFigureFilter.json`
- `src/Plugins/OrientationAnalysis/docs/WritePoleFigureFilter.md`
- `src/Plugins/OrientationAnalysis/test/CMakeLists.txt` (download_test_data for `PoleFigure_Exemplars_v5.tar.gz`)

## Algorithm Relationship

- **Tentative classification:** **Port → Rewrite (over time).** Originally a direct translation of the legacy SIMPL `WritePoleFigure` filter (legacy UUID preserved in a comment). Over the audit period, the implementation has been progressively rewritten so that the heavy mathematical and rendering code is no longer in this repo at all — first the EbsdLib 2.0.0 API migration (#1472), then the EbsdLib 2.4.0 layout-code migration (#1587). After #1587 the algorithm in this repo is a **driver** that builds an `ebsdlib::CompositePoleFigureConfiguration_t`, calls `ebsdlib::PoleFigureCompositor::generateCompositeImage()`, and stuffs the resulting RGBA buffer into a SIMPLNX `ImageGeometry` and/or a TIFF on disk. The actual stereographic projection, modified Lambert square interpolation, symmetry handling, color mapping, panel layout, scale-bar drawing, and font rendering all live in EbsdLib now.
- **Evidence:**
  - PR #1587 stripped ~391 lines from `WritePoleFigure.cpp` (`drawInformationBlock`, `drawScalarBar`, `convertColorOrder`, `canvas_ity` font/text rendering, `LatoBold`/`LatoRegular`/`FiraSansRegular` font tables, RgbColor color-table generation) and replaced them with a single `ebsdlib::PoleFigureCompositor` call.
  - The current `operator()` in `WritePoleFigure.cpp` lines 511–795 reads SIMPLNX arrays, slices Euler angles by phase, builds a `PoleFigureConfiguration_t`, and delegates layout/rendering to EbsdLib.
- **Action required:** Confirm by reading the corresponding SIMPL filter source (legacy UUID `a10bb78e-fcff-553d-97d6-830a43c85385`) and verifying that any "Port" claim is bounded — the modified Lambert + canvas_ity rendering path that lived in this repo through #1587 is now gone. **The thing to verify against legacy is whether the EbsdLib compositor produces equivalent images to the legacy SIMPL DREAM3D 6.5.172 implementation, because the production code path has changed substantially.**

## PRs inspected (since 2025-10-01)

> Pruned: pure-style/repo-wide refactor PRs (#1457 static-inline, #1439 multi-D tuple support API, #1491 clang-format toggle syntax, #1538 zlib extraction in tests, #1476 backwards-compat infrastructure) are listed at the bottom of this section but not detailed individually. **#1472 is promoted** (per the refined pruning rules: EbsdLib bumps must be inspected for filters that delegate math to EbsdLib — and this filter delegates everything to EbsdLib). **#1438 is promoted** per standard rules (always inspect). **#1587 is the most material change in the audit period** and dwarfs everything else.

### PR #1438 — *"ENH: Microtexture related filter cleanup"* — merged 2025-10-25

- **Files in this filter:** algorithm (.cpp) only
- **Diff size:** 1 file, +14 / −14 lines
- **Change nature:** Pure include-syntax change for this filter — `#include "EbsdLib/..."` → `#include <EbsdLib/...>` to use proper external-library include style. The umbrella PR contains many algorithmic bug fixes for *other* microtexture filters (CAxisSegmentFeatures, ComputeAvgOrientations, etc.), but the WritePoleFigure portion was style-only.
- **V&V content:** None directly. No behavioral change to this filter.

### PR #1472 — *"ENH: Update to EbsdLib 2.0.0 API"* — merged 2025-11-24 (broad refactor, exception flagged because pole figure math is delegated to EbsdLib)

- **Files in this filter:** algorithm (.cpp) only
- **Diff size:** 1 file, +64 / −64 lines (1:1 substitution, no net code change)
- **Change nature:** API rename pass to track the EbsdLib 2.0.0 release — symbols moved into the `ebsdlib::` namespace, `LaueOps`/`PoleFigureConfiguration_t` types moved. Mechanical substitution; no behavioral change *expected* but the underlying library was bumped, which means any fixed-point regression in EbsdLib's pole-figure routines would silently change SIMPLNX output.
- **V&V content:** **Implicit.** The unit-test exemplar archive carries the SHA512 hash that pinned acceptable EbsdLib output at the time. If EbsdLib's pole-figure routines drifted between 1.x and 2.0, the exemplar comparison would have caught it (or the exemplar was regenerated with new output). Worth flagging for the legacy comparison pass — see Deviation candidates below.

### PR #1476 — *"BUG/ENH: Fix Backwards Pipeline Compatibility and Add Testing"* — merged 2025-12-09

- **Files in this filter:** *(no direct WritePoleFigure file changes were listed in the per-file log — appears in the file's git log because the PR touched a shared `BackwardsCompatibilityTest.cpp` that was later replaced by the per-filter test redesign in #1588)*
- **Change nature:** Repo-wide — added the SIMPL-conversion testing harness that PR #1588 then replaced.
- **V&V content:** None for this filter individually.

### PR #1566 — *"BUG: Fix parameter linking in WritePoleFigure Filter"* — merged 2026-03-23

- **Files in this filter:** filter (.cpp) only
- **Diff size:** 1 file, +5 / −4 lines
- **Change nature:** **Material UI/UX bug fix.** Two parameter-linking corrections plus one separator-name change:
  1. `params.linkParameters(k_SaveIntensityDataArrays, k_ImageGeometryPath_Key, true)` was wrong — it linked the intensity-arrays toggle to the *Image Geometry* output, not to the *Intensity Geometry* output. Replaced with `linkParameters(k_SaveIntensityDataArrays, k_IntensityGeometryPath, true)` and a new link to `k_NormalizeToMRD`.
  2. Section separator renamed from "Output File Parameters" to "Output Image File".
- **V&V content:** Affects only how the GUI grays out / shows fields; does not affect numerical results given a complete set of arguments. Would NOT show up in any unit test that supplies all arguments explicitly. **No new tests were added for this fix.**

### PR #1571 — *"DOC: Add standardized ChoicesParameter descriptions to filter docs"* — merged 2026-03-30

- **Files in this filter:** docs (.md) +14 / −1
- **Change nature:** Documentation hygiene — added a per-choice description block for the `Image Layout` ChoicesParameter (Horizontal / Vertical / Square) and a new `Pole Figure Type` block (Color Intensity / Discrete).
- **V&V content:** Doc currency improvement. Not algorithmic.

### PR #1587 — *"ENH: WritePoleFigure filter layout code moved to EbsdLib 2.4.0"* — merged 2026-04-15 ⭐ **most material PR in the audit period**

- **Files in this filter:** algorithm (.cpp) +124 / −267, algorithm (.hpp) −4, filter (.cpp) +1, test (.cpp) +220 / −0, test/CMakeLists.txt SHA bump, plus `conda/meta.yaml` + `vcpkg.json` EbsdLib version bump.
- **Diff size:** 4 plugin files changed totaling **+216 / −400** lines.
- **Change nature:** **Major delegation refactor.** All page-layout, scale-bar drawing, font handling, color-table generation, and `canvas_ity`-based 2D drawing was removed from this repo and replaced with a single `ebsdlib::PoleFigureCompositor::generateCompositeImage()` call against EbsdLib 2.4.0. The exemplar test archive was bumped from `PoleFigure_Exemplars_v4.tar.gz` to `PoleFigure_Exemplars_v5.tar.gz` with a new SHA512, indicating that **the expected pixel output changed**.
- **V&V content:** **High and ambivalent.**
  - On the positive side: the test file was expanded by 220 lines and the four algorithmic test cases now compare more arrays (color RGB, plus 3 MRD intensity arrays per case at `<001>`, `<011>`, `<111>`).
  - On the negative side: the exemplar archive was *regenerated* to match the new output, which means that whatever drift exists between v4 (canvas_ity / SIMPLNX-internal compositor) and v5 (EbsdLib 2.4.0 compositor) is now baked into the oracle. **There is no record on file of a side-by-side comparison between the v4 and v5 exemplars.** This is the single most important Deviation candidate for legacy comparison: pre-#1587 SIMPLNX output, post-#1587 SIMPLNX output, and SIMPL 6.5.172 output should all be compared.

### PR #1588 — *"ENH: SIMPL Backwards Compatibility Test Redesign"* — merged 2026-04-22

- **Files in this filter:** test (.cpp) +60, plus two new fixture files
  - `test/simpl_conversion/6_4/WritePoleFigureFilter.json` (1 KB)
  - `test/simpl_conversion/6_5/WritePoleFigureFilter.json` (1 KB)
- **Change nature:** **Test addition.** Added a per-filter SIMPL→SIMPLNX backwards-compatibility test exercising both SIMPL 6.4 (`Filter_Name` fallback) and 6.5 (UUID-mapped) conversion paths via `DYNAMIC_SECTION`. Test name: `"OrientationAnalysis::WritePoleFigureFilter: SIMPL Backwards Compatibility"`.
- **V&V content:** **Pipeline-conversion correctness only** — confirms that opening a legacy SIMPL pipeline produces a filter instance with the correct parameter values (Title="TestName", LambertSize=5, NumColors=5, ImageSize=5, all DataPaths = `DataContainer/CellData/TestArray`, etc.). It does **not** verify that the filter's *output* matches legacy. The 6.4 and 6.5 fixtures both check the same key set; the 6.5 fixture additionally verifies `k_ImageFormat_Key == 0` (SIMPL 6.5 carried that field; SIMPL 6.4 did not).

### Pruned PRs (touched the file but not behaviorally relevant to this filter)

| PR | Subject | Why pruned |
|---|---|---|
| #1439 | Multi-Dimensional Tuple Support for StringArray and NeighborList | API change, no per-filter behavior change |
| #1457 | Clean up 'static inline' from filter headers | Style only |
| #1491 | More distinct syntax to toggle clang-formatting for specific sections | Style only (touched 4 lines in WritePoleFigure.cpp, all comment-style) |
| #1476 | Fix Backwards Pipeline Compatibility and Add Testing | Test infrastructure that #1588 replaced |
| #1538 | Replace cmake subprocess tar.gz extraction with zlib in unit tests | Test infrastructure |

## Test coverage detected

`WritePoleFigureTest.cpp` contains **5** `TEST_CASE`s:

1. `OrientationAnalysis::WritePoleFigureFilter-Discrete` — Discrete pole figure (algorithm choice = 1), no mask, Horizontal layout (=0). Compares 3 RGB components of the calculated image vs exemplar plus the 3 MRD intensity arrays (`<001>`, `<011>`, `<111>`) exactly.
2. `OrientationAnalysis::WritePoleFigureFilter-Discrete-Masked` — Discrete + mask enabled. Same comparisons.
3. `OrientationAnalysis::WritePoleFigureFilter-Color` — Color intensity (algorithm choice = 0, modified Lambert), no mask. RGB components compared exactly; MRD intensity arrays compared with `CompareFloatArraysWithNans` at tolerance 1e-4.
4. `OrientationAnalysis::WritePoleFigureFilter-Color-Masked` — Color + mask. Same comparisons.
5. `OrientationAnalysis::WritePoleFigureFilter: SIMPL Backwards Compatibility` — SIMPL 6.4 + 6.5 conversion paths via `DYNAMIC_SECTION` *(added by PR #1588)*

Tests 1–4 exercise the algorithmic 2×2 cross-product (rendering algorithm × mask). All four use cubic crystal structure (input file `fw-ar-IF1-aptr12-corr`) and only validate **Phase 1**. Test 5 is conversion-only.

**Coverage gaps observed:**
- No explicit test for `ImageLayout = Vertical (1)` or `Square (2)` — only Horizontal is exercised.
- No test for non-cubic Laue groups (hexagonal, trigonal, tetragonal, monoclinic, orthorhombic, triclinic) — the EbsdLib `LaueOps` dispatch is exercised only on cubic input data.
- No test for the `WriteImageToDisk = false` only-DataStructure path vs `SaveAsImageGeometry = false` only-disk path; both are always set true.
- No test for `NormalizeToMRD = false`.
- No test for image format choices other than the default; the unit tests do not even set `k_ImageFormat_Key`.
- No multi-phase test (single phase only).
- No test asserting that output TIFF on disk matches a reference image byte-for-byte (or hash).

## Exemplar archive

- **Archive name:** `PoleFigure_Exemplars_v5.tar.gz` *(superseded `_v4.tar.gz` in PR #1587)*
- **SHA512:** `a092b02a734ac706143c1c9ded0206f141b1f8a1359621e0bbfdbc8b4188ccc075151405d1c931292e9d9952e428877f14196e751f82a3c1cdbf734366ea1293`
- **Referenced in:** `src/Plugins/OrientationAnalysis/test/CMakeLists.txt` line 151
- **Provenance:** *(TBD — engineer must inspect the archive to determine how the exemplar was generated and whether an Oracle Provenance block exists in any ReadMe inside it.)*
- **Action required:**
  1. Download the v5 archive locally and inspect for: an inner `ReadMe.md`, the input `.dream3d` file (`PoleFigure_Exemplars_v5.dream3d`), the pipeline file that produced the exemplars, the EbsdLib version that produced them, and any provenance notes. Promote this content into the verification archive ReadMe per Step 0's Oracle Provenance policy.
  2. **Crucial:** locate the v4 archive (last referenced before PR #1587) and produce a side-by-side comparison report against v5. Because the EbsdLib upgrade in #1587 *changed expected output*, this comparison is the closest thing to a legacy-vs-current ground truth check we have on file. Document any pixel deltas between v4 and v5 in the verification archive.

## Oracle classification (tentative)

- **Recommended primary class:** **3 (Paper-based)** — pole figures and stereographic / modified Lambert projections are textbook material. Suggested references:
  - Bunge, *Texture Analysis in Materials Science* (1982), Butterworths — canonical orientation-distribution / pole-figure derivation.
  - Rowenhorst et al., *"Consistent representations of and conversions between 3D rotations"*, Modelling Simul. Mater. Sci. Eng. 23 (2015) 083501 — used as the standard SIMPLNX/EbsdLib reference for orientation conventions.
  - Roşca, *"New uniform grids on the sphere"*, Astronomy & Astrophysics 520 (2010) — modified Lambert square / equal-area projection.
- **Recommended companion class:** **2 (Reference implementation)** — compare against MTEX (MATLAB), OIM Analysis (EDAX), or HKL Channel-5 outputs on the same input EBSD scan. EbsdLib itself can also serve as a reference if its outputs are independently validated against MTEX.
- **Recommended companion class:** **4 (Invariant-based)** — strong invariants apply:
  - Identity orientation (φ1 = Φ = φ2 = 0) on a single-phase cubic dataset must place a hot pixel at (0,0) of the `<001>` pole figure (canonical pole at center).
  - A uniform random orientation field must produce a near-uniform pole figure intensity (within MRD ≈ 1 ± sampling noise).
  - Crystal-symmetry equivalents: any two Euler angles related by a Laue-class symmetry operation must land on the same binned pixel.
  - All MRD intensity values on a normalized figure must integrate to a constant (1.0 over the unit hemisphere by construction).
- **Class 5 (Expert visual)** legitimately applies because pole figures are a *visualization* output and visual sanity checks are part of canonical practice — but expert visual MUST NOT replace classes 1–4 here. Use it as the final acceptance step, not the only one.
- **Why not class 1 (Closed-form analytic):** No closed-form expected pixel value — the output is a binned image whose contents depend on quadrature and the modified Lambert / stereographic projection. Closed-form checks reduce to invariant checks (which is class 4).
- **Action required:** Developer to defend or replace this multi-class proposal. Specifically: pick the primary citation (Bunge or Rowenhorst), pick the reference-implementation comparison target (MTEX is most defensible), and write at least one Class-4 invariant test in the test file (the current 4 algorithmic tests are exemplar-comparison only, not invariant assertions).

## V&V status so far

| Item | Status | Notes |
|---|---|---|
| Algorithm review (`review-algorithm` skill) | Not visible from PR history | No PR explicitly performs the line-by-line review. Useful, but post-#1587 most of the algorithm is in EbsdLib and the review target shifts there. |
| Code path coverage (algorithmic) | Partial | 4 tests cover the (algorithm × mask) cross-product on cubic phase. Layout, non-cubic Laue groups, multi-phase, and disable-output paths are uncovered. |
| Code path coverage (SIMPL conversion) | Good | PR #1588 added SIMPL 6.4 + 6.5 conversion test. |
| Exemplar data in Data_Archive | **Yes** | `PoleFigure_Exemplars_v5.tar.gz` referenced in test/CMakeLists.txt. |
| Exemplar provenance documented | Unknown | TBD by inspecting archive contents. v4→v5 transition under PR #1587 is undocumented. |
| Oracle class recorded | **No** | This document is the first to propose one. |
| Toy data / independent expected output (Step 0 c) | No | No script or hand-derivation on file. |
| Legacy comparison report (Step 0 e) | No | `compare-legacy-dream3d` has not been run. **Especially important here because both EbsdLib bumps (#1472 → 2.0.0 and #1587 → 2.4.0) could have shifted output relative to legacy DREAM3D 6.5.172.** |
| Deviation entries (`WritePoleFigure-D<N>`) | None | Not yet written. Strong candidates listed below. |
| Documentation currency | Probably current | Updated by PR #1571 (ChoicesParameter descriptions). The doc still describes "Pole Figure Type" with values 0=Color, 1=Discrete and "Image Layout" with 0=Horizontal/1=Vertical/2=Square — matches the source `enum class LayoutType` and `Algorithm` in `WritePoleFigure.hpp`. Needs accuracy audit per `review-filter-docs` after PR #1587 to confirm the rendering description still matches what EbsdLib's `PoleFigureCompositor` actually does. |
| Verification archive (OneDrive) | No | Not yet created. |

## Gaps to close (to meet Step 0 / Legacy Comparison policy)

1. **Confirm the oracle.** Defend or refine the 3 (Paper) / 2 (Reference) / 4 (Invariant) tri-class proposal. Pick a primary paper citation. Defend why class 5 is acceptable as a final-acceptance step but not a substitute for 1–4.
2. **Promote at least one of the four existing tests into an explicit Class-4 invariant assertion.** For example: assert that summed MRD intensity over a normalized pole figure is ≈ 1, or that the discrete count over all hemispheres equals the input voxel count (minus masked voxels).
3. **Add a non-cubic Laue-group test.** The EbsdLib `LaueOps` dispatch table for hexagonal / trigonal / tetragonal / monoclinic / orthorhombic / triclinic is not exercised by any current unit test. A multi-phase exemplar with at least one cubic + one hexagonal phase would close this gap.
4. **Add a layout test.** Layouts 1 (Vertical) and 2 (Square) are uncovered.
5. **Inspect `PoleFigure_Exemplars_v5.tar.gz` and document provenance.** Especially document what changed between v4 and v5 — that is the only on-record signal of how PR #1587 affected the numerical output.
6. **Run the legacy comparison (`compare-legacy-dream3d`).** This is more important for this filter than for most because the production-code path has been substantially rewritten across two EbsdLib bumps. Expected outcome: at least one Deviation entry calling out output drift between SIMPL 6.5.172 and current SIMPLNX (post-#1587).
7. **Produce the Algorithm Relationship one-liner.** Tentative: *"Originally a port of the SIMPL `WritePoleFigure` filter; over PRs #1472 and #1587 the rendering and projection math was migrated to EbsdLib 2.4.0, leaving this filter as a thin driver around `ebsdlib::PoleFigureCompositor`. UI fix in #1566. Per-filter SIMPL conversion test added in #1588."*
8. **Archive everything** per `archive-filter-verification` for the OneDrive folder.

## Recommended Deviation entries (proposed, pending legacy comparison)

> **Deviation ID:** `WritePoleFigure-D1`
> **Filter UUID:** `00cbb97e-a5c2-43e6-9a35-17a0f9ce26ed`
> **Symptom:** Pixel-level output (RGB and MRD intensity arrays) differs between SIMPLNX pre-PR #1587 and SIMPLNX post-PR #1587 on the same input dataset and the same parameter values.
> **Root cause:** PR #1587 replaced an in-repo `canvas_ity`-based rendering pipeline plus a SIMPLNX-internal information-block / scale-bar drawer with `ebsdlib::PoleFigureCompositor::generateCompositeImage()` from EbsdLib 2.4.0. The exemplar archive was bumped from v4 to v5 to match the new output; the v4→v5 delta is the magnitude of this deviation but is not currently quantified anywhere.
> **Affected users:** Anyone scripting against the byte-level contents of TIFFs produced by this filter, anyone whose downstream analysis ingests the MRD intensity arrays at high precision, and anyone generating reproducible figures across SIMPLNX versions before/after April 2026.
> **Recommendation:** Run a side-by-side v4-vs-v5 exemplar comparison and quantify the per-pixel L∞ and L2 difference for each test case. If the difference is below a sub-pixel anti-alias threshold, document and accept; if not, identify which step in the EbsdLib `PoleFigureCompositor` produces the drift and decide whether to file an EbsdLib issue or accept as a known migration deviation.
> **Status:** Proposed — pending the v4-vs-v5 comparison.

> **Deviation ID:** `WritePoleFigure-D2`
> **Filter UUID:** `00cbb97e-a5c2-43e6-9a35-17a0f9ce26ed`
> **Symptom:** Output may differ between SIMPLNX (current, EbsdLib 2.4.0) and DREAM3D 6.5.172 (legacy SIMPL `WritePoleFigure`) due to two compounding EbsdLib upgrades during the audit period.
> **Root cause:** PR #1472 moved this filter onto the EbsdLib 2.0.0 API; PR #1587 moved it onto the EbsdLib 2.4.0 `PoleFigureCompositor`. Either upgrade could have changed the numerical pole-figure routine (modified Lambert square interpolation, color-table generation, layout placement) relative to whatever EbsdLib version SIMPL 6.5.172 was built against.
> **Affected users:** Anyone comparing pole-figure outputs across DREAM3D 6.5.172 and DREAM3D-NX.
> **Recommendation:** Run `compare-legacy-dream3d` against a shared toy EBSD dataset with both cubic and hexagonal phases. Quantify per-pixel drift in RGB images and per-bin drift in MRD arrays. If non-trivial, document which version is the more correct reference (cross-check with MTEX) and recommend trusting that version.
> **Status:** Proposed — pending the legacy comparison run.

> **Deviation ID:** `WritePoleFigure-D3`
> **Filter UUID:** `00cbb97e-a5c2-43e6-9a35-17a0f9ce26ed`
> **Symptom:** Pre-PR #1566, the `Save Intensity Plots` toggle in the GUI did not gate the *Intensity Geometry Path* and *Normalize To MRD* controls (instead it was wrongly linked to the *Image Geometry Path*). Users on pre-#1566 versions could be confused about which output paths controlled which output, but supplied-arguments executions still worked correctly.
> **Root cause:** Cut-and-paste error in `parameters()`. Fixed in PR #1566 by re-wiring the `linkParameters()` calls.
> **Affected users:** GUI users of pre-PR-#1566 builds.
> **Recommendation:** Trust SIMPLNX post-#1566. No legacy comparison action needed (this is a SIMPLNX-internal UX bug, not a SIMPL-vs-SIMPLNX deviation).
> **Status:** Proposed — confirm by checking the legacy SIMPL filter's parameter linking (if any equivalent existed).

> **Deviation ID:** `WritePoleFigure-D4` *(speculative — flag for inspection)*
> **Filter UUID:** `00cbb97e-a5c2-43e6-9a35-17a0f9ce26ed`
> **Symptom:** Behavior of masked voxels may differ between SIMPL and SIMPLNX. Specifically, whether masked-out voxels are simply skipped (don't contribute to any pixel) vs. counted-as-zero (contribute to denominator in MRD normalization) is not clearly stated in the documentation and is not directly tested.
> **Root cause:** EbsdLib's compositor handles the mask via the input Euler-angle slice; the SIMPLNX filter pre-filters the Euler angles by mask before passing them. Whether the MRD denominator uses `total_voxels` or `unmasked_voxels` should be confirmed.
> **Affected users:** Any user with a non-trivial mask whose downstream interpretation depends on absolute MRD values.
> **Recommendation:** Inspect the EbsdLib source for the masking semantics, document explicitly in this filter's `.md`, and add a unit test that compares the same dataset run with mask=all-ones vs no mask — they should produce identical results.
> **Status:** Speculative — verify against EbsdLib source and current test exemplars.
