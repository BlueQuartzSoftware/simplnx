# Deviations from DREAM.3D v6.5.171: ComputeCAxisLocationsFilter

Part 1 lists the phase-domain differences from DREAM.3D v6.5.171. Part 2 lists the fixed SIMPLNX phase-index defect. Both parts use the stable `ComputeCAxisLocationsFilter-D<N>` sequence. The Filter UUID is the permanent cross-reference anchor.

---

## Part 1 — Differences from DREAM.3D v6.5.171

### ComputeCAxisLocationsFilter-D1

| Field | Value |
|---|---|
| **Deviation ID** | `ComputeCAxisLocationsFilter-D1` |
| **Filter UUID** | `a51c257a-ddc1-499a-9b21-f2d25a19d098` |
| **Status** | active |

**Symptom:** SIMPLNX writes three NaNs for each non-hexagonal cell; DREAM.3D v6.5.171 calculates a vector without checking the crystal structure.

**Root cause:** Algorithmic choice. SIMPLNX checks each cell's phase and calculates a C-axis only for Hexagonal-High or Hexagonal-Low. The DREAM.3D v6.5.171 filter rotates `[0, 0, 1]` for every quaternion and has no phase lookup.

**Affected users:** Users with non-hexagonal cells and at least one hexagonal entry in the non-background ensemble table.

**Recommendation:** Trust SIMPLNX. NaN identifies cells outside the filter's hexagonal domain.

**Evidence:** `Class 1 Oracle - Mixed hexagonal and non-hexagonal phases` in `src/Plugins/OrientationAnalysis/test/ComputeCAxisLocationsTest.cpp` checks analytical vectors and NaNs. The 2026-07-31 comparison in `vv_work:ComputeCAxisLocationsFilter/original_comparison/Legacy_Comparison.md` records 15 matching all-hexagonal vectors, 45 bit-identical float32 components, identical shared inputs and agreement with the analytical oracle at absolute tolerance `1e-7`. Its CSV and pipelines are in the same folder. The 2026-09-17 comparison of DREAM.3D v6.5.171 and DREAM3D-NX v7.4.2 in `vv_work:ComputeCAxisLocationsFilter/results/nonhex/comparison.md` and `comparison.json` records 8 bit-identical hexagonal vectors and 7 cubic cells with finite legacy vectors and SIMPLNX NaNs. The runner selection is in `vv_work:ComputeCAxisLocationsFilter/scripts/run_nonhex_matrix.py`. Both comparisons precede PR #1568 (`4ec21e93f`, 2026-09-20), which adds buffered chunks and phase-index validation; they describe the binaries used in those runs.

---

### ComputeCAxisLocationsFilter-D2

| Field | Value |
|---|---|
| **Deviation ID** | `ComputeCAxisLocationsFilter-D2` |
| **Filter UUID** | `a51c257a-ddc1-499a-9b21-f2d25a19d098` |
| **Status** | active |

**Symptom:** SIMPLNX returns error `-3522` when the ensemble table has no hexagonal entry after index zero; DREAM.3D v6.5.171 executes.

**Root cause:** Algorithmic choice. SIMPLNX scans the ensemble table before processing cells. This check uses the ensemble entries, even if no cell refers to them. DREAM.3D v6.5.171 accepts only quaternions and does not make this check.

**Affected users:** Users whose non-background ensemble entries are all non-hexagonal, or whose ensemble table has no non-background entries.

**Recommendation:** Trust SIMPLNX. Supply the correct hexagonal ensemble data before computing C-axis locations.

**Evidence:** `No hexagonal phases error` in `src/Plugins/OrientationAnalysis/test/ComputeCAxisLocationsTest.cpp` checks error `-3522` for a cubic ensemble. The 2026-09-17 `all_cubic` comparison in `vv_work:ComputeCAxisLocationsFilter/results/nonhex/comparison.md` records successful DREAM.3D v6.5.171 execution and DREAM3D-NX v7.4.2 rejection with `-3522`. This run precedes the current algorithm's buffered chunks and phase-index validation.

---

### ComputeCAxisLocationsFilter-D3

| Field | Value |
|---|---|
| **Deviation ID** | `ComputeCAxisLocationsFilter-D3` |
| **Filter UUID** | `a51c257a-ddc1-499a-9b21-f2d25a19d098` |
| **Status** | active |

**Symptom:** SIMPLNX emits warning `-3521` after successful preflight and warning `-3523` when the non-background ensemble table contains both hexagonal and non-hexagonal entries; DREAM.3D v6.5.171 emits neither warning.

**Root cause:** Algorithmic choice. Preflight reminds the user of the hexagonal requirement. Execution warns about NaN output for non-hexagonal phases after checking that at least one hexagonal ensemble exists. The warnings depend on the ensemble table, not on which phases cells use.

**Affected users:** All users who pass preflight see `-3521`. Users with mixed non-background ensemble entries also see `-3523` during execution.

**Recommendation:** Trust SIMPLNX. Check the selected phase and crystal-structure arrays when these warnings appear.

**Evidence:** `Not all hexagonal phases warning` in `src/Plugins/OrientationAnalysis/test/ComputeCAxisLocationsTest.cpp` checks both codes and NaN output. The 2026-09-17 DREAM3D-NX v7.4.2 runs in `vv_work:ComputeCAxisLocationsFilter/results/nonhex/comparison.md` record warnings `-3521` and `-3523` for `mixed_hex_cubic`, and warning `-3521` before error `-3522` for `all_cubic`. These runs precede the current algorithm's buffered chunks and phase-index validation.

---

## Part 2 — SIMPLNX defects and behavior changes

### ComputeCAxisLocationsFilter-D4

| Field | Value |
|---|---|
| **Deviation ID** | `ComputeCAxisLocationsFilter-D4` |
| **Filter UUID** | `a51c257a-ddc1-499a-9b21-f2d25a19d098` |
| **Status** | active |
| **Affected released versions** | DREAM3D-NX v7.0.0 through v7.4.2. DREAM.3D v6.5.171 was not affected. |
| **Fixed in** | The DREAM3D-NX release after v7.4.2 |

**Symptom:** A negative cell phase or a phase at least as large as the ensemble tuple count could cause an out-of-bounds read from Crystal Structures.

**Root cause:** Bug. Released SIMPLNX code reads `crystalStructures[cellPhases[i]]` without checking the phase value. Its preflight checks array types, component shapes and cell tuple counts, but not the phase range. The current algorithm checks both bounds before indexing the ensemble cache and returns `-3524` with the array paths, phase value and voxel index. DREAM.3D v6.5.171 was not affected because `FindCAxisLocations` has no phase input or ensemble lookup.

**Affected users:** Users with invalid cell phase indices when at least one non-background ensemble entry is hexagonal.

**Recommendation:** Trust SIMPLNX. Correct each invalid phase index or supply its missing ensemble entry before execution.

**Evidence:** Direct inspection of `src/Plugins/OrientationAnalysis/src/OrientationAnalysis/Filters/Algorithms/ComputeCAxisLocations.cpp` and `src/Plugins/OrientationAnalysis/src/OrientationAnalysis/Filters/ComputeCAxisLocationsFilter.cpp` on all ten `upstream/release/v<version>` branches from v7.0.0 through v7.4.2 shows the unchecked lookup and no preflight range check. The PR #1568 diff adds the guard. `src/Plugins/OrientationAnalysis/test/ComputeCAxisLocationsTest.cpp::OrientationAnalysis::ComputeCAxisLocationsFilter: Phase Index Bounds` checks error `-3524` for phases -1 and 2 with two ensemble tuples.
