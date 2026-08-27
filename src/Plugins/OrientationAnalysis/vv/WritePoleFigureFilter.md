# V&V Report: WritePoleFigureFilter

|           |                          |
|-----------|--------------------------|
| Plugin    | OrientationAnalysis      |
| SIMPLNX UUID | 00cbb97e-a5c2-43e6-9a35-17a0f9ce26ed |
| DREAM3D 6.5.171 equivalent | WritePoleFigure (legacy SIMPL UUID `a10bb78e-fcff-553d-97d6-830a43c85385`) |
| Verified commit | *<filled at SBIR deliverable assembly>* |
| Status | COMPLETE |
| Sign-off | Michael Jackson <mike.jackson@bluequartz.net> — 2026-07-16 |

## At a glance

| Aspect                 | Current state            |
|------------------------|--------------------------|
| Algorithm Relationship | **Rewrite** — same intent (generate `<001>/<011>/<111>` pole figures) but a different output medium and rendering stack: legacy writes a **PDF per phase** via libharu; SIMPLNX creates an **image geometry** (+ optional intensity arrays) and optional raster image on disk, rendered by the **EbsdLib compositor**. |
| Oracle (confirmed)     | **Class 5 (Expert-visual)** primary + **Class 4 (Invariant)** companions. Expert side-by-side sign-off of hex and cubic renders (6.5.171 / 6.5.172 / SIMPLNX). Invariants encoded in `WritePoleFigureTest.cpp` (mask-effectiveness, hex-convention plumbing) pass on EbsdLib 3.1.0. |
| Code paths enumerated  | 10 of 13 simplnx-wrapper paths exercised in CI; the 3 uncovered are defensive/error branches noted below. (Per-Laue-class + pixel rendering is owned and byte-tested by EbsdLib upstream.) |
| Tests today            | 3 test cases — mask-effectiveness (Class 4), hex-convention plumbing (Class 4, intensity + composite paths), SIMPL 6.4/6.5 backward-compat. All pass against EbsdLib 3.1.0. |
| Exemplar archive       | `Pole_Figure_Exemplars_v6.tar.gz` (inputs only — 502 hex-Ti orientations + 251/251 mask). No baked image exemplar (deliberate: avoids coupling CI to EbsdLib pixel byte-identity). |
| Legacy comparison      | Three-way expert-visual (6.5.171 / 6.5.172 / SIMPLNX) on hex **and** cubic. 6.5.171 == 6.5.172 (byte-identical). Data (pole positions, intensity, color mapping) visually identical; 4 cosmetic/rendering deviations. |
| Bug flags              | **None.** All 4 deviations are cosmetic (axis/family labels, font) or an intentional rendering improvement (discrete vector markers). No correctness defect. |
| V&V phase              | All phases complete: oracle chosen + applied before legacy comparison, expert sign-off recorded, invariants pass on EbsdLib 3.1.0. V&V complete and signed off by Michael Jackson (technical authority) 2026-07-16. |

## Summary

`WritePoleFigureFilter` generates `<001>/<011>/<111>` (or the hexagonal/trigonal equivalents) pole figures for each phase from per-cell Euler angles, phases, and crystal structures, optionally masked. It was verified by expert (Class 5) side-by-side comparison of hex and cubic pole figures rendered through DREAM3D 6.5.171, 6.5.172, and SIMPLNX (EbsdLib 3.1.0), backed by Class 4 invariant unit tests for the simplnx-unique wiring. The pole-figure data is visually identical across all versions; the only differences are cosmetic labeling/font and an intentional discrete-marker rendering improvement — four documented, non-defect deviations.

## Algorithm Relationship

**Rewrite**

*Evidence:* SIMPLNX inherits the legacy SIMPL UUID `a10bb78e-…`, but the implementation and output are substantially different:

1. **Output medium** — legacy writes one **PDF per phase** to disk (libharu/HPDF); nothing enters the data structure. SIMPLNX creates an **ImageGeometry** with an RGB `Phase_N` array (and optional Float64 intensity arrays) in the DataStructure, and optionally writes a raster image (TIFF/PNG) to disk.
2. **Rendering stack** — legacy draws pole-figure chrome (circle, axes, labels, color bar) directly with libharu primitives; SIMPLNX delegates to the **EbsdLib pole-figure compositor** (`GeneratePoleFigureComposite`), whose rasterized output is byte-tested upstream in `PoleFigureCompositorTest::All_Laue_Classes`.
3. **Discrete mode** — SIMPLNX renders discrete figures with EbsdLib 3.1.0's **vector-marker renderer** (configurable **Discrete Marker Radius**), replacing legacy's single-black-pixel-per-orientation.
4. **New capabilities** — optional intensity Float64 arrays, MRD normalization, in-DataStructure image geometry, and the X‖a / X‖a* Hex/Trig basis convention parameter.

