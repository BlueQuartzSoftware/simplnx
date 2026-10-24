# Deviations from DREAM.3D v6.5.171: ComputeFeaturePhasesFilter

Part 1 lists the three differences from `FindFeaturePhases` in DREAM.3D v6.5.171. Entries retain the stable IDs `ComputeFeaturePhasesFilter-D1` through `ComputeFeaturePhasesFilter-D3`. The Filter UUID is the permanent cross-reference anchor.

## Part 1 — Differences from DREAM.3D v6.5.171

### ComputeFeaturePhasesFilter-D1

| Field | Value |
|---|---|
| **Deviation ID** | `ComputeFeaturePhasesFilter-D1` |
| **Filter UUID** | `da5bb20e-4a8e-49d9-9434-fbab7bc434fc` |
| **Status** | active |

**Symptom:** SIMPLNX writes zero to feature zero. DREAM.3D v6.5.171 writes the phase of the last background cell.

**Root cause:** Algorithmic choice. `ComputeFeaturePhases.cpp` skips cells with FeatureId zero and writes a feature cache initialized to zero. `FindFeaturePhases::execute()` in DREAM.3D v6.5.171 assigns every cell phase, including background cells. This is a deliberate background convention, not a defect in the last-cell rule for positive features.

**Affected users:** Users who read feature zero. DREAM3D-NX v7.4.2 introduces background exclusion; DREAM3D-NX v7.0.0 through v7.4.1 process background cells as DREAM.3D v6.5.171 does.

**Recommendation:** Trust SIMPLNX. Treat feature zero as background, not as a measured phase for a positive feature.

**Evidence:** `Valid: Feature 0 Skip` checks output `[0,1,2]`. `Valid: Gap Feature Stays Zero` checks the separate unused-feature rule. Source comparison uses DREAM.3D tag `v6.5.171`, `Source/Plugins/Generic/GenericFilters/FindFeaturePhases.cpp`, and each SIMPLNX release source listed in `../provenance/ComputeFeaturePhasesFilter.md`. `vv_work:ComputeFeaturePhasesFilter/compare_outputs.py` specifies Case C expectations but is not a saved execution result.

---

### ComputeFeaturePhasesFilter-D2

| Field | Value |
|---|---|
| **Deviation ID** | `ComputeFeaturePhasesFilter-D2` |
| **Filter UUID** | `da5bb20e-4a8e-49d9-9434-fbab7bc434fc` |
| **Status** | active |
| **Affected released versions** | DREAM3D-NX v7.0.0 through v7.4.1. DREAM.3D v6.5.171 was affected. |
| **Fixed in** | DREAM3D-NX v7.4.2 |

**Symptom:** Earlier versions could copy a negative phase into a positive feature without reporting an error. SIMPLNX now returns `-61861` for this input.

**Root cause:** Bug. Earlier implementations copied signed cell phases without checking for negative values. The current algorithm checks each phase after excluding background cells and before updating its feature cache. Storing a negative integer is defined arithmetic; the defect was accepting an invalid phase value.

**Affected users:** Users with negative cell phases on positive features in DREAM3D-NX v7.0.0 through v7.4.1 or DREAM.3D v6.5.171. Negative phases on background cells are excluded from the current check.

**Recommendation:** Trust SIMPLNX. Correct the negative phase before processing a positive feature; DREAM3D-NX v7.4.2 contains the validation fix.

**Evidence:** `Invalid: Negative Cell Phase` checks `-61861`; `Valid: Negative Cell Phase On Background Cell` checks success and output `[0,2]`. Direct source inspection finds no phase-sign guard in the nine release snapshots before v7.4.2 or in DREAM.3D tag `v6.5.171`; v7.4.2 contains the guard added by `bbc470e6d` (PR #1672). The release table in `../provenance/ComputeFeaturePhasesFilter.md` identifies the source paths.

---

### ComputeFeaturePhasesFilter-D3

| Field | Value |
|---|---|
| **Deviation ID** | `ComputeFeaturePhasesFilter-D3` |
| **Filter UUID** | `da5bb20e-4a8e-49d9-9434-fbab7bc434fc` |
| **Status** | active |

**Symptom:** Mixed phases produce different warning text under the same warning code, `-500`.

**Root cause:** Algorithmic choice. DREAM.3D v6.5.171 counts cells whose phase differs from the first phase of each feature. It combines a header and one line per affected feature in one warning. SIMPLNX collects conflicting feature IDs in a sorted set, lists up to 15 IDs and appends the omitted-feature count. Consecutive phase comparisons identify the same conflicting positive features as comparison with the first phase. Both implementations retain the last phase.

**Affected users:** Tools that parse warning text. DREAM3D-NX v7.0.0 through v7.2.2 emit a header warning and separate feature-count warnings. DREAM3D-NX v7.4.0 and v7.4.1 use one unlimited ID list; v7.4.2 introduces the 15-ID limit.

**Recommendation:** Trust SIMPLNX. Use warning code `-500` to identify mixed phases; avoid parsing the message body as a stable interface.

**Evidence:** `Valid: Mixed Phase Warning` checks one warning, ID 1 and final phase 2. `Valid: Truncated Mixed Feature Warning` checks IDs 1 through 15 and the omitted count of one. DREAM.3D `FindFeaturePhases.cpp` at tag `v6.5.171` contains the per-feature conflict counts. The release-source table in `../provenance/ComputeFeaturePhasesFilter.md` records the warning changes.
