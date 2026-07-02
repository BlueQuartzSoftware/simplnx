# Deviations from DREAM3D 6.5.171: RotateSampleRefFrameFilter

This file lists every documented behavioral difference between this SIMPLNX filter and its DREAM3D 6.5.171 equivalent.

Entries are referenced by stable ID (`RotateSampleRefFrameFilter-D<N>`) from the V&V report and from public migration guidance. The ID is stable across renames; the Filter UUID field is the permanent cross-reference anchor.

Legacy equivalent: `RotateSampleRefFrame` (SIMPL UUID `{e25d9b4c-2b37-578c-b1de-cf7032b5ef19}`) in the Sampling plugin. On the **supported domain** (a 90/180/270-degree rotation about the X, Y, or Z axis) SIMPLNX and 6.5.171 are **bit-identical** — same output dimensions and same voxel values on all four A/B fixtures (see `../comparisons/RotateSampleRefFrameFilter/results.md`). The single deviation below is about rotations *outside* that domain.

---

## RotateSampleRefFrameFilter-D1

| Field | Value |
|---|---|
| **Deviation ID** | `RotateSampleRefFrameFilter-D1` |
| **Filter UUID** | `d2451dc1-a5a1-4ac2-a64d-7991669dcffc` |
| **Status** | active |

**Symptom:** For a rotation that is *not* a multiple of 90 degrees about a principal (X/Y/Z) axis — e.g. 45 degrees about Z, or any rotation about (111) — DREAM3D 6.5.171 runs and produces an output, whereas SIMPLNX rejects the rotation in preflight with error `-6850` (and rejects slice-by-slice on a slice-reordering rotation with `-6851`).

**Root cause:** Algorithmic choice. A reference-frame rotation is only a lossless re-labeling of the voxels when the rotation maps the grid onto itself (a 90-degree-multiple about a principal axis). For any other rotation the nearest-neighbor resample is lossy: it enlarges the bounding box, drops and duplicates voxels, and pads the result with background (0) values — 45° about Z of a 4×3×2 test volume, for example, turns 24 voxels into 50 with 24 introduced zeros. Legacy 6.5.171 performed this lossy resample silently (its documentation only *hedged* that the filter was "verified" for 90/180 about a principal axis). SIMPLNX makes the supported domain an enforced contract via a preflight guard (`RotateSampleRefFrameFilter.cpp`, `IsPrincipalAxis90Rotation`), so the arbitrary-rotation path can no longer be reached — arguably a latent-bug fix for a filter whose purpose is a reference-frame rotation.

**Affected users:** Anyone who previously fed a non-90-degree or non-principal-axis rotation to the legacy filter (an undocumented/unsupported use). Pipelines that used the standard EBSD sample transforms (n×90 about a principal axis, e.g. 180° about Y) are unaffected — those remain bit-identical.

**Recommendation:** Trust SIMPLNX. For an arbitrary rotation with interpolation, use the `Apply Transformation To Geometry` filter, which is designed for lossy resampling rotations (nearest-neighbor or trilinear). The legacy result for a non-principal-90 rotation was a silent lossy resample, not a faithful reference-frame rotation.
