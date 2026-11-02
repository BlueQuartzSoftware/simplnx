# Deviations from DREAM.3D v6.5.171: WritePoleFigureFilter

Part 1 lists rendering and output differences (D1–D5). Part 2 lists fixed SIMPLNX defects (D6–D9). Both parts use the stable `WritePoleFigureFilter-D<N>` sequence.

## Part 1 — Differences from DREAM.3D v6.5.171

### WritePoleFigureFilter-D1

| Field | Value |
|---|---|
| **Deviation ID** | `WritePoleFigureFilter-D1` |
| **Filter UUID** | `00cbb97e-a5c2-43e6-9a35-17a0f9ce26ed` |
| **Status** | active |

**Symptom:** Sample-axis labels are X/Y in DREAM.3D v6.5.171 and A1/A2 in SIMPLNX.

**Root cause:** Algorithmic choice. `WritePoleFigure.cpp` supplies A1, A2 and A3 to the EbsdLib compositor. The labels identify the sample axes.

**Affected users:** Users who compare axis annotations between applications.

**Recommendation:** Trust SIMPLNX. The annotation change does not change the selected sample directions.

**Evidence:** The DREAM.3D v6.5.171 source writes X and Y with libharu. Current `src/Plugins/OrientationAnalysis/src/OrientationAnalysis/Filters/Algorithms/WritePoleFigure.cpp` sets `config.axisNames`. The visual comparison is recorded in `vv_work:WritePoleFigureFilter/README.md`.

---

### WritePoleFigureFilter-D2

| Field | Value |
|---|---|
| **Deviation ID** | `WritePoleFigureFilter-D2` |
| **Filter UUID** | `00cbb97e-a5c2-43e6-9a35-17a0f9ce26ed` |
| **Status** | active |

**Symptom:** Title, label and legend fonts have different shapes and spacing.

**Root cause:** Library. DREAM.3D v6.5.171 uses libharu Helvetica. SIMPLNX renders through the EbsdLib raster compositor, with canvas-based text metrics.

**Affected users:** Users who compare image typography or require a matching publication layout.

**Recommendation:** Either rendering is acceptable for typography. Font differences do not define a numerical tolerance for pole positions or intensities.

**Evidence:** The tagged legacy source selects Helvetica. Current `WritePoleFigureFilter.cpp::GetXCharWidth` uses Lato Bold and canvas_ity. The render comparison is recorded in `vv_work:WritePoleFigureFilter/README.md`.

---

### WritePoleFigureFilter-D3

| Field | Value |
|---|---|
| **Deviation ID** | `WritePoleFigureFilter-D3` |
| **Filter UUID** | `00cbb97e-a5c2-43e6-9a35-17a0f9ce26ed` |
| **Status** | active |

**Symptom:** The displayed family labels for hexagonal and trigonal phases use a different convention.

**Root cause:** Algorithmic choice. The EbsdLib renderer selects the family-label strings. The hexagonal basis parameter also controls pole positions and must be held constant when comparing figures.

**Affected users:** Users who read hexagonal or trigonal family labels or compare plots made with different basis conventions.

**Recommendation:** Trust SIMPLNX with the basis convention that matches the input Euler angles. Compare the physical families rather than label text alone.

**Evidence:** The 2026-07-08 visual review of hexagonal images records symmetry-equivalent family labels in `vv_work:WritePoleFigureFilter/README.md`. Current intensity and composite configurations both receive `HexConvention`; `HexConvention choice reaches algorithm` asserts output differences but does not measure a 30-degree rotation. No trigonal comparison fixture is recorded.

---

### WritePoleFigureFilter-D4

| Field | Value |
|---|---|
| **Deviation ID** | `WritePoleFigureFilter-D4` |
| **Filter UUID** | `00cbb97e-a5c2-43e6-9a35-17a0f9ce26ed` |
| **Status** | active |

**Symptom:** Discrete plots use filled-circle markers in SIMPLNX instead of the single-pixel marks used by DREAM.3D v6.5.171.

