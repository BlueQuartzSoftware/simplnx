# Retroactive V&V: ComputeIPFColorsFilter

*Report status:* **DRAFT**. Generated from git-history and source-tree inspection. Developer must confirm or correct the Oracle class, Algorithm Relationship, and the V&V status entries.

## Metadata

| Field | Value |
|---|---|
| SIMPLNX UUID | `64cb4f27-6e5e-4dd2-8a03-0c448cb8f5e6` |
| SIMPLNX ClassName | `ComputeIPFColorsFilter` |
| SIMPLNX Human Name | Compute IPF Colors |
| SIMPL UUID | `{a50e6532-8075-5de5-ab63-945feb0de7f7}` *(from `test/simpl_conversion/6_5/ComputeIPFColorsFilter.json`)* |
| SIMPL ClassName | `GenerateIPFColors` *(from `Filter_Name` in 6.4 and 6.5 conversion fixtures)* |
| SIMPL Human Name | Generate IPF Colors |
| Plugin | OrientationAnalysis |

### Source files scanned

- `src/Plugins/OrientationAnalysis/src/OrientationAnalysis/Filters/ComputeIPFColorsFilter.{hpp,cpp}`
- `src/Plugins/OrientationAnalysis/src/OrientationAnalysis/Filters/Algorithms/ComputeIPFColors.{hpp,cpp}`
- `src/Plugins/OrientationAnalysis/test/ComputeIPFColorsTest.cpp`
- `src/Plugins/OrientationAnalysis/test/simpl_conversion/6_5/ComputeIPFColorsFilter.json`
- `src/Plugins/OrientationAnalysis/test/simpl_conversion/6_4/ComputeIPFColorsFilter.json`
- `src/Plugins/OrientationAnalysis/docs/ComputeIPFColorsFilter.md`
- `src/Plugins/OrientationAnalysis/test/CMakeLists.txt` (line 27 lists the test, line 157 declares the test-data archive)

## Algorithm Relationship

- **Tentative classification:** **Port** — the SIMPLNX filter is a renamed but otherwise direct translation of the legacy SIMPL `GenerateIPFColors` filter (UUID `{a50e6532-8075-5de5-ab63-945feb0de7f7}`). The SIMPLNX UUID `64cb4f27-…` is a new UUID assigned during the SIMPL→SIMPLNX migration; the SIMPL→SIMPLNX UUID mapping is recorded in the FilterList for backwards-compatibility loading. The IPF math itself is **delegated to EbsdLib** (`ebsdlib::LaueOps::generateIPFColor()`), so the filter is a thin orchestration layer (parallel range, mask handling, phase-bounds check, RGBA→RGB unpacking).
- **Evidence:**
  - `executeImpl()` only constructs an `InputValues` struct and calls `ComputeIPFColors(...)()`.
  - The algorithm's per-voxel work is a single call to `ops[crystalStructure]->generateIPFColor(eulers, refDir, false)` followed by `RgbColor::dRed/dGreen/dBlue` extraction.
  - No PR in the audit window changed the IPF math.
  - `FromSIMPLJson()` in `ComputeIPFColorsFilter.cpp` maps the seven legacy SIMPL parameter keys 1:1 to the new keys.
- **Action required:** Confirm by inspecting the legacy `GenerateIPFColors` source in DREAM3D 6.5.171 — at minimum verify that 6.5.171 also calls EbsdLib's `generateIPFColor` with the same `convertDegrees=false` flag and the same Bunge convention. Note that the legacy filter linked against an older version of EbsdLib, so any per-pixel diff is most likely an EbsdLib version difference rather than a filter difference.

## PRs inspected (since 2025-10-01)

