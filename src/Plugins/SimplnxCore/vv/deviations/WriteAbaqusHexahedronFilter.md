# Deviations from DREAM3D 6.5.171: WriteAbaqusHexahedronFilter

This file lists the documented behavioral differences between this SIMPLNX filter and its DREAM3D 6.5.171 equivalent, `AbaqusHexahedronWriter`.

Entries use stable IDs (`WriteAbaqusHexahedronFilter-D<N>`). The filter UUID is the permanent cross-reference anchor.

---

## WriteAbaqusHexahedronFilter-D1

| Field | Value |
|---|---|
| **Deviation ID** | `WriteAbaqusHexahedronFilter-D1` |
| **Filter UUID** | `4bc81b8c-1594-409a-89eb-3ea8d8f061b0` |
| **Status** | active |

**Symptom:** The SIMPLNX filter requires a scalar Cell Phases array with one tuple per Image Geometry cell. A converted DREAM3D 6.5.171 pipeline fails preflight until the user selects this array.

**Root cause:** Algorithmic choice. The shared Abaqus writer includes each grain's phase ID in its element-set and material names. DREAM3D 6.5.171 did not accept phase data and cannot supply this selection during pipeline conversion. SIMPLNX does not invent a phase because a derived value can silently assign an incorrect material.

**Affected users:** Users who convert a DREAM3D 6.5.171 pipeline that contains `AbaqusHexahedronWriter` must select Cell Phases before the pipeline can run.

**Recommendation:** Trust SIMPLNX. Select the Cell Phases array that corresponds to the selected Cell Feature IDs array.

---

## WriteAbaqusHexahedronFilter-D2

| Field | Value |
|---|---|
| **Deviation ID** | `WriteAbaqusHexahedronFilter-D2` |
| **Filter UUID** | `4bc81b8c-1594-409a-89eb-3ea8d8f061b0` |
| **Status** | active |

**Symptom:** A newly added SIMPLNX filter writes reduced-integration `C3D8R` elements by default and writes the configured hourglass stiffness in each grain section. DREAM3D 6.5.171 wrote standard `C3D8` elements.

**Root cause:** Algorithmic choice. Both SIMPLNX Abaqus filters now use the same `C3D8R` default. The integration parameter remains available. The SIMPL converter explicitly selects `C3D8` because DREAM3D 6.5.171 pipelines have no integration parameter and used that element type.

**Affected users:** New SIMPLNX pipelines use `C3D8R` unless the user disables *Use Reduced Integration Elements*. Converted DREAM3D 6.5.171 pipelines continue to use `C3D8` after the required phase selection is supplied.

**Recommendation:** Trust the selected SIMPLNX behavior. Use `C3D8R` with an applicable hourglass stiffness, or disable reduced integration when the analysis requires `C3D8`.

---

## WriteAbaqusHexahedronFilter-D3

| Field | Value |
|---|---|
| **Deviation ID** | `WriteAbaqusHexahedronFilter-D3` |
| **Filter UUID** | `4bc81b8c-1594-409a-89eb-3ea8d8f061b0` |
| **Status** | active |

**Symptom:** The SIMPLNX node, element, section, element-set, and master files are not byte-compatible with DREAM3D 6.5.171 or earlier SIMPLNX Hexahedron output.

**Root cause:** Algorithmic choice. The shared writer uses `ALLNODES` and `ALLELEMENTS`, three fractional digits for coordinates, conventional C3D8 local node ordering, phase-qualified grain names, and a common include order. The physical voxel mesh is unchanged, but the element-local node and face labels differ. The optional dummy node remains available.

**Affected users:** Scripts that parse or compare the earlier text format require updates. Abaqus models that refer to element-local faces, node positions, or the former `cube`, `Grain<id>_set`, or `Grain_Mat<id>` names require review.

**Recommendation:** Trust SIMPLNX. Update downstream names and face references for the canonical shared format before running the migrated model.
