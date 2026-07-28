# V&V Report: CAxisSegmentFeaturesFilter

|                            |                                                                                                                                              |
|----------------------------|----------------------------------------------------------------------------------------------------------------------------------------------|
| Plugin                     | OrientationAnalysis                                                                                                                          |
| SIMPLNX UUID               | `9fe07e17-aef1-4bf1-834c-d3a73dafc27d`                                                                                                       |
| SIMPLNX Human Name         | Segment Features (C-Axis Misalignment)                                                                                                       |
| DREAM3D 6.5.171 equivalent | `CAxisSegmentFeatures` — `Source/Plugins/Reconstruction/ReconstructionFilters/CAxisSegmentFeatures.{h,cpp}` (SIMPL UUID `bff6be19-1219-5876-8838-1574ad29d965`) |
| Verified commit            | *<filled at SBIR deliverable assembly>*                                                                                                      |
| Status                     | READY FOR REVIEW                                                                                                                             |
| Sign-off                   | pending second-engineer review                                                                                                               |

## At a glance

| Aspect                 | Current state                                                                                                                                                                                                                                                                              |
|------------------------|------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------|
| Algorithm Relationship | **Minor changes.** Line-for-line port of legacy `getSeed`/`determineGrouping` (same c-axis math, including the clamped `acos` and the π-fold), plus deliberate NX additions: crystal-structure validation, 26-neighbor scheme, uint8 masks, RectGrid input, deterministic opt-in FeatureId randomization. Three SIMPLNX defects (D1, D4, D5) found and **fixed during this V&V cycle**. |
| Oracle (confirmed)     | **Class 1 (Analytical) primary + Class 4 (Invariant) companion** — pure-Phi Bunge quats make the folded c-axis distance between cells exactly `min(\|ΔΦ\|, 180°−\|ΔΦ\|)`; expected FeatureIds derive in closed form. Encoded as 10 fixture tests in `test/CAxisSegmentFeaturesTest.cpp`; all pass. |
| Code paths enumerated  | 20 of 20 enumerated; 18 exercised (2 gaps: defensive mask-instantiation error, cancel-signal path).                                                                                                                                                                                            |
| Tests today            | 20 test cases: 8 Class 1 analytical (chain, π-fold, neighbor-scheme, mask bool/uint8, phase separation, 3D linearization, Quats-outside-cell-AM, RectGrid), 2 Class 4 invariants (randomize non-identity/determinism, masked-zero preservation), 2 exemption pins (phase-0, masked non-hex), 3 execute-error, 4 preflight-error, 1 SIMPL 6.4/6.5 conversion (DYNAMIC_SECTION). |
| Exemplar archive       | **None — fixtures inlined in the test source.** The filter's consumption of `segment_features_test_data.tar.gz` (circular oracle) is retired; the archive remains for the EBSD segmentation tests.                                                                                            |
| Legacy comparison      | **Run** (2026-07-22, rerun with TC5_3D 2026-07-24, `vv/comparisons/CAxisSegmentFeaturesFilter/`) — all 5 shared-behavior fixtures (incl. a 3×2×2 masked fixture covering the y/z stride branches) match 6.5.171 at the segmentation-partition level with identical feature counts; bit-identical ids are unattainable because 6.5.171 always clock-randomizes FeatureIds (D2). |
| Bug flags              | D1, D4, D5 — all SIMPLNX defects, all **fixed this cycle** and pinned by tests. No legacy bug flags.                                                                                                                                                                                           |
| V&V phase              | Discovery, relationship, oracle, reconciliation, algorithm review, tests (dual-build), legacy comparison, deviations, provenance, docs — complete. **Outstanding:** second-engineer review at PR (per sign-off convention).                                                                    |

## Summary

`CAxisSegmentFeaturesFilter` flood-fills neighboring cells into features when their crystallographic c-axes (hexagonal [0001] directions) are misaligned by less than a user tolerance. Verification used a Class 1 analytical oracle (pure-Phi Bunge fixtures with closed-form expected segmentations) plus Class 4 invariants; reconciliation exposed and fixed three SIMPLNX defects (unvalidated first seed in the shared driver, spurious rejection of unindexed/masked cells, RectGrid crash). Post-fix SIMPLNX matches the oracle exactly and matches DREAM3D 6.5.171 at the partition level on every shared code path; 5 deviations are documented, 3 of them fixed-this-cycle SIMPLNX bugs.

