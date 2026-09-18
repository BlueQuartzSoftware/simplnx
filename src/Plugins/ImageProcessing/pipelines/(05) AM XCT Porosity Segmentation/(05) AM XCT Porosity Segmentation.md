# AM XCT Porosity Segmentation

Executable pipeline: `(05) AM XCT Porosity Segmentation.d3dpipeline`

> **Validation warning:** Do not treat this example as a validated acceptance procedure. Adapt and validate it for the scanner, material, resolution, and quality requirement.

## Purpose and real-world setting

A quality engineer receives a reconstructed XCT volume from a laser powder-bed fusion part. The engineer needs a repeatable first-pass map of pores and lack-of-fusion regions before feature statistics or acceptance decisions.

Segments and labels internal pores in an additive-manufactured metal part from an XCT volume, then creates contours and a physical signed-distance map.

## When to use this workflow

- Screen an AM coupon or part for internal porosity
- Prepare a clean pore mask for defect statistics
- Create boundary and distance arrays for wall-thickness review


## Input data and assumptions

- Low-density defects are darker than the metal matrix.
- The MHA spacing is calibrated in physical units.
- The example thresholds are tuned to the included fixture, not to every scanner or alloy.

<!-- INPUT-DATA-STRATEGY:BEGIN -->
### Input source strategy

Strategy: `synthetic-paper-inspired`.

Paper visual target: Paper XCT coupon-interior cross-sections and segmented 3-D gas-pore and lack-of-fusion renderings.

Target traits:

- cropped metal coupon interior
- rounded gas pores
- elongated lack-of-fusion voids
- reconstruction noise
- weak ring artifacts

Generator: `Generators/GenerateImageProcessingExampleData.py` with fixed seed `20260901`.

Generated metrics: `PaperInputMetrics.json`. Target contract: `Generators/PaperInputTargets.json`.

Known differences: The paper's raw XCT volume is unavailable. The generated volume represents a cropped coupon interior, is smaller, uses a deterministic intensity model, and does not reproduce the paper's exact pore statistics.

See `Data/ImageProcessing_Examples/Provenance.json` for per-file transformations, checksums, dimensions, spacing, and licensing.
<!-- INPUT-DATA-STRATEGY:END -->

## Data flow

XCT volume -> intensity window -> normalized floating-point intensity -> edge-preserving diffusion -> binary pore mask -> closed mask -> pore labels, contours, and signed distances -> review images and DREAM3D file.

The `.d3dpipeline` file is authoritative for executable arguments. The matching `.yaml` file repeats the ordered steps in a structured form for software, LLM, and MCP use.

## Step-by-step filter explanation and parameter rationale

### 1. `ReadMhaFileFilter`

Import the volume with its physical spacing and scalar type.

The selected values preserve the demonstrated input domain and create a named result that later steps can address explicitly.

Key parameters:

- `apply_image_transformation`: `false`
- `cell_attribute_matrix_name`: `"Cell Data"`
- `cropping_options_index`: `{"bounds_x_physical":[0.0,0.0],"bounds_x_voxels":[0,0],"bounds_y_physical":[0.0,0.0],"bounds_y_voxels":[0,0],"bounds_z_physical":[0.0,0.0],"bounds_z_voxels":[0,0],"crop_x":true,...`
- `file_name`: `"Data/ImageProcessing_Examples/am/xct_porosity.mha"`
- `image_data_array_name`: `"XCT Intensity"`
- `interpolation_type_index`: `0`
- `output_geometry_path`: `"AM XCT"`
- `output_transformation_matrix_path`: `"MHA Image/TransformationMatrix"`
- `save_image_transformation`: `false`
- `transpose_transform_matrix`: `false`

Resolved inputs:

- `file_name` -> `"Data/ImageProcessing_Examples/am/xct_porosity.mha"`

Declared outputs:

- `output_geometry_path` -> `"AM XCT"`
- `output_transformation_matrix_path` -> `"MHA Image/TransformationMatrix"`

### 2. `IntensityWindowingImageFilter`

Window the reconstructed XCT range before denoising and segmentation.

The selected values preserve the demonstrated input domain and create a named result that later steps can address explicitly.

Key parameters:

