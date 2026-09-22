# Powder Particle Watershed Segmentation

Executable pipeline: `(08) Powder Particle Watershed Segmentation.d3dpipeline`

> **Validation warning:** Do not treat this example as a validated acceptance procedure. Adapt and validate it for the scanner, material, resolution, and quality requirement.

## Purpose and real-world setting

A powder laboratory has a microscopy image in which neighboring particles touch. A simple threshold would merge them, so the analyst creates stable internal markers and grows one watershed region from each marker.

Uses marker-controlled watershed to separate touching powder particles after Gaussian smoothing, gradient calculation, and shallow-minimum suppression.

## When to use this workflow

- Separate touching particles before size and shape analysis
- Reduce watershed over-segmentation with H-minima suppression
- Create labeled particle boundaries for visual review


## Input data and assumptions

- Particle boundaries create a usable gradient ridge.
- Each retained regional minimum corresponds to one intended particle seed.
- The example uses 2-D processing and does not infer hidden 3-D particle shape.

<!-- INPUT-DATA-STRATEGY:BEGIN -->
### Input source strategy

Strategy: `synthetic-paper-inspired`.

Paper visual target: Metal-powder XCT or microscopy views with irregular particles, a broad size distribution, edge shading, and touching clusters.

Target traits:

- irregular elliptical particles
- broad diameter distribution
- touching particles
- bright rims
- dark background

Generator: `Generators/GenerateImageProcessingExampleData.py` with fixed seed `20260901`.

Generated metrics: `PaperInputMetrics.json`. Target contract: `Generators/PaperInputTargets.json`.

Known differences: The paper's powder volume is unavailable. The generated image is a 2-D analogue and cannot reproduce 3-D sphericity or internal particle porosity.

See `Data/ImageProcessing_Examples/Provenance.json` for per-file transformations, checksums, dimensions, spacing, and licensing.
<!-- INPUT-DATA-STRATEGY:END -->

## Data flow

Particle image -> Gaussian smoothing -> gradient surface -> H-minima suppression -> regional-minimum mask -> connected marker labels -> marker-controlled watershed -> contours -> DREAM3D file.

The `.d3dpipeline` file is authoritative for executable arguments. The matching `.yaml` file repeats the ordered steps in a structured form for software, LLM, and MCP use.

## Step-by-step filter explanation and parameter rationale

### 1. `ReadImageFilter`

Import the source image and preserve its original pixel values.

Spacing and unit settings control the physical meaning of distance, size, and position.

Key parameters:

- `cell_attribute_matrix_name`: `"Cell Data"`
- `center_origin`: `false`
- `change_image_data_type`: `false`
- `change_origin`: `false`
- `change_spacing`: `false`
- `cropping_options`: `{"bounds_x_physical":[0.0,0.0],"bounds_x_voxels":[0,0],"bounds_y_physical":[0.0,0.0],"bounds_y_voxels":[0,0],"bounds_z_physical":[0.0,0.0],"bounds_z_voxels":[0,0],"crop_x":true,...`
- `file_name`: `"Data/ImageProcessing_Examples/materials/powder_particles.png"`
- `image_data_array_name`: `"Particle Intensity"`
- `image_data_type_index`: `0`
- `length_unit_index`: `6`
- `origin`: `[0.0,0.0,0.0]`
- `origin_spacing_processing_index`: `1`
- `output_geometry_path`: `"Powder Particles"`
- `spacing`: `[1.0,1.0,1.0]`

Resolved inputs:

- `file_name` -> `"Data/ImageProcessing_Examples/materials/powder_particles.png"`

Declared outputs:

- `output_geometry_path` -> `"Powder Particles"`

### 2. `DiscreteGaussianImageFilter`

Smooth small intensity variations before edge detection.

Scale parameters control which spatial structures are treated as noise, background, or a retained feature. Spacing and unit settings control the physical meaning of distance, size, and position.

Key parameters:

- `input_image_data_path`: `"Powder Particles/Cell Data/Particle Intensity"`
- `input_image_geometry_path`: `"Powder Particles"`
- `maximum_error`: `[0.01,0.01,0.01]`
- `maximum_kernel_width`: `16`
- `output_array_name`: `"Smoothed Particles"`
- `use_image_spacing`: `true`
- `variance`: `[1.0,1.0,0.0]`

Resolved inputs:

- `input_image_data_path` -> `"Powder Particles/Cell Data/Particle Intensity"`
- `input_image_geometry_path` -> `"Powder Particles"`

Declared outputs:

- `output_array_name` -> `"Smoothed Particles"`

### 3. `GradientMagnitudeRecursiveGaussianImageFilter`

Create a smooth edge-strength surface for watershed segmentation.

Scale parameters control which spatial structures are treated as noise, background, or a retained feature.

Key parameters:

- `input_image_data_path`: `"Powder Particles/Cell Data/Smoothed Particles"`
- `input_image_geometry_path`: `"Powder Particles"`
- `normalize_across_scale`: `false`
- `output_array_name`: `"Particle Gradient"`
- `sigma`: `1.0`

Resolved inputs:

- `input_image_data_path` -> `"Powder Particles/Cell Data/Smoothed Particles"`
- `input_image_geometry_path` -> `"Powder Particles"`

Declared outputs:

- `output_array_name` -> `"Particle Gradient"`

### 4. `HMinimaImageFilter`

Suppress shallow minima that would over-segment powder particles.

Threshold and extrema values are tuned to the included fixture and must be recalibrated for a different material or sensor. Topology and foreground conventions must match the mask and the intended definition of object contact.

Key parameters:

- `fully_connected`: `true`
- `height`: `5.0`
- `input_image_data_path`: `"Powder Particles/Cell Data/Particle Gradient"`
- `input_image_geometry_path`: `"Powder Particles"`
- `output_array_name`: `"Suppressed Minima"`

Resolved inputs:

- `input_image_data_path` -> `"Powder Particles/Cell Data/Particle Gradient"`
- `input_image_geometry_path` -> `"Powder Particles"`

Declared outputs:

- `output_array_name` -> `"Suppressed Minima"`

### 5. `RegionalMinimaImageFilter`

Find the remaining regional minima as particle markers.

Topology and foreground conventions must match the mask and the intended definition of object contact.

Key parameters:

- `background_value`: `0.0`
- `flat_is_minima`: `true`
- `foreground_value`: `1.0`
- `fully_connected`: `true`
- `input_image_data_path`: `"Powder Particles/Cell Data/Suppressed Minima"`
- `input_image_geometry_path`: `"Powder Particles"`
- `output_array_name`: `"Marker Mask"`

Resolved inputs:

- `input_image_data_path` -> `"Powder Particles/Cell Data/Suppressed Minima"`
- `input_image_geometry_path` -> `"Powder Particles"`

Declared outputs:

- `output_array_name` -> `"Marker Mask"`

### 6. `ConnectedComponentImageFilter`

Assign a unique label to each watershed marker.

Topology and foreground conventions must match the mask and the intended definition of object contact.

Key parameters:

- `fully_connected`: `true`
- `input_image_data_path`: `"Powder Particles/Cell Data/Marker Mask"`
- `input_image_geometry_path`: `"Powder Particles"`
- `output_array_name`: `"Marker Labels"`

Resolved inputs:

- `input_image_data_path` -> `"Powder Particles/Cell Data/Marker Mask"`
- `input_image_geometry_path` -> `"Powder Particles"`

Declared outputs:

- `output_array_name` -> `"Marker Labels"`

### 7. `MorphologicalWatershedFromMarkersImageFilter`

Grow the particle markers across the gradient surface without merging touching particles.

Topology and foreground conventions must match the mask and the intended definition of object contact.

Key parameters:

- `fully_connected`: `true`
- `input_image_data_path`: `"Powder Particles/Cell Data/Particle Gradient"`
- `input_image_geometry_path`: `"Powder Particles"`
- `mark_watershed_line`: `true`
- `marker_image_data_path`: `"Powder Particles/Cell Data/Marker Labels"`
- `output_array_name`: `"Particle Labels"`

Resolved inputs:

- `input_image_data_path` -> `"Powder Particles/Cell Data/Particle Gradient"`
- `input_image_geometry_path` -> `"Powder Particles"`

Declared outputs:

- `output_array_name` -> `"Particle Labels"`

### 8. `LabelContourImageFilter`

Extract the final particle boundaries.

Topology and foreground conventions must match the mask and the intended definition of object contact.

Key parameters:

- `background_value`: `0.0`
- `fully_connected`: `true`
- `input_image_data_path`: `"Powder Particles/Cell Data/Particle Labels"`
- `input_image_geometry_path`: `"Powder Particles"`
- `output_array_name`: `"Particle Boundaries"`

