# Deviations from DREAM3D 6.5.171: ComputeFeatureNeighborCAxisMisalignmentsFilter

This file lists every documented behavioral difference between this SIMPLNX filter and its DREAM3D 6.5.171 equivalent (`FindFeatureNeighborCAxisMisalignments`, source at `Source/Plugins/OrientationAnalysis/OrientationAnalysisFilters/FindFeatureNeighborCAxisMisalignments.{h,cpp}` in DREAM3D 6.5.171).

Entries are referenced by stable ID (`ComputeFeatureNeighborCAxisMisalignmentsFilter-D<N>`) from the V&V report and from public migration guidance. The ID is stable across renames; the Filter UUID field is the permanent cross-reference anchor.

## Comparison summary

The legacy A/B comparison was performed **empirically** on 2026-06-04 against three binaries:

- **A:** DREAM3D 6.5.171 (official release, `/Users/mjackson/Applications/DREAM3D.app/Contents/bin/PipelineRunner`) — buggy.
- **B:** A local build of the DREAM3D 6.5.171 source with the surgical divisor-bug fix applied (`/Users/mjackson/DREAM3D-Dev/DREAM3D-Build/D3D-Rel-Qt515-6_5_171/Bin/PipelineRunner`) — used only to prove the root cause — fixed.
- **C:** SIMPLNX (post-fix, `/Users/mjackson/Workspace6/DREAM3D-Build/NX-Com-Qt69-Vtk95-Rel-EbsdLib/Bin/nxrunner`) — fixed.

All three binaries were run on the same hand-built `.dream3d` input file containing the realistic-microstructure fixture (10×10×1 ImageGeom, 6 features, mixed hex/non-hex phases, pure-Φ Bunge ZXZ rotations matching the SIMPLNX test fixture). A/B test workspace and artifacts (input `.dream3d`, 3 output `.dream3d`, per-binary pipeline files, comparison script, run results) at `/Users/mjackson/Desktop/F6_AB_Test/`.

**Result summary:**

- Per-pair `CAxisMisalignmentList` values: all three binaries produce identical values within float32 precision (`~1e-6°` drift between A/B and C).
- Per-feature `AvgCAxisMisalignments` for F2/F5/F6 (the bug-exposing features): A produces the predicted-buggy values; B and C both produce the analytical-correct values.
- Per-feature `AvgCAxisMisalignments` for F3 (all-non-hex neighbor list): A produces `0.0`; B and C both produce `NaN`. **Additional symptom of D1** documented under D1 below.

SIMPLNX `ComputeFeatureNeighborCAxisMisalignments::operator()()` is a clean Port of legacy `FindFeatureNeighborCAxisMisalignments::execute()` (same per-feature outer loop, same per-neighbor inner loop, same hex-hex phase gate, same optional per-feature averaging finalize). Both implementations originally shared the divisor bug at the `hexNeighborListSize` reassignment (D1 below). The bug went undetected for the lifetime of both implementations because the existing SIMPLNX exemplar test consumed a hex-phase-only dataset, which never exercises the per-mismatch decrement branch.

This filter is the c-axis analog of `ComputeFeatureNeighborMisorientationsFilter`. Important distinction: this filter does NOT route through `LaueOps::calculateMisorientation`. It uses Eigen for the c-axis vector math (orientation matrix → c-axis rotation → `arccos(|c1·c2|)` folded to `[0°, 90°]`), so the EbsdLib 2.4.1 precision improvement that surfaced as a deviation in F#1/F#2/F#4/F#5 of this V&V cycle **does not apply here**.

---

## ComputeFeatureNeighborCAxisMisalignmentsFilter-D1