- `input_image_data_path`: `"AM XCT/Cell Data/XCT Intensity"`
- `input_image_geometry_path`: `"AM XCT"`
- `output_array_name`: `"Windowed Intensity"`
- `output_maximum`: `65535.0`
- `output_minimum`: `0.0`
- `window_maximum`: `50000.0`
- `window_minimum`: `0.0`

Resolved inputs:

- `input_image_data_path` -> `"AM XCT/Cell Data/XCT Intensity"`
- `input_image_geometry_path` -> `"AM XCT"`

Declared outputs:

- `output_array_name` -> `"Windowed Intensity"`

### 3. `NormalizeImageFilter`

Convert the windowed XCT data to a normalized floating-point field.

The selected values preserve the demonstrated input domain and create a named result that later steps can address explicitly.

Key parameters:

- `input_image_data_path`: `"AM XCT/Cell Data/Windowed Intensity"`
- `input_image_geometry_path`: `"AM XCT"`
- `output_array_name`: `"Normalized Intensity"`

Resolved inputs:

- `input_image_data_path` -> `"AM XCT/Cell Data/Windowed Intensity"`
- `input_image_geometry_path` -> `"AM XCT"`

Declared outputs:

- `output_array_name` -> `"Normalized Intensity"`

### 4. `CurvatureAnisotropicDiffusionImageFilter`

Reduce reconstruction noise while retaining pore boundaries.

Diffusion settings balance noise reduction against boundary preservation and numerical stability.

Key parameters:

- `conductance_parameter`: `3.0`
- `conductance_scaling_update_interval`: `1`
- `input_image_data_path`: `"AM XCT/Cell Data/Normalized Intensity"`
- `input_image_geometry_path`: `"AM XCT"`
- `number_of_iterations`: `3`
- `output_array_name`: `"Denoised Intensity"`
- `time_step`: `0.001`

Resolved inputs:

- `input_image_data_path` -> `"AM XCT/Cell Data/Normalized Intensity"`
- `input_image_geometry_path` -> `"AM XCT"`

Declared outputs:

- `output_array_name` -> `"Denoised Intensity"`

### 5. `BinaryThresholdImageFilter`

Segment low-density pores and lack-of-fusion regions from the metal volume.

Threshold and extrema values are tuned to the included fixture and must be recalibrated for a different material or sensor. Topology and foreground conventions must match the mask and the intended definition of object contact.

Key parameters:

- `input_image_data_path`: `"AM XCT/Cell Data/Denoised Intensity"`
- `input_image_geometry_path`: `"AM XCT"`
- `inside_value`: `1`
- `lower_threshold`: `-20.0`
- `output_array_name`: `"Pore Mask"`
- `outside_value`: `0`
- `upper_threshold`: `-2.0`

Resolved inputs:

- `input_image_data_path` -> `"AM XCT/Cell Data/Denoised Intensity"`
- `input_image_geometry_path` -> `"AM XCT"`

Declared outputs:

- `output_array_name` -> `"Pore Mask"`

### 6. `BinaryMorphologicalClosingImageFilter`

Close one-voxel gaps inside segmented defects.

Scale parameters control which spatial structures are treated as noise, background, or a retained feature. Topology and foreground conventions must match the mask and the intended definition of object contact.

Key parameters:

- `foreground_value`: `1.0`
- `input_image_data_path`: `"AM XCT/Cell Data/Pore Mask"`
- `input_image_geometry_path`: `"AM XCT"`
- `kernel_radius`: `[1,1,1]`
- `kernel_type_index`: `1`
- `output_array_name`: `"Closed Pore Mask"`
- `safe_border`: `true`

Resolved inputs:

- `input_image_data_path` -> `"AM XCT/Cell Data/Pore Mask"`
- `input_image_geometry_path` -> `"AM XCT"`

Declared outputs:

- `output_array_name` -> `"Closed Pore Mask"`

### 7. `ConnectedComponentImageFilter`

Assign one label to each face-connected pore.

Topology and foreground conventions must match the mask and the intended definition of object contact.

Key parameters:

- `fully_connected`: `false`
- `input_image_data_path`: `"AM XCT/Cell Data/Closed Pore Mask"`
- `input_image_geometry_path`: `"AM XCT"`
- `output_array_name`: `"Pore Labels"`

