# Deviations from DREAM3D 6.5.171: CreateFeatureArrayFromElementArrayFilter

This file lists every documented behavioral difference between this SIMPLNX filter and its DREAM3D 6.5.171 equivalent (`CreateFeatureArrayFromElementArray`, SIMPL UUID `94438019-21bb-5b61-a7c3-66974b9a34dc`).

Entries are referenced by stable ID (`CreateFeatureArrayFromElementArrayFilter-D<N>`) from the V&V report and from public migration guidance. The ID is stable across renames; the Filter UUID field is the permanent cross-reference anchor.

---

## CreateFeatureArrayFromElementArrayFilter-D1

| Field | Value |
|---|---|
| **Deviation ID** | `CreateFeatureArrayFromElementArrayFilter-D1` |
| **Filter UUID** | `50e1be47-b027-4f40-8f70-1283682ee3e7` |
| **Status** | active |

**Symptom:** When the destination Feature Attribute Matrix's tuple count does not equal `max(FeatureIds) + 1`, DREAM3D 6.5.171 stops with an error (`-5555` when a Feature Id indexes past the matrix, `-5556` when the sizes merely disagree) while SIMPLNX succeeds by resizing the Attribute Matrix.

**Root cause:** Algorithmic choice. 6.5.171 `execute()` validates `largestFeature == totalFeatures - 1` against the pre-existing Attribute Matrix and refuses to run otherwise (`SIMPL/Source/SIMPLib/CoreFilters/CreateFeatureArrayFromElementArray.cpp`, lines 238–269). SIMPLNX deliberately resizes the destination Attribute Matrix — and every array inside it — to `max(FeatureIds) + 1` tuples at execute time (`Algorithms/CreateFeatureArrayFromElementArray.cpp`). On correctly pre-sized inputs the two implementations produce identical output; the deviation is only observable on mis-sized inputs, where legacy errors and SIMPLNX proceeds.

**Affected users:** Anyone migrating a 6.5 pipeline that relied on the legacy error to catch a wrongly selected Feature Attribute Matrix. Note the SIMPLNX resize truncates sibling Feature arrays when the selected Attribute Matrix was LARGER than `max(FeatureIds) + 1` — users who select the wrong Feature Attribute Matrix can silently lose data that legacy would have protected with an error. This is called out in the filter documentation.

**Recommendation:** Trust SIMPLNX. The resize behavior is the documented, intended behavior in DREAM3D-NX (the unit test suite pins it), and on any input the legacy filter accepted, the outputs are identical. Users should select the Feature Attribute Matrix generated from the same FeatureIds array they pass to this filter.

---

## CreateFeatureArrayFromElementArrayFilter-D2

| Field | Value |
|---|---|
| **Deviation ID** | `CreateFeatureArrayFromElementArrayFilter-D2` |
| **Filter UUID** | `50e1be47-b027-4f40-8f70-1283682ee3e7` |
| **Status** | active |

**Symptom:** Negative Feature Ids cause DREAM3D 6.5.171 to write out of bounds (undefined behavior — silent memory corruption or a crash), while SIMPLNX stops with error `-5570`.

**Root cause:** Bug in 6.5.171. The legacy copy kernel computes the destination pointer as `fPtr + (numComp * featureIdx)` with no sign check, so a negative Feature Id writes before the start of the destination array (`copyCellData()` in the legacy source). The legacy execute-time validation scans only for the *largest* Feature Id and never rejects negative values. SIMPLNX previously shared the fault (an unsigned wrap in the destination index) and now rejects negative Feature Ids at execute time as part of this V&V pass.

**Affected users:** Anyone whose FeatureIds array contains negative values (e.g., imported data using `-1` as a "no feature" sentinel). In 6.5.171 the result was undefined; in SIMPLNX the filter now fails with a clear error message.

**Recommendation:** Trust SIMPLNX. A deterministic error is strictly better than undefined behavior; users with `-1` sentinels should remap them (e.g., to 0) before running the filter.

---

## CreateFeatureArrayFromElementArrayFilter-D3

| Field | Value |
|---|---|
| **Deviation ID** | `CreateFeatureArrayFromElementArrayFilter-D3` |
| **Filter UUID** | `50e1be47-b027-4f40-8f70-1283682ee3e7` |
| **Status** | active |

**Symptom:** When the FeatureIds array and the selected element array have different tuple counts, DREAM3D 6.5.171 reads out of bounds (undefined behavior), while SIMPLNX fails preflight with error `-5571`.

**Root cause:** Bug in 6.5.171. The legacy kernel iterates over the element array's tuple count and indexes `featureIds[i]` without verifying the FeatureIds array is at least as long; the two arrays were not required to live in the same Attribute Matrix, so mismatched lengths were reachable. SIMPLNX now requires identical tuple counts at preflight.

**Affected users:** Anyone selecting a FeatureIds array from a different (smaller) Attribute Matrix than the element array. In 6.5.171 the result was undefined; in SIMPLNX the pipeline fails preflight with a clear message.

**Recommendation:** Trust SIMPLNX. Select FeatureIds and element arrays with matching tuple counts (normally siblings in the same Cell Attribute Matrix).

---

## CreateFeatureArrayFromElementArrayFilter-D4

| Field | Value |
|---|---|
| **Deviation ID** | `CreateFeatureArrayFromElementArrayFilter-D4` |
| **Filter UUID** | `50e1be47-b027-4f40-8f70-1283682ee3e7` |
| **Status** | active |

**Symptom:** On floating-point element arrays containing NaN values, DREAM3D 6.5.171 emits the spurious "-1000 Elements from Feature N do not all have the same value" warning for a feature whose cells are all NaN (perfectly consistent data), while SIMPLNX does not. Output values are identical in both versions — NaN tuples are copied verbatim.

**Root cause:** Bug in 6.5.171 (cosmetic). The legacy consistency check uses `currentDataPtr[j] != cSourcePtr[j]`, and `NaN != NaN` is true by IEEE-754 semantics, so an all-NaN feature triggers the warning on its first repeat cell. Because both implementations emit at most ONE warning total, the legacy spurious warning also masks any *real* inconsistency elsewhere in the dataset. SIMPLNX treats two NaNs as consistent (added during this V&V pass) so the single warning is reserved for genuine inconsistencies.

**Affected users:** Anyone running the filter on float32/float64 element arrays with NaN padding (common for masked or unindexed regions). Only the warning text differs; the created array is bit-identical between versions.

**Recommendation:** Trust SIMPLNX. The legacy warning was a false positive that could hide a true positive.
