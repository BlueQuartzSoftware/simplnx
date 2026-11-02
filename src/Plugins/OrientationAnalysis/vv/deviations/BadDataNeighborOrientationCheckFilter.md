# Deviations from DREAM.3D v6.5.171: BadDataNeighborOrientationCheckFilter

This file has two parts. Part 1 lists the documented differences from DREAM.3D v6.5.171 (D1 and D2). Part 2 lists the precision and input-safety defects fixed in SIMPLNX (D3 through D5). These defects also occur in DREAM.3D v6.5.171.

Entries use stable IDs (`BadDataNeighborOrientationCheckFilter-D<N>`) from one sequence. The Filter UUID is the permanent cross-reference anchor. Release-source checks and comparison evidence are recorded in `../provenance/BadDataNeighborOrientationCheckFilter.md`.

---

## Part 1 — Differences from DREAM.3D v6.5.171

### BadDataNeighborOrientationCheckFilter-D1

| Field | Value |
|---|---|
| **Deviation ID** | `BadDataNeighborOrientationCheckFilter-D1` |
| **Filter UUID** | `3f342977-aea1-49e1-a9c2-f73760eba0d3` |
| **Status** | active |

**Symptom:** A bad voxel can stay bad when its matching good-neighbor count equals the requested minimum.

**Root cause:** Bug. DREAM.3D v6.5.171 uses `while(currentLevel > m_NumberOfNeighbors)`. This condition omits the minimum level, including all propagation when the requested count is six. Both current SIMPLNX implementations use the inclusive `>=` bound.

**Affected users:** Users of DREAM.3D v6.5.171 or DREAM3D-NX v7.0.0 through v7.4.0 whose bad voxels need propagation at the minimum neighbor level. A changed mask can change downstream results.

**Recommendation:** Trust SIMPLNX. DREAM3D-NX v7.4.1 contains the fix. The inclusive bound agrees with the parameter definition of a minimum number of neighbors.

**Evidence:** PR #1499 (`ebdce592f`) changes `>` to `>=`. `Case 1.1.1` through `Case 1.6.1` check the minimum-level result. `BadDataNeighborOrientationCheckWorklist.cpp` and `BadDataNeighborOrientationCheckScanline.cpp` retain the inclusive bound. Release-source checks are recorded in `../provenance/BadDataNeighborOrientationCheckFilter.md`.

---

### BadDataNeighborOrientationCheckFilter-D2

| Field | Value |
|---|---|
| **Deviation ID** | `BadDataNeighborOrientationCheckFilter-D2` |
| **Filter UUID** | `3f342977-aea1-49e1-a9c2-f73760eba0d3` |
| **Status** | active |

**Symptom:** A different-phase neighbor can inherit a small angle from an earlier neighbor and incorrectly increase the matching-neighbor count.

**Root cause:** Bug. DREAM.3D v6.5.171 tests the angle outside the same-phase condition in both counting and propagation. A different-phase neighbor can use the angle from an earlier neighbor. Worklist now tests the angle inside the same-phase condition. Scanline uses `isMisorientationMatch()` to reject a phase mismatch before calculating the angle. The rewrite changes the execution method but preserves this corrected rule and the D1 inclusive bound.

**Affected users:** Users of DREAM.3D v6.5.171 or DREAM3D-NX v7.0.0 through v7.4.0 with mixed-phase neighbors. A stale angle can overcount matches at a phase boundary.

**Recommendation:** Trust SIMPLNX. DREAM3D-NX v7.4.1 contains the fix. Only a good neighbor of the same positive phase can count as a match.

**Evidence:** PR #1499 (`ebdce592f`) moves both angle tests inside the same-phase condition. `Case 1.2.2` checks a mixed-phase expected mask. A local proof version isolates 37 extra good voxels caused by stale angles; moving both tests removes these differences. See `vv_work:BadDataNeighborOrientationCheckFilter/algorithm_review/source_comparison.md`, `vv_work:BadDataNeighborOrientationCheckFilter/comparison/legacy_emulator_results.md`, and the release-source checks in `../provenance/BadDataNeighborOrientationCheckFilter.md`.

---

## Part 2 — SIMPLNX defects and behavior changes

### BadDataNeighborOrientationCheckFilter-D3

| Field | Value |
|---|---|
| **Deviation ID** | `BadDataNeighborOrientationCheckFilter-D3` |
| **Filter UUID** | `3f342977-aea1-49e1-a9c2-f73760eba0d3` |
| **Status** | active |
| **Affected released versions** | DREAM3D-NX v7.0.0 through v7.4.1. DREAM.3D v6.5.171 was affected. |
| **Fixed in** | DREAM3D-NX v7.4.2 |

