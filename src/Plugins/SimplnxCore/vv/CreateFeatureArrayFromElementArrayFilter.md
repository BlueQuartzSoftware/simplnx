# V&V Report: CreateFeatureArrayFromElementArrayFilter

|        |              |
|--------|--------------|
| Plugin | SimplnxCore |
| SIMPLNX UUID | `50e1be47-b027-4f40-8f70-1283682ee3e7` |
| SIMPLNX Human Name | Create Feature Array from Element Array |
| DREAM.3D v6.5.171 equivalent | `CreateFeatureArrayFromElementArray` (SIMPL UUID `94438019-21bb-5b61-a7c3-66974b9a34dc`) |
| Verified commit | *<filled at SBIR deliverable assembly>* |
| Status | COMPLETE |
| Sign-off | Nathan Young, 2026-07-28; second engineer: Michael Jackson, 2026-08-07 |

## At a glance

| Aspect | Current state |
|--------|---------------|
| Algorithm Relationship | Minor changes to `CreateFeatureArrayFromElementArray` in DREAM.3D v6.5.171. Chunked reads and a final output write preserve the last-value rule; validation and feature-matrix sizing differ. |
| Oracle (confirmed) | Classes 1 and 4: hand-derived values, shape checks and error expectations in 10 constructed fixtures in `test/CreateFeatureArrayFromElementArrayTest.cpp`. All pass in CI. |
| Code paths enumerated | 15 of 25 exercised. Cancellation, transfer failures, resize failure, the no-growth case and remaining data types lack direct tests. |
| Tests today | 13 test cases: 2 archive regressions, 9 AF fixtures, 1 bulk-boundary oracle and 1 SIMPL conversion case. All pass in CI. |
| Exemplar archive | `6_5_test_data_1_v2.tar.gz` supplies Small IN100 inputs and two stored regression arrays. Independent expected values are in C++ fixtures. |
| Legacy comparison | Not run on the current algorithm. The previous DREAM.3D v6.5.171 comparison ran on 2026-07-23. |
| Bug flags | None |
| V&V phase | Oracle fixtures, source analysis and defect records are complete. The legacy comparison predates the current algorithm. |

## Summary

`CreateFeatureArrayFromElementArrayFilter` copies element values into feature tuples and keeps the last value for each feature. Hand-derived fixtures check output values, one-warning behavior, array shapes and input errors; all pass in CI. Four fixed defects cover negative IDs, unequal input tuple counts, empty IDs and loss of existing feature data during resizing.

## Algorithm Relationship

*Classification:* Minor changes

*Evidence:* Both implementations compare each tuple with its feature's first tuple and retain its last tuple; SIMPLNX adds input guards, grows undersized feature matrices and uses 65,536-tuple input chunks.

## Oracle

*Class:* 1 (Analytical), with 4 (Invariant) checks.

*Applied:* For each feature and component, select the last input value with that FeatureId; expect zero for unused features, at most one inconsistency warning and the specified input errors.

*Encoded:* `src/Plugins/SimplnxCore/test/CreateFeatureArrayFromElementArrayTest.cpp::SimplnxCore::CreateFeatureArrayFromElementArrayFilter: AF-1 single-component consistent` through `AF-9 error path featureIds tuple count mismatch`, plus `Bulk last-value boundary oracle` — 10 fixtures. All pass in CI.

*Second-engineer review:* Michael Jackson, 2026-08-07, for AF-1 through AF-9.

## Bugs found and fixed

All defects in this table are fixed. DREAM3D-NX v7.4.2 contains all four fixes.

| Deviation | Defect | Affected released versions | Resolution |
|-----------|--------|----------------------------|------------|
| `CreateFeatureArrayFromElementArrayFilter-D2` | Negative FeatureIds could index outside the output array. | DREAM3D-NX v7.0.0 through v7.4.1. DREAM.3D v6.5.171 was affected. | Rejects any negative ID with error `-81880` before copying. |
| `CreateFeatureArrayFromElementArrayFilter-D3` | Unequal input tuple counts could read beyond FeatureIds. | DREAM3D-NX v7.0.0 through v7.4.1. DREAM.3D v6.5.171 was affected. | Preflight rejects unequal counts with error `-81883`. |
| `CreateFeatureArrayFromElementArrayFilter-D4` | An empty FeatureIds array caused an out-of-bounds read when finding its maximum. | DREAM3D-NX v7.0.0 through v7.4.1. DREAM.3D v6.5.171 was not affected. | Rejects empty FeatureIds with error `-81882`. |
| `CreateFeatureArrayFromElementArrayFilter-D5` | Unconditional resizing could truncate existing feature arrays. | DREAM3D-NX v7.0.0 through v7.4.1. DREAM.3D v6.5.171 was not affected. | Preserves larger matrices and rejects growth that would truncate a child array with error `-81881`. |

