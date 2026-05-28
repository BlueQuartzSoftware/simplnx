# Deviations from DREAM3D 6.5.171: Compute Feature Face Misorientations

This file lists every documented behavioral difference between this SIMPLNX filter and its DREAM3D 6.5.171 equivalent.

Entries are referenced by stable ID (`<FilterName>-D<N>`) from the V&V report and from public migration guidance. The ID is stable across renames; the Filter UUID field is the permanent cross-reference anchor.

---

## ComputeFeatureFaceMisorientations-D1

| Field | Value |
|---|---|
| **Deviation ID** | `ComputeFeatureFaceMisorientations-D1` |
| **Filter UUID** | `f3473af9-db77-43db-bd25-60df7230ea73` |
| **Status** | active |

**Symptom:** *Laue groups other than `Hexagonal_High` or `Cubic_High` will produce zeroes*

**Root cause:** *library*
*The version of `EBSDLib` bundled with 6.5.171 does not support the function used for `calculateMisorientation()` for all Laue classes*

**Affected users:** *datasets with a Laue grouping other than `Hexagonal_High` or `Cubic_High`*

**Recommendation:** *trust SIMPLNX*
*The `EBSDLib` library and `simplnx` source code allows for all valid Laue classes now*

---

## ComputeFeatureFaceMisorientations-D2

| Field | Value |
|---|---|
| **Deviation ID** | `ComputeFeatureFaceMisorientations-D2` |
| **Filter UUID** | `f3473af9-db77-43db-bd25-60df7230ea73` |
| **Status** | active |

**Symptom:** *Ouput arrays are completely different between versions*

**Root cause:** *algorithmic choice*
*Decison was made by Mike Jackson to bring the output in line with other Misorientation filters in `simplnx`. The output of `6.5.171` is a normalized 3-component orientation and the output of `simplnx` is a single-component angle of misorientation in degrees*

**Affected users:** *any user coming from 6.5.171 to simplnx, especially those trying to run pipelines made in 6.5.171*

**Recommendation:** *either acceptable*
*Neither output is wrong, just different representations of the same calculations*

---

## ComputeFeatureFaceMisorientations-D3

| Field | Value |
|---|---|
| **Deviation ID** | `ComputeFeatureFaceMisorientations-D3` |
| **Filter UUID** | `f3473af9-db77-43db-bd25-60df7230ea73` |
| **Status** | active |

**Symptom:** *In 6.5.171 there is no way to differentiate between a true zero misorientation and an error misorientation*

**Root cause:** *bug*
*In 6.5.171. If phase 1 is zero it is filled with zeros explicilty, if the phases don't match or the Laue class is not `Hexagonal_High` or `Cubic_High` it is left to be filled with 0s implicilty. There is no way to differentiate from true 0 misorientation.*

**Affected users:** *datasets that contain zero misorientations and other edge cases*

**Recommendation:** *trust SIMPLNX*
*In `simplnx` the edge cases explicitly produce `NaN`s in the output array to allow clear distinction between true 0 misorientation and edge cases.*

---

## ComputeFeatureFaceMisorientations-D4

| Field | Value |
|---|---|
| **Deviation ID** | `ComputeFeatureFaceMisorientations-D4` |
| **Filter UUID** | `f3473af9-db77-43db-bd25-60df7230ea73` |
| **Status** | active |

**Symptom:** Misorientation output differs from 6.5.171 by up to 0.003° in orientations near grain boundaries.

**Root cause:** Precision. SIMPLNX performs the internal orientation-matrix operations in `double`; 6.5.171 performed the same operations in `float`.

**Affected users:** Workflows that compute orientation statistics on features larger than ~10⁴ voxels, where accumulated float32 round-off becomes visible at the 10⁻³ degree level. Users who only visualize IPF colors will not notice.

**Recommendation:** Trust SIMPLNX. The 6.5.171 output was limited by float32 round-off and is not materially more correct for any downstream calculation.
