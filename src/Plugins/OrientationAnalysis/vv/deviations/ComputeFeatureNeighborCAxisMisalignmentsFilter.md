# Deviations from DREAM.3D v6.5.171: ComputeFeatureNeighborCAxisMisalignmentsFilter

Part 1 lists differences from DREAM.3D v6.5.171 (D1, D4, and D6). Part 2 lists SIMPLNX initialization and diagnostic changes (D2 and D5), and the fixed phase-index defect (D7).

Entries use one stable ID sequence. D3 was not assigned to an entry and remains unused. The Filter UUID is the permanent cross-reference anchor. Source and release evidence are recorded in `../provenance/ComputeFeatureNeighborCAxisMisalignmentsFilter.md`.

---

## Part 1 — Differences from DREAM.3D v6.5.171

### ComputeFeatureNeighborCAxisMisalignmentsFilter-D1

| Field | Value |
|---|---|
| **Deviation ID** | `ComputeFeatureNeighborCAxisMisalignmentsFilter-D1` |
| **Filter UUID** | `636ee030-9f07-4f16-a4f3-592eff8ef1ee` |
| **Status** | active |

**Symptom:** An excluded neighbor before an accepted neighbor reduces the calculated average below the mean of the accepted angles. A nonempty list with no accepted neighbors can produce zero instead of NaN.

**Root cause:** Bug. DREAM.3D v6.5.171 resets `hexneighborlistsize` to the full list size inside each neighbor iteration. DREAM3D-NX v7.0.0 through v7.4.1 do the same. Each reset discards earlier decrements for unequal crystal structures or non-hexagonal structures. The final divisor is N if the last neighbor is accepted, or N minus one if it is excluded. DREAM3D-NX v7.4.2 initializes the divisor before the neighbor loop. The divisor then counts every accepted pair.

**Affected users:** Users who enable averaging on neighbor lists with excluded pairs. The old value depends on neighbor order. Hexagonal-only lists of matching crystal structures do not expose the defect.

**Recommendation:** Trust SIMPLNX in DREAM3D-NX v7.4.2 or later. The average uses all accepted neighbors, regardless of their order.

**Evidence:** PR #1631 (`ec22ac375`) corrects the divisor. `Class 1 - Realistic Microstructure (exposes divisor bug)` checks the independent expected averages: F2 = 10°, F3 = NaN, F5 = 25/3°, and F6 = 5°. The defective divisor gives 7.5°, 0°, 6.25°, and 2.5°, respectively, by calculation; these are not legacy-run measurements. `Class 4 - Invariants` checks the mean of the non-NaN entries. `Class 1 - Mismatch Last Order` checks an ordering where the old divisor gives the correct result.

---

### ComputeFeatureNeighborCAxisMisalignmentsFilter-D4

| Field | Value |
|---|---|
| **Deviation ID** | `ComputeFeatureNeighborCAxisMisalignmentsFilter-D4` |
| **Filter UUID** | `636ee030-9f07-4f16-a4f3-592eff8ef1ee` |
| **Status** | active |

**Symptom:** Finite pair angles and averages can differ in their last digits from DREAM.3D v6.5.171.

**Root cause:** Precision and library. DREAM.3D v6.5.171 uses `QuatF`, float matrices, `MatrixMath`, and `acosf`. SIMPLNX promotes the stored float32 quaternions into `QuaternionDType`, calculates orientation matrices and normalized c-axis vectors with double precision, and calls `std::acos`. Both algorithms fold the angle into [0°, 90°] and store float32 results. Neither angle calculation uses the full crystal misorientation search.

**Affected users:** Users who compare floating-point angle arrays between DREAM.3D v6.5.171 and SIMPLNX.

**Recommendation:** Either acceptable within the 0.001° tolerance used by the analytical fixtures for finite pair angles. Compare averages only after accounting for D1, and compare NaN masks separately. This fixture tolerance is not a bound for all orientations.

**Evidence:** PR #1472 (`413e6fa46`) changes the SIMPLNX quaternion-to-matrix API and transpose expression. The analytical fixtures use a 0.001° absolute tolerance for finite results.

---

### ComputeFeatureNeighborCAxisMisalignmentsFilter-D6

| Field | Value |
|---|---|
| **Deviation ID** | `ComputeFeatureNeighborCAxisMisalignmentsFilter-D6` |
| **Filter UUID** | `636ee030-9f07-4f16-a4f3-592eff8ef1ee` |
| **Status** | active |

**Symptom:** SIMPLNX computes c-axis angles for pairs with Hexagonal_Low crystal structures. DREAM.3D v6.5.171 writes NaN for those pairs.

**Root cause:** Algorithmic choice. The DREAM.3D v6.5.171 pair gate accepts only equal `Hexagonal_High` values. SIMPLNX accepts equal `Hexagonal_High` or equal `Hexagonal_Low` values. Both compare crystal-structure values, not phase IDs. Different phase IDs with the same accepted crystal structure therefore pass the gate. A Hexagonal_High/Hexagonal_Low pair fails because the two structure values differ. The c-axis calculation uses the orientation matrix and the crystal [001] vector. It does not require a symmetry-operator search.

