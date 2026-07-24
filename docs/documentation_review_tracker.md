# DREAM3DNX Filter Documentation Review Tracker

**Design Spec:** [docs/superpowers/specs/2026-04-12-documentation-review-design.md](superpowers/specs/2026-04-12-documentation-review-design.md)
**Style Guide:** [docs/style_palette_final.svg](style_palette_final.svg)
**Branch:** topic/documentation_update (will submit as single PR when complete)

---

## Progress Summary

| Batch | Category | Total Filters | Triaged | Rewritten | Status |
|-------|----------|---------------|---------|-----------|--------|
| 1 | Orientation / Crystallography Statistics | 17 | 17 | 17 | Complete |
| 2 | Alignment Filters | 5 | 5 | 5 | Complete |
| 3 | Segmentation / Feature Identification | 10 | 10 | 10 | Complete |
| 4 | Neighbor / Kernel Operations | 11 | 11 | 11 | Complete |
| 5 | Geometry Creation / Manipulation | 15 | 15 | 15 | Complete |
| 6 | Data Manipulation (Copy, Create, Delete, Rename) | 21 | 21 | 21 | Complete |
| 7 | I/O Filters (Read/Write) | 44 | 44 | 44 | Complete |
| 8 | Image Processing (ITK wrappers) | 88 | 88 | 88 | Complete |
| 9 | Remaining SimplnxCore filters | 73 | 73 | 73 | Complete |

---

## Batch 1: Orientation / Crystallography Statistics

**Plugin:** OrientationAnalysis
**Filters:** 17 (15 crystallography + 2 morphological)

### Tier 1 — Critical

| Filter | Plugin | Status |
|--------|--------|--------|
| ComputeAvgCAxes | OrientationAnalysis | Done |
| ComputeCAxisLocations | OrientationAnalysis | Done |
| ComputeGBCD | OrientationAnalysis | Done |
| ComputeSchmids | OrientationAnalysis | Done |

- [x] **ComputeAvgCAxesFilter** (OrientationAnalysis)
  - **Clarity:** Assumed deep quaternion/transform knowledge; no explanation of what C-axis is
  - **Figures Needed:** Hexagonal unit cell with C-axis labeled
  - **Real-World Viz:** IPF color map showing C-axis orientations
  - **Concept Links:** C-axis, hexagonal materials, quaternions, reference frames
  - **Changes Made:** Added "What is the C-Axis?" section with example materials, plain-language algorithm, hexagonal-only explanation. Added hexagonal unit cell figure.

- [x] **ComputeCAxisLocationsFilter** (OrientationAnalysis)
  - **Clarity:** Very sparse, just restated algorithm
  - **Figures Needed:** Same hexagonal C-axis figure (shared with ComputeAvgCAxes)
  - **Real-World Viz:** C-axis direction map colored by orientation
  - **Concept Links:** C-axis, hexagonal materials, quaternions, reference frames
  - **Changes Made:** Added "What is the C-Axis?" section, upper-hemisphere convention note, comparison with ComputeAvgCAxes to help users pick the right filter. Shared hexagonal unit cell figure.

- [x] **ComputeGBCDFilter** (OrientationAnalysis)
  - **Clarity:** 2-sentence stub; non-expert cannot understand what GBCD is
  - **Figures Needed:** Conceptual diagram of 5D boundary space (deferred -- complex)
  - **Real-World Viz:** GBCD pole figure output screenshot
  - **Concept Links:** grain boundaries, misorientation, boundary normals, MRD units
  - **Changes Made:** Added "What is the GBCD?" section explaining 5 parameters, MRD units, algorithm steps, resolution parameter. Added Rohrer citations [1][2]. Cross-reference to metric-based filter.

- [x] **ComputeSchmidsFilter** (OrientationAnalysis)
  - **Clarity:** Formula presented but no explanation of slip systems or physical meaning
  - **Figures Needed:** Diagram showing tensile axis, slip plane, slip direction, and angles φ/λ
  - **Real-World Viz:** Microstructure colored by Schmid factor
  - **Concept Links:** slip systems, crystal plasticity, loading axis, CRSS
  - **Changes Made:** Added "What is the Schmid Factor?" section with plain-language slip explanation, 0-0.5 range meaning, 4-step algorithm, CRSS limitation note. Added Schmid factor geometric diagram.

### Tier 2 — Important

| Filter | Plugin | Status |
|--------|--------|--------|
| ComputeAvgOrientations | OrientationAnalysis | Done |
| ComputeBoundaryStrengths | OrientationAnalysis | Done |
| ComputeFeatureNeighborCAxisMisalignments | OrientationAnalysis | Done |
| ComputeFeatureNeighborMisorientations | OrientationAnalysis | Done |
| ComputeFeatureReferenceCAxisMisorientations | OrientationAnalysis | Done |
| ComputeFeatureReferenceMisorientations | OrientationAnalysis | Done |
| ComputeSlipTransmissionMetrics | OrientationAnalysis | Done |
| ComputeTwinBoundaries | OrientationAnalysis | Done |
| ComputeShapes | OrientationAnalysis | Done |

- [x] **ComputeAvgOrientationsFilter** (OrientationAnalysis)
  - **Clarity:** Comprehensive but very math-heavy; 3 methods with dense statistical notation
  - **Figures Needed:** Diagram showing orientations being averaged within a grain; vMF distribution concept
  - **Real-World Viz:** IPF color map showing average orientation assignment
  - **Concept Links:** quaternions, Fundamental Zone, symmetry operators, Euler angles, EM algorithm
  - **Changes Made:** Added introductory paragraph explaining what average orientation means and why it matters for downstream filters. Existing method descriptions were already thorough; kept intact.

- [x] **ComputeBoundaryStrengthsFilter** (OrientationAnalysis)
  - **Clarity:** Relies on citations for metric definitions; readers need access to papers
  - **Figures Needed:** Slip transmission across grain boundary concept diagram
  - **Real-World Viz:** Feature boundary map colored by boundary strength metrics
  - **Concept Links:** slip transmission, Luster-Morris parameter, fracture initiation, grain boundaries
  - **Changes Made:** Clarified relationship to ComputeSlipTransmissionMetrics (same metrics, per-Face storage). Added cross-reference for detailed explanations. Explained M' value meaning.

- [x] **ComputeFeatureNeighborCAxisMisalignmentsFilter** (OrientationAnalysis)
  - **Clarity:** Good basic description but "misalignment" vs "misorientation" distinction unclear
  - **Figures Needed:** Diagram showing C-axis misalignment angle between two grains
  - **Real-World Viz:** Neighbor misalignment values on microstructure
  - **Concept Links:** C-axis, misalignment angle, hexagonal crystals, neighbor relationships
  - **Changes Made:** Distinguished C-axis misalignment from full misorientation. Explained flexible neighbor list concept. Cross-referenced ComputeAvgCAxes for hexagonal-only explanation.

- [x] **ComputeFeatureNeighborMisorientationsFilter** (OrientationAnalysis)
  - **Clarity:** Straightforward but minimal
  - **Figures Needed:** Diagram showing misorientation angle concept between neighboring grains
  - **Real-World Viz:** Microstructure with neighbor misorientations displayed
  - **Concept Links:** misorientation, orientation angle, grain boundaries
  - **Changes Made:** Added plain-language explanation of misorientation. Explained NaN constraint (different crystal structures not comparable). Clarified optional average output.

- [x] **ComputeFeatureReferenceCAxisMisorientationsFilter** (OrientationAnalysis)
  - **Clarity:** Clear what it does but doesn't explain why this metric matters
  - **Figures Needed:** Diagram showing cell-to-average C-axis deviation concept
  - **Real-World Viz:** Misalignment map within a feature showing spatial variation
  - **Concept Links:** C-axis misalignment, hexagonal materials, orientation variation
  - **Changes Made:** Reframed around intragranular orientation gradients. Explained what average/std dev outputs tell you (low = uniform, high = internal variation). Cross-referenced ComputeAvgCAxes.

- [x] **ComputeFeatureReferenceMisorientationsFilter** (OrientationAnalysis)
  - **Clarity:** Good. Two reference orientation options clearly explained with use cases
  - **Figures Needed:** Legend for existing color scale images would help interpretation
  - **Real-World Viz:** Already has 3 images showing IPF colors and two reference orientation results
  - **Concept Links:** misorientation, plastic deformation, reference orientation, IPF colors
  - **Changes Made:** Added purpose statement about detecting deformation. Rewrote reference orientation options with physical reasoning for when to use each.

- [x] **ComputeSlipTransmissionMetricsFilter** (OrientationAnalysis)
  - **Clarity:** Relies on citations; metrics not explained in the documentation itself
  - **Figures Needed:** Slip transmission concept diagram (same as ComputeBoundaryStrengths)
  - **Real-World Viz:** Feature pairs with transmission metrics visualized
  - **Concept Links:** slip transmission, slip planes/directions, grain boundaries, Luster-Morris
  - **Changes Made:** Added introductory explanation of slip transmission concept. Explained M' value meaning (1.0 = perfect alignment). Added cross-reference to ComputeBoundaryStrengths.

- [x] **ComputeTwinBoundariesFilter** (OrientationAnalysis)
  - **Clarity:** Good sigma-3 identification logic but "incoherence" concept needs definition
  - **Figures Needed:** Sigma-3 twin relationship diagram; incoherence concept
  - **Real-World Viz:** Microstructure with twin boundaries highlighted
  - **Concept Links:** twin boundaries, sigma-3, misorientation axis, CSL, incoherence
  - **Changes Made:** Added "What is a Twin Boundary?" section explaining Sigma-3 twins and their significance. Explained incoherence in plain terms (0 = perfect coherent twin).

- [x] **ComputeShapesFilter** (OrientationAnalysis)
  - **Clarity:** Good algorithmic steps but missing interpretation guidance
  - **Figures Needed:** Best-fit ellipsoid concept; Omega3 sphericity measure
  - **Real-World Viz:** Features colored by aspect ratio or Omega3
  - **Concept Links:** principal moments, ellipsoid fitting, eigenvalues, Omega3, aspect ratios
  - **Changes Made:** Added "What This Filter Produces" section explaining each output. Defined Omega3 (1.0 = sphere). Cross-referenced triangle geometry version.

### Tier 3 — Polish

| Filter | Plugin | Status |
|--------|--------|--------|
| ComputeGBCDMetricBased | OrientationAnalysis | Done |
| ComputeGBPDMetricBased | OrientationAnalysis | Done |
| ComputeShapesTriangleGeom | OrientationAnalysis | Done |

- [x] **ComputeGBCDMetricBasedFilter** (OrientationAnalysis)
  - **Clarity:** Excellent technical documentation with clear metric-based vs bin-based distinction
  - **Figures Needed:** Maybe one more diagram showing metric distances visually
  - **Real-World Viz:** Already has 3 excellent figures (GBCD result, sampling points, error maps)
  - **Concept Links:** GBCD, metrics, boundary space, MRD units
  - **Changes Made:** Restructured dense metric explanation into numbered two-stage process. Added note about advantages over binning (avoids discretization artifacts).

- [x] **ComputeGBPDMetricBasedFilter** (OrientationAnalysis)
  - **Clarity:** Good. References GBCD Metric-Based filter effectively
  - **Figures Needed:** Plane normal vs misorientation sampling differences
  - **Real-World Viz:** Already has 1 good figure
  - **Concept Links:** GBPD, grain boundary plane distribution, boundary normal
  - **Changes Made:** Added opening explaining what GBPD is and how it differs from GBCD (2D plane normals vs full 5D). Added MRD interpretation (1.0 = random, >1.0 = preferred).

- [x] **ComputeShapesTriangleGeomFilter** (OrientationAnalysis)
  - **Clarity:** Very technical but thorough. Excellent caveats section and validation table
  - **Figures Needed:** Tetrahedron construction from mesh; watertight vs non-watertight mesh
  - **Real-World Viz:** Comparison of voxelized vs triangle geometry shape results
  - **Concept Links:** triangle geometry, mesh watertightness, Euler characteristic, winding
  - **Changes Made:** Consolidated caveats into "Differences from Image Geometry Version" section. Merged watertight warning and Euler characteristic into "Mesh Quality Requirements" section. Cross-referenced Image Geometry version for output descriptions.

### Already Complete (Prior to Triage)

| Filter | Plugin | Status |
|--------|--------|--------|
| ComputeKernelAvgMisorientations | OrientationAnalysis | Done |

- [x] **ComputeKernelAvgMisorientationsFilter** (OrientationAnalysis)
  - **Changes Made:** Full rewrite with kernel radius explanation, 6 figures (formula, 1D radius 1 & 2, 2D radius (1,1,0) & (1,2,0), quick reference table). Serves as template for rewrite style.

---

## Batch 4: Neighbor / Kernel Operations

**Plugin:** SimplnxCore + OrientationAnalysis
**Filters:** 11 (cell-level cleanup, morphology, neighbor-list utilities)

### Tier 1 — Critical

| Filter | Plugin | Status |
|--------|--------|--------|
| AddBadData | SimplnxCore | Done |

- [x] **AddBadDataFilter** (SimplnxCore)
  - **Clarity:** Single dense paragraph with one run-on sentence; the filter's purpose (introduce realism into synthetic structures) is buried.
  - **Completeness:** No example imagery, no parameter guidance. Mentions "Manhattan distances" with no context. Requires the reader to know about the *Convert Attribute Data Type* filter as a prerequisite. Group is "Synthetic Building (Misc)" which is an outlier vs. the other Batch 4 cleanup filters but is correct given the filter's purpose.
  - **Accessibility:** Volume-fraction semantics are easy to misread (the *0.2* applies only to the affected cells, not the whole volume — a subtle but important point).
  - **Figures Needed:** Diagram showing the two noise modes (random voxels scattered through volume vs. boundary voxels along feature surfaces). Visual of how a 0.2 volume fraction translates to actual changed-cell counts.
  - **Real-World Viz:** Before/after of a synthetic microstructure with random noise applied; same with boundary noise applied.
  - **Concept Links:** Manhattan distance, feature boundary cells, synthetic microstructure realism
  - **Changes Made:** Added "Why Use This Filter?" section motivating the experimental-realism use case (matched cleanup pipelines for apples-to-apples comparison). Added "Random vs. Boundary Noise" section explaining the two independent modes. Added "Volume Fraction Semantics" section clarifying the per-eligible-cell meaning. Added "What 'Bad' Means in Output" section. Added explicit two-step prerequisite recipe (Compute Euclidean Distance Map → Convert Data Type). Added Required Input Sources.

### Tier 2 — Important

| Filter | Plugin | Status |
|--------|--------|--------|
| FillBadData | SimplnxCore | Done |
| ErodeDilateBadData | SimplnxCore | Done |
| ErodeDilateMask | SimplnxCore | Done |
| ErodeDilateCoordinationNumber | SimplnxCore | Done |
| ReplaceElementAttributesWithNeighborValues | SimplnxCore | Done |
| NeighborOrientationCorrelation | OrientationAnalysis | Done |
| BadDataNeighborOrientationCheck | OrientationAnalysis | Done |

- [x] **FillBadDataFilter** (SimplnxCore)
  - **Changes Made:** Rewrote lead-in to distinguish "small noise (filled)" vs "large defects (preserved)". Restructured implementation-detail Phase 1-4 into a concise "How This Filter Works" 3-step summary. Added "Minimum Defect Size Units" section stating cells/integer voxel count with conversion formula and typical ranges (5-50 for noise, 500-5000 for preserving real pores). Promoted "Store Defects as New Phase" option to its own section. Trimmed performance discussion to a brief note. Added Required Input Sources.

- [x] **ErodeDilateBadDataFilter** (SimplnxCore)
  - **Changes Made:** Removed stray backtick. Restructured into Dilation/Erosion/When-to-Use/Iterations-and-Direction sections. Named the morphological *opening* (erode-then-dilate) and *closing* (dilate-then-erode) patterns explicitly. Stated iterations are in **cell-layers**. Documented the X/Y/Z directional toggle for anisotropic morphology. Added Required Input Sources.

- [x] **ErodeDilateMaskFilter** (SimplnxCore)
  - **Changes Made:** Defined "mask" inline as a boolean cell-level array with cross-link to Multi-Threshold Objects. Added "When to Use" with concrete examples (erode = discard unreliable boundary cells; dilate = recover over-thresholded valid cells). Stated iterations are in **cell-layers**. Documented X/Y/Z directional toggle motivation (anisotropic serial-section resolution). Added Required Input Sources.

- [x] **ErodeDilateCoordinationNumberFilter** (SimplnxCore)
  - **Changes Made:** Stated coordination-number range is **0 to 6** with worked examples (CN=0 inside, CN=6 isolated voxel, CN=4-5 thin protrusion). Cross-linked "isotropic coarsening" definition to RequireMinimumSizeFeatures. Promoted "Loop Until Gone" to its own section with single-pass-vs-full-smoothing guidance. Added "When to Use This Filter" section calling out salt-and-pepper cleanup. Added Required Input Sources.

- [x] **ReplaceElementAttributesWithNeighborValuesFilter** (SimplnxCore)
  - **Changes Made:** Added generic "How This Filter Works" section before EBSD vendor-specific examples. Promoted "Comparison Operator" to its own subsection with use-case guidance for higher-is-better (CI, IQ) vs lower-is-better (MAD, Error) scalars. Added "Loop Until Gone" subsection with caution about flood-fill behavior. Promoted the "too much replacement" warning to its own "Caution: Flood Fill Behavior" section. Reframed EBSD examples as "Example Use Cases". Added Required Input Sources.

- [x] **NeighborOrientationCorrelationFilter** (OrientationAnalysis)
  - **Changes Made:** Added lead-in distinguishing this filter from sister filter ReplaceElementAttributesWithNeighborValues. Restructured algorithm into 4 numbered steps. Promoted *Cleanup Level* to its own subsection with **1-6 range** stated explicitly and worked guidance for each value (6=conservative, 4-5=moderate, 2-3=aggressive, 1=flood-fill). Cross-linked sister filters and EBSD readers. Added Required Input Sources.

- [x] **BadDataNeighborOrientationCheckFilter** (OrientationAnalysis)
  - **Changes Made:** Verified the doc title matches humanName() ("Neighbor Orientation Comparison (Bad Data)"); kept as-is. Added lead-in distinguishing this filter (mask-only update) from NeighborOrientationCorrelation (full-attribute replacement) and recommending sequencing. Promoted *Required Number of Neighbors* to its own section with **1-6 range** stated explicitly. Added cross-references to Multi-Threshold Objects, EBSD readers, and the sister filter. Added Required Input Sources.

### Tier 3 — Polish

| Filter | Plugin | Status |
|--------|--------|--------|
| RequireMinNumNeighbors | SimplnxCore | Done |
| ComputeNeighborhoods | SimplnxCore | Done |
| ComputeNeighborListStatistics | SimplnxCore | Done |

- [x] **RequireMinNumNeighborsFilter** (SimplnxCore)
  - **Changes Made:** Rewrote lead with explicit motivation (isolated single-feature islands from segmentation). Cross-linked "isotropic coarsening" to the shared definition in RequireMinimumSizeFeatures. Stated threshold is in **count of contiguous neighbors**. Added inspection-recommendation (look at *Number of Neighbors* output before choosing). Added Required Input Sources.

- [x] **ComputeNeighborhoodsFilter** (SimplnxCore)
  - **Changes Made:** Added lead-in motivating the use case (clustering, second-nearest-neighbor analysis). Defined Equivalent Sphere Diameter inline with cross-link to Compute Feature Sizes. Stated *Multiplier* is **dimensionless**. Added typical-value guidance for the multiplier (1.0 immediate neighbors, 2.0-3.0 second-nearest, 5.0+ broader clustering). Added Required Input Sources.

- [x] **ComputeNeighborListStatisticsFilter** (SimplnxCore)
  - **Changes Made:** Added concrete motivating example (summarizing per-feature neighbor misorientations from ComputeFeatureNeighborMisorientations). Restructured statistics list with inline descriptions. Stated outputs inherit units from input. Added Required Input Sources listing the four common NeighborList producers across both plugins.

---

## Batch 5: Geometry Creation / Manipulation

**Plugin:** SimplnxCore
**Filters:** 15 (geometry creation, transformation, resampling, cropping/padding, mesh generation, partitioning)

### Tier 1 — Critical

None. No filter in this batch has a non-expert blocker.

### Tier 2 — Important

| Filter | Plugin | Status |
|--------|--------|--------|
| CreateGeometry | SimplnxCore | Done |
| ApplyTransformationToGeometry | SimplnxCore | Done |
| RotateSampleRefFrame | SimplnxCore | Done |
| ResampleImageGeom | SimplnxCore | Done |
| ResampleRectGridToImageGeom | SimplnxCore | Done |
| PadImageGeometry | SimplnxCore | Done |
| QuickSurfaceMesh | SimplnxCore | Done |
| InitializeImageGeomCellData | SimplnxCore | Done |

- [x] **CreateGeometryFilter** (SimplnxCore)
  - **Clarity:** Comprehensive but very long (~19KB); the 8-geometry-type taxonomy is buried in dense prose.
  - **Completeness:** Excellent — covers all 8 geometry types with examples, including a complete worked example for importing from text files. Possibly too much for a single filter doc.
  - **Accessibility:** Heavy DREAM3D-NX terminology assumed (Element, Attribute Matrix, shared vertex list).
  - **Figures Needed:** Side-by-side infographic showing all 8 geometry types (Image, Rectilinear Grid, Vertex, Edge, Triangle, Quadrilateral, Tetrahedral, Hexahedral) with their defining elements illustrated.
  - **Real-World Viz:** None additional needed; the text import workflow could use a quick diagram.
  - **Units Clarity:** Spacing units ("microns per pixel" example) called out — good.
  - **Concept Links:** Geometry types, Element type, shared vertex list, winding/right-hand rule, Attribute Matrix
  - **Notes:** "DREAM3D Review (Geometry)" group is unusual — verify this is intended. Typo "dimenionality" appears multiple times. Each geometry-type section duplicates content from per-type filters. Consider promoting the geometry-type taxonomy into a shared concept page and trimming this doc to filter-specific guidance.

