# Filter Documentation Figure Style Guide

Visual style and file conventions for figures generated to support filter documentation. Established during the documentation review project (batches 1+) so that figures across plugins look consistent. Reference palette: `docs/style_palette_final.svg` in this repo.

## When to Add a Figure

**Add a figure when:**

- The filter operates on spatial data and the spatial relationship matters (kernels, neighborhoods, alignment, neighbor counts).
- The filter has a multi-step algorithm that benefits from a flowchart or sequence diagram.
- The filter transforms data in a way easier to show than describe (orientation conversions, geometry operations).
- The filter has parameters whose effect is non-obvious (kernel radius, dilation iterations, coordination number).

**Skip a figure when:**

- The filter performs a simple mathematical operation (radians-to-degrees, threshold).
- The filter is a basic I/O operation (read file, write file).
- The filter's behavior is fully captured by its parameter names (delete data, rename data).

## Font

```
'Segoe UI', 'Helvetica Neue', Arial, sans-serif
```

| Element | Size | Weight | Color |
|---------|------|--------|-------|
| Figure title | 17-18px | Bold (700) | #2C3E50 |
| Section heading | 14px | Semibold (600) | #2C3E50 |
| Body text | 13px | Regular (400) | #555555 |
| Captions / notes | 11px | Italic | #888888 |

## Color Palette

### Primary (Diagram Elements)

| Role | Color | Hex | Usage |
|------|-------|-----|-------|
| Focus | Dark Blue | #2874A6 | Center cell, primary element, section headers |
| Active | Light Blue | #AED6F1 | Kernel neighbors, included elements |
| Inactive | Light Gray | #E8ECF0 | Outside kernel, excluded elements |

### Semantic Accents

| Role | Color | Hex | Usage |
|------|-------|-----|-------|
| Danger / Warning | Red | #E74C3C | Thread-safety warnings, invalid approaches, common mistakes |
| Good / Correct | Green | #27AE60 | Recommended approaches, valid results, correct measurements |
| Annotation | Purple | #8E44AD | Brackets, dimensions, labels, measurements |
| Tip / Highlight | Amber | #E67E22 | Tips, parameters of interest, optional recommendations |

### Supporting

| Role | Color | Hex | Usage |
|------|-------|-----|-------|
| Primary text | Dark Slate | #2C3E50 | Headings, primary labels |
| Secondary text | Medium Gray | #5D6D7E | Secondary labels, descriptions |
| Card background | White | #FFFFFF | Figure card background |
| Formula background | Pale Blue | #EBF5FB | Formula / equation highlight boxes |
| Warning background | Pale Red | #FDEDEC | Warning callout box fill |
| Success background | Pale Green | #EAFAF1 | Success callout box fill |
| Annotation background | Pale Purple | #F4ECF7 | Note callout box fill |
| Tip background | Pale Amber | #FEF5E7 | Tip callout box fill |
| Page background | Off-white | #F0F2F5 | Outer page fill (when used) |

## Callout Boxes

Four semantic callout box types are available:

- **Warning (Red)** — `!` icon, #FDEDEC background, #E74C3C border. For dangers and things to watch out for.
- **Good / Correct (Green)** — checkmark icon, #EAFAF1 background, #27AE60 border. For recommended approaches.
- **Note (Purple)** — `i` icon, #F4ECF7 background, #8E44AD border. For general annotations and clarifications.
- **Tip (Amber)** — star icon, #FEF5E7 background, #E67E22 border. For tips and highlights.

## Section Headers

Colored bars with white text, using the semantic color that best fits the section content. Default to blue (#2874A6) for neutral sections.

## File Format and Naming

- **Author as SVG.** Hand-write or generate the source.
- **Convert to PNG at 2x resolution** using `rsvg-convert -z 2 source.svg -o source.png`. The 2x scale provides a clean appearance on high-DPI displays.
- **Keep both SVG and PNG** in the plugin's `<plugin>/docs/Images/` directory (e.g., `src/Plugins/OrientationAnalysis/docs/Images/`).
- **Reference the PNG** in the filter documentation markdown, not the SVG.
- **Naming convention:** `<FilterName>_<Description>.png`. Examples:
  - `ComputeKernelAvgMisorientations_1D_Radius1.png`
  - `ComputeAvgCAxes_HexagonalCAxis.png`
  - `ComputeSchmids_SchmidFactor.png`

## Markdown Reference Pattern

Use the figure caption as the alt text and as a short Fig. N descriptor:

```markdown
![Fig. 1: The geometric relationship between the tensile axis, slip plane, and slip direction that defines the Schmid factor.](Images/ComputeSchmids_SchmidFactor.png)
```

For paired before/after figures, use a 2-column table:

```markdown
| Before                           | After                            |
|----------------------------------|----------------------------------|
| ![](Images/ExampleFilter_1.png)  | ![](Images/ExampleFilter_2.png)  |
```

## Reference SVGs

- `docs/style_palette_final.svg` — canonical color palette, font, and callout box styles. Open in any SVG viewer to copy colors.
- Existing finished figures in `src/Plugins/OrientationAnalysis/docs/Images/` (especially `ComputeKernelAvgMisorientations_*.svg` and `ComputeAvgCAxes_HexagonalCAxis.svg`) are good reference templates to fork from.