**Root cause:** Algorithmic choice. `GeneratePoleFigureComposite` selects the discrete marker renderer. The filter converts the requested marker radius in pixels to a fraction of image size.

**Affected users:** Users of Discrete mode who compare marker size and visibility.

**Recommendation:** Trust SIMPLNX. Set Discrete Marker Radius to the required display size; it controls the marker glyph.

**Evidence:** `Discrete mode and marker radius reach algorithm` compares radii of 1 and 10 pixels and also compares Color with Discrete. `vv_work:WritePoleFigureFilter/README.md` records the visual rendering difference.

---

### WritePoleFigureFilter-D5

| Field | Value |
|---|---|
| **Deviation ID** | `WritePoleFigureFilter-D5` |
| **Filter UUID** | `00cbb97e-a5c2-43e6-9a35-17a0f9ce26ed` |
| **Status** | active |

**Symptom:** DREAM.3D v6.5.171 writes a PDF per phase. SIMPLNX writes PNG files and can retain RGB image geometry and numerical count arrays.

**Root cause:** Algorithmic choice. The legacy image-format selector is inside disabled code, and execution uses `HPDF_SaveToFile`. SIMPLNX uses `PngWriter::WriteColorImage` and does not convert the obsolete `ImageFormat` key. The rewrite keeps the pole-figure purpose but changes the output container and adds numerical outputs.

**Affected users:** Users whose downstream tools expect PDF files, or whose imported pipeline contains an obsolete image-format key.

**Recommendation:** Trust SIMPLNX for PNG and data-array output. Convert PNG externally if a PDF container is required.

**Evidence:** DREAM.3D tag `v6.5.171`, `Source/Plugins/OrientationAnalysis/OrientationAnalysisFilters/WritePoleFigure.cpp`, disables the selector with `#if 0`. Current `WritePoleFigureFilter.cpp::FromSIMPLJson` omits ImageFormat. The SIMPL conversion test checks the remaining supported arguments.

---

## Part 2 — SIMPLNX defects and behavior changes

### WritePoleFigureFilter-D6

| Field | Value |
|---|---|
| **Deviation ID** | `WritePoleFigureFilter-D6` |
| **Filter UUID** | `00cbb97e-a5c2-43e6-9a35-17a0f9ce26ed` |
| **Status** | active |
| **Affected released versions** | DREAM3D-NX v7.0.0 through v7.4.1 for the image-size omission. DREAM.3D v6.5.171 was affected by that omission. No released version had the marker-radius omission. |
| **Fixed in** | DREAM3D-NX v7.4.2 |

**Symptom:** Invalid image sizes could reach allocation or rendering. During development, a zero image size also reached marker-radius division, and non-positive marker radii were accepted.

**Root cause:** Bug. Preflight lacked lower-bound checks. It now rejects image sizes below 1 before output calculations and rejects radii below 1 when Discrete mode is selected. DREAM.3D v6.5.171 also omitted an image-size lower bound; it had no marker-radius parameter.

**Affected users:** Users who supply zero or negative image sizes. No released version exposed the missing marker-radius validation.

**Recommendation:** Trust SIMPLNX in DREAM3D-NX v7.4.2 or later. Use an image size of at least 1 and a discrete marker radius of at least 1.

**Evidence:** PR #1647 (`64f5f27cb`) adds guards `-680002` and `-680003`. Direct inspection of `WritePoleFigureFilter.cpp` on `upstream/release/v7.0.0` through `upstream/release/v7.4.1` finds no image-size guard; `upstream/release/v7.4.2` has both guards. The legacy `dataCheck()` has no image-size guard. No current test directly checks these invalid inputs.

---

### WritePoleFigureFilter-D7