- [x] **ApplyTransformationToGeometryFilter** (SimplnxCore)
  - **Clarity:** Strong before/after image sequences explaining the successive-rotation artifact problem. Excellent caveat.
  - **Completeness:** Has duplicate transformation-type listing (bullets at lines 96-103 and the same content as a table at lines 107-114). One needs to go.
  - **Accessibility:** Node vs Image distinction is critical and well-explained; row-major matrix encoding explained.
  - **Figures Needed:** Already has many. Could add a diagram explaining the four-element axis-angle representation.
  - **Real-World Viz:** Already excellent.
  - **Units Clarity:** Rotation angle units (degrees) explicit; translation in geometry's coordinate units; scale dimensionless.
  - **Concept Links:** transformation matrix, axis-angle, row-major matrix order, interpolation modes
  - **Notes:** Remove the duplicated transformation-type listing. Convert "[Combine Transformation Matrices](CombineTransformationMatricesFilter.md)" link to a proper MyST link (already is). Add Required Input Sources. The relative `./Filter.md` should be checked — current uses both `./` and bare forms.

- [x] **RotateSampleRefFrameFilter** (SimplnxCore)
  - **Clarity:** Verified-only-for-axis-aligned-90/180 warning is at the top — good. Distinction from ApplyTransformation (sample reference frame vs geometric rotation) is not made explicit.
  - **Completeness:** Has rotation matrix equivalent shown. Note about origin shift after rotation is valuable.
  - **Accessibility:** "Sample reference frame" jargon assumed; the term needs first-use definition.
  - **Figures Needed:** Diagram showing sample frame rotation (the axes of the sample relabeled) vs geometric rotation (the data physically rotated).
  - **Real-World Viz:** Already has EBSD rotation example.
  - **Units Clarity:** Rotation angle in degrees; axis is dimensionless unit vector.
  - **Concept Links:** sample reference frame, EBSD frame convention, axis-angle rotation
  - **Notes:** Add explicit "When to Use vs ApplyTransformation" guidance. Make verified-axis-only banner more prominent. Cross-link to SetImageGeomOriginScaling for the origin reset workflow. Add Required Input Sources.