## Algorithm Relationship

*Classification:* **Minor changes**

*Evidence:* SIMPL UUID `bff6be19-1219-5876-8838-1574ad29d965` is preserved via `FromSIMPLJson` (see `test/simpl_conversion/6_*/CAxisSegmentFeaturesFilter.json`). `getSeed()` and `determineGrouping()` are line-for-line translations of the legacy methods (`CAxisSegmentFeatures.cpp` in the Reconstruction plugin), with the flood-fill driver hoisted into the shared `src/simplnx/Utilities/SegmentFeatures.cpp` (legacy used a `SegmentFeatures` base class the same way).

*Port-time deltas:*

1. **Math API.** Legacy `FOrientTransformsType::qu2om` + `MatrixMath` → NX `ebsdlib::Quaternion::toOrientationMatrix()` + Eigen. Same arithmetic (quat → OM, transpose, ·[0,0,1], normalize, clamped `acos`, `w ≤ tol || π−w ≤ tol`); the A/B run shows no observable difference.
2. **Crystal-structure validation added** (D3). Legacy computes the [001] misalignment for any Laue class; NX requires participating cells to be Hexagonal_High/Low (errors `-8363`/`-8364`). Deliberate correctness guard; changes behavior only for inputs whose legacy output was meaningless.
3. **FeatureId randomization exposed and made deterministic** (D2). Legacy always randomizes with a clock seed (not a parameter); NX exposes `Randomize Feature Ids` (default false) with a fixed-seed shuffle.
4. **26-neighbor "All Connected" scheme added** (NX-only option; default remains the legacy 6-face behavior).
5. **RectGrid geometry and uint8 masks accepted** (NX-only capability; legacy is Image + bool-mask only).
6. **D1 (fixed this cycle):** PR #1466's driver restructure made the first seed an unvalidated raw index 0 — phantom/misgrown first feature when voxel 0 could not seed. Restored `getSeed()` for the first seed (also fixes EBSD/Scalar segmentation, which share the driver).
7. **D4 (fixed this cycle):** the delta-2 validation loop originally checked phase-0 and masked-out cells (which can never participate) and indexed `CrystalStructures` without a bounds check.
8. **D5 (fixed this cycle):** stale `getDataAs<ImageGeom>` cast crashed RectGrid input; now `IGridGeometry`, matching the sibling algorithms.
9. **Review hardening (this cycle, no legacy behavior change):** preflight now rejects cell arrays whose tuple count differs from the geometry's cell count (`-652`, previously an out-of-bounds walk) and a cell AttributeMatrix smaller than the geometry (`-653`); `executeImpl` derives the FeatureIds path from the geometry's cell AttributeMatrix exactly as preflight does (previously from the Quats array's parent — a crash when Quats lived elsewhere) and creates it with that AttributeMatrix's tuple shape; a canceled run returns cleanly instead of misreporting `-87000`.

*Material PRs since baseline:* #1373 (26-neighbor option), #1444/#1498 (progress messaging), #1466 (feature-count fix; introduced D1), #1472 (EbsdLib 2.0 API), #1535 (preflight cleanup), #1501 (Vec3 consolidation).

## Oracle

*Class:* **1 (Analytical)** primary + **4 (Invariant)** companion.

*Applied:* For pure Bunge Euler angles (φ1=0, Φ, φ2=0), stored as quats `{sin(Φ/2), 0, 0, cos(Φ/2)}`, the sample-frame c-axis of a cell is `(0, ±sin Φ, cos Φ)`, so the c-axis angle between two cells is exactly `|ΦA−ΦB|` and the algorithm's folded metric is `min(|ΔΦ|, 180°−|ΔΦ|)`. Expected FeatureIds follow in closed form from per-cell Φ, the tolerance, and grid adjacency (derivation comment at `AnalyticalFixtures::QuatFromPhiDeg`). Class 4 companions: feature AM has `numFeatures+1` tuples, `Active[0]==0`, all real features active, masked/unindexed cells keep id 0, randomization preserves the partition and is a deterministic permutation of `{1..N}`.

*Encoded:* `test/CAxisSegmentFeaturesTest.cpp` — `Class 1 Analytical (Pure-Phi Chain, Face)`, `(Pi-Fold Antiparallel C-Axes)`, `(Neighbor Scheme Face vs All)` [2 sections], `(Mask Excludes Voxel 0)` [2 sections], `(Phase Separation)`, `(3D Linearization, 3x2x2)`, `(Quats Outside Cell AttributeMatrix)`, `(RectGrid Geometry)`, `Class 4 Invariants (RandomizeFeatureIds)`, `Class 4 Invariants (RandomizeFeatureIds Preserves Masked Zeros)` — all pass in both builds.

*Second-engineer review:* pending at PR review (sign-off convention). The pure-Phi c-axis derivation is sibling-shared with the previously reviewed `GroupMicroTextureRegionsFilter` / `ComputeFeatureNeighborCAxisMisalignmentsFilter` Class 1 oracles.

## Code path coverage

*20 of 20 paths enumerated; 18 exercised.*

Source: `src/Plugins/OrientationAnalysis/src/OrientationAnalysis/Filters/Algorithms/CAxisSegmentFeatures.cpp` (207 lines) + shared driver `src/simplnx/Utilities/SegmentFeatures.cpp` (execute loop).

Logical phases: **(a) init + validation** in `operator()`, **(b) flood-fill driver** in `SegmentFeatures::execute`, **(c) seeding** in `getSeed`, **(d) grouping decision** in `determineGrouping`, **(e) finalize** in `operator()`.

| #  | Phase | Path                                                                                       | Test case                                                                              |
|----|-------|--------------------------------------------------------------------------------------------|-----------------------------------------------------------------------------------------|
| 1  | (a)   | Mask array missing/wrong type at execute → error `-8362`                                    | *Not directly tested. Defensive guard; `ArraySelectionParameter` validates the path through the normal IFilter API.* |
| 2  | (a)   | Validation skips phase ≤ 0 cells                                                            | `Phase 0 (Unindexed) Cells Tolerated`                                                     |
| 3  | (a)   | Validation skips masked-out cells                                                           | `Masked Non-Hexagonal Cells Tolerated`                                                    |
| 4  | (a)   | Phase value ≥ CrystalStructures tuples → error `-8364`                                      | `Execute Error - Phase Out of Ensemble Bounds (-8364)`                                    |
| 5  | (a)   | Participating non-hex cell → error `-8363`                                                  | `Execute Error - Non-Hexagonal Crystal Structure (-8363)`                                 |
| 6  | (a)   | Hexagonal_High and Hexagonal_Low both accepted                                              | All Class 1 tests (High); `Class 1 Analytical (Phase Separation)` (Low, ensemble 2)       |
| 7  | (a)   | Geometry fetched as `IGridGeometry` (Image and RectGrid)                                    | All tests (Image); `Class 1 Analytical (RectGrid Geometry)` (RectGrid)                    |
| 8  | (b)   | First seed obtained via `getSeed` (D1 pin)                                                  | `Class 1 Analytical (Mask Excludes Voxel 0)`; `Execute Error - No Features Found (-87000)` |
| 9  | (b)   | Face (6-neighbor) scheme, incl. y-/z-stride branches and the x-fastest linearization        | `Class 1 Analytical (Pure-Phi Chain, Face)`; `Class 1 Analytical (3D Linearization, 3x2x2)` (axis-asymmetric Phi field kills any dims-permutation mutant) |
| 10 | (b)   | All-connected (26-neighbor) scheme                                                          | `Class 1 Analytical (Neighbor Scheme Face vs All)` — "All Connected Neighbors" section    |
| 11 | (b)   | Cancel requested → early return                                                             | *Not directly tested. Requires cancel-signal injection; low-value gap.*                   |
| 12 | (c)   | Seed scan skips owned / masked / phase ≤ 0 cells                                            | `Mask Excludes Voxel 0`; `Phase 0 (Unindexed) Cells Tolerated`                            |
| 13 | (c)   | Seed found → stamp FeatureId (feature AM resized once, post-run, in phase (e))              | Every successful Class 1 test (`CheckActiveArray` tuple counts)                           |
| 14 | (c)   | No seed remains → return −1, driver exits                                                   | Every test (loop termination); immediately in `No Features Found (-87000)`                |
| 15 | (d)   | Neighbor already owned or masked-out → reject                                               | Chain fixtures (burst revisits owned neighbors); `Mask Excludes Voxel 0` (cells 0, 3)     |
| 16 | (d)   | Phases differ → reject                                                                      | `Class 1 Analytical (Phase Separation)`; `Phase 0 (Unindexed) Cells Tolerated`            |
| 17 | (d)   | `w ≤ tol` → group                                                                           | `Pure-Phi Chain` (Δ = 3°, 4°, 5° pairs)                                                   |
| 18 | (d)   | `π − w ≤ tol` → group (antiparallel c-axes)                                                 | `Class 1 Analytical (Pi-Fold Antiparallel C-Axes)`                                        |
| 19 | (e)   | `foundFeatures < 1` → error `-87000`; else AM resize, Active refill, `Active[0]=0`          | `No Features Found (-87000)` (error); `CheckActiveArray` in every passing test (success)  |
| 20 | (e)   | `RandomizeFeatureIds == true` → deterministic, non-identity shuffle preserving id 0         | `Class 4 Invariants (RandomizeFeatureIds)` (non-identity + determinism); `Class 4 Invariants (RandomizeFeatureIds Preserves Masked Zeros)` |

Filter-level (preflight) paths — tolerance == 0 → `-655`, cell-array tuple mismatch → `-651`, cell arrays vs geometry cell count → `-652`, cell AttributeMatrix vs geometry cell count → `-653` — are covered by the four `Preflight Error` tests; SIMPL 6.4/6.5 argument conversion by `SIMPL Backwards Compatibility`. The cancel early-return added to `operator()` (clean return instead of `-87000` on cancel) shares path 11's not-directly-tested status.

## Test inventory

| Test case | Status | Notes |
|-----------|--------|-------|
| `Class 1 Analytical (Pure-Phi Chain, Face)` | new-for-V&V | 8-cell chain, 4 features; 8 element-wise FeatureId assertions + Active invariants. |
| `Class 1 Analytical (Pi-Fold Antiparallel C-Axes)` | new-for-V&V | Exercises the `(π − w) ≤ tol` branch (Φ=2° vs 176° → folded 6°). |
| `Class 1 Analytical (Neighbor Scheme Face vs All)` | new-for-V&V | DYNAMIC_SECTION over both schemes on a 2×2×1 diagonal fixture; Face → 4 features, All → 3. |
| `Class 1 Analytical (Mask Excludes Voxel 0)` | new-for-V&V | DYNAMIC_SECTION over bool + uint8 masks; **D1 regression pin** (pre-fix: phantom feature + shifted ids). |
| `Class 1 Analytical (Phase Separation)` | new-for-V&V | Identical orientations split at a phase boundary; also covers Hexagonal_Low acceptance. |
| `Class 1 Analytical (3D Linearization, 3x2x2)` | new-for-V&V | Pins the x-fastest linearization + y/z stride branches with an axis-asymmetric Phi field (added after adversarial review showed the original fixtures were invariant under dims permutation). |
| `Class 1 Analytical (Quats Outside Cell AttributeMatrix)` | new-for-V&V | Pins the preflight/execute FeatureIds path agreement when Quats lives in a sibling AttributeMatrix (pre-fix: execute dereferenced a null FeatureIds array). |
| `Class 1 Analytical (RectGrid Geometry)` | new-for-V&V | **D5 regression pin** (pre-fix: null-pointer crash on RectGrid input). |
| `Class 4 Invariants (RandomizeFeatureIds)` | new-for-V&V | Partition preservation, permutation of {1..4}, non-identity vs canonical labeling (kills a dead randomizer), same-seed determinism across two runs. |
| `Class 4 Invariants (RandomizeFeatureIds Preserves Masked Zeros)` | new-for-V&V | Masked cells keep FeatureId 0 through the shuffle (pins the 0→0 mapping guarantee). |
| `Phase 0 (Unindexed) Cells Tolerated` | new-for-V&V | **D4 regression pin** (pre-fix: spurious `-8363` on the 999 sentinel). |
| `Masked Non-Hexagonal Cells Tolerated` | new-for-V&V | **D4 regression pin** (masked cubic phase must not be validated). |
| `Execute Error - Non-Hexagonal Crystal Structure (-8363)` | new-for-V&V | Unmasked cubic cells rejected. |
| `Execute Error - Phase Out of Ensemble Bounds (-8364)` | new-for-V&V | **D4 regression pin** (pre-fix: out-of-bounds read). |
| `Execute Error - No Features Found (-87000)` | new-for-V&V | All cells masked; **D1 regression pin** (pre-fix: success with 1 phantom feature). |
| `Preflight Error - Zero Tolerance (-655)` | new-for-V&V | Preflight rejects tolerance == 0. |
| `Preflight Error - Cell Arrays Smaller Than Geometry (-652)` | new-for-V&V | Cell arrays consistent with each other but not with the geometry's cell count (pre-fix: out-of-bounds walk at execute). |
| `Preflight Error - Cell AttributeMatrix Smaller Than Geometry (-653)` | new-for-V&V | Cell arrays match the geometry but the FeatureIds-hosting AttributeMatrix does not. |
| `Preflight Error - Cell array tuple count mismatch (-651)` | kept | Synthetic tuple-count mismatch between Quats and Phases. |
| `SIMPL Backwards Compatibility` | kept | DYNAMIC_SECTION over SIMPL 6.4 + 6.5 conversion fixtures; UUID + argument-key conversion only. |
| *(retired)* `CAxisSegmentFeatures:Face` / `:All` / `:MaskFace` / `:MaskAll` | retired | Consumed the `segment_features_test_data.tar.gz` exemplar whose `CAxis_FeatureIds_*` arrays were generated from SIMPLNX output — a circular oracle (see provenance sidecar). Replaced by the Class 1 fixtures, which cover the same scheme × mask parameter cube with independent expected output. |

All non-retired tests pass at the verified commit in **both** builds: in-core `NX-Com-Qt69-Vtk95-Rel` and OOC `simplnx-ooc-Rel` (20/20 each, 2026-07-24). The shared-driver fix (D1) was regression-checked against the full `EBSDSegmentFeatures` (8/8) and `ScalarSegmentFeatures` (5/5) suites in both builds, including per-sibling masked-voxel-0, all-cells-masked (`-87000`), and periodic-boundary-wrap regression pins.

## Exemplar archive

- **Archive:** None — fixtures inlined in `test/CAxisSegmentFeaturesTest.cpp` (namespace `AnalyticalFixtures`).
- **SHA512:** N/A
- **Provenance:** `src/Plugins/OrientationAnalysis/vv/provenance/CAxisSegmentFeaturesFilter.md` (documents the retired circular-oracle archive consumption).

## Deviations from DREAM3D 6.5.171

Comparison run 2026-07-22, extended with a 3-D fixture and fully rerun 2026-07-24, on five pure-Phi fixtures (chain, π-fold, bool mask, phase-0, and a 3×2×2 masked fixture covering the y/z stride branches) through the official 6.5.171 PipelineRunner and nxrunner from one shared legacy-format input (`vv/comparisons/CAxisSegmentFeaturesFilter/`). All five match at the segmentation-partition level with identical feature counts.

- `CAxisSegmentFeaturesFilter-D1` — unvalidated first seed in SIMPLNX (post-#1466, pre-fix) could add a phantom feature or grow from a masked voxel — **fixed this cycle** — see `vv/deviations/CAxisSegmentFeaturesFilter.md`
- `CAxisSegmentFeaturesFilter-D2` — 6.5.171 always clock-randomizes FeatureIds (irreproducible); SIMPLNX is deterministic with opt-in randomization — see `vv/deviations/CAxisSegmentFeaturesFilter.md`
- `CAxisSegmentFeaturesFilter-D3` — SIMPLNX rejects non-hexagonal participating cells (`-8363`/`-8364`); legacy silently produced meaningless output — see `vv/deviations/CAxisSegmentFeaturesFilter.md`
- `CAxisSegmentFeaturesFilter-D4` — pre-fix SIMPLNX spuriously rejected unindexed/masked cells — **fixed this cycle** — see `vv/deviations/CAxisSegmentFeaturesFilter.md`
- `CAxisSegmentFeaturesFilter-D5` — pre-fix SIMPLNX crashed on RectGrid input — **fixed this cycle** — see `vv/deviations/CAxisSegmentFeaturesFilter.md`