## Code path coverage

15 of 25 paths exercised. The uncovered rows identify the remaining gaps.

Source: `src/Plugins/SimplnxCore/src/SimplnxCore/Filters/Algorithms/CreateFeatureArrayFromElementArray.cpp` (214 lines); `src/Plugins/SimplnxCore/src/SimplnxCore/Filters/CreateFeatureArrayFromElementArrayFilter.cpp` (149 lines).

Test names omit the common `SimplnxCore::CreateFeatureArrayFromElementArrayFilter: ` prefix. AF numbers refer to the full names in Test inventory.

| # | Phase | Path | Test case |
|---|-------|------|-----------|
| 1 | Preflight | Unequal element and FeatureIds tuple counts → `-81883`. | `AF-9 error path featureIds tuple count mismatch` |
| 2 | Preflight | Equal tuple counts → output action preserves input type and component shape. | `AF-1 single-component consistent`; `AF-3 three-component consistent` |
| 3 | Validation | Empty FeatureIds → `-81882`. | `AF-7 error path empty featureIds array` |
| 4 | Validation | Negative FeatureId → `-81880`. | `AF-4 error path all-negative featureIds`; `AF-8 error path mixed negative and positive featureIds` |
| 5 | Validation | Scan a full ID chunk and a partial tail to find the maximum. | `Bulk last-value boundary oracle` — 65,538 tuples |
| 6 | Validation | ID scan read fails → propagate error. | *Not directly tested. No fixture injects an ID-scan read failure.* |
| 7 | Validation | Cancel during or after the maximum-ID scan → return. | *Not directly tested. No fixture injects cancellation during validation.* |
| 8 | Sizing | Growing the matrix would shrink a larger child → `-81881`. | `AF-5 error path shrink-protection guard` |
| 9 | Sizing | Required feature count exceeds matrix size → grow matrix and children. | AF-1, AF-2, AF-3, AF-6; both `Valid filter execution` cases |
| 10 | Sizing | Matrix already has enough tuples → skip resizing. | *Not directly tested. All successful execution fixtures begin with a one-tuple feature matrix and require growth.* |
| 11 | Sizing | Matrix child is not an array → skip child-size check. | *Not directly tested. Fixtures contain array children only.* |
| 12 | Sizing | Matrix resize fails → propagate error. | *Not directly tested. No fixture injects a resize failure.* |
| 13 | Copy | Read a full element/ID chunk and a partial tail. | `Bulk last-value boundary oracle` |
| 14 | Copy | FeatureIds read fails → propagate error. | *Not directly tested. No fixture injects a copy-stage ID read failure.* |
| 15 | Copy | Element read fails → propagate error. | *Not directly tested. No fixture injects an element read failure.* |
| 16 | Copy | Cancel between chunks → return before the final write. | *Not directly tested. No fixture injects cancellation during copying.* |
| 17 | Copy | First encounter of a feature → cache its first tuple. | AF-1, AF-3; `Bulk last-value boundary oracle` — feature 1 first appears in the final tuple |
| 18 | Copy | Later tuple equals the first tuple → no warning. | AF-1, AF-3, AF-6 |
| 19 | Copy | Later tuple differs → warning `-1000`; keep its last value. | AF-2; `Bulk last-value boundary oracle` |
| 20 | Copy | More components differ after the first warning → suppress additional warnings. | `Bulk last-value boundary oracle` — three conflicting components, one warning |
| 21 | Output | Unused feature tuples, including an unmapped background, remain zero. | AF-2, AF-3, AF-6; `Bulk last-value boundary oracle` |
| 22 | Output | Write the completed feature buffer successfully. | AF-1, AF-2, AF-3, AF-6; `Bulk last-value boundary oracle` |
| 23 | Output | Final write fails → propagate error. | *Not directly tested. No fixture injects an output write failure.* |
| 24 | Dispatch | float32 scalar, uint8 three-component and int32 three-component copies. | AF-1, AF-3; `Bulk last-value boundary oracle` |
| 25 | Dispatch | Other accepted primitive data types. | *Not directly tested. No value fixture uses bool, int8, int16, uint16, uint32, int64, uint64 or float64.* |

## Test inventory

All cases are in `src/Plugins/SimplnxCore/test/CreateFeatureArrayFromElementArrayTest.cpp`. All pass in CI.

