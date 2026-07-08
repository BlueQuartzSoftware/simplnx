# Deviations from DREAM3D 6.5.171: ConvertOrientationsFilter

This file lists every documented behavioral difference between this SIMPLNX filter and its DREAM3D 6.5.171 equivalent (`ConvertOrientations`, SIMPL UUID `e5629880-98c4-5656-82b8-c9fe2b9744de`).

Entries are referenced by stable ID (`ConvertOrientationsFilter-D<N>`) from the V&V report and from public migration guidance. The ID is stable across renames; the Filter UUID field is the permanent cross-reference anchor.

The SIMPLNX algorithm is a **Rewrite** of the filter plumbing under the retained UUID (see `../ConvertOrientationsFilter.md`). All four deviations below are consequences of that rewrite. SIMPLNX is verified-correct independently of 6.5.171 against the Class 3 (Rowenhorst 2015) / Class 1 / Class 4 oracle encoded in `test/ConvertOrientationsTest.cpp` (1032 assertions); these entries are diff-explanation, not correctness findings.

> **Comparison status:** D1, D2, D4 are derived from a side-by-side reading of both implementations (`DREAM3D/.../OrientationAnalysisFilters/ConvertOrientations.cpp` @ 6.5.171 vs `Algorithms/ConvertOrientations.cpp`). A live A/B run is tracked in the report's `Legacy comparison` row.

---

## ConvertOrientationsFilter-D1

| Field | Value |
|---|---|
| **Deviation ID** | `ConvertOrientationsFilter-D1` |
| **Filter UUID** | `501e54e6-a66f-4eeb-ae37-00e649c00d4b` (SIMPL `e5629880-98c4-5656-82b8-c9fe2b9744de`) |
| **Status** | active |

**Symptom:** For a given (input, output) representation pair, SIMPLNX and 6.5.171 may differ in the last 1–2 significant figures of float32 output. **Measured** on the toy orientation (Euler 45°/30°/60°): eu→Quaternion, eu→Axis-Angle, eu→Rodrigues, eu→Homochoric are **bit-identical**; eu→OrientationMatrix differs by ≤ 1.5e-8; eu→Cubochoric by ≤ 1.8e-6 (worst single component). Overall max |Δ| = **1.78e-6**. *Scope: the A/B measured only the six Euler-input pairs on a single in-range orientation; the other 36 legacy-shared pairs were compared by source reading only, so the numeric bound above is established for eu→X and inferred (not measured) elsewhere.*

**Root cause:** *library-generation drift (OrientationLib vs EbsdLib 3.x).* Both implementations dispatch each requested pair to a **direct** pairwise transform — 6.5.171 `OrientationConverter::convertRepresentationTo()` calls per-pair `toX()` methods that invoke e.g. `eu2om` (the Euler closed form, `OC_CONVERT_BODY(9, OrientationMatrix, eu2om, Eu2Om)`, `OrientationConverter.hpp:492`) and `eu2cu` (which chains `eu2ho→ho2cu`, `:517`); SIMPLNX calls the EbsdLib `input.toX()` members which take the same nominal routes. The residual deltas come from implementation differences accumulated between legacy **OrientationLib** and **EbsdLib 3.x** (constant definitions, expression ordering, series-evaluation details) — largest for Cubochoric, whose cube-root + series expansion is most sensitive to intermediate float32 round-off. That four of six conversions are bit-identical is consistent with this: those transforms' code paths are unchanged between library generations. Both implement the same Rowenhorst 2015 equations; neither is "more correct." *(An earlier draft of this entry attributed the deltas to legacy routing everything through a quaternion intermediate; a source check of `OrientationConverter.hpp` disproved that mechanism and it was corrected here.)* See `../comparisons/ConvertOrientationsFilter/results/comparison.md`.

**Affected users:** Anyone diffing SIMPLNX output bit-for-bit against archived 6.5.171 output. Differences (≤ ~2e-6) are below visualization and typical downstream-analysis thresholds.

**Recommendation:** *either acceptable within tolerance ~1e-5.* Trust SIMPLNX; both satisfy the Rowenhorst oracle within float32 precision, and the measured A/B difference is ≤ 1.8e-6.

---

## ConvertOrientationsFilter-D2

| Field | Value |
|---|---|
| **Deviation ID** | `ConvertOrientationsFilter-D2` |
| **Filter UUID** | `501e54e6-a66f-4eeb-ae37-00e649c00d4b` (SIMPL `e5629880-98c4-5656-82b8-c9fe2b9744de`) |
| **Status** | active |

**Symptom:** A pipeline that supplied a **`double`** (float64) orientation array to 6.5.171 cannot supply one to SIMPLNX (the input parameter accepts float32 only), and the output dtype is float32 rather than float64.

**Root cause:** *precision (deliberate scope reduction).* 6.5.171 `ConvertOrientations::execute()` branched on the input array type and ran the conversion in `double` for `DoubleArrayType` inputs (`generateRepresentation<double>`). SIMPLNX restricts the input `ArraySelectionParameter` to `DataType::float32` and converts in float32 only. For float64 inputs the legacy intermediate math carried ~16 digits vs SIMPLNX's ~7.

