# Deviations from DREAM.3D v6.5.171: ComputeSchmidsFilter

Part 1 lists differences from DREAM.3D v6.5.171. Part 2 records the separate SIMPLNX output-initialization defect. Both parts use one stable ID sequence; the Filter UUID is the permanent cross-reference anchor.

## Part 1 — Differences from DREAM.3D v6.5.171

### ComputeSchmidsFilter-D1

| Field | Value |
|---|---|
| **Deviation ID** | `ComputeSchmidsFilter-D1` |
| **Filter UUID** | `b4681855-0a3d-4237-97f2-5aec509115c4` |
| **Status** | active |
| **Affected released versions** | DREAM3D-NX v7.0.0 through v7.4.1. DREAM.3D v6.5.171 was affected. |
| **Fixed in** | DREAM3D-NX v7.4.2 |

**Symptom:** Cubic-High automatic Schmid factors were too large by about 0.018%, and the maximum could exceed the physical limit of 0.5.

**Root cause:** Bug in the library. `CubicOps::getSchmidFactorAndSS` divided plane and direction products by `1.732f` and `1.414f`. These values are smaller than `sqrt(3)` and `sqrt(2)`. EbsdLib 3.1.1 replaces them with full-precision constants. The override calculation already used square roots and did not have this defect.

The old-to-correct value ratios are `sqrt(3)/1.732f = 1.0000293384785181` for the plane cosine, `sqrt(2)/1.414f = 1.0001510099261918` for the direction cosine, and `sqrt(6)/(1.732f*1.414f) = 1.0001803528351113` for the factor. The last ratio gives a theoretical maximum of `0.500090176418`. A common positive multiplier does not change the maximizing slip-system index; the normalizers do not enter the pole calculation.

**Affected users:** Users of automatic Cubic-High slip systems in the affected releases. The error changes absolute factors and angle components; it does not apply to every crystal structure.

**Recommendation:** Trust SIMPLNX in DREAM3D-NX v7.4.2 or later. Its full-precision calculation agrees with the analytical expectations in `ComputeSchmidsTest.cpp`.

