# Microstructure Watershed Segmentation

Executable pipeline: `(09) Microstructure Watershed Segmentation.d3dpipeline`

> **Validation warning:** Do not treat this example as a validated acceptance procedure. Adapt and validate it for the scanner, material, resolution, and quality requirement.

## Purpose and real-world setting

A metallographer needs an exploratory segmentation of grains or phases in a grayscale micrograph. The pipeline keeps multiple extrema products so the analyst can understand why the watershed splits or merges regions.

Builds an edge-preserving gradient surface, explores regional extrema, and performs unseeded morphological watershed segmentation of a grayscale microstructure.

## When to use this workflow

- Explore watershed segmentation of a grayscale microstructure
- Compare binary and valued regional extrema
- Tune extrema suppression before basin segmentation


## Input data and assumptions

- Meaningful boundaries correspond to gradient ridges.
- The image has enough contrast after edge-preserving denoise.
- Unseeded watershed is exploratory and can over-segment noisy data.

<!-- INPUT-DATA-STRATEGY:BEGIN -->
### Input source strategy

Strategy: `original-paper-linked`.

Paper visual target: Paper-linked bimodal Ti-6Al-4V micrograph with globular regions, lath colonies, and bright phase boundaries.

Target traits:

- measured micrograph
- bimodal Ti-6Al-4V
- globular regions
- lath colonies
- bright boundaries

