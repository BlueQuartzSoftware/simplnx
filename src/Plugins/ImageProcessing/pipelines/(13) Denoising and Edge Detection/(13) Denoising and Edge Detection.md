# Denoising and Edge Detection

Executable pipeline: `(13) Denoising and Edge Detection.d3dpipeline`

> **Validation warning:** Do not treat this example as a validated acceptance procedure. Adapt and validate it for the scanner, material, resolution, and quality requirement.

## Purpose and real-world setting

An imaging scientist needs to choose a denoising and edge operator before building a production segmentation. This pipeline runs alternatives from the same source so edge localization and noise suppression can be compared directly.

Compares curvature flow, min-max curvature flow, Gaussian smoothing, gradient magnitude, recursive Gaussian gradient, Laplacian-of-Gaussian, and zero-crossing edge detection.

## When to use this workflow

- Select a denoising method for a noisy scientific image
- Compare first- and second-derivative edge responses
- Create zero-crossing edges from a Laplacian-of-Gaussian response


## Input data and assumptions

- The source contains boundaries at several spatial scales.
- Alternative outputs are diagnostic branches, not sequential recommendations.
- Sigma, variance, and iteration count must match the acquisition resolution and noise.

<!-- INPUT-DATA-STRATEGY:BEGIN -->
### Input source strategy

Strategy: `synthetic-paper-inspired`.

Paper visual target: Long-axis carotid or brachial ultrasound with speckled tissue, dark lumen, bright arterial walls, depth gain, and acoustic shadow.

Target traits:

- multiplicative speckle
- dark vessel lumen
- bright paired walls
- depth-dependent gain
- acoustic shadow

Generator: `Generators/GenerateImageProcessingExampleData.py` with fixed seed `20260901`.

Generated metrics: `PaperInputMetrics.json`. Target contract: `Generators/PaperInputTargets.json`.

Known differences: Clinical ultrasound frames cannot be redistributed. The generated image models visual statistics but not patient anatomy, transducer response, or diagnostic content.

See `Data/ImageProcessing_Examples/Provenance.json` for per-file transformations, checksums, dimensions, spacing, and licensing.
<!-- INPUT-DATA-STRATEGY:END -->

## Data flow

Source image -> curvature-flow and Gaussian denoise branches -> gradient and recursive-gradient branches -> Laplacian-of-Gaussian response -> zero-crossing edges -> DREAM3D file.

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
- `file_name`: `"Data/ImageProcessing_Examples/materials/edge_input.png"`
- `image_data_array_name`: `"Input Intensity"`
- `image_data_type_index`: `0`
- `length_unit_index`: `6`
- `origin`: `[0.0,0.0,0.0]`
- `origin_spacing_processing_index`: `1`
- `output_geometry_path`: `"Edge Image"`
- `spacing`: `[1.0,1.0,1.0]`

Resolved inputs:

- `file_name` -> `"Data/ImageProcessing_Examples/materials/edge_input.png"`

Declared outputs:

- `output_geometry_path` -> `"Edge Image"`

### 2. `NormalizeImageFilter`

Convert the image to a normalized floating-point field.

The selected values preserve the demonstrated input domain and create a named result that later steps can address explicitly.

Key parameters:

- `input_image_data_path`: `"Edge Image/Cell Data/Input Intensity"`
- `input_image_geometry_path`: `"Edge Image"`
- `output_array_name`: `"Normalized Intensity"`

Resolved inputs:

- `input_image_data_path` -> `"Edge Image/Cell Data/Input Intensity"`
- `input_image_geometry_path` -> `"Edge Image"`

Declared outputs:

- `output_array_name` -> `"Normalized Intensity"`

### 3. `CurvatureFlowImageFilter`

Smooth noise along curved intensity structures.

Diffusion settings balance noise reduction against boundary preservation and numerical stability.

Key parameters:

- `input_image_data_path`: `"Edge Image/Cell Data/Normalized Intensity"`
- `input_image_geometry_path`: `"Edge Image"`
- `number_of_iterations`: `3`
- `output_array_name`: `"Curvature Flow"`
- `time_step`: `0.05`

Resolved inputs:

- `input_image_data_path` -> `"Edge Image/Cell Data/Normalized Intensity"`
- `input_image_geometry_path` -> `"Edge Image"`

Declared outputs:

- `output_array_name` -> `"Curvature Flow"`

### 4. `MinMaxCurvatureFlowImageFilter`

Smooth noise while limiting curvature motion across local extrema.

Diffusion settings balance noise reduction against boundary preservation and numerical stability.

Key parameters:

