# Deviations from DREAM3D 6.5.171: ComputeFeaturePhasesFilter

This file lists every documented behavioral difference between this SIMPLNX filter and its DREAM3D 6.5.171 equivalent (`FindFeaturePhases`, SIMPL UUID `6334ce16-cea5-5643-83b5-9573805873fa`).

Entries use stable IDs (`ComputeFeaturePhasesFilter-D<N>`). D1, D2, and D3 are active. None affect `featurePhases[1..N]` for valid inputs.

> **Source references:** DREAM3D 6.5.171 `FindFeaturePhases::execute()` is in `DREAM3D/Source/Plugins/Generic/GenericFilters/FindFeaturePhases.cpp`. SIMPLNX algorithm is in `src/Plugins/SimplnxCore/src/SimplnxCore/Filters/Algorithms/ComputeFeaturePhases.cpp`.

---

## ComputeFeaturePhasesFilter-D1

| Field | Value |
|---|---|
| **Deviation ID** | `ComputeFeaturePhasesFilter-D1` |
| **Filter UUID** | `da5bb20e-4a8e-49d9-9434-fbab7bc434fc` (SIMPL `6334ce16-cea5-5643-83b5-9573805873fa`) |
| **Status** | active |
| **Tested by** | `SimplnxCore::ComputeFeaturePhasesFilter: Valid: Feature 0 Skip` |

**Symptom:** `featurePhases[0]` is always `0` in SIMPLNX; under 6.5.171 it is written with the phase of the last cell whose `featureId == 0`. Confirmed empirically in the 2026-07-10 comparison run (Case C: SIMPL writes `featurePhases[0] = 2`; SIMPLNX leaves it `0`).

**Root cause:** *algorithmic choice (explicit background skip).* The SIMPLNX loop contains `if(featureId == 0) { continue; }` before any write. The 6.5.171 `FindFeaturePhases::execute()` performs `m_FeaturePhases[gnum] = m_CellPhases[i]` unconditionally for every cell index `i` without any guard on `gnum == 0` (source: `FindFeaturePhases.cpp:202`), so background cells (featureId 0) are processed and `featurePhases[0]` receives the last background cell's phase.

**Affected users:** Any consumer that reads `featurePhases[0]` directly. Standard downstream filters treat feature index 0 as the background and ignore it.

**Recommendation:** Treat `featurePhases[0]` as undefined. Its value in SIMPLNX (0, from zero-initialization by `CreateArrayAction`) is not equivalent to the last background cell's phase produced by 6.5.171.

---

## ComputeFeaturePhasesFilter-D2

| Field | Value |
|---|---|
| **Deviation ID** | `ComputeFeaturePhasesFilter-D2` |
| **Filter UUID** | `da5bb20e-4a8e-49d9-9434-fbab7bc434fc` (SIMPL `6334ce16-cea5-5643-83b5-9573805873fa`) |
| **Status** | active |
| **Tested by** | `SimplnxCore::ComputeFeaturePhasesFilter: Invalid: Negative Cell Phase` |

**Symptom:** A cell phases array containing a negative value in a valid feature (not 0) causes an immediate error (`-61861`) in SIMPLNX. Under 6.5.171 the same input is processed silently.

**Root cause:** *algorithmic choice (defensive guard added).* SIMPLNX checks `if(currentPhaseId < 0) { return MakeErrorResult(-61861, ...); }` before each write. The 6.5.171 `FindFeaturePhases::execute()` has no corresponding check; `m_FeaturePhases[gnum] = m_CellPhases[i]` would silently store the negative value.

**Affected users:** Pipelines that supply negative cell phase values. Such values represent invalid input; the legacy silent-accept behavior was a defect. SIMPLNX surfaces the error explicitly.

**Recommendation:** Trust SIMPLNX. Negative phase values are invalid; the 6.5.171 silent-pass was undefined behavior.

---

## ComputeFeaturePhasesFilter-D3

| Field | Value |
|---|---|
| **Deviation ID** | `ComputeFeaturePhasesFilter-D3` |
| **Filter UUID** | `da5bb20e-4a8e-49d9-9434-fbab7bc434fc` (SIMPL `6334ce16-cea5-5643-83b5-9573805873fa`) |
| **Status** | active |
| **Tested by** | `SimplnxCore::ComputeFeaturePhasesFilter: Valid: Mixed Phase Warning`, `SimplnxCore::ComputeFeaturePhasesFilter: Valid: Truncated Mixed Feature Warning` |

**Symptom:** When features have cells with inconsistent phases, the two implementations emit different warning message formats under the same warning code (`-500`).

- **SIMPLNX** emits a single line listing up to 15 affected feature IDs as a comma-separated list, followed by `"and N more occurrence(s)"` when more than 15 features are affected. Example: `"... Affected Phase Features: 2, 5, 7, and 3 more occurrences"`.
- **6.5.171** emits a multi-line message: a header line (`"Elements from some features did not all have the same phase Id. The last phase Id copied into each feature will be used"`) followed by one line per conflicting feature: `"  Phase Feature X created Y warnings."` where `Y` is the number of phase-mismatched cells for that feature. All lines are joined with `\n` in a single `notifyWarningMessage` call. There is no truncation limit.

**Root cause:** *message format rewritten at port time.* The legacy implementation tracked a per-feature conflict count in a `QMap<int32_t, int32_t> warningMap` and emitted that count per feature. SIMPLNX replaces the count with a `std::vector<bool> warnFeature` (boolean per feature) and formats the output as a flat ID list rather than per-feature lines. SIMPLNX also caps the list at 15 IDs to keep the warning compact.

**Affected users:** Scripts or tooling that parse warning message text. The warning code (`-500`) is unchanged; only the message text and structure differ.

**Recommendation:** Trust SIMPLNX. The warning code is stable; do not parse the message body for programmatic use.

---

## Comparison runs

### 2026-07-10 — current (post-cleanup, 3 cases)

| Case | featureIds | cellPhases | Result |
|---|---|---|---|
| A | `[1,1,2,2,3,3,4,4]` | `[1,1,1,1,2,2,2,2]` | `featurePhases[1..4]` **BIT-IDENTICAL** |
| B | `[1,2,3,4,1,2,3,4]` | `[1,1,2,2,2,1,2,2]` | `featurePhases[1..4]` **BIT-IDENTICAL** |
| C | `[0,0,1,1,2,2,3,3]` | `[2,2,1,1,2,2,1,1]` | `featurePhases[1..3]` bit-identical; `featurePhases[0]`: SIMPL=2, SIMPLNX=0 — **D1 confirmed** |

**Conclusion:** `featurePhases[1..N]` is bit-identical across all three cases. The only output deviation is D1 at `featurePhases[0]` when background cells are present. D2 and D3 affect error/warning paths only.