**Symptom:** A rounded-up angle tolerance can admit a neighbor that should fail the strict boundary comparison.

**Root cause:** Bug. The earlier algorithm calculated radians with `float` and `numbers::pi_v<float>`. DREAM.3D v6.5.171 also uses float arithmetic for this conversion. Worklist and Scanline now use double precision. The stored float32 quaternions give an angle slightly above 5°; both tolerance conversion and angle rounding affect the decision.

**Affected users:** Users with misorientations close to the strict tolerance boundary. The output effect depends on the orientation calculation as well as the tolerance conversion.

**Recommendation:** Trust SIMPLNX. Double precision reduces the conversion error without changing the strict `<` comparison.

**Evidence:** PR #1631 (`ec22ac375`) replaces the float tolerance conversion with double precision. The expected masks for `Case 1.3.3`, `Case 1.4.3`, `Case 1.5.3` and `Case 1.6.3` keep the center bad. A local proof comparison isolates four center-voxel differences and removes them with the corrected conversion. See `vv_work:BadDataNeighborOrientationCheckFilter/algorithm_review/source_comparison.md` and the release-source checks in `../provenance/BadDataNeighborOrientationCheckFilter.md`.

---

### BadDataNeighborOrientationCheckFilter-D4

| Field | Value |
|---|---|
| **Deviation ID** | `BadDataNeighborOrientationCheckFilter-D4` |
| **Filter UUID** | `3f342977-aea1-49e1-a9c2-f73760eba0d3` |
| **Status** | active |
| **Affected released versions** | DREAM3D-NX v7.0.0 through v7.4.1. DREAM.3D v6.5.171 was affected. |
| **Fixed in** | DREAM3D-NX v7.4.2 |

**Symptom:** A positive phase with an invalid crystal-structure value can cause an out-of-bounds orientation-operator access.

**Root cause:** Bug. Released SIMPLNX source uses `crystalStructures[cellPhases[i]]` as an operator index without validating the crystal-structure value. DREAM.3D v6.5.171 performs the same unchecked operator lookup, and its `dataCheck()` validates array types and tuple counts without checking crystal-structure values. Both current implementations return `-54901` for an invalid non-sentinel value and skip a positive phase that resolves to `UnknownCrystalStructure`.

**Affected users:** Users with invalid values in Crystal Structures, or a participating positive phase mapped to the unknown-structure sentinel.

**Recommendation:** Trust SIMPLNX. Supply valid Laue indices; the unknown-structure sentinel cannot take part in an orientation match.

**Evidence:** PR #1631 (`ec22ac375`) adds a Laue-index check and a per-voxel guard. No current fixture directly tests these rejection and skip paths. Release-source and legacy-source checks are recorded in `../provenance/BadDataNeighborOrientationCheckFilter.md`.

---

### BadDataNeighborOrientationCheckFilter-D5

| Field | Value |
|---|---|
| **Deviation ID** | `BadDataNeighborOrientationCheckFilter-D5` |
| **Filter UUID** | `3f342977-aea1-49e1-a9c2-f73760eba0d3` |
| **Status** | active |
| **Affected released versions** | DREAM3D-NX v7.0.0 through v7.4.2. DREAM.3D v6.5.171 was affected. |
| **Fixed in** | The DREAM3D-NX release after v7.4.2 |

**Symptom:** A bad voxel with an invalid phase can cause an out-of-bounds read from Crystal Structures.

**Root cause:** Bug. DREAM3D-NX v7.4.2 reads `crystalStructures[cellPhases[voxelIndex]]` before checking the phase range. Earlier DREAM3D-NX releases and DREAM.3D v6.5.171 also read through an unchecked phase index. Worklist and Scanline now check the phase before the lookup. Nonpositive phases do not participate; a positive phase beyond the array returns `-54902`.

**Affected users:** Users with phase values that do not identify an entry in Crystal Structures on a bad voxel.

**Recommendation:** Trust SIMPLNX. Correct the phase or supply the corresponding ensemble entry before processing that voxel.

**Evidence:** PR #1568 (`4ec21e93f`) adds phase bounds checks. `Phase Index Bounds` checks the bad-voxel error and permits an invalid phase on an already good voxel. Release-source and legacy-source checks are recorded in `../provenance/BadDataNeighborOrientationCheckFilter.md`.