The shared pole-figure projection math (modified Lambert for Color, stereographic for Discrete) descends from the same lineage, which is why the rendered figures agree visually. Being a Rewrite under the same UUID, the Deviations file defends the equivalence claim.

## Oracle

*Class:* **5 (Expert-visual)** primary, **4 (Invariant)** companion.

*Justification for Class 5:* the filter's output is a rasterized pole-figure image whose correctness is inherently visual; legacy DREAM3D emits **only PDFs** (no numeric ground truth to diff), and the pixel-level rendering is owned and byte-tested by EbsdLib upstream. No Class 1–3 oracle fully specifies the rendered image. The analytically-checkable part (pole positions) and the simplnx-unique wiring are covered by the Class 4 invariants below.

*Applied:* the same 502 hex-Ti orientations (and a cubic-relabeled variant) were rendered through DREAM3D 6.5.171, 6.5.172, and SIMPLNX (EbsdLib 3.1.0) in Color and Discrete modes; a domain expert reviewed the figures side by side. Pole positions, intensity distribution, and color-intensity mapping are visually identical across all three; the only differences are the four cosmetic/rendering items in the Deviations file.

*Encoded:*
- **Class 4 (Invariant):** `test/WritePoleFigureTest.cpp::"OrientationAnalysis::WritePoleFigureFilter: Mask filter changes the rendered pole figure"` (masked output differs from unmasked by >1% of bytes → mask is wired) and `::"…: HexConvention choice reaches algorithm"` (X‖a vs X‖a* rotates the basal families 30° in both the intensity array and the composite RGB → both plumbing paths honor the convention). Both pass against EbsdLib 3.1.0.
- **Class 5 (Expert-visual):** the hex + cubic renders (legacy PDFs + SIMPLNX PNGs), signed off. Generator scripts + pipelines are committed under `Code_Review/vv/WritePoleFigure/`; the binary renders are archived to OneDrive — see the provenance sidecar and that folder's `README.md`.
- **Class 2 (Reference), cited not duplicated:** EbsdLib `PoleFigureCompositorTest::All_Laue_Classes` pins per-Laue-class pixel reproduction.

*Second-engineer review:* **Signed off by Michael Jackson (technical authority), 2026-07-16.**

## Code path coverage

10 of 13 simplnx-wrapper paths exercised in CI. The filter is a wrapper around the EbsdLib compositor; per-Laue-class projection and pixel rendering are owned/tested by EbsdLib. Logical phases: (a) preflight validation + array creation, (b) parameter→enum translation, (c) per-phase mask filtering, (d) intensity generation, (e) composite image generation.

Source: `src/Plugins/OrientationAnalysis/src/OrientationAnalysis/Filters/Algorithms/WritePoleFigure.cpp` + `WritePoleFigureFilter.cpp`.

