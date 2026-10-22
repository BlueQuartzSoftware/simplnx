# Deviations from DREAM.3D v6.5.171: RegularizeZSpacingFilter

Part 1 records differences confirmed by source comparison with `RegularizeZSpacing` in the Sampling plugin. Entries use stable IDs from one sequence. The Filter UUID is the permanent cross-reference anchor.

---

## Part 1 — Differences from DREAM.3D v6.5.171

### RegularizeZSpacingFilter-D1

| Field | Value |
|---|---|
| **Deviation ID** | `RegularizeZSpacingFilter-D1` |
| **Filter UUID** | `d6599986-1932-4bfc-993d-71eafefe6db0` |
| **Status** | active |

**Symptom:** A converted pipeline selects the geometry's assigned cell AttributeMatrix, even if its legacy parameter names another AttributeMatrix.

**Root cause:** Algorithmic choice. DREAM.3D v6.5.171 selects cell data through the full `CellAttributeMatrixPath`. SIMPLNX selects an Image Geometry and reads its assigned cell AttributeMatrix. `FromSIMPLJson()` keeps only the DataContainer name from the legacy path. With one matching cell AttributeMatrix, this preserves the selection. With multiple cell AttributeMatrices, it can select different data. A stale AttributeMatrix name that DREAM.3D rejects does not survive conversion. SIMPLNX returns `-5560` when the geometry has no assigned cell AttributeMatrix and `-5561` for a non-DataArray cell member.

**Affected users:** Users who convert pipelines with multiple cell AttributeMatrices or a stale AttributeMatrix name.

**Recommendation:** Trust SIMPLNX after checking the converted selection. Assign the intended cell AttributeMatrix to the selected Image Geometry before execution.

**Evidence:** `src/Plugins/SimplnxCore/src/SimplnxCore/Filters/RegularizeZSpacingFilter.cpp` uses `DataArraySelectionToGeometrySelectionFilterParameterConverter`, whose implementation in `src/simplnx/Parameters/GeometrySelectionParameter.cpp` retains only the DataContainer name. DREAM.3D v6.5.171 `Source/Plugins/Sampling/SamplingFilters/RegularizeZSpacing.cpp` validates and uses the full AttributeMatrix path. `SIMPL Backwards Compatibility` in `src/Plugins/SimplnxCore/test/RegularizeZSpacingTest.cpp` checks the converted geometry path.

---

### RegularizeZSpacingFilter-D2

| Field | Value |
|---|---|
| **Deviation ID** | `RegularizeZSpacingFilter-D2` |
| **Filter UUID** | `d6599986-1932-4bfc-993d-71eafefe6db0` |
| **Status** | active |

**Symptom:** SIMPLNX rejects incomplete or nonnumeric boundary files, decreasing boundaries and a non-positive final boundary that DREAM.3D v6.5.171 does not explicitly reject.

**Root cause:** Algorithmic choice. SIMPLNX checks every requested extraction and returns `-5557` if it fails. Preflight requires nondecreasing boundaries (`-5558`) and a positive final boundary (`-5559`). DREAM.3D v6.5.171 reads values without checking extraction success, order or the final boundary. Both implementations reject non-positive requested spacing and an input file that cannot be opened.

**Affected users:** Users whose Z-position files are incomplete, contain nonnumeric tokens among the required values, decrease between boundaries, or end at zero or a negative value.

**Recommendation:** Trust SIMPLNX. Supply at least `ZPoints + 1` parseable boundary values in nondecreasing order, with a positive final boundary.

**Evidence:** Compare `ReadZBoundsFile()` in `src/Plugins/SimplnxCore/src/SimplnxCore/Filters/Algorithms/RegularizeZSpacing.cpp` and `preflightImpl()` in `src/Plugins/SimplnxCore/src/SimplnxCore/Filters/RegularizeZSpacingFilter.cpp` with `dataCheck()` and `execute()` in DREAM.3D v6.5.171 `Source/Plugins/Sampling/SamplingFilters/RegularizeZSpacing.cpp`. The `Invalid Parameters` test checks short files, decreasing boundaries and zero extent with their exact error codes.

---

### RegularizeZSpacingFilter-D3

| Field | Value |
|---|---|
| **Deviation ID** | `RegularizeZSpacingFilter-D3` |
| **Filter UUID** | `d6599986-1932-4bfc-993d-71eafefe6db0` |
| **Status** | active |

**Symptom:** SIMPLNX can keep the input geometry and write the resampled data to a separate geometry.

**Root cause:** Algorithmic choice. `Perform In Place` defaults to true. When false, SIMPLNX creates the requested output geometry and retains the input. DREAM.3D v6.5.171 replaces the selected cell AttributeMatrix and changes the input geometry dimensions and spacing. SIMPLNX preserves the same strict `>` plane-selection rule and output Z-dimension calculation in both modes.

**Affected users:** Users who disable `Perform In Place` and expect the input geometry to contain the resampled data.

**Recommendation:** Trust SIMPLNX. Select the created geometry in downstream filters, or keep `Perform In Place` enabled for the legacy output arrangement.

**Evidence:** `src/Plugins/SimplnxCore/src/SimplnxCore/Filters/RegularizeZSpacingFilter.cpp` creates output geometry and applies deferred replacement actions only in in-place mode. `Valid Execution (New Geometry)` and `Valid Execution (In Place)` in `src/Plugins/SimplnxCore/test/RegularizeZSpacingTest.cpp` check the two output arrangements. DREAM.3D v6.5.171 `Source/Plugins/Sampling/SamplingFilters/RegularizeZSpacing.cpp::execute()` replaces the selected AttributeMatrix.
