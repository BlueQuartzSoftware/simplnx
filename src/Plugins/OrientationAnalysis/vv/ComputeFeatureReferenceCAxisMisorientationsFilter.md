# V&V Report: ComputeFeatureReferenceCAxisMisorientationsFilter

|           |                          |
|-----------|--------------------------|
| Plugin    | OrientationAnalysis      |
| SIMPLNX UUID | `16c487d2-8f99-4fb5-a4df-d3f70a8e6b25` |
| DREAM3D 6.5.171 equivalent | `FindFeatureReferenceCAxisMisorientations` (SIMPL UUID `1a0848da-2edd-52c0-b111-62a4dc6d2886`) — `DREAM3D/Source/Plugins/OrientationAnalysis/OrientationAnalysisFilters/FindFeatureReferenceCAxisMisorientations.cpp` |
| Verified commit | *<filled at SBIR deliverable assembly>* |
| Status | COMPLETE     |
| Sign-off | *Michael Jackson <mike.jackson@bluequartz.net> — 2026-06-11* |

## At a glance

| Aspect                 | Current state            |
|------------------------|--------------------------|
| Algorithm Relationship | **Port with Minor Changes** — same per-cell c-axis projection + per-feature avg/stddev finalize as legacy `FindFeatureReferenceCAxisMisorientations`. PR #1438 added the `isHex` gate (skip non-hex cells, closes legacy D1 bug) + Eigen-based math (closes D4 precision drift); PR #1472 swapped EbsdLib 2.0 API; PR #1582 added 4 cancel checks. |
| Oracle (confirmed)     | **Class 1 (Analytical)** — closed-form `|Φ_cell − Φ_avg|` reduction on pure-Φ Bunge ZXZ `(0, Φ, 0)` rotations. **Class 4 (Invariant)** companion — 6 invariants incl. I6 NaN-on-empty-feature (load-bearing). 4 fixtures in `test/ComputeFeatureReferenceCAxisMisorientationsTest.cpp` under the `AnalyticalFixtures` namespace; all pass. |
| Code paths enumerated  | **7 of 8 exercised** by V&V fixtures. Cancellation (path 8) is structurally present + reviewed at 4 sites but not exercised by an injected cancel signal. |
| Tests today            | **6 TEST_CASEs**, 100% pass: 4 new Class 1 + Class 4 inlined fixtures + 1 kept `InValid Filter Execution` + 1 kept `SIMPL Backwards Compatibility`. Restructured 3 → 6 (1 retired exemplar consumer, 2 kept verbatim, 4 new). |
| Exemplar archive       | **None — inlined.** Pre-V&V `Valid Filter Execution` exemplar consumer retired 2026-06-10 as a circular oracle. `caxis_data.tar.gz` archive download retained in `test/CMakeLists.txt` (shared with `ComputeCAxisLocationsTest`). Provenance sidecar at `vv/provenance/ComputeFeatureReferenceCAxisMisorientationsFilter.md`. |
| Legacy comparison      | **Run — SIMPLNX vs DREAM3D 6.5.171** on the Realistic Microstructure fixture, 2026-06-10. Two documented deviations vs 6.5.171 (D1 + D4); each root cause was proven by applying the corresponding surgical fixes (Eigen + isHex skip; double-precision stddev accumulation) to a local build of the legacy source, after which the legacy output became byte-for-byte identical to SIMPLNX across all 3 output arrays. |
| Bug flags              | None in SIMPLNX. **D1 is a legacy bug in 6.5.171** (non-hex cells fall through the validity gate and produce garbage c-axis projections + non-NaN per-feature avg for all-non-hex features). SIMPLNX has the fix since PR #1438. |
| V&V phase              | **COMPLETE.** Class 1 + Class 4 oracles confirmed against 6-test suite; circular-oracle consumer retired; three-way empirical A/B against legacy completed. Three source-tree deliverables (this report + `vv/deviations/...` + `vv/provenance/...`) in place. |

## Summary

