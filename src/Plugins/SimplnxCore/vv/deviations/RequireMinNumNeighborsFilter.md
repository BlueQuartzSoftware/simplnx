# Deviations from DREAM3D 6.5.171: RequireMinNumNeighborsFilter

This file lists every documented behavioral difference between SIMPLNX `RequireMinNumNeighborsFilter` and DREAM3D 6.5.171 `MinNeighbors`.

The valid all-phase analytical fixture produced matching FeatureIds, copied and ignored cell arrays, NumNeighbors, and Phases. The deviations below concern negative or out-of-range FeatureIds and a coarsening state that cannot make progress.

---

## RequireMinNumNeighborsFilter-D1

| Field | Value |
|---|---|
| **Deviation ID** | `RequireMinNumNeighborsFilter-D1` |
| **Filter UUID** | `4ab5153f-6014-4e6d-bbd6-194068620389` |
| **Status** | active |

**Symptom:** When a cell has a negative FeatureId and a valid non-negative face neighbor, SIMPLNX reassigns the cell and completes; DREAM3D 6.5.171 performs an out-of-bounds read while marking cells for removal.

**Root cause:** `bug` in DREAM3D 6.5.171. `MinNeighbors::merge_containedfeatures()` uses each FeatureId directly as an index into `activeObjects`, including negative values. SIMPLNX checks `featureId < 0`, leaves that cell for the coarsening pass, and copies from the dominant valid face neighbor.

**Affected users:** Users whose input FeatureIds array already contains negative sentinel values. Inputs containing only valid, non-negative, in-range FeatureIds are unaffected.

**Recommendation:** `trust SIMPLNX`. It handles the sentinel value without an invalid memory access and deterministically reassigns the cell when a valid face neighbor exists.

---

## RequireMinNumNeighborsFilter-D2

| Field | Value |
|---|---|
| **Deviation ID** | `RequireMinNumNeighborsFilter-D2` |
| **Filter UUID** | `4ab5153f-6014-4e6d-bbd6-194068620389` |
| **Status** | active |

**Symptom:** When a non-negative FeatureId is greater than or equal to the number of feature tuples, SIMPLNX returns error `-55567`; DREAM3D 6.5.171 uses the value as an out-of-bounds `activeObjects` index.

**Root cause:** `bug` in DREAM3D 6.5.171. The legacy marking pass does not validate FeatureIds before indexing its feature-state vector. SIMPLNX validates the value before the first `activeObjects` lookup and stops with a deterministic error.

**Affected users:** Users with malformed FeatureIds arrays or FeatureIds that do not correspond to the supplied feature Attribute Matrix. Valid in-range FeatureIds are unaffected.

**Recommendation:** `trust SIMPLNX`. The explicit `-55567` failure prevents undefined behavior and identifies the invalid value and array.

---

## RequireMinNumNeighborsFilter-D3

| Field | Value |
|---|---|
| **Deviation ID** | `RequireMinNumNeighborsFilter-D3` |
| **Filter UUID** | `4ab5153f-6014-4e6d-bbd6-194068620389` |
| **Status** | active |

**Symptom:** When negative cells remain and none has a non-negative face neighbor, SIMPLNX returns error `-55572`; DREAM3D 6.5.171 continues the coarsening loop indefinitely.

**Root cause:** `bug` in DREAM3D 6.5.171. The legacy loop assumes every rejected cell will eventually reach a retained feature and does not detect an iteration that fills no cells. SIMPLNX tracks whether the current iteration found any fill candidate and returns `-55572` when unresolved cells remain without progress.

**Affected users:** Users with disconnected negative regions that have no retained-feature seed, including an all-negative FeatureIds array paired with a feature table that still contains an active feature.

**Recommendation:** `trust SIMPLNX`. The deterministic error prevents an unbounded execution and tells the user how to correct the input.
