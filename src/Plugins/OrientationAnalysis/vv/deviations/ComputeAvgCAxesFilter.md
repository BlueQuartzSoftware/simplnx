# Deviations from DREAM.3D v6.5.171: ComputeAvgCAxesFilter

Part 1 lists the output differences from DREAM.3D v6.5.171 (D1 and D2). Part 2 lists fixed SIMPLNX defects (D3 through D5). Both parts use one stable ID sequence. The Filter UUID is the permanent cross-reference anchor.

---

## Part 1 — Differences from DREAM.3D v6.5.171

### ComputeAvgCAxesFilter-D1

| Field | Value |
|---|---|
| **Deviation ID** | `ComputeAvgCAxesFilter-D1` |
| **Filter UUID** | `453cdb58-7bbb-4576-ad5e-f75a1c54d348` |
| **Status** | active |

**Symptom:** SIMPLNX writes three NaN components for a feature with no hexagonal contribution. DREAM.3D v6.5.171 leaves feature zero at `(0,0,0)` and writes `(0,0,1)` for an empty positive feature.

**Root cause:** Algorithmic choice. DREAM.3D v6.5.171 `FindAvgCAxes` has no Cell Phases or Crystal Structures inputs. It includes every cell with a positive FeatureId. Its final loop starts at feature 1 and supplies `(0,0,1)` when the count is zero. SIMPLNX accepts only Hexagonal_High and Hexagonal_Low contributions, finalizes from feature zero, and writes NaN when the count is zero. The current code also preserves hexagonal contributions when a feature contains both hexagonal and non-hexagonal cells. DREAM3D-NX v7.0.0 through v7.4.1 wrote NaN into the sum when a non-hexagonal cell occurred. DREAM3D-NX v7.4.2 decides whether to write NaN during finalization.

**Affected users:** Users with empty features, non-hexagonal phases, or mixed-phase features. Downstream calculations must handle NaN values.

**Recommendation:** Trust SIMPLNX for hexagonal C-axis statistics. Exclude features with NaN averages from downstream direction calculations.

**Evidence:** PR #1631 (`ec22ac375`) moved the NaN decision to finalization. DREAM.3D tag `v6.5.171`, `Source/Plugins/OrientationAnalysis/OrientationAnalysisFilters/FindAvgCAxes.cpp`, compared with `src/Plugins/OrientationAnalysis/src/OrientationAnalysis/Filters/Algorithms/ComputeAvgCAxes.cpp`. `Class 1 Oracle (hand-built dataset)` checks NaN values for F0 (no assigned cells), F5 (empty positive feature), and F6 (one identity cell marked Cubic_High). The legacy source leaves F0 at zero and writes `(0,0,1)` for F5 and F6. The fixture does not contain a feature with both hexagonal and non-hexagonal cells.

---

### ComputeAvgCAxesFilter-D2

| Field | Value |
|---|---|
| **Deviation ID** | `ComputeAvgCAxesFilter-D2` |
| **Filter UUID** | `453cdb58-7bbb-4576-ad5e-f75a1c54d348` |
| **Status** | active |

**Symptom:** SIMPLNX returns a unit average C-axis. DREAM.3D v6.5.171 returns an unnormalized mean. Rounding near a zero dot product can also change the selected direction.

**Root cause:** Precision and algorithmic choice. DREAM.3D v6.5.171 uses float orientation calculations and float sums. SIMPLNX uses double orientation matrices, vectors, and cosine calculations, but accumulates components in float32. Both implementations flip a contribution when its cosine with the running direction is negative. SIMPLNX increments the count before this decision and normalizes the final mean; DREAM.3D v6.5.171 increments it afterward and does not normalize the final mean.

**Affected users:** Users who compare magnitudes with DREAM.3D v6.5.171, or whose inputs put the sign decision near a zero dot product.

**Recommendation:** Trust SIMPLNX for unit-direction output. At the sign boundary, use the documented unit-magnitude invariant; the test does not select one expected direction.

**Evidence:** PR #1438 (`e6896714b`) introduced double orientation calculations and removed the final normalization then present in SIMPLNX. PR #1472 (`413e6fa46`) changed the EbsdLib matrix API. PR #1631 (`ec22ac375`) restored final normalization. Current code narrows each contribution before adding it to the float32 sum; the PR #1631 implementation narrowed the result of the addition. The tagged legacy source and the current algorithm contain the stated arithmetic. `Class 1 Oracle (hand-built dataset)` checks F7 magnitude and the general unit-vector invariant with tolerance `1e-5`. The last ideal F7 axis has zero dot product with the preceding sum. Keeping or flipping the last axis gives unit directions that are 60 degrees apart. The full derivation is in `../provenance/ComputeAvgCAxesFilter.md`.

---

## Part 2 — SIMPLNX defects and behavior changes

### ComputeAvgCAxesFilter-D3

