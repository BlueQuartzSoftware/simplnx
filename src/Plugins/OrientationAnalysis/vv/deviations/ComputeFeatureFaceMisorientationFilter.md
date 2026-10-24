# Deviations from DREAM.3D v6.5.171: ComputeFeatureFaceMisorientationFilter

Part 1 lists differences from `GenerateFaceMisorientationColoring` in DREAM.3D v6.5.171. Part 2 lists the SIMPLNX phase-bounds defect. D1–D4 keep the existing `ComputeFeatureFaceMisorientations-D<N>` IDs; D5 continues the sequence under the current filter name. The Filter UUID is the permanent cross-reference anchor.

---

## Part 1 — Differences from DREAM.3D v6.5.171

### ComputeFeatureFaceMisorientations-D1

| Field | Value |
|---|---|
| **Deviation ID** | `ComputeFeatureFaceMisorientations-D1` |
| **Filter UUID** | `f3473af9-db77-43db-bd25-60df7230ea73` |
| **Status** | active |

**Symptom:** SIMPLNX computes angles for all 11 Laue classes. DREAM.3D v6.5.171 leaves zero vectors for classes other than Hexagonal_High and Cubic_High.

**Root cause:** Algorithmic choice. The DREAM.3D v6.5.171 source `Source/Plugins/OrientationAnalysis/OrientationAnalysisFilters/GenerateFaceMisorientationColoring.cpp` selects only the two high-symmetry classes. Current `src/Plugins/OrientationAnalysis/src/OrientationAnalysis/Filters/Algorithms/ComputeFeatureFaceMisorientation.cpp` accepts every index in `LaueOps::GetAllOrientationOps()`.

**Affected users:** Users with Hexagonal_Low, Cubic_Low, Triclinic, Monoclinic, OrthoRhombic, Tetragonal_Low, Tetragonal_High, Trigonal_Low or Trigonal_High phases.

**Recommendation:** Trust SIMPLNX. Its analytical tests distinguish valid angles from the zero vectors left by the legacy filter.

**Evidence:** `Curated Data` checks three analytical angles for each of the 11 Laue classes.

---

### ComputeFeatureFaceMisorientations-D2

| Field | Value |
|---|---|
| **Deviation ID** | `ComputeFeatureFaceMisorientations-D2` |
| **Filter UUID** | `f3473af9-db77-43db-bd25-60df7230ea73` |
| **Status** | active |

**Symptom:** SIMPLNX returns one float32 angle in degrees per face. DREAM.3D v6.5.171 returns three float32 components: the unit rotation axis multiplied by that angle.

**Root cause:** Algorithmic choice. SIMPLNX changes the output component shape from `{3}` to `{1}`. The scalar preserves the magnitude and omits the axis direction. Its array storage is one third of the former size for the same face count. The filter's `parametersVersion()` is 2; it also changes the output-name parameter key to `misorientation_array_name` and supplies no automatic migration for the former SIMPLNX key.

The legacy UUID `7cd30864-7bcf-5c10-aea7-d107373e2d40` maps to SIMPLNX UUID `f3473af9-db77-43db-bd25-60df7230ea73`. The conversion preserves the filter's angle-computation purpose, but consumers must adapt to the changed representation. The legacy vector norm can be compared with the new scalar on supported same-phase faces. The archived comparison uses this mapping.

**Affected users:** Users who consume the three-component output or load a SIMPLNX version-1 pipeline. Magnitude consumers must use the scalar angle; axis consumers need a separate axis calculation. Version-1 pipelines must use the new output-name key.

**Recommendation:** Trust SIMPLNX for angle magnitudes. Preserve a separate axis output when a downstream operation needs direction.

**Evidence:** PR #1618 (`7cff7f2ea`) changes the output to one component and changes the output-name parameter key. `SIMPL Backwards Compatibility` checks SIMPL 6.4 and 6.5 conversion.

---

### ComputeFeatureFaceMisorientations-D3

| Field | Value |
|---|---|
| **Deviation ID** | `ComputeFeatureFaceMisorientations-D3` |
| **Filter UUID** | `f3473af9-db77-43db-bd25-60df7230ea73` |
| **Status** | active |

**Symptom:** Background faces, mixed-phase faces and faces with unsupported Laue indices give NaN in SIMPLNX. DREAM.3D v6.5.171 gives zero vectors.

**Root cause:** Algorithmic choice. SIMPLNX explicitly writes NaN when the face cannot enter the angle calculation. The legacy output is initialized to zero and remains zero for these faces. Both filters use a positive shared phase for a valid calculation. NaN distinguishes a missing angle from a genuine zero angle; it is a representation change, not a failed zero-angle calculation.

**Affected users:** Users who aggregate angles across faces. Including legacy zero vectors changes the aggregate; SIMPLNX consumers must decide whether to exclude NaN values or propagate them.

**Recommendation:** Trust SIMPLNX. Select faces with finite angles before calculating an aggregate that must exclude missing values.