| Test case | Status | Notes |
|-----------|--------|-------|
| `SimplnxCore::CreateFeatureArrayFromElementArrayFilter: Valid filter execution - 1 Component` | kept | Loads Small IN100, grows a one-tuple matrix and compares float32 ConfidenceIndex with the stored feature array using `CompareDataArrays`. |
| `SimplnxCore::CreateFeatureArrayFromElementArrayFilter: Valid filter execution - 3 Component` | kept | Loads Small IN100, grows a one-tuple matrix and compares all uint8 IPFColors components with the stored feature array. |
| `SimplnxCore::CreateFeatureArrayFromElementArrayFilter: AF-1 single-component consistent` | new-for-V&V | Checks `[5,10,20]`, no warnings, three output tuples and matching type/component count. |
| `SimplnxCore::CreateFeatureArrayFromElementArrayFilter: AF-2 single-component inconsistent` | new-for-V&V | Checks `[0,15,20]`, exactly one warning, three tuples and matching type/component count. |
| `SimplnxCore::CreateFeatureArrayFromElementArrayFilter: AF-3 three-component consistent` | new-for-V&V | Checks all nine uint8 values against `[[0,0,0],[10,20,30],[40,50,60]]`, no warnings and matching type/component count. |
| `SimplnxCore::CreateFeatureArrayFromElementArrayFilter: AF-4 error path all-negative featureIds` | new-for-V&V | Uses IDs `[-1,-2,-1]`; expects execution error `-81880`. |
| `SimplnxCore::CreateFeatureArrayFromElementArrayFilter: AF-5 error path shrink-protection guard` | new-for-V&V | Uses a two-tuple matrix with a five-tuple child and required size four; expects `-81881`. |
| `SimplnxCore::CreateFeatureArrayFromElementArrayFilter: AF-6 gap in FeatureIds range (resize-grown tuple never written)` | new-for-V&V | Uses IDs `[0,2,0,2]`; checks `[5,0,30]`, no warnings, growth to three tuples and matching type/component count. |
| `SimplnxCore::CreateFeatureArrayFromElementArrayFilter: AF-7 error path empty featureIds array` | new-for-V&V | Uses empty element and ID arrays; expects execution error `-81882` after successful preflight. |
| `SimplnxCore::CreateFeatureArrayFromElementArrayFilter: AF-8 error path mixed negative and positive featureIds` | new-for-V&V | Uses IDs `[-1,1,2,-1]`; expects execution error `-81880`. |
| `SimplnxCore::CreateFeatureArrayFromElementArrayFilter: AF-9 error path featureIds tuple count mismatch` | new-for-V&V | Uses four element tuples and two ID tuples; expects preflight error `-81883`. |
| `SimplnxCore::CreateFeatureArrayFromElementArrayFilter: Bulk last-value boundary oracle` | kept | Checks 12 int32 output values across a 65,536-tuple boundary and a two-tuple tail, a gap feature, and exactly one warning `-1000`. |
| `SimplnxCore::CreateFeatureArrayFromElementArrayFilter: SIMPL Backwards Compatibility` | kept | Variants `SIMPL 6.5 (UUID)` and `SIMPL 6.4 (Filter_Name)` check filter identity, empty comments and four converted parameter values. |

## Exemplar archive

- **Archive:** `6_5_test_data_1_v2.tar.gz`
- **SHA512:** `585b51ba1da9784a204fe88073ca562b45afd7007cf451b0193079b885c4b4caff7cf21b13e016433b84155546ac0f73f003a8b8ebb1c58360b2c56de3027d6c`
- **Provenance:** `src/Plugins/SimplnxCore/vv/provenance/CreateFeatureArrayFromElementArrayFilter.md`

## Deviations from DREAM.3D v6.5.171

See `vv/deviations/CreateFeatureArrayFromElementArrayFilter.md` for the root cause, affected users and recommendation. This table lists only Part 1 entries.

| Deviation | Observed difference |
|-----------|---------------------|
| `CreateFeatureArrayFromElementArray-D1` | SIMPLNX grows an undersized feature matrix and preserves an oversized matrix. DREAM.3D v6.5.171 rejects both size mismatches. |
| `CreateFeatureArrayFromElementArrayFilter-D2` | SIMPLNX rejects negative FeatureIds before copying; DREAM.3D v6.5.171 can access memory outside the output array. |
| `CreateFeatureArrayFromElementArrayFilter-D3` | SIMPLNX rejects unequal element and FeatureIds tuple counts; DREAM.3D v6.5.171 does not check this condition. |
