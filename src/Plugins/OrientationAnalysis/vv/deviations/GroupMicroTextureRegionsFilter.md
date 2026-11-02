# Deviations from DREAM.3D v6.5.171: GroupMicroTextureRegionsFilter

Part 1 lists differences from the private DREAM.3D v6.5.171 filter source. Part 2 lists SIMPLNX defects. DREAM.3D v6.5.171 does not register this filter for pipeline use.

The stable ID `GroupMicroTextureRegionsFilter-D1` remains reserved and closed: no deviation. Both implementations require the non-contiguous neighbor list only when that option is enabled. D1 is not a confirmed defect.

---

## Part 1 — Differences from DREAM.3D v6.5.171

### GroupMicroTextureRegionsFilter-D2

| Field | Value |
|---|---|
| **Deviation ID** | `GroupMicroTextureRegionsFilter-D2` |
| **Filter UUID** | `3f695987-81b1-47c3-8cff-b49cfa219be0` |
| **Status** | active |

**Symptom:** Parent-ID numbers can differ even when the same Features belong to each region.

**Root cause:** Algorithmic choice. DREAM.3D enables final label randomization and creates a new random generator for each seed search. SIMPLNX uses one generator per execution, disables final label randomization by default, and exposes a fixed seed. The final label permutation preserves membership. A change in seed order can change membership when the running average is enabled.

**Affected users:** Users who compare raw parent IDs, or use those IDs to assign colors. These are differences from the private DREAM.3D filter source.

**Recommendation:** Trust SIMPLNX. Compare region membership separately from label numbers. Enable `RandomizeParentIds` for shuffled labels and keep `UseSeed=true` for repeatable output.

**Evidence:** `GroupMicroTextureRegions::getSeed()` and `execute()` in the DREAM.3D v6.5.171 source define the legacy randomization. `RandomizeParentIds invariants` checks partition preservation and repeatability in SIMPLNX. The 2026-08-27 comparison is recorded in `vv_work:GroupMicroTextureRegionsFilter/rerun/comparison_results.txt`.

---

### GroupMicroTextureRegionsFilter-D3

| Field | Value |
|---|---|
| **Deviation ID** | `GroupMicroTextureRegionsFilter-D3` |
| **Filter UUID** | `3f695987-81b1-47c3-8cff-b49cfa219be0` |
| **Status** | active |

**Symptom:** With running average enabled, DREAM.3D can group a cubic Feature with a hexagonal Feature. SIMPLNX rejects that pair.

**Root cause:** Bug. The DREAM.3D reference Laue index starts at zero and is assigned only when running average is disabled. Zero also denotes the hexagonal-high Laue class. The running-average condition therefore checks only the candidate. SIMPLNX reads both Laue indices before the comparison. DREAM.3D v6.5.171 was affected in its private filter source. No DREAM3D-NX release contains this defect; DREAM3D-NX v7.4.2 contains the correction.

**Affected users:** Users of the private DREAM.3D code with a non-hexagonal Feature adjacent to a hexagonal Feature and running average enabled. The defective condition is equivalent to the corrected condition when both Features are hexagonal-high.

**Recommendation:** Trust SIMPLNX. Both Features must be hexagonal-high before their c-axis angle can permit grouping.

**Evidence:** The tagged legacy `determineGrouping()` places the reference assignment inside `if(!m_UseRunningAverage)`. The v7.4.2 release source assigns both classes unconditionally. `Class 1 Analytical (Mixed-Laue Rejection)` checks 20 pairs. `vv_work:GroupMicroTextureRegionsFilter/rerun/mutation_d3_red.log` records 14 failed pair assertions after a temporary restoration of the defect.

---

### GroupMicroTextureRegionsFilter-D4

| Field | Value |
|---|---|
| **Deviation ID** | `GroupMicroTextureRegionsFilter-D4` |
| **Filter UUID** | `3f695987-81b1-47c3-8cff-b49cfa219be0` |
| **Status** | active |

**Symptom:** The default comparison target and tolerance differ: DREAM.3D uses the touching Feature and 1 degree; SIMPLNX uses the running average and 20 degrees.

**Root cause:** Algorithmic choice. With `UseRunningAverage=false`, each candidate is compared with the current Feature in the neighbor traversal. With `true`, it is compared with the region's volume-weighted c-axis sum. Each accepted Feature changes that sum, so seed order can change the resulting groups.

**Affected users:** Users who reconstruct the private DREAM.3D workflow with default parameters. SIMPL pipeline conversion preserves an explicitly stored `UseRunningAverage` value and tolerance.

**Recommendation:** Trust SIMPLNX. Set the comparison mode and tolerance explicitly when migrating a workflow. Keep a fixed seed when the running average is enabled.

**Evidence:** The DREAM.3D v6.5.171 constructor sets `m_UseRunningAverage(false)` and `m_CAxisTolerance(1.0f)`. Current `parameters()` and the v7.4.2 source set `true` and `20.0f`. The conversion test checks the stored values. The 0/9/18-degree analytical fixture checks running-average grouping at 10 degrees.

---

### GroupMicroTextureRegionsFilter-D5

| Field | Value |
|---|---|
| **Deviation ID** | `GroupMicroTextureRegionsFilter-D5` |
| **Filter UUID** | `3f695987-81b1-47c3-8cff-b49cfa219be0` |
| **Status** | active |