`ComputeFeatureReferenceCAxisMisorientationsFilter` computes, for each hex cell, the angular misorientation between the cell's quaternion-derived c-axis and its feature's pre-computed average c-axis (a unit vector supplied by upstream `ComputeAvgCAxes`), plus per-feature arithmetic mean and population standard deviation of those per-cell values. Verification used a **Class 1 (Analytical) hand-built 4-fixture set** — Simple Hex Triple, Realistic Microstructure (with one all-cubic feature exposing the `counts == 0` divide-by-zero path), All-Identical Orientation, and a Class 4 Invariants sweep — with closed-form expected values derived from pure Bunge ZXZ `(0, Φ, 0)` rotations under which the per-cell c-axis miso reduces analytically to `|Φ_cell − Φ_avg|` folded to `[0°, 90°]`. The pre-existing `caxis_data.tar.gz` exemplar test was retired as a circular oracle (archive download retained — still consumed by `ComputeCAxisLocationsTest`), and an empirical A/B against DREAM3D 6.5.171 documented two deviations (D1 — legacy lacks the `isHex` gate and produces garbage non-hex cell values; D4 — `~1e-4°` per-cell precision drift from hand-rolled MatrixMath + float32 stddev); each root cause was proven by applying the corresponding surgical fixes (Eigen + isHex skip; double-precision stddev accumulation) to a local build of the legacy source, after which the legacy output became byte-for-byte identical to SIMPLNX across all 3 output arrays.

## Algorithm Relationship

*Classification:* **Port** with Minor changes 

*Evidence:* Same SIMPL UUID retained via the SIMPL 6.5 conversion fixture at `test/simpl_conversion/6_5/ComputeFeatureReferenceCAxisMisorientationsFilter.json`. SIMPLNX algorithm at `Algorithms/ComputeFeatureReferenceCAxisMisorientations.cpp` (206 lines) preserves the legacy `FindFeatureReferenceCAxisMisorientations::execute()` (DREAM3D 6.5.171, 421-line `.cpp` — line delta is structural, since SIMPLNX splits Filter + Algorithm) structure: same all-non-hex preflight error, same mixed-phase warning, same per-cell triple loop computing `arccos(c1 · avgCAxis)`, same per-feature average + stddev finalize. Sibling `ComputeFeatureReferenceMisorientationsFilter` (single-axis variant) was classified Port in its V&V cycle on `topic/ebsdlib_v3_updates`; this filter is the c-axis-specific analog and follows the same pattern. PR titles since baseline contain no rewrite signals.

*Material PRs since baseline (2025-10-01):*

- **#1438** — *Microtexture related filter cleanup* (2025-10-25, algorithm `+88/-73`, filter `+8/-?`, hpp `+18/-?`) — algorithm rewrite for microtexture handling. **Largest delta in scope — central to this V&V cycle.** Cross-cutting hotspot per audit (also touched ComputeAvgCAxes, ComputeFeatureNeighborCAxisMisalignments, etc. — see those filters' V&V cycles for the deviation pattern this PR creates).
- **#1472** — *Update to EbsdLib 2.0.0 API* (2025-11-24, algorithm `+9/-8`) — EbsdLib namespace + `QuaternionDType.toOrientationMatrix()` API swap. Small line count but semantically material (filter delegates orientation math to EbsdLib). Same precision-class deviation pattern as sibling V&V cycles.
- **#1582** — *Add missing cancel checks to lots of filters* (2026-04-08, algorithm `+20`) — three `m_ShouldCancel` guards added. UX-only; non-behavioral on completed runs.
- *(excluded — broad refactor, no behavioral delta on this filter)* #1547 (doc typos), #1538 (test infra), #1457 (style), #1439 (test API).

## Oracle

*Class:* **1 (Analytical)** primary, **4 (Invariant)** companion.

### Applied (Class 1 — Analytical)