| Field | Value |
|---|---|
| **Deviation ID** | `WritePoleFigureFilter-D7` |
| **Filter UUID** | `00cbb97e-a5c2-43e6-9a35-17a0f9ce26ed` |
| **Status** | active |
| **Affected released versions** | DREAM3D-NX v7.0.0 through v7.4.1. DREAM.3D v6.5.171 was not affected. |
| **Fixed in** | DREAM3D-NX v7.4.2 |

**Symptom:** An imported SIMPL pipeline could fail preflight because the output geometry reused an existing input container name.

**Root cause:** Bug. Conversion mapped the crystal-structure input container to the new output geometry path. The output action then attempted to create a geometry at that existing path. Conversion now keeps the default `PoleFigure` output path. DREAM.3D v6.5.171 writes files and does not create this output geometry.

**Affected users:** Users who import a legacy pipeline whose crystal-structure array is in an existing data container.

**Recommendation:** Trust SIMPLNX in DREAM3D-NX v7.4.2 or later. Keep the output geometry path distinct from every input container.

**Evidence:** PR #1682 (`5eb69f359`) removes the erroneous container mapping. `WritePoleFigureFilter.cpp` contains it in every release source from `upstream/release/v7.0.0` through `upstream/release/v7.4.1`; `upstream/release/v7.4.2` omits it. `SIMPL Backwards Compatibility` checks the default `PoleFigure` path for both input formats.

---

### WritePoleFigureFilter-D8

| Field | Value |
|---|---|
| **Deviation ID** | `WritePoleFigureFilter-D8` |
| **Filter UUID** | `00cbb97e-a5c2-43e6-9a35-17a0f9ce26ed` |
| **Status** | active |
| **Affected released versions** | None. DREAM3D-NX v7.0.0 through v7.4.2 were not affected. DREAM.3D v6.5.171 was not affected. |
| **Fixed in** | DREAM3D-NX v7.4.2 |

**Symptom:** The output example named TIFF, and an image-write error named a TIFF writer, while execution wrote PNG.

**Root cause:** Bug. The writer changed to PNG before the example extension and error text changed. Both now identify PNG.

**Affected users:** Users of the development version with PNG output and stale TIFF text. No released DREAM3D-NX version contained this mismatch.

**Recommendation:** Trust SIMPLNX in DREAM3D-NX v7.4.2 or later. Use the PNG extension for the generated image files.

**Evidence:** PR #1631 (`ec22ac375`) switches the writer to PNG. PR #1647 (`64f5f27cb`) changes the example extension and error text. Every release source from `upstream/release/v7.0.0` through `upstream/release/v7.4.1` uses TIFF for both output and diagnostics; `upstream/release/v7.4.2` uses PNG for both. DREAM.3D v6.5.171 writes PDF through libharu.

---

### WritePoleFigureFilter-D9

| Field | Value |
|---|---|
| **Deviation ID** | `WritePoleFigureFilter-D9` |
| **Filter UUID** | `00cbb97e-a5c2-43e6-9a35-17a0f9ce26ed` |
| **Status** | active |
| **Affected released versions** | None. DREAM3D-NX v7.0.0 through v7.4.2 were not affected. DREAM.3D v6.5.171 was not affected. |
| **Fixed in** | DREAM3D-NX v7.4.2 |

**Symptom:** New filter instances temporarily selected Discrete instead of Color Intensity.

**Root cause:** Bug. A development default changed the output mode when the pipeline did not supply the parameter. The default was restored to Color Intensity before release.

**Affected users:** Users of the temporary development version who omitted Pole Figure Type. No released DREAM3D-NX version contained this default change.

**Recommendation:** Trust SIMPLNX. Set Pole Figure Type explicitly when the mode is part of a reproducible workflow.

**Evidence:** The PR #1647 merge record (`64f5f27cb`) documents the temporary default and its correction. Direct source inspection of all ten release versions from `upstream/release/v7.0.0` through `upstream/release/v7.4.2` confirms default 0 (Color Intensity). DREAM.3D v6.5.171 also defaults to Color. The current render tests supply the mode explicitly.

---
