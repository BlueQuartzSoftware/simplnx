# Deviations from DREAM.3D v6.5.171: FillBadDataFilter

Part 1 lists source-confirmed differences from DREAM.3D v6.5.171. Part 2 lists defects in SIMPLNX selection and alias handling. All entries use one stable ID sequence.

## Part 1 — Differences from DREAM.3D v6.5.171

### FillBadDataFilter-D1

| Field | Value |
|---|---|
| **Deviation ID** | `FillBadDataFilter-D1` |
| **Filter UUID** | `a59eb864-9e6b-40bb-9292-e5281b0b4f3e` |
| **Status** | active |
| **Affected released versions** | DREAM3D-NX v7.0.0 through v7.4.1. DREAM.3D v6.5.171 was affected. |
| **Fixed in** | DREAM3D-NX v7.4.2 |

**Symptom:** A minimum defect size below 1 passed preflight.

**Root cause:** bug. The error result was created and discarded. Preflight now returns -16500. DREAM.3D v6.5.171 has no check for this invalid minimum size.

**Affected users:** Users who enter zero or a negative minimum defect size.

**Recommendation:** Trust SIMPLNX. Use a minimum size of at least 1.

**Evidence:** `Invalid Preflight Min Defect Size` checks zero and error -16500. PR #1653 (`a9d7e17e3`) fixes the discarded return. The earlier report called this defect `FillBadDataFilter-B1`. Release and legacy source checks are recorded in `../provenance/FillBadDataFilter.md`.

---

### FillBadDataFilter-D2

| Field | Value |
|---|---|
| **Deviation ID** | `FillBadDataFilter-D2` |
| **Filter UUID** | `a59eb864-9e6b-40bb-9292-e5281b0b4f3e` |
| **Status** | active |
| **Affected released versions** | DREAM3D-NX v7.0.0 through v7.4.1. DREAM.3D v6.5.171 was affected. |
| **Fixed in** | DREAM3D-NX v7.4.2 |

**Symptom:** The filter could run forever when negative cells remained without a positive face neighbor.

**Root cause:** bug. The fill loop counted remaining cells but did not check whether it assigned any source neighbor. Both current implementations stop with a warning when no assignment is possible. Unfilled cells keep their current negative markers. DREAM.3D v6.5.171 also lacks this guard.

**Affected users:** Users with an all-zero image smaller than the minimum defect size, or negative cells without a positive neighbor.

**Recommendation:** Trust SIMPLNX. The filter now terminates and reports that some cells could not be filled.

**Evidence:** PR #1653 (`a9d7e17e3`) adds the guard. `AllBadData_TerminatesWithoutHang` has nine zero cells and threshold 1; its region remains zero, so the test does not exercise the guard. Release and legacy source checks are recorded in `../provenance/FillBadDataFilter.md`.

---

### FillBadDataFilter-D3

| Field | Value |
|---|---|
| **Deviation ID** | `FillBadDataFilter-D3` |
| **Filter UUID** | `a59eb864-9e6b-40bb-9292-e5281b0b4f3e` |
| **Status** | active |
| **Affected released versions** | DREAM3D-NX v7.0.0 through v7.4.1. DREAM.3D v6.5.171 was affected. |
| **Fixed in** | DREAM3D-NX v7.4.2 |

**Symptom:** Negative input FeatureIds could cause an invalid vote-array allocation or access.

**Root cause:** bug. The maximum-ID scan compared a signed FeatureId with an unsigned maximum. A negative ID became a large unsigned value. Both current implementations count only positive IDs when sizing vote storage. DREAM.3D v6.5.171 uses the same signed/unsigned comparison.

**Affected users:** Users whose input FeatureIds contain negative values.

**Recommendation:** Trust SIMPLNX. The maximum-ID scan now excludes negative values from allocation sizing.

**Evidence:** PR #1653 (`a9d7e17e3`) adds the positive-ID condition. No current test supplies negative input IDs. Release and legacy source checks are recorded in `../provenance/FillBadDataFilter.md`.

---

### FillBadDataFilter-D4

| Field | Value |
|---|---|
| **Deviation ID** | `FillBadDataFilter-D4` |
| **Filter UUID** | `a59eb864-9e6b-40bb-9292-e5281b0b4f3e` |
| **Status** | active |
| **Affected released versions** | DREAM3D-NX v7.0.0 through v7.4.1. DREAM.3D v6.5.171 was affected. |
| **Fixed in** | DREAM3D-NX v7.4.2 |

**Symptom:** A region larger than INT32_MAX cells could be filled instead of retained.

**Root cause:** bug. The size comparison narrowed the component count to Int32. Large counts could become negative and pass the small-region test. Current BFS compares `usize` counts; CCL compares `uint64` counts. DREAM.3D v6.5.171 also narrows the component count to Int32.

