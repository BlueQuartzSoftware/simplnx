# Deviations from DREAM3D 6.5.171: ComputeCAxisLocationsFilter

This file lists every documented behavioral difference between this SIMPLNX filter and its DREAM3D 6.5.171 equivalent.

Entries are referenced by stable ID (`ComputeCAxisLocationsFilter-D<N>`) from the V&V report and from public migration guidance. The ID is stable across renames; the Filter UUID field is the permanent cross-reference anchor.

---

## ComputeCAxisLocationsFilter-D1

| Field | Value |
|---|---|
| **Deviation ID** | `ComputeCAxisLocationsFilter-D1` |
| **Filter UUID** | `a51c257a-ddc1-499a-9b21-f2d25a19d098` |
| **Status** | active |

**Symptom:** For non-hexagonal cells, NX places NaN values whereas 6.5.171 places meaningless-but-finite computed values.

**Root cause:** Algorithmic choice - Intentional improvement to signal to the user the that the values for those phases could not be calculated.

**Affected users:** Anyone who has non-hexagonal phases in their input to ComputeCAxisLocationsFilter.

**Recommendation:** Trust SIMPLNX - ComputeCAxisLocationsFilter's calculation is only correct for hexagonal cells. The legacy filter computes meaningless values for non-hexagonal phases.

---

## ComputeCAxisLocationsFilter-D2

| Field | Value |
|---|---|
| **Deviation ID** | `ComputeCAxisLocationsFilter-D2` |
| **Filter UUID** | `a51c257a-ddc1-499a-9b21-f2d25a19d098` |
| **Status** | active |

**Symptom:** When there are no hexagonal phases present, NX emits an error (-3522) whereas 6.5.171 executes.

**Root cause:** Algorithmic choice - Intentional improvement to prevent running the filter on data where the filter is not valid.

**Affected users:** Anyone who has no hexagonal phases in their input to ComputeCAxisLocationsFilter.

**Recommendation:** Trust SIMPLNX - ComputeCAxisLocationsFilter's calculation is only correct for hexagonal cells. If there are no hexagonal cells, then the filter does no actual calculation.

---

## ComputeCAxisLocationsFilter-D3

| Field | Value |
|---|---|
| **Deviation ID** | `ComputeCAxisLocationsFilter-D3` |
| **Filter UUID** | `a51c257a-ddc1-499a-9b21-f2d25a19d098` |
| **Status** | active |

**Symptom:** NX emits an unconditional warning (-3521) in preflight which advises the user to make sure their data has hexagonal phases and emits a warning (-3523) if there are non-hexagonal phases. 6.5.171 does not emit any warning in either case.

**Root cause:** Algorithmic choice - Intentional improvement to warn the user that the filter's output is only valid for hexagonal phases.

**Affected users:** Anyone running the filter for the preflight warning, and anyone who has mixed non-hexagonal phases in their input to ComputeCAxisLocationsFilter.

**Recommendation:** Trust SIMPLNX - ComputeCAxisLocationsFilter's calculation is only correct for hexagonal cells.

---
