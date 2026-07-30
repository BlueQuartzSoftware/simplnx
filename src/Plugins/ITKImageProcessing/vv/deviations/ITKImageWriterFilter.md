# Deviations from DREAM3D 6.5.171: ITKImageWriterFilter

This file lists every documented behavioral difference between this SIMPLNX filter and its DREAM3D 6.5.171 equivalent.

Entries are referenced by stable ID (ITKImageWriterFilter-D<N>) from the V&V report and from public migration guidance. The ID is stable across renames; the Filter UUID field is the permanent cross-reference anchor.

## ITKImageWriterFilter-D1

| Field | Value |
|---|---|
| **Deviation ID** | `ITKImageWriterFilter-D1` |
| **Filter UUID** | `a181ee3e-1678-4133-b9c5-a9dd7bfec62f` |
| **Status** | active |

**Symptom:** For a multi-slice 2D output, 6.5.171 names the first slice `<name>_0.ext`; NX defaults to `<name>_000.ext`.

**Root cause:** Algorithmic choice. NX added index offset, total-digit, and fill-character parameters and defaults the total digit count to three; 6.5.171 always wrote an unpadded zero-based suffix.

**Affected users:** Any workflow that consumes 2D image-stack filenames directly, rather than discovering them by pattern, can fail to find NX output after migration.

**Recommendation:** Trust SIMPLNX. To retain the legacy filename shape, set *Index Offset* to `0` and *Total Number of Index Digits* to `1`.

## ITKImageWriterFilter-D2

| Field | Value |
|---|---|
| **Deviation ID** | `ITKImageWriterFilter-D2` |
| **Filter UUID** | `a181ee3e-1678-4133-b9c5-a9dd7bfec62f` |
| **Status** | active |

**Symptom:** For XZ and YZ stack output, NX writes the two selected physical axes as the 2D image spacing and origin. DREAM3D 6.5.171 writes identity spacing and zero origin for the newly created 2D image.

**Root cause:** Library/API adaptation. NX constructs the temporary 2D geometry from the selected ImageGeom axes so that output metadata describes the written plane; the legacy implementation created a new 2D geometry with default metadata.

**Affected users:** Workflows that consume XZ or YZ physical image metadata can observe different spacing and origin after migration.

**Recommendation:** Trust SIMPLNX. The NX metadata preserves the source ImageGeom's physical coordinate system for the output plane.