- `input_image_data_path`: `"Edge Image/Cell Data/Normalized Intensity"`
- `input_image_geometry_path`: `"Edge Image"`
- `number_of_iterations`: `3`
- `output_array_name`: `"Min-Max Curvature Flow"`
- `stencil_radius`: `2`
- `time_step`: `0.05`

Resolved inputs:

- `input_image_data_path` -> `"Edge Image/Cell Data/Normalized Intensity"`
- `input_image_geometry_path` -> `"Edge Image"`

Declared outputs:

- `output_array_name` -> `"Min-Max Curvature Flow"`

### 5. `SmoothingRecursiveGaussianImageFilter`

Create a recursive Gaussian denoising reference.

Scale parameters control which spatial structures are treated as noise, background, or a retained feature.

Key parameters:

- `input_image_data_path`: `"Edge Image/Cell Data/Normalized Intensity"`
- `input_image_geometry_path`: `"Edge Image"`
- `normalize_across_scale`: `false`
- `output_array_name`: `"Recursive Gaussian"`
- `sigma`: `[1.0,1.0,0.0]`

Resolved inputs:

- `input_image_data_path` -> `"Edge Image/Cell Data/Normalized Intensity"`
- `input_image_geometry_path` -> `"Edge Image"`

Declared outputs:

- `output_array_name` -> `"Recursive Gaussian"`

### 6. `DiscreteGaussianImageFilter`

Create a discrete Gaussian denoising reference.

Scale parameters control which spatial structures are treated as noise, background, or a retained feature. Spacing and unit settings control the physical meaning of distance, size, and position.

Key parameters:

- `input_image_data_path`: `"Edge Image/Cell Data/Normalized Intensity"`
- `input_image_geometry_path`: `"Edge Image"`
- `maximum_error`: `[0.01,0.01,0.01]`
- `maximum_kernel_width`: `16`
- `output_array_name`: `"Discrete Gaussian"`
- `use_image_spacing`: `false`
- `variance`: `[1.0,1.0,0.0]`

Resolved inputs:

- `input_image_data_path` -> `"Edge Image/Cell Data/Normalized Intensity"`
- `input_image_geometry_path` -> `"Edge Image"`

Declared outputs:

- `output_array_name` -> `"Discrete Gaussian"`

### 7. `GradientMagnitudeImageFilter`

Measure edge strength with finite differences.

Spacing and unit settings control the physical meaning of distance, size, and position.

Key parameters:

- `input_image_data_path`: `"Edge Image/Cell Data/Normalized Intensity"`
- `input_image_geometry_path`: `"Edge Image"`
- `output_array_name`: `"Finite-Difference Gradient"`
- `use_image_spacing`: `true`

Resolved inputs:

- `input_image_data_path` -> `"Edge Image/Cell Data/Normalized Intensity"`
- `input_image_geometry_path` -> `"Edge Image"`

Declared outputs:

- `output_array_name` -> `"Finite-Difference Gradient"`

### 8. `GradientMagnitudeRecursiveGaussianImageFilter`

Measure edge strength after Gaussian derivative smoothing.

Scale parameters control which spatial structures are treated as noise, background, or a retained feature.

Key parameters:

- `input_image_data_path`: `"Edge Image/Cell Data/Normalized Intensity"`
- `input_image_geometry_path`: `"Edge Image"`
- `normalize_across_scale`: `false`
- `output_array_name`: `"Gaussian Gradient"`
- `sigma`: `1.0`

Resolved inputs:

- `input_image_data_path` -> `"Edge Image/Cell Data/Normalized Intensity"`
- `input_image_geometry_path` -> `"Edge Image"`

Declared outputs:

- `output_array_name` -> `"Gaussian Gradient"`

### 9. `LaplacianRecursiveGaussianImageFilter`

Create a signed second-derivative response for zero-crossing edges.

Scale parameters control which spatial structures are treated as noise, background, or a retained feature.

Key parameters:

- `input_image_data_path`: `"Edge Image/Cell Data/Normalized Intensity"`
- `input_image_geometry_path`: `"Edge Image"`
- `normalize_across_scale`: `false`
- `output_array_name`: `"Laplacian Response"`
- `sigma`: `1.0`

Resolved inputs:

- `input_image_data_path` -> `"Edge Image/Cell Data/Normalized Intensity"`
- `input_image_geometry_path` -> `"Edge Image"`

Declared outputs:

- `output_array_name` -> `"Laplacian Response"`

### 10. `ZeroCrossingImageFilter`

Extract edge locations from Laplacian sign changes.

Topology and foreground conventions must match the mask and the intended definition of object contact.

Key parameters:

- `background_value`: `0`
- `foreground_value`: `1`
- `input_image_data_path`: `"Edge Image/Cell Data/Laplacian Response"`
- `input_image_geometry_path`: `"Edge Image"`
- `output_array_name`: `"Laplacian Zero Crossings"`