> Pruned: pure-style/repo-wide refactor PRs (#1457 static-inline cleanup, #1501 Vec3 unification, #1524 filter-tag style fix, #1538 zlib extraction in tests, #1543 doc pipeline-reference sweep, #1500 dataset-comparison utility) are listed at the bottom. **#1472 (EbsdLib 2.0.0 bump)** is also listed in the pruned table but with a flagged note — IPF coloring is implemented inside EbsdLib, so the EbsdLib version bump is a **material library change** for this filter even though the filter source diff is API-renaming-only.

### PR #1438 — *"ENH: Microtexture related filter cleanup"* — merged 2025-10-25

- **Files in this filter:** algorithm (.cpp) only
- **Diff size:** 1 file, +1 / -1 lines (changed an EbsdLib include from `"…"` to `<…>`)
- **Change nature:** Pure include-style cleanup as part of a larger microtexture pipeline cleanup PR. The PR body lists many bug fixes for *other* filters; the IPF-Colors hunk is cosmetic.
- **V&V content:** None for this filter. Listed here only because the policy maintainer flagged #1438 as "always promote." For ComputeIPFColors specifically the diff is one include-style line.

### PR #1476 — *"BUG/ENH: Fix Backwards Pipeline Compatibility and Add Testing"* — merged 2026-01-06

- **Files in this filter:** test (.cpp), +20 lines
- **Change nature:** Added a stand-alone debug/exercise block at the top of the test (lines 60–70) that constructs a `HomochoricDType`, converts to Rodrigues, projects into the OrthoRhombic FZ, and converts to Euler — printing each intermediate to `std::cout`. This is **diagnostic / smoke output for EbsdLib orientation conversions**, not a `REQUIRE` against IPF-color values. Included EbsdLib's `OrthoRhombicOps`, `Euler`, `Homochoric`, `Rodrigues` headers.
- **V&V content:** Low. The block executes but does not assert. It does serve as an unintentional **EbsdLib regression sentinel** — if any of those conversion outputs change, the printed numbers change but the test still passes.

### PR #1535 — *"ENH: Remove redundant preflight checks that are already done in the parameter"* — merged 2026-02-18 *(broad refactor, exception flagged because it was substantive preflight removal for THIS filter)*

- **Files in this filter:** filter (.cpp), 1 file, +3 / -49 lines
- **Change nature:** **Substantive preflight cleanup.** Removed the anonymous-namespace error-code constants (`k_MissingGeomError`, `k_IncorrectInputArray`, `k_MissingInputArray`, `k_MissingOrIncorrectGoodVoxelsArray`) and stripped four preflight guards: (1) crystal-structure component-count check, (2) Euler component-count check, (3) phases component-count check, (4) good-voxels existence + dtype check. All four are now enforced by `ArraySelectionParameter::AllowedTypes` / `AllowedComponentShapes` in `parameters()`. Also flipped one `getDataAs<Float32Array>(…)*` pointer-dereference to a `getDataRefAs<Float32Array>(…)` reference per project convention.
- **V&V content:** Negative-test surface area was reduced — the four removed error codes (and the messages they produced) are no longer reachable from this filter. Behavior on **valid** inputs is unchanged. Any downstream test that asserted on the old error codes (`-71440…-71443`) would now fail; none appears in `ComputeIPFColorsTest.cpp`.

### PR #1588 — *"ENH: SIMPL Backwards Compatibility Test Redesign"* — merged 2026-04-22

- **Files in this filter:** test (.cpp) +48 lines, plus two new fixture files
  - `test/simpl_conversion/6_4/ComputeIPFColorsFilter.json` (1071 bytes)
  - `test/simpl_conversion/6_5/ComputeIPFColorsFilter.json` (1128 bytes)
- **Change nature:** **Test addition.** Added a per-filter SIMPL→SIMPLNX backwards-compatibility test (`OrientationAnalysis::ComputeIPFColorsFilter: SIMPL Backwards Compatibility`) that exercises both SIMPL 6.4 (`Filter_Name = GenerateIPFColors`) and 6.5 (UUID-mapped `{a50e6532-…}`) pipeline conversion paths via `DYNAMIC_SECTION`. Verifies `UseMask`, the four `DataPath` parameters, and the output-array name parameter were all converted. The `ReferenceDir` (`FloatVec3FilterParameterConverter`) is verified by *successful pipeline loading* only, not by value-equality (per the comment).
- **V&V content:** **Pipeline-conversion correctness only** — the test confirms a legacy `GenerateIPFColors` JSON deserializes into the new `ComputeIPFColorsFilter` with the right argument values. It does **not** verify the filter's *output* matches legacy.

### Pruned PRs (touched the file but not behaviorally relevant to this filter)

| PR | Subject | Why pruned |
|---|---|---|
| #1457 | Clean up 'static inline' from filter headers | Style-only, header refactor (+7 / -7) |
| #1472 | Update to EbsdLib 2.0.0 API | API rename only inside this filter (`LaueOps::` → `ebsdlib::LaueOps::`, `EbsdLib::CrystalStructure` → `ebsdlib::CrystalStructure`). **HOWEVER** — IPF coloring math lives in EbsdLib's `LaueOps::generateIPFColor`. The 1.x → 2.0 EbsdLib upgrade is a **material library version change for this filter** and is the prime suspect for any per-pixel RGB difference vs. SIMPL 6.5.171. See Deviation D1 below. |
| #1500 | Add unit test functions for comparing full datastructure | Single `+1` line (added a `Pipeline.hpp` include); not used yet by this test |
| #1501 | Combine Matrix3x1, Point3D, Vec3 into a Vec3<T> in Array.hpp | Refactor; algorithm body uses Vec3 in the same way (+6 / -9) |
| #1524 | Fixed filter tags to consistently use the full filter name | Test cosmetic (+1 / -1 in test tag) |
| #1538 | Replace cmake subprocess tar.gz extraction with zlib | Test infrastructure (constructor signature for `TestFileSentinel`) |
| #1543 | DOC: Update pipeline references in each of the documentation files | Docs only (+1 / -3 in `.md`) |

## Test coverage detected

`ComputeIPFColorsTest.cpp` contains 2 `TEST_CASE`s:

1. `OrientationAnalysis::ComputeIPFColors` — End-to-end exemplar comparison. Loads `so3_cubic_high_ipf_001.dream3d` (which contains both the inputs *and* a pre-computed `IPF Colors` exemplar from SIMPL/DREAM3D), runs the filter against `Reference Direction = [0, 0, 1]` with mask enabled, and byte-by-byte compares the output `IPF Colors_Test_Output` array against the embedded exemplar `IPF Colors`. Wraps the assertion in a hand-written loop with a single boolean `valid`. Also writes the resulting DataStructure to `ComputeIPFColors_Test.dream3d` for later inspection. Includes the diagnostic Homochoric→Rodrigues→Euler `std::cout` block from PR #1476 at the top.
2. `OrientationAnalysis::ComputeIPFColorsFilter: SIMPL Backwards Compatibility` *(added by PR #1588)* — Loads each of the 6.4 and 6.5 fixture pipelines via `DYNAMIC_SECTION`, asserts the resulting `PipelineFilter` has the right UUID, and `CHECK`s that `UseMask`, the four `DataPath` parameters, and the output-array-name parameter all match the expected values. `ReferenceDir` value-equality is **not checked** — only successful pipeline loading.

Test 1 covers a single configuration (cubic-high crystal symmetry, `Reference Direction = [0,0,1]`, mask enabled). It does **not** parameterize across:
- Other Laue groups (hexagonal, trigonal, orthorhombic, etc.) — though all are exercised inside EbsdLib via dispatch
- Other reference directions (e.g. `[1,0,0]`, `[1,1,1]`)
- The `useMask = false` code path
- The `mask` array of `DataType::uint8` (only `boolean` is tested)
- The phase-out-of-bounds warning path (`m_PhaseWarningCount > 0` → error -48000)

## Exemplar archive

- **Archive name:** `so3_cubic_high_ipf_001.tar.gz`
- **SHA512:** `dfe4598cd4406e8b83f244302dc4fe0d4367527835c5ddd6567fe8d8ab3484d5b10ba24a8bb31db269256ec0b5272daa4340eedb5a8b397755541b32dd616b85`
- **Referenced in:** `src/Plugins/OrientationAnalysis/test/CMakeLists.txt` line 157
- **Provenance per the test source comment (lines 1–20):** *"This test file was produced by SIMPL/DREAM3D. Our results should match theirs."* The `.dream3d` file ships with both the inputs (Euler angles, phases, mask, crystal structures) **and** the pre-computed `IPF Colors` array that DREAM3D 6.x produced. The original ASCII inputs (`EulerAngles.csv`, `Phases.csv`, `IPFColor.csv`) referenced in the comment header are no longer used by the live test path but indicate how the exemplar was generated.
- **Action required:** Download the archive locally and inspect for: an inner `ReadMe.md`, the source `.csv` files, the SIMPL pipeline that produced the exemplar IPF Colors array (so the comparison is reproducible), and the version of DREAM3D / EbsdLib used. Promote this content into the verification archive ReadMe per Step 0's Oracle Provenance policy. Particular attention: confirm whether the embedded `IPF Colors` array was generated by SIMPL using EbsdLib 1.x or an even older orientation library — that determines the size of any expected EbsdLib-attributable per-pixel diff.

## Oracle classification (tentative)

- **Recommended class:** **3 (Paper-based)** with **Class 4 (Invariant) companion**.
- **Class 3 rationale:** IPF coloring is canonically defined in the textbooks (Bunge, *Texture Analysis in Materials Science*; Esling et al.) and modernized in **Rowenhorst, Rollett, Rohrer, Groeber, Jackson, Konijnenberg & De Graef (2015), *"Consistent representations of and conversions between 3D rotations,"* Modelling Simul. Mater. Sci. Eng. 23 083501**. The reference colors for canonical orientations under cubic-high symmetry with reference direction [001] are well-known (e.g. {001}→red, {101}→green, {111}→blue, with continuous interpolation across the standard stereographic triangle). These canonical orientations can be entered directly as Euler triplets and the resulting RGB triplets checked against published values.
- **Class 4 (Invariant) companion rationale:** Two strong invariants apply regardless of the paper reference:
  1. **Symmetry equivariance** — the entire purpose of `LaueOps::generateIPFColor` is that all symmetry-equivalent orientations produce the same IPF color. A test that takes one Euler triplet, applies each of the symmetry operators for the relevant Laue group, and checks all results map to the same RGB is a clean Class-4 assertion.
  2. **Mask sentinel** — voxels with `mask == false` (or `useGoodVoxels == true` and `goodVoxels[i] == 0`) must receive the fixed sentinel color `(0,0,0)`. Confirmed by inspection of `ComputeIPFColorsImpl::convert()` lines 64–66 — the algorithm zero-initializes every output triplet before the conditional assignment.
- **Note on EbsdLib delegation:** Because the math is library-delegated, the SIMPLNX↔SIMPL **algorithmic** diff for IPF colors should be near-zero. Any per-pixel diff is almost certainly attributable to **EbsdLib version drift between SIMPL 6.5.171's bundled EbsdLib and current `ebsdlib::` (post-#1472 EbsdLib 2.0.0)** — not to a SIMPLNX filter change.
- **Action required:** Developer to confirm the Rowenhorst et al. 2015 reference is appropriate (or substitute a more specific IPF-coloring paper), and to defend or replace the Class-3 + Class-4-companion recommendation.

## V&V status so far

| Item | Status | Notes |
|---|---|---|
| Algorithm review (`review-algorithm` skill) | Not visible from PR history | No PR explicitly performs the line-by-line review. Algorithm is small (~120 lines including the threaded impl class). |
| Code path coverage (algorithmic) | **Limited** | Single configuration: cubic-high, refDir = [0,0,1], `useMask = true` with bool mask. No other Laue groups, no other refDirs, no `uint8` mask, no `useMask = false`, no phase-out-of-bounds path tested in SIMPLNX. (EbsdLib internally exercises all Laue ops, so the SIMPLNX-side dispatch is covered indirectly only.) |
| Code path coverage (SIMPL conversion) | Good | PR #1588 added SIMPL 6.4 (Filter_Name fallback) + 6.5 (UUID) conversion test. |
| Exemplar data in Data_Archive | **Yes** | `so3_cubic_high_ipf_001.tar.gz` referenced in test/CMakeLists.txt line 157. |
| Exemplar provenance documented | **Partially** | Inline test comment says "produced by SIMPL/DREAM3D" but the SIMPL pipeline, the SIMPL+EbsdLib version used, and the `.csv` provenance are not on file. Needs an archive-level ReadMe. |
| Oracle class recorded | **No** | This document is the first to propose one (Class 3 + Class 4 companion). |
| Toy data / independent expected output (Step 0 c) | **Partial** | The test compares against an embedded SIMPL exemplar, which is closer to Class-2 (legacy oracle) than to a hand-derived expected value. A separate hand-derived check on a cubic-high {001}/{101}/{111} cell triplet would be a clean Class-3 contribution. |
| Legacy comparison report (Step 0 e) | **No** | `compare-legacy-dream3d` has not been run. The current end-to-end test does compare against an exemplar produced by legacy SIMPL — this is *implicitly* a legacy comparison but is not formally captured as a Deviation report and is restricted to one configuration. |
| Deviation entries (`ComputeIPFColors-D<N>`) | None | Not yet written. The EbsdLib 2.0.0 bump (PR #1472) is the leading Deviation candidate — see D1 proposal below. |
| Documentation currency | Mostly current | Updated by PR #1543 (pipeline references). The doc lacks any reference to the Reference Direction parameter, the mask parameter, the Crystal Structures input, the `useGoodVoxels` linkage, or the Rowenhorst et al. 2015 paper. Needs `review-filter-docs`. |
| Verification archive (OneDrive) | No | Not yet created. |

## Gaps to close (to meet Step 0 / Legacy Comparison policy)

1. **Confirm the oracle.** Class 3 (Paper-based, Rowenhorst et al. 2015) + Class 4 (Invariant: symmetry equivariance + mask sentinel) is the recommended pair. Defend or replace.
2. **Promote the implicit legacy comparison into an explicit Deviation report.** The existing exemplar IS legacy output. Add metadata to the archive recording (a) which DREAM3D / EbsdLib version produced it, (b) the SIMPL pipeline used, and (c) the input `.csv` files so a future reviewer can reproduce. If current SIMPLNX still byte-matches the embedded exemplar (the test passes, so apparently yes), then there is **no** EbsdLib-attributable Deviation — but this needs confirmation across more Laue groups and reference directions before that conclusion can be made.
3. **Add Class-4 invariant tests** to `ComputeIPFColorsTest.cpp`:
   - For each Laue group, take a representative Euler triplet, apply each of that group's symmetry operators, and assert all results produce the same RGB.
   - For a mask with `i`-th voxel false, assert `output[3*i] == 0 && output[3*i+1] == 0 && output[3*i+2] == 0`.
4. **Expand the configuration matrix.** Test at minimum: `useMask = false`; `mask` of `DataType::uint8`; reference directions other than `[0,0,1]`; the phase-out-of-bounds path that triggers error -48000.
5. **Add a Class-3 hand-derived smoke check.** For cubic-high symmetry with `refDir = [0,0,1]`, a voxel with Euler `(0, 0, 0)` should produce **red** (~`(255, 0, 0)`); a voxel oriented so that `[101]` aligns with `[001]_sample` should produce **green**; `[111]_crystal` aligned with `[001]_sample` should produce **blue**. Encode these directly as `REQUIRE`s and they become a paper-grounded oracle independent of the SIMPL exemplar.
6. **Run the legacy comparison.** Use `compare-legacy-dream3d` against DREAM3D 6.5.171 with at least 3 Laue groups × 3 reference directions. Expected outcome: either zero Deviations (EbsdLib 1.x and 2.0 produce identical IPF colors) or one Deviation entry attributable to EbsdLib version drift, never a SIMPLNX filter bug.
7. **Replace the diagnostic `std::cout` block** (lines 60–70 of the test) with a real `REQUIRE` against the published Homochoric→Rodrigues→Euler reference values, or move it into an EbsdLib-side test where it belongs.
8. **Refresh the user-facing doc.** Add a parameter table covering the Reference Direction, mask, Euler Angles, Phases, Crystal Structures, and output array name. Cite Rowenhorst et al. 2015 for the IPF coloring scheme.
9. **Produce the Algorithm Relationship one-liner.** Tentative: *"Port — direct translation of the SIMPL `GenerateIPFColors` filter (UUID `{a50e6532-…}`); IPF math fully delegated to EbsdLib's `LaueOps::generateIPFColor`. No algorithmic change in SIMPLNX since 2025-10-01; the only library-level change in window is the EbsdLib 1.x → 2.0.0 upgrade in PR #1472."*
10. **Archive everything** per `archive-filter-verification` for the OneDrive folder.

## Recommended Deviation entries (proposed, pending legacy comparison)

> **Deviation ID:** `ComputeIPFColors-D1`
> **Filter UUID:** `64cb4f27-6e5e-4dd2-8a03-0c448cb8f5e6`
> **Symptom:** *(potential, not yet confirmed)* Per-pixel RGB differences between SIMPLNX `ComputeIPFColors` output and SIMPL 6.5.171 `GenerateIPFColors` output on the same Euler / phase / crystal-structure inputs.
> **Root cause:** The IPF coloring computation is delegated to EbsdLib (`LaueOps::generateIPFColor`). PR #1472 (merged 2025-11-24) upgraded SIMPLNX from EbsdLib 1.x to **EbsdLib 2.0.0**. Any changes between those EbsdLib versions to (a) the standard-stereographic-triangle projection, (b) the orientation-conversion intermediates (Euler↔Rodrigues↔Quaternion), or (c) the per-Laue-group RGB interpolation will show up as a per-pixel RGB diff for this filter even though the SIMPLNX filter source itself made no algorithmic change. The current ComputeIPFColors test still passes against the legacy-produced exemplar, which is **evidence — but not proof** — that no such drift exists for the specific configuration tested (cubic-high, refDir = [0,0,1], boolean mask).
> **Affected users:** Anyone byte-comparing IPF color output between DREAM3D 6.5.171 and current DREAM3DNX, or anyone whose downstream visualization is sensitive to ±1 LSB drift in a single RGB channel.
> **Recommendation:** **Run the legacy comparison across multiple Laue groups and reference directions.** If zero diff: close this Deviation as "no observed drift; EbsdLib 2.0.0 is bit-compatible with the version SIMPL 6.5.171 shipped." If non-zero diff: trust SIMPLNX (newer EbsdLib is the maintained codebase) and document the magnitude of the diff per Laue group.
> **Status:** **Proposed — not yet verified.** Verification = run `compare-legacy-dream3d` per Gap #6 above.

> **Deviation ID:** `ComputeIPFColors-D2`
> **Filter UUID:** `64cb4f27-6e5e-4dd2-8a03-0c448cb8f5e6`
> **Symptom:** SIMPL `GenerateIPFColors` may have raised one of four specific error codes (`-71440…-71443`) on malformed input arrays (wrong component count, missing mask, wrong mask dtype). SIMPLNX no longer raises those error codes for the same malformed inputs.
> **Root cause:** PR #1535 (merged 2026-02-18) removed those four guards on the grounds that the same checks are now enforced earlier by `ArraySelectionParameter::AllowedTypes` and `AllowedComponentShapes` in `parameters()`. The user-facing failure mode is unchanged (the filter still rejects the bad input) but the failure point and the error code are different.
> **Affected users:** Anyone whose pipeline-validation tooling parses SIMPL error codes by number, or anyone with a custom test that expected error code `-71440`/`-71441`/`-71442`/`-71443`.
> **Recommendation:** Trust SIMPLNX. The new failure path is earlier (parameter validation runs before `preflightImpl`) and produces a structured `ParameterError` instead of an ad-hoc int code. Document the error-code mapping in the migration notes for legacy-pipeline maintainers.
> **Status:** **Proposed — high confidence.** Diff is directly visible in the PR #1535 source diff; confirmation only requires re-running a SIMPL 6.5.171 pipeline that intentionally feeds a 2-component Euler array and recording the error code that legacy emits.
