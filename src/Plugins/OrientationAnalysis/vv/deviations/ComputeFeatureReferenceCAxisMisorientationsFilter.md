# Deviations from DREAM.3D v6.5.171: ComputeFeatureReferenceCAxisMisorientationsFilter

Part 1 lists differences from DREAM.3D v6.5.171 (D1 and D2). Part 2 lists fixed SIMPLNX defects (D3 and D4). Both parts use the stable `ComputeFeatureReferenceCAxisMisorientationsFilter-D<N>` sequence. The Filter UUID is the permanent cross-reference anchor.

---

## Part 1 — Differences from DREAM.3D v6.5.171

### ComputeFeatureReferenceCAxisMisorientationsFilter-D1

| Field | Value |
|---|---|
| **Deviation ID** | `ComputeFeatureReferenceCAxisMisorientationsFilter-D1` |
| **Filter UUID** | `16c487d2-8f99-4fb5-a4df-d3f70a8e6b25` |
| **Status** | active |

**Symptom:** DREAM.3D v6.5.171 includes non-hexagonal cells in c-axis statistics; SIMPLNX skips them and gives NaN feature statistics when no hexagonal cell contributes.

**Root cause:** Bug. DREAM.3D v6.5.171 gates the angle calculation only on positive FeatureIds and CellPhases. It has no Crystal Structures input or hexagonal-phase check. A cubic cell therefore produces an angle from its quaternion and the supplied feature reference vector. That angle has no meaning under this filter's hexagonal-phase contract.

DREAM3D-NX v7.0.0 through v7.4.2 check Hexagonal-High or Hexagonal-Low before accumulating. DREAM.3D v6.5.171 was affected by the non-hexagonal cell defect; these DREAM3D-NX releases were not affected. DREAM3D-NX v7.0.0 through v7.2.2 write zero mean and standard deviation when no cells contribute. DREAM3D-NX v7.4.0 and v7.4.1 obtain NaN through division by zero. DREAM3D-NX v7.4.2 writes quiet NaN explicitly.

DREAM.3D v6.5.171 also replaces a zero-count feature mean with zero. Its standard-deviation calculation has no zero-count guard. Current SIMPLNX explicitly writes NaN for both outputs. It rejects an ensemble with no hexagonal phase with `-9802` and reports mixed phases with `-9803`.

**Affected users:** Users who process non-hexagonal cells, mixed-phase data, or features with no contributing cells.

**Recommendation:** Trust SIMPLNX. Zero cell angles mark skipped cells, and NaN feature statistics mark the absence of valid contributors.

**Evidence:** The `Class 1 — Realistic Microstructure (exposes divide-by-zero)` and `Class 4 — Invariants` cases assert both results. The source comparison uses `Source/Plugins/OrientationAnalysis/OrientationAnalysisFilters/FindFeatureReferenceCAxisMisorientations.cpp` at the DREAM.3D `v6.5.171` tag and the current SIMPLNX algorithm. The algorithm on all ten `upstream/release/v<version>` branches from v7.0.0 through v7.4.2 contains the hexagonal gate. PR #566 (`6cd130eff`) introduced the gate; PR #1438 (`e6896714b`) changed aggregation; PR #1636 (`c225e8a24`) made NaN output explicit.

---

### ComputeFeatureReferenceCAxisMisorientationsFilter-D2

| Field | Value |
|---|---|
| **Deviation ID** | `ComputeFeatureReferenceCAxisMisorientationsFilter-D2` |
| **Filter UUID** | `16c487d2-8f99-4fb5-a4df-d3f70a8e6b25` |
| **Status** | active |

**Symptom:** Cell angles and feature statistics can differ numerically from DREAM.3D v6.5.171 because intermediate precision and evaluation order differ.

**Root cause:** Precision and library. DREAM.3D v6.5.171 uses float orientation matrices, `MatrixMath` vector operations, `acosf`, and a float array for the count, angle sum and squared-difference sum. SIMPLNX uses a double quaternion-to-matrix conversion, Eigen double vectors and a double angle calculation. Its feature-angle sum, feature mean and variance accumulator use double precision. It converts each stored float32 cell angle to double before subtracting the double feature mean. Output arrays use float32.