**Evidence:** `Curated Data` checks the two background and two mixed-phase cases; unsupported Laue indices have no direct fixture.

---

### ComputeFeatureFaceMisorientations-D4

| Field | Value |
|---|---|
| **Deviation ID** | `ComputeFeatureFaceMisorientations-D4` |
| **Filter UUID** | `f3473af9-db77-43db-bd25-60df7230ea73` |
| **Status** | active |

**Symptom:** Cubic angles lose precision near zero. Before the EbsdLib fix, the SIMPLNX fixture for a 90° c-axis symmetry rotation returned about 0.0212° instead of zero.

**Root cause:** Precision. EbsdLib computed a reduced quaternion scalar from `(qco.z() + qco.w()) / sqrt(2)` and extracted the angle with `acos`. Float32 quaternion inputs promoted to double can put this scalar slightly below one. The fix forms the reduced quaternion's vector components explicitly and uses `2*atan2(|v|, w)`. Equal components subtract to zero without amplifying the scalar rounding error.

DREAM.3D v6.5.171 uses float intermediates and `acos` in `Source/OrientationLib/LaueOps/CubicOps.cpp`. This calculation loses precision near zero.

**Affected users:** Users of DREAM3D-NX v7.0.0 through v7.4.1 or DREAM.3D v6.5.171 who compute cubic angles close to a symmetry-equivalent orientation or apply thresholds sensitive to small numerical differences.

**Recommendation:** Trust SIMPLNX. DREAM3D-NX v7.4.2 contains the fix. The filter uses stable cubic angle extraction near zero.

**Evidence:** EbsdLib commit `5c8c993`, included by SIMPLNX PR #1631 (`ec22ac375`), fixes the calculation. That PR replaces the 0.021200536° expectation in `Curated Data` with zero, with an absolute error limit of 0.000012°. The dependency declarations on `upstream/release/v7.0.0` through `upstream/release/v7.4.1` specify EbsdLib 1.0.35, 1.0.37, 1.0.38, 1.0.39, 1.0.40 or 2.1.0; their `Source/EbsdLib/LaueOps/CubicOps.cpp` uses `acos`. DREAM3D-NX v7.4.2 requires EbsdLib 3.1.2, which contains the explicit-component `atan2` fix; EbsdLib v2.4.1 also contains it. The archived comparison in `vv_work:ComputeFeatureFaceMisorientationFilter/comparison/summary.md` and `vv_work:ComputeFeatureFaceMisorientationFilter/comparison/difference_report.md` has all six comparable curated faces within 0.0001°, without the 0.0212° residual. The production cubic comparison has 1,238 of 636,474 faces outside that tolerance, with a maximum difference of 0.001925168°.

---

## Part 2 — SIMPLNX defects and behavior changes

### ComputeFeatureFaceMisorientationFilter-D5

| Field | Value |
|---|---|
| **Deviation ID** | `ComputeFeatureFaceMisorientationFilter-D5` |
| **Filter UUID** | `f3473af9-db77-43db-bd25-60df7230ea73` |
| **Status** | active |
| **Affected released versions** | DREAM3D-NX v7.0.0 through v7.4.2. DREAM.3D v6.5.171 was affected. |
| **Fixed in** | The DREAM3D-NX release after v7.4.2 |

**Symptom:** A positive phase outside the Crystal Structures array can cause an out-of-bounds read.

**Root cause:** Bug. DREAM3D-NX v7.0.0 through v7.4.2 read Crystal Structures through a positive phase without checking its upper bound. DREAM.3D v6.5.171 does the same in `GenerateFaceMisorientationColoring.cpp`. The v7.4.2 algorithm also reads the front phase before checking whether the two phases match. Current `ValidateParticipatingFeaturePhases()` checks shared positive phases in blocks of 65,536 faces and returns `-98412` before the lookup. The per-face calculation now reads Crystal Structures only after the positive, same-phase condition succeeds.

**Affected users:** Users whose referenced feature phases exceed the Crystal Structures tuple count. In v7.4.2, an invalid front phase on a mixed-phase face can also reach the lookup.

**Recommendation:** Trust SIMPLNX. Correct an invalid participating phase or supply its ensemble entry.

**Evidence:** `src/Plugins/OrientationAnalysis/src/OrientationAnalysis/Filters/Algorithms/ComputeFeatureFaceMisorientation.cpp` on the ten release branches from `upstream/release/v7.0.0` through `upstream/release/v7.4.2` omits the participating-phase bounds validation. PR #1568 (`4ec21e93f`) adds it. `Phase Index Bounds` checks the error for shared phase 2 with only two ensemble tuples; its second section checks that phases 2 and 1 give NaN without an error. The legacy evidence is the `v6.5.171` tag's `Source/Plugins/OrientationAnalysis/OrientationAnalysisFilters/GenerateFaceMisorientationColoring.cpp`.