**Affected users:** Users with Hexagonal_Low neighbor pairs who migrate from DREAM.3D v6.5.171.

**Recommendation:** Trust SIMPLNX. The same geometric c-axis angle applies to both hexagonal classes.

**Evidence:** The source for each DREAM3D-NX release from v7.0.0 through v7.4.2 includes Hexagonal_Low in the pair gate. No current analytical fixture directly selects Hexagonal_Low.

---

## Part 2 — SIMPLNX defects and behavior changes

### ComputeFeatureNeighborCAxisMisalignmentsFilter-D2

| Field | Value |
|---|---|
| **Deviation ID** | `ComputeFeatureNeighborCAxisMisalignmentsFilter-D2` |
| **Filter UUID** | `636ee030-9f07-4f16-a4f3-592eff8ef1ee` |
| **Status** | active |
| **Affected released versions** | DREAM3D-NX v7.0.0 through v7.4.2 use the output array as the accumulator. DREAM.3D v6.5.171 was not affected by the initialization dependency; it supplies an explicit zero fill value. |
| **Fixed in** | The DREAM3D-NX release after v7.4.2 |

**Symptom:** Current SIMPLNX initializes average accumulators to zero without reading the output array's initial values.

**Root cause:** Behavior change. DREAM3D-NX v7.0.0 through v7.4.2 add angles directly to the output array created without an explicit fill value. Current SIMPLNX initializes a buffer with `avgCAxisBuf.resize(totalFeatures, 0.0f)`, accumulates in that buffer, and writes it once at the end. This removes the dependency on the output store's initial contents. This entry records the initialization change.

**Affected users:** Users who enable the average output. The accumulation now starts at an explicit zero for every feature.

**Recommendation:** Trust SIMPLNX. Explicit zero initialization makes the average independent of the output store's initial contents.

**Evidence:** PR #1568 (`4ec21e93f`) adds the zero-initialized buffer. The Class 1 averages and Class 4 mean invariant define the expected numerical result.

---

### ComputeFeatureNeighborCAxisMisalignmentsFilter-D5

| Field | Value |
|---|---|
| **Deviation ID** | `ComputeFeatureNeighborCAxisMisalignmentsFilter-D5` |
| **Filter UUID** | `636ee030-9f07-4f16-a4f3-592eff8ef1ee` |
| **Status** | active |
| **Affected released versions** | DREAM3D-NX v7.4.0 through v7.4.2 contain the preflight information-channel change. DREAM.3D v6.5.171 was not affected; it has no corresponding SIMPLNX preflight channel. |
| **Fixed in** | DREAM3D-NX v7.4.0 |

**Symptom:** The general hexagonal-symmetry notice is a preflight updated value instead of a preflight warning. Mixed-structure execution still returns warning `-1563`.

**Root cause:** Behavior change. DREAM3D-NX v7.4.0 moves the general notice from `resultOutputActions.warnings().push_back(...)` to `preflightUpdatedValues.push_back(...)` in the filter. The algorithm's mixed-structure warning remains. Current execution returns `-1562` if no hexagonal ensemble exists and warning `-1563` if hexagonal and non-hexagonal ensembles coexist.

**Affected users:** Clients that read preflight warnings but do not read preflight updated values. The filter's result channel still contains the execution-time warning.

**Recommendation:** Read preflight updated values for the general notice and execution warnings for the data-dependent result. No warning-restoration fix is required by this entry.

**Evidence:** PR #1438 (`e6896714b`) introduces the channel change. The release sources return the general notice as a preflight warning through v7.2.2 and as an updated value in v7.4.0 through v7.4.2.

---

### ComputeFeatureNeighborCAxisMisalignmentsFilter-D7

| Field | Value |
|---|---|
| **Deviation ID** | `ComputeFeatureNeighborCAxisMisalignmentsFilter-D7` |
| **Filter UUID** | `636ee030-9f07-4f16-a4f3-592eff8ef1ee` |
| **Status** | active |
| **Affected released versions** | DREAM3D-NX v7.0.0 through v7.4.2. DREAM.3D v6.5.171 was affected. |
| **Fixed in** | The DREAM3D-NX release after v7.4.2 |

**Symptom:** A negative or excessive focal or neighbor phase index can cause an out-of-bounds read from Crystal Structures.

**Root cause:** Bug. DREAM3D-NX v7.0.0 through v7.4.2 use `crystalStructures[featurePhases[...]]` without checking the phase range. DREAM.3D v6.5.171 uses the same unchecked lookup. Current SIMPLNX checks both focal and neighbor phase indices before reading the cached crystal structures. The check returns `-1564` with the array paths, index, invalid value, and valid range.

**Affected users:** Users whose processed features have phase values outside the Crystal Structures tuple range. An unused background phase is not read because the feature loop starts at one.

**Recommendation:** Trust SIMPLNX. Correct the invalid phase or provide its ensemble tuple.

**Evidence:** PR #1568 (`4ec21e93f`) adds the bounds checks. `Phase Index Bounds` checks -1 and 2 against a two-tuple ensemble array for both lookup locations and permits an invalid unused background phase.