**Symptom:** DREAM.3D creates an `Active` array. SIMPLNX omits this output and retains the parent Attribute Matrix.

**Root cause:** Algorithmic choice. The private DREAM.3D source creates the array with true values and uses its tuple count to size the parent output. It does not use those values to decide grouping. SIMPLNX stores the parent count directly. DREAM3D-NX v7.4.2 already omits the array.

**Affected users:** Users whose migrated workflows explicitly select `Active`. The Feature-parent and cell-parent arrays remain available.

**Recommendation:** Trust SIMPLNX. Remove references to `Active` and use the parent Attribute Matrix or parent-ID arrays for the required region information.

**Evidence:** The `Pure-Phi Bunge` test checks that `Active` is absent. `SIMPL Backwards Compatibility` checks both conversion fixtures after the obsolete mapping was removed. `vv_work:GroupMicroTextureRegionsFilter/rerun/logs/nx_removed_active_compat_preflight_after_format.log` records warning `-5433` for an obsolete `active_array_name` argument.

---

### GroupMicroTextureRegionsFilter-D7

| Field | Value |
|---|---|
| **Deviation ID** | `GroupMicroTextureRegionsFilter-D7` |
| **Filter UUID** | `3f695987-81b1-47c3-8cff-b49cfa219be0` |
| **Status** | active |

**Symptom:** A candidate at the angular tolerance boundary can be accepted by one implementation and rejected by the other.

**Root cause:** Precision. DREAM.3D v6.5.171 converts degrees using the double-precision `SIMPLib::Constants::k_Pi` constant and stores the result as float. SIMPLNX uses `k_PiF` in the conversion. The two calculations can round to different float32 thresholds.

**Affected users:** Users who compare results with candidate angles at the float32 tolerance boundary.

**Recommendation:** Either result is acceptable within float32 boundary precision. Use the same implementation and explicit tolerance for repeatable comparisons.

**Evidence:** The dated SIMPL source defines `k_Pi` as double. The local proof experiment at seed 5489 changed only the tolerance constant and removed two differing touching-pair decisions. Changing vector normalization did not remove them. See `vv_work:GroupMicroTextureRegionsFilter/rerun/pif_hypothesis_results.txt` and `vv_work:GroupMicroTextureRegionsFilter/rerun/floatnorm_hypothesis_results.txt`.

---

## Part 2 — SIMPLNX defects and behavior changes

### GroupMicroTextureRegionsFilter-D6

| Field | Value |
|---|---|
| **Deviation ID** | `GroupMicroTextureRegionsFilter-D6` |
| **Filter UUID** | `3f695987-81b1-47c3-8cff-b49cfa219be0` |
| **Status** | active |
| **Affected released versions** | No DREAM3D-NX release. DREAM.3D v6.5.171 was affected in its private filter source when running average was enabled. |
| **Fixed in** | DREAM3D-NX v7.4.2 |

**Symptom:** Before the fixed-seed default was introduced, repeated running-average executions could produce different region partitions.

**Root cause:** Bug. The default seed came from the system clock. The running-average target changes as Features join a region, so a different seed order can change membership. SIMPLNX now enables `UseSeed` by default and stores the seed in the output.

**Affected users:** Users of the pre-release implementation who enabled running average without a fixed seed. Released DREAM3D-NX defaults already use a fixed seed.

**Recommendation:** Trust SIMPLNX. Keep `UseSeed=true` and retain the stored seed when repeating an analysis.

**Evidence:** PR #1703 (`5a0b8327a`) changes the default to true. Direct inspection of all release trees finds no filter in v7.0.0 through v7.4.1; the v7.4.2 source has the correction. `vv_work:GroupMicroTextureRegionsFilter/rerun/preserved_seed_variation_results.txt` records 194,658 differing touching pairs between two runs, and 192,496 in a separate seed comparison.

---

### GroupMicroTextureRegionsFilter-D8

| Field | Value |
|---|---|
| **Deviation ID** | `GroupMicroTextureRegionsFilter-D8` |
| **Filter UUID** | `3f695987-81b1-47c3-8cff-b49cfa219be0` |
| **Status** | active |
| **Affected released versions** | DREAM3D-NX v7.4.2. DREAM.3D v6.5.171 was affected in its private filter source. |
| **Fixed in** | The DREAM3D-NX release after v7.4.2 |

**Symptom:** A participating Feature with a positive phase beyond Crystal Structures can cause an out-of-bounds read.

**Root cause:** Bug. The earlier code used positive Feature phases as Crystal Structures indices without an upper-bound check. SIMPLNX now checks both indices before either lookup and returns `-87001` on failure. An isolated Feature does not require that lookup.

**Affected users:** DREAM3D-NX v7.4.2 users with invalid positive phases on Features considered for grouping. The private DREAM.3D filter source has the same unchecked lookup.

**Recommendation:** Trust SIMPLNX. Correct the Feature phase or supply its Crystal Structures entry. The fixed filter returns an error for a participating invalid phase.

**Evidence:** `upstream/release/v7.4.2` reads `m_CrystalStructures.getValue(referenceFeaturePhase)` and the neighbor value without bounds checks. PR #1568 (`4ec21e93f`) adds both checks. `Phase Index Bounds` tests reference, neighbor, and isolated cases. The DREAM.3D `v6.5.171` source also indexes through unchecked phases.

---
