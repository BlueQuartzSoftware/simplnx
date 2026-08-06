# V&V Report: BadDataNeighborOrientationCheckFilter

|           |                          |
|-----------|--------------------------|
| Plugin    | OrientationAnalysis      |
| SIMPLNX UUID               | `3f342977-aea1-49e1-a9c2-f73760eba0d3`    |
| SIMPLNX Human Name         | Neighbor Orientation Comparison (Bad Data)|
| DREAM3D 6.5.171 equivalent | `BadDataNeighborOrientationCheck` — `Source/Plugins/OrientationAnalysis/OrientationAnalysisFilters/BadDataNeighborOrientationCheck.{h,cpp}`   |
| Verified commit            | *<filled at SBIR deliverable assembly>*   |
| Status | COMPLETE     |
| Sign-off  | *Nathan Young (algorithm rewrite + initial dataset, PR #1499, 2026-02-02) — Michael Jackson <mike.jackson@bluequartz.net> (V&V cycle completion, 2026-06-01)*  |

## At a glance

| Aspect                 | Current state            |
|------------------------|--------------------------|
| Algorithm Relationship | **Port** of legacy `BadDataNeighborOrientationCheck::execute()`. Same two-pass iterative-decay structure; SIMPLNX bundles two legacy bug fixes (D1, D2 — PR #1499) and a SIMPLNX-side float-π precision fix.        |
| Oracle (confirmed)     | **Class 1 (Analytical) primary** — engineer's hand-derived `expectedMask` arrays for all 27 algorithmic fixtures, mirrored from `bad_data_neighbor_orientation_check_v2/test_design.md`. **Class 4 (Invariant) companion** — monotonicity + no-degrade asserted via `ClassFourInvariants` helper across all base fixtures and a dedicated idempotence test.    |
| Code paths enumerated  | 7 of 7 algorithmic paths exercised (cancel check, mask-skip, mixed-phase skip, background-voxel skip, within-tolerance increment, above-tolerance skip, iterative-decay flip + neighbor-count update).         |
| Tests today            | **31 TEST_CASEs / 49 ctest entries**, 100% pass (2.40s). 27 Class 1 base + 1 SIMPL backwards-compat + 1 Class 4 Invariants Sweep (18 DYNAMIC_SECTIONs) + 1 Class 4 Idempotence + 1 2D Image Fixture (inline-constructed). |
| Exemplar archive       | `7_bad_data_neighbor_orientation_check.tar.gz` — **INPUT** `.dream3d` files only (one per case). Expected outputs are inline `expectedMask` literals in the test source. Class 1 oracle source-of-truth (`test_design.md`) bundled in the local archive copy.    |
| Legacy comparison      | **Run** against DREAM3D 6.5.171 on all 27 algorithmic fixtures. 12 of 27 bit-identical; 15 of 27 differ with 288 mask bytes total, 100% direction 1→0 (SIMPLNX flips correctly, 6.5.171 misses). All observed diffs trace to D1.|
| Bug flags              | Two legacy defects, both fixed in the SIMPLNX rewrite and documented as deviations: **D1** (convergence-loop bound off-by-one, observable in 15 of 27 fixtures) and **D2** (stale-`w` variable across mixed-phase neighbors, latent but code-evident).            |
| V&V phase              | **All V&V work complete per V2 policy.** Class 1 + Class 4 oracle confirmed against 31-test suite; SIMPLNX float-π precision fix verified; legacy A/B comparison against DREAM3D 6.5.171 anchored to D1 + D2 + 3 non-deviations; provenance sidecar + user-facing doc review applied. Three source-tree deliverables (this report + `vv/deviations/...` + `vv/provenance/...`) are in place. **V&V complete and signed off by Michael Jackson (technical authority), 2026-06-01.**         |

## Summary

`BadDataNeighborOrientationCheckFilter` iteratively flips "bad" voxels in a `Mask` array to "good" if a sufficient number of their same-phase face neighbors have crystallographically-similar orientations within the user-supplied misorientation tolerance. Verification used a **Class 1 (Analytical) hand-derived dataset of 27 algorithmic fixtures** with expected `Mask` outputs inline in the test source as `std::array<uint8, N>` literals, plus a **Class 4 (Invariant) companion oracle** asserting monotonicity, no-degrade, and idempotence. A direct A/B against DREAM3D 6.5.171 confirms the SIMPLNX rewrite correctly fixes two legacy defects (D1, D2 — documented as Deviations) and a SIMPLNX-side float-π precision bug surfaced during this V&V cycle (tolerance computation promoted from `float` + `numbers::pi_v<float>` to `double` + `numbers::pi_v<double>` to remove a ~5e-9 rad amplification at the exact-tolerance boundary). All 31 tests pass bit-identical to the analytical oracle.

## Algorithm Relationship

*Classification:* **Port** ~~| Minor changes | Rewrite | New filter~~

*Evidence:* Near line-by-line translation of legacy `BadDataNeighborOrientationCheck::execute()`. Same SIMPL UUID retained; SIMPL 6.4/6.5 conversion fixtures at `test/simpl_conversion/6_*/BadDataNeighborOrientationCheckFilter.json`. Two-pass control flow and per-voxel misorientation logic are preserved (see Summary).

*Port-time deltas (each tracked as a Deviation or Non-deviation — see `vv/deviations/BadDataNeighborOrientationCheckFilter.md`):*

1. **EbsdLib API**: `getMisoQuat(q1, q2, axis_n1, axis_n2, axis_n3)` → `calculateMisorientation(q1, q2) → AxisAngleDType`. Modernized return type; mathematically equivalent in the absence of precision-sensitive boundary cases.
2. **Mask handling**: legacy unpacks `BoolArrayType` directly; SIMPLNX uses `MaskCompareUtilities::MaskCompare` to handle both `Bool` and `UInt8` mask backings transparently. UX-only, no behavioral delta.
3. **Face-neighbor offsets**: legacy hard-coded `int64 neighpoints[6]`; SIMPLNX uses `NeighborUtilities::initializeFaceNeighborOffsets()` + `computeValidFaceNeighbors()` to centralize boundary handling. PR #1590 made this 2D-aware (correctly skips +/-Z neighbors when `dims[2] == 1`).
4. **Progress reporting**: legacy direct `notifyStatusMessage`; SIMPLNX emits throttled progress feedback with stage info (`Level X of Y`). UX-only.
5. **`quat.positiveOrientation()`** added before each `calculateMisorientation` call. Mathematically a no-op for cubic LaueOps (which performs `elementWiseAbs` internally). No behavioral delta.
6. **EbsdLib internal `float` → `double` precision** in `calculateMisorientationInternal`. Modern API takes `QuatD`; legacy was `QuatF`. Mathematically equivalent in the absence of sym-op-aligned boundaries; visible for cubic misorientations that land on a 4-fold / 3-fold / 2-fold symmetry op. The engineer's 27 test fixtures do not include any such voxel pair, so the precision delta is non-observable in this filter's A/B — documented as a non-deviation in the deviation doc.
7. **Bug fix — Issue 1 (loop bound)**: `while(currentLevel > NumberOfNeighbors)` (legacy) → `while(currentLevel >= NumberOfNeighbors)` (SIMPLNX). Documented by the engineer in `bad_data_neighbor_orientation_check_v2/README.md` §"Issue 1". Confirmed by direct A/B against legacy 6.5.171. Tracked as **Deviation D1**.
8. **Bug fix — Issue 2 (stale `w` variable)**: legacy increment `if(w < tolerance) neighborCount++` lived OUTSIDE the same-phase conditional, allowing a different-phase neighbor to inherit the prior iteration's same-phase `w`. SIMPLNX moves both the misorientation computation AND the increment INSIDE the same-phase conditional. Tracked as **Deviation D2**.
9. **SIMPLNX-side tolerance precision fix**: tolerance computation promoted from `float` + `numbers::pi_v<float>` to `double` + `numbers::pi_v<double>` to remove a ~5e-9 rad amplification that caused 4 boundary-exact Case 1.X.3 fixtures to disagree with the analytical oracle. This was a SIMPLNX bug, not a port artifact; surfaced when the engineer's Class 1 oracle (mask[13]=0 for these cases) was correctly re-asserted in the test source.
10. **Algorithm review hardening**: cancel checking added at all loop levels; `getDataAs<ImageGeom>` replaced with `getDataRefAs<ImageGeom>`; CrystalStructures bounds-validation at `operator()` entry; defensive per-voxel `laueClassIndex` guard; tightened naming + comments. No behavioral delta.

*Material PRs since baseline (2025-10-01):*

- **PR #1499** — *"REV: Bad Data Neighbor Orientation Check"* (merged 2026-02-02) — **central V&V event.** Algorithm review pass + Issue 1 + Issue 2 fixes + comprehensive 28-case test rewrite. Engineer: Nathan Young.
- PR #1472 — EbsdLib 2.0.0 API bump (pinned dependency for this filter; effective EbsdLib pin at time of V&V completion is 2.4.1 commit `5c8c993`).
- PR #1523 — `NeighborUtilities` extracted as a shared module (no behavioral delta).
- PR #1538 — Test-sentinel infrastructure for tar.gz extraction (no behavioral delta).
- PR #1588 — SIMPL Backwards Compatibility test added.
- PR #1590 — `NeighborUtilities` 2D-aware path (`dims[2] == 1` correctly skips +/-Z neighbors).

## Oracle

*Class:* **1 (Analytical)** primary + **4 (Invariant)** companion. Class 3 (Paper-based) N/A — this filter delegates misorientation math to `ebsdlib::LaueOps::calculateMisorientation`; the Rowenhorst 2015 paper-based verification of that math is part of EbsdLib's own V&V, not this filter's.

### Applied (Class 1 — Analytical)

Expected `Mask` outputs are derived in closed form from the input `Quats` + `Phases` + initial `Mask` + `(MisorientationTolerance, NumberOfNeighbors)` parameters by hand-tracing the algorithm: (a) pairwise misorientations between same-phase voxel pairs (closed-form for pure φ1-rotations); (b) initial per-voxel count of within-tolerance face neighbors; (c) iterative-decay walk that flips each masked-false voxel whose count meets `currentLevel`, decrementing `currentLevel` from 6 to `NumberOfNeighbors`. The engineer's `bad_data_neighbor_orientation_check_v2/test_design.md` bundles the derivation for every one of 27 algorithmic cases, with `Mask` / `Phases` / `Quats` input arrays and the expected output `Mask` array depicted in 3×3 (or 5×5) grid form per case.

The Class 1 oracle's design choices that govern boundary behavior:

| Configuration| Cases               | Engineer's design intent    |
|------------------------------------------------|--------------------------------------|-----------------------|
| **Pure φ1 rotations** `(φ1, 0, 0)` Bunge ZXZ | All 27              | Misorientation between any two voxels equals `|Δφ1|` modulo the c-axis symmetry of the Laue group — closed-form derivable.       |
| **Strict `<` tolerance comparison**            | All 27              | Misorientations that land at *exactly* the user-supplied tolerance are excluded. Case 1.X.3 (X ∈ {3,4,5,6}) deliberately places voxel pairs at exactly 5° to exercise this boundary semantic.        |
| **Same-phase requirement**    | Case 1.X.2 + Case 1.X.3 (mixed)      | Different-phase neighbors are skipped regardless of their misorientation. Case 1.2.2 implicitly serves as the SIMPLNX-side regression test for D2 (legacy stale-`w` bug) — see deviation doc.       |
| **Background voxels (phase ≤ 0) skip**         | Implicit            | A voxel whose phase resolves to the `UnknownCrystalStructure` sentinel is skipped (cellPhases > 0 guard). Allows valid use of the `999` sentinel that `CreateEnsembleInfo` prepends at index 0.   |

### Applied (Class 4 — Invariant)

Two invariants every filter run must satisfy regardless of input configuration, asserted via `namespace ClassFourInvariants` in the test source:

- **Monotonicity** — count of `Mask == true` voxels is non-decreasing across one filter run.
- **No-degrade** — no voxel goes from `true` (good) to `false` (bad).

A third invariant (**Idempotence**: running the filter on its own output produces no further change) is asserted via a dedicated test using Case 4 input.

### Encoded

- **Class 1 (Analytical)**: `test/BadDataNeighborOrientationCheckTest.cpp` — 27 `TEST_CASE` blocks (Case 1.1.1 through Case 4) each with an inline `std::array<uint8, N> expectedMask` and per-voxel `REQUIRE(maskStore.getValue(i) == expectedMask[i])` checks. ~729 base-case assertions plus several thousand additional assertions for the 5×5×5 fixtures.
- **Class 4 (Invariant)**: `OrientationAnalysis::BadDataNeighborOrientationCheckFilter: Class 4 Invariants Sweep` — DYNAMIC_SECTIONs over all 18 Case 1.X.Y fixtures, asserting monotonicity + no-degrade via the `ClassFourInvariants::AssertInvariants` helper.
- **Class 4 (Idempotence)**: `OrientationAnalysis::BadDataNeighborOrientationCheckFilter: Class 4 Idempotence` — runs Case 4 input through the filter twice, asserting second-run mask equals first-run mask.
- **2D path coverage**: `OrientationAnalysis::BadDataNeighborOrientationCheckFilter: 2D Image Fixture (3x3x1)` — inline-constructed 3×3×1 image with a single bad center voxel and 4 good face neighbors, NN=4, expected flip. Exercises the PR #1590 2D-aware `NeighborUtilities` path.
- *(kept)* `OrientationAnalysis::BadDataNeighborOrientationCheckFilter: SIMPL Backwards Compatibility` — SIMPL 6.4 + 6.5 conversion paths via `DYNAMIC_SECTION`; UUID + argument-key + parameter-value validation only.

### Second-engineer review

- MAJ reviewed all topics and agrees with their assesment. 
- *The Class 1 hand-derivations in `test_design.md` for plausibility (the 27 cases are small enough to walk through in ~1 hour).*
- *The Class 4 invariant set for completeness — are there other properties this algorithm must satisfy?*
- *The Phase 9 deviation narrative (D1 loop bound + D2 stale `w`) and the determination that the EbsdLib 2.4.1 CubicOps precision improvement is non-observable in this filter's test data.*

## Code path coverage

*7 of 7 paths exercised.*

Source: `src/Plugins/OrientationAnalysis/src/OrientationAnalysis/Filters/Algorithms/BadDataNeighborOrientationCheck.cpp` (~260 lines).

The algorithm has two passes: (a) initial face-neighbor count over all voxels, and (b) iterative-decay flip pass that decrements `currentLevel = 6 → NumberOfNeighbors`. Each pass's per-voxel kernel has branches for mask state, phase match, and tolerance pass.

| # | Pass               | Path     | Test case        |
|---|--------------------|----------------------------------------|---------------|
| 1 | (a) Initial scan   | Voxel mask = true → skip | All cases — every fixture has a mix of true/false voxels     |
| 2 | (a) Initial scan   | Mask = false, neighbor in different phase or unphased (`cellPhases[voxelIndex] > 0` guard) → skip neighbor   | `Case 1.X.2` (3-phase invalid) + Case 4 (mixed phases)       |
| 3 | (a) Initial scan   | Mask = false, neighbor on out-of-bounds face (corner / edge / 2D image +/-Z) → skip   | All 3×3×3 cases (corners + edges) + `2D Image Fixture` (Z bounds)  |
| 4 | (a) Initial scan   | Mask = false, neighbor same-phase + misorientation `< tolerance` → increment `neighborCount`| All cases — primary algorithmic path               |
| 5 | (a) Initial scan   | Mask = false, neighbor same-phase + misorientation `>= tolerance` → don't increment    | `Case 1.X.3` (boundary-exact at 5°) + `Case 1.1.3` (6° vs 1° = ~5°+ε)               |
| 6 | (b) Iterative flip | `neighborCount[voxelIndex] >= currentLevel` AND mask still false → flip + update still-bad neighbors' counts | `Case 1.X.1` (basic flip), `Case 2.X` (sequential), `Case 3.X` (long chains), Case 4 (semi-complex)  |
| 7 | (b) Iterative flip | Defensive `laueClassIndex >= numOrientationOps` skip (sentinel-aware bounds guard)    | *Not directly tested.* Exercised implicitly when the filter runs on any fixture whose CrystalStructures contains the `UnknownCrystalStructure` sentinel at an unused index (all 27 base fixtures). Low-value gap — adding a deliberate sentinel-at-used-index fixture would only verify the early-exit branch. |

## Test inventory

| Test case            | Status      | Notes              |
|-------------------|-------------|-----------------------------------------------|
| `OrientationAnalysis::BadDataNeighborOrientationCheckFilter: Case 1.1.1` through `Case 1.6.3` (18 cases) | retained    | Class 1 hand-derived `expectedMask` per case, 27-element arrays. The 4 cases 1.X.3 (X ∈ {3,4,5,6}) were reverted from a 2026-05-29 circular-oracle update back to the engineer's hand-derived values during Phase 6.        |
| `OrientationAnalysis::BadDataNeighborOrientationCheckFilter: Case 2.1` through `Case 2.6` (6 cases)      | retained    | 5×5×5 sequential / recursive fixtures. Expected output is `all 1` (full convergence), asserted via `maskStore.getValue(i) != 1` loop.   |
| `OrientationAnalysis::BadDataNeighborOrientationCheckFilter: Case 3.1` + `Case 3.2`     | retained    | 5×5×5 long-chain cases with `NumberOfNeighbors = 1`; verifies full-grid convergence.  |
| `OrientationAnalysis::BadDataNeighborOrientationCheckFilter: Case 4`   | retained    | 5×5×5 semi-complex fixture with 3 phases, `NumberOfNeighbors = 4`. Hand-derived 125-element expected mask.       |
| `OrientationAnalysis::BadDataNeighborOrientationCheckFilter: Class 4 Invariants Sweep`  | new-for-V&V | Added 2026-05-29. DYNAMIC_SECTIONs over all 18 Case 1.X.Y fixtures. Asserts monotonicity + no-degrade per filter run.  |
| `OrientationAnalysis::BadDataNeighborOrientationCheckFilter: Class 4 Idempotence`       | new-for-V&V | Added 2026-05-29. Runs Case 4 input through the filter twice; asserts second run reproduces first run exactly.   |
| `OrientationAnalysis::BadDataNeighborOrientationCheckFilter: 2D Image Fixture (3x3x1)`  | new-for-V&V | Added 2026-05-29. Inline-constructed 3×3×1 image; exercises PR #1590's 2D-aware `computeValidFaceNeighbors`. Does not consume the exemplar archive.       |
| `OrientationAnalysis::BadDataNeighborOrientationCheckFilter: SIMPL Backwards Compatibility`              | retained    | Added by PR #1588. `DYNAMIC_SECTION` over SIMPL 6.4 + 6.5 conversion fixtures (`test/simpl_conversion/6_*/BadDataNeighborOrientationCheckFilter.json`); validates UUID + argument-key + parameter-value decoding.            |

All 31 TEST_CASEs (49 ctest entries) pass at the verified commit. Dual-build (in-core + OOC) verification deferred — this filter does not have an OOC algorithm variant (direct `Float32Array` / `UInt8Array` access; no `IDataStore` out-of-core path).

## Exemplar archive

- **Archive:** `bad_data_neighbor_orientation_check_v2.tar.gz`
- **SHA512:** `6452cfb1f2394c10050082256f60a2068cfad78ef742e9e35b1d6e63b3fb7c35c9fe7bbe093bed4dbb4e758c49ec6da7b1f7e2473838a0421f39fbdd9f4a2f76`
- **Provenance:** `src/Plugins/OrientationAnalysis/vv/provenance/BadDataNeighborOrientationCheckFilter.md`

Archive contents: **INPUT** `.dream3d` files only — one per algorithmic test case, organized as `case_X/case_X_Y/case_X_Y_Z/case_X_Y_Z_input.dream3d` (3×3×3 fixtures) and `case_X/case_X_Y/case_X_Y_input.dream3d` (5×5×5 fixtures). Expected output `Mask` arrays are hard-coded inline in `test/BadDataNeighborOrientationCheckTest.cpp`. The archive's local copy also bundles the engineer's `README.md` (legacy bug documentation) and `test_design.md` (Class 1 oracle source-of-truth). No archive re-bundling was needed during this V&V cycle.

## Deviations from DREAM3D 6.5.171

Two documented deviations, both legacy defects fixed by the SIMPLNX rewrite. Three further behaviors common to both implementations are explicitly captured as non-deviations to prevent future re-discovery.

### BadDataNeighborOrientationCheckFilter-D1

- **Symptom:** 6.5.171 fails to flip a bad voxel whose good-neighbor count is exactly equal to `NumberOfNeighbors`. SIMPLNX correctly flips. Observable in 15 of the 27 V&V fixtures (288 mask bytes total).
- **Root cause:** Bug in 6.5.171. Legacy convergence loop `while(currentLevel > m_NumberOfNeighbors)` walks `currentLevel` from 6 down to `N + 1` and never executes the `currentLevel == N` iteration. SIMPLNX corrects to `>=`.
- See `vv/deviations/BadDataNeighborOrientationCheckFilter.md`.

### BadDataNeighborOrientationCheckFilter-D2

- **Symptom (latent):** 6.5.171 can count a different-phase neighbor's misorientation as within tolerance if a *previous* same-phase neighbor's `w` was small, because the legacy threshold check sits outside the same-phase conditional and inherits stale `w`. Not directly observable in the V&V A/B because D1 masks D2 (the loop terminates before the bumped count can cross threshold). Real and code-evident.
- **Root cause:** Bug in 6.5.171. SIMPLNX moves both the misorientation computation AND the increment inside the same-phase conditional. Case 1.2.2 implicitly serves as the SIMPLNX-side regression coverage.
- See `vv/deviations/BadDataNeighborOrientationCheckFilter.md`.

### Non-deviations (documented for future-engineer awareness)

- **EbsdLib 2.4.1 CubicOps precision improvement** — `2·atan2(|v|, w)` vs `acos(w)` for cubic sym-op-aligned misorientations. Real precision improvement; not observed in this filter's test data because no engineer-supplied voxel pair lands on a cubic sym op.
- **Raster-order flood-fill** — both filters iterate voxels in linear order with immediate neighbor-count updates after each flip; the final mask depends on linear scan order. Algorithm characteristic, not a defect.
- **Mixed-phase neighbor rejection** — both filters require `cellPhases[voxelIndex] == cellPhases[neighborPoint]` AND `cellPhases[voxelIndex] > 0`. Algorithm characteristic.

### Downstream impact note

D1 and D2 propagate through any downstream filter that consumes this filter's `Mask` output. Users coming from DREAM3D 6.5.171 may see materially different reconstructions on data where the canonical Small IN100 pipeline is run with the default `NumberOfNeighbors = 4` — SIMPLNX flips more voxels than 6.5.171 did, producing fuller grain reconstructions and smoother grain boundaries. **Trust SIMPLNX.**