**Affected users:** Users with a connected defect region larger than 2,147,483,647 cells.

**Recommendation:** Trust SIMPLNX. Component sizes now retain their full width during classification.

**Evidence:** PR #1653 (`a9d7e17e3`) removes the CCL size cast. The release sources through v7.4.0 use an Int32 cast on the BFS visit-list size; v7.4.1 uses an Int32 cast on the CCL component size. The fixtures do not reach this size. Release and legacy source checks are recorded in `../provenance/FillBadDataFilter.md`.

---

### FillBadDataFilter-D5

| Field | Value |
|---|---|
| **Deviation ID** | `FillBadDataFilter-D5` |
| **Filter UUID** | `a59eb864-9e6b-40bb-9292-e5281b0b4f3e` |
| **Status** | active |
| **Affected released versions** | DREAM3D-NX v7.0.0 through v7.4.1. DREAM.3D v6.5.171 was affected. |
| **Fixed in** | DREAM3D-NX v7.4.2 |

**Symptom:** A large image could copy an invalid neighbor tuple during filling.

**Root cause:** bug. The neighbor map stored indices as Int32. An index above INT32_MAX could become negative or wrap. Current BFS uses `int64` neighbor entries; CCL uses 64-bit candidate indices. DREAM.3D v6.5.171 stores its neighbor map in an Int32 array.

**Affected users:** Users whose selected fill source has a linear index above 2,147,483,647.

**Recommendation:** Trust SIMPLNX. The neighbor map now preserves 64-bit indices.

**Evidence:** PR #1653 (`a9d7e17e3`) widens the neighbor map. The current CCL uses `FillPair` records. The fixtures do not reach this index range. Release and legacy source checks are recorded in `../provenance/FillBadDataFilter.md`.

---

## Part 2 — SIMPLNX defects and behavior changes

### FillBadDataFilter-D6

| Field | Value |
|---|---|
| **Deviation ID** | `FillBadDataFilter-D6` |
| **Filter UUID** | `a59eb864-9e6b-40bb-9292-e5281b0b4f3e` |
| **Status** | active |
| **Affected released versions** | DREAM3D-NX v7.0.0 through v7.4.2. DREAM.3D v6.5.171 was not affected. |
| **Fixed in** | The DREAM3D-NX release after v7.4.2 |

**Symptom:** FeatureIds from another geometry could be interpreted with the selected image dimensions.

**Root cause:** bug. The image and FeatureIds parameters were selected independently, without checking that they belonged together. The filter now checks the resolved Cell Attribute Matrix and returns -16501 for an invalid selection. DREAM.3D v6.5.171 derives the geometry from the FeatureIds container, so it has no independent image-selection path.

**Affected users:** Users who select FeatureIds outside the selected image geometry.

**Recommendation:** Trust SIMPLNX. Select FeatureIds from the selected image geometry's Cell Attribute Matrix.

**Evidence:** PR #1829 (`419f9db35`) adds the containment checks. `Strict Feature Ids` checks rejection even when the other Attribute Matrix has the same tuple shape. Separate checks reject modified Phases (-16502) and ignored arrays (-16503) outside the Cell Attribute Matrix. Release and legacy source checks are recorded in `../provenance/FillBadDataFilter.md`.

---

### FillBadDataFilter-D7

| Field | Value |
|---|---|
| **Deviation ID** | `FillBadDataFilter-D7` |
| **Filter UUID** | `a59eb864-9e6b-40bb-9292-e5281b0b4f3e` |
| **Status** | active |
| **Affected released versions** | DREAM3D-NX v7.0.0 through v7.4.2. DREAM.3D v6.5.171 was not affected. |
| **Fixed in** | The DREAM3D-NX release after v7.4.2 |

**Symptom:** An array selected through an alias could be modified despite appearing in Attribute Arrays to Ignore.

**Root cause:** bug. Array discovery used the geometry's Cell Attribute Matrix path. The ignored list could name the same array through another parent path, so path equality failed. The filter now normalizes FeatureIds and ignored paths before reporting and applying modifications. DREAM.3D v6.5.171 has no shared-parent alias path.

**Affected users:** Users who access a Cell Attribute Matrix through more than one parent.

**Recommendation:** Trust SIMPLNX. Ignored arrays now remain unchanged when selected through an alias.

**Evidence:** PR #1829 (`419f9db35`) adds path normalization. `Ignored Array Cell AM Alias` checks unchanged sentinel values, changed FeatureIds and modification actions. Released `GetAllChildDataPaths()` compares paths rather than resolved object identities. Release and legacy source checks are recorded in `../provenance/FillBadDataFilter.md`.

---
