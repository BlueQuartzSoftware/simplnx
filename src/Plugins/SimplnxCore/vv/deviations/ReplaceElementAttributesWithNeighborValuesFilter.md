# Deviations from DREAM.3D v6.5.171: ReplaceElementAttributesWithNeighborValuesFilter

Part 1 lists the numeric-selection difference from DREAM.3D v6.5.171. Part 2 lists the fixed sibling-cast defect and the unfixed loop-termination defect. All entries use one stable ID sequence.

---

## Part 1 — Differences from DREAM.3D v6.5.171

### ReplaceElementAttributesWithNeighborValuesFilter-D1

| Field | Value |
|---|---|
| **Deviation ID** | `ReplaceElementAttributesWithNeighborValuesFilter-D1` |
| **Filter UUID** | `65128c53-d3be-4a69-a559-32a48d603884` |
| **Status** | active |

**Symptom:** A float64 or large-integer comparison array can select a different neighbor in SIMPLNX than in DREAM.3D v6.5.171.

**Root cause:** Bug. DREAM.3D v6.5.171 stores the best candidate in a `float`; affected DREAM3D-NX releases use `float32`. The comparison then converts this rounded value back to the array type. For LessThan at threshold 0.5, two passing float64 candidates encountered as 1.00000004 then 1.00000002 both round to 1.0 in the temporary. The second candidate replaces the larger first candidate. SIMPLNX now uses `T best` and `T& best`, so it retains the larger value.

**Affected users:** Users with comparison values that lose precision when converted to float32. DREAM3D-NX v7.0.0 through v7.4.1 were affected. DREAM.3D v6.5.171 was affected.

**Recommendation:** Trust SIMPLNX. DREAM3D-NX v7.4.2 contains the fix and selects the best candidate in the input array's numeric type.

**Evidence:** PR #1680 (`4fd0abee9`) changes the temporary and reference from float32 to `T`. Direct inspection of all `upstream/release/v7.0.0` through `upstream/release/v7.4.1` algorithm snapshots finds float32; `upstream/release/v7.4.2` has `T`. The DREAM.3D `v6.5.171` source is `Source/Plugins/OrientationAnalysis/OrientationAnalysisFilters/ReplaceElementAttributesWithNeighborValues.cpp`. The current test file uses float32 comparison arrays and has no precision-specific regression fixture.

---

## Part 2 — SIMPLNX defects and behavior changes

### ReplaceElementAttributesWithNeighborValuesFilter-D2

| Field | Value |
|---|---|
| **Deviation ID** | `ReplaceElementAttributesWithNeighborValuesFilter-D2` |
| **Filter UUID** | `65128c53-d3be-4a69-a559-32a48d603884` |
| **Status** | active |
| **Affected released versions** | DREAM3D-NX v7.0.0 through v7.4.1. DREAM.3D v6.5.171 was not affected. |
| **Fixed in** | DREAM3D-NX v7.4.2 |

**Symptom:** Replacing a cell could throw `std::bad_cast` when the cell AttributeMatrix also contained a nonnumeric object, such as a `StringArray` or `NeighborList`.

**Root cause:** Bug. The copy loop applied `dynamic_cast<IDataArray&>` to every child without checking its type. SIMPLNX now uses a pointer cast and skips null results. It copies numeric `IDataArray` siblings; other children remain unchanged. DREAM.3D v6.5.171 calls the legacy `IDataArray::copyTuple()` interface without the failing SIMPLNX reference cast.

**Affected users:** Users whose cell AttributeMatrix contains nonnumeric children and at least one cell selected for replacement.

**Recommendation:** Trust SIMPLNX. Use DREAM3D-NX v7.4.2 or later to avoid this cast failure. Handle nonnumeric sibling values separately if they must follow the copied numeric tuples.

**Evidence:** PR #1680 (`4fd0abee9`) replaces the unchecked cast with typed child collection. Every release snapshot from `upstream/release/v7.0.0` through `upstream/release/v7.4.1` has the unchecked reference cast; `upstream/release/v7.4.2` has typed collection. The current algorithm retains the protection with a checked pointer cast. The DREAM.3D `v6.5.171` source uses the legacy array interface. No current test fixture includes a nonnumeric sibling.

---

### ReplaceElementAttributesWithNeighborValuesFilter-D3

| Field | Value |
|---|---|
| **Deviation ID** | `ReplaceElementAttributesWithNeighborValuesFilter-D3` |
| **Filter UUID** | `65128c53-d3be-4a69-a559-32a48d603884` |
| **Status** | active |
| **Affected released versions** | DREAM3D-NX v7.0.0 through v7.4.2. DREAM.3D v6.5.171 was affected. |
| **Fixed in** | Not fixed |

**Symptom:** With loop mode enabled, an image whose comparison values all fail the threshold runs until cancelled.

**Root cause:** Bug. The loop counts failing cells, not completed replacements. If no neighbor passes the threshold, no value changes, but `loopUntilDone && count > 0` starts another pass. The algorithm has no check for a pass without progress and no iteration limit.

**Affected users:** Users who enable Loop Until Gone on data with no passing comparison value, or on a failing region that cannot receive a passing value.

**Recommendation:** Disable Loop Until Gone for such inputs. A single pass returns with cells unchanged when they have no passing face neighbor.

**Evidence:** All ten release snapshots from `upstream/release/v7.0.0` through `upstream/release/v7.4.2`, the current algorithm, and the DREAM.3D `v6.5.171` source repeat based on the count of failing cells. The current algorithm's API comment also identifies this nontermination condition. The synthetic surrounded-center section disables the loop and checks one pass; it does not test termination with no passing cells.