| Field | Value |
|---|---|
| **Deviation ID** | `ComputeAvgCAxesFilter-D3` |
| **Filter UUID** | `453cdb58-7bbb-4576-ad5e-f75a1c54d348` |
| **Status** | active |
| **Affected released versions** | DREAM3D-NX v7.0.0 through v7.4.2. DREAM.3D v6.5.171 was not affected. |
| **Fixed in** | The DREAM3D-NX release after v7.4.2 |

**Symptom:** A participating cell with a negative or out-of-range phase could read outside Crystal Structures.

**Root cause:** Bug. Released algorithms use `crystalStructures[cellPhases[i]]`, or the equivalent lookup through `currentCellPhase`, without a bounds check. The current algorithm rejects a negative phase or a phase at or above the ensemble tuple count with `-76404`. It skips nonpositive FeatureIds before checking the phase. DREAM.3D v6.5.171 has no phase lookup and was not affected.

**Affected users:** Users whose positive-FeatureId cells contain invalid phase indices.

**Recommendation:** Trust SIMPLNX with the fix. Correct any participating phase identified by error `-76404`.

**Evidence:** PR #1568 (`4ec21e93f`) adds the phase bounds check. `Phase Index Bounds` checks phase -1 and phase 2 with two ensemble tuples, on participating and background cells. The current guard is in `src/Plugins/OrientationAnalysis/src/OrientationAnalysis/Filters/Algorithms/ComputeAvgCAxes.cpp`. Direct inspection of that path in every release branch from `upstream/release/v7.0.0` through `upstream/release/v7.4.2` finds the unchecked lookup.

---

### ComputeAvgCAxesFilter-D4

| Field | Value |
|---|---|
| **Deviation ID** | `ComputeAvgCAxesFilter-D4` |
| **Filter UUID** | `453cdb58-7bbb-4576-ad5e-f75a1c54d348` |
| **Status** | active |
| **Affected released versions** | DREAM3D-NX v7.0.0 through v7.4.1. DREAM.3D v6.5.171 was not affected. |
| **Fixed in** | DREAM3D-NX v7.4.2 |

**Symptom:** Converted SIMPL pipelines selected a cell matrix for feature output and supplied AttributeMatrix paths where the filter required phase and crystal-structure arrays.

**Root cause:** Bug. `FromSIMPLJson()` derived the feature matrix from Quats, the phase path from the FeatureIds matrix, and the crystal-structure path from the AvgCAxes matrix. The correction derives the feature matrix from the output AvgCAxes path. It places `Phases` beside FeatureIds and `CrystalStructures` in the same container's `CellEnsembleData`. DREAM.3D v6.5.171 has no SIMPL-to-SIMPLNX conversion path and was not affected.

**Affected users:** Users who import legacy `FindAvgCAxes` pipelines into the affected DREAM3D-NX releases.

**Recommendation:** Trust SIMPLNX with the fix. After conversion, select the phase and crystal-structure arrays manually if their names differ from the conventional names.

**Evidence:** PR #1682 (`5eb69f359`) corrects the converted array paths. `SIMPL Backwards Compatibility` checks six arguments in the `SIMPL 6.5 (UUID)` and `SIMPL 6.4 (Filter_Name)` sections. Both fixtures use `CellData` for the input and output matrix, so they do not isolate the corrected feature-matrix selection. Direct inspection of `src/Plugins/OrientationAnalysis/src/OrientationAnalysis/Filters/ComputeAvgCAxesFilter.cpp` in every release branch finds the wrong mappings through v7.4.1 and the correction in v7.4.2.

---

### ComputeAvgCAxesFilter-D5

| Field | Value |
|---|---|
| **Deviation ID** | `ComputeAvgCAxesFilter-D5` |
| **Filter UUID** | `453cdb58-7bbb-4576-ad5e-f75a1c54d348` |
| **Status** | active |
| **Affected released versions** | DREAM3D-NX v7.0.0 through v7.4.0. DREAM.3D v6.5.171 was not affected. |
| **Fixed in** | DREAM3D-NX v7.4.1 |

**Symptom:** Conversion rejected the legacy AvgCAxes output path instead of extracting its array name.

**Root cause:** Bug. `DataArrayNameFilterParameterConverter` requires a JSON string, but the legacy output parameter is a path object. `DataArrayCreationToDataObjectNameFilterParameterConverter` reads the array-name field from that object. DREAM.3D v6.5.171 has no SIMPL-to-SIMPLNX conversion path and was not affected.

**Affected users:** Users who import legacy `FindAvgCAxes` pipelines into the affected DREAM3D-NX releases.

**Recommendation:** Trust SIMPLNX with the fix. D4 also requires DREAM3D-NX v7.4.2 or later for the array-path mappings.

**Evidence:** PR #1476 (`e45bca2a5`) corrects the output-name converter. `SIMPL Backwards Compatibility` checks the converted output name `TestArray` for both legacy fixtures. The converter implementations are in `src/simplnx/Parameters/DataObjectNameParameter.cpp`. Direct inspection of `ComputeAvgCAxesFilter.cpp` in every release branch finds the string converter through v7.4.0 and the path-object converter in v7.4.1 and v7.4.2.