Resolved inputs:

- `input_image_data_path` -> `"AM XCT/Cell Data/Closed Pore Mask"`
- `input_image_geometry_path` -> `"AM XCT"`

Declared outputs:

- `output_array_name` -> `"Pore Labels"`

### 8. `RelabelComponentImageFilter`

Remove very small detections and sort retained pores by size.

The selected values preserve the demonstrated input domain and create a named result that later steps can address explicitly.

Key parameters:

- `input_image_data_path`: `"AM XCT/Cell Data/Pore Labels"`
- `input_image_geometry_path`: `"AM XCT"`
- `minimum_object_size`: `8`
- `output_array_name`: `"Relabeled Pores"`
- `sort_by_object_size`: `true`

Resolved inputs:

- `input_image_data_path` -> `"AM XCT/Cell Data/Pore Labels"`
- `input_image_geometry_path` -> `"AM XCT"`

Declared outputs:

- `output_array_name` -> `"Relabeled Pores"`

### 9. `BinaryContourImageFilter`

Extract pore boundaries for visual review.

Topology and foreground conventions must match the mask and the intended definition of object contact.

Key parameters:

- `background_value`: `0.0`
- `foreground_value`: `1.0`
- `fully_connected`: `false`
- `input_image_data_path`: `"AM XCT/Cell Data/Closed Pore Mask"`
- `input_image_geometry_path`: `"AM XCT"`
- `output_array_name`: `"Pore Contours"`

Resolved inputs:

- `input_image_data_path` -> `"AM XCT/Cell Data/Closed Pore Mask"`
- `input_image_geometry_path` -> `"AM XCT"`

Declared outputs:

- `output_array_name` -> `"Pore Contours"`

### 10. `SignedMaurerDistanceMapImageFilter`

Measure signed physical distance from each voxel to the pore boundary.

Topology and foreground conventions must match the mask and the intended definition of object contact. Spacing and unit settings control the physical meaning of distance, size, and position.

Key parameters:

- `background_value`: `0.0`
- `input_image_data_path`: `"AM XCT/Cell Data/Closed Pore Mask"`
- `input_image_geometry_path`: `"AM XCT"`
- `inside_is_positive`: `false`
- `output_array_name`: `"Signed Pore Distance"`
- `squared_distance`: `false`
- `use_image_spacing`: `true`

Resolved inputs:

- `input_image_data_path` -> `"AM XCT/Cell Data/Closed Pore Mask"`
- `input_image_geometry_path` -> `"AM XCT"`

Declared outputs:

- `output_array_name` -> `"Signed Pore Distance"`

### 11. `WriteImageFilter`

Write the pore contour stack for review outside DREAM3D-NX.

The selected values preserve the demonstrated input domain and create a named result that later steps can address explicitly.

Key parameters:

- `add_scale_bar`: `false`
- `create_color_table`: `false`
- `file_name`: `"Data/Output/ImageProcessing_Examples/01_AM_XCT/pore_contours.tif"`
- `flip_mode_index`: `0`
- `image_array_path`: `"AM XCT/Cell Data/Pore Contours"`
- `index_offset`: `0`
- `input_image_geometry_path`: `"AM XCT"`
- `invalid_color_value`: `[0,0,0]`
- `leading_digit_character`: `"0"`
- `mask_array_path`: `""`
- `plane_index`: `0`
- `selected_preset`: `"Black-Body Radiation"`
- `total_index_digits`: `3`
- `use_mask`: `false`

Resolved inputs:

- `input_image_geometry_path` -> `"AM XCT"`

Declared outputs:

- `file_name` -> `"Data/Output/ImageProcessing_Examples/01_AM_XCT/pore_contours.tif"`

### 12. `WriteDREAM3DFilter`

Save the complete XCT porosity analysis.

The selected values preserve the demonstrated input domain and create a named result that later steps can address explicitly.

Key parameters:

- `compression_level`: `5`
- `export_file_path`: `"Data/Output/ImageProcessing_Examples/01_AM_XCT/am_xct_porosity.dream3d"`
- `use_compression`: `true`
- `write_xdmf_file`: `false`

