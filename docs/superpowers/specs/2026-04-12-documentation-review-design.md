# DREAM3DNX Filter Documentation Review — Design Spec

**Date:** 2026-04-12
**Scope:** ~295 filter documentation files across SimplnxCore (154), OrientationAnalysis (53), and ITKImageProcessing (88)
**Branch:** Single PR when all batches are complete

---

## 1. Goals

### Primary Goals

1. **Clarity for non-domain-experts** — Documentation should be understandable by a general engineer or scientist (audience B) who understands data processing concepts but may not know EBSD, crystallography, or microstructure terminology.
2. **Accessibility for non-native English speakers** — Straightforward language, short sentences, consistent terminology. Avoid idioms and ambiguous phrasing.
3. **Figures where they help** — Add conceptual diagrams for filters where visual explanation meaningfully aids understanding. Not every filter needs a figure (e.g., unit conversion filters don't).
4. **Parameter guidance** — Help users determine if a filter is appropriate for their use case and what parameter values to use.

### Non-Goals (Future Work)

- **Documentation architecture consolidation** — The existing concept docs are scattered across `wrapping/python/docs/source/`, `DREAM3DNX/Documentation/Sphinx/source/Concepts/`, and `Downloads/dream3d_infographics/`. Consolidating these into a single canonical location is a future task, not part of this review.
- **Sphinx format migration** — Current filter docs are Markdown. Converting to RST or standardizing format is a future task.
- **Real-world pipeline visualizations** — Pipeline-generated screenshots require DREAM3DNX automation tooling that is still in progress. This review will build a wishlist of desired visualizations, but generating them is a future task.

---

## 2. Target Audience

**Audience (B): General engineer/scientist**

- Understands data processing, arrays, filtering, statistics concepts
- May know something about EBSD or have data they want to process
- Looks to DREAM3D to help with processing and needs help figuring out if a filter is appropriate
- May not know crystallography, quaternions, Euler angles, microstructure terminology
- English may not be their primary language

The documentation should serve both this audience AND domain experts. Domain experts need parameter guidance and edge case documentation; non-experts need conceptual explanations of what the filter does and when to use it.

---

## 3. Review Process

### Phase 1: Triage Pass

Claude autonomously reviews all ~295 filter docs and produces a tracker document (`docs/documentation_review_tracker.md`) that categorizes every filter into a priority tier.

**Triage Rubric — 5 Dimensions:**

| Dimension | What We're Assessing |
|-----------|---------------------|
| **Clarity** | Can audience (B) understand what the filter does and when to use it? |
| **Completeness** | Are all parameters explained? Are edge cases/caveats documented? |
| **Accessibility** | Is the language straightforward? Are domain terms explained or defined on first use? |
| **Figures Needed** | Would a conceptual diagram meaningfully help understanding? |
| **Real-World Viz** | Would a pipeline-generated screenshot/visualization add value? (wishlist only) |
| **Units Clarity** | Are all numeric parameters (sizes, distances, radii, tolerances, offsets) documented with explicit units (cells/voxels, degrees, physical units, dimensionless)? |

Additionally, track:
- **Concept Links** — Domain concepts that should link to shared concept pages (e.g., Quaternions, Feature IDs, Image Geometry). When the same concept appears across 3+ filters, that signals a future shared concept page.

**Priority Tiers:**

| Tier | Description | Expected Action |
|------|-------------|-----------------|
| **Tier 1 (Critical)** | Domain-heavy filters with sparse or expert-only docs. Non-expert cannot understand what the filter does. | Major rewrite: add conceptual explanation, define terms, add figures |
| **Tier 2 (Important)** | Docs are functional but assume too much domain knowledge, or would clearly benefit from figures. | Moderate rewrite: add context, simplify language, generate figures |
| **Tier 3 (Polish)** | Docs are mostly clear but could benefit from minor language cleanup, consistency, or term definitions. | Light editing: clarify wording, add term definitions inline |
| **Tier 4 (Adequate)** | Docs are already clear, complete, and accessible. Simple utility/I/O filters that don't need more. | No action needed |

### Phase 2: Batched Rewrites

After triage, work through filters in batches grouped by functional category (e.g., all alignment filters, all orientation statistics filters). Working by category ensures:

- Consistent explanations across related filters
- Shared domain concepts identified naturally
- Cross-references between related filters

**Batch workflow:**

1. Claude rewrites the batch of filter docs
2. Claude generates any needed conceptual figures (following the style guide)
3. User reviews the changes
4. User commits the batch when satisfied
5. Move to next batch

All batches accumulate on one branch and are submitted as a single PR when complete.

### Phase 3: Future Work

- Consolidate shared concept pages into a canonical location within this repo
- Generate real-world pipeline visualizations using DREAM3DNX automation
- Consider RST migration for consistency with Sphinx build

---

## 4. Tracker Document Format

The tracker (`docs/documentation_review_tracker.md`) uses a two-level structure:

**Level 1: Summary tables** — One table per priority tier for quick scanning.

```markdown
## Tier 1: Critical

| Filter | Plugin | Category |
|--------|--------|----------|
| ComputeAvgOrientations | OrientationAnalysis | Statistics |
| ConvertOrientations | OrientationAnalysis | Conversion |
```

**Level 2: Detailed checklists** — Below the summary tables, a checklist with full details for each filter, organized by tier.

```markdown
### Tier 1 — Detailed Checklist

- [ ] **ComputeAvgOrientations** (OrientationAnalysis)
  - **Clarity:** Assumes deep quaternion knowledge; no intuitive explanation
  - **Figures Needed:** Diagram showing orientations being averaged within a grain
  - **Real-World Viz:** IPF color map showing average orientation assignment
  - **Concept Links:** Quaternions, Symmetry Operators, Feature/Grain
  - **Notes:** Only 36 lines; needs significant expansion

- [ ] **ConvertOrientations** (OrientationAnalysis)
  - **Clarity:** Lists 8 types but doesn't explain when to choose each
  - **Figures Needed:** Visual comparison of orientation representations
  - **Real-World Viz:** None
  - **Concept Links:** Euler Angles, Quaternions, Rodrigues Vectors
  - **Notes:** 113 lines but very dense; needs "which should I use?" guidance
```

Filters are checked off as their rewrites are completed and committed.

---

## 5. Documentation Rewrite Guidelines

### Structure for Each Filter Doc

Every filter doc should follow this structure (scaled to the filter's complexity):

1. **Title** — Filter name
2. **Group (Subgroup)** — Category classification
3. **Description** — What the filter does in plain language. Lead with the "what" and "when would I use this?" before diving into algorithm details. If domain terms are needed, define them on first use or link to a concept page.
4. **Algorithm Details** (if applicable) — How it works, with figures where they help
5. **Parameter Guidance** (if applicable) — What the parameters mean, what values are typical, how they affect results
6. **Figures** — Conceptual diagrams embedded as PNG images
7. **Auto-generated parameter table** — `% Auto generated parameter table will be inserted here`
8. **Example Pipelines** — Links to relevant pipelines
9. **License & Copyright**
10. **DREAM3D-NX Help** — Link to discussions

### Writing Style

- **Short sentences.** Prefer active voice.
- **Define domain terms on first use.** Example: "the misorientation (the angular difference between two crystal orientations)"
- **Lead with purpose.** Start with what the filter does and why someone would use it, not how it works internally.
- **Use "you" sparingly.** Prefer "the filter" or "this operation" for international clarity.
- **Avoid idioms and colloquialisms.** Write for translation-friendliness.
- **Be specific about parameters.** Don't just name them — explain what values are typical and how changing them affects results.
- **Always specify units for numeric parameters.** Any parameter that accepts a size, distance, radius, offset, tolerance, or similar numeric value must clearly state whether the value is in **cell/voxel units** (integer cell counts), **physical units** (micrometers, degrees, etc.), or **dimensionless** (e.g., a direction vector). This is critical for users who may not know whether a "radius of 3" means 3 cells or 3 micrometers.

### Figure Decision Framework

**Add a figure when:**
- The filter operates on spatial data and the spatial relationship matters (kernels, neighborhoods, alignment)
- The filter has a multi-step algorithm that benefits from a flowchart or step diagram
- The filter transforms data in a way that's easier to show than describe (orientation conversions, geometry operations)
- The filter has parameters whose effect is non-obvious (kernel radius, dilation distance)

**Skip a figure when:**
- The filter performs a simple mathematical operation (radians to degrees, threshold)
- The filter is a basic I/O operation (read file, write file)
- The filter's behavior is fully captured by its parameter names (delete data, rename data)

---

## 6. Figure Style Guide

All generated figures follow a consistent visual style. The style guide is saved as `docs/style_palette_final.svg` for reference.

### Font

```
'Segoe UI', 'Helvetica Neue', Arial, sans-serif
```

| Element | Size | Weight | Color |
|---------|------|--------|-------|
| Figure title | 17-18px | Bold (700) | #2C3E50 |
| Section heading | 14px | Semibold (600) | #2C3E50 |
| Body text | 13px | Regular (400) | #555555 |
| Captions/notes | 11px | Italic | #888888 |

### Color Palette

**Primary (Diagram Elements):**

| Role | Color | Hex | Usage |
|------|-------|-----|-------|
| Focus | Dark Blue | #2874A6 | Center cell, primary element, section headers |
| Active | Light Blue | #AED6F1 | Kernel neighbors, included elements |
| Inactive | Light Gray | #E8ECF0 | Outside kernel, excluded elements |

**Semantic Accents:**

| Role | Color | Hex | Usage |
|------|-------|-----|-------|
| Danger/Warning | Red | #E74C3C | Thread-safety warnings, invalid approaches, common mistakes |
| Good/Correct | Green | #27AE60 | Recommended approaches, valid results, correct measurements |
| Annotation | Purple | #8E44AD | Brackets, dimensions, labels, measurements |
| Tip/Highlight | Amber | #E67E22 | Tips, parameters of interest, optional recommendations |

**Supporting:**

| Role | Color | Hex | Usage |
|------|-------|-----|-------|
| Primary text | Dark Slate | #2C3E50 | Headings, primary labels |
| Secondary text | Medium Gray | #5D6D7E | Secondary labels, descriptions |
| Card background | White | #FFFFFF | Figure card background |
| Formula background | Pale Blue | #EBF5FB | Formula/equation highlight boxes |
| Warning background | Pale Red | #FDEDEC | Warning callout box fill |
| Success background | Pale Green | #EAFAF1 | Success callout box fill |
| Annotation background | Pale Purple | #F4ECF7 | Note callout box fill |
| Tip background | Pale Amber | #FEF5E7 | Tip callout box fill |
| Page background | Off-white | #F0F2F5 | Outer page fill (when used) |

### Callout Box Styles

Four semantic callout box types are available:

- **Warning (Red)** — `!` icon, #FDEDEC background, #E74C3C border. For dangers and things to watch out for.
- **Good/Correct (Green)** — checkmark icon, #EAFAF1 background, #27AE60 border. For recommended approaches.
- **Note (Purple)** — `i` icon, #F4ECF7 background, #8E44AD border. For general annotations and clarifications.
- **Tip (Amber)** — star icon, #FEF5E7 background, #E67E22 border. For tips and highlights.

### Section Headers

Colored bars with white text, using the semantic color that best fits the section content. Default to blue (#2874A6) for neutral sections.

### Figure File Format

- Generate as SVG source files
- Convert to PNG at 2x resolution using `rsvg-convert -z 2`
- Keep both SVG and PNG in `docs/Images/` (or the plugin's `docs/Images/` directory)
- Reference the PNG in documentation markdown
- Naming convention: `FilterName_Description.png` (e.g., `ComputeKernelAvgMisorientations_1D_Radius1.png`)

---

## 7. Concept Link Strategy

During triage and rewrite, track domain concepts that appear across multiple filters. When a concept appears in 3+ filter docs, it is a candidate for a shared concept page.

**During this review:**
- Note concept links needed in the tracker document
- Define terms inline in filter docs on first use
- Use placeholder links in the format `[concept name](link-tbd)` where a shared page would be ideal

**Future task:**
- Create shared concept pages in a canonical location within this repo
- Replace placeholder links with real links
- Consolidate existing concept docs from scattered locations

**Likely concept pages based on initial survey:**
- Quaternions and Orientation Representations
- Feature IDs and Grains
- Image Geometry (Voxels, Dimensions, Spacing, Origin)
- Cell vs. Feature vs. Ensemble Data
- Misorientation
- Crystallographic Symmetry
- Reference Frames and Conventions

---

## 8. Batch Organization

Rewrites are organized by functional category. Suggested batch order (highest impact first):

| Batch | Category | Approx. Count | Rationale |
|-------|----------|---------------|-----------|
| 1 | Orientation / Crystallography Statistics | ~15 | Highest domain complexity, most likely Tier 1 |
| 2 | Alignment Filters | ~8 | Spatial concepts benefit from figures |
| 3 | Segmentation / Feature Identification | ~10 | Core workflow, many users encounter these |
| 4 | Neighbor / Kernel Operations | ~12 | Spatial concepts, parameter guidance needed |
| 5 | Geometry Creation / Manipulation | ~15 | Many options, complex parameter interactions |
| 6 | Data Manipulation (Copy, Create, Delete, Rename) | ~20 | Mostly Tier 3-4, quick to process |
| 7 | I/O Filters (Read/Write) | ~15 | Mostly Tier 3-4 |
| 8 | Image Processing (ITK wrappers) | ~88 | Large batch, many follow a template pattern |
| 9 | Remaining SimplnxCore filters | ~varies | Catch-all for uncategorized |

Exact batch boundaries will be refined after the triage pass reveals the actual distribution across tiers and categories.

---

## 9. Workflow Summary

```
Phase 1: Triage
  Claude reviews all ~295 filter docs
  → Produces docs/documentation_review_tracker.md
  → User reviews tracker, adjusts tiers if needed
  → User commits tracker

Phase 2: Batched Rewrites (repeat per batch)
  Claude rewrites batch of related filter docs
  Claude generates conceptual figures (SVG → PNG)
  → User reviews changes
  → User commits batch
  → Check off completed filters in tracker

Phase 3: Final
  All batches complete
  → Submit single PR to develop
  → Future: concept page consolidation, real-world visualizations
```

---

## 10. Files Created During This Design Phase

| File | Purpose |
|------|---------|
| `docs/style_palette_final.svg` | Visual style guide for all generated figures |
| `docs/style_palette_options.svg` | Palette options explored during design (can be deleted) |
| `kernel_radius_infographic.svg` | Original combined infographic (repo root, can be deleted) |
| `src/Plugins/OrientationAnalysis/docs/Images/ComputeKernelAvgMisorientations_*.svg` | Individual figure SVG sources |
| `src/Plugins/OrientationAnalysis/docs/Images/ComputeKernelAvgMisorientations_*.png` | Individual figure PNGs for documentation |
| `src/Plugins/OrientationAnalysis/docs/ComputeKernelAvgMisorientationsFilter.md` | Updated filter doc (serves as template for rewrite style) |
