# Deviations from DREAM.3D v6.5.171: CreateFeatureArrayFromElementArrayFilter

Part 1 lists feature-matrix sizing and input-validation differences from DREAM.3D v6.5.171. Part 2 lists the fixed empty-array and data-truncation defects in SIMPLNX.

The existing ID `CreateFeatureArrayFromElementArray-D1` is unchanged. New entries continue the same number sequence with the full filter name. The Filter UUID is the permanent cross-reference anchor.

---

## Part 1 — Differences from DREAM.3D v6.5.171

### CreateFeatureArrayFromElementArray-D1

| Field | Value |
|---|---|
| **Deviation ID** | `CreateFeatureArrayFromElementArray-D1` |
| **Filter UUID** | `50e1be47-b027-4f40-8f70-1283682ee3e7` |
| **Status** | active |

**Symptom:** SIMPLNX accepts feature-matrix sizes that DREAM.3D v6.5.171 rejects.

**Root cause:** Algorithmic choice. DREAM.3D v6.5.171 requires the matrix tuple count to equal the largest FeatureId plus one. A positive ID beyond the matrix gives error `-5555`; an oversized matrix gives error `-5556`. SIMPLNX grows an undersized matrix and its children after checking for child data loss. It preserves a matrix that already has enough tuples. For successful execution with nonnegative IDs, the output tuple count is `max(initial matrix tuple count, largest FeatureId + 1)`.

**Affected users:** Users whose feature matrix has fewer or more tuples than the FeatureId range requires. Growth can fail if a child array would lose tuples; see `CreateFeatureArrayFromElementArrayFilter-D5`.

**Recommendation:** Trust SIMPLNX. Size the matrix to the FeatureId range for compatibility with DREAM.3D v6.5.171. SIMPLNX retains larger matrices and sets unused output tuples to zero.

**Evidence:** The legacy `Source/SIMPLib/CoreFilters/CreateFeatureArrayFromElementArray.cpp` contains both size errors. `vv_work:CreateFeatureArrayFromElementArrayFilter/results/legacy_oversized_run.log` records `-5556` on 2026-07-23. The SIMPLNX behavior in that run precedes the growth-only correction; current sizing is defined by `Algorithms/CreateFeatureArrayFromElementArray.cpp`. AF-1, AF-3 and AF-6 check growth; no current fixture checks an oversized matrix.

---

### CreateFeatureArrayFromElementArrayFilter-D2

| Field | Value |
|---|---|
| **Deviation ID** | `CreateFeatureArrayFromElementArrayFilter-D2` |
| **Filter UUID** | `50e1be47-b027-4f40-8f70-1283682ee3e7` |
| **Status** | active |
| **Affected released versions** | DREAM3D-NX v7.0.0 through v7.4.1. DREAM.3D v6.5.171 was affected. |
| **Fixed in** | DREAM3D-NX v7.4.2 |

**Symptom:** A negative FeatureId could cause an access outside the output array and corrupt memory.

**Root cause:** Bug. Earlier SIMPLNX code used an ID as an output index without rejecting negative values. DREAM.3D v6.5.171 also uses the signed ID in destination-pointer arithmetic. Its maximum-ID check does not reject negative IDs when the matrix size otherwise passes. SIMPLNX now scans every ID and returns `-81880` for a negative value before the copy starts.

**Affected users:** Users with negative IDs, including inputs that mix negative and nonnegative IDs.

**Recommendation:** Trust SIMPLNX. Correct negative IDs before execution. Use DREAM3D-NX v7.4.2 or later for the input check.

**Evidence:** AF-4 and AF-8 assert `-81880`. The release-source checks in `../provenance/CreateFeatureArrayFromElementArrayFilter.md` show the missing validation through v7.4.1 and the fix in v7.4.2. The legacy copy loop and size checks confirm the same unsafe negative-index path.

---

### CreateFeatureArrayFromElementArrayFilter-D3

| Field | Value |
|---|---|
| **Deviation ID** | `CreateFeatureArrayFromElementArrayFilter-D3` |
| **Filter UUID** | `50e1be47-b027-4f40-8f70-1283682ee3e7` |
| **Status** | active |
| **Affected released versions** | DREAM3D-NX v7.0.0 through v7.4.1. DREAM.3D v6.5.171 was affected. |
| **Fixed in** | DREAM3D-NX v7.4.2 |