Resolved inputs:

- `input_image_data_path` -> `"Powder Particles/Cell Data/Particle Labels"`
- `input_image_geometry_path` -> `"Powder Particles"`

Declared outputs:

- `output_array_name` -> `"Particle Boundaries"`

### 9. `WriteDREAM3DFilter`

Save the separated powder-particle labels and intermediate images.

The selected values preserve the demonstrated input domain and create a named result that later steps can address explicitly.

Key parameters:

- `compression_level`: `5`
- `export_file_path`: `"Data/Output/ImageProcessing_Examples/04_Powder_Watershed/powder_particles.dream3d"`
- `use_compression`: `true`
- `write_xdmf_file`: `false`

Declared outputs:

- `export_file_path` -> `"Data/Output/ImageProcessing_Examples/04_Powder_Watershed/powder_particles.dream3d"`

## Outputs and how to interpret them

- `Data/Output/ImageProcessing_Examples/04_Powder_Particles/powder_particles.dream3d`

Intermediate arrays remain in the final DREAM3D file. Compare them in order. A plausible final mask does not prove that every upstream assumption is correct.

## How to adapt the pipeline

- Set Gaussian scale from image noise and particle-edge width.
- Raise H-minima height to merge weak internal minima; lower it to preserve close particle seeds.
- Validate fully connected topology for the image sampling grid.

Keep input and output paths relative when the pipeline must run from a DREAM3D-NX Anaconda environment or an installed application bundle.

## Failure modes and quality checks

- Too many markers split one particle into several labels.
- Too few markers merge touching particles.
- Low-contrast boundaries let watershed regions leak.
- Review intermediate arrays, not only the final writer output.
- Compare measurements with an independent reference before production use.

## Scientific basis and annotated references

Workflow reproduction status: `method_reproduction`.

The paper's powder volume is unavailable. The generated image is a 2-D analogue and cannot reproduce 3-D sphericity or internal particle porosity.

Paper dataset availability: The original paper input is unavailable or not redistributable. A deterministic paper-inspired synthetic input is included.

Dataset license: No separate dataset license is stated.

Original or paper-linked dataset included in the example archive: `no`.

Paper-inspired synthetic fallback included in the example archive: `yes`.

1. Anton du Plessis, Philip Sperling, Andre Beerlink, Werner du Preez, Stephan G. le Roux. “Standard method for microCT-based additive manufacturing quality control 4: Metal powder analysis.” *MethodsX* (2018). [10.1016/j.mex.2018.10.021](https://doi.org/10.1016/j.mex.2018.10.021). Primary industrial workflow for image-based metal-powder particle characterization.
2. Luc Vincent, Pierre Soille. “Watersheds in digital spaces: An efficient algorithm based on immersion simulations.” *IEEE Transactions on Pattern Analysis and Machine Intelligence* (1991). [10.1109/34.87344](https://doi.org/10.1109/34.87344). Foundational watershed segmentation method used to separate basins and touching objects.
3. Luc Vincent. “Morphological grayscale reconstruction in image analysis: Applications and efficient algorithms.” *IEEE Transactions on Image Processing* (1993). [10.1109/83.217222](https://doi.org/10.1109/83.217222). Foundational reference for reconstruction, extrema suppression, and top-hat morphology.

## Guidance for an LLM or MCP assistant

Use this document to explain intent and tradeoffs. Use the `.yaml` sidecar to retrieve exact structured metadata. Use the `.d3dpipeline` file as the executable source of truth, and use the `.py` companion for Anaconda execution.

Before adapting this workflow, ask:

- What imaging system, material, and acquisition mode produced the data?
- What are the voxel or pixel spacing and physical units?
- What is the smallest feature that must be retained or reported?
- Which independent measurement or labeled dataset will validate the result?

Constraints:

- Particle boundaries create a usable gradient ridge.
- Each retained regional minimum corresponds to one intended particle seed.
- The example uses 2-D processing and does not infer hidden 3-D particle shape.
- Keep all file paths relative to the DREAM3D-NX application binary directory.
- Run preflight and execute the complete pipeline after any parameter or DataPath change.
- Treat example parameter values as starting points, not production acceptance limits.

Related examples:

- `(09) Microstructure Watershed Segmentation.d3dpipeline`

When a user asks for a new pipeline, preserve DataPath consistency between steps, validate every mathematical domain, and run the result end to end with representative data.