**Evidence:** The filter and dependency declarations were read at every `upstream/release/v7.0.0` through `upstream/release/v7.4.2`. Their EbsdLib requirements are 1.0.35, 1.0.37, 1.0.38, 1.0.39, 1.0.40, 2.1.0 and 3.1.2. The corresponding tagged `Source/EbsdLib/LaueOps/CubicOps.cpp` sources retain the literals through 2.1.0; 3.1.1 and 3.1.2 use exact constants. DREAM.3D tag `v6.5.171`, `Source/OrientationLib/LaueOps/CubicOps.cpp`, contains the literals. `vv_work:ComputeSchmidsFilter/predictions.txt` has a post-run amendment dated 2026-08-20, and the NX logs identify a build from that date. The stock legacy runner identifies `SIMPLib Version 1.2.828` in `vv_work:ComputeSchmidsFilter/logs/legacy_auto_123.log`. `vv_work:ComputeSchmidsFilter/results_compare.txt` records 10 pipeline pairs and zero prediction failures, including the expected override-index difference. The largest recorded automatic-path ratio residual is `9.638e-8`; the largest override float difference is `1.788e-7` relative. This comparison predates algorithm change `95768c9a8` (PR #1723, 2026-08-27). Current test source matches CI commit `419f9db35`. All pass in CI at `419f9db35`.

---

### ComputeSchmidsFilter-D2

| Field | Value |
|---|---|
| **Deviation ID** | `ComputeSchmidsFilter-D2` |
| **Filter UUID** | `b4681855-0a3d-4237-97f2-5aec509115c4` |
| **Status** | active |
| **Affected released versions** | DREAM.3D v6.5.171 was affected. DREAM3D-NX was not affected. |
| **Fixed in** | DREAM3D-NX v7.0.0 |

**Symptom:** Skipped angle rows contain `-301` in DREAM.3D v6.5.171 and zero in SIMPLNX.

**Root cause:** Bug in DREAM.3D. `FindSchmids::dataCheck()` passes `-301` as the initial value for `SchmidPhis` and `SchmidLambdas`. Feature 0 and features with a crystal-structure value at or above `LaueGroupEnd` keep that value because execution does not calculate their angles. The other three output arrays start at zero. SIMPLNX uses zero for skipped rows; its separate explicit-fill correction is `ComputeSchmidsFilter-D7`.

**Affected users:** Users who consume angle values for unassigned or skipped features. A valid class with no automatic slip systems is a separate case: after the D3 fix, its Schmid outputs are zero but its poles are calculated.

**Recommendation:** Trust SIMPLNX. Use phase and crystal-structure validity to identify skipped features; do not depend on the undocumented `-301` value.

**Evidence:** DREAM.3D tag `v6.5.171`, `Source/Plugins/OrientationAnalysis/OrientationAnalysisFilters/FindSchmids.cpp`, explicitly initializes both angles to `-301`. Released SIMPLNX filter sources never request that value. The v7.0.0 algorithm zeros Feature 0 and its core store initializes skipped rows to zero. `vv_work:ComputeSchmidsFilter/results_compare.txt` records the `-301` versus zero differences for the 2026-08-20 fixture. `options, skip path and phase guards` now asserts every skipped row in a 20,000-feature fixture.

---

### ComputeSchmidsFilter-D3

| Field | Value |
|---|---|
| **Deviation ID** | `ComputeSchmidsFilter-D3` |
| **Filter UUID** | `b4681855-0a3d-4237-97f2-5aec509115c4` |
| **Status** | active |
| **Affected released versions** | DREAM3D-NX v7.0.0 through v7.4.1. DREAM.3D v6.5.171 was affected. |
| **Fixed in** | DREAM3D-NX v7.4.2 |

**Symptom:** Automatic Schmid calculations could reuse a previous feature's values when a library routine did not initialize every output.

**Root cause:** Bug in the library. `HexagonalLowOps` compared candidates with the incoming factor before resetting it. `HexagonalOps` did not initialize the index or angle components. The seven classes with no automatic slip systems left the angles unchanged. Both filters reused local output variables across features. EbsdLib 3.1.1 initializes these outputs; SIMPLNX also resets its local values inside the feature loop.

The hexagonal routines use indices 1 through 6 when a candidate wins. Zero means that no candidate exceeded the initial factor of zero. Cubic-High automatic indices start at zero. These numbering conventions remain different.

**Affected users:** Users with hexagonal phases or mixed data in which a feature with calculated angles precedes a Triclinic, Monoclinic, Orthorhombic, Tetragonal or Trigonal feature.

**Recommendation:** Trust SIMPLNX in DREAM3D-NX v7.4.2 or later. Treat a zero hexagonal index as no selected slip system, not as an index into its 1-based list.

**Evidence:** Every release through `upstream/release/v7.4.1` keeps output locals outside the feature loop in `src/Plugins/OrientationAnalysis/src/OrientationAnalysis/Filters/Algorithms/ComputeSchmids.cpp`; v7.4.2 resets them inside it. The EbsdLib versions listed in D1 have incomplete initialization before 3.1.1. The corresponding DREAM.3D `v6.5.171` library routines have the same omissions. The current `options, skip path and phase guards` test checks a Triclinic feature after a Cubic-High feature. It does not directly test the hexagonal routines.

---

### ComputeSchmidsFilter-D4

| Field | Value |
|---|---|
| **Deviation ID** | `ComputeSchmidsFilter-D4` |
| **Filter UUID** | `b4681855-0a3d-4237-97f2-5aec509115c4` |
| **Status** | active |

**Symptom:** Default angle-array names and the error for a slip direction outside its plane differ between DREAM.3D v6.5.171 and SIMPLNX.

**Root cause:** Algorithmic choice during the port. DREAM.3D defaults to `SchmidPhis` and `SchmidLambdas`; SIMPLNX defaults to `Schmid_Phis` and `Schmid_Lambdas`. The perpendicularity error changes from `-1001` to `-13500`. SIMPLNX now includes the two vectors and their dot product in the error.

Both applications store cosines in automatic mode and radians in override mode. This unit change is shared behavior, not a legacy difference. In automatic mode, `m = Phis * Lambdas`. For a positive override maximum, `m = cos(Phis) * cos(Lambdas)`. When no override candidate exceeds zero, the factor and both angles remain zero; the cosine-product identity does not apply to that sentinel.

**Affected users:** Users who construct output paths, match error codes, or process angle arrays after changing the override setting.

**Recommendation:** Either interface is acceptable. Use the actual output names and error codes, and interpret angle components according to the selected mode.

**Evidence:** `FindSchmids.cpp` at DREAM.3D tag `v6.5.171` and current `ComputeSchmidsFilter.cpp` define these defaults and errors. `ComputeSchmidsTest.cpp` checks the positive-factor identities, the zero override sentinel and error `-13500`. `vv_work:ComputeSchmidsFilter/results_compare.txt` and the `legacy_override_badnormal.log`/`nx_override_badnormal.log` files in its `logs/` directory record the comparison.

---

### ComputeSchmidsFilter-D5

| Field | Value |
|---|---|
| **Deviation ID** | `ComputeSchmidsFilter-D5` |
| **Filter UUID** | `b4681855-0a3d-4237-97f2-5aec509115c4` |
| **Status** | active |

**Symptom:** With the `(001)[100]` override system and `[1,2,3]` loading, DREAM.3D v6.5.171 reports index 8 and SIMPLNX reports index 3 for the same maximum factor, `3/7`.

**Root cause:** Algorithmic choice. The override index identifies an entry in the library's symmetry-operator table. The tables contain the same 24 cubic operators but differ at 18 positions. Six operators tie at the maximum. A strict `>` comparison selects the first one in each table, so the indices differ without changing the physical maximum.

**Affected users:** Users who compare, store or select override `SlipSystems` values across application versions. Automatic Cubic-High indices instead identify the built-in slip systems.

**Recommendation:** Either result is acceptable. Interpret an override index with the library that produced it; do not treat it as a portable physical label or a unique maximizing operator.

**Evidence:** `vv_work:ComputeSchmidsFilter/symop_order.txt` gives the SIMPLNX tie set `{3,6,8,9,11,14}` and the legacy tie set `{8,9,10,12,14,16}`. Its companion `symop_order.py` reads the source tables. `vv_work:ComputeSchmidsFilter/results_compare.txt` records the index difference and matching physical factors within float precision. `override slip system path` pins index 3 for the current table order.

---

### ComputeSchmidsFilter-D6

| Field | Value |
|---|---|
| **Deviation ID** | `ComputeSchmidsFilter-D6` |
| **Filter UUID** | `b4681855-0a3d-4237-97f2-5aec509115c4` |
| **Status** | active |
| **Affected released versions** | DREAM3D-NX v7.0.0 through v7.4.1. DREAM.3D v6.5.171 was affected. |
| **Fixed in** | DREAM3D-NX v7.4.2 |

**Symptom:** Invalid phase indices, unequal feature tuple counts, empty arrays and zero input vectors could cause invalid reads or output.

**Root cause:** Bug. Both implementations used unchecked phase indices and normalized vectors without a magnitude check. DREAM.3D collected feature-array paths but did not validate their tuple counts. SIMPLNX now returns `-13501` or `-13502` for an out-of-range or negative phase. Preflight returns `-13503` through `-13508` for unequal feature tuple counts, empty feature arrays, empty Crystal Structures, and invalid loading, plane or direction vectors. The vector guards reject magnitudes below float32 epsilon.

**Affected users:** Pipelines with malformed feature/ensemble selections or zero or very small direction vectors. Valid inputs keep the same calculation.

**Recommendation:** Trust SIMPLNX in DREAM3D-NX v7.4.2 or later. Supply equal nonzero feature tuple counts, at least one ensemble tuple, valid phase indices and direction vectors with sufficient magnitude.

**Evidence:** All release sources through `upstream/release/v7.4.1` omit these guards. `upstream/release/v7.4.2` contains the current filter and algorithm source. DREAM.3D tag `v6.5.171`, `Source/Plugins/OrientationAnalysis/OrientationAnalysisFilters/FindSchmids.cpp`, also omits the checks. `preflight input validation` asserts six error codes; `options, skip path and phase guards` asserts both phase errors. `vv_work:ComputeSchmidsFilter/mutation_transcript.txt` shows that removing the phase guards caused the guard test to fail.

---

## Part 2 — SIMPLNX defects and behavior changes

### ComputeSchmidsFilter-D7

| Field | Value |
|---|---|
| **Deviation ID** | `ComputeSchmidsFilter-D7` |
| **Filter UUID** | `b4681855-0a3d-4237-97f2-5aec509115c4` |
| **Status** | active |
| **Affected released versions** | DREAM3D-NX v7.0.0 through v7.4.1 omitted the explicit fill. DREAM.3D v6.5.171 was not affected by this omission. |
| **Fixed in** | DREAM3D-NX v7.4.2 |

**Symptom:** Skipped output rows depended on the data store's initialization because the filter did not specify their fill value. The tested core store supplied zeros.

**Root cause:** Bug in the output-creation contract. The five `CreateArrayAction` calls omitted a fill value. The algorithm does not write features whose crystal-structure value is at or above `LaueGroupEnd`. DREAM.3D explicitly initialized each array, although its angle value was the separate D2 defect. SIMPLNX now passes `"0"` to all five output actions.

**Affected users:** Users with skipped features and a store whose default initialization does not supply zero. This correction does not change the observed core-store output.

**Recommendation:** Trust SIMPLNX in DREAM3D-NX v7.4.2 or later. Skipped-row values now follow an explicit filter requirement.

**Evidence:** The output actions omit a fill value in each `upstream/release/v7.0.0` through `upstream/release/v7.4.1` filter source. The v7.4.2 source contains all five explicit fills from PR #1723. The v7.4.1 `src/simplnx/DataStructure/IO/Generic/CoreDataIOManager.cpp` supplies zero to the numeric stores. In `vv_work:ComputeSchmidsFilter/mutation_transcript.txt`, removing explicit fills is the surviving mutation; transpose, quaternion-component swap, pole rounding, tie-break and phase-guard mutations fail their mapped tests. The transcript records five detected mutations out of six. The current 20,000-feature fixture checks skipped rows in all five arrays.