DREAM3D-NX v7.0.0 already uses double vector math. DREAM3D-NX v7.4.0 replaces the float variance accumulator with `std::vector<double>` and changes cell traversal to plane, row, column order. Later releases retain these changes. This filter does not call `LaueOps::calculateMisorientation`; changes to that function do not explain this difference.

**Affected users:** Users who compare numerical outputs across DREAM.3D v6.5.171 and DREAM3D-NX or across the change in feature-statistics accumulation.

**Recommendation:** Trust SIMPLNX within the analytical tests' 0.001° tolerance. This tolerance applies to the analytical fixtures.

**Evidence:** The three Class 1 cases independently check the cell angles, means and population standard deviations. PR #1438 (`e6896714b`) changed variance accumulation and traversal order. The release algorithm sources show double vector math in v7.0.0 and double variance accumulation from v7.4.0.

---

## Part 2 — SIMPLNX defects and behavior changes

### ComputeFeatureReferenceCAxisMisorientationsFilter-D3

| Field | Value |
|---|---|
| **Deviation ID** | `ComputeFeatureReferenceCAxisMisorientationsFilter-D3` |
| **Filter UUID** | `16c487d2-8f99-4fb5-a4df-d3f70a8e6b25` |
| **Status** | active |
| **Affected released versions** | DREAM3D-NX v7.0.0 through v7.4.1. DREAM.3D v6.5.171 was not affected. |
| **Fixed in** | DREAM3D-NX v7.4.2 |

**Symptom:** Successful execution omits warning `-9803` when non-hexagonal phases are skipped.

**Root cause:** Bug. The affected algorithms append the warning to a local `Result<> result`, then return an empty result at the end. DREAM3D-NX v7.4.2 and the current filter return `result` and retain the warning. DREAM.3D v6.5.171 has no equivalent mixed-phase warning path.

**Affected users:** Users who depend on execution warnings to identify skipped non-hexagonal phases. The separate preflight warning `-9801` is not affected.

**Recommendation:** Trust SIMPLNX. The returned result now carries the warning that the algorithm creates.

**Evidence:** PR #1636 (`c225e8a24`) changes the final statement to `return result;`. The release sources through v7.4.1 discard the warning; v7.4.2 returns it. `Realistic Microstructure` exercises mixed phases, but its nonempty-warning assertion does not isolate `-9803` from the preflight warning.

---

### ComputeFeatureReferenceCAxisMisorientationsFilter-D4

| Field | Value |
|---|---|
| **Deviation ID** | `ComputeFeatureReferenceCAxisMisorientationsFilter-D4` |
| **Filter UUID** | `16c487d2-8f99-4fb5-a4df-d3f70a8e6b25` |
| **Status** | active |
| **Affected released versions** | DREAM3D-NX v7.0.0 through v7.4.2. DREAM.3D v6.5.171 was not affected. |
| **Fixed in** | The DREAM3D-NX release after v7.4.2 |

**Symptom:** An invalid phase can read outside Crystal Structures, including on a cell that should be skipped.

**Root cause:** Bug. DREAM3D-NX v7.0.0 through v7.4.2 read `crystalStructures[cellPhases[cellIdx]]` before testing whether the cell participates. No phase-range check precedes that read. The current filter first requires a positive FeatureId and phase, then checks the phase against the ensemble tuple count. An invalid positive participating phase returns `-9804`; a nonpositive phase or feature zero skips the lookup. DREAM.3D v6.5.171 does not read Crystal Structures in this filter, so it has no such lookup.

**Affected users:** Users whose Cell Phases array contains an invalid ensemble index.

**Recommendation:** Trust SIMPLNX. Correct the phase index or supply the corresponding ensemble entry.

**Evidence:** PR #1568 (`4ec21e93f`) adds the participation and phase-range checks. The release algorithm sources from v7.0.0 through v7.4.2 perform the lookup before those checks. `Phase Index Bounds` checks the error and the two skip conditions. All visible tests pass in CI at `419f9db35`.