Original source: [https://raw.githubusercontent.com/ArunBaskaran/Image-Driven-Machine-Learning-Approach-for-Microstructure-Classification-and-Segmentation-Ti-6Al-4V/master/Images1/image_500.png](https://raw.githubusercontent.com/ArunBaskaran/Image-Driven-Machine-Learning-Approach-for-Microstructure-Classification-and-Segmentation-Ti-6Al-4V/master/Images1/image_500.png).

Original source SHA-512: `6739da11d8d4098ef0f7086b521e9a75a55936b53ce198ac4caf1c802ca1af8a23bb96461e28213a1b6574ad0ab6807775113aeffc90da2eccfc2f7498d7412c`.

Known differences: The input is original paper-linked data, but the pipeline omits the paper's boundary-class neural network and therefore cannot reproduce HADMA accuracy.

See `Data/ImageProcessing_Examples/Provenance.json` for per-file transformations, checksums, dimensions, spacing, and licensing.
<!-- INPUT-DATA-STRATEGY:END -->

## Data flow

Micrograph -> normalized intensity -> anisotropic diffusion -> gradient magnitude -> maxima and minima diagnostics -> morphological watershed labels -> DREAM3D file.

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
- `file_name`: `"Data/ImageProcessing_Examples/materials/microstructure_grayscale.png"`
- `image_data_array_name`: `"Microstructure Intensity"`
- `image_data_type_index`: `0`
- `length_unit_index`: `6`
- `origin`: `[0.0,0.0,0.0]`
- `origin_spacing_processing_index`: `1`
- `output_geometry_path`: `"Microstructure"`
- `spacing`: `[1.0,1.0,1.0]`

Resolved inputs:

- `file_name` -> `"Data/ImageProcessing_Examples/materials/microstructure_grayscale.png"`

Declared outputs:

- `output_geometry_path` -> `"Microstructure"`

### 2. `NormalizeImageFilter`

Convert the grayscale microstructure to a normalized floating-point field.

The selected values preserve the demonstrated input domain and create a named result that later steps can address explicitly.

Key parameters:

- `input_image_data_path`: `"Microstructure/Cell Data/Microstructure Intensity"`
- `input_image_geometry_path`: `"Microstructure"`
- `output_array_name`: `"Normalized Microstructure"`

Resolved inputs:

- `input_image_data_path` -> `"Microstructure/Cell Data/Microstructure Intensity"`
- `input_image_geometry_path` -> `"Microstructure"`

Declared outputs:

- `output_array_name` -> `"Normalized Microstructure"`

### 3. `GradientAnisotropicDiffusionImageFilter`

Reduce noise while preserving microstructure boundaries.

Diffusion settings balance noise reduction against boundary preservation and numerical stability.

Key parameters:

- `conductance_parameter`: `3.0`
- `conductance_scaling_update_interval`: `1`
- `input_image_data_path`: `"Microstructure/Cell Data/Normalized Microstructure"`
- `input_image_geometry_path`: `"Microstructure"`
- `number_of_iterations`: `3`
- `output_array_name`: `"Diffusion Smoothed"`
- `time_step`: `0.05`

Resolved inputs:

- `input_image_data_path` -> `"Microstructure/Cell Data/Normalized Microstructure"`
- `input_image_geometry_path` -> `"Microstructure"`

Declared outputs:

- `output_array_name` -> `"Diffusion Smoothed"`

### 4. `GradientMagnitudeImageFilter`

Measure local intensity changes at microstructure boundaries.

Spacing and unit settings control the physical meaning of distance, size, and position.

Key parameters:

- `input_image_data_path`: `"Microstructure/Cell Data/Diffusion Smoothed"`
- `input_image_geometry_path`: `"Microstructure"`
- `output_array_name`: `"Boundary Strength"`
- `use_image_spacing`: `true`

Resolved inputs:

- `input_image_data_path` -> `"Microstructure/Cell Data/Diffusion Smoothed"`
- `input_image_geometry_path` -> `"Microstructure"`

Declared outputs:

- `output_array_name` -> `"Boundary Strength"`

### 5. `HMaximaImageFilter`

Suppress weak peaks and retain significant bright regions.

Threshold and extrema values are tuned to the included fixture and must be recalibrated for a different material or sensor.

Key parameters:

- `height`: `0.5`
- `input_image_data_path`: `"Microstructure/Cell Data/Diffusion Smoothed"`
- `input_image_geometry_path`: `"Microstructure"`
- `output_array_name`: `"Prominent Peaks"`

Resolved inputs:

- `input_image_data_path` -> `"Microstructure/Cell Data/Diffusion Smoothed"`
- `input_image_geometry_path` -> `"Microstructure"`

Declared outputs:

- `output_array_name` -> `"Prominent Peaks"`

### 6. `RegionalMaximaImageFilter`

Create a binary map of regional intensity maxima.

Topology and foreground conventions must match the mask and the intended definition of object contact.

Key parameters:

- `background_value`: `0.0`
- `flat_is_maxima`: `true`
- `foreground_value`: `1.0`
- `fully_connected`: `true`
- `input_image_data_path`: `"Microstructure/Cell Data/Diffusion Smoothed"`
- `input_image_geometry_path`: `"Microstructure"`
- `output_array_name`: `"Regional Maxima"`

Resolved inputs:

- `input_image_data_path` -> `"Microstructure/Cell Data/Diffusion Smoothed"`
- `input_image_geometry_path` -> `"Microstructure"`

Declared outputs:

- `output_array_name` -> `"Regional Maxima"`

### 7. `ValuedRegionalMaximaImageFilter`

Retain the original intensity at each regional maximum.

Topology and foreground conventions must match the mask and the intended definition of object contact.

Key parameters:

- `fully_connected`: `true`
- `input_image_data_path`: `"Microstructure/Cell Data/Diffusion Smoothed"`
- `input_image_geometry_path`: `"Microstructure"`
- `output_array_name`: `"Valued Maxima"`

Resolved inputs:

- `input_image_data_path` -> `"Microstructure/Cell Data/Diffusion Smoothed"`
- `input_image_geometry_path` -> `"Microstructure"`

Declared outputs:

- `output_array_name` -> `"Valued Maxima"`

### 8. `ValuedRegionalMinimaImageFilter`

Retain the original intensity at each regional minimum.

Topology and foreground conventions must match the mask and the intended definition of object contact.

Key parameters:

- `fully_connected`: `true`
- `input_image_data_path`: `"Microstructure/Cell Data/Diffusion Smoothed"`
- `input_image_geometry_path`: `"Microstructure"`
- `output_array_name`: `"Valued Minima"`

Resolved inputs:

- `input_image_data_path` -> `"Microstructure/Cell Data/Diffusion Smoothed"`
- `input_image_geometry_path` -> `"Microstructure"`

Declared outputs:

- `output_array_name` -> `"Valued Minima"`

### 9. `MorphologicalWatershedImageFilter`

Segment the microstructure from the boundary-strength surface.

Threshold and extrema values are tuned to the included fixture and must be recalibrated for a different material or sensor. Topology and foreground conventions must match the mask and the intended definition of object contact.

Key parameters:

- `fully_connected`: `true`
- `input_image_data_path`: `"Microstructure/Cell Data/Boundary Strength"`
- `input_image_geometry_path`: `"Microstructure"`
- `level`: `0.3`
- `mark_watershed_line`: `true`
- `output_array_name`: `"Watershed Labels"`

Resolved inputs:

- `input_image_data_path` -> `"Microstructure/Cell Data/Boundary Strength"`
- `input_image_geometry_path` -> `"Microstructure"`

Declared outputs:

- `output_array_name` -> `"Watershed Labels"`

### 10. `WriteDREAM3DFilter`

Save the watershed labels and extrema analysis.

The selected values preserve the demonstrated input domain and create a named result that later steps can address explicitly.

Key parameters:

- `compression_level`: `5`
- `export_file_path`: `"Data/Output/ImageProcessing_Examples/05_Microstructure_Watershed/microstructure_watershed.dream3d"`
- `use_compression`: `true`
- `write_xdmf_file`: `false`

Declared outputs:

- `export_file_path` -> `"Data/Output/ImageProcessing_Examples/05_Microstructure_Watershed/microstructure_watershed.dream3d"`

## Outputs and how to interpret them

- `Data/Output/ImageProcessing_Examples/05_Microstructure_Watershed/microstructure_watershed.dream3d`

Intermediate arrays remain in the final DREAM3D file. Compare them in order. A plausible final mask does not prove that every upstream assumption is correct.

## How to adapt the pipeline

- Tune conductance and iteration count against boundary preservation.
- Use H-maxima height to ignore peaks below the material contrast of interest.
- Move to marker-controlled watershed when domain knowledge can define reliable seeds.

Keep input and output paths relative when the pipeline must run from a DREAM3D-NX Anaconda environment or an installed application bundle.

## Failure modes and quality checks

- Polishing scratches can become false watershed ridges.
- Uneven illumination can create broad false basins.
- An aggressive diffusion setting can erase narrow boundaries.
- Review intermediate arrays, not only the final writer output.
- Compare measurements with an independent reference before production use.

## Scientific basis and annotated references

Workflow reproduction status: `illustrative`.

The input is original paper-linked data, but the pipeline omits the paper's boundary-class neural network and therefore cannot reproduce HADMA accuracy.

Paper dataset availability: Original paper-linked input data is publicly available and a measured-derived input is included in this archive.

Dataset license: MIT license in the paper-linked GitHub repository.

Original or paper-linked dataset included in the example archive: `yes`.

Dataset record: [https://doi.org/10.15129/46955351-408b-4dc3-840e-3bc6a9f3432a](https://doi.org/10.15129/46955351-408b-4dc3-840e-3bc6a9f3432a)

1. G. Fotos, A. Campbell, P. Murray, E. Yakushina. “Deep learning enhanced Watershed for microstructural analysis using a boundary class semantic segmentation.” *Journal of Materials Science* (2023). [10.1007/s10853-023-08901-w](https://doi.org/10.1007/s10853-023-08901-w). Workflow reference for automated metallic-microstructure segmentation that combines boundary preparation, distance-based markers, H-maxima suppression, and watershed separation.
2. Luc Vincent, Pierre Soille. “Watersheds in digital spaces: An efficient algorithm based on immersion simulations.” *IEEE Transactions on Pattern Analysis and Machine Intelligence* (1991). [10.1109/34.87344](https://doi.org/10.1109/34.87344). Foundational watershed segmentation method used to separate basins and touching objects.
3. Pietro Perona, Jitendra Malik. “Scale-space and edge detection using anisotropic diffusion.” *IEEE Transactions on Pattern Analysis and Machine Intelligence* (1990). [10.1109/34.56205](https://doi.org/10.1109/34.56205). Foundational edge-preserving diffusion method used before segmentation or edge detection.
4. Luc Vincent. “Morphological grayscale reconstruction in image analysis: Applications and efficient algorithms.” *IEEE Transactions on Image Processing* (1993). [10.1109/83.217222](https://doi.org/10.1109/83.217222). Foundational reference for reconstruction, extrema suppression, and top-hat morphology.

## Guidance for an LLM or MCP assistant

Use this document to explain intent and tradeoffs. Use the `.yaml` sidecar to retrieve exact structured metadata. Use the `.d3dpipeline` file as the executable source of truth, and use the `.py` companion for Anaconda execution.

Before adapting this workflow, ask:

- What imaging system, material, and acquisition mode produced the data?
- What are the voxel or pixel spacing and physical units?
- What is the smallest feature that must be retained or reported?
- Which independent measurement or labeled dataset will validate the result?

Constraints:

- Meaningful boundaries correspond to gradient ridges.
- The image has enough contrast after edge-preserving denoise.
- Unseeded watershed is exploratory and can over-segment noisy data.
- Keep all file paths relative to the DREAM3D-NX application binary directory.
- Run preflight and execute the complete pipeline after any parameter or DataPath change.
- Treat example parameter values as starting points, not production acceptance limits.

Related examples:

- `(08) Powder Particle Watershed Segmentation.d3dpipeline`
- `(13) Denoising and Edge Detection.d3dpipeline`

When a user asks for a new pipeline, preserve DataPath consistency between steps, validate every mathematical domain, and run the result end to end with representative data.