**Affected users:** The small number of legacy pipelines that stored orientations as `double`. Standard EBSD ingest produces float32 orientations, which are unaffected.

**Recommendation:** *trust SIMPLNX for float32 workflows.* Users with float64 orientation arrays who require double-precision conversion should note this scope reduction; for EBSD-scale data float32 is the native precision and the difference is immaterial.

---

## ConvertOrientationsFilter-D3

| Field | Value |
|---|---|
| **Deviation ID** | `ConvertOrientationsFilter-D3` |
| **Filter UUID** | `501e54e6-a66f-4eeb-ae37-00e649c00d4b` (SIMPL `e5629880-98c4-5656-82b8-c9fe2b9744de`) |
| **Status** | active |

**Symptom:** SIMPLNX offers a **Stereographic** representation (input/output type index 7) that 6.5.171 does not.

**Root cause:** *algorithmic choice (new capability).* 6.5.171 `generateRepresentation<T>` constructed a 7-element converter vector (Euler, OrientationMatrix, Quaternion, AxisAngle, Rodrigues, Homochoric, Cubochoric); Stereographic did not exist. SIMPLNX adds the 8th type, verified analytically (Class 1, `st = (x,y,z)/(1+w)`) in the unit test.

**Affected users:** None negatively — purely additive. There is no 6.5.171 output to compare against for any conversion involving Stereographic.

**Recommendation:** *trust SIMPLNX.* New capability with an independent analytical oracle; no legacy equivalent exists.

---

## ConvertOrientationsFilter-D4

| Field | Value |
|---|---|
| **Deviation ID** | `ConvertOrientationsFilter-D4` |
| **Filter UUID** | `501e54e6-a66f-4eeb-ae37-00e649c00d4b` (SIMPL `e5629880-98c4-5656-82b8-c9fe2b9744de`) |
| **Status** | active |

**Symptom:** Selecting the same representation for input and output produces an error in SIMPLNX (`-67005`) before any computation; 6.5.171 errored similarly (`-1000`) but with a different code/message, and 6.5.171 additionally emitted distinct error codes for out-of-range type indices (`-1001`/`-1002`).

**Root cause:** *algorithmic choice (preflight refactor).* SIMPLNX delegates input/output type-range validation to the `ChoicesParameter` (which emits the framework `k_Validate_OutOfRange_Error`) and keeps only the same-type check (`-67005`) and array-shape checks (`-67003`/`-67004`) in `preflightImpl`. The legacy `-1001`/`-1002`/`-1004` (converter-failure) error codes have no SIMPLNX equivalent.

**Affected users:** Scripts or tests that matched on the legacy numeric error codes `-1000`/`-1001`/`-1002`. No effect on successful conversions.

**Recommendation:** *trust SIMPLNX.* Behavior is equivalent (invalid configurations are still rejected at preflight); only the error-code surface changed.

---

## ConvertOrientationsFilter-D5

| Field | Value |
|---|---|
| **Deviation ID** | `ConvertOrientationsFilter-D5` |
| **Filter UUID** | `501e54e6-a66f-4eeb-ae37-00e649c00d4b` (SIMPL `e5629880-98c4-5656-82b8-c9fe2b9744de`) |
| **Status** | active |

**Symptom:** For Euler-angle **input** outside `[0,2π]×[0,π]×[0,2π]`, 6.5.171 and SIMPLNX produce **genuinely different orientations** (not float round-off), and 6.5.171 additionally **mutated the user's stored input array** while SIMPLNX leaves it untouched.

**Root cause:** *algorithmic choice (sanitization removed).* Every legacy `toX()` ran `sanityCheckInputData()` (`OC_CONVERT_BODY`, `OrientationConverter.hpp:386,403`); for Euler input, `EulerConverter::sanityCheckInputData()` ran `EulerSanityCheck` (`:425-446`) **in place on the actual stored array**: `fmod(φ1, 2π)`, `fmod(Φ, π)`, `fmod(φ2, 2π)` followed by sign flips for negative values. Two consequences: (a) the persisted `EulerAngles` array was silently modified as a side effect of running the filter; (b) `fmod(Φ, π)` and the sign flips are **not rotation-preserving**, so for out-of-range input the legacy conversion result corresponds to a *different orientation* than the one stored. SIMPLNX copies each tuple into a local `OrientationF` and never normalizes, converting exactly the orientation the user supplied. The A/B comparison (D1) used only in-range angles, so this difference did not appear in the measured deltas.

**Affected users:** Pipelines feeding Euler angles outside the fundamental Bunge ranges (e.g. negative angles from upstream arithmetic, or Φ ∈ (π, 2π)). Under 6.5.171 those inputs were silently rewritten and converted as a different orientation; under SIMPLNX they convert as-supplied (EbsdLib's transforms are periodic in φ1/φ2, so only Φ out of `[0,π]` yields a mathematically distinct rotation description).

**Recommendation:** *trust SIMPLNX.* Mutating input data as a side effect was a defect-prone behavior, and `fmod(Φ, π)` silently changed the orientation. Users with out-of-range Euler data should normalize it explicitly (upstream) rather than rely on a lossy implicit rewrite.
