# Deviations from DREAM.3D v6.5.171: ErodeDilateBadDataFilter

Part 1 lists current validation differences from DREAM.3D v6.5.171. Part 2 lists the fixed SIMPLNX direction-control defect. Entries share the stable `ErodeDilateBadDataFilter-D<N>` sequence.

---

## Part 1 — Differences from DREAM.3D v6.5.171

### ErodeDilateBadDataFilter-D2

| Field | Value |
|---|---|
| **Deviation ID** | `ErodeDilateBadDataFilter-D2` |
| **Filter UUID** | `7f2f7378-580e-4337-8c04-a29e7883db0b` |
| **Status** | active |
| **Affected released versions** | DREAM3D-NX v7.0.0 through v7.4.2. DREAM.3D v6.5.171 was not affected. |
| **Fixed in** | Not fixed |

**Symptom:** SIMPLNX accepts zero or negative iteration counts and leaves cell data unchanged. DREAM.3D v6.5.171 rejects these counts with error `-5555`.

**Root cause:** Bug. `ErodeDilateBadDataFilter::preflightImpl()` has no positive-iteration check. `Int32Parameter` accepts these values, and the algorithm's iteration loop does not run when `NumIterations <= 0`.

**Affected users:** Users whose scripts or saved pipelines supply an iteration count below one.

**Recommendation:** Supply at least one iteration. Do not use successful preflight as proof that morphology will run with a non-positive count.

**Evidence:** Direct inspection of `ErodeDilateBadDataFilter.cpp` and `Algorithms/ErodeDilateBadData.cpp` in every release from `upstream/release/v7.0.0` through `upstream/release/v7.4.2` shows no positive-iteration guard. Current `src/simplnx/Parameters/NumberParameter.cpp::validateNumber()` returns success without a range check. DREAM.3D tag `v6.5.171`, `Source/Plugins/Processing/ProcessingFilters/ErodeDilateBadData.cpp::dataCheck()`, rejects non-positive counts. No current filter test supplies such a count.

---

### ErodeDilateBadDataFilter-D3

| Field | Value |
|---|---|
| **Deviation ID** | `ErodeDilateBadDataFilter-D3` |
| **Filter UUID** | `7f2f7378-580e-4337-8c04-a29e7883db0b` |
| **Status** | active |

**Symptom:** SIMPLNX rejects a pipeline with all three direction flags disabled. DREAM.3D v6.5.171 accepts it and leaves the cell arrays unchanged.

**Root cause:** Algorithmic choice. SIMPLNX requires at least one enabled axis and returns `-14601` before execution. DREAM.3D v6.5.171 has no equivalent preflight guard; its six direction checks exclude every neighbor.

**Affected users:** Users who disable all axes to bypass morphology. DREAM3D-NX v7.4.2 and later require an enabled axis.

**Recommendation:** Trust SIMPLNX validation. Disable the filter in the pipeline when morphology is not required.

**Evidence:** The `No Direction` test checks `-14601` for both operations and one or two iterations. The guard is present in `upstream/release/v7.4.2:src/Plugins/SimplnxCore/src/SimplnxCore/Filters/ErodeDilateBadDataFilter.cpp`. The DREAM.3D `v6.5.171` source has direction checks in the neighbor loop and no all-directions-off rejection.

---

## Part 2 — SIMPLNX defects and behavior changes

### ErodeDilateBadDataFilter-D1

| Field | Value |
|---|---|
| **Deviation ID** | `ErodeDilateBadDataFilter-D1` |
| **Filter UUID** | `7f2f7378-580e-4337-8c04-a29e7883db0b` |
| **Status** | active |
| **Affected released versions** | DREAM3D-NX v7.4.1. DREAM.3D v6.5.171 was not affected. |
| **Fixed in** | DREAM3D-NX v7.4.2 |

**Symptom:** Disabling X, Y or Z still allowed erosion or dilation along that axis.

**Root cause:** Bug. The neighbor-utility refactor omitted direction gating from the scan. The filter passed the flags to the algorithm, but the algorithm did not use them. Current `Algorithms/ErodeDilateBadData.cpp` calls `adjustValidNeighbors()` after each boundary check; the helper applies each flag to the two matching faces.

**Affected users:** Users of DREAM3D-NX v7.4.1 who disable an axis that has valid neighbors. Runs with all axes enabled do not expose this defect.

**Recommendation:** Trust SIMPLNX in DREAM3D-NX v7.4.2 or later. Regenerate results produced with a restricted direction set in v7.4.1.

**Evidence:** The algorithm in `upstream/release/v7.0.0` through `upstream/release/v7.4.0` gates all six faces. The v7.4.1 source has no direction checks. The v7.4.2 source calls a helper that gates the boolean validity array with the correct axis mapping. DREAM.3D tag `v6.5.171`, `Source/Plugins/Processing/ProcessingFilters/ErodeDilateBadData.cpp`, gates all six faces. PR #1687 (`ea4f1ae20`) supplies the correction. The two `Expanded` tests compare FeatureIds and Misc for all seven direction combinations and one or two iterations. The saved intermediate-fix review is `vv_work:ErodeDilateBadDataFilter/pr_comment.md`; it is separate from current CI results.
