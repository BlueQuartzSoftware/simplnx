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

**Symptom:** For a rotation that does *not* map the voxel grid onto itself — e.g. 45 degrees about Z, or 90 degrees about (111) — DREAM3D 6.5.171 runs and produces an output, whereas SIMPLNX rejects the rotation in preflight with error `-6850` (and rejects slice-by-slice on a slice-reordering rotation with `-6851`). Note the enforced domain is the full octahedral rotation group, so lossless off-principal rotations (180° about (110), 120°/240° about (111)) are *accepted*, not rejected.

**Root cause:** Algorithmic choice. A reference-frame rotation is only a lossless re-labeling of the voxels when the rotation maps the grid onto itself (a signed axis-permutation / octahedral-group rotation). For any other rotation the nearest-neighbor resample is lossy: it enlarges the bounding box, drops and duplicates voxels, and pads the result with background (0) values — 45° about Z of a 4×3×2 test volume, for example, turns 24 voxels into 50 with 24 introduced zeros. Legacy 6.5.171 performed this lossy resample silently (its documentation only *hedged* that the filter was "verified" for 90/180 about a principal axis). SIMPLNX makes the supported domain an enforced contract via a preflight guard (`RotateSampleRefFrameFilter.cpp`, `IsLosslessGridRotation`), so the arbitrary-rotation path can no longer be reached — arguably a latent-bug fix for a filter whose purpose is a reference-frame rotation.

**Affected users:** Anyone who previously fed a lossy rotation to the legacy filter (an undocumented/unsupported use). Pipelines that used the standard EBSD sample transforms (n×90 about a principal axis, e.g. 180° about Y) are unaffected — those remain bit-identical.

**Blast radius — internal callers.** `ReadH5Ebsd` (`ReadH5Ebsd.cpp`) forwards the sample-reference-frame transform stored in the `.h5ebsd` file directly into this filter with slice-by-slice enabled, and propagates preflight errors. If a file's *recommended* sample transform is not a lossless, Z-preserving rotation (for example a stored 90° about X, which trips `-6851`, or any non-group angle, which trips `-6850`), the entire H5EBSD import now fails preflight with an error naming a filter the user never placed. The prior behavior was a silently corrupted (slice-pinned) import, so failing loudly is arguably correct, but the message can be confusing in that context. The practical workaround is to uncheck *Use Recommended Transformations* in `ReadH5Ebsd` (or apply the transform separately). This is called out here so the failure mode is documented rather than surprising.

**Recommendation:** Trust SIMPLNX. For an arbitrary rotation with interpolation, use the `Apply Transformation To Geometry` filter, which is designed for lossy resampling rotations (nearest-neighbor or trilinear). The legacy result for a lossy rotation was a silent lossy resample, not a faithful reference-frame rotation. For H5EBSD imports whose recommended transform is lossy, uncheck *Use Recommended Transformations* in `ReadH5Ebsd`.