Closed-form derivation: a Bunge ZXZ Euler `(0, Φ, 0)` is a pure rotation about x, yielding `c = R^T · [0, 0, 1] = [0, sin(Φ), cos(Φ)]`. For a cell tilted at `Φ_cell` and a feature average c-axis at `Φ_avg` (same construction), the per-cell misorientation is `|Φ_cell − Φ_avg|` folded to `[0°, 90°]` (the fold is a no-op for all fixtures, which use tilts in `[0°, 25°]`).

Per-feature finalize:

- `featAvgCAxisMis[f] = (Σ miso[hex+valid cells in f]) / counts[f]`
- `featStdevCAxisMis[f] = sqrt(Σ (cellRefCAxisMis[i] − featAvg[f])² / counts[f])` — **population stddev** (divisor `counts`, NOT `counts−1`)

This collapses every expected value in the test fixtures to integer arithmetic.

### Applied (Class 4 — Invariant)

| # | Invariant | Algorithm contract / fixture relevance |
|---|---|---|
| I1 | `cellRefCAxisMis[i] ∈ [0.0f, 90.0f]` for hex cells | Fold contract `if(w > 90) w = 180 − w` |
| I2 | `cellRefCAxisMis[i] == 0.0f` for non-hex / invalid cells | Skip-branch explicitly writes 0 (lines 153-156) |
| I3 | `featAvgCAxisMis[f]` equals the arithmetic mean of `cellRefCAxisMis[i]` for hex+valid cells `i ∈ f` | Algorithm formula at line 176 |
| I4 | `featStdevCAxisMis[f]` is the **population** stddev | Algorithm uses `counts[f]` not `counts[f]−1` (line 202) |
| I5 | All-identical-orientation feature → all `cellRefCAxisMis == 0`, `featAvg == 0`, `featStdev == 0` | Self-consistency of the c-axis projection |
| I6 | All-non-hex feature → `featAvg == NaN`, `featStdev == NaN` | **Documents the latent divide-by-zero** at lines 176, 202 (code paths 5 + 7). Load-bearing for the V&V finding. |

### Encoded

`test/ComputeFeatureReferenceCAxisMisorientationsTest.cpp` — 4 inlined fixtures in the `AnalyticalFixtures` namespace, all pass:

- **Class 1:** `Class 1 — Simple Hex Triple` (closed-form 3-cell sanity), `Class 1 — Realistic Microstructure (exposes divide-by-zero)` (5×5×1, 6 features incl. F3 all-cubic), `Class 1 — All-Identical Orientation Feature` (invariant I5 confirmation).
- **Class 4:** `Class 4 — Invariants` (3 SECTIONs: range + per-feature-mean formula + I6 NaN-on-empty).

All `REQUIRE(actual == Approx(expected).margin(1e-3f))` assertions pass; `std::isnan(...)` assertions pass for the F3 all-non-hex feature.

### Second-engineer review