| Field            | Value                                                                                                                                |
|------------------|--------------------------------------------------------------------------------------------------------------------------------------|
| **Deviation ID** | `ComputeFeatureNeighborCAxisMisalignmentsFilter-D1`                                                                                  |
| **Filter UUID**  | `636ee030-9f07-4f16-a4f3-592eff8ef1ee`                                                                                               |
| **Status**       | active (SIMPLNX fixed 2026-06-04; legacy 6.5.171 still has the bug — root cause proven 2026-06-04 by applying the same fix to a local build of the legacy source); SIMPLNX-side fix ships in DREAM3D-NX **7.4.2** |

**Symptom:** Per-feature `AvgCAxisMisalignments` (output of `find_avg_misals=true` / legacy `FindAvgMisals=true`) differ between SIMPLNX (post-2026-06-04 fix) and DREAM3D 6.5.171 on any dataset where features have mixed hex/non-hex neighbor lists. The legacy result depends on the *order* in which neighbors appear in the per-feature `NeighborList`: if the last-iterated neighbor is hex-hex same-phase, the divisor used is the full neighbor-list length (incorrect); if the last neighbor is non-hex or different-phase, the divisor is decremented by 1 from the full length (the per-mismatch decrement at line 150 happens to be the last write to `hexNeighborListSize`). The legacy result is therefore correct in some cases by accident and wrong by up to `(N-K) / N` of the true value in others, where N is the neighbor count and K is the number of hex-hex same-phase neighbors.

The bug is **not observable on the legacy SIMPLNX exemplar dataset** (`7_5_simplnx_test_file_25x50_Hex.dream3d` — hex-phase-only, so no mismatch decrements ever fire). The bug **IS observable** on the V&V `Realistic Microstructure (exposes divisor bug)` fixture, which constructs a 10×10×1 microstructure with mixed hex (F1, F2, F4, F5, F6) and cubic (F3) features. Three of the six features have neighbor lists that fire the bug (F2, F5, F6), with measured pre-fix averages of `7.500°`, `6.250°`, `2.500°` instead of the correct `10.000°`, `8.333°`, `5.000°`.

**Additional symptom (surfaced by empirical A/B testing 2026-06-04):** for features whose entire neighbor list is non-hex (F3 in the V&V fixture), legacy 6.5.171 produces `avg = 0.0` whereas the correct behavior is `avg = NaN`. The mechanism: the buggy code reassigns `hexNeighborListSize` to the full list size at the top of each j-iteration and decrements it in the non-hex else-branch. On a 3-neighbor all-non-hex list, the iteration sequence is: j=0 (assign hex=3, decrement to 2); j=1 (assign hex=3, decrement to 2); j=2 (assign hex=3, decrement to 2). Final `hexNeighborListSize = 2`. The post-loop `if(hexNeighborListSize > 0)` branch is then true → `avg = 0.0 / 2 = 0.0` (the accumulator is 0 because no hex-match contributions were ever added). The intended behavior (executed correctly under the post-fix code) is for `hexNeighborListSize` to decrement to 0 after all three non-hex matches, falling through to the `else: avg = NaN` branch. Empirically: 6.5.171 produces `avg[F3] = 0.0`; the patched local build of the legacy source and SIMPLNX both produce `avg[F3] = NaN`.

**Root cause:** **Bug** in both legacy DREAM3D 6.5.171 and SIMPLNX pre-fix.

The legacy code at `Source/Plugins/OrientationAnalysis/OrientationAnalysisFilters/FindFeatureNeighborCAxisMisalignments.cpp:280` and the SIMPLNX pre-fix code at `Algorithms/ComputeFeatureNeighborCAxisMisalignments.cpp:111` both contain `hexNeighborListSize = currentNeighborList.size();` (or the legacy `hexneighborlistsize = neighborlist[i].size();`) *inside* the inner per-neighbor j-loop. The intended behavior is for `hexNeighborListSize` to start each outer-loop iteration (per feature) at the neighbor-list size and then decrement by 1 for each phase-mismatched neighbor (line 150: `hexNeighborListSize--;`). Because the reassignment happens at the *top* of each j-iteration, the decrement from the *previous* iteration is clobbered. Only the *last* j-iteration's match/mismatch state actually affects `hexNeighborListSize`: if the last neighbor is hex-hex same-phase, the assignment runs and the decrement doesn't, so the final divisor is N; if the last neighbor is non-hex or different-phase, both the assignment and the decrement run, so the final divisor is N - 1.