**Symptom:** An element array longer than FeatureIds could cause a read beyond FeatureIds.

**Root cause:** Bug. Both earlier implementations looped over the element tuple count and indexed FeatureIds without checking equal tuple counts. DREAM.3D v6.5.171 `dataCheck()` validates the selected arrays but does not compare their lengths. SIMPLNX preflight now rejects unequal counts with `-81883`.

**Affected users:** Users who select element and ID arrays with different tuple counts, including arrays in separate attribute matrices.

**Recommendation:** Trust SIMPLNX. Supply one FeatureId per element tuple. Use DREAM3D-NX v7.4.2 or later for the preflight check.

**Evidence:** AF-9 supplies four element tuples and two IDs and asserts `-81883`. Release sources through v7.4.1 omit the check; v7.4.2 contains it. The legacy `dataCheck()` and `copyCellData()` confirm the unchecked length relationship.

---

## Part 2 — SIMPLNX defects and behavior changes

### CreateFeatureArrayFromElementArrayFilter-D4

| Field | Value |
|---|---|
| **Deviation ID** | `CreateFeatureArrayFromElementArrayFilter-D4` |
| **Filter UUID** | `50e1be47-b027-4f40-8f70-1283682ee3e7` |
| **Status** | active |
| **Affected released versions** | DREAM3D-NX v7.0.0 through v7.4.1. DREAM.3D v6.5.171 was not affected. |
| **Fixed in** | DREAM3D-NX v7.4.2 |

**Symptom:** Empty FeatureIds could cause an out-of-bounds read during maximum-ID lookup.

**Root cause:** Bug. Earlier SIMPLNX code called `std::max_element` on an empty range, converted the returned end iterator to index zero, and read that index from the empty store. The invalid read caused the defect; the empty-range search itself was valid. The current algorithm returns `-81882` before scanning. DREAM.3D v6.5.171 uses a loop with a zero iteration count for empty IDs and has no maximum-element dereference.

**Affected users:** Users with empty element and FeatureIds arrays.

**Recommendation:** Trust SIMPLNX. Supply nonempty input arrays. Use DREAM3D-NX v7.4.2 or later for the empty-input check.

**Evidence:** AF-7 asserts successful preflight followed by error `-81882`. Release sources through v7.4.1 index the empty array; v7.4.2 has the guard. The legacy `execute()` source confirms that this maximum-lookup defect does not occur there.

---

### CreateFeatureArrayFromElementArrayFilter-D5

| Field | Value |
|---|---|
| **Deviation ID** | `CreateFeatureArrayFromElementArrayFilter-D5` |
| **Filter UUID** | `50e1be47-b027-4f40-8f70-1283682ee3e7` |
| **Status** | active |
| **Affected released versions** | DREAM3D-NX v7.0.0 through v7.4.1. DREAM.3D v6.5.171 was not affected. |
| **Fixed in** | DREAM3D-NX v7.4.2 |

**Symptom:** Resizing the feature matrix to the ID range could delete trailing tuples from existing child arrays.

**Root cause:** Bug. Earlier SIMPLNX code always resized the output and feature matrix to the largest ID plus one. The current algorithm only grows the matrix. Before growth, it rejects a child array larger than the target with `-81881`. DREAM.3D v6.5.171 rejects a mismatched matrix size and does not resize its children.

**Affected users:** Users with an oversized feature matrix, or a child array whose tuple count exceeds the proposed growth target.

**Recommendation:** Trust SIMPLNX. Use DREAM3D-NX v7.4.2 or later to preserve existing feature tuples. Correct inconsistent child sizes before execution.

**Evidence:** AF-5 checks the child-array protection error. Release sources through v7.4.1 resize unconditionally; v7.4.2 contains the growth-only condition and child check. `vv_work:CreateFeatureArrayFromElementArrayFilter/notes/LegacyComparison.md` records the former 10-to-5 tuple truncation. No current fixture directly asserts preservation of an oversized matrix.
