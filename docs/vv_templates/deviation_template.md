# Deviations from DREAM.3D v6.5.171: <FilterName>

This file has two parts. Part 1 lists every documented behavioral difference between this SIMPLNX filter and its DREAM.3D v6.5.171 equivalent. Part 2 lists SIMPLNX defects and SIMPLNX behavior changes that are not differences from DREAM.3D v6.5.171: defects fixed in SIMPLNX, SIMPLNX defects that are not fixed, and changes to SIMPLNX output between DREAM3D-NX releases. Omit a part that has no entries.

Entries are referenced by stable ID (`<FilterName>-D<N>`) from the V&V report and from public migration guidance. Both parts use one ID sequence. The ID is stable across renames; the Filter UUID field is the permanent cross-reference anchor.

A user reads this file. Each entry has only the fields shown below: the table, Symptom, Root cause, Affected users, Recommendation, and an optional Evidence paragraph. Do not add other fields. Outside the Evidence paragraph, name DREAM3D-NX release versions, not branches, commits, or PRs.

---

## Part 1 — Differences from DREAM.3D v6.5.171

### <FilterName>-D1

| Field | Value |
|---|---|
| **Deviation ID** | `<FilterName>-D1` |
| **Filter UUID** | `<uuid>` |
| **Status** | active *or* superseded by `<FilterName>-D<M>` *or* retired YYYY-MM-DD |

**Symptom:** *<one-sentence user-visible symptom>*

**Root cause:** *bug | precision | order of operations | library | algorithmic choice*
*One paragraph explaining the technical mechanism. Cite source files and line ranges where helpful.*

**Affected users:** *<who actually sees this — e.g., "anyone with features spanning image boundaries", "only Hex-symmetry datasets", "datasets larger than 10M voxels">*

**Recommendation:** *trust SIMPLNX | trust 6.5.171 | either acceptable within tolerance X | see quick-patch link for legacy-parity*
*One sentence justifying the recommendation.*

---

### <FilterName>-D2

| Field | Value |
|---|---|
| **Deviation ID** | `<FilterName>-D2` |
| **Filter UUID** | `<uuid>` |
| **Status** | active |

**Symptom:** ...

**Root cause:** ...

**Affected users:** ...

**Recommendation:** ...

---

## Part 2 — SIMPLNX defects and behavior changes

### <FilterName>-D3

| Field | Value |
|---|---|
| **Deviation ID** | `<FilterName>-D3` |
| **Filter UUID** | `<uuid>` |
| **Status** | active *or* superseded by `<FilterName>-D<M>` *or* retired YYYY-MM-DD |
| **Affected released versions** | *DREAM3D-NX v<first> through v<last>. State whether DREAM.3D v6.5.171 was affected.* |
| **Fixed in** | *DREAM3D-NX v<release>* *or* *The DREAM3D-NX release after v<latest affected release>* *or* *Not fixed* |

**Symptom:** *<one-sentence user-visible symptom>*

**Root cause:** *bug | behavior change*
*One paragraph explaining the technical mechanism. Cite source files and line ranges where helpful.*

**Affected users:** *<who actually sees this>*

**Recommendation:** *trust SIMPLNX | <workaround for a defect that is not fixed>*
*One sentence justifying the recommendation.*

**Evidence:** *Optional. One short paragraph for engineers: the tests, release sources, legacy source, or comparison artifacts (`vv_work:<FilterName>/<file>`) that confirm this entry. Only this paragraph can cite PR numbers, commits, or release branches.*

---

## Examples (delete this section before sign-off)

### Precision example

| Field | Value |
|---|---|
| **Deviation ID** | `ComputeEulerAngles-D1` |
| **Filter UUID** | `aaaa1111-0000-0000-0000-000000000001` *(illustrative)* |
| **Status** | active |

**Symptom:** Euler-angle output differs from 6.5.171 by up to 0.003° in orientations near grain boundaries.

**Root cause:** Precision. SIMPLNX performs the internal orientation-matrix operations in `double`; 6.5.171 performed the same operations in `float`.

**Affected users:** Workflows that compute orientation statistics on features larger than ~10⁴ voxels, where accumulated float32 round-off becomes visible at the 10⁻³ degree level. Users who only visualize IPF colors will not notice.

**Recommendation:** Trust SIMPLNX. The 6.5.171 output was limited by float32 round-off and is not materially more correct for any downstream calculation.

### Legacy-bug example

| Field | Value |
|---|---|
| **Deviation ID** | `SegmentFeatures-D2` |
| **Filter UUID** | `aaaa2222-0000-0000-0000-000000000002` *(illustrative)* |
| **Status** | active |

**Symptom:** FeatureId count on a 50×50×50 block test pattern is 27 in SIMPLNX and 26 in 6.5.171.

**Root cause:** Bug in 6.5.171. The outer segmentation loop used `< dimZ` where it should have used `<= dimZ`, silently dropping features that touched the +Z boundary. Corrected in SIMPLNX.

**Affected users:** Anyone who ran `SegmentFeatures` on datasets where a feature touched the +Z volume boundary. The missing feature was always the one nearest +Z.

**Recommendation:** Trust SIMPLNX. The 6.5.171 result was mathematically incorrect.