The SIMPLNX fix (2026-06-04) moves the `hexNeighborListSize = currentNeighborList.size();` assignment from line 111 to before the inner j-loop (alongside `currentMisalignmentList.resize(...)` at line 106), so the assignment runs once per outer-loop iteration (per feature) and the decrement is preserved across j-iterations. The result is the mathematically correct divisor: the number of hex-hex same-phase neighbors.

The bug went undetected for the lifetime of both implementations because:

1. **The legacy 6.5.171 implementation had no automated test coverage of the `FindAvgMisals=true` path on mixed-phase data.** Legacy DREAM3D's CI tested filters with default parameter values; this parameter defaults to false in many user-facing pipelines and the test infrastructure didn't sweep over both values.
2. **The SIMPLNX Port preserved the bug** and the existing exemplar dataset was hex-phase-only. The exemplar would have happily passed even on the buggy code, because the per-mismatch decrement branch never fires on hex-only data.
3. **PR #1467 ("OEM-reviewed cleanup") signed off on the buggy code.** The review focused on naming, comments, and structure — not on the inner-loop divisor invariant.
4. **The retroactive bug-triage cycle (2026-05) caught it** by source inspection. Documented in `/Users/mjackson/Desktop/bug_triage.md` as Bug #3 (sibling of Bug #2 / F#2 D1).

**Affected users:** Anyone running DREAM3D 6.5.171 or SIMPLNX pre-2026-06-04 with `FindAvgMisals=true` / `find_avg_misals=true` on data containing features with mixed hex / non-hex neighbor lists. **Production-relevant via shipping pipeline.** The reference pipeline `pipelines/EBSD_File_Processing/EBSD_Hexagonal_Data_Analysis.d3dpipeline` runs this filter with `find_avg_misals: true`. Any user running that pipeline on multi-phase EBSD data containing at least one cubic, tetragonal, or trigonal phase will have produced incorrect per-feature `AvgCAxisMisalignments` values.

**Recommendation:** **Trust SIMPLNX (post-2026-06-04 fix).** The pre-fix per-feature `AvgCAxisMisalignments` values from both DREAM3D 6.5.171 and pre-fix SIMPLNX are mathematically incorrect for any feature with a mixed-phase neighbor list. Users migrating from 6.5.171 should expect per-feature averages to shift toward the mathematically correct value, with the shift size proportional to the fraction of phase-mismatched neighbors per feature.

The root cause was proven by applying the same fix (move the `hexneighborlistsize` reassignment outside the inner loop) to `FindFeatureNeighborCAxisMisalignments.cpp` in a local build of the legacy source, bundled with the sibling `FindMisorientations` fix; contact the DREAM3D team for the legacy-parity patch.

---

## ComputeFeatureNeighborCAxisMisalignmentsFilter-D2

| Field            | Value                                                                                                |
|------------------|------------------------------------------------------------------------------------------------------|
| **Deviation ID** | `ComputeFeatureNeighborCAxisMisalignmentsFilter-D2`                                                  |
| **Filter UUID**  | `636ee030-9f07-4f16-a4f3-592eff8ef1ee`                                                               |
| **Status**       | active (latent — empirically confirmed DORMANT on current SIMPLNX in-memory DataStore; may surface on future OOC backends) |

**Symptom:** Latent. When `find_avg_misals=true`, the output `AvgCAxisMisalignments` array is allocated via `CreateArrayAction` in the filter's preflight WITHOUT an explicit `fillValue` argument. Inside the algorithm, the first per-feature hex-hex match write does:

