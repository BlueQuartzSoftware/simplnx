# Deviations from DREAM3D 6.5.171: WritePoleFigureFilter

This file lists every documented behavioral difference between this SIMPLNX filter and its DREAM3D 6.5.171 equivalent.

Entries are referenced by stable ID (`WritePoleFigureFilter-D<N>`) from the V&V report and from public migration guidance. The ID is stable across renames; the Filter UUID field is the permanent cross-reference anchor.

All entries below were established by an expert (Class 5) side-by-side review of pole figures rendered from the same 502 hex-Ti orientations through DREAM3D 6.5.171, DREAM3D 6.5.172, and SIMPLNX (EbsdLib 3.1.0). 6.5.171 and 6.5.172 outputs are byte-identical. The pole-figure **data** (pole positions, intensity distribution, color-intensity mapping) is visually identical across all three; the only differences are the cosmetic / labeling / rendering items below. None is a correctness defect.

---

## WritePoleFigureFilter-D1

| Field | Value |
|---|---|
| **Deviation ID** | `WritePoleFigureFilter-D1` |
| **Filter UUID** | `00cbb97e-a5c2-43e6-9a35-17a0f9ce26ed` |
| **Status** | active |

**Symptom:** The in-plane axis annotations on each pole figure differ: legacy DREAM3D labels them **`X` / `Y`**; SIMPLNX labels them **`A1` / `A2`**.

**Root cause:** Algorithmic choice (cosmetic relabeling in the EbsdLib 3.1.0 pole-figure chrome). The axes denote the same sample-frame directions; only the text label changed.

**Affected users:** Anyone visually comparing a SIMPLNX pole figure image to a legacy one. No effect on data or pole positions.

**Recommendation:** Trust SIMPLNX. Purely a label change; the axes are the same sample directions.

---

## WritePoleFigureFilter-D2

| Field | Value |
|---|---|
| **Deviation ID** | `WritePoleFigureFilter-D2` |
| **Filter UUID** | `00cbb97e-a5c2-43e6-9a35-17a0f9ce26ed` |
| **Status** | active |

**Symptom:** Text in the rendered image (title, labels, legend, color-bar numbers) is drawn in a slightly different font / metrics than legacy.

**Root cause:** Library. Legacy rasterizes text into a PDF via libharu (Helvetica); SIMPLNX rasterizes via EbsdLib's `canvas_ity` text renderer into a raster image. Different text stack → different glyph shapes and spacing.

**Affected users:** Anyone doing a pixel-level or visual font comparison. No effect on data, pole positions, or numeric legend values.

**Recommendation:** Either acceptable. The text content is the same; only the typeface/metrics differ. This is inherent to the PDF→raster output-medium change (see report Algorithm Relationship: Rewrite).

---

## WritePoleFigureFilter-D3

| Field | Value |
|---|---|
| **Deviation ID** | `WritePoleFigureFilter-D3` |
| **Filter UUID** | `00cbb97e-a5c2-43e6-9a35-17a0f9ce26ed` |
| **Status** | active |

**Symptom:** The per-pole-figure title labels (the Miller-index family printed above each of the three figures) differ between legacy and SIMPLNX for hexagonal/trigonal phases.

**Root cause:** Algorithmic choice / library (label convention). The labels name **symmetrically equivalent** plane/direction families (e.g. the two basal families), so the labeled families are crystallographically equivalent and the rendered pole figure is the same. Only the chosen representative label string differs.

**Affected users:** Hexagonal/trigonal users reading the family label text. No effect on the plotted poles.

**Recommendation:** Trust SIMPLNX. The labels denote symmetry-equivalent families; the figures are identical.

---

## WritePoleFigureFilter-D4

| Field | Value |
|---|---|
| **Deviation ID** | `WritePoleFigureFilter-D4` |
| **Filter UUID** | `00cbb97e-a5c2-43e6-9a35-17a0f9ce26ed` |
| **Status** | active |

**Symptom:** In **Discrete** pole figures, each plotted orientation is drawn as a small filled **circle** (configurable radius) in SIMPLNX, whereas legacy DREAM3D flipped a single pixel to black per orientation.

**Root cause:** Algorithmic choice — a deliberate rendering improvement introduced with EbsdLib 3.1.0's vector-marker renderer (`GeneratePoleFigureComposite` routes discrete, non-heatmap figures to the marker renderer; the new **Discrete Marker Radius** parameter controls the marker size). The underlying pole positions are unchanged; only the per-point glyph is larger and anti-aliased.

**Affected users:** Users of the Discrete pole-figure mode. Markers are more visible than legacy single pixels (the legacy points were often nearly invisible at typical image sizes). Pole positions are identical.

**Recommendation:** Trust SIMPLNX. This is an intentional, user-controllable visibility improvement; set **Discrete Marker Radius** to taste. No data change.

---

## WritePoleFigureFilter-D5

| Field | Value |
|---|---|
| **Deviation ID** | `WritePoleFigureFilter-D5` |
| **Filter UUID** | `00cbb97e-a5c2-43e6-9a35-17a0f9ce26ed` |
| **Status** | active |

**Symptom:** Legacy DREAM3D offered an **Image Format** choice (tif/bmp/png/pdf) for the on-disk pole figure file; SIMPLNX has no such parameter and always writes a **PNG** file.

**Root cause:** Algorithmic choice (API simplification). The SIMPLNX rewrite renders through EbsdLib's raster compositor and writes the composite with `PngWriter`; the legacy multi-format option was not carried over. The legacy `ImageFormat` key is dropped during SIMPL pipeline conversion.

**Affected users:** Anyone whose legacy pipeline selected a non-PNG output format, or whose downstream tooling expects a `.tif`/`.pdf` file extension. The image content is the same composite; only the container format is fixed to PNG.

**Recommendation:** Trust SIMPLNX. PNG is lossless and universally readable; convert externally if another container is required.
