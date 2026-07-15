# Deviations from DREAM3D 6.5.171: ComputeFeaturePhasesFilter

Behavioral differences between `ComputeFeaturePhasesFilter` and `FindFeaturePhases` (SIMPL UUID `6334ce16-cea5-5643-83b5-9573805873fa`). All four deviations were introduced in post-Phase-9 algorithm cleanup (branch `vv/fill_bad_data`, 2026-07-10). None affect `featurePhases[1..N]` for valid inputs.

---

## D1 — Background feature (index 0) excluded from phase assignment

| Field | Value |
|---|---|
| **Affects output** | Yes — `featurePhases[0]` only |
| **Severity** | Low |
| **Tested by** | Case 2 (`Background Feature Skip`) |

**SIMPLNX:** Cells with `featureId == 0` are skipped. `featurePhases[0]` retains zero-initialization from `CreateArrayAction`.

**SIMPL:** Background cells are processed; `featurePhases[0]` is written with the last background cell's phase.

**Downstream note:** Any consumer reading `featurePhases[0]` will observe `0` from SIMPLNX instead of the last background cell's phase. `featurePhases[0]` should be treated as undefined.

---

## D2 — Hard error on negative cell phase values

| Field | Value |
|---|---|
| **Affects output** | No — error path only |
| **Severity** | Behavioral change |
| **Tested by** | Case 7 (`Negative Cell Phase`, error -61861) |

**SIMPLNX:** Negative cell phase → immediate error -61861.

**SIMPL:** No check; negative phase values processed silently.

---

## D3 — Hard error on cell array size mismatch

| Field | Value |
|---|---|
| **Affects output** | No — error path only |
| **Severity** | Behavioral change |
| **Tested by** | Case 6 (`Cell Array Size Mismatch`, error -61860) |

**SIMPLNX:** `featureIds.getNumberOfTuples() != cellPhases.getNumberOfTuples()` → immediate error -61860.

**SIMPL:** No check; mismatch would result in an out-of-bounds read.

**Note:** Unreachable in normal use — `AttributeMatrixSelectionParameter` and `ArraySelectionParameter` enforce same-AM selection. Guard is defensive.

---

## D4 — Warning message phrasing and truncation

| Field | Value |
|---|---|
| **Affects output** | No — warning text only |
| **Severity** | None |
| **Tested by** | Cases 3 (`Inconsistent Phase Warning`) and 4 (`Warning Truncation`) |

**SIMPLNX:** Single consolidated warning (code -500) listing up to 15 affected feature IDs by index, followed by `"and N more occurrence(s)"` for large conflict sets. Introduced at port time (PR #1455).

**SIMPL:** Single generic message with no feature ID list.

---

## Comparison runs

### 2026-07-10 — current (post-cleanup, 3 cases)

| Case | featureIds | cellPhases | Result |
|---|---|---|---|
| A | `[1,1,2,2,3,3,4,4]` | `[1,1,1,1,2,2,2,2]` | `featurePhases[1..4]` **BIT-IDENTICAL** |
| B | `[1,2,3,4,1,2,3,4]` | `[1,1,2,2,2,1,2,2]` | `featurePhases[1..4]` **BIT-IDENTICAL** |
| C | `[0,0,1,1,2,2,3,3]` | `[2,2,1,1,2,2,1,1]` | `featurePhases[1..3]` bit-identical; `featurePhases[0]`: SIMPL=2, SIMPLNX=0 — **D1 confirmed** |

Scripts: `compute_feature_phases_vv/{generate_inputs.py,pipeline_simpl_{A,B,C}.json,pipeline_nx_{A,B,C}.d3dpipeline,compare_outputs.py}`

**Conclusion:** `featurePhases[1..N]` is bit-identical across all three cases. The only deviation is D1 at `featurePhases[0]` when background cells are present.

### 2026-07-09 — archived (pre-cleanup, 2 cases)

Cases A and B only; no background cells; all elements bit-identical. `featurePhases[0]` not explicitly compared. Superseded by the 2026-07-10 run.