- [x] **ResampleImageGeomFilter** (SimplnxCore)
  - **Clarity:** Three resampling modes (Spacing, Scaling, Exact Dimensions) each with worked numerical examples. Clear.
  - **Completeness:** "No interpolation, closest cell wins" behavior noted. Renumber Features + NeighborList removal warning present.
  - **Accessibility:** Spacing/dimensions/scaling distinction depends on understanding Image Geometry.
  - **Figures Needed:** Before/after voxel-grid diagram showing how a 2x spacing reduction halves the cell count.
  - **Real-World Viz:** Cubic Small IN100 sliced at different resolutions side-by-side.
  - **Units Clarity:** Spacing values are in **physical units** (same as input geometry's spacing); scaling is **percent**; exact dimensions are in **cells**.
  - **Concept Links:** spacing vs scaling vs dimensions, closest-cell resampling, Feature renumbering
  - **Notes:** Cross-link to ApplyTransformation is present but uses `./` path; convert to plain MyST form. Add explicit units callouts per mode. Add Required Input Sources.

- [x] **ResampleRectGridToImageGeomFilter** (SimplnxCore)
  - **Clarity:** Brief, just describes the "last one wins" rule.
  - **Completeness:** Missing: when to use this, how to specify the target image geometry's spacing/dimensions, what happens when target is finer than source.
  - **Accessibility:** Rectilinear Grid concept not introduced.
  - **Figures Needed:** Side-by-side diagram of variable-spacing RectGrid → regular Image Geom with the "last one wins" cells highlighted.
  - **Real-World Viz:** A real RectGrid resampled to a uniform Image Geom.
  - **Units Clarity:** Target spacing in physical units.
  - **Concept Links:** Rectilinear Grid vs Image Geometry, downsampling vs upsampling
  - **Notes:** Add "When to Use" section. Explain why "last one wins" was chosen (interpolation is wrong for label data). Add Required Input Sources.

- [x] **PadImageGeometryFilter** (SimplnxCore)
  - **Clarity:** Very brief; the figures are helpful but the prose underdocuments the "default padding value" semantics.
  - **Completeness:** Doesn't explain what the *Update Origin* option does or when to use it.
  - **Accessibility:** "Default padding value" is undefined.
  - **Figures Needed:** Already has before/after. Could add a 3-panel diagram showing Update Origin ON vs OFF.
  - **Real-World Viz:** Before/after with both Update Origin settings.
  - **Units Clarity:** Pad amounts are in **cells/voxels**. Default value is in whatever units the target array uses.
  - **Concept Links:** padding, image extension, origin shift
  - **Notes:** Expand "default padding value" to "Each padded cell is initialized to the user-specified Default Value, which is interpreted in the same units as each cell-level array being padded". Document *Update Origin* and *Update Spacing* options. Add Required Input Sources.

- [x] **QuickSurfaceMeshFilter** (SimplnxCore)
  - **Clarity:** Deprecation notice at top recommending "Surface Nets". Node Types table is excellent. Triangle-pair-per-cell-face algorithm explained.
  - **Completeness:** Has good images for each Node Type. The Verify Triangle Winding reference is implicit; needs a link.
  - **Accessibility:** "Stair stepped" surface mesh result not pictured directly.
  - **Figures Needed:** Already has plenty. Could use a small diagram of how a single voxel face becomes 2 triangles.
  - **Real-World Viz:** Already present.
  - **Concept Links:** voxel-face-to-triangle conversion, Node Types, FaceLabels convention, mesh windings
  - **Notes:** Improve deprecation banner — make clear when to use this vs Surface Nets. Convert "see Verify Triangle Winding documentation" to a MyST link. Add Required Input Sources (FeatureIds from a segment filter).

- [x] **InitializeImageGeomCellDataFilter** (SimplnxCore)
  - **Clarity:** Three modes (Manual, Random, Random With Range) are listed but the "subvolume" parameter is undefined.
  - **Completeness:** Missing: how is the subvolume specified (min/max cell indices? physical bounds?). What data types support each random mode? What about boolean arrays?
  - **Accessibility:** Reader has to guess the subvolume specification UI.
  - **Figures Needed:** Diagram showing a subvolume highlighted within a larger volume; before/after.
  - **Real-World Viz:** Before/after on a real cube showing the initialized subvolume.
  - **Units Clarity:** Subvolume bounds in **cells/voxels** vs **physical units** — must be stated.
  - **Concept Links:** subvolume, cell-level initialization, random number generation seed
  - **Notes:** Document the subvolume specification mode and units. Note random-mode behavior for boolean, integer, and float types separately. Document the random seed parameter. Add Required Input Sources.

### Tier 3 — Polish

| Filter | Plugin | Status |
|--------|--------|--------|
| CreateImageGeometry | SimplnxCore | Done |
| CombineTransformationMatrices | SimplnxCore | Done |
| SetImageGeomOriginScaling | SimplnxCore | Done |
| CropImageGeometry | SimplnxCore | Done |
| AppendImageGeometry | SimplnxCore | Done |
| PartitionGeometry | SimplnxCore | Done |
| ComputeCoordinatesImageGeom | SimplnxCore | Done |

- [x] **CreateImageGeometryFilter** (SimplnxCore)
  - **Clarity:** Deprecation banner is right under the title but as a level-2 heading, making it look like its own section. Confusing.
  - **Completeness:** Dimensions/Spacing/Origin definitions are good.
  - **Accessibility:** Same dense terminology as CreateGeometry.
  - **Figures Needed:** Dimensions/origin/spacing diagram (would also serve SetImageGeomOriginScaling and ResampleImageGeom).
  - **Real-World Viz:** N/A — filter creates rather than transforms data.
  - **Units Clarity:** Already states spacing example "microns per pixel".
  - **Concept Links:** Image Geometry dimensions/origin/spacing
  - **Notes:** Move deprecation notice into a clear callout. Fix "dimenionality" typo. Cross-link to CreateGeometry filter. Add Required Input Sources note (none — geometry is created from user parameters).

- [x] **CombineTransformationMatricesFilter** (SimplnxCore)
  - **Clarity:** Brief and adequate. Output format documented.
  - **Completeness:** Missing motivating context for why you'd combine matrices (the answer is in ApplyTransformation — referenced indirectly there).
  - **Accessibility:** Assumes user knows row-major 4x4 convention.
  - **Figures Needed:** Optional — a diagram showing two 4x4 matrices multiplying into a single 4x4 result.
  - **Real-World Viz:** N/A.
  - **Units Clarity:** N/A (matrix is dimensionless apart from translation components).
  - **Concept Links:** matrix composition, transformation order, row-major matrix order
  - **Notes:** Add a short "Why Use This Filter" pointing to ApplyTransformationToGeometry's caveat about successive image-geometry transformations. Note that matrices are applied left-to-right in the order listed. Add Required Input Sources.

- [x] **SetImageGeomOriginScalingFilter** (SimplnxCore)
  - **Clarity:** Title says "Set Origin & Spacing" but body only describes the origin parameters. Spacing is implicit.
  - **Completeness:** "Put Input Origin at the Center of Geometry" option mentioned but not explained in detail.
  - **Accessibility:** OK for the limited content provided.
  - **Figures Needed:** Diagram showing origin-at-corner vs origin-at-center.
  - **Real-World Viz:** N/A.
  - **Units Clarity:** Origin coordinates and spacing are in physical units.
  - **Concept Links:** Image Geometry origin, spacing
  - **Notes:** Document both Origin and Spacing parameter sets. Document the linkable parameters (only-change-origin, only-change-spacing options). Expand the "Put Origin at Center" explanation. Add Required Input Sources.

- [x] **CropImageGeometryFilter** (SimplnxCore)
  - **Clarity:** Three worked examples with images. Inclusive-bounds note is critical and called out.
  - **Completeness:** Voxels-vs-physical-coordinates mode is implicit; the "Use Physical Bounds" parameter should be stated explicitly.
  - **Accessibility:** Voxel index 0-based convention is implied via examples.
  - **Figures Needed:** Already adequate.
  - **Real-World Viz:** Already present.
  - **Units Clarity:** Bounds are in **cells (0-based, inclusive)** OR **physical units (depending on parameter)** — explicit per-mode.
  - **Concept Links:** ROI cropping, inclusive bounds, voxel indexing
  - **Notes:** Add explicit "Bounds Mode" subsection naming the parameter that toggles voxels vs physical. State units per mode. Update NeighborList warning to use the standard pattern shared with RequireMinimumSizeFeatures.

- [x] **AppendImageGeometryFilter** (SimplnxCore)
  - **Clarity:** Exhaustive examples for X/Y/Z directions with figures.
  - **Completeness:** "Resolution" terminology used in one place — should be "Spacing" per the modern simplnx convention.
  - **Accessibility:** Good.
  - **Figures Needed:** Already comprehensive.
  - **Real-World Viz:** Already present.
  - **Units Clarity:** Append happens in cell-count units along the chosen direction; the *Check Spacing* option checks physical-unit match.
  - **Concept Links:** geometry concatenation, mirroring, spacing check
  - **Notes:** Replace remaining "Resolution" with "Spacing" for consistency. Cross-link to CreateImageGeometry for the geometry-definition concepts. Add Required Input Sources.

- [x] **PartitionGeometryFilter** (SimplnxCore)
  - **Clarity:** Four modes with excellent visual examples and walk-throughs.
  - **Completeness:** "Reconstruction (Reconstruction)" group/subgroup is a stutter — verify.
  - **Accessibility:** Long doc but well-organized with figures.
  - **Figures Needed:** Already excellent.
  - **Real-World Viz:** Already present.
  - **Units Clarity:** Cell Length is in physical units; Number of Cells Per Axis is in **integer cell counts**; Origin in physical units; Min/Max in physical units.
  - **Concept Links:** spatial partitioning, partition grid, out-of-bounds handling, vertex mask
  - **Notes:** Verify group "Reconstruction (Reconstruction)". Add a short lead-in explaining why partition (spatial analysis, sub-volume statistics, parallel processing prep). Add Required Input Sources.

- [x] **ComputeCoordinatesImageGeomFilter** (SimplnxCore)
  - **Clarity:** "Implicit vs explicit" framing is jargon; the example output is clear.
  - **Completeness:** Three output modes documented with raster scheme.
  - **Accessibility:** Reader benefits from concrete example, which is provided.
  - **Figures Needed:** Optional — a small voxel-grid diagram with one cell highlighted showing its (i,j,k) index and (x,y,z) physical position.
  - **Real-World Viz:** N/A — output is numerical.
  - **Units Clarity:** Physical coordinates in geometry's physical units; indices are **integer cell indices (0-based)**.
  - **Concept Links:** cell indexing scheme, physical coordinates, ijk vs xyz, raster order
  - **Notes:** Rewrite "implicit/explicit" framing in plain language ("makes the per-cell coordinates available as a regular cell-level array"). Add a "When to Use" — typically for CSV/text export workflows. Add Required Input Sources (Image Geometry).

---

## Batch 6: Data Manipulation

**Plugin:** SimplnxCore
**Filters:** 21 (create / copy / delete / rename / move / combine / split / convert / initialize / calculate)

### Tier 1 — Critical

None. No filter in this batch is incomprehensible to a non-expert.

### Tier 2 — Important

| Filter | Plugin | Status |
|--------|--------|--------|
| CreateAttributeMatrix | SimplnxCore | Done |
| CreateDataGroup | SimplnxCore | Done |
| DeleteData | SimplnxCore | Done |
| RenameDataObject | SimplnxCore | Done |
| MoveData | SimplnxCore | Done |
| CopyFeatureArrayToElementArray | SimplnxCore | Done |
| CreateFeatureArrayFromElementArray | SimplnxCore | Done |
| ConditionalSetValue | SimplnxCore | Done |
| ConvertColorToGrayScale | SimplnxCore | Done |
| ReshapeDataArray | SimplnxCore | Done |

- [ ] **CreateAttributeMatrixFilter** (SimplnxCore)
  - **Clarity:** Extremely brief (only ~20 lines). Lone example does not explain what an Attribute Matrix conceptually represents.
  - **Completeness:** Missing the *why* (tuple-dimensions discipline for arrays in the same matrix), the *when* (typical use cases — creating a Feature Attribute Matrix, an Ensemble Attribute Matrix), and the relationship to DataGroup.
  - **Accessibility:** Assumes the user already knows what an Attribute Matrix is.
  - **Figures Needed:** Optional — a hierarchy diagram showing Image Geometry → Cell Attribute Matrix → arrays would also serve CreateGeometry/CreateDataGroup.
  - **Concept Links:** Attribute Matrix, tuple dimensions, DataGroup vs Attribute Matrix
  - **Notes:** Add "What is an Attribute Matrix?" subsection. Cross-link to CreateDataGroup with explicit when-to-use-which guidance. Note tuple-dimension consistency requirement.

- [ ] **CreateDataGroupFilter** (SimplnxCore)
  - **Clarity:** Only 1 paragraph; says "unlike AttributeMatrix, DataGroups are capable of holding any DataObject of any size" but doesn't explain when to choose one over the other.
  - **Completeness:** Missing typical use case (organizing related arrays of different sizes, creating an output structure for a future filter).
  - **Concept Links:** DataGroup, AttributeMatrix
  - **Notes:** Expand. Cross-reference CreateAttributeMatrix and explain the choice: DataGroup for heterogeneous content, Attribute Matrix when all child arrays must share tuple dimensions.

- [ ] **DeleteDataFilter** (SimplnxCore)
  - **Clarity:** Two use cases (memory and name collisions) clearly explained.
  - **Completeness:** Missing: cascade behavior (does deleting a Geometry also delete its Attribute Matrices? Does deleting an Attribute Matrix delete its child arrays?), what happens to downstream filters that reference the deleted object.
  - **Concept Links:** cascade delete, object lifetimes
  - **Notes:** Document the cascade behavior. Add explicit warning: subsequent filters that selected the deleted object will fail preflight.

- [ ] **RenameDataObjectFilter** (SimplnxCore)
  - **Clarity:** Single sentence.
  - **Completeness:** Missing: what happens to filter parameters downstream that reference the old name (they won't auto-update — preflight will fail). What name collisions are allowed/disallowed.
  - **Notes:** Expand to one paragraph. Warn about downstream filter parameter references not updating automatically.

- [ ] **MoveDataFilter** (SimplnxCore)
  - **Clarity:** Brief but covers the tuple-count requirement.
  - **Completeness:** Missing: examples (moving a computed array into a Feature Attribute Matrix; moving an array between DataGroups). What happens to arrays that are children of the moved object.
  - **Concept Links:** parent-child hierarchy, tuple validation
  - **Notes:** Add concrete examples. Explain that tuple-dimension *shape* doesn't need to match — only the *number of tuples*.

- [ ] **CopyFeatureArrayToElementArrayFilter** (SimplnxCore)
  - **Clarity:** "Xmdf visualization files write only the Element attributes" is jargon and almost the entire rationale.
  - **Completeness:** Missing: general "when to use" (broadcasting a per-Feature scalar to every cell of that feature for visualization), parameter description.
  - **Notes:** Lead with the general purpose (broadcast Feature-level value back to all cells of the Feature). Explain the Xmdf-export reason as one specific use case rather than the only one. Add Required Input Sources.

- [ ] **CreateFeatureArrayFromElementArrayFilter** (SimplnxCore)
  - **Clarity:** "the value of the *last element copied*" is buried as the central footgun.
  - **Completeness:** Missing: warning that this filter is destructive when used on per-cell scalars that vary within a feature (since most cells get discarded). When to use vs. ComputeArrayStatistics (which computes per-feature means).
  - **Notes:** Promote the "last element copied" caveat to a Warning section. Cross-reference ComputeArrayStatistics for averaging behavior. Add Required Input Sources.

- [ ] **ConditionalSetValueFilter** (SimplnxCore)
  - **Clarity:** Two modes (conditional mask vs value-replacement) buried in one dense sentence.
  - **Completeness:** Mode parameter (*Use Conditional Mask*) is the central control but not promoted.
  - **Notes:** Split the two modes into their own subsections. Document *Use Conditional Mask* explicitly. Add Required Input Sources.

- [ ] **ConvertColorToGrayScaleFilter** (SimplnxCore)
  - **Clarity:** Four conversion algorithms documented with formulas.
  - **Completeness:** Missing: when to use (preprocessing color images for downstream grayscale-only filters), expected input format (uint8 RGB/RGBA), output type.
  - **Notes:** Add "When to Use" lead-in. State input/output array types and component shapes. Add Required Input Sources (ITK image reader).

- [ ] **ReshapeDataArrayFilter** (SimplnxCore)
  - **Clarity:** Critical footgun warning ("DOES NOT MOVE ANY VALUES IN MEMORY") is buried at line 11.
  - **Completeness:** Stride-mismatch example is good but the warning needs more prominence given that misuse silently produces wrong results.
  - **Concept Links:** strides, row-major / C-order storage, tuple dimensions
  - **Notes:** Promote the "no memory rearrangement" warning into a top-level Warning section. Add a "When NOT to Use" callout. Add Required Input Sources.

### Tier 3 — Polish

| Filter | Plugin | Status |
|--------|--------|--------|
| CreateDataArray | SimplnxCore | Done |
| CreateDataArrayAdvanced | SimplnxCore | Done |
| CopyDataObject | SimplnxCore | Done |
| CombineAttributeArrays | SimplnxCore | Done |
| ConcatenateDataArrays | SimplnxCore | Done |
| SplitDataArrayByComponent | SimplnxCore | Done |
| SplitDataArrayByTuple | SimplnxCore | Done |
| ExtractComponentAsArray | SimplnxCore | Done |
| ConvertData | SimplnxCore | Done |
| ArrayCalculator | SimplnxCore | Done |
| InitializeData | SimplnxCore | Done |

- [ ] **CreateDataArrayFilter** (SimplnxCore)
  - **Notes:** Already has good data-type ranges table and semicolon-notation example. Minor polish: state that Number of Components must be ≥ 1; explicitly note that this filter creates a *single component dimension* (use CreateDataArrayAdvanced for multi-dimension components). Add Required Input Sources (none).

- [ ] **CreateDataArrayAdvancedFilter** (SimplnxCore)
  - **Notes:** Documents 4 initialization modes (Fill, Incremental, Random, Random With Range) and Step Operation. Mostly good. Polish: state component-dimension product must be ≥ 1; clarify that multi-dimensional components (e.g., 3x3 tensor) are supported here. Cross-link CreateDataArray as the simpler version.

- [ ] **CopyDataObjectFilter** (SimplnxCore)
  - **Notes:** Clear; "deep copy" semantics for BaseGroup containers documented. Polish: cross-link Copy to New Parent option more explicitly; add Required Input Sources (none).

- [ ] **CombineAttributeArraysFilter** (SimplnxCore)
  - **Notes:** Already has worked examples and figures. Polish: cross-link "Concatenate Data Arrays" already present; mention the use case of building a Vertex coordinates array from three separate x/y/z arrays. Add Required Input Sources.

- [ ] **ConcatenateDataArraysFilter** (SimplnxCore)
  - **Notes:** Brief but clear. Cross-link to Combine Attribute Arrays is present. Polish: state that the result is always 1-D (already does); add example.

- [ ] **SplitDataArrayByComponentFilter** (SimplnxCore)
  - **Notes:** Clear worked example. Polish: minor copy-edit ("unput" → "input"); state the "specifying a subset of components" mode more cleanly; cross-link Combine Attribute Arrays as the inverse operation (already does).

- [ ] **SplitDataArrayByTupleFilter** (SimplnxCore)
  - **Notes:** Clear worked example. Polish: cross-link Split Data Array (By Component) present. Polish: state units on tuple counts (integer counts).

- [ ] **ExtractComponentAsArrayFilter** (SimplnxCore)
  - **Notes:** Title in cpp is "Extract/Remove Components" — verify match. Brief but covers the 3 operation modes. Polish: add a worked example showing the 3 modes; add Required Input Sources.

- [ ] **ConvertDataFilter** (SimplnxCore)
  - **Notes:** Already very thorough with up/down casting and signed/unsigned warnings. Polish: cross-link to Reshape Data Array for the related-but-different "interpret data differently" use case. Add Required Input Sources.

- [ ] **ArrayCalculatorFilter** (SimplnxCore)
  - **Notes:** Already extremely thorough — operator tables, multi-component handling, explicit array name escaping, multiple worked examples. Polish: minor only. Cross-link to Convert Angles to Degrees or Radians (already mentioned). Add Required Input Sources.

- [ ] **InitializeDataFilter** (SimplnxCore)
  - **Notes:** Content is good but the prose is rambly. Polish: restructure each initialization mode into its own subsection with a brief intro and bullet list of nuances rather than nested bullet trees. Move boolean entry rules to a single dedicated subsection.

---

## Batches 8-9: Not Yet Triaged

The following batches have been identified in the design spec but have not yet been triaged. Each batch will be triaged at the start of its work cycle. (Batch 7 was triaged 2026-06-10; see the Batch 7 section below.)

## Batch 2: Alignment Filters

**Plugin:** OrientationAnalysis + SimplnxCore
**Filters:** 5

### Tier 2 — Important

| Filter | Plugin | Status |
|--------|--------|--------|
| AlignSectionsMutualInformation | OrientationAnalysis | Done |
| AlignSectionsMisorientation | OrientationAnalysis | Done |
| AlignSectionsList | SimplnxCore | Done |

- [x] **AlignSectionsMutualInformationFilter** (OrientationAnalysis)
  - **Clarity:** Functional but domain-heavy; mutual information concept not explained for non-experts
  - **Figures Needed:** Diagram showing 7x7 grid search with high/low MI regions
  - **Real-World Viz:** Pipeline screenshot showing input/output shift arrays
  - **Concept Links:** mutual information, feature segmentation, 7x7 grid search, local minima
  - **Changes Made:** Added "What is Mutual Information?" section. Added "When to Use This Method" guidance. Restructured algorithm into clear numbered steps with named subsections. Explained misorientation tolerance parameter. Cross-referenced other alignment methods.

- [x] **AlignSectionsMisorientationFilter** (OrientationAnalysis)
  - **Clarity:** Clear algorithm steps but non-experts need misorientation definition; background subtraction underdocumented
  - **Figures Needed:** Diagram of 7x7 grid search with before/after alignment; background subtraction concept
  - **Real-World Viz:** Example output shifts for a real dataset
  - **Concept Links:** misorientation angle, cell-to-cell pairing, 7x7 grid search, local minima, background shift removal
  - **Changes Made:** Added "When to Use This Method" guidance with cross-references to alternatives. Restructured algorithm with clear step descriptions. Separated local minima warning, masking, and linear background subtraction into named subsections.

- [x] **AlignSectionsListFilter** (SimplnxCore)
  - **Clarity:** Core concept clear but overwhelmed by backwards compatibility cruft and file format specs
  - **Figures Needed:** Flowchart showing relative vs cumulative shift interpretation
  - **Real-World Viz:** Typical usage showing output from another alignment filter feeding into this one
  - **Concept Links:** relative shifts, cumulative shifts, slice ordering, CSV import
  - **Changes Made:** Rewrote lead paragraph to explain the three use cases. Clarified relative vs cumulative with concise definitions. Consolidated file import instructions into a single streamlined section. Moved legacy format details into a brief "Note on Legacy Files" section.

### Tier 3 — Polish

| Filter | Plugin | Status |
|--------|--------|--------|
| AlignSectionsFeatureCentroid | SimplnxCore | Done |

- [x] **AlignSectionsFeatureCentroidFilter** (SimplnxCore)
  - **Clarity:** Clear and accessible; centroid-based alignment is straightforward
  - **Figures Needed:** Diagram showing centroid calculation on adjacent slices before/after alignment
  - **Real-World Viz:** Before/after visualization of aligned sections
  - **Concept Links:** centroid, cell resolution, reference slice, background shift removal
  - **Changes Made:** Restructured into clear subsections (When to Use, How It Works, Reference Slice, Linear Background Subtraction). Added comparison note about local minima advantage over other methods.

### Tier 4 — Adequate

| Filter | Plugin | Status |
|--------|--------|--------|
| AlignGeometries | SimplnxCore | No Changes Needed |

- [x] **AlignGeometriesFilter** (SimplnxCore)
  - **Notes:** 32 lines. Two simple alignment methods (Origin and Centroid) clearly explained. No domain jargon. Adequate as-is.

---

## Batch 3: Segmentation / Feature Identification

**Plugin:** SimplnxCore + OrientationAnalysis
**Filters:** 10 (3 core segmentation + 7 feature identification/post-processing)

### Tier 1 — Critical

| Filter | Plugin | Status |
|--------|--------|--------|
| MergeTwins | OrientationAnalysis | Done |

- [x] **MergeTwinsFilter** (OrientationAnalysis)
  - **Clarity:** Very sparse (~27 lines). Cubic-High restriction stated in caps but no context on why twins are merged; Sigma-3 and `<111>` axis terminology used with no definitions.
  - **Completeness:** Only one paragraph of description; no output array description; no guidance on typical tolerance values; no before/after imagery.
  - **Accessibility:** Assumes reader knows what a twin is, what Sigma-3 notation means, and how to interpret `<111>` direction notation.
  - **Figures Needed:** Sigma-3 twin relationship diagram; visual explanation of axis tolerance vs. angle tolerance.
  - **Real-World Viz:** Before/after microstructure showing twin variants merged into the parent grain.
  - **Concept Links:** twin boundaries, Sigma-3, FCC, `<111>` axis, misorientation
  - **Changes Made:** Added "What is a Twin?" section explaining Sigma-3, FCC annealing twins, and CSL notation. Added "Why Merge Twins?" section giving the downstream-analysis motivation (grain counting, size distributions, neighbor stats). Restructured algorithm into numbered steps on the Feature level. Added "Parameter Guidance" with explicit degree units and typical tolerance ranges. Added "Required Input Sources" section listing each upstream filter that must run first. Added "Limitations" clarifying Cubic-High/m3m restriction, FCC-only practical applicability, and Sigma-3-only detection.

### Tier 2 — Important

| Filter | Plugin | Status |
|--------|--------|--------|
| ScalarSegmentFeatures | SimplnxCore | Done |
| CAxisSegmentFeatures | OrientationAnalysis | Done |
| EBSDSegmentFeatures | OrientationAnalysis | Done |
| RequireMinimumSizeFeatures | SimplnxCore | Done |
| ComputeFeatureNeighbors | SimplnxCore | Done |

- [x] **ScalarSegmentFeaturesFilter** (SimplnxCore)
  - **Clarity:** Burn algorithm steps clear, but term "burn algorithm" undefined; no introduction to what segmentation accomplishes conceptually.
  - **Completeness:** Neighbor scheme well documented with 4 paired figure sets. No guidance on tolerance values (tolerance is in whatever units the scalar array uses).
  - **Accessibility:** "Cells" and "Features" used without first-use definition; bold emphasis is stylistic rather than explanatory.
  - **Figures Needed:** Neighbor-scheme figures already excellent; could add a simple "what is segmentation" before/after conceptual diagram.
  - **Real-World Viz:** Before/after FeatureIds map from a scalar-based segmentation (e.g., image-quality thresholding).
  - **Units Clarity:** Tolerance has no explicit units — depends entirely on input array data type; this must be stated.
  - **Concept Links:** Feature IDs, burn algorithm, segmentation, Cell vs Feature data
  - **Changes Made:** Added lead-in describing the FeatureIds output and cross-referencing the two orientation-based segment filters. Added "What is Feature Segmentation?" conceptual section. Rewrote algorithm into a burn-algorithm step list. Added "Tolerance and Units" section with concrete examples for integer phase maps, 0-1 image quality, and 0-255 grayscale. Added "Mask Array" and "Periodic Option" sections. Added Required Input Sources with cross-plugin links.

- [x] **CAxisSegmentFeaturesFilter** (OrientationAnalysis)
  - **Clarity:** Brief C-axis definition provided, but assumes hexagonal-system knowledge; no guidance on when to use this vs EBSDSegmentFeatures.
  - **Completeness:** Shares structure and neighbor scheme figures with other segment filters. Missing: explicit hexagonal-only warning, phase handling, when-to-use guidance.
  - **Accessibility:** `<001>`, hexagonal system, C-axis all used without plain-language expansion.
  - **Figures Needed:** Reuse hexagonal C-axis figure from ComputeAvgCAxes (Batch 1); add a "C-axis misalignment between two neighboring cells" diagram.
  - **Real-World Viz:** Before/after segmentation on hexagonal (e.g., Ti) EBSD data.
  - **Units Clarity:** Tolerance is in **degrees** — must be stated explicitly in the parameter description.
  - **Concept Links:** C-axis, hexagonal materials, burn algorithm, reference frames
  - **Changes Made:** Added explicit hexagonal-only warning at the top with cross-references to alternative segment filters. Added "When to Use This Filter" explaining why C-axis alignment is the right criterion for hexagonal materials and when to use misorientation-based segmentation instead. Rewrote algorithm steps; added "Tolerance and Units" with degree units and typical values (1-3 / 5 / 10+). Added Required Input Sources linking ComputeAvgCAxes for C-axis concept.

- [x] **EBSDSegmentFeaturesFilter** (OrientationAnalysis)
  - **Clarity:** Generic term "misorientation" used without definition; core burn algorithm clear but domain context missing.
  - **Completeness:** No guidance on typical tolerance values (5° is the common industry default for grain segmentation); no mention of how multiple phases are handled.
  - **Accessibility:** Assumes EBSD workflow and misorientation knowledge.
  - **Figures Needed:** Misorientation angle diagram (shared with Batch 1 misorientation concept work); before/after segmentation example.
  - **Real-World Viz:** EBSD IPF map → segmented grains visualization — this is one of the highest-value workflow visualizations in DREAM3DNX.
  - **Units Clarity:** Tolerance is in **degrees** — must be stated explicitly.
  - **Concept Links:** misorientation, EBSD workflow, burn algorithm, reference frames, grain segmentation
  - **Changes Made:** Added "What is Misorientation-Based Segmentation?" section explaining grains and misorientation for non-experts. Rewrote algorithm into numbered burn steps including symmetry handling. Added "Typical Tolerance Values" section with 5° industry default, 2-3° for subgrains, 10-15° high-angle cutoff. Added "Phase Handling" explaining different-phase and phase-0 behavior. Added "Mask Array" and "Periodic Option" sections. Added Required Input Sources.

- [x] **RequireMinimumSizeFeaturesFilter** (SimplnxCore)
  - **Clarity:** Core action (remove features below size threshold) clear, but "isotropically coarsened" is unexplained jargon.
  - **Completeness:** Has good warnings (feature data invalidation, NeighborList removal); no guidance on typical minimum-size values.
  - **Accessibility:** "Ensemble" bolded but not defined; "isotropically coarsened" needs a plain-language explanation.
  - **Figures Needed:** Before/after showing small features removed and gaps filled; diagram of isotropic coarsening (neighbors growing outward uniformly).
  - **Real-World Viz:** Before/after microstructure with minimum-size filter applied.
  - **Units Clarity:** Minimum size is in **cells** (integer voxel count) — must be stated explicitly.
  - **Concept Links:** feature cleanup, isotropic coarsening, Ensemble, NeighborList invalidation
  - **Changes Made:** Added lead-in explaining typical use (discarding spurious single-cell grains after segmentation). Added "What is Isotropic Coarsening?" section with plain-language explanation. Added "Minimum Size Units" section stating units are **cells** (integer voxel count), with a formula for converting physical volume to cell count. Rewrote the "Single Phase" option more clearly. Kept existing warnings. Added Required Input Sources.

- [x] **ComputeFeatureNeighborsFilter** (SimplnxCore)
  - **Clarity:** Algorithm steps clear, but the multiple output arrays are described inline in running prose and hard to scan.
  - **Completeness:** Mentions several outputs (neighbor count, shared surface area, boundary-cell count, surface-feature flag) but all buried in a single paragraph.
  - **Accessibility:** Face-sharing neighbor concept not illustrated; implicit assumption that user knows what a NeighborList is.
  - **Figures Needed:** Diagram showing feature-to-feature shared boundary with shared surface area highlighted.
  - **Real-World Viz:** Grain map colored by number of neighbors; or shared-surface-area visualization.
  - **Units Clarity:** Shared surface area is in **cell-face units** (dimensionless count of shared faces, not physical area) — should be stated.
  - **Concept Links:** contiguous neighbors, Feature IDs, shared surface area, NeighborList
  - **Changes Made:** Rewrote lead-in describing why downstream filters depend on this (misorientation stats, GBCD, twin merging, boundary strengths). Separated algorithm steps from output descriptions. Added "What This Filter Produces" section with bulleted list of the three main outputs plus two optional outputs; explained each output's downstream use. Explicitly stated shared surface area units are **cell-face count**, not physical area. Added Required Input Sources.

### Tier 3 — Polish

| Filter | Plugin | Status |
|--------|--------|--------|
| IdentifySample | SimplnxCore | Done |
| ComputeSurfaceFeatures | SimplnxCore | Done |
| ComputeBiasedFeatures | SimplnxCore | Done |
| RemoveFlaggedFeatures | SimplnxCore | Done |

- [x] **IdentifySampleFilter** (SimplnxCore)
  - **Clarity:** Clear purpose (remove overscan border), steps well explained, slice-by-slice option well documented.
  - **Completeness:** Good example images showing when to use vs when not to use the filter.
  - **Accessibility:** FIB-SEM mentioned but not critical to understanding; typo: "thresheld" should be "thresholded".
  - **Figures Needed:** Already adequate; could add a before/after showing cleanup effect on a real overscanned dataset.
  - **Real-World Viz:** Already has good/bad-dataset examples.
  - **Concept Links:** thresholding, sample identification, contiguous regions
  - **Changes Made:** Fixed typo "thresheld" → "thresholded". Rewrote intro prose for readability (shorter sentences, active voice). Added explicit link to Multi-Threshold Objects as the typical mask source. Added Required Input Sources. Title preserved as it matches the filter's humanName().

- [x] **ComputeSurfaceFeaturesFilter** (SimplnxCore)
  - **Clarity:** Purpose clear; algorithm clear; two WARNING sections are helpful and well-placed.
  - **Completeness:** Broken/truncated sentence on line 9: "the Cells that sit at either ." — should be completed or deleted. Intro paragraph and the "This Filter determines..." paragraph are partially redundant.
  - **Accessibility:** Feature ID=0 convention referenced in a warning but could use an inline first-use definition.
  - **Figures Needed:** Already has good example output images.
  - **Real-World Viz:** Already present.
  - **Concept Links:** Feature ID=0, surface features, bounding box
  - **Changes Made:** Deleted broken truncated sentence. Consolidated the two redundant intro paragraphs into one. Added inline definition of Feature ID = 0 (the "unassigned / outside sample" convention) with cross-link to IdentifySample. Added cross-link to ComputeBiasedFeatures as the more rigorous approach to boundary bias. Added Required Input Sources.

- [x] **ComputeBiasedFeaturesFilter** (SimplnxCore)
  - **Clarity:** Algorithm and rationale well explained; before/after example figure is strong.
  - **Completeness:** Good — explains why larger features are more likely to be biased and why bounding-box logic works.
  - **Accessibility:** Most terms defined inline; credit to Dave Rowenhorst retained.
  - **Figures Needed:** Already adequate.
  - **Real-World Viz:** Already present.
  - **Concept Links:** stereology, feature centroids, bounding-box bias, unbiased statistics
  - **Changes Made:** Added "Why Bias Matters for Statistics" section explaining the size-dependent sampling bias — why excluding only surface-touching features still leaves bias, and why centroid-based bounding-box logic fixes it. Cross-linked ComputeSurfaceFeatures. Added Required Input Sources.

- [x] **RemoveFlaggedFeaturesFilter** (SimplnxCore)
  - **Clarity:** Three operations (Remove / Extract / Extract then Remove) well documented.
  - **Completeness:** NeighborList warning present; no Example Pipeline link populated.
  - **Accessibility:** "isotropically coarsened" is jargon (same issue as RequireMinimumSizeFeatures).
  - **Figures Needed:** Before/after showing each of the three operation modes; at least a side-by-side of Remove vs Extract.
  - **Real-World Viz:** Microstructure showing extraction vs removal outcomes.
  - **Concept Links:** feature cleanup, isotropic coarsening, feature extraction
  - **Changes Made:** Rewrote description to position this filter as the general-purpose flag-based removal tool and cross-reference RequireMinimumSizeFeatures for size-based use. Linked "isotropic coarsening" to the definition in RequireMinimumSizeFeatures rather than repeating. Cleaned up operation list phrasing. Converted NeighborList warning to sub-heading. Added Required Input Sources listing typical flag-producing filters (ComputeBiasedFeatures, ComputeSurfaceFeatures).

---

## Batch 7: I/O Filters (Read/Write)

**Plugin:** SimplnxCore + OrientationAnalysis
**Filters:** 44 (24 readers + 20 writers)

> **Scope note:** The design spec estimated ~15 I/O filters. The actual count is **44** once the full set of EBSD/HDF5 readers (Ang, Ctf, Channel5, H5Ebsd, H5Esprit, H5Oim, H5Oina, EnsembleInfo, GrainMapper3D) and the format-specific exporters (Abaqus, LAMMPS, SPParks, LosAlamos FFT, VTK, Avizo, INL, GBCD, StatsGen ODF, etc.) are included. Rewrites will be committed at each tier boundary, and the large Tier 2 group may be split into sub-commits by sub-theme (core data I/O, image/volume readers, simulation exporters, VTK/Avizo, EBSD readers, OrientationAnalysis writers).

### Tier 1 — Critical

| Filter | Plugin | Status |
|--------|--------|--------|
| ReadH5OinaData | OrientationAnalysis | Done |
| WriteGBCDGMTFile | OrientationAnalysis | Done |

- [x] **ReadH5OinaDataFilter** (OrientationAnalysis)
  - **Clarity:** Reads a single Oxford Aztec `.h5oina` file; vendor clearly identified. FORMAT VERSION 2.0 limitation and the Read HDF5 Dataset workaround are useful.
  - **Completeness:** STRUCTURAL DEFECT (fixed) — the doc had hand-written `## Parameters` and `## Created Objects` tables and was missing the auto-table marker entirely.
  - **Accessibility:** Same undefined EBSD jargon (Euler angles, crystal structures, reference frames) as the other EBSD readers.
  - **Figures Needed:** Has Figure 1 (Hexagonal alignment) + a UI overview; a sample/crystal reference-frame diagram would help.
  - **Real-World Viz:** IPF map of imported Aztec data.
  - **Units Clarity:** `.h5oina` angles stated as radians (good); Z Spacing units (microns) now live in the parameter help text (the auto-table).
  - **Concept Links:** Euler angles (Bunge Z-X-Z), crystal structures/Ensemble, lattice constants, sample/crystal reference frame, hexagonal symmetry, H5OINA v2.0 format
  - **Changes Made:** DELETED the hand-written `## Parameters` and `## Created Objects` tables and INSERTED the `% Auto generated parameter table` marker (so the real parameter table now injects). Added a "What This Filter Produces" prose paragraph explaining the imported EBSD data in plain language (orientation as Euler angles, per-pixel phase, pattern-quality metrics, per-phase Ensemble data) instead of a stale type table. Fixed the phi2 +30° bullet that wrongly referenced "(.ctf) files" → "`.h5oina` files". Reworded the historical reference-frame note from ".ctf file" to "Oxford data". Converted the two `{ref}` rotation links and the bold-only "Threshold Objects" to MyST links; fixed the Read HDF5 Dataset link to the cross-plugin `../SimplnxCore/` form. Added a "Downstream Processing" note cross-linking Convert Orientation Representation. Fixed typo "agment" → "augment". Normalized footer to "## DREAM3D-NX Help".

- [x] **WriteGBCDGMTFileFilter** (OrientationAnalysis)
  - **Clarity:** Was one sentence; never explained what a GBCD pole figure is, what the `.dat` contains, or why an engineer would want one.
  - **Completeness:** No parameter explanation beyond "phase index"; the Misorientation Axis-Angle parameter was undocumented. No Required Input Sources.
  - **Accessibility:** "GBCD", "GMT", "pole figure", "phase index" all undefined. GMT URL (`gmt.soest.hawaii.edu`) was stale.
  - **Figures Needed:** Data-flow diagram: GBCD data (from Compute GBCD) → this filter → `.dat` → GMT → rendered pole figure (shows the external toolchain).
  - **Real-World Viz:** Already has a rendered GMT pole figure image (caption now explains MRD contours).
  - **Units Clarity:** Phase index now stated as 1-based dimensionless index; misorientation angle stated in degrees, axis (h,k,l) dimensionless.
  - **Concept Links:** GBCD, GMT, pole figure, stereographic projection, Ensemble/phase data, MRD
  - **Changes Made:** Major rewrite. Added "What is a GBCD Pole Figure?" defining grain boundary, GBCD, misorientation, pole figure, and MRD in plain language. Added "What This Filter Does" making explicit that GMT is an external toolkit and this filter only writes its input `.dat` file (with the Compute GBCD → this filter → GMT toolchain spelled out). Added "Parameter Guidance" documenting Phase of Interest (1-based index), the previously-undocumented Misorientation Axis-Angle (angle in degrees + crystal axis h,k,l, with the Σ3 example), and Output GMT File. Added a Required Input Sources section naming [Compute GBCD](ComputeGBCDFilter.md) (GBCD) and the EBSD readers (Crystal Structures). Fixed the GMT URL to generic-mapping-tools.org. Improved the figure caption to explain the MRD contours.

### Tier 2 — Important

| Filter | Plugin | Status |
|--------|--------|--------|
| ReadHDF5Dataset | SimplnxCore | Done |
| WriteBinaryData | SimplnxCore | Done |
| ReadStlFile | SimplnxCore | Done |
| ReadVtkStructuredPoints | SimplnxCore | Done |
| ReadVolumeGraphicsFile | SimplnxCore | Done |
| WriteAbaqusHexahedron | SimplnxCore | Done |
| WriteLAMMPSFile | SimplnxCore | Done |
| WriteSPParksSites | SimplnxCore | Done |
| WriteLosAlamosFFT | SimplnxCore | Done |
| ReadDeformKeyFileV12 | SimplnxCore | Done |
| WriteVtkRectilinearGrid | SimplnxCore | Done |
| WriteVtkStructuredPoints | SimplnxCore | Done |
| WriteAvizoRectilinearCoordinate | SimplnxCore | Done |
| WriteAvizoUniformCoordinate | SimplnxCore | Done |
| ReadDREAM3D | SimplnxCore | Done |
| ReadEnsembleInfo | OrientationAnalysis | Done |
| ReadAngData | OrientationAnalysis | Done |
| ReadCtfData | OrientationAnalysis | Done |
| ReadChannel5Data | OrientationAnalysis | Done |
| ReadH5Ebsd | OrientationAnalysis | Done |
| ReadH5EspritData | OrientationAnalysis | Done |
| ReadH5OimData | OrientationAnalysis | Done |
| WriteGBCDTriangleData | OrientationAnalysis | Done |
| WritePoleFigure | OrientationAnalysis | Done |

- [x] **ReadHDF5DatasetFilter** (SimplnxCore)
  - **Clarity:** Element-count matching rule is explained, but the worked examples confuse more than clarify (Example 1 is an error case; Example 2's arithmetic is internally inconsistent).
  - **Completeness:** Missing description of key parameters (which dataset(s) to select, the file path, multi-dataset import in one pass). Only the dimension math is covered.
  - **Accessibility:** Assumes the reader knows HDF5 dataset vs group; "component/tuple dimensions", "attribute matrix" undefined.
  - **Figures Needed:** Diagram of a flat HDF5 dataset (N elements) reshaped into tuples × components.
  - **Real-World Viz:** Screenshot of the HDF5 tree dataset-selection UI.
  - **Units Clarity:** N/A (element counts, dimensionless).
  - **Concept Links:** HDF5 dataset/group, component dimensions, tuple dimensions, attribute matrix, total element count
  - **Changes Made:** Added a one-line definition of HDF5 and first-use definitions of tuple vs component dimensions. Described the dataset-selection behavior and the optional Attribute-Matrix parent placement in prose. Replaced the three inconsistent worked examples (including the '4-dims-labeled-3D' / mismatched-arithmetic case) with three internally consistent ones built on the rule total-element-count = tuples x components. Added Required Input Sources (None -- external .h5/.hdf5 file) and a License & Copyright section.

- [x] **WriteBinaryDataFilter** (SimplnxCore)
  - **Clarity:** Thin — one sentence plus an Endianess section. Does not state output file naming, extension, multi-component behavior, or that no header is written.
  - **Completeness:** Only Endianess documented. Missing output directory/naming, raw-no-header behavior, on-disk component ordering, file extension parameter.
  - **Accessibility:** The Endianess section itself is clear; the problem is omission.
  - **Figures Needed:** None.
  - **Real-World Viz:** Optional screenshot of the output files (low priority).
  - **Units Clarity:** N/A.
  - **Concept Links:** endianness, raw binary (no header), component ordering, output file naming
  - **Changes Made:** Expanded the thin description to the actual behavior (one raw file per **Data Array**, `<arrayName><ext>` naming with default `.bin`, interleaved components, no header written). Added the explicit warning that the user must record type/dims/endianness, and a cross-link to read the files back with [Read Raw Binary](ReadRawBinaryFilter.md). Italicized the endianness values; fixed the run-on/period grammar.

- [x] **ReadStlFileFilter** (SimplnxCore)
  - **Clarity:** Dives straight into binary layout/attribute-byte-count without first stating in plain terms that STL is a triangle surface mesh and the output is a Triangle Geometry.
  - **Completeness:** Covers binary format, vendor detection, strictness, a Python fix script. Never states ASCII STL is unsupported; no mention of produced arrays/normals.
  - **Accessibility:** Heading typo "## IMPORANT NOTES". Strictness warning repeated verbatim twice. Aimed at STL-format experts.
  - **Figures Needed:** Illustration of one triangle (normal + 3 vertices) and how triangles form a surface mesh.
  - **Real-World Viz:** Screenshot of an STL surface as a Triangle Geometry.
  - **Units Clarity:** STL is unitless — one sentence stating coordinates are in the file author's units (dimensionless to the reader).
  - **Concept Links:** Triangle Geometry, surface mesh, STL binary format, attribute byte count, face normals
  - **Changes Made:** Added a plain-language opening (an STL file is a list of triangles -> **Triangle Geometry**, with Face Normals/Labels). Fixed the 'IMPORANT NOTES' heading typo. Added an explicit binary-only / ASCII-rejected caveat (confirmed against preflight). Collapsed the duplicated attribute-byte-count warning; noted STL coordinates are unitless. Required Input Sources: None; fixed the Wikipedia anchor.

- [x] **ReadVtkStructuredPointsFilter** (SimplnxCore)
  - **Clarity:** Core idea clear (legacy VTK structured-points → Image Geometry) but POINT_DATA vs CELL_DATA and "eight values per voxel" will confuse non-experts without a figure. Uses legacy SIMPL "Data Container" wording.
  - **Completeness:** Explains attribute types, binary/ASCII, point vs cell data with an example. Missing: behavior when both POINT_DATA and CELL_DATA present (output naming/paths).
  - **Accessibility:** "STRUCTURED_POINTS", "POINT_DATA", "CELL_DATA", "LOOKUP_TABLE" only partly explained.
  - **Figures Needed:** A voxel cube showing cell-center value vs the 8 corner (point) values.
  - **Real-World Viz:** Screenshot of the example GrainIds dataset as an Image Geometry.
  - **Units Clarity:** State spacing/origin come from the file; length units file-defined.
  - **Concept Links:** Image Geometry, Cell Attribute Matrix, structured points, point vs cell data, voxel corners vs center, SCALARS/VECTORS
  - **Changes Made:** Replaced legacy 'Data Container' with **Image Geometry** throughout. Added a Point Data vs Cell Data subsection and described the two-output naming (`VTK Cell Data` / `VTK Point Data`) when both are present. Added the missing DREAM3D-NX Help footer and a License section, plus a Spacing/Origin/Units note. Required Input Sources: None.

- [x] **ReadVolumeGraphicsFileFilter** (SimplnxCore)
  - **Clarity:** Thin one-paragraph description; a non-expert won't know "Volume Graphics" data is a CT/VGStudio export.
  - **Completeness:** Underspecified — no output geometry type, volume data type, cropping support, or created array named. Only caveat is "both files in same directory".
  - **Accessibility:** "Volume Graphics data files" is a vendor term used without explanation.
  - **Figures Needed:** Optional small diagram of the `.vgi` (metadata) + `.vol` (raw voxels) pairing.
  - **Real-World Viz:** Screenshot of a CT volume from a `.vgi`/`.vol` pair.
  - **Units Clarity:** Says units come from the `.vgi`; expand to state which units and that spacing/origin derive from it.
  - **Concept Links:** Image Geometry, `.vgi`/`.vol` file pair, spacing/dimensions/units, CT volume
  - **Changes Made:** Normalized the group to IO (Input). Defined Volume Graphics data (.vgi metadata + .vol raw voxels; a CT/VGStudio export). Stated the output is an **Image Geometry** with a single float32 density Cell array and origin (0,0,0) (confirmed from preflight); noted no sub-volume extraction. Added an Example Pipelines heading and Required Input Sources: None.

- [x] **WriteAbaqusHexahedronFilter** (SimplnxCore)
  - **Clarity:** Non-expert won't know Abaqus is a commercial FEA package or why one exports a voxel grid to it. Jumps into `.inp` files without framing the Image-Geometry → hexahedral-mesh conversion.
  - **Completeness:** Five output files listed but their relationship (master `*Include`s the others) only implied. The `Write Dummy Node` parameter referenced at top but never explained.
  - **Accessibility:** "Abaqus", ".inp", "hexahedron", "C3D8", "Hourglass Stiffness", "stress-strain" undefined. The huge raw `_elset.inp` dump hurts readability.
  - **Figures Needed:** Diagram of a voxel grid mapping to a hexahedral mesh (8 nodes per voxel).
  - **Real-World Viz:** Screenshot of the exported mesh in Abaqus/CAE colored by grain.
  - **Units Clarity:** Node coordinates are physical units (matching spacing) but never stated; "Hourglass Stiffness 250" has no units/explanation.
  - **Concept Links:** Abaqus/FEA, hexahedral mesh, Image Geometry, FeatureIds, element sets (grains), nodes/elements
  - **Changes Made:** Defined Abaqus and FEA; framed the filter as converting an **Image Geometry** voxel grid into a C3D8 8-node hexahedral mesh. Documented the five `.inp` files and the master-file `*Include` relationship, the *Write Dummy Node* and *Hourglass Stiffness* parameters, and that node coordinates are in physical units. Trimmed the large `_elset.inp` dump; added Required Input Sources (FeatureIds + Image Geometry).

- [x] **WriteLAMMPSFileFilter** (SimplnxCore)
  - **Clarity:** Non-expert won't know LAMMPS is a molecular-dynamics code. Purpose stated but why/when unclear.
  - **Completeness:** No output-format example. "Example Pipelines" heading empty. Names "Insert Atoms" as producer (good, but informal).
  - **Accessibility:** "LAMMPS" never expanded. Legacy "Vertex Data Container" wording. Self-reference "Export LAMMPS Filter" is the wrong name.
  - **Figures Needed:** Optional before/after of features filled with atoms.
  - **Real-World Viz:** Screenshot of the resulting atom configuration (OVITO/VMD).
  - **Units Clarity:** Atom-coordinate units undocumented; state them.
  - **Concept Links:** LAMMPS/molecular dynamics, atomistic representation, Vertex Geometry, Insert Atoms
  - **Changes Made:** Expanded LAMMPS (Sandia molecular-dynamics code); replaced 'Vertex Data Container' with **Vertex Geometry**; fixed the 'Export LAMMPS Filter' self-reference. Added an output-format example and atom-coordinate units. Added Required Input Sources (Vertex Geometry, e.g. from Insert Atoms -- referenced in plain text since no Insert Atoms doc exists in the repo). Removed the empty Example Pipelines heading.

- [x] **WriteSPParksSitesFilter** (SimplnxCore)
  - **Clarity:** Terse and oddly worded ("LINE 4 evidently must be what is shown"); non-expert won't understand a "sites" file or site/value pairs.
  - **Completeness:** No Required Input Sources (needs FeatureIds on an Image Geometry). Line-by-line explanation partial (lines 1, 8 unexplained).
  - **Accessibility:** "SPPARKS" linked but not contextualized as a Kinetic Monte Carlo grain-growth simulator. "Kinetic Monte Carlo", "max neighbors", "xlo xhi" undefined. Header style `## Description ##` inconsistent.
  - **Figures Needed:** None.
  - **Real-World Viz:** None.
  - **Units Clarity:** Dimension lines (0 200) are cell-index extents (dimensionless cell counts) — clarify vs physical units.
  - **Concept Links:** SPPARKS/Kinetic Monte Carlo, sites, FeatureIds, Image Geometry, Cells
  - **Changes Made:** Defined SPPARKS (Sandia Kinetic Monte Carlo microstructure simulator); rewrote the tentative 'LINE 4 evidently' wording authoritatively. Explained site/value = cell/FeatureId, clarified the box-extent lines are dimensionless cell-index counts, and explained the blank separator lines. Normalized '## X ##' headings; added Required Input Sources (FeatureIds on an Image Geometry).

- [x] **WriteLosAlamosFFTFilter** (SimplnxCore)
  - **Clarity:** Output column layout (Phi1 Phi Phi2 X Y Z Feature_ID Phase_ID) explained well, but non-expert won't know what an "FFT 3D simulation code" is for or what Euler angles/phases are.
  - **Completeness:** Good format detail and a real reference. No Required Input Sources (needs Euler angles, FeatureIds, Phases on an Image Geometry). Empty "Example Pipelines" heading.
  - **Accessibility:** "Euler angles", "Phi1/Phi/Phi2", "phase", "FFT" undefined for a general reader.
  - **Figures Needed:** None.
  - **Real-World Viz:** None.
  - **Units Clarity:** Excellent — Euler angles in degrees, X/Y/Z integer indices, IDs from 1. Model for the batch; keep.
  - **Concept Links:** FFT/crystal plasticity, Euler angles, phases, FeatureIds, Image Geometry, CellData
  - **Changes Made:** Contextualized Euler angles and the Lebensohn FFT crystal-plasticity code; preserved the existing (model) units paragraph. Added Required Input Sources (Euler angles from an EBSD reader, FeatureIds, Phases) and removed the empty Example Pipelines heading.

- [x] **ReadDeformKeyFileV12Filter** (SimplnxCore)
  - **Clarity:** Two sentences. Non-expert won't know DEFORM is a metal-forming FEA package or what a "key file" is; can't tell what data results.
  - **Completeness:** Very thin. Lists variable names (stress, strain, ndtmp) without explanation. Does not describe the resulting DataStructure (a Quad Geometry plus cell/node attribute matrices).
  - **Accessibility:** "DEFORM v12", "key file", "ndtmp", "quadrilateral mesh", legacy "Data Container" all undefined.
  - **Figures Needed:** Optional screenshot of the imported quad mesh colored by a variable.
  - **Real-World Viz:** Render of the loaded DEFORM mesh colored by stress/strain.
  - **Units Clarity:** N/A (importer); note imported variable units are whatever DEFORM exported.
  - **Concept Links:** DEFORM/FEA, key file, Quad Geometry, nodes/cells, cell vs node variables
  - **Changes Made:** Defined DEFORM (metal-forming finite-element package) and a v12 key file. Described the created **Quad Geometry** plus node- and cell-level arrays (ndtmp = nodal temperature); replaced 'Data Container' with Quad Geometry. Added an example key-file snippet; Required Input Sources: None.

- [x] **WriteVtkRectilinearGridFilter** (SimplnxCore)
  - **Clarity:** Never says what a "VTK legacy file" or "RECTILINEAR_GRID dataset" is, nor why one wants this (ParaView/VisIt interop).
  - **Completeness:** Single sentence. No ASCII vs binary mention, vertex vs cell data, extension, or that input must be an Image Geometry.
  - **Accessibility:** "VTK legacy file", "RECTILINEAR_GRID" undefined.
  - **Figures Needed:** Optional shared diagram contrasting rectilinear-grid vs structured-points (variable vs uniform spacing).
  - **Real-World Viz:** Screenshot of the `.vtk` in ParaView.
  - **Units Clarity:** N/A in prose; confirm in param table.
  - **Concept Links:** VTK legacy format, ASCII vs binary VTK, RectilinearGrid vs Image Geometry, ParaView/VisIt
  - **Changes Made:** Expanded the one-sentence description; defined VTK legacy format and RECTILINEAR_GRID and named ParaView/VisIt. Explained the **Image Geometry** input is written as an explicit-coordinate rectilinear grid; documented ASCII vs binary. Added Required Input Sources (Image Geometry + cell arrays) and a cross-link to the structured-points variant.

- [x] **WriteVtkStructuredPointsFilter** (SimplnxCore)
  - **Clarity:** Assumes the reader knows STRUCTURED_POINTS. The one useful caveat ("only writes cell data") is present.
  - **Completeness:** Very thin. Doesn't explain how structured points differs from rectilinear grid, or when to pick this writer.
  - **Accessibility:** "VTK legacy file", "STRUCTURED_POINTS" undefined; indistinguishable from the rectilinear writer to a non-expert.
  - **Figures Needed:** Shared diagram (uniform structured points vs variable rectilinear spacing).
  - **Real-World Viz:** Screenshot in ParaView.
  - **Units Clarity:** N/A in prose.
  - **Concept Links:** VTK legacy format, structured points vs rectilinear grid, uniform spacing, ParaView/VisIt
  - **Changes Made:** Defined STRUCTURED_POINTS (uniform-spacing image data) with the same VTK/ParaView framing; kept the cell-data-only caveat. Added a 'when to use vs [Write Vtk Rectilinear Grid]' note and ASCII/binary documentation; added Required Input Sources.

- [x] **WriteAvizoRectilinearCoordinateFilter** (SimplnxCore)
  - **Clarity:** Confusing opening ("Values should be present from segmentation … cannot be determined by this filter") — vague about what "values" means (the FeatureIds array).
  - **Completeness:** Helpful raw AmiraMesh header example, but never says what Avizo is, what FeatureIds are, or that input is an Image Geometry + Int32 FeatureIds. Example contains garbled DateTime bytes.
  - **Accessibility:** "Avizo", "AmiraMesh", "Rectilinear Coordinate", "FeatureIds", "segmentation" undefined.
  - **Figures Needed:** None.
  - **Real-World Viz:** Screenshot of the output in Avizo.
  - **Units Clarity:** Example shows "Coordinates microns" — note the Avizo file hardcodes microns regardless of the geometry's actual units (caveat).
  - **Concept Links:** Avizo/AmiraMesh format, FeatureIds, segmentation, RectilinearGrid vs Image Geometry, units (microns)
  - **Changes Made:** Rewrote the confusing opening to state the **Image Geometry** + Int32 FeatureIds input plainly; defined Avizo and AmiraMesh. Cleaned the garbled DateTime bytes in the header example; added the hardcoded-microns caveat and ASCII/binary note; added a Group line, Required Input Sources, and a link to the Uniform variant.

- [x] **WriteAvizoUniformCoordinateFilter** (SimplnxCore)
  - **Clarity:** Same vague "Values should be present…" sentence; doesn't clarify FeatureIds input or how "Uniform" differs from "Rectilinear".
  - **Completeness:** Example header present but contains empty/garbled fields (blank DateTime, truncated Content/BoundingBox). Avizo/AmiraMesh undefined.
  - **Accessibility:** Same undefined jargon as the rectilinear variant.
  - **Figures Needed:** None.
  - **Real-World Viz:** Screenshot in Avizo.
  - **Units Clarity:** "Coordinates microns" hardcoded; caveat needed. BoundingBox units/order should tie to spacing/origin.
  - **Concept Links:** Avizo/AmiraMesh format, uniform vs rectilinear coordinates, FeatureIds, BoundingBox, units (microns)
  - **Changes Made:** Same clarifications as the Rectilinear variant (FeatureIds input, Avizo/AmiraMesh definitions, microns caveat, cleaned garbled example fields). Added the one-line contrast (single uniform spacing/bounding box vs per-axis coordinate arrays) and a link to the Rectilinear variant; added Required Input Sources.

- [x] **ReadDREAM3DFilter** (SimplnxCore)
  - **Clarity:** Core action clear (reads the DataStructure from a `.dream3d` HDF5 file), but omits the most important behavior: selective/partial import.
  - **Completeness:** Missing partial-import behavior, merge-into-current-DataStructure and name-collision behavior, pipeline-metadata read. Lacks the "Group (Subgroup)" header other docs carry. No Required Input Sources statement.
  - **Accessibility:** ".dream3d", "HDF5", "legacy" used without a one-line gloss.
  - **Figures Needed:** None.
  - **Real-World Viz:** Screenshot of the import-data tree selector showing partial-import checkboxes.
  - **Units Clarity:** N/A.
  - **Concept Links:** `.dream3d` file, HDF5, DataStructure import, partial import, legacy SIMPL vs NX format
  - **Changes Made:** Added the missing Group header (IO (Input)) and the key selective/partial-import behavior; explained imported objects merge into the current **DataStructure** at their paths (overwrite on collision) and that pipeline metadata may be read. Glossed .dream3d/HDF5/'legacy'; added Required Input Sources (None) and the standard footer.

- [x] **ReadEnsembleInfoFilter** (OrientationAnalysis)
  - **Clarity:** Mechanics (format, sections, keys) well laid out with tables and a worked example, but the *purpose* is buried in EBSD jargon.
  - **Completeness:** Strong on format/enum tables/phase-numbering caveat. Missing: what an Ensemble attribute matrix is, that this filter creates one, where the output goes; `.ini`/`.txt` are the same format.
  - **Accessibility:** Main blocker — Ensemble, Feature, Cell, Crystal Structure, Phase Type, Laue class all undefined.
  - **Figures Needed:** None (tables serve well).
  - **Real-World Viz:** None high-value.
  - **Units Clarity:** N/A (enumerated integer codes, well documented).
  - **Concept Links:** Ensemble Info, Ensemble/phase attribute matrix, Crystal Structure / Laue class, Phase Type, Feature vs Cell, ASCII `.ini`/`.txt`
  - **Changes Made:** Added a plain-language opening defining *ensemble* = phase and clarifying the filter reads a .ini/.txt file and writes an **Ensemble Attribute Matrix** into an existing Data Container. Added one-line glosses for Crystal Structure and Phase Type before the enum tables; documented downstream statistics consumers; removed the empty Example Pipelines heading.

- [x] **ReadAngDataFilter** (OrientationAnalysis)
  - **Clarity:** Reads a single EDAX/TSL `.ang` into an Image Geometry. Good, but never explains what the imported data MEANS (Euler angles, phases, CI, IQ named but undefined).
  - **Completeness:** Field-ordering block useful but raw jargon; does not say phi1/Phi/phi2 ARE the three Euler angles (Bunge). Mask/threshold guidance present.
  - **Accessibility:** Heavy undefined jargon (Euler angles, crystal/sample reference frame, Confidence Index, Image Quality, phase, ODF, IPF, hexagonal grid).
  - **Figures Needed:** Sample vs crystal reference-frame diagram.
  - **Real-World Viz:** IPF map of a freshly imported Small IN100 `.ang` slice.
  - **Units Clarity:** Weak — should state `.ang` angles are in radians; step size/origin in microns; reference-frame rotations in degrees.
  - **Concept Links:** Euler angles (Bunge), sample/crystal reference frame, confidence index, image quality, phases, IPF/ODF, hex-vs-square grid
  - **Changes Made:** Glossed phi1/Phi/phi2 as the three Euler angles; defined crystal/sample reference frames and Confidence Index/Image Quality. Fixed 'he following'; converted the Hex-Grid `{ref}` and the bold-only 'Threshold Objects' to MyST links; fixed the broken 'Ensemble Attribute Matrix' bold; added Downstream Processing and Required Input Sources.

- [x] **ReadCtfDataFilter** (OrientationAnalysis)
  - **Clarity:** Clearly reads an Oxford/HKL `.ctf` into an Image Geometry. Good vendor ID.
  - **Completeness:** Good on HKL default transforms, radians/degrees conversion, hexagonal phi2 +30°. Created-array meanings not explained.
  - **Accessibility:** Same undefined jargon; hexagonal section highly expert-oriented (flagged as edge case).
  - **Figures Needed:** Has Figure 1 (Hexagonal_Axis_Alignment); a reference-frame diagram would still help.
  - **Real-World Viz:** IPF map before/after the +30° phi2 correction, or correct-vs-wrong radians/degrees.
  - **Units Clarity:** Better than Ang (notes `.ctf` usually degrees, NX expects radians); still missing origin/step-size (microns).
  - **Concept Links:** Euler angles, sample/crystal reference frame, radians vs degrees, hexagonal symmetry/Bravais lattice, pole figure/IPF
  - **Changes Made:** Fixed the intro broken bold and the mangled Figure 1 caption; marked the hexagonal-alignment section Advanced. Converted `{ref}` and bold-only filter mentions to MyST links; added Downstream Processing (Convert Orientation Representation) and Required Input Sources.

- [x] **ReadChannel5DataFilter** (OrientationAnalysis)
  - **Clarity:** Reads an Oxford Channel 5 `.cpr`/`.crc` pair into an Image Geometry. Uses `ImageGeometry`/`ImageDataContainer` as code, inconsistent with the **Image Geometry** bolding used elsewhere.
  - **Completeness:** Mirrors the Ctf doc; good note that `.cpr`/`.crc` angles are radians and that Error=0 marks valid TRUE. Created-array meanings not explained.
  - **Accessibility:** Same undefined jargon; hexagonal section copied verbatim and wrongly says "(.ctf) files".
  - **Figures Needed:** Has Figure 1; reference-frame diagram still helpful.
  - **Real-World Viz:** IPF map of an imported Channel 5 dataset.
  - **Units Clarity:** Good on radians; missing origin/step-size (microns).
  - **Concept Links:** Euler angles, sample/crystal reference frame, hexagonal symmetry, pole figures, IPF, `.cpr`/`.crc` pair
  - **Changes Made:** Fixed the broken bold and caption; corrected the phi2 +30-degree bullet that wrongly referenced '.ctf' to Channel 5 (.cpr/.crc). Standardized code-style 'ImageGeometry' to **Image Geometry**; removed the empty Example Pipelines heading; converted links to MyST and added Required Input Sources.

- [x] **ReadH5EbsdFilter** (OrientationAnalysis)
  - **Clarity:** Good — states it reads a `.h5ebsd` built by *Import Orientation File(s) to H5EBSD*; the UI screenshot + checkbox/slice-subset/transform description is genuinely helpful. The one reader with a real upstream dependency.
  - **Completeness:** Strong on transforms, angle representation, slice subsetting, correct-vs-incorrect IPF example. Created-array meanings rely on auto-table (correct). Could note headless parameter equivalents for UI features.
  - **Accessibility:** Same domain jargon undefined, but the IF-steel correct/incorrect example is excellent.
  - **Figures Needed:** Already has UI + correct/incorrect + Hexagonal alignment figures; a reference-frame concept diagram would still help.
  - **Real-World Viz:** Already present (IF-steel IPF maps).
  - **Units Clarity:** Reference-frame rotations in degrees OK; lines 42/46 are missing the word "degrees" ("apparent 30 shifts", "add 30 to phi2").
  - **Concept Links:** H5Ebsd intermediate format, Euler angle representation (deg/rad), sample/crystal reference frame, slice stacking/subsetting, hexagonal symmetry, IPF
  - **Changes Made:** Added a Required Input Sources section stating the .h5ebsd must be built by [Import Orientation File(s) to H5EBSD](EbsdToH5EbsdFilter.md). Converted the three `{ref}` links to MyST; fixed the truncated '30' -> '30 degrees' (two places) and 'excellant' typo; marked the hexagonal section Advanced; added Downstream Processing.

- [x] **ReadH5EspritDataFilter** (OrientationAnalysis)
  - **Clarity:** Clearly reads a single Bruker Nano Esprit `.h5`; before/after UI screenshots for Z Spacing and scan selection help.
  - **Completeness:** Good — documents Z Spacing, multi-scan selection, X/Y Step=0 → 1.0 fallback, recommended rotation filters. "MAD" used without expansion.
  - **Accessibility:** Same undefined jargon; "MAD" (Mean Angular Deviation) not expanded.
  - **Figures Needed:** Has two UI screenshots; reference-frame diagram would help.
  - **Real-World Viz:** IPF map of an imported multi-scan Bruker volume (illustrate Z-stacking).
  - **Units Clarity:** Best of the `.h5` readers — Z Spacing "microns between each layer". Origin units not stated.
  - **Concept Links:** Euler angles, sample/crystal reference frame, scan/slice (Z) stacking, MAD/image quality, Z spacing (microns), multi-scan files
  - **Changes Made:** Expanded MAD -> Mean Angular Deviation on first use; converted all `{ref}` links to MyST; noted the zero-step fallback is 1.0 micron. Fixed the broken bold; added Downstream Processing and Required Input Sources; removed the empty Example Pipelines heading.

- [x] **ReadH5OimDataFilter** (OrientationAnalysis)
  - **Clarity:** Clearly reads an EDAX OIMAnalysis `.h5`. UI screenshots referenced for Z Spacing / scan selection.
  - **Completeness:** Good — multi-slice import, the valuable unique "Perform Slice By Slice Transform" caveat, thresholding. Created-array meanings rely on auto-table (correct).
  - **Accessibility:** Same undefined jargon; slice-by-slice transform note is clear and practical.
  - **Figures Needed:** Two UI screenshots; reference-frame diagram would help.
  - **Real-World Viz:** IPF map of imported multi-slice EDAX `.h5` showing the slice-by-slice transform consequence.
  - **Units Clarity:** Weaker than Esprit — Z Spacing units (microns) not stated here; origin units absent.
  - **Concept Links:** Euler angles, sample/crystal reference frame, slice-by-slice transform, scan/slice stacking, confidence index/image quality, IPF
  - **Changes Made:** Stated Z Spacing and Origin units as microns (confirmed from parameters); cross-linked the 'Perform Slice By Slice Transform' mention to Rotate Sample Reference Frame. Converted `{ref}` links to MyST, fixed the broken bold, defined CI/IQ; added Downstream Processing and Required Input Sources; removed the empty Example Pipelines heading.

- [x] **WriteGBCDTriangleDataFilter** (OrientationAnalysis)
  - **Clarity:** Decent — writes per-triangle GBCD info (inward/outward Euler angles, normals, areas) with sample output. A general reader won't know why this matters or what "inward/outward" means.
  - **Completeness:** Good example with column legend, but the legend has a numbering bug ("Column 8: surface area" should be Column 10; cols 7-9 are the normal). No parameter explanation, no Required Input Sources.
  - **Accessibility:** "GBCD", "Euler angles", "triangle normals", "right/left hand average orientation" undefined. Cites Rohrer without a reference link.
  - **Figures Needed:** Boundary-triangle diagram: two grains, their average orientations (left/right), triangle normal, area — maps to the columns.
  - **Real-World Viz:** Optional screenshot of a surface mesh colored by feature.
  - **Units Clarity:** Euler angles stated as RADIANS (good). Normals are unitless direction cosines — say so. Area units (square microns?) not stated.
  - **Concept Links:** GBCD, Euler angles, triangle/surface mesh, feature average orientation, crystal structures
  - **Changes Made:** Rewrote the description in plain language (grain boundary, **Triangle Geometry**, Euler angles, normal, area). Fixed the column-legend numbering bug (surface area is column 10, not 8). Stated units (Euler radians, unitless normals, square-micron areas), defined inward/outward (left/right) orientations, added a Rohrer citation, and added Required Input Sources naming each producer. NOTE: the same column-legend bug also exists in the C++ algorithm's emitted file header.

- [x] **WritePoleFigureFilter** (OrientationAnalysis)
  - **Clarity:** Output Options sections clear and well organized, but never defines "pole figure"; "modified Lambert square", "unit circle interpolation", "Laue Class" appear undefined.
  - **Completeness:** Good coverage + example images. Missing Required Input Sources. The "In a practical sense…" bullet list (Cell Euler Angles, Phases, Mask; Ensemble Laue Class, Material Names) is effectively a hand-written input-array list — borderline defect; reframe as Required Input Sources.
  - **Accessibility:** "pole figure", "Lambert projection/square", "Laue Class", "Ensemble", "stereographic" undefined. No MyST links.
  - **Figures Needed:** Conceptual diagram of the stereographic/Lambert projection (3D orientation → 2D unit circle).
  - **Real-World Viz:** Already shows colorized vs discrete examples; optionally a labeled example (color bar, hemisphere label, phase name).
  - **Units Clarity:** "Lambert Image Size (Pixels)" and output size are pixels — state in prose. Euler angles radians (good).
  - **Concept Links:** pole figure, Lambert projection, stereographic projection, Laue class/crystal structure, Ensemble/Feature data, IPF, GBCD
  - **Changes Made:** Added a plain-language pole-figure definition and first-use definitions of Lambert/stereographic projection and Laue Class. Converted the hand-written 'In a practical sense' input-array list into a proper Required Input Sources section; stated pixel units for image sizes; matched the doc to the current refactored parameters; kept the example images.

### Tier 3 — Polish

| Filter | Plugin | Status |
|--------|--------|--------|
| ReadCSVFile | SimplnxCore | Done |
| ReadStringDataArray | SimplnxCore | Done |
| ReadTextDataArray | SimplnxCore | Done |
| WriteASCIIData | SimplnxCore | Done |
| WriteFeatureDataCSV | SimplnxCore | Done |
| ReadImage | SimplnxCore | Done |
| ReadImageStack | SimplnxCore | Done |
| ReadNIfTIFile | SimplnxCore | Done |
| ReadZeissTxmFile | SimplnxCore | Done |
| ReadBinaryCTNorthstar | SimplnxCore | Done |
| WriteNodesAndElementsFiles | SimplnxCore | Done |
| WriteStlFile | SimplnxCore | Done |
| ReadGrainMapper3D | OrientationAnalysis | Done |
| WriteINLFile | OrientationAnalysis | Done |
| WriteStatsGenOdfAngleFile | OrientationAnalysis | Done |

- [x] **ReadCSVFileFilter** (SimplnxCore)
  - **Changes Made:** Converted the bold-only 'Combine Attribute Arrays' mention to a MyST link; gave the 10 identical image alt-texts unique descriptive alts; added Required Input Sources (None).

- [x] **ReadStringDataArrayFilter** (SimplnxCore)
  - **Changes Made:** Fixed typos 'Created Arra Path' and 'mulplying'; converted the quoted 'Read CSV Filter' to a MyST link; added Required Input Sources (None).

- [x] **ReadTextDataArrayFilter** (SimplnxCore)
  - **Changes Made:** Fixed typos 'Created Arra Path', 'mulplying', 'Componenets'; converted 'Read CSV Filter' to a MyST link; added a descriptive image alt; confirmed via the cpp there is no 'New Line' delimiter option here; added Required Input Sources (None).

- [x] **WriteASCIIDataFilter** (SimplnxCore)
  - **Changes Made:** Cleaned the cluttered Group line to 'IO (Output)'; added a 'Maximum Tuples Per Line' description (units = tuples per row, multiple-files mode); noted inputs are user-selected Data Arrays.

- [x] **WriteFeatureDataCSVFilter** (SimplnxCore)
  - **Changes Made:** Defined **Feature** on first use; explained the leading output line is the total Feature count; added Required Input Sources naming [Compute Feature Neighbors](ComputeFeatureNeighborsFilter.md) and Compute Feature Neighbor Misorientations.

- [x] **ReadImageFilter** (SimplnxCore)
  - **Changes Made:** Added an up-front 'single 2D image' note with a link to the stack reader; stated Origin/Spacing units; added Required Input Sources (None).

- [x] **ReadImageStackFilter** (SimplnxCore)
  - **Changes Made:** Fixed the title to match humanName 'Read Images [3D Stack]'; converted the bold Read Image mention to a MyST link; added a 'File List and Slice Ordering' subsection; defined the luminosity algorithm; stated spacing/origin units; added Required Input Sources (None).

- [x] **ReadNIfTIFileFilter** (SimplnxCore)
  - **Changes Made:** Removed the hand-written `## Parameters` table that duplicated the auto-table (kept the Caveats list); added plain-language glosses for sform/qform/pixdim/vox_offset/scl_slope/scl_inter; noted spacing units come from the file (often mm); added Required Input Sources (None).

- [x] **ReadZeissTxmFileFilter** (SimplnxCore)
  - **Changes Made:** Expanded 'xCT' to X-ray computed tomography on first use; stated spacing/origin/units are read from the file and named the created output array; normalized trailing-## headings; added Required Input Sources (None).

- [x] **ReadBinaryCTNorthstarFilter** (SimplnxCore)
  - **Changes Made:** Normalized the Group line to 'IO (Input)'; defined CT on first use; named the created output array; noted the subvolume start/end are zero-based inclusive voxels; added Required Input Sources (None).

- [x] **WriteNodesAndElementsFilesFilter** (SimplnxCore)
  - **Changes Made:** Added Required Input Sources (any node-based geometry from a meshing/geometry-creation filter); fixed the 'requried' typo; stated node coordinates are in the geometry's physical units.

- [x] **WriteStlFileFilter** (SimplnxCore)
  - **Changes Made:** Fixed the 'implict' typo; clarified the 2-component Features array holds the two Feature IDs on either side of each triangle; noted vertex coordinates are in the Triangle Geometry's units; added Required Input Sources (Triangle Geometry + Face Labels).

- [x] **ReadGrainMapper3DFilter** (OrientationAnalysis)
  - **Changes Made:** Added a GrainMapper3D/XNovo LabDCT gloss; clarified the two IPF-color options; added a millimeters-vs-microns downstream caveat; added Required Input Sources (None).

- [x] **WriteINLFileFilter** (OrientationAnalysis)
  - **Changes Made:** Annotated the output columns with explicit units (Euler radians, positions/step sizes microns); explained the Symmetry column holds crystal-symmetry codes (43 = cubic m-3m, 62 = hexagonal); added Required Input Sources.

- [x] **WriteStatsGenOdfAngleFileFilter** (OrientationAnalysis)
  - **Changes Made:** Expanded ODF to Orientation Distribution Function on first use; fixed 'can not' -> 'cannot'; noted weight and sigma are dimensionless; added Required Input Sources (Euler angles from an EBSD reader; optional Mask from Multi-Threshold Objects).

### Tier 4 — Adequate

| Filter | Plugin | Status |
|--------|--------|--------|
| ReadRawBinary | SimplnxCore | Done (minor) |
| WriteImage | SimplnxCore | Done (minor) |
| WriteDREAM3D | SimplnxCore | Done (minor) |

- [x] **ReadRawBinaryFilter** (SimplnxCore)
  - **Changes Made:** Added a 'Required Input Sources: None' note. No other changes — the doc was already a model for the batch.

- [x] **WriteImageFilter** (SimplnxCore)
  - **Changes Made:** Added a Required Input Sources section naming the image readers / image-processing producers. No other changes — already publication-ready.

- [x] **WriteDREAM3DFilter** (SimplnxCore)
  - **Changes Made:** Added the missing Group (Subgroup) header and a Required Input Sources note (none; overwrites existing file). No other changes — already thorough.

## Batch 8: Image Processing / ITK Wrappers

**Plugin:** ITKImageProcessing
**Filters:** 88 (ITK-wrapped image-processing filters)

> Triaged 2026-06-11. Tier split: T1=14, T2=45, T3=26, T4=3. These descriptions were lifted from ITK; the guiding question is whether a general engineer can understand the filter and set its parameters WITHOUT reading ITK's Doxygen. User decisions: trivial intensity ops get minimal-clean treatment; Required Input Sources is a light generic note (ITK inputs are generic scalar images).
>
> **Recurring ITK defects to sweep:** stale `## DREAM3D Mailing Lists` footer (~17 docs); raw Doxygen math `\f[ \f]`/`\f$ \f$` (BinaryThreshold, RescaleIntensity, Normalize, CurvatureFlow, MinMaxCurvatureFlow); mangled `.* ClassName`/`\sa`/`\li` See-Also artifacts dumped inline (pervasive); C++/developer symbols (`itk::`, `NumericTraits<>`, 'templated over', `Set*()` methods, `m_*`); hand-duplicated parameter tables missing the auto-table marker (IsoContourDistance, GradientMagnitudeRecursiveGaussian, ImportFijiMontage, ThresholdMaximumConnectedComponents).
>
> **Bugs / mismatches found:** ITKGrayscaleErode says 'Erosion takes the maximum' (should be minimum); ITKExpNegative describes a user-provided constant K the filter does not expose; ITKLog has no description text; ITKThreshold lists Above/Below/Outside methods not exposed as params; ITKAdaptiveHistogramEqualization prose may describe alpha/beta that are not exposed — all to verify against the cpp during rewrite.
>
> **Shared concepts to author once and reuse:** morphology (erosion/dilation/opening/closing + the structuring element / kernel radius in pixels); projection-along-an-axis (Projection Dimension); connectivity (Fully Connected = face-only vs face+edge+vertex); distance maps (signed = inside negative; squared-distance and use-image-spacing toggles). **Naming collisions:** ITK Read Image / Read Images [3D Stack] vs the SimplnxCore filters of the same human name — disambiguate and cross-link.

### Tier 1 — Critical

| Filter | Plugin | Status |
|--------|--------|--------|
| ITKCurvatureAnisotropicDiffusionImageFilter | ITKImageProcessing | Done |
| ITKGradientAnisotropicDiffusionImageFilter | ITKImageProcessing | Done |
| ITKCurvatureFlowImageFilter | ITKImageProcessing | Done |
| ITKMinMaxCurvatureFlowImageFilter | ITKImageProcessing | Done |
| ITKBinaryOpeningByReconstructionImageFilter | ITKImageProcessing | Done |
| ITKBinaryThinningImageFilter | ITKImageProcessing | Done |
| ITKMorphologicalGradientImageFilter | ITKImageProcessing | Done |
| ITKClosingByReconstructionImageFilter | ITKImageProcessing | Done |
| ITKOpeningByReconstructionImageFilter | ITKImageProcessing | Done |
| ITKBinaryThresholdImageFilter | ITKImageProcessing | Done |
| ITKDoubleThresholdImageFilter | ITKImageProcessing | Done |
| ITKIsoContourDistanceImageFilter | ITKImageProcessing | Done |
| ITKMorphologicalWatershedImageFilter | ITKImageProcessing | Done |
| ITKMorphologicalWatershedFromMarkersImageFilter | ITKImageProcessing | Done |

- [x] **ITKCurvatureAnisotropicDiffusionImageFilter** (ITKImageProcessing)
  - **Changes Made:** Added a plain-language 'what is anisotropic diffusion' lead and an MCDE note; stripped itk:: symbols; documented the previously-undocumented Conductance Scaling Update Interval; kept the TimeStep/Conductance/Iterations guidance with stability values; fixed the stale footer; cleaned the See-Also.

- [x] **ITKGradientAnisotropicDiffusionImageFilter** (ITKImageProcessing)
  - **Changes Made:** Added a plain-language Perona-Malik / edge-stopping lead; stripped itk:: symbols and fixed the copy-paste 'CurvatureND' reference; documented Conductance Scaling Update Interval; promoted the bare URLs to a See-Also; fixed the stale footer.

- [x] **ITKCurvatureFlowImageFilter** (ITKImageProcessing)
  - **Changes Made:** Replaced the raw Doxygen math with a plain contour-curvature explanation; trimmed the solver-internals/padding paragraphs; added a concrete TimeStep typical value; fixed the stale footer; kept the Sethian reference.

- [x] **ITKMinMaxCurvatureFlowImageFilter** (ITKImageProcessing)
  - **Changes Made:** Replaced ALL raw Doxygen math with a plain explanation of the min/max scale switch; defined Stencil Radius in pixels with a typical value; added TimeStep/iterations guidance; fixed the stale footer.

- [x] **ITKBinaryOpeningByReconstructionImageFilter** (ITKImageProcessing)
  - **Changes Made:** Defined reconstruction and the shape-preservation advantage over plain opening; fixed the \sa run-on; stated Kernel Radius in pixels and defined Foreground/Background/Fully Connected; kept the figure and Kernel Type table; light Required Input Sources note.

- [x] **ITKBinaryThinningImageFilter** (ITKImageProcessing)
  - **Changes Made:** Removed the 'parameterized over...' and 'To do: Make this filter ND.*' fragments; defined skeleton/medial axis and restated the 2D-only behavior as a plain caveat; light Required Input Sources note; kept the Gonzalez/Woods reference.

- [x] **ITKMorphologicalGradientImageFilter** (ITKImageProcessing)
  - **Changes Made:** Rewrote the description from scratch: morphological gradient = dilation - erosion = edge magnitude; stripped the ITK boilerplate/cross-ref tail; stated Kernel Radius in pixels; kept the Kernel Type table.

- [x] **ITKClosingByReconstructionImageFilter** (ITKImageProcessing)
  - **Changes Made:** Fixed the garbled 'levels raises' and broken PreserveIntensities sentences; defined Preserve Intensities and Fully Connected; stated Kernel Radius in pixels; explained the reconstruction concept; kept the Soille reference.

- [x] **ITKOpeningByReconstructionImageFilter** (ITKImageProcessing)
  - **Changes Made:** As with Closing-by-Reconstruction: fixed the garbled sentences, defined Preserve Intensities + Fully Connected, stated Kernel Radius in pixels, explained reconstruction.

- [x] **ITKBinaryThresholdImageFilter** (ITKImageProcessing)
  - **Changes Made:** Converted the raw Doxygen \f[...\f] piecewise math to a MyST $$ block; stripped NumericTraits/'templated over' text; stated Lower/Upper are in intensity units (usually set only one) and Inside/Outside are uint8 labels; kept the figure.

- [x] **ITKDoubleThresholdImageFilter** (ITKImageProcessing)
  - **Changes Made:** Mapped Threshold1-4 to the narrow (T2-T3) inside wide (T1-T4) hysteresis ranges with the ordering requirement and intensity units; removed the broken ITK See-Also bullets; defined Fully Connected; fixed the stale footer.

- [x] **ITKIsoContourDistanceImageFilter** (ITKImageProcessing)
  - **Changes Made:** Led with a plain definition (for a level-set/signed image, the distance from near-contour pixels to the zero contour); defined level set/iso-contour/narrowband; DELETED the hand-written Parameters/Required/Created tables and ADDED the auto-table marker; described Far Value correctly as a clamp distance (default 10) — NOTE the cpp FarValue docstring is a copy-paste bug; fixed the stale footer.

- [x] **ITKMorphologicalWatershedImageFilter** (ITKImageProcessing)
  - **Changes Made:** Led with the terrain/flooding analogy; defined catchment basin and watershed line; documented Level (intensity, over-segmentation control), Mark Watershed Line, Fully Connected; noted the over-segmentation tendency, the Relabel follow-up, and the marker-controlled variant; stripped TOutputImage; kept the Soille/IJ reference.

- [x] **ITKMorphologicalWatershedFromMarkersImageFilter** (ITKImageProcessing)
  - **Changes Made:** Removed the redundant title parenthetical; led with the marker-controlled watershed concept; documented Mark Watershed Line and Fully Connected; added a prominent note that the NX wrapper currently exposes only a single input array (no separate marker-image parameter) — flagged for verification; fixed the malformed See-Also and stale footer.

### Tier 2 — Important

| Filter | Plugin | Status |
|--------|--------|--------|
| ITKSigmoidImageFilter | ITKImageProcessing | Done |
| ITKIntensityWindowingImageFilter | ITKImageProcessing | Done |
| ITKRescaleIntensityImageFilter | ITKImageProcessing | Done |
| ITKDiscreteGaussianImageFilter | ITKImageProcessing | Done |
| ITKSmoothingRecursiveGaussianImageFilter | ITKImageProcessing | Done |
| ITKMedianImageFilter | ITKImageProcessing | Done |
| ITKAdaptiveHistogramEqualizationImageFilter | ITKImageProcessing | Done |
| ITKBinaryDilateImageFilter | ITKImageProcessing | Done |
| ITKBinaryErodeImageFilter | ITKImageProcessing | Done |
| ITKBinaryMorphologicalClosingImageFilter | ITKImageProcessing | Done |
| ITKBinaryMorphologicalOpeningImageFilter | ITKImageProcessing | Done |
| ITKBinaryContourImageFilter | ITKImageProcessing | Done |
| ITKDilateObjectMorphologyImageFilter | ITKImageProcessing | Done |
| ITKErodeObjectMorphologyImageFilter | ITKImageProcessing | Done |
| ITKGrayscaleDilateImageFilter | ITKImageProcessing | Done |
| ITKGrayscaleErodeImageFilter | ITKImageProcessing | Done |
| ITKGrayscaleMorphologicalClosingImageFilter | ITKImageProcessing | Done |
| ITKGrayscaleMorphologicalOpeningImageFilter | ITKImageProcessing | Done |
| ITKGrayscaleFillholeImageFilter | ITKImageProcessing | Done |
| ITKGrayscaleGrindPeakImageFilter | ITKImageProcessing | Done |
| ITKBlackTopHatImageFilter | ITKImageProcessing | Done |
| ITKWhiteTopHatImageFilter | ITKImageProcessing | Done |
| ITKThresholdImageFilter | ITKImageProcessing | Done |
| ITKOtsuMultipleThresholdsImageFilter | ITKImageProcessing | Done |
| ITKThresholdMaximumConnectedComponentsImageFilter | ITKImageProcessing | Done |
| ITKHConvexImageFilter | ITKImageProcessing | Done |
| ITKHMaximaImageFilter | ITKImageProcessing | Done |
| ITKHMinimaImageFilter | ITKImageProcessing | Done |
| ITKApproximateSignedDistanceMapImageFilter | ITKImageProcessing | Done |
| ITKDanielssonDistanceMapImageFilter | ITKImageProcessing | Done |
| ITKSignedDanielssonDistanceMapImageFilter | ITKImageProcessing | Done |
| ITKSignedMaurerDistanceMapImageFilter | ITKImageProcessing | Done |
| ITKGradientMagnitudeRecursiveGaussianImageFilter | ITKImageProcessing | Done |
| ITKZeroCrossingImageFilter | ITKImageProcessing | Done |
| ITKConnectedComponentImageFilter | ITKImageProcessing | Done |
| ITKRelabelComponentImageFilter | ITKImageProcessing | Done |
| ITKLabelContourImageFilter | ITKImageProcessing | Done |
| ITKMaximumProjectionImageFilter | ITKImageProcessing | Done |
| ITKMeanProjectionImageFilter | ITKImageProcessing | Done |
| ITKMedianProjectionImageFilter | ITKImageProcessing | Done |
| ITKMinimumProjectionImageFilter | ITKImageProcessing | Done |
| ITKStandardDeviationProjectionImageFilter | ITKImageProcessing | Done |
| ITKSumProjectionImageFilter | ITKImageProcessing | Done |
| ITKBinaryProjectionImageFilter | ITKImageProcessing | Done |
| ITKImportFijiMontageFilter | ITKImageProcessing | Done |

- [x] **ITKSigmoidImageFilter** (ITKImageProcessing)
  - **Changes Made:** Defined the sigmoid S-curve remap with MyST math; documented Alpha (transition width), Beta (center), Output Min/Max; light Required Input Sources.

- [x] **ITKIntensityWindowingImageFilter** (ITKImageProcessing)
  - **Changes Made:** Defined the window/level remap; fixed the mangled See-Also fragment and 'RealType'; stated all four params in intensity units.

- [x] **ITKRescaleIntensityImageFilter** (ITKImageProcessing)
  - **Changes Made:** Converted the Doxygen formula to MyST; clarified only Output Min/Max are user-set (input auto-detected); fixed the mangled See-Also.

- [x] **ITKDiscreteGaussianImageFilter** (ITKImageProcessing)
  - **Changes Made:** Defined Gaussian blur; clarified Variance = sigma^2 with units (px^2 / physical^2); documented MaximumKernelWidth/MaximumError/UseImageSpacing; removed the ITK artifact + class list.

- [x] **ITKSmoothingRecursiveGaussianImageFilter** (ITKImageProcessing)
  - **Changes Made:** Led with the constant-time-vs-sigma advantage; Sigma units; explained Normalize Across Scale; fixed the stale footer and stray '|'.

- [x] **ITKMedianImageFilter** (ITKImageProcessing)
  - **Changes Made:** Defined the median filter (salt-and-pepper removal, edge-preserving); Radius in pixels (window 2r+1); removed the operator<()/class-list artifact.

- [x] **ITKAdaptiveHistogramEqualizationImageFilter** (ITKImageProcessing)
  - **Changes Made:** Confirmed alpha/beta ARE exposed in the cpp; added pixels unit to Radius; documented alpha/beta/window; light Required Input Sources.

- [x] **ITKBinaryDilateImageFilter** (ITKImageProcessing)
  - **Changes Made:** Defined dilation (grows foreground); documented Foreground/Background/BoundaryToForeground and Kernel Radius (pixels); stripped C++ symbols + See-Also run-on; cross-linked siblings.

- [x] **ITKBinaryErodeImageFilter** (ITKImageProcessing)
  - **Changes Made:** Defined erosion (shrinks foreground; eroded->Background); replaced the NonpositiveMin() wording; Kernel Radius (pixels); cross-linked siblings.

- [x] **ITKBinaryMorphologicalClosingImageFilter** (ITKImageProcessing)
  - **Changes Made:** Defined closing = dilate-then-erode (fills small holes); Foreground Value, Kernel Radius (pixels), Safe Border; stripped ITK Related-Filters; cross-linked siblings.

- [x] **ITKBinaryMorphologicalOpeningImageFilter** (ITKImageProcessing)
  - **Changes Made:** Defined opening = erode-then-dilate (removes small specks); fixed 'Dilatation'; Foreground/Background, Kernel Radius (pixels); cross-linked siblings.

- [x] **ITKBinaryContourImageFilter** (ITKImageProcessing)
  - **Changes Made:** Defined object-outline extraction; Foreground/Background values and Fully Connected (On=thicker); replaced SetFullyConnected(); cross-linked Label Contour.

- [x] **ITKDilateObjectMorphologyImageFilter** (ITKImageProcessing)
  - **Changes Made:** Defined single-object dilation; Object Value + Kernel Radius (pixels); fixed the See-Also run-on; noted the distinction from Binary Dilate.

- [x] **ITKErodeObjectMorphologyImageFilter** (ITKImageProcessing)
  - **Changes Made:** Defined single-object erosion; Object Value/Background Value + Kernel Radius (pixels); fixed the See-Also run-on; noted the distinction from Binary Erode.

- [x] **ITKGrayscaleDilateImageFilter** (ITKImageProcessing)
  - **Changes Made:** Defined grayscale dilation = local max filter; Kernel Radius (pixels); stripped the ITK cross-ref tail.

- [x] **ITKGrayscaleErodeImageFilter** (ITKImageProcessing)
  - **Changes Made:** FIXED the factual error (erosion is the minimum, not maximum); defined grayscale erosion = local min; Kernel Radius (pixels); stripped the ITK tail.

- [x] **ITKGrayscaleMorphologicalClosingImageFilter** (ITKImageProcessing)
  - **Changes Made:** Defined grayscale closing = dilate-then-erode (removes small dark features); Kernel Radius (pixels), Safe Border; stripped the mismatched ITK tail.

- [x] **ITKGrayscaleMorphologicalOpeningImageFilter** (ITKImageProcessing)
  - **Changes Made:** Defined grayscale opening = erode-then-dilate (removes small bright features); Kernel Radius (pixels), Safe Border; stripped the ITK tail.

- [x] **ITKGrayscaleFillholeImageFilter** (ITKImageProcessing)
  - **Changes Made:** Defined hole-filling (enclosed dark minima); documented Fully Connected; trimmed the geodesic-implementation noise; kept the Soille reference.

- [x] **ITKGrayscaleGrindPeakImageFilter** (ITKImageProcessing)
  - **Changes Made:** Defined peak-removal (enclosed bright maxima; dual of Fillhole); documented Fully Connected; trimmed/fixed the geodesic implementation paragraph.

- [x] **ITKBlackTopHatImageFilter** (ITKImageProcessing)
  - **Changes Made:** Defined black top-hat = closing minus image (small dark detail); Kernel Radius (pixels) tied to feature scale, Safe Border; removed the empty Related Filters heading.

- [x] **ITKWhiteTopHatImageFilter** (ITKImageProcessing)
  - **Changes Made:** Added a real description (image minus opening = small bright detail); Kernel Radius (pixels), Safe Border; removed the empty Related Filters heading.

- [x] **ITKThresholdImageFilter** (ITKImageProcessing)
  - **Changes Made:** Removed the phantom ThresholdAbove/Below/Outside method list; rewrote to the actual Lower/Upper/OutsideValue params (out-of-range -> OutsideValue, in-range kept); intensity units; contrasted with Binary Threshold.

- [x] **ITKOtsuMultipleThresholdsImageFilter** (ITKImageProcessing)
  - **Changes Made:** Defined Otsu (automatic variance-maximizing thresholds; N+1-class label map); removed ITK class/setter/test names and fixed the mangled run-on; per-param guidance (thresholds/bins/offset/valley emphasis/bin midpoint).

- [x] **ITKThresholdMaximumConnectedComponentsImageFilter** (ITKImageProcessing)
  - **Changes Made:** Defined the topological auto-threshold (maximizes object count above a minimum size); removed debug/ITK-method text, the broken sentence, and the personal-email block; fixed the stale footer; documented UpperBoundary/MinimumObjectSizeInPixels/Inside/Outside; replaced the hand Parameters prose.

- [x] **ITKHConvexImageFilter** (ITKImageProcessing)
  - **Changes Made:** Defined h-convex (extracts peaks rising more than h above background); Height in intensity units, FullyConnected; stripped the mangled See-Also tail.

- [x] **ITKHMaximaImageFilter** (ITKImageProcessing)
  - **Changes Made:** Defined h-maxima (suppresses maxima shallower than h); Height in intensity units; removed SetHeight + the mangled See-Also; cross-linked H Convex/H Minima.

- [x] **ITKHMinimaImageFilter** (ITKImageProcessing)
  - **Changes Made:** Defined h-minima (fills minima shallower than h); Height + FullyConnected; stripped the mangled See-Also; replaced the non-NX HConcave reference.

- [x] **ITKApproximateSignedDistanceMapImageFilter** (ITKImageProcessing)
  - **Changes Made:** Defined signed distance map (inside negative) via Chamfer approximation (pixels); concrete Inside/Outside Value example; demoted ITK siblings to See Also; fixed the stale footer.

- [x] **ITKDanielssonDistanceMapImageFilter** (ITKImageProcessing)
  - **Changes Made:** Defined distance map + Voronoi partition + offset-vector outputs in lay terms; documented Input Is Binary / Squared Distance / Use Image Spacing (pixels vs physical); stripped itk::Offset/'4SED'; fixed the stale footer.

- [x] **ITKSignedDanielssonDistanceMapImageFilter** (ITKImageProcessing)
  - **Changes Made:** Stripped the \li markup and the 'parameterized over' text; fixed the broken ITK link; documented Inside Is Positive / Squared Distance / Use Image Spacing; fixed the stale footer.

- [x] **ITKSignedMaurerDistanceMapImageFilter** (ITKImageProcessing)
  - **Changes Made:** Trimmed the C++ type discussion; prominently flagged that Squared Distance defaults TRUE; documented Use Image Spacing / Background Value; fixed run-in sub-headers; kept the Maurer reference.

- [x] **ITKGradientMagnitudeRecursiveGaussianImageFilter** (ITKImageProcessing)
  - **Changes Made:** DELETED the hand Parameters/Required/Created tables and ADDED the missing auto-table marker; defined gradient magnitude (edge strength); emphasized Sigma is in image-spacing (physical) units; rewrote Normalize Across Scale; fixed the stale footer.

- [x] **ITKZeroCrossingImageFilter** (ITKImageProcessing)
  - **Changes Made:** Defined zero-crossing edge detection on a signed input; stripped itk::/operator/NumericTraits text; restated the Foreground/Background label params; Required Input Sources = a signed (Laplacian-of-Gaussian) image; fixed the stale footer.

- [x] **ITKConnectedComponentImageFilter** (ITKImageProcessing)
  - **Changes Made:** Defined connected components / label image; documented Fully Connected (+1-pixel note); noted the Relabel Component follow-up; stripped ITK method names + the useless See-Also; fixed the stale footer.

- [x] **ITKRelabelComponentImageFilter** (ITKImageProcessing)
  - **Changes Made:** Defined relabel (consecutive, size-sorted; optional minimum size); documented Minimum Object Size in pixels and SortByObjectSize; stripped ITK method names + the InPlace paragraph + the mangled See-Also; Required Input Sources = a label image.

- [x] **ITKLabelContourImageFilter** (ITKImageProcessing)
  - **Changes Made:** Defined label-region outline extraction; documented Background Value + Fully Connected (thicker-contours note); stripped SetFullyConnected(); moved the Insight-Journal URL to See Also; Required Input Sources = a label image.

- [x] **ITKMaximumProjectionImageFilter** (ITKImageProcessing)
  - **Changes Made:** Added the shared projection concept (3D->2D, maximum along an axis); documented Projection Dimension (axis index) + Perform In-Place + the dimensionality note; trimmed the Doxygen See-Also.

- [x] **ITKMeanProjectionImageFilter** (ITKImageProcessing)
  - **Changes Made:** Shared projection concept (mean along an axis); cross-linked Median for outlier sensitivity; removed the stray title parenthetical; trimmed the See-Also.

- [x] **ITKMedianProjectionImageFilter** (ITKImageProcessing)
  - **Changes Made:** Shared projection concept (median along an axis; outlier-robust vs mean); removed the stray title parenthetical; trimmed the See-Also.

- [x] **ITKMinimumProjectionImageFilter** (ITKImageProcessing)
  - **Changes Made:** Shared projection concept (minimum along an axis; dark-feature counterpart); removed the stray title parenthetical; trimmed the See-Also.

- [x] **ITKStandardDeviationProjectionImageFilter** (ITKImageProcessing)
  - **Changes Made:** Shared projection concept (standard deviation along an axis; highlights variability); fixed the stale footer; deduped the See-Also.

- [x] **ITKSumProjectionImageFilter** (ITKImageProcessing)
  - **Changes Made:** Shared projection concept (sum along an axis); added an overflow/data-type caution; fixed the stale footer; deduped the See-Also.

- [x] **ITKBinaryProjectionImageFilter** (ITKImageProcessing)
  - **Changes Made:** Refined the projection text; documented the logical-OR rule, Foreground/Background Value, Projection Dimension, Perform In-Place; deduped Related Filters; removed the ITK Doxygen link list.

- [x] **ITKImportFijiMontageFilter** (ITKImageProcessing)
  - **Changes Made:** Title matches humanName 'Read Fiji Montage (ITK)'; rewrote the output description in current DataStructure terms (per the cpp: per-tile Image Geometry -> Cell Attribute Matrix -> Image array); removed the hand-listed Length Unit / Output Data Type enums; fixed typos; replaced the stale Google-Groups footer; corrected the false 'same name' claim.

### Tier 3 — Polish

| Filter | Plugin | Status |
|--------|--------|--------|
| ITKAbsImageFilter | ITKImageProcessing | Done |
| ITKAcosImageFilter | ITKImageProcessing | Done |
| ITKAsinImageFilter | ITKImageProcessing | Done |
| ITKAtanImageFilter | ITKImageProcessing | Done |
| ITKCosImageFilter | ITKImageProcessing | Done |
| ITKSinImageFilter | ITKImageProcessing | Done |
| ITKTanImageFilter | ITKImageProcessing | Done |
| ITKExpImageFilter | ITKImageProcessing | Done |
| ITKExpNegativeImageFilter | ITKImageProcessing | Done |
| ITKLogImageFilter | ITKImageProcessing | Done |
| ITKLog10ImageFilter | ITKImageProcessing | Done |
| ITKSqrtImageFilter | ITKImageProcessing | Done |
| ITKNotImageFilter | ITKImageProcessing | Done |
| ITKInvertIntensityImageFilter | ITKImageProcessing | Done |
| ITKBoundedReciprocalImageFilter | ITKImageProcessing | Done |
| ITKNormalizeImageFilter | ITKImageProcessing | Done |
| ITKNormalizeToConstantImageFilter | ITKImageProcessing | Done |
| ITKRegionalMaximaImageFilter | ITKImageProcessing | Done |
| ITKRegionalMinimaImageFilter | ITKImageProcessing | Done |
| ITKValuedRegionalMaximaImageFilter | ITKImageProcessing | Done |
| ITKValuedRegionalMinimaImageFilter | ITKImageProcessing | Done |
| ITKGradientMagnitudeImageFilter | ITKImageProcessing | Done |
| ITKLaplacianRecursiveGaussianImageFilter | ITKImageProcessing | Done |
| ITKMaskImageFilter | ITKImageProcessing | Done |
| ITKImageReaderFilter | ITKImageProcessing | Done |
| ITKMhaFileReaderFilter | ITKImageProcessing | Done |

- [x] **ITKAbsImageFilter** (ITKImageProcessing)
  - **Changes Made:** Replaced the 'itk::Math::abs()' line with a plain one-sentence description (absolute value / magnitude).

- [x] **ITKAcosImageFilter** (ITKImageProcessing)
  - **Changes Made:** Collapsed the templating/cast boilerplate to one sentence: arccosine; input [-1,1]; output in radians.

- [x] **ITKAsinImageFilter** (ITKImageProcessing)
  - **Changes Made:** One sentence: arcsine; input [-1,1]; output in radians.

- [x] **ITKAtanImageFilter** (ITKImageProcessing)
  - **Changes Made:** One sentence: arctangent; output in radians.

- [x] **ITKCosImageFilter** (ITKImageProcessing)
  - **Changes Made:** One sentence: cosine; values interpreted as radians; output [-1,1].

- [x] **ITKSinImageFilter** (ITKImageProcessing)
  - **Changes Made:** Reworded to plain 'sine (values in radians)'; dropped std::sin; kept the figure.

- [x] **ITKTanImageFilter** (ITKImageProcessing)
  - **Changes Made:** Reworded to plain 'tangent (values in radians)'; dropped std::tan.

- [x] **ITKExpImageFilter** (ITKImageProcessing)
  - **Changes Made:** Reworded to 'natural exponential e^pixel'; dropped std::exp.

- [x] **ITKExpNegativeImageFilter** (ITKImageProcessing)
  - **Changes Made:** Verified NO K parameter is exposed (functor default-constructed, K fixed at 1); corrected the summary and description to exp(-x) instead of the phantom 'user-provided constant K'.

- [x] **ITKLogImageFilter** (ITKImageProcessing)
  - **Changes Made:** Added the missing description sentence (natural logarithm; input must be > 0); kept the figure.

- [x] **ITKLog10ImageFilter** (ITKImageProcessing)
  - **Changes Made:** Reworded to 'base-10 logarithm; input must be > 0'; dropped std::log10.

- [x] **ITKSqrtImageFilter** (ITKImageProcessing)
  - **Changes Made:** Replaced the std::sqrt line with a plain description (square root; non-negative input).

- [x] **ITKNotImageFilter** (ITKImageProcessing)
  - **Changes Made:** Removed the templated/bool text and the raw C++ code block + m_* symbols; plain description (integer image; 0->Foreground, non-zero->Background).

- [x] **ITKInvertIntensityImageFilter** (ITKImageProcessing)
  - **Changes Made:** Replaced 'SetMaximum' with plain Maximum-minus-pixel guidance (set Maximum to the largest intensity, e.g. 255 for 8-bit); trimmed ITK Author/Related-Filters.

- [x] **ITKBoundedReciprocalImageFilter** (ITKImageProcessing)
  - **Changes Made:** Replaced the dimension/scalar-type sentence with the 1/(1+x) description (bounded (0,1] for non-negative input).

- [x] **ITKNormalizeImageFilter** (ITKImageProcessing)
  - **Changes Made:** Converted the Doxygen $-sigma$/$+sigma$ to MyST math; removed the mangled See-Also fragment and internal class names; kept the integral-type caveat.

- [x] **ITKNormalizeToConstantImageFilter** (ITKImageProcessing)
  - **Changes Made:** Dropped the redundant title parenthetical; replaced 'SetConstant()' with the Constant parameter (dimensionless target sum, default 1); fixed the stale footer.

- [x] **ITKRegionalMaximaImageFilter** (ITKImageProcessing)
  - **Changes Made:** Defined regional maxima; documented Flat Is Maxima + Fully Connected; noted Background/Foreground are output labels; fixed the stale footer; dropped ITK Author/See-Also.

- [x] **ITKRegionalMinimaImageFilter** (ITKImageProcessing)
  - **Changes Made:** Defined regional minima; de-duplicated the repeated author block; documented Flat Is Minima + Fully Connected; fixed the stale footer; dropped the non-NX HConcave See-Also.

- [x] **ITKValuedRegionalMaximaImageFilter** (ITKImageProcessing)
  - **Changes Made:** Moved the connectivity paragraph into the Description; trimmed the non-NX Related Filters; kept the FullyConnected explanation; added Required Input Sources.

- [x] **ITKValuedRegionalMinimaImageFilter** (ITKImageProcessing)
  - **Changes Made:** Added the FullyConnected/connectivity explanation (mirroring the Maxima doc); tidied the Related Filters run-on; dropped the non-NX entries.

- [x] **ITKGradientMagnitudeImageFilter** (ITKImageProcessing)
  - **Changes Made:** Replaced the duplicated description with a one-sentence gradient-magnitude definition + the noise-sensitivity contrast vs the Recursive Gaussian variant; removed the meaningless ITK building-block Related-Filters list.

- [x] **ITKLaplacianRecursiveGaussianImageFilter** (ITKImageProcessing)
  - **Changes Made:** Added the Laplacian-of-Gaussian definition + the Zero Crossing follow-up link; documented Sigma (image-spacing units) and Normalize Across Scale; fixed the stale footer.

- [x] **ITKMaskImageFilter** (ITKImageProcessing)
  - **Changes Made:** Rewrote the Description as plain prose (dropped 'templated over', the diff-marked code block, Wiki-Examples cruft); documented Outside Value; fixed the stale footer; kept the figure.

- [x] **ITKImageReaderFilter** (ITKImageProcessing)
  - **Changes Made:** Dropped the 'directly wraps an ITK filter' boilerplate; added a note distinguishing it from the SimplnxCore Read Image filter (same human name) + cross-link. (humanName: Read Image (ITK))

- [x] **ITKMhaFileReaderFilter** (ITKImageProcessing)
  - **Changes Made:** Fixed the typos; added a cross-link to Read Image (ITK) noting this variant handles the embedded transform; trimmed the orthogonal-matrix Technical Discussion.

### Tier 4 — Adequate

| Filter | Plugin | Status |
|--------|--------|--------|
| ITKSquareImageFilter | ITKImageProcessing | Done |
| ITKImageWriterFilter | ITKImageProcessing | Done |
| ITKImportImageStackFilter | ITKImageProcessing | Done |

- [x] **ITKSquareImageFilter** (ITKImageProcessing)
  - **Changes Made:** Added the missing one-sentence description ('square of each pixel'). Otherwise adequate.

- [x] **ITKImageWriterFilter** (ITKImageProcessing)
  - **Changes Made:** No change needed — already clean and complete (humanName: Write Image (ITK)).

- [x] **ITKImportImageStackFilter** (ITKImageProcessing)
  - **Changes Made:** Dropped the 'directly wraps' boilerplate; added a note distinguishing it from the SimplnxCore Read Images [3D Stack] filter + cross-link. Otherwise adequate. (humanName: Read Images [3D Stack] (ITK))

## Batch 9: Remaining SimplnxCore Filters

**Plugin:** SimplnxCore
**Filters:** 73 (catch-all for SimplnxCore filters not covered by Batches 1-7; the ITK plugin is Batch 8, deferred)

> Triaged 2026-06-11. Tier split: T1=4, T2=35, T3=31, T4=3. Rewrites will be committed per tier (Tier 2 split into sub-theme sub-commits given its size). Recurring defects flagged: hand-duplicated parameter tables (HierarchicalSmooth; CreatePythonSkeleton — also missing the marker), a legacy `@ref` directive (ComputeBoundingBoxStats), Doxygen math/`@image` artifacts (Silhouette, FeatureFaceCurvature, PointSampleTriangleGeometry), legacy 'Data Container' wording (NearestPointFuseRegularGrids, RemoveFlaggedVertices, ExtractInternalSurfaces), and many bold-only filter mentions.

### Tier 1 — Critical

| Filter | Plugin | Status |
|--------|--------|--------|
| FlyingEdges3D | SimplnxCore | Done |
| TriangleDihedralAngle | SimplnxCore | Done |
| NearestPointFuseRegularGrids | SimplnxCore | Done |
| PointSampleEdgeGeometry | SimplnxCore | Done |

- [x] **FlyingEdges3DFilter** (SimplnxCore)
  - **Changes Made:** Expanded the one-sentence stub: added 'What is an Isosurface?' (contour-line analogy, isovalue), noted Flying Edges is a fast marching-cubes variant, added 'When to Use' contrasting with Surface Nets / Quick Surface Mesh for label maps, documented the Contour Value units (same as the data array) and Required Input Sources (Image Geometry + scalar array). Captioned the figure.

- [x] **TriangleDihedralAngleFilter** (SimplnxCore)
  - **Changes Made:** Replaced the 'matrix mathematics' stub with a plain explanation of the minimum interior angle as a mesh-quality metric (60 deg = equilateral, near-0 deg = sliver). Stated output units are degrees (0-60). Added Required Input Sources (Triangle Geometry from a surface-meshing filter).

- [x] **NearestPointFuseRegularGridsFilter** (SimplnxCore)
  - **Changes Made:** Replaced all legacy 'Data Container' terminology with Image Geometry. Explained Reference (destination) vs Sampling (source) roles, that assignment is by nearest cell-center using each grid's origin/spacing (no interpolation), and that the grids may differ in spacing/extent. Documented the Use Custom Fill Value behavior for out-of-overlap cells. Added Required Input Sources (the two Image Geometries).

- [x] **PointSampleEdgeGeometryFilter** (SimplnxCore)
  - **Changes Made:** Added the missing Group header (Sampling (Geometry)). Defined Edge Geometry, 'scan vector' (an additive-manufacturing laser-path segment), and 'edge ID'. Stated the Sampling Spacing units are millimeters and explained Cumulative Sample Distance. Added Required Input Sources (Edge Geometry from Slice Triangle Geometry / Create AM Scan Paths).

### Tier 2 — Important

| Filter | Plugin | Status |
|--------|--------|--------|
| ComputeArrayStatistics | SimplnxCore | Done |
| ComputeBoundingBoxStats | SimplnxCore | Done |
| ComputeVolumeFractions | SimplnxCore | Done |
| ComputeFeatureClustering | SimplnxCore | Done |
| ComputeBoundaryElementFractions | SimplnxCore | Done |
| ComputeEuclideanDistMap | SimplnxCore | Done |
| ComputeMomentInvariants2D | SimplnxCore | Done |
| ComputeKMeans | SimplnxCore | Done |
| ComputeKMedoids | SimplnxCore | Done |
| Silhouette | SimplnxCore | Done |
| ApproximatePointCloudHull | SimplnxCore | Done |
| MapPointCloudToRegularGrid | SimplnxCore | Done |
| IterativeClosestPoint | SimplnxCore | Done |
| IdentifyDuplicateVertices | SimplnxCore | Done |
| ComputeTriangleAreas | SimplnxCore | Done |
| ComputeTriangleGeomCentroids | SimplnxCore | Done |
| ComputeTriangleGeomVolumes | SimplnxCore | Done |
| TriangleCentroid | SimplnxCore | Done |
| TriangleNormal | SimplnxCore | Done |
| FeatureFaceCurvature | SimplnxCore | Done |
| SharedFeatureFace | SimplnxCore | Done |
| SurfaceNets | SimplnxCore | Done |
| LaplacianSmoothing | SimplnxCore | Done |
| UncertainRegularGridSampleSurfaceMesh | SimplnxCore | Done |
| ExtractInternalSurfacesFromTriangleGeometry | SimplnxCore | Done |
| ReverseTriangleWinding | SimplnxCore | Done |
| VerifyTriangleWinding | SimplnxCore | Done |
| RemoveFlaggedEdges | SimplnxCore | Done |
| RemoveFlaggedTriangles | SimplnxCore | Done |
| CropVertexGeometry | SimplnxCore | Done |
| CombineNodeBasedGeometries | SimplnxCore | Done |
| PointSampleTriangleGeometry | SimplnxCore | Done |
| ExtractFeatureBoundaries2D | SimplnxCore | Done |
| RobustAutomaticThreshold | SimplnxCore | Done |
| ComputeVectorColors | SimplnxCore | Done |

- [x] **ComputeArrayStatisticsFilter** (SimplnxCore)
  - **Changes Made:** Fixed the stale group to 'Statistics'; defined Feature/Ensemble; condensed the redundant Ranges Breakdown Options 1-4; fixed spelling (aproiri/achived/occurance/exisitng); added Required Input Sources (Feature Ids producer). Kept the statistic-type table.

- [x] **ComputeBoundingBoxStatsFilter** (SimplnxCore)
  - **Changes Made:** Converted the legacy @ref directive and the Compute Array Statistics / Compute Feature Bounding Boxes mentions to MyST links; explained the 1-to-many cell-to-box mapping; stated bounds are in physical coordinate units; fixed 'columns'->'tuples'; added Required Input Sources.

- [x] **ComputeVolumeFractionsFilter** (SimplnxCore)
  - **Changes Made:** Reconciled the count-vs-volume-fraction wording (equal-volume cells make them identical); stated output is a per-Ensemble dimensionless fraction in [0,1] summing to 1; added Required Input Sources (Cell Phases); removed the empty Example Pipelines.

- [x] **ComputeFeatureClusteringFilter** (SimplnxCore)
  - **Changes Made:** Defined RDF (normalized histogram of inter-centroid distances) and Ensemble; added when/why (clustering/ordering detection, >1 vs <1 interpretation); stated distances are physical length units; added Required Input Sources (Compute Feature Centroids + Phases).

- [x] **ComputeBoundaryElementFractionsFilter** (SimplnxCore)
  - **Changes Made:** Defined a 'surface element' (one whose Surface Elements input value > 0); stated output is a [0,1] fraction (boundary/total per Feature); added Required Input Sources (Feature Ids + Surface Elements); removed the empty Example Pipelines.

- [x] **ComputeEuclideanDistMapFilter** (SimplnxCore)
  - **Changes Made:** Defined Feature boundary/GB, triple line/TJ, quadruple point/QP; stated the Manhattan output is an integer cell count and the Euclidean output is a physical float distance; added Required Input Sources (Feature Ids).

- [x] **ComputeMomentInvariants2DFilter** (SimplnxCore)
  - **Changes Made:** Added a plain-language definition of 2D moment invariants and what high/low Omega-1/Omega-2 indicate; stated Omega values dimensionless and Z Delta in cells; fixed typos (appllication/particales/'et. al'); fixed the missing newline before the citations; added Required Input Sources (Feature Ids + Feature Rect).

- [x] **ComputeKMeansFilter** (SimplnxCore)
  - **Changes Made:** Defined cluster/centroid/Voronoi tessellation/within-cluster variance; fixed the broken 'Ensemble Attribute Matrix' bold and typos (randomnes, tesselation); added MyST links to Compute K Medoids/DBSCAN/Silhouette and Required Input Sources.

- [x] **ComputeKMedoidsFilter** (SimplnxCore)
  - **Changes Made:** Defined cluster/Voronoi/topology (kept the good medoid definition); fixed the broken bold and typos (randomnes, arbirtary); added MyST links to Compute K Means/DBSCAN/Silhouette and Required Input Sources.

- [x] **SilhouetteFilter** (SimplnxCore)
  - **Changes Made:** Converted the Doxygen math delimiters to MyST $$/$ math; stated the output array location and dimensionless [-1,1] range; added MyST links to Compute K Means/K Medoids and Required Input Sources.

- [x] **ApproximatePointCloudHullFilter** (SimplnxCore)
  - **Changes Made:** Fixed typos (of/or, hve, To/The, 'is able of', 'faster that'); added Parameter Guidance stating grid resolution is in the point cloud's coordinate units and the empty-neighbor threshold is a 0-26 count; added Required Input Sources.

- [x] **MapPointCloudToRegularGridFilter** (SimplnxCore)
  - **Changes Made:** Added a downstream link to Interpolate Point Cloud to Regular Grid; stated Manual-mode dimensions are voxel counts; fixed the uint64-max off-by-one and the 'User may to trace' grammar; added Required Input Sources.

- [x] **IterativeClosestPointFilter** (SimplnxCore)
  - **Changes Made:** Fixed the all-'1.' list to 1-6 and the trailing-period heading; stated the output is a row-major flattened 4x4 transform (translation in coordinate units), reported always but applied only when the apply-transform option is on; cross-linked Apply Transformation to Geometry; added Required Input Sources (two Vertex Geometries).

- [x] **IdentifyDuplicateVerticesFilter** (SimplnxCore)
  - **Changes Made:** Defined 'duplicate' as all-three-coordinate equality within machine epsilon (no user tolerance); reworded the 'visual explanation' claim (the example is text); converted the italic RemoveFlaggedVertices to a MyST link; fixed the 'this filters output' apostrophe.

- [x] **ComputeTriangleAreasFilter** (SimplnxCore)
  - **Changes Made:** Defined Triangle Geometry; added when/why (mesh quality, statistic weighting); stated output area units (length^2); added Required Input Sources.

- [x] **ComputeTriangleGeomCentroidsFilter** (SimplnxCore)
  - **Changes Made:** Defined Feature/Face Labels ('owners')/nodes; stated centroid coordinate units; documented the -1 exterior-label edge case; added Required Input Sources (Triangle Geometry + Face Labels); removed the empty Example Pipelines.

- [x] **ComputeTriangleGeomVolumesFilter** (SimplnxCore)
  - **Changes Made:** Defined Feature/Face Labels/winding; added a watertight-requirement note; stated volume units (length^3); added Required Input Sources (closed Triangle Geometry + Face Labels); removed the empty Example Pipelines.

- [x] **TriangleCentroidFilter** (SimplnxCore)
  - **Changes Made:** Added when/why; stated centroid coordinate units; cross-linked the per-Feature Compute Feature Centroids from Triangle Geometry to disambiguate (this is per-triangle); added Required Input Sources.

- [x] **TriangleNormalFilter** (SimplnxCore)
  - **Changes Made:** Stated the output is a dimensionless unit 3-vector whose direction depends on winding; defined normal/cross-product/winding; fixed 'vertexes'->'vertices'; added Required Input Sources.

- [x] **FeatureFaceCurvatureFilter** (SimplnxCore)
  - **Changes Made:** Removed the stray @image latex Doxygen line; fixed the missing-space bold; replaced the Google-Groups mailing-list footer with the standard DREAM3D-NX Help footer; converted the bold Parameter-Notes filter mentions to a MyST-linked Required Input Sources list; defined curvature; stated units (principal/mean 1/length, Gaussian 1/length^2, directions dimensionless).

- [x] **SharedFeatureFaceFilter** (SimplnxCore)
  - **Changes Made:** Added a prose explanation of the Randomize Face IDs parameter (visualization-only ID permutation); defined Feature/Face Labels/Feature Attribute Matrix; added Required Input Sources (Triangle Geometry + Face Labels).

- [x] **SurfaceNetsFilter** (SimplnxCore)
  - **Changes Made:** Fixed typos (mush/equivelent/Traingle/FaceLables); converted the Verify Triangle Winding and QuickMesh mentions to MyST links; defined isosurface/marching cubes/triple line/winding; stated units (iterations dimensionless, max distance physical length); added Required Input Sources (Image Geometry + Feature Ids); fixed the non-standard footer.

- [x] **LaplacianSmoothingFilter** (SimplnxCore)
  - **Changes Made:** Fixed the corrupted ref [1] author and the malformed Belyaev URL; removed the dangling 'visit the tutorial' line; added alt text to the equation images; cross-linked Hierarchical Smoothing; stated Iteration Steps as a count and lambda/mu as dimensionless; added Required Input Sources (Triangle Geometry + Node Type).

- [x] **UncertainRegularGridSampleSurfaceMeshFilter** (SimplnxCore)
  - **Changes Made:** Defined Cells/rectilinear grid/conformal mesh/Feature; stated uncertainty and resolution/origin values are physical lengths; cross-linked the non-uncertain sibling; added Required Input Sources (Triangle Geometry + Face Labels).

- [x] **ExtractInternalSurfacesFromTriangleGeometryFilter** (SimplnxCore)
  - **Changes Made:** Fixed the two unclosed '**Vertex' bolds; replaced legacy 'Data Container' with 'data group'; added the missing DREAM3D-NX Help footer; cross-linked the Node Type producers (Surface Nets / Quick Surface Mesh); added Required Input Sources.

- [x] **ReverseTriangleWindingFilter** (SimplnxCore)
  - **Changes Made:** Defined 'winding' (vertex ordering that sets normal direction); cross-linked Verify Triangle Winding and Compute Triangle Normals; added Required Input Sources; removed the empty Example Pipelines.

- [x] **VerifyTriangleWindingFilter** (SimplnxCore)
  - **Changes Made:** Converted all bold/italic filter mentions to MyST links; defined 'winding' and 'negative volume'; fixed typos (Traingle/addtionally/preform/untounched/'difference n output'/Zero's/convience); reconciled the group to 'Surface Meshing (Connectivity/Arrangement)'; added Required Input Sources.

- [x] **RemoveFlaggedEdgesFilter** (SimplnxCore)
  - **Changes Made:** Defined Edge Geometry and 'mask'; fixed the group to 'Surface Meshing (Cleanup)'; added Required Input Sources (mask producer); fixed the footer /discussions link.

- [x] **RemoveFlaggedTrianglesFilter** (SimplnxCore)
  - **Changes Made:** Fixed the Vertices->Triangles copy-paste error; defined Triangle Geometry/Face Data/mask; fixed the group to 'Surface Meshing (Cleanup)'; added Required Input Sources; corrected the example-pipeline name.

- [x] **CropVertexGeometryFilter** (SimplnxCore)
  - **Changes Made:** Expanded the thin doc; stated Min/Max are physical coordinates (not cell indices) with inclusive bounds; replaced the stale 'DREAM3D Review (Cropping/Cutting)' group with 'Geometry (Cropping/Cutting)'; cross-linked Crop Edge/Image Geometry; added Required Input Sources and the missing footer.

- [x] **CombineNodeBasedGeometriesFilter** (SimplnxCore)
  - **Changes Made:** Defined node-based and higher/lower-order geometry; clarified vertex-index renumbering and that duplicate vertices are NOT merged; set the group to 'Core (Combining)'; added Required Input Sources.

- [x] **PointSampleTriangleGeometryFilter** (SimplnxCore)
  - **Changes Made:** Added descriptive alt text to the equation PNGs (corrected to the actual barycentric sqrt form); fixed 'beloning'; removed a stale sample-count table that no longer matches the parameter; added a random-sampling/seed note; fixed the stale 'DREAM3D Review (Geometry)' group to 'Sampling (Geometry)'; added Required Input Sources.

- [x] **ExtractFeatureBoundaries2DFilter** (SimplnxCore)
  - **Changes Made:** Defined Feature IDs/Image Geometry/Edge Geometry/overscan; fixed '.d3dpipline'->'.d3dpipeline'; replaced the stale subgroup with 'Surface Meshing (Generation)'; converted bold filter mentions to MyST links; added image alt text and Required Input Sources.

- [x] **RobustAutomaticThresholdFilter** (SimplnxCore)
  - **Changes Made:** Fixed the stale group to 'Threshold'; stated the robust/automatic rationale up front; defined gradient magnitude / 2-norm; replaced the suspect a_i*g_i/g_i equation with a prose weighted-average description; replaced the nonexistent 'Find Derivatives' reference with the real gradient-magnitude producer (ITK Gradient Magnitude Image Filter) as a MyST link and Required Input Sources.

- [x] **ComputeVectorColorsFilter** (SimplnxCore)
  - **Changes Made:** Expanded the description: input is a 3-component float32 vector Cell array, output is uint8 RGB (0-255), each vector is normalized (direction only); documented the mask/bad-voxel behavior; added Required Input Sources.

### Tier 3 — Polish

| Filter | Plugin | Status |
|--------|--------|--------|
| ComputeArrayHistogram | SimplnxCore | Done |
| ComputeArrayHistogramByFeature | SimplnxCore | Done |
| ComputeDifferencesMap | SimplnxCore | Done |
| ComputeNumFeatures | SimplnxCore | Done |
| ComputeSurfaceAreaToVolume | SimplnxCore | Done |
| ComputeFeaturePhases | SimplnxCore | Done |
| ComputeFeaturePhasesBinary | SimplnxCore | Done |
| ComputeFeatureBounds | SimplnxCore | Done |
| ComputeFeatureCentroids | SimplnxCore | Done |
| ComputeFeatureRect | SimplnxCore | Done |
| ComputeLargestCrossSections | SimplnxCore | Done |
| ComputeCoordinateThreshold | SimplnxCore | Done |
| DBSCAN | SimplnxCore | Done |
| InterpolatePointCloudToRegularGrid | SimplnxCore | Done |
| ExtractVertexGeometry | SimplnxCore | Done |
| ComputeVertexToTriangleDistances | SimplnxCore | Done |
| LabelTriangleGeometry | SimplnxCore | Done |
| HierarchicalSmooth | SimplnxCore | Done |
| RegularGridSampleSurfaceMesh | SimplnxCore | Done |
| SliceTriangleGeometry | SimplnxCore | Done |
| RemoveFlaggedVertices | SimplnxCore | Done |
| CropEdgeGeometry | SimplnxCore | Done |
| CombineStlFiles | SimplnxCore | Done |
| MultiThresholdObjects | SimplnxCore | Done |
| CreateColorMap | SimplnxCore | Done |
| ChangeAngleRepresentation | SimplnxCore | Done |
| RandomizeFeatureIds | SimplnxCore | Done |
| ExecuteProcess | SimplnxCore | Done |
| CreatePythonSkeleton | SimplnxCore | Done |
| ExtractPipelineToFile | SimplnxCore | Done |
| CreateAMScanPaths | SimplnxCore | Done |

- [x] **ComputeArrayHistogramFilter** (SimplnxCore)
  - **Changes Made:** Trimmed the long raw 'Old Faithful' data dump; defined 'mode' on first use; stated bin bounds inherit the input array units; added Required Input Sources (None).

- [x] **ComputeArrayHistogramByFeatureFilter** (SimplnxCore)
  - **Changes Made:** Defined Feature/Feature Id; added Required Input Sources (Feature Ids producer); removed the empty Example Pipelines.

- [x] **ComputeDifferencesMapFilter** (SimplnxCore)
  - **Changes Made:** Stated the output inherits the input type/shape/location; added a brief Required Input Sources (None -- generic arrays); removed the empty Example Pipelines.

- [x] **ComputeNumFeaturesFilter** (SimplnxCore)
  - **Changes Made:** Defined Ensemble/phase; stated the output is a per-Ensemble count array; added Required Input Sources (Compute Feature Phases).

- [x] **ComputeSurfaceAreaToVolumeFilter** (SimplnxCore)
  - **Changes Made:** Defined 'aliasing'; stated the ratio units (1/length) and that sphericity is dimensionless; linked Compute Boundary Cells; added Required Input Sources (Feature Ids).

- [x] **ComputeFeaturePhasesFilter** (SimplnxCore)
  - **Changes Made:** Defined Feature/Ensemble/Element; stated the per-Feature phase output; added Required Input Sources (Feature Ids + Cell Phases).

- [x] **ComputeFeaturePhasesBinaryFilter** (SimplnxCore)
  - **Changes Made:** Added a generic non-materials framing; stated the boolean-mask input and binary Ensemble output; added Required Input Sources (Multi-Threshold Objects); removed the empty Example Pipelines.

- [x] **ComputeFeatureBoundsFilter** (SimplnxCore)
  - **Changes Made:** Stated bounds are physical coordinates (not cell indices); fixed the 'Dimesions' typo and the doubled ';;'; defined Feature/Cell; added Required Input Sources (Feature Ids).

- [x] **ComputeFeatureCentroidsFilter** (SimplnxCore)
  - **Changes Made:** Stated centroids are physical coordinates (3-component float32, not pixel indices); defined Feature/Cell; added Required Input Sources (Feature Ids).

- [x] **ComputeFeatureRectFilter** (SimplnxCore)
  - **Changes Made:** Fixed 'This values'->'These values'; clarified Pixel coords are zero-based voxel indices; cross-linked Compute Feature Bounding Boxes; added Required Input Sources.

- [x] **ComputeLargestCrossSectionsFilter** (SimplnxCore)
  - **Changes Made:** Stated the area is square physical units (cellCount x in-plane voxel area, confirmed from the cpp); described the per-feature float32 output; added Required Input Sources (Feature Ids).

- [x] **ComputeCoordinateThresholdFilter** (SimplnxCore)
  - **Changes Made:** Converted the prose Remove Flagged Vertices/Edges/Triangles mentions to MyST links; stated the rectangle min/max and sphere center/radius are physical coordinate units.

- [x] **DBSCANFilter** (SimplnxCore)
  - **Changes Made:** Converted eight bold/quoted inter-filter mentions to MyST links; fixed typos (sqaure_root, preforming/preform, 'vise versa'); added Required Input Sources.

- [x] **InterpolatePointCloudToRegularGridFilter** (SimplnxCore)
  - **Changes Made:** Converted the italic Map Point Cloud to Regular Grid to a MyST link; added Required Input Sources (Vertex Geometry producer).

- [x] **ExtractVertexGeometryFilter** (SimplnxCore)
  - **Changes Made:** Standardized the Rectilinear Grid / Vertex Geometry naming; added Required Input Sources.

- [x] **ComputeVertexToTriangleDistancesFilter** (SimplnxCore)
  - **Changes Made:** Fixed the 4x 'Geoemtry' typo; stated distance units (length unit); noted normals must be present/consistent for sign correctness; added Required Input Sources (Vertex + Triangle Geometry).

- [x] **LabelTriangleGeometryFilter** (SimplnxCore)
  - **Changes Made:** Added Required Input Sources with a note that STL is a common CAD mesh format; fixed the footer /discussions link.

- [x] **HierarchicalSmoothFilter** (SimplnxCore)
  - **Changes Made:** Removed the hand-written Parameters table (it duplicated the auto-table), replacing it with prose; added a MyST link to Laplacian Smoothing; added Required Input Sources; normalized the non-standard footer; removed the empty Example Pipelines.

- [x] **RegularGridSampleSurfaceMeshFilter** (SimplnxCore)
  - **Changes Made:** Stated Origin/Spacing physical units and Dimensions as voxel counts; cross-linked the Uncertain variant; added Required Input Sources (Triangle Geometry + Face Labels).

- [x] **SliceTriangleGeometryFilter** (SimplnxCore)
  - **Changes Made:** Fixed typos (geoemtry, bewteen, perimieter); stated Slice Spacing/range/area/perimeter are in the geometry's physical units; added Required Input Sources.

- [x] **RemoveFlaggedVerticesFilter** (SimplnxCore)
  - **Changes Made:** Replaced legacy 'Data Container' wording with NX terminology; added Required Input Sources (the boolean-mask producer).

- [x] **CropEdgeGeometryFilter** (SimplnxCore)
  - **Changes Made:** Added the missing Group header (Core (Conversion)); stated Min/Max are physical units, not indices; removed the empty Example Pipelines; cross-linked Crop Geometry (Image).

- [x] **CombineStlFilesFilter** (SimplnxCore)
  - **Changes Made:** Defined STL on first use; converted the bold Import/Write STL mentions to MyST links.

- [x] **MultiThresholdObjectsFilter** (SimplnxCore)
  - **Changes Made:** Defined Cell/Attribute Matrix/EBSD context; added Required Input Sources naming EBSD readers as typical producers; removed the empty Example Pipelines.

- [x] **CreateColorMapFilter** (SimplnxCore)
  - **Changes Made:** Added the missing Group header (Core (Image)); added Required Input Sources (scalar array + optional mask); noted the preset table is hand-maintained; removed the empty Example Pipelines; fixed the License line.

- [x] **ChangeAngleRepresentationFilter** (SimplnxCore)
  - **Changes Made:** Added a Required Input Sources one-liner naming EBSD readers as typical angle producers (units content already exemplary).

- [x] **RandomizeFeatureIdsFilter** (SimplnxCore)
  - **Changes Made:** Defined Feature Ids/Attribute Matrix/NeighborList; documented that the cpp exposes no seed parameter (not reproducible); added Required Input Sources; fixed the License line.

- [x] **ExecuteProcessFilter** (SimplnxCore)
  - **Changes Made:** Stated the absolute-path requirement; documented the Should Block / Timeout (ms) parameters; added an explicit Required Input Sources: None.

- [x] **CreatePythonSkeletonFilter** (SimplnxCore)
  - **Changes Made:** Removed the hand-written Configuration parameter list and ADDED the missing auto-table marker; added the missing Group header (Core (Generation)); added Required Input Sources: None.

- [x] **ExtractPipelineToFileFilter** (SimplnxCore)
  - **Changes Made:** Added the missing Group header (IO (Output)) and the missing DREAM3D-NX Help footer; noted the no-embedded-pipeline behavior; added Required Input Sources: None; fixed the License line.

- [x] **CreateAMScanPathsFilter** (SimplnxCore)
  - **Changes Made:** Added a Units section (Hatch Spacing/Length in geometry coordinate units, Hatch Rotation Angle in degrees per the cpp label); converted the quoted SliceTriangleGeometry to a MyST link and promoted it to Required Input Sources.

### Tier 4 — Adequate

| Filter | Plugin | Status |
|--------|--------|--------|
| ComputeFeatureSizes | SimplnxCore | Done |
| ComputeBoundaryCells | SimplnxCore | Done |
| ComputeGroupingDensity | SimplnxCore | Done |

- [x] **ComputeFeatureSizesFilter** (SimplnxCore)
  - **Changes Made:** Added Required Input Sources (Feature Ids from a segmentation filter). Otherwise adequate as triaged.

- [x] **ComputeBoundaryCellsFilter** (SimplnxCore)
  - **Changes Made:** Stated the output range 0-6 and volume-edge handling; added Required Input Sources (Feature Ids). Otherwise adequate.

- [x] **ComputeGroupingDensityFilter** (SimplnxCore)
  - **Changes Made:** Converted the quoted 'Compute Feature Neighborhoods' to a MyST link (and fixed the dangling quote); added Required Input Sources (Parent IDs, neighbor lists, Feature Volumes). Otherwise model-quality.

---

## Real-World Visualization Wishlist

These are pipeline-generated screenshots/visualizations that would enhance the documentation but require DREAM3DNX automation tooling (future work).

| Filter | Desired Visualization | Priority |
|--------|----------------------|----------|
| ComputeAvgCAxes | IPF color map showing C-axis orientations per grain | Medium |
| ComputeCAxisLocations | C-axis direction map colored by orientation | Medium |
| ComputeGBCD | GBCD pole figure output from Write GBCD Pole Figure filter | High |
| ComputeSchmids | Microstructure colored by Schmid factor under a loading direction | High |
| ComputeKernelAvgMisorientations | KAM map showing orientation gradients within grains | High |
| ComputeAvgOrientations | IPF color map showing average orientation assignment per grain | Medium |
| ComputeFeatureReferenceMisorientations | Color scale legend for existing images; additional loading conditions | Low |
| ComputeShapes | Features colored by aspect ratio or Omega3 value | Medium |
| ComputeTwinBoundaries | Microstructure with twin boundaries highlighted and incoherence heat map | Medium |
| ComputeSlipTransmissionMetrics | Feature pairs with high vs low transmission metrics visualized | Medium |
| ComputeBoundaryStrengths | Boundary mesh colored by M' or fip values | Medium |
| AlignSectionsFeatureCentroid | Before/after comparison showing when centroid alignment is appropriate vs when it fails (e.g., sample with consistent vs inconsistent mask regions) | High |
| AlignSectionsMisorientation | Before/after slice alignment showing corrected vs uncorrected sections; side-by-side comparison of alignment with mask vs without mask showing when masking is needed (e.g., sample with mounting material or voids at edges) | High |
| AlignSectionsMutualInformation | Before/after alignment comparison; side-by-side with misorientation method | Medium |
| AlignSectionsList | Typical workflow showing shift output from another alignment filter used as input | Low |
| ITKNormalizeImage | Before/after images showing normalization effect with parameters used to generate result | High |
| ITKRescaleIntensityImage | Before/after images showing rescaling effect with parameters used to generate result | High |
| ScalarSegmentFeatures | Before/after FeatureIds map from a scalar-based segmentation (e.g., image-quality thresholding) | Medium |
| CAxisSegmentFeatures | Before/after segmentation on hexagonal (e.g., Ti) EBSD data | Medium |
| EBSDSegmentFeatures | EBSD IPF map → segmented grains visualization (flagship EBSD workflow) | High |
| MergeTwins | Before/after microstructure showing twin variants merged into parent grain | High |
| IdentifySample | Before/after showing overscan cleanup on a real FIB-SEM dataset | Low |
| RequireMinimumSizeFeatures | Before/after microstructure with minimum-size filter applied | Medium |
| RemoveFlaggedFeatures | Side-by-side comparison of Remove vs Extract vs Extract-then-Remove operation modes | Medium |
| ComputeFeatureNeighbors | Grain map colored by number of neighbors | Medium |
| AddBadData | Synthetic structure before and after random noise; same with boundary noise | High |
| FillBadData | Conceptual diagram: small defects (filled) vs large defects (preserved) at threshold | Medium |
| ErodeDilateBadData | Multi-frame iteration sequence diagram (iter 0 → 3) for both erode and dilate | High |
| ErodeDilateBadData | Erode-then-dilate ("opening") removing isolated single-cell noise but preserving a larger pore | Medium |
| ErodeDilateMask | Directional erosion (X-only vs uniform) on a mask | Medium |
| ErodeDilateCoordinationNumber | 3x3x3 cell cluster diagram with coordination numbers labeled (CN=6 isolated, CN=3 edge, etc.) | High |
| ReplaceElementAttributesWithNeighborValues | Conceptual diagram: flagged cell + neighbors with chosen replacement highlighted by max/min | Medium |
| NeighborOrientationCorrelation | Cell-with-6-neighbors diagram showing Cleanup Level=2,4,6 outcomes side by side | High |
| BadDataNeighborOrientationCheck | Voxel-and-neighbors diagram showing the false→true flip when criterion is met | Medium |
| RequireMinNumNeighbors | Microstructure before/after with isolated low-neighbor-count grains absorbed | Medium |
| ComputeNeighborhoods | Histogram of neighborhood counts for a real microstructure | Low |
| ComputeNeighborListStatistics | Example feature with its neighbor-misorientation list and computed statistics | Low |
| CreateGeometry | Side-by-side infographic of all 8 geometry types (Image, RectGrid, Vertex, Edge, Triangle, Quad, Tet, Hex) | High |
| CreateImageGeometry / SetImageGeomOriginScaling | Image Geometry dimensions/origin/spacing diagram (shared concept) | High |
| ApplyTransformationToGeometry | Axis-angle representation diagram | Medium |
| RotateSampleRefFrame | Sample frame rotation vs geometric rotation comparison | High |
| ResampleImageGeom | Before/after voxel grid showing 2x spacing reduction halving cell count | Medium |
| ResampleRectGridToImageGeom | Variable-spacing RectGrid → uniform Image Geom with "last one wins" cells highlighted | Medium |
| CropImageGeometry | Already adequate | - |
| PadImageGeometry | 3-panel diagram showing Update Origin ON vs OFF | Medium |
| AppendImageGeometry | Already comprehensive | - |
| QuickSurfaceMesh | Single voxel face → 2 triangles diagram | Low |
| PartitionGeometry | Already excellent | - |
| InitializeImageGeomCellData | Subvolume highlight before/after on real cube | Medium |
| CombineTransformationMatrices | Two 4x4 matrices multiplying into a single result | Low |
| ComputeCoordinatesImageGeom | Voxel grid with one cell highlighted showing (i,j,k) and (x,y,z) | Low |
| ReadImageStack | N 2D slices stacking into a 3D volume (Z = slice count) | Medium |
| ReadHDF5Dataset | Flat HDF5 dataset (N elements) reshaped into tuples x components | Medium |
| ReadStlFile | One triangle (normal + 3 vertices); many triangles forming a surface mesh | Low |
| ReadVtkStructuredPoints | Voxel cube showing cell-center value vs 8 corner (point) values | Medium |
| ReadH5Ebsd / ReadAng / ReadCtf | Sample vs crystal reference-frame diagram (why a rotation is needed); IPF map of freshly imported data | High |
| WriteAbaqusHexahedron | Image-geometry voxel grid mapping to a hexahedral element mesh (8 nodes per voxel) | Medium |
| WriteGBCDGMTFile | Data-flow: Compute GBCD -> Write GBCD GMT File -> .dat -> GMT -> rendered pole figure | High |
| WriteGBCDTriangleData | Boundary triangle: two grains, left/right average orientations, normal vector, area | Medium |
| WritePoleFigure | Stereographic/Lambert projection: 3D orientation/pole projected onto the 2D unit circle | High |
| WriteVtkRectilinearGrid / WriteVtkStructuredPoints | Rectilinear grid (variable spacing) vs structured points (uniform spacing) side by side | Medium |
| ReadDREAM3D | Import-data tree selector UI showing partial-import checkboxes | Medium |

---

## Concept Pages Needed (Future Work)

Domain concepts that appear across 3+ filters and should eventually become shared reference pages:

| Concept | Filters That Need It |
|---------|---------------------|
| C-Axis / Hexagonal Crystals | ComputeAvgCAxes, ComputeCAxisLocations, ComputeFeatureNeighborCAxisMisalignments, ComputeFeatureReferenceCAxisMisorientations |
| Quaternions & Orientation Representations | ComputeAvgCAxes, ComputeAvgOrientations, ComputeCAxisLocations, ConvertOrientations |
| Misorientation | ComputeKernelAvgMisorientations, ComputeFeatureNeighborMisorientations, ComputeFeatureReferenceMisorientations, ComputeGBCD |
| Slip Systems & Crystal Plasticity | ComputeSchmids, ComputeSlipTransmissionMetrics, ComputeBoundaryStrengths |
| Grain Boundaries | ComputeGBCD, ComputeGBCDMetricBased, ComputeGBPDMetricBased, ComputeTwinBoundaries |
| Feature IDs & Grains | Nearly all filters |
| Reference Frames (Sample vs Crystal) | ComputeAvgCAxes, ComputeCAxisLocations, ComputeSchmids |
| Burn Algorithm / Segmentation | ScalarSegmentFeatures, CAxisSegmentFeatures, EBSDSegmentFeatures |
| Feature Cleanup / Isotropic Coarsening | RequireMinimumSizeFeatures, RemoveFlaggedFeatures |
| Cell vs Feature vs Ensemble Data | ScalarSegmentFeatures, RequireMinimumSizeFeatures, ComputeFeatureNeighbors, and most feature-level filters |
| Morphological Erosion / Dilation / Opening | ErodeDilateBadData, ErodeDilateMask, ErodeDilateCoordinationNumber |
| Coordination Number / Face Neighbors | ErodeDilateCoordinationNumber, NeighborOrientationCorrelation, BadDataNeighborOrientationCheck |
| Equivalent Sphere Diameter | ComputeFeatureSizes, ComputeNeighborhoods, ComputeShapes |
| Geometry Types (Image, RectGrid, Vertex, Edge, Triangle, Quad, Tet, Hex) | CreateGeometry, CreateImageGeometry, and every geometry-aware filter |
| Transformation Matrices (4x4 row-major) | ApplyTransformationToGeometry, CombineTransformationMatrices, RotateSampleRefFrame |
| Sample Reference Frame vs Crystal Reference Frame | RotateSampleRefFrame (already listed above) |
| HDF5 File Format (datasets, groups, tuples vs components) | ReadHDF5Dataset, ReadDREAM3D, WriteDREAM3D, ReadH5Ebsd, ReadH5Esprit, ReadH5Oim, ReadH5Oina, ReadGrainMapper3D |
| VTK Legacy Formats (structured points vs rectilinear grid; ASCII vs binary) | ReadVtkStructuredPoints, WriteVtkStructuredPoints, WriteVtkRectilinearGrid |
| EBSD Vendor File Formats & What They Contain (Euler angles, CI/IQ/MAD, phases) | ReadAngData, ReadCtfData, ReadChannel5Data, ReadH5Ebsd, ReadH5EspritData, ReadH5OimData, ReadH5OinaData |
| External Simulation Exporters (FEA / MD / KMC / FFT input files) | WriteAbaqusHexahedron, WriteLAMMPSFile, WriteSPParksSites, WriteLosAlamosFFT, ReadDeformKeyFileV12 |
| Pole Figures & Projections (stereographic, Lambert, Laue class) | WritePoleFigure, WriteGBCDGMTFile, ComputeGBCD |
