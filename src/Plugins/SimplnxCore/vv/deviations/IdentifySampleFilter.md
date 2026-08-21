# Deviations from DREAM3D 6.5.171: IdentifySample

This file lists every documented behavioral difference between this SIMPLNX filter and its DREAM3D 6.5.171 equivalent.

Entries are referenced by stable ID (`IdentifySample-D<N>`) from the V&V report and from public migration guidance. The ID is stable across renames; the Filter UUID field is the permanent cross-reference anchor.

---

## 3D mode — no deviations

Source-inspection comparison of the genuinely-3D path (`IdentifySampleFunctor<Image3D>`) against DREAM3D 6.5.171 `IdentifySample::execute()` confirmed identical algorithmic structure:

- **Phase 1 (largest-component isolation):** BFS flood-fill over all good voxels; tracks the largest connected component; all good voxels outside that component are set to false/0.
- **Phase 2 (hole fill, optional):** BFS flood-fill over all bad voxels; connected components that do not reach the volume boundary are flipped to good.

The algorithm is purely boolean/integer — no floating-point arithmetic — so precision drift cannot occur between two correct implementations of the same 3D logic.

**Confirmed parity — `checked` reset between phases:** `vv/provenance/IdentifySampleFilter.md` documents an "Internal cluster" fixture in `identify_sample_v2.tar.gz`, purpose-built to catch a bug where the `checked` BFS-visited tracker is not reset between Phase 1 and Phase 2 — voxels visited (and marked `checked=true`) while being absorbed into a non-largest component during Phase 1 would then be skipped entirely during Phase 2's hole-fill scan, since `checked` would already be `true` for them, leaving them permanently un-filled even when they qualify as an enclosed hole.

Direct source comparison confirms **both versions reset correctly and this is not a deviation**:

- **Legacy** (`DREAM3D/Source/Plugins/Processing/ProcessingFilters/IdentifySample.cpp:254`, checked out at tag `v6.5.171`): `checked.assign(totalPoints, false);` immediately before the `m_FillHoles` block.
- **SIMPLNX 3D mode** (`Algorithms/IdentifySample.cpp:112`): `checked.assign(totalPoints, false);` immediately before the `if(fillHoles)` block.
- **SIMPLNX SliceBySlice mode** (`Algorithms/IdentifySample.cpp:325`): `checked.assign(planeDim1 * planeDim2, false);` immediately before its own `if(fillHoles)` block, scoped per-slice.

The bug referenced in the test data's `generated_data/ReadMe.md` was a SIMPLNX-only regression at some prior point in development (already fixed in current source, as shown above) — it never existed in DREAM3D 6.5.171. The "Internal cluster" fixture remains in the archive as a guard against reintroducing it, not as evidence of any current or legacy deviation.

---

## IdentifySample-D1

| Field | Value |
|---|---|
| **Deviation ID** | `IdentifySample-D1` |
| **Filter UUID** | `94d47495-5a89-4c7f-a0ee-5ff20e6bd273` |
| **Affected output** | hole fill (`FillHoles=true`) on 2D / 1D `ImageGeom` |
| **Status** | Active — **proven by A/B run (2026-06-29)**; legacy is incorrect, SIMPLNX is correct |

**Symptom:** With `FillHoles=true` on a 2D or 1D `ImageGeom` (any dimension == 1), legacy `IdentifySample` never fills enclosed holes — every interior bad component is left bad. SIMPLNX correctly fills enclosed holes. Connectivity / largest-component selection (Phase 1) is **not** affected (verified identical — see below).

**Root cause — the hole-fill boundary test, NOT neighbor dispatch.** This corrects the original (source-inspection) hypothesis that the deviation lived in neighbor-offset dispatch; the A/B run showed connectivity is identical and isolated the difference to the Phase-2 boundary test. Legacy's hole-fill boundary check (`Processing/ProcessingFilters/IdentifySample.cpp:287`) is:

```cpp
if(column==0 || column==(xp-1) || row==0 || row==(yp-1) || plane==0 || plane==(zp-1))
  touchesBoundary = true;
```

For a 2D geometry `zp==1`, so `plane==0` (and `plane==(zp-1)`) is unconditionally true — **every** voxel is flagged as touching the sample boundary, so no bad component is ever "enclosed" and no hole is filled. The same applies to 1D (two degenerate axes). SIMPLNX's `EmptyZ/Y/XImage2D` (and 1D) hole-fill flags the boundary only when an in-plane neighbor is actually invalid, so a genuinely interior hole is filled.

**A/B verification (2026-06-29):** degenerate fixtures were authored as legacy `.dream3d` and run through stock 6.5.171, SIMPLNX, and a local build of the legacy source with a surgical fix applied:

- 2D `{5,5,1}` with one enclosed bad voxel, `FillHoles=true`: **stock 6.5.171 left it bad; SIMPLNX filled it.**
- Connectivity-only cases (2D `{3,4,1}` / `{1,3,4}` no-fill, 1D `{5,1,1}` with a tie-break, 3D `{4,4,4}` fill) were **byte-identical** across all three — confirming the deviation is isolated to hole-fill on degenerate geometry, and that the `>=` tie-break matches legacy.
- The **surgical fix to the local legacy build** (boundary test made dimensionality-aware: only extremes of non-degenerate dims count; an all-degenerate geometry is treated as boundary) made legacy fill the 2D hole **identically to SIMPLNX**, while leaving 3D and connectivity byte-for-byte unchanged. This pins the root cause to the boundary test.

**Affected users:** Anyone running `IdentifySampleFilter` with `FillHoles=true` on a 2D or 1D `ImageGeom` (single-slice EBSD scan, line scan) expecting parity with DREAM3D 6.5.171 — legacy silently leaves holes unfilled.

**Recommendation:** Trust SIMPLNX. Legacy cannot fill holes in degenerate geometries — a genuine, now-proven bug.

---

## Single-voxel hole fill — a SIMPLNX bug fixed to match legacy (NOT a legacy deviation)

This was originally suspected (alongside `IdentifySample-D1`) to be a legacy deficiency. The A/B run showed the opposite. For a single-voxel `{1,1,1}` `ImageGeom` with `FillHoles=true`, **legacy correctly does not fill the lone bad voxel** (its coordinate boundary test flags it as boundary). The pre-fix SIMPLNX main functor left `touchesBoundary=false` because the neighbor loop never executes when `k_NeighborCount==0` (`SingleVoxelImage`), so it wrongly filled the voxel.

The current one-line form — `bool touchesBoundary = k_NeighborCount == 0;` (`Algorithms/IdentifySample.cpp:139`) — initializes the flag to `true` for `SingleVoxelImage`, fixing SIMPLNX to match legacy. Verified by A/B: after the fix, SIMPLNX and stock 6.5.171 both leave the single bad voxel unfilled. This is a SIMPLNX correctness fix, not a deviation from DREAM3D 6.5.171.

---

## New features (no legacy code path to compare against — not deviations)

These differ from `IdentifySample-D1` in that no input could have triggered the corresponding legacy behavior at all — there is nothing to deviate from.

- **`SliceBySlice` mode** (`IdentifySampleSliceBySliceFunctor`): gated behind a parameter (`SliceBySlice`) that does not exist in DREAM3D 6.5.171 and is not mapped by `FromSIMPLJson`. No legacy pipeline could have exercised this path.
- **Cancel checks** (`m_ShouldCancel`): UX-only; no algorithmic effect on a run that completes.

*If a future run against DREAM3D 6.5.171 reveals additional deviations, add them here as `IdentifySample-D2` etc.*