```cpp
float32 value = avgCAxisMisalignmentPtr->getValue(featureIdx) + currentMisalignmentList[j];
avgCAxisMisalignmentPtr->setValue(featureIdx, value);
```

which reads the array's pre-write value before adding the new contribution. If `CreateArrayAction` does not zero-initialize the array when `fillValue` is empty (the underlying behavior depends on the `DataStoreUtilities::CreateDataStore` implementation and the IOCollection's default-init contract), the accumulator starts from undefined or implementation-defined state, and the per-feature average is wrong by exactly that initial garbage value.

**Root cause:** **Bug** (latent) in SIMPLNX. The filter's `preflightImpl` at lines 125-127 of `ComputeFeatureNeighborCAxisMisalignmentsFilter.cpp` constructs the `CreateArrayAction` without a fillValue:

```cpp
auto createArrayAction = std::make_unique<CreateArrayAction>(
    DataType::float32,
    featurePhases.getIDataStore()->getTupleShape(),
    std::vector<usize>{1},
    pAvgCAxisMisalignmentsPathValue);
```

The algorithm at line 142 assumes the array starts at zero (`getValue(featureIdx) + ...`). If `DataStoreUtilities::CreateDataStore<float32>` zero-initializes by default (which it currently does for the in-memory `DataStore<T>` constructor — `m_DataStore = std::vector<T>(numTuples * numComponents);` value-initializes), the bug is dormant. But this behavior is implementation-detail of the underlying DataStore type and is not enforced by the `CreateArrayAction` contract.