Resolved inputs:

- `input_image_data_path` -> `"Edge Image/Cell Data/Laplacian Response"`
- `input_image_geometry_path` -> `"Edge Image"`

Declared outputs:

- `output_array_name` -> `"Laplacian Zero Crossings"`

### 11. `WriteDREAM3DFilter`

Save the complete workflow result.

The selected values preserve the demonstrated input domain and create a named result that later steps can address explicitly.

Key parameters:

- `compression_level`: `5`
- `export_file_path`: `"Data/Output/ImageProcessing_Examples/09_Denoising_Edges/denoising_and_edges.dream3d"`
- `use_compression`: `true`
- `write_xdmf_file`: `false`

Declared outputs:

- `export_file_path` -> `"Data/Output/ImageProcessing_Examples/09_Denoising_Edges/denoising_and_edges.dream3d"`

## Outputs and how to interpret them

- `Data/Output/ImageProcessing_Examples/09_Denoising_Edges/denoising_edges.dream3d`

Intermediate arrays remain in the final DREAM3D file. Compare them in order. A plausible final mask does not prove that every upstream assumption is correct.

## How to adapt the pipeline

- Measure edge width and noise correlation before selecting sigma or variance.
- Use diffusion when edge preservation matters more than linear smoothing.
- Inspect both response magnitude and final edge location.

Keep input and output paths relative when the pipeline must run from a DREAM3D-NX Anaconda environment or an installed application bundle.

## Failure modes and quality checks

- Too much smoothing shifts or removes small features.
- Derivative filters amplify high-frequency noise.
- Zero crossings include weak transitions unless response strength is checked separately.
- Review intermediate arrays, not only the final writer output.
- Compare measurements with an independent reference before production use.

## Scientific basis and annotated references

Workflow reproduction status: `illustrative`.

Clinical ultrasound frames cannot be redistributed. The generated image models visual statistics but not patient anatomy, transducer response, or diagnostic content.

Paper dataset availability: The original paper input is unavailable or not redistributable. A deterministic paper-inspired synthetic input is included.

Dataset license: Clinical source images are not licensed for redistribution in the paper.

Original or paper-linked dataset included in the example archive: `no`.

Paper-inspired synthetic fallback included in the example archive: `yes`.

1. Mehravar Rafati, Masoud Arabfard, Mehrdad Rafati. “Comparison of Different Edge Detections and Noise Reduction on Ultrasound Images of Carotid and Brachial Arteries Using a Speckle Reducing Anisotropic Diffusion Filter.” *Iranian Red Crescent Medical Journal* (2014). [10.5812/ircmj.14658](https://doi.org/10.5812/ircmj.14658). Workflow reference for comparing diffusion-based denoising with gradient, Laplacian-of-Gaussian, and other edge-detection outputs on the same scientific images.
2. Pietro Perona, Jitendra Malik. “Scale-space and edge detection using anisotropic diffusion.” *IEEE Transactions on Pattern Analysis and Machine Intelligence* (1990). [10.1109/34.56205](https://doi.org/10.1109/34.56205). Foundational edge-preserving diffusion method used before segmentation or edge detection.
3. David Marr, Ellen Hildreth. “Theory of edge detection.” *Proceedings of the Royal Society of London B* (1980). [10.1098/rspb.1980.0020](https://doi.org/10.1098/rspb.1980.0020). Scientific basis for Laplacian-of-Gaussian response and zero-crossing edge detection.

## Guidance for an LLM or MCP assistant

Use this document to explain intent and tradeoffs. Use the `.yaml` sidecar to retrieve exact structured metadata. Use the `.d3dpipeline` file as the executable source of truth, and use the `.py` companion for Anaconda execution.

Before adapting this workflow, ask:

- What imaging system, material, and acquisition mode produced the data?
- What are the voxel or pixel spacing and physical units?
- What is the smallest feature that must be retained or reported?
- Which independent measurement or labeled dataset will validate the result?

Constraints:

- The source contains boundaries at several spatial scales.
- Alternative outputs are diagnostic branches, not sequential recommendations.
- Sigma, variance, and iteration count must match the acquisition resolution and noise.
- Keep all file paths relative to the DREAM3D-NX application binary directory.
- Run preflight and execute the complete pipeline after any parameter or DataPath change.
- Treat example parameter values as starting points, not production acceptance limits.

Related examples:

- `(09) Microstructure Watershed Segmentation.d3dpipeline`

When a user asks for a new pipeline, preserve DataPath consistency between steps, validate every mathematical domain, and run the result end to end with representative data.