Declared outputs:

- `export_file_path` -> `"Data/Output/ImageProcessing_Examples/01_AM_XCT/am_xct_porosity.dream3d"`

## Outputs and how to interpret them

- `Data/Output/ImageProcessing_Examples/01_AM_XCT/pore_contours.tif`
- `Data/Output/ImageProcessing_Examples/01_AM_XCT/am_xct_porosity.dream3d`

Intermediate arrays remain in the final DREAM3D file. Compare them in order. A plausible final mask does not prove that every upstream assumption is correct.

## How to adapt the pipeline

- Calibrate the window and threshold with a representative coupon and traceable reference method.
- Set the morphology radius from voxel size and the smallest gap that should be closed.
- Set minimum object size from the smallest reportable defect volume.

Keep input and output paths relative when the pipeline must run from a DREAM3D-NX Anaconda environment or an installed application bundle.

## Failure modes and quality checks

- Beam hardening or ring artifacts can be segmented as pores.
- An incorrect voxel spacing makes physical distances wrong.
- A global threshold can miss pores when the reconstruction intensity changes across the part.
- Review intermediate arrays, not only the final writer output.
- Compare measurements with an independent reference before production use.

## Scientific basis and annotated references

Workflow reproduction status: `method_reproduction`.

The paper's raw XCT volume is unavailable. The generated volume represents a cropped coupon interior, is smaller, uses a deterministic intensity model, and does not reproduce the paper's exact pore statistics.

Paper dataset availability: The original paper input is unavailable or not redistributable. A deterministic paper-inspired synthetic input is included.

Dataset license: No separate dataset license is stated.

Original or paper-linked dataset included in the example archive: `no`.

Paper-inspired synthetic fallback included in the example archive: `yes`.

1. Anton du Plessis, Philip Sperling, Andre Beerlink, Lerato Tshabalala, Shafick Hoosain, Nontombi Mathe, Stephan G. le Roux. “Standard method for microCT-based additive manufacturing quality control 1: Porosity analysis.” *MethodsX* (2018). [10.1016/j.mex.2018.09.005](https://doi.org/10.1016/j.mex.2018.09.005). Primary industrial workflow for threshold-based XCT porosity analysis in additive manufacturing.
2. Pietro Perona, Jitendra Malik. “Scale-space and edge detection using anisotropic diffusion.” *IEEE Transactions on Pattern Analysis and Machine Intelligence* (1990). [10.1109/34.56205](https://doi.org/10.1109/34.56205). Foundational edge-preserving diffusion method used before segmentation or edge detection.
3. Calvin R. Maurer Jr., Rensheng Qi, Vijay Raghavan. “A linear time algorithm for computing exact Euclidean distance transforms of binary images in arbitrary dimensions.” *IEEE Transactions on Pattern Analysis and Machine Intelligence* (2003). [10.1109/TPAMI.2003.1177156](https://doi.org/10.1109/TPAMI.2003.1177156). Exact Euclidean distance-transform basis for physical pore and wall-distance measurements.

## Guidance for an LLM or MCP assistant

Use this document to explain intent and tradeoffs. Use the `.yaml` sidecar to retrieve exact structured metadata. Use the `.d3dpipeline` file as the executable source of truth, and use the `.py` companion for Anaconda execution.

Before adapting this workflow, ask:

- What imaging system, material, and acquisition mode produced the data?
- What are the voxel or pixel spacing and physical units?
- What is the smallest feature that must be retained or reported?
- Which independent measurement or labeled dataset will validate the result?

Constraints:

- Low-density defects are darker than the metal matrix.
- The MHA spacing is calibrated in physical units.
- The example thresholds are tuned to the included fixture, not to every scanner or alloy.
- Keep all file paths relative to the DREAM3D-NX application binary directory.
- Run preflight and execute the complete pipeline after any parameter or DataPath change.
- Treat example parameter values as starting points, not production acceptance limits.

Related examples:

- `(12) Pore Distance and Wall-Thickness Metrology.d3dpipeline`
- `(14) Projection-Based Quality Summaries.d3dpipeline`

When a user asks for a new pipeline, preserve DataPath consistency between steps, validate every mathematical domain, and run the result end to end with representative data.