This was not exercised by any V&V fixture because the realistic-microstructure fixture happens to have every feature with `find_avg_misals=true` and a non-zero expected average start with a hex-hex first-neighbor (F1=F2 hex-hex first, F2=F1 hex-hex first, F4=F1 hex-hex first, F5=F2 hex-hex first, F6=F3 NON-hex first but F6's expected avg is 5° from a single hex-hex contribution — so F6 reads its initial value before the first hex-hex write at j=1, exposing the read pattern but the actual default-init behavior in the in-memory build is zero so the test passes).

**Affected users:** Anyone running this filter on SIMPLNX with `find_avg_misals=true` on a backend where `DataStoreUtilities::CreateDataStore<float32>` does not zero-initialize. Currently no shipping backend exhibits non-zero default-init, but future out-of-core DataStore implementations may. Latent → active reclassification recommended once an OOC DataStore lands.

**Recommendation:** **Defensive fix** — pass `"0"` as the `fillValue` argument to `CreateArrayAction`, OR add an explicit `avgCAxisMisalignmentPtr->fill(0.0f)` at the top of `operator()()` when `FindAvgMisals` is true. Either change is a one-line edit. Confirm by inspection of `DataStoreUtilities::CreateDataStore`'s default-init behavior whether this is currently a real bug or a latent one; the V&V cycle did NOT make this change (out of scope for the divisor-bug-fix focus).

---

## ComputeFeatureNeighborCAxisMisalignmentsFilter-D4

| Field            | Value                                                       |
|------------------|-------------------------------------------------------------|
| **Deviation ID** | `ComputeFeatureNeighborCAxisMisalignmentsFilter-D4`         |
| **Filter UUID**  | `636ee030-9f07-4f16-a4f3-592eff8ef1ee`                      |
| **Status**       | active (precision-class; non-deviation in algorithmic sense) |

**Symptom:** Per-feature `CAxisMisalignmentList` and `AvgCAxisMisalignments` values differ between SIMPLNX (PR #1472+) and DREAM3D 6.5.171 by approximately `1e-6°` per neighbor pair and up to `~2e-5°` per per-feature average. **Empirically measured 2026-06-04** on the realistic-microstructure A/B fixture: 6.5.171 returns `5.0000` and `15.0000` exactly (legacy float32 path through hand-rolled MatrixMath); SIMPLNX returns values like `5.000001`, `14.999999`, `15.000001` (float32 output cast from Eigen double-precision intermediate). Per-feature averages on F1 (the simplest case): 6.5.171 = `10.0000124`, the local legacy build before the D4 fix = `10.0000124`, SIMPLNX = `10.0000000` (exact). Existing doc note in `docs/ComputeFeatureNeighborCAxisMisalignmentsFilter.md` quotes "~0.0001°" — conservative; actual drift is ~100× smaller. Non-observable on the V&V analytical assertions (the closed-form `|ΔΦ|` derivation matches all three binaries within the `1e-3°` Approx tolerance).

**Root cause proven 2026-06-04** by surgically applying the Eigen + double precision conversion to a local build of the legacy source, following the `FindAvgCAxes` and `FindFeatureReferenceCAxisMisorientations` legacy-patch precedents. **Post-fix empirical result: the patched legacy build produces BIT-IDENTICAL output to SIMPLNX on the F#6 fixture** — 18 per-pair `CAxisMisalignmentList` entries + 6 per-feature `AvgCAxisMisalignments` entries all byte-compared and confirmed identical via `h5py` direct comparison. This conclusively attributes the entire pre-fix `~1e-6°` drift to the (Eigen + double) ↔ (hand-rolled MatrixMath + float) precision style difference — no other latent algorithmic difference remains.

**Root cause:** **Library swap** during PR #1472 ("EbsdLib bump"). Two pieces of orientation math were replaced:

1. **Quaternion → orientation matrix conversion.** Legacy and pre-#1472 SIMPLNX used `OrientationTransformation::qu2om<QuatD, OrientationD>(quat)` (a hand-rolled conversion in the OrientationLib/EbsdLib transformation utilities). PR #1472 replaced this with `ebsdlib::QuaternionDType(quat).toOrientationMatrix()` (an EbsdLib member function on the quaternion class itself). Both produce the same matrix up to float64 precision, but the internal arithmetic order differs.

2. **G-matrix transpose.** Legacy used `OrientationMatrixToGMatrixTranspose(oMatrix)`, which built the transpose into a NEW matrix object. PR #1472 replaced this with `oMatrix.transpose()` (Eigen's lazy `Transpose` view). The downstream `.transpose() * cAxis` operation then evaluates the transpose-multiply as a single Eigen expression. Numerically identical to a pre-built transposed matrix multiplied by the same vector, but the rounding sequence is different.

**Affected users:** Anyone diff-ing per-pair c-axis misalignment values between DREAM3D 6.5.171 output and post-PR-#1472 SIMPLNX output. The shift is well below typical EBSD measurement resolution (~`0.5°`) and will not materially affect downstream microstructural analyses.

**Recommendation:** **Trust SIMPLNX.** The shift is precision-class noise from a library swap, not an algorithmic difference. The Eigen-based form is the cleaner expression and is consistent with the rest of the post-#1472 OrientationAnalysis plugin. The doc note's "0.0001 degrees" estimate is in line with what the V&V cycle observed — no growth over the original PR #1472 commit.

---

## ComputeFeatureNeighborCAxisMisalignmentsFilter-D5

| Field            | Value                                                              |
|------------------|--------------------------------------------------------------------|
| **Deviation ID** | `ComputeFeatureNeighborCAxisMisalignmentsFilter-D5`                |
| **Filter UUID**  | `636ee030-9f07-4f16-a4f3-592eff8ef1ee`                             |
| **Status**       | active (UX-only downgrade; preflight banner no longer shown in GUI parameter panel — execute-time algorithm warning still surfaces correctly to all users) |

**Symptom (partially retracted after empirical A/B 2026-06-04):** PR #1438 moved a FILTER-LEVEL preflight banner from `resultOutputActions.warnings()` to `preflightUpdatedValues`. The ALGORITHM-side warning at lines 53-56 of `Algorithms/ComputeFeatureNeighborCAxisMisalignments.cpp` (`"Non Hexagonal phases were found. All calculations for non Hexagonal phases will be skipped and a NaN value inserted."`) is STILL pushed into the algorithm's `Result<>::warnings()` collection and DOES surface in CLI nxrunner output. Empirically confirmed during the A/B test: SIMPLNX nxrunner stderr printed `Code: -1563   Message: Non Hexagonal phases were found...` when run on the realistic-microstructure fixture.

What was actually lost in PR #1438 is the **preflight-time** banner that GUI users see in the parameter panel before they hit "Execute." Pipeline-mode users still see the same warning, but only at execute-time (when the algorithm hits the early-exit / warning branch). This is a UX downgrade (delayed feedback) but NOT the "users see nothing" regression originally claimed.

**Root cause:** **Intentional UX change** in PR #1438 ("Microtexture cleanup"). The PR author's intent was likely to reduce CLI noise by demoting an "informational" message — but the message describes a real algorithmic behavior (NaN insertion) that downstream consumers need to know about, especially when running the shipping `EBSD_Hexagonal_Data_Analysis.d3dpipeline` on data that turns out to contain non-hex phases.

The current algorithm code still produces the warning correctly (lines 53-56), but it's pushed into the algorithm's `Result<>` return, which is surfaced via different channels in GUI vs CLI mode. PR #1438's specific change was the demotion at the FILTER level, not the algorithm level.

**Affected users:** Pipeline-mode users (CLI / Python / nxrunner) running this filter on mixed-phase data. They will see NaN values appear in the `CAxisMisalignmentList` output without any warning that explains why. GUI users still see the message correctly.

**Recommendation:** **Restore the warning to `Result<>::warnings()`** so pipeline-mode users see it. This is a one-line addition to the filter or algorithm — pushing the warning into both `Result<>` AND `preflightUpdatedValues` is fine. The V&V cycle did NOT make this change (out of scope for the divisor-bug-fix focus); flag as follow-up.

---

## ComputeFeatureNeighborCAxisMisalignmentsFilter-D6

| Field            | Value                                                              |
|------------------|--------------------------------------------------------------------|
| **Deviation ID** | `ComputeFeatureNeighborCAxisMisalignmentsFilter-D6`                |
| **Filter UUID**  | `636ee030-9f07-4f16-a4f3-592eff8ef1ee`                             |
| **Status**       | active (behavior class; SIMPLNX correct since port, legacy gap)    |

**Symptom:** For datasets containing features whose shared phase has Laue class **Hexagonal_Low** (6/m), SIMPLNX computes the c-axis misalignment exactly as for Hexagonal_High (6/mmm); DREAM3D 6.5.171 writes `NaN` because the legacy phase-match gate restricts the calculation to Hex_High pairs only.

**Root cause:** **Library + algorithmic choice** — same root pattern as D1 of `ComputeFeatureFaceMisorientations`.

The legacy code at `Source/Plugins/OrientationAnalysis/OrientationAnalysisFilters/FindFeatureNeighborCAxisMisalignments.cpp:286` reads:
```cpp
if(phase1 == phase2 && (phase1 == Ebsd::CrystalStructure::Hexagonal_High))
```
which restricts the c-axis-misalignment computation to Hex_High↔Hex_High neighbor pairs only.

The SIMPLNX code at `Algorithms/ComputeFeatureNeighborCAxisMisalignments.cpp:114` reads:
```cpp
if(xtalPhase1 == xtalPhase2 && (xtalPhase1 == ebsdlib::CrystalStructure::Hexagonal_High || xtalPhase1 == ebsdlib::CrystalStructure::Hexagonal_Low))
```
which correctly handles BOTH hex Laue classes. The c-axis math is independent of symmetry-operator-set choice (the algorithm only uses the orientation matrix and the c-axis direction; no `LaueOps` calls are made), so the same code path produces the mathematically correct misalignment for both hex Laue classes. The legacy restriction to Hex_High was historical — the OrientationLib of that era only had `LaueOps` symmetry operators for Hex_High, and the original author conservatively restricted the gate to match what other hex-aware filters could handle. This filter doesn't need symmetry operators, so the restriction is unnecessary.

The early-exit preflight in SIMPLNX (lines 35-45) also treats both Hex Laue classes as "valid hex phases" for the all-non-hex error and mixed-phase warning logic. Legacy 6.5.171 only counts Hex_High features.

**Not observable on the F#6 V&V fixture** because the realistic-microstructure fixture only uses Hex_High features (no Hex_Low). The deviation IS observable on any real EBSD dataset containing Hex_Low phases (e.g., wurtzite-structure materials, some intermetallics).

**Affected users:** Anyone running this filter on DREAM3D 6.5.171 with a dataset containing Hex_Low phases. Legacy writes `NaN` for those features' misalignment entries; SIMPLNX computes the real c-axis misalignment.

**Recommendation:** **Trust SIMPLNX.** The c-axis math is correct for both hex Laue classes; the legacy restriction was overly conservative. **Proven 2026-06-04** by applying the fix to a local build of the legacy source, bundled with D4 (Eigen+double conversion). Post-fix: the patched legacy `FindFeatureNeighborCAxisMisalignments` accepts both Hex_High and Hex_Low pairs, identical to SIMPLNX.

---

## Non-deviations (algorithm characteristics common to both filters)

The following behaviors are NOT deviations — SIMPLNX (post-D1 fix) and DREAM3D 6.5.171 (with D1 still present) agree on them where D1 is not exercised. Captured here so future engineers don't re-discover them and propose them as deviations.

### NaN entry on phase mismatch and non-hex Laue class

Both implementations write `NaN` (via `std::nanf("")` or the C macro `NAN`) into the per-neighbor `CAxisMisalignmentList` entry when the focal feature's phase differs from the neighbor's phase, or when the shared phase's Laue class is not `Hexagonal_High` (or `Hexagonal_Low` — both are accepted). **Both filters share this behavior** — algorithm characteristic, not a defect.

### Per-feature outer-loop iteration starts at index 1 (skips background feature 0)

Both implementations iterate `for(size_t i = 1; i < totalFeatures; i++)` in the per-feature outer loop, skipping the background feature at index 0. The `CAxisMisalignmentList[0]` and `AvgCAxisMisalignments[0]` entries are therefore left at their initialized default values (empty list and `0.0f`, respectively). **Both filters share this behavior**.

### Default output array name rename (formerly proposed as D3)

PR #1438 renamed the default output array from `"AvgCAxisMisalignments"` to `"AvgNeighborCAxisMisalignments"`, and reworded the parameter labels:
- `"C-Axis Misalignment List"` → `"Feature C-Axis Misalignment NeighborList"`
- `"Average C-Axis Misalignments"` → `"Feature Average C-Axis Misalignments"`

This is **user-facing migration noise**, not a behavioral deviation. The SIMPLNX backwards-compat path (PR #1588) preserves the SIMPL conversion semantics. Existing pipeline files in the repo were re-saved with the new name; user-saved pipelines that explicitly named arrays still work. Documented here for completeness; not a numbered deviation.

### EbsdLib 2.4.1 CubicOps precision improvement does NOT apply

Unlike F#1 / F#2 / F#4 / F#5, this filter does NOT route through `LaueOps::calculateMisorientation` or `CubicOps::calculateMisorientationInternal`. The algorithm computes c-axis vectors directly from the orientation matrix (`R^T · [0,0,1]`) and takes `arccos(c1·c2)` without any cubic-symmetry-operator search. The EbsdLib 2.4.1 precision improvement therefore has no effect on this filter's output, and there is no `D-precision` entry analogous to the other filters in the cycle.