| #  | Phase              | Path                                                                     | Test case                                                        |
|----|--------------------|--------------------------------------------------------------------------|------------------------------------------------------------------|
| 1  | (b) Param→enum     | GenerationAlgorithm = Color (0)                                          | Mask + HexConvention tests; legacy A/B (hex + cubic)             |
| 2  | (b) Param→enum     | GenerationAlgorithm = Discrete (1) → vector markers                     | `Discrete mode and marker radius reach algorithm` (CI) + legacy A/B Discrete renders (hex + cubic), expert sign-off |
| 3  | (b) Param→enum     | HexConvention X‖a (0) vs X‖a* (1)                                        | `HexConvention choice reaches algorithm` (intensity + composite) |
| 4  | (b) Param→enum     | ImageLayout (Horizontal/Vertical/Square)                                | *Not directly asserted; Horizontal exercised in all runs. EbsdLib layout enum — low-value to sweep here.* |
| 5  | (c) Mask filter    | UseMask off / on                                                        | `Mask filter changes the rendered pole figure`                   |
| 6  | (c) Per-phase      | per-phase Euler extraction; empty phase → skip                          | Single-phase fixtures exercise extraction. *Empty-phase skip not directly tested — defensive guard.* |
| 7  | (d) Intensity      | SaveIntensityDataArrays on + NormalizeToMRD                             | `HexConvention choice reaches algorithm` (SaveIntensity=true, MRD=true) |
| 8  | (d) Intensity      | crystal-structure dispatch (Cubic_High, Hexagonal_High)                 | Hex: tests + A/B; Cubic: A/B. Other Laue classes owned by EbsdLib upstream. |
| 9  | (d) Intensity      | unknown crystal structure → warning, skip                              | *Not directly tested — defensive warning branch.*                |
| 10 | (e) Composite      | SaveAsImageGeometry / WriteImageToDisk; DiscreteMarkerRadius            | `Discrete mode and marker radius reach algorithm` (1 px vs 10 px composites differ); A/B renders (write-to-disk); Mask/HexConv tests (image geometry) |
| 11 | (a) Preflight      | mask array wrong type → error `-53900`                                  | *Not directly tested — low-value validation branch.*             |
| 12 | (a) Preflight      | ImageSize ≤ 0 → error `-680002`; Discrete mode with marker radius < 1 → error `-680003` | *Guards added during review; not directly tested — low-value validation branches.* |
| 13 | (h) SIMPL convert  | 6.4 / 6.5 SIMPL JSON → Arguments (legacy ImageFormat dropped, D5)       | `SIMPL Backwards Compatibility`                                  |

## Test inventory

| Test case | Status | Notes |
|-----------|--------|-------|
| `Mask filter changes the rendered pole figure` | kept | Class 4 invariant: masked vs unmasked composite RGB differ by >1% of bytes. Consumes `Pole_Figure_Exemplars_v6`. Passes on EbsdLib 3.1.0. |
| `HexConvention choice reaches algorithm` | kept | Class 4 invariant: X‖a vs X‖a* differ in both the intensity array and the composite RGB (both plumbing paths). Passes on EbsdLib 3.1.0. |
| `Discrete mode and marker radius reach algorithm` | new-for-V&V | Class 4 invariant: Discrete vs Color composites differ; 1 px vs 10 px marker radii differ. Pins the GenerationAlgorithm and DiscreteMarkerRadius plumbing in CI (previously covered only by manual A/B renders). |
| `SIMPL Backwards Compatibility` | kept | 6.4 + 6.5 SIMPL JSON → Arguments round-trip. The legacy `ImageFormat` key is intentionally not converted (D5). |

## Exemplar archive

- **Archive:** `Pole_Figure_Exemplars_v6.tar.gz` (inputs only — 502 hex-Ti orientations + 251/251 mask + crystal structures + phase names).
- **SHA512:** *(see `test/CMakeLists.txt` `download_test_data(... Pole_Figure_Exemplars_v6.tar.gz ...)`)*
- **Provenance:** `src/Plugins/OrientationAnalysis/vv/provenance/WritePoleFigureFilter.md`
- No baked image exemplar: pixel-level reproduction is owned by EbsdLib upstream; duplicating it here couples simplnx CI to EbsdLib byte-identity (the source of prior v5 baseline drift).

## Deviations from DREAM3D 6.5.171

Established by expert (Class 5) visual comparison on hex and cubic; 6.5.171 == 6.5.172. Full renders archived to OneDrive; regeneration scripts committed under `Code_Review/vv/WritePoleFigure/`. All four are cosmetic / labeling / intentional-rendering; none is a defect.

- `WritePoleFigureFilter-D1` — axis labels `X`/`Y` (legacy) → `A1`/`A2` (SIMPLNX). Cosmetic.
- `WritePoleFigureFilter-D2` — font/text-metrics differ (libharu Helvetica → EbsdLib canvas_ity). Library, cosmetic.
- `WritePoleFigureFilter-D3` — hex/trig per-figure family labels differ but name symmetry-equivalent families. Cosmetic.
- `WritePoleFigureFilter-D4` — Discrete markers: filled circles (configurable radius) → intentional rendering improvement over legacy single black pixels.
- `WritePoleFigureFilter-D5` — legacy Image Format choice (tif/bmp/png/pdf) dropped; SIMPLNX always writes PNG. The legacy `ImageFormat` key is not converted from SIMPL JSON.

See `vv/deviations/WritePoleFigureFilter.md`.
