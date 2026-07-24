# Deviations from DREAM3D 6.5.171: CreateFeatureArrayFromElementArrayFilter

This file lists every documented behavioral difference between this SIMPLNX filter and its DREAM3D 6.5.171 equivalent.

Entries are referenced by stable ID (`CreateFeatureArrayFromElementArray-D<N>`) from the V&V report and from public migration guidance. The Filter UUID field is the permanent cross-reference anchor.

---

## Headline

**No deviations.** The empirical A/B comparison was run on 2026-07-23 using synthetic 8×1×1 fixtures with both 1-component (float32) and 3-component (uint8) cell arrays. SIMPL 6.5.171 and SIMPLNX produced **bit-identical output** matching the hand-derived Class 1 oracle on every fixture. No data deviations were found. No behavioral deviations were found — the warning count is identical (both implementations emit exactly one warning total per execution when any feature's cell values are inconsistent).

---

## Comparison method

| | |
|---|---|
| **Comparison type** | Runtime A/B (both implementations executed on identical input) |
| **Fixture** | Synthetic 8×1×1 image geometry; `FeatureIds=[1,2,1,2,1,2,1,2]`; `CellFloat` (float32, 1-comp): `[10,20,30,20,10,20,30,20]`; `CellRGB` (uint8, 3-comp): cells 0,2,4,6→`[10,20,30]/[70,80,90]` interleaved with cells 1,3,5,7→`[40,50,60]` |
| **Tolerance** | Bit-identical (copy-only filter; no floating-point accumulation) |
| **Comparison driver** | `/home/nyoung/Apps/DREAM3DNX-Dev/feature_from_element_vv/compare_outputs.py` |
| **Run date** | 2026-07-23 |
| **SIMPL runner** | `/home/nyoung/Downloads/DREAM3D-6.5.171-Linux-x86_64/bin/PipelineRunner` |
| **NX runner** | `/home/nyoung/Apps/DREAM3DNX-Dev/DREAM3D-Build/DREAM3DNX-Release-Linux-x64/Bin/nxrunner` |

---

## Results

| Test | SIMPL vs Oracle | NX vs Oracle | A/B |
|---|---|---|---|
| 1-component float32 (`CellFloat → FeatureFloat`) | PASS | PASS | MATCH |
| 3-component uint8 (`CellRGB → FeatureRGB`) | PASS | PASS | MATCH |

**Oracle values (hand-derived, `FeatureFloat`):** `[0.0, 30.0, 20.0]`

**Oracle values (hand-derived, `FeatureRGB`):** `[[0,0,0], [70,80,90], [40,50,60]]`

**Warning count:** Both SIMPL 6.5.171 and SIMPLNX emitted exactly **one** warning per execution:
```
Elements from Feature 1 do not all have the same value. The last value copied into Feature 1 will be used
```
The SIMPL implementation uses a `bool warningThrown = false;` guard (equivalent to SIMPLNX's `result.warnings().empty()` guard) — warning behavior is identical, not a delta.

---

## Known deviations

**None.** No data deviations and no behavioral deviations between SIMPLNX and DREAM3D 6.5.171.

---

## Migration recommendation

**Trust SIMPLNX.** For any pipeline using `CreateFeatureArrayFromElementArray`, the output array values are bit-identical between SIMPLNX and DREAM3D 6.5.171 for the same inputs. Warning behavior (one warning total when any feature's cell values are inconsistent) is also identical. No migration action required.
