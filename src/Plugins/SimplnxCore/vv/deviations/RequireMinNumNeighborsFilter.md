# Deviations from DREAM.3D v6.5.171: RequireMinNumNeighborsFilter

Part 1 lists the three source-confirmed differences from `MinNeighbors` in DREAM.3D v6.5.171. The same defects affected SIMPLNX. DREAM3D-NX v7.4.2 contains all three fixes.

Entries use the stable `RequireMinNumNeighborsFilter-D<N>` sequence. The Filter UUID is the permanent cross-reference anchor.

---

## Part 1 — Differences from DREAM.3D v6.5.171

### RequireMinNumNeighborsFilter-D1

| Field | Value |
|---|---|
| **Deviation ID** | `RequireMinNumNeighborsFilter-D1` |
| **Filter UUID** | `4ab5153f-6014-4e6d-bbd6-194068620389` |
| **Status** | active |
| **Affected released versions** | DREAM3D-NX v7.0.0 through v7.4.1. DREAM.3D v6.5.171 was affected. |
| **Fixed in** | DREAM3D-NX v7.4.2 |

**Symptom:** When a cell has a negative FeatureId and a valid non-negative face neighbor, SIMPLNX reassigns the cell and completes; DREAM.3D v6.5.171 performs an out-of-bounds read while marking cells for removal.

**Root cause:** `bug` in DREAM.3D v6.5.171. `MinNeighbors::merge_containedfeatures()` uses each FeatureId directly as an index into `activeObjects`, including negative values. SIMPLNX guards the lookup with a nonnegative-ID check, marks the cell as `-1`, and copies from the dominant valid face neighbor.

**Affected users:** Users whose input FeatureIds array already contains negative sentinel values. Inputs containing only valid, non-negative, in-range FeatureIds are unaffected.

**Recommendation:** `trust SIMPLNX`. It handles the sentinel value without an invalid memory access and deterministically reassigns the cell when a valid face neighbor exists.

**Evidence:** `src/Plugins/SimplnxCore/test/RequireMinNumNeighborsTest.cpp::Execute - negative feature ID is reassigned` checks this correction. Source at each release from `upstream/release/v7.0.0` through `upstream/release/v7.4.1` contains the defect; `upstream/release/v7.4.2` contains the fix from PR #1694 (`0c8fa9f59`). DREAM.3D `v6.5.171`, `Source/Plugins/Processing/ProcessingFilters/MinNeighbors.cpp`, contains the defective path.

---

### RequireMinNumNeighborsFilter-D2

| Field | Value |
|---|---|
| **Deviation ID** | `RequireMinNumNeighborsFilter-D2` |
| **Filter UUID** | `4ab5153f-6014-4e6d-bbd6-194068620389` |
| **Status** | active |
| **Affected released versions** | DREAM3D-NX v7.0.0 through v7.4.1. DREAM.3D v6.5.171 was affected. |
| **Fixed in** | DREAM3D-NX v7.4.2 |

**Symptom:** When a non-negative FeatureId is greater than or equal to the number of feature tuples, SIMPLNX returns error `-55567`; DREAM.3D v6.5.171 uses the value as an out-of-bounds `activeObjects` index.

**Root cause:** `bug` in DREAM.3D v6.5.171. The legacy marking pass does not validate FeatureIds before indexing its feature-state vector. SIMPLNX validates the value before the first `activeObjects` lookup and stops with a deterministic error.

**Affected users:** Users with malformed FeatureIds arrays or FeatureIds that do not correspond to the supplied feature Attribute Matrix. Valid in-range FeatureIds are unaffected.

**Recommendation:** `trust SIMPLNX`. The explicit `-55567` failure prevents undefined behavior and identifies the invalid value and array.

**Evidence:** `src/Plugins/SimplnxCore/test/RequireMinNumNeighborsTest.cpp::Execute Error - feature ID out of range (-55567)` checks this correction. Source at each release from `upstream/release/v7.0.0` through `upstream/release/v7.4.1` contains the defect; `upstream/release/v7.4.2` contains the fix from PR #1694 (`0c8fa9f59`). DREAM.3D `v6.5.171`, `Source/Plugins/Processing/ProcessingFilters/MinNeighbors.cpp`, contains the defective path.

---

### RequireMinNumNeighborsFilter-D3

| Field | Value |
|---|---|
| **Deviation ID** | `RequireMinNumNeighborsFilter-D3` |
| **Filter UUID** | `4ab5153f-6014-4e6d-bbd6-194068620389` |
| **Status** | active |
| **Affected released versions** | DREAM3D-NX v7.0.0 through v7.4.1. DREAM.3D v6.5.171 was affected. |
| **Fixed in** | DREAM3D-NX v7.4.2 |

**Symptom:** When negative cells remain and none has a non-negative face neighbor, SIMPLNX returns error `-55572`; DREAM.3D v6.5.171 continues the coarsening loop indefinitely.

**Root cause:** `bug` in DREAM.3D v6.5.171. The legacy loop assumes every rejected cell will eventually reach a retained feature and does not detect an iteration that fills no cells. SIMPLNX counts the current scan's fill candidates and returns `-55572` when unresolved cells remain without progress.

**Affected users:** Users with disconnected negative regions that have no retained-feature seed, including an all-negative FeatureIds array paired with a feature table that still contains an active feature.

**Recommendation:** `trust SIMPLNX`. The deterministic error prevents an unbounded execution and tells the user how to correct the input.

**Evidence:** `src/Plugins/SimplnxCore/test/RequireMinNumNeighborsTest.cpp::Execute Error - no coarsening progress (-55572)` checks this correction. Source at each release from `upstream/release/v7.0.0` through `upstream/release/v7.4.1` contains the defect; `upstream/release/v7.4.2` contains the fix from PR #1694 (`0c8fa9f59`). DREAM.3D `v6.5.171`, `Source/Plugins/Processing/ProcessingFilters/MinNeighbors.cpp`, contains the defective path.
