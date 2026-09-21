# V&V Report: ComputeFaceIPFColoringFilter

|           |                          |
|-----------|--------------------------|
| Plugin    | OrientationAnalysis      |
| SIMPLNX UUID | 30759600-7c02-4650-b5ca-e7036d6b568e |
| DREAM3D 6.5.171 equivalent | GenerateFaceIPFColoring (legacy SIMPL UUID `0a121e03-3922-5c29-962d-40d88653f4b6`) |
| Verified commit | *<filled at SBIR deliverable assembly>* |
| Status | COMPLETE     |
| Sign-off | Michael Jackson <mike.jackson@bluequartz.net> — 2026-07-16 |

## At a glance

| Aspect                 | Current state            |
|------------------------|--------------------------|
| Algorithm Relationship | **Port** of legacy `GenerateFaceIPFColoring` — per-face IPF color math is line-for-line identical. Deltas: output split from one 6-component array into two 3-component arrays, an added Color Key choice (TSL/PUCM/Nolze-Hielscher), and the EbsdLib 3.0.0 API. The 2023 wrong-phase bug (issue #1635) was ported verbatim and is **fixed here**. |
| Oracle (confirmed)     | **Class 1 (Analytical)** + Class 4 companion — closed-form IPF corner colors on a hand-built mixed cubic/hex mesh. Encoded as `Class 1 Oracle - mixed-phase analytical` and the real-HDF5 65,536-block tail oracle; both pass. |
| Code paths enumerated  | 13 of 16 exercised; 1 is unreachable dead code (`-2431`) and 2 are defensive guards (out-of-range crystal-structure index, out-of-range color key) noted below. |
| Tests today            | 5 registered test cases plus 1 hidden OOC contract test — analytical output, phase bounds, negative validation, Color-Key plumbing, SIMPL conversion, and a 65,536-block real-HDF5 tail witness. |
| Exemplar archive       | **None — analytical fixtures inlined.** The prior `Valid filter execution` test compared against a `SurfaceMeshFaceIPFColors` array baked into the shared `6_6_Small_IN100_GBCD.tar.gz`; that comparison was a circular oracle (filter's own pre-fix output) and is **retired**. |
| Legacy comparison      | **Run** — SIMPLNX vs DREAM3D 6.5.171 on a hand-built cubic/hex mesh and a 756,474-face real mesh found 2 deviations. A patched local legacy build confirms the D1 root cause. Cubic coloring is identical. |
| Bug flags              | `…-D1` — wrong-phase Laue operator on the Phase-2 side (a 6.5.171 bug, fixed). `…-D2` — hex basal IPF hue differs between EbsdLib (NX, green) and legacy EbsdLib (blue); resolved as a library convention difference — EbsdLib is canonical (3.0.0 and 3.1.0 agree), legacy is the deviation, trust SIMPLNX. Affects all hex IPF coloring. |
| V&V phase | The original COMPLETE status and 2026-07-16 sign-off are retained. OOC recertification adds the analytical HDF5 tail witness and paired CTest checks. The optional documentation image remains outside this work. |

## Summary

`ComputeFaceIPFColoringFilter` assigns an IPF color to each side of every surface-mesh triangle. A Class 1 analytical oracle and a real-HDF5 65,536-block tail oracle verify the output. SIMPLNX matches the analytical oracle, and the documented DREAM3D 6.5.171 deviations remain reconciled.

## Algorithm Relationship

**Port**

*Evidence:* SIMPLNX inherits the legacy SIMPL UUID `0a121e03-3922-5c29-962d-40d88653f4b6` (see `OrientationAnalysisLegacyUUIDMapping.hpp`). The per-face coloring kernel (`CalculateFaceIPFColorsImpl::generate`) is a line-for-line translation of legacy `GenerateFaceIPFColoring.cpp`.

*Port-time deltas (each assessed for output impact):*

1. **Output layout** — legacy wrote a single 6-component `SurfaceMeshFaceIPFColors` array (`m_Colors[6*i+0..5]`); SIMPLNX writes two separate 3-component arrays (`FirstFaceIPFColors`, `SecondFaceIPFColors`). Pure repackaging — the six bytes per face are identical, only regrouped. No value change.
2. **Color Key parameter** — SIMPLNX adds a TSL/PUCM/Nolze-Hielscher choice routed into `generateIPFColor(..., m_ColorKey)`; legacy always used TSL. Default (TSL) reproduces legacy output exactly; the new option is additive.
3. **EbsdLib API** — `Ebsd::` → `ebsdlib::`, and `generateIPFColor` gained the `ColorKeyKind` argument. EbsdLib upgraded to 3.0.0. No change to the TSL color math at the standard-triangle corners used by the oracle.
4. **Parallelization** — legacy used TBB-style parallel-for over faces. SIMPLNX structures the loop with `ParallelDataAlgorithm` but **parallelization is disabled**: the worker writes `UInt8Array` outputs via `operator[]`, and per project thread-safety policy DataArray/DataStore access is not thread-safe even for distinct indices (same disposition as the `ComputeFeatureFaceMisorientations` V&V). No output impact.
5. **Wrong-phase bug** — the Phase-2 branch's `m_CrystalStructures[phase1]` guard and operator lookup were ported from the legacy code. **Fixed here** (`phase1`→`phase2`). A patched local legacy build confirms the root cause.

*Material PRs since baseline:* #1631 (EbsdLib 3.0.0 + V&V cohort) added the Color Key option and first documented this bug with a `// KNOWN BUG` block; this V&V cycle removes that block and applies the fix.

## Oracle

*Class:* **1 (Analytical)**, with a **Class 4 (Invariant)** companion.

*Applied:* IPF color is a closed-form function of (orientation, reference direction, Laue symmetry). At the standard stereographic-triangle corners the color is a pure primary, independent of implementation: cubic-high `<100>`→(255,0,0), cubic-high `<111>`→(0,0,255), hex-high c-axis→(255,0,0), hex-high basal→(0,255,0) (red channel exactly 0, since a basal direction sits at χ = χ_max so r = 1 − χ/χ_max = 0). A 5-face hand-built mesh with corner-aligned normals (Phase 1 = cubic, Phase 2 = hex) yields fully hand-derivable expected colors for both sides of every face, and **all four** corner primaries are exercised (face 4 pins the hex c-axis red corner). Every feature carries a **distinct orientation** chosen so the corner colors remain hand-derivable (cubic: 90° about Z maps `<100>`→`<100>`, `<111>`→`<111>`; hex: 60° about c is a 6/mmm symmetry operation) — so a wrong-Euler-index defect (the Euler analogue of the #1635 phase-index bug) changes the output and fails the exact-value assertions. The Class 4 companion asserts the crispest bug signature: the hex (Phase-2) side of a basal face has red channel == 0, whereas the bug's cubic `<100>` lookup gives 255.

*Encoded:* `test/ComputeFaceIPFColoringTest.cpp::"OrientationAnalysis::ComputeFaceIPFColoringFilter: Class 1 Oracle - mixed-phase analytical"` — 5 faces × (3 first + 3 second) = 30 color-byte assertions + 1 invariant assertion, all pass. Verified to **fail** (a) on Face 0's second-color red channel when the `phase1`→`phase2` fix is reverted, and (b) on the hex-side exact values when the Phase-2 branch is mutated to read feature1's Euler angles (wrong-Euler-index mutation check, run 2026-07-08). The hidden `genuine HDF5 65536-block tail oracle` uses HDF5-OOC inputs and outputs. It checks distinct exact RGB values at tuple 65,535 and the one-face partial tail at tuple 65,536. It passes with 38 assertions.

*Caveat (surfaced by the legacy A/B):* the exact hex basal value `(0,255,0)` green is the **EbsdLib** assignment (canonical); legacy DREAM3D's older EbsdLib assigns the other basal corner (blue) to the same direction (deviation `-D2`). The convention-independent part of the oracle is the **red channel == 0** invariant. Both EbsdLib generations satisfy this invariant. The exact green is confirmed under EbsdLib 3.0.0 and 3.1.0.

*Second-engineer review:* **Signed off by Michael Jackson (technical authority), 2026-07-16.**

## Bugs found and fixed

This branch fixes all defects in this table. The fix will be in the DREAM3D-NX release after v7.4.1.

| Deviation | Defect | Affected released versions | Resolution in this branch |
|-----------|--------|----------------------------|---------------------------|
| `ComputeFaceIPFColoringFilter-D1` | The second face side used the first side's Laue operator. Mixed-phase colors were wrong, and some exterior sides stayed black. | DREAM.3D 6.5.171; DREAM3D-NX v7.0.0 through v7.4.1. | The second side now uses its own phase index. The analytical oracle verifies the correction. |

## Code path coverage

13 of 16 paths exercised (1 unreachable dead guard, 2 defensive guards untested).

Source: `src/Plugins/OrientationAnalysis/src/OrientationAnalysis/Filters/Algorithms/ComputeFaceIPFColoring.cpp` (265 lines), plus `ComputeFaceIPFColoringFilter.cpp` preflight/execute. Logical phases: (a) preflight validation, (b) execute color-key routing, (c) per-face label→phase resolution, (d) Phase-1 first-color, (e) Phase-2 second-color.

| #  | Phase            | Path          | Test case           |
|----|------------------|-----------------------------------------------------------------------------------|------------------------------------------------------------------------|
| 1  | (a) Preflight    | face label/normal tuple mismatch → error `-2430`| `Invalid filter execution` — "Inconsistent face data tuple dimensions" |
| 2  | (a) Preflight    | feature euler/phase tuple mismatch → error `-2432`               | `Invalid filter execution` — "Inconsistent cell data tuple dimensions" |
| 3  | (a) Preflight    | face labels array missing → error `-2431`       | *Unreachable dead code: the path is an `ArraySelectionParameter`, which guarantees existence before `preflightImpl` runs, and the `-2430` tuple validation would fail first anyway.* |
| 4  | (b) Execute      | Color Key 0/1/2 → TSL/PUCM/Nolze-Hielscher       | `ColorKey choice reaches algorithm` — all three kinds |
| 5  | (b) Execute      | Color Key out of [0,2] → error `-24340`         | *Not directly tested. `ChoicesParameter` constrains the index; low-value.* |
| 6  | (c) Per-face     | `feature1 > 0` → `phase1 = m_Phases[feature1]`   | `Class 1 Oracle` — faces 0, 2, 3     |
| 7  | (c) Per-face     | `feature1 <= 0` → `phase1 = 0`  | `Class 1 Oracle` — face 1 (label −1)|
| 8  | (c) Per-face     | `feature2 > 0` / `feature2 <= 0` → `phase2` set or 0              | `Class 1 Oracle` — faces 0/1 (valid), face 2 (−1)    |
| 9  | (d) First color  | `phase1 > 0` and crystal structure valid → own-phase IPF color   | `Class 1 Oracle` — faces 0, 2 (cubic red), face 3 (cubic blue), face 4 (hex c-axis red) |
| 10 | (d) First color  | `phase1 <= 0` → first color black               | `Class 1 Oracle` — face 1 → (0,0,0)  |
| 11 | (e) Second color | `phase2 > 0` and `CrystalStructures[phase2]` valid → **own-phase** IPF color (fix) | `Class 1 Oracle` — face 0 hex green, face 1 hex green, face 3 blue     |
| 12 | (e) Second color | `phase2 <= 0` → second color black              | `Class 1 Oracle` — face 2 → (0,0,0)  |
| 13 | (d/e)            | phase valid but `CrystalStructures[phase] >= LaueGroupEnd` → color left untouched | *Not directly tested. Low-value guard for a corrupt crystal-structure index.* |
| 14 | Validation | Referenced feature phase is outside the ensemble range | `Phase Index Bounds` — first and second feature sections |
| 15 | Validation | Invalid unreferenced phase is ignored | `Phase Index Bounds / Unreferenced Feature Phase is ignored` |
| 16 | Boundary | A 65,536-face block and one-face tail produce different colors | `genuine HDF5 65536-block tail oracle` — literal red/black and black/green sides |


## Test inventory

| Test case | Status | Notes |
|-----------|--------|-------|
| `Class 1 Oracle - mixed-phase analytical` | new-for-V&V | Hand-built 5-face cubic/hex mesh with distinct per-feature orientations; 30 color-byte + 1 invariant assertions; analytical corner-primary oracle covering all four corners. Verified to fail under both the phase-index and Euler-index mutations. |
| `Phase Index Bounds` | kept | Uses HDF5-OOC stores and checks both referenced-feature phase bounds plus the valid unreferenced-feature case. |
| `genuine HDF5 65536-block tail oracle` | new-for-V&V | Hidden OOC contract test. Uses HDF5-OOC inputs and outputs and checks exact cubic-red and hex-green RGB values at tuple 65,535 and the one-face partial tail at tuple 65,536. Passes with 38 assertions. |
| `Valid filter execution` | retired | Compared against `SurfaceMeshFaceIPFColors` baked into `6_6_Small_IN100_GBCD` — the filter's own pre-fix output (circular oracle). It encoded the bug on `feature1`-invalid boundary faces, so the fix correctly broke it. Superseded by the Class 1 analytical test; real-data exercise retained by the Color-Key test below. |
| `Invalid filter execution` | kept | Two preflight tuple-mismatch sections (`-2430`, `-2432`). |
| `ColorKey choice reaches algorithm` | kept | Runs the filter on the full Small IN100 GBCD surface mesh three times (TSL/PUCM/NH) and asserts the outputs differ — real-data smoke test independent of any baked exemplar. |
| `SIMPL Backwards Compatibility` | kept | 6.4 and 6.5 SIMPL JSON → Arguments round-trip. |

OOC recertification, 2026-09-18: serial CTest passed 5/5 in `NX-Com-Qt69-Vtk96-Rel` and 5/5 in `NX-Com-Qt69-Vtk96-OoC-Rel`. The new hidden boundary case passed 38 assertions in the OOC binary and is included in the OOC-only `OrientationAnalysisOocStoreContracts` CTest entry. The original report status and sign-off above are historical and unchanged.

## Exemplar archive

- **Archive:** None — Class 1 analytical fixtures are inlined in the test source.
- **SHA512:** n/a
- **Provenance:** No sidecar required (no archive owns the oracle). The retired circular-oracle situation is documented in `vv/provenance/ComputeFaceIPFColoringFilter-circular-oracle.md`.

## Deviations from DREAM3D 6.5.171

Three-way binary comparison (6.5.171 / local patched legacy build / SIMPLNX) on a hand-built legacy-native cubic/hex mesh, plus SIMPLNX-vs-baked on the 756,474-face real mesh.

> **Evidence (reproducible from OneDrive archive):** the A/B **fixture builder and comparison pipelines** are archived in the OneDrive verification archive (`vv_work/face_ipf/`); the measured figures below are **reproducible by re-running those generators + pipelines** against DREAM3D 6.5.171 / local patched legacy build / SIMPLNX. The rendered output snapshots and the prose `legacy_comparison_summary.md` write-up were not preserved, so the byte-level figures are reproducible rather than directly archived as result files. This A/B is **corroborating, not load-bearing**: the `phase1`→`phase2` fix is independently verified by the Class 1 analytical oracle (mutation-tested), which is the primary correctness evidence.

- `ComputeFaceIPFColoringFilter-D1` — Phase-2 face side colored with Phase-1's Laue symmetry operator (and left black on `feature1`-invalid boundary faces). The #1635 bug; 6.5.171 reproduces it; SIMPLNX and a patched local legacy build correct it. 120,000/756,474 faces affected on real data (reproducible from the archived A/B generators). See `vv/deviations/ComputeFaceIPFColoringFilter.md`.
- `ComputeFaceIPFColoringFilter-D2` — hex basal IPF hue differs between EbsdLib 3.0.0 (SIMPLNX, green) and legacy EbsdLib (6.5.171 and the patched local build, blue); a library deviation affecting all hex IPF coloring, flagged for review.

*Note:* SIMPLNX-written `.dream3d` files are, by design, not readable by legacy DREAM3D 6.5.171 (the FileVersion dataset gates this), so the legacy A/B input was authored directly in legacy format with the `legacy_dream3d` writer — the standard approach for legacy comparisons.