*Skipped — reason:* The closed-form derivation reuses the math already reviewed and signed off during the sibling `ComputeFeatureNeighborCAxisMisalignmentsFilter` V&V cycle (F#6, branch `vv/compute_feature_neighbor_caxis_misalignments`), which used the same pure-Φ Bunge rotation argument with the same `|ΔΦ|` reduction. The Class 4 invariants are standard for a per-feature aggregation. External cross-validation will be obtained via the empirical A/B against the legacy 6.5.171 binary (with root causes proven via a surgically patched local build of the legacy source) — diff-explanation only, not oracle. Per V&V policy line 33, legacy is never a correctness oracle.

## Code path coverage

**7 of 8 paths exercised** by the V&V test suite. The cancellation path (#8) is structurally present and reviewed but not exercised by an injected cancel signal in the fixtures.

Source: `src/Plugins/OrientationAnalysis/src/OrientationAnalysis/Filters/Algorithms/ComputeFeatureReferenceCAxisMisorientations.cpp` (206 lines).

The algorithm runs in three logical phases: **(a) preflight scan** of `CrystalStructures` to enforce the hex-only contract; **(b) per-cell pass** over the ImageGeometry computing per-cell C-axis misorientation vs. the feature's average and accumulating a per-feature sum; **(c) per-feature finalize** computing the average + population standard deviation.

| #  | Phase           | Path    | Test case            |
|----|-----------------|---------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------|----------------------------------------------------------------------------------------------------------------------------|
| 1  | (a) Preflight   | No hex phases in `CrystalStructures` → return error `-9802`. Early exit before any data array is touched. (lines 52-56)         | `InValid Filter Execution` (mutates `CrystalStructures[1] = Cubic_High` post-load and asserts the execute fails).            |
| 2  | (a) Preflight   | Mixed phases (some hex, some non-hex) → push warning `-9803` to `Result<>::warnings()`, proceed. (lines 59-64) | `Class 1 — Realistic Microstructure` (mixed Hex_High + Cubic_High; algorithm-level warning surfaced via `executeResult.result.warnings()`).      |
| 3  | (b) Per-cell    | Hex Laue (Hex_High or Hex_Low) AND `featureId > 0` AND `cellPhase > 0` → compute `arccos(c1 · avgCAxis)` folded to `[0°, 90°]`, write to per-cell output, accumulate sum + count for the feature. (lines 124-152)    | All 4 Class 1 fixtures (every cell with hex phase across `Simple Hex Triple`, `Realistic Microstructure`, `All-Identical Orientation`).         |
| 4  | (b) Per-cell    | Non-hex OR `featureId == 0` OR `cellPhase == 0` → write `0.0f` to the per-cell output, skip accumulation. (lines 153-156)       | `Class 1 — Realistic Microstructure` (F3 cubic cells); `Class 4 — Invariants` sub-section (i) asserts `cellRefCAxisMis == 0` for those cells.   |
| 5  | (c) Finalize    | Per-feature average `sum / counts` for `featureId ∈ [1, totalFeatures)`. When `counts == 0`, IEEE 754 `0.0f / 0 = NaN` produces the analytically-correct output. (line 176)        | All 4 Class 1 fixtures; `Class 4 — Invariants` sub-section (iii) asserts `std::isnan(featAvg[F3])` for the all-non-hex feature. |
| 6  | (c) Finalize    | Per-cell population-stddev accumulation: `Σ (cellRefCAxisMis − featAvg)²` over every cell, attributed to its feature. (lines 182-192)            | All 4 Class 1 fixtures (stddev compared against closed-form `sqrt(populationVariance)` for F2 + F5 in the Realistic Microstructure).             |
| 7  | (c) Finalize    | Per-feature stddev finalize: `sqrt(sumSqDiff / counts)`. When `counts == 0`, NaN propagates through `sqrt(NaN / 0)` to produce the analytically-correct NaN. (line 202)            | All 4 Class 1 fixtures; `Class 4 — Invariants` sub-section (iii) asserts `std::isnan(featStdev[F3])`.          |
| 8  | Cancellation    | `m_ShouldCancel` checked at 4 sites: outermost cell z-loop (line 105), per-feature avg loop (169), stddev cell loop (184), stddev finalize loop (197). Early `return {}` skips remaining output.    | *Not directly tested — would require cancel-signal injection mid-execution. Structurally reviewed and confirmed to early-return cleanly at each of the 4 sites.* |

The all-non-hex feature path (paths 5 + 7) is the load-bearing case for the IEEE 754 NaN-propagation behavior — confirmed by the `Class 4 — Invariants` sub-section (iii) test, which would fail if the algorithm produced a finite value instead of NaN for `counts[F3] == 0`. See the Deviations section for the legacy A/B finding (D1) that surfaced 6.5.171's contrasting "garbage non-hex cell values + non-NaN avg" behavior.

## Test inventory

- Pre-V&V: 3 TEST_CASEs (1 exemplar-based regression + 1 invalid-execution + 1 SIMPL backwards-compat).
- Post-V&V: **6 TEST_CASEs / 6 ctest entries**, 100% pass (1 retired + 2 kept verbatim + 4 new).

| Test case       | Status               | Notes |
|--------------------------------------------------------------------|----------------------|-------|
| `Valid Filter Execution`         | **RETIRE**           | Circular oracle — consumes `caxis_data.tar.gz` whose `FeatureReferenceCAxisMisorientations` / `FeatureAvgCAxisMisorientations` / `FeatureStdevCAxisMisorientations` arrays were generated from a SIMPLNX run (or a pre-EbsdLib-2.4.1 SIMPLNX run), not from an independent oracle. Per V&V policy line 33, "matches SIMPLNX-then" is not a correctness check. **Archive download stays in `test/CMakeLists.txt`** — `caxis_data.tar.gz` is also consumed by `ComputeCAxisLocationsTest.cpp` (lines 27, 61). |
| `InValid Filter Execution`       | **KEEP**             | Exercises Path 1 (all-non-hex preflight error `-9802`). Mutates `CrystalStructures[1] = 1` (Cubic_High) and asserts execute fails. Re-use unchanged. |
| `SIMPL Backwards Compatibility`  | **KEEP**             | DYNAMIC_SECTION over 6.4 (Filter_Name) + 6.5 (UUID) conversion fixtures at `test/simpl_conversion/6_{4,5}/`. Re-use unchanged. |
| `Class 1 — Simple Hex Triple`    | **NEW**              | Minimal closed-form fixture. 3×1×1 ImageGeom, 1 hex feature (sentinel + F1), cells at `Φ = 0°, 5°, 10°` (Bunge ZXZ `(0, Φ, 0)`). `AvgCAxes[F1]` set to the pre-computed c-axis at `Φ = 5°` (the geometric mean). Expected: `cellRefCAxisMis = [5°, 0°, 5°]`, `featAvg[F1] = 10/3 ≈ 3.333°`, `featStdev[F1] = √(50/9) ≈ 2.357°`. Exercises Paths 3, 5, 6, 7. |
| `Class 1 — Realistic Microstructure (exposes divide-by-zero)`      | **NEW**              | The meaty fixture. 5×5×1 ImageGeom, 6 features (sentinel + 5 real): F1 hex (Φ all 0°), F2 hex (Φ = 8,9,10,11,12°), F3 cubic, F4 hex (Φ all 20°), F5 hex (Φ = 25,28,30,32,35°). Per-feature cell spreads chosen for non-trivial stddev. **F3 is the load-bearing feature**: 0 hex cells → `counts[F3] = 0` → exercises the all-non-hex-feature → NaN path (paths 5, 7) → `featAvg[F3] = NaN`, `featStdev[F3] = NaN` via IEEE 754. Exercises Paths 2, 3, 4, 5, 6, 7. |
| `Class 1 — All-Identical Orientation Feature`     | **NEW**              | Invariant I5 + minimal stddev=0 confirmation. 5×1×1, 1 hex feature, 5 cells all at `Φ = 10°`, `AvgCAxes[F1]` = c-axis at `Φ = 10°`. Expected: `cellRefCAxisMis = [0°, 0°, 0°, 0°, 0°]`, `featAvg[F1] = 0°`, `featStdev[F1] = 0°`. |
| `Class 4 — Invariants` (3 SECTIONs)               | **NEW**              | Reuses the Realistic Microstructure fixture data. Sub-sections: **(i) Range** — every `cellRefCAxisMis[i] ∈ [0°, 90°]` for hex cells, `== 0.0f` for non-hex cells (invariants I1 + I2). **(ii) Per-feature averaging formula** — `featAvg[f]` equals the arithmetic mean of `cellRefCAxisMis[hex+valid cells in f]` (invariant I3). **(iii) All-non-hex feature → NaN** — `featAvg[F3]` and `featStdev[F3]` are both NaN (invariant I6 — **load-bearing for the V&V finding** at paths 5, 7). |

### Test scaffolding pattern

Mirrors the established `AnalyticalFixtures` namespace pattern from sibling `ComputeFeatureNeighborCAxisMisalignmentsTest.cpp` (F#6):

- `CreateScaffold(nX, nY, nZ, numFeatures, numCrystalStructures)` — builds the in-memory `DataStructure` with an ImageGeom + Cell AM + Feature AM + Ensemble AM and pre-allocates all input/output arrays.
- `QuatFromPhiDeg(phiDeg)` — returns `{sin(phi/2 rad), 0, 0, cos(phi/2 rad)}` (quaternion for Bunge ZXZ `(0, phiDeg, 0)`).
- `CAxisFromPhiDeg(phiDeg)` — returns `{0, sin(phiDeg rad), cos(phiDeg rad)}` (the pre-computed c-axis for the AvgCAxes input).
- `BuildArgs()` — constructs the `Arguments` object with all 9 standard input/output paths.
- `BuildRealisticMicrostructure()` — builds the 5×5×1 6-feature fixture (reused by Fixtures 2 + 4).

### Pipeline impact note

`pipelines/EBSD_File_Processing/EBSD_Hexagonal_Data_Analysis.d3dpipeline` runs this filter. No algorithm change was applied during this V&V cycle (the per-feature NaN-on-empty behavior at paths 5 + 7 was confirmed as correct IEEE 754 output by the Class 4 sub-section (iii) test), so the pipeline's output is byte-identical to its pre-V&V SIMPLNX output. Users migrating from DREAM3D 6.5.171 will see deviations D1 + D4 in the pipeline's output — see the Deviations section.

## Exemplar archive

- **Archive:** None — the 4 Class 1 + Class 4 V&V fixtures are inlined in `test/ComputeFeatureReferenceCAxisMisorientationsTest.cpp` under the `AnalyticalFixtures` namespace.
- **Retired exemplar:** `caxis_data.tar.gz` (consumed by the pre-V&V `Valid Filter Execution` TEST_CASE — retired 2026-06-10 as a circular oracle). Archive download **retained** in `test/CMakeLists.txt` because `ComputeCAxisLocationsTest.cpp` still consumes it.
- **Provenance sidecar:** `vv/provenance/ComputeFeatureReferenceCAxisMisorientationsFilter.md` — records the closed-form derivation, per-fixture expected outputs, retired-test disposition, and Claude authorship attribution per the LLM-attribution policy.

## Deviations from DREAM3D 6.5.171

**Empirical A/B** (2026-06-10): SIMPLNX vs DREAM3D 6.5.171, on the Realistic Microstructure fixture. Each root cause was proven by applying the corresponding surgical fixes (Eigen + isHex skip; double-precision stddev accumulation) to a local build of the legacy source, after which the legacy output became byte-for-byte identical to SIMPLNX across all 3 output arrays. A/B workspace at `/Users/mjackson/Desktop/FRCAM_AB_Test/`.

Two documented deviations vs 6.5.171 official:

- **`ComputeFeatureReferenceCAxisMisorientationsFilter-D1`** — Legacy 6.5.171 lacks the `isHex` gate; computes c-axis projection misorientation for non-hex cells and produces garbage values + non-NaN feature averages for all-non-hex features. SIMPLNX skips non-hex cells and produces NaN for `counts == 0` features. PR #1438 fix; root cause proven by applying the same fix to a local build of the legacy source. **Recommendation: trust SIMPLNX.** See `vv/deviations/ComputeFeatureReferenceCAxisMisorientationsFilter.md`.
- **`ComputeFeatureReferenceCAxisMisorientationsFilter-D4`** — Precision-class drift `~1e-4°` per cell / `~1e-5°` per feature avg between 6.5.171 (hand-rolled MatrixMath + float32 stddev) and SIMPLNX (Eigen + double stddev). Root cause proven by applying the Eigen + double-precision-stddev fixes to a local build of the legacy source. **Recommendation: trust SIMPLNX.** See `vv/deviations/ComputeFeatureReferenceCAxisMisorientationsFilter.md`.

For both D1 and D4 the root cause was proven by patching a local build of the legacy source — empirical A/B confirms byte-identical output between the patched legacy build and SIMPLNX. No additional legacy-side action required from this V&V cycle; contact the DREAM3D team for the legacy-parity patch.