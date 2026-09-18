# Radiography Intensity Calibration

Executable pipeline: `(15) Radiography Intensity Calibration.d3dpipeline`

> **Validation warning:** Do not treat this example as a validated acceptance procedure. Adapt and validate it for the scanner, material, resolution, and quality requirement.

## Purpose and real-world setting

A radiography engineer converts detector transmission values into analysis-ready fields. The pipeline keeps mathematical branches separate so each transform can be inspected before a calibrated attenuation or display workflow is selected.

Demonstrates logarithmic attenuation conversion, exponential and reciprocal response transforms, range correction, normalization, inversion, thresholding, and detector masking.

## When to use this workflow

- Convert transmission to logarithmic attenuation
- Prepare display-range and masked detector images
- Explore response linearization and normalization transforms


## Input data and assumptions

- Transmission input is positive for logarithm and square-root operations.
- The mask uses nonzero values for valid detector pixels.
- The fixture illustrates mathematics and is not a complete detector dark-field and flat-field calibration.

<!-- INPUT-DATA-STRATEGY:BEGIN -->
### Input source strategy

Strategy: `synthetic-paper-inspired`.

Paper visual target: X-ray projection and flat-field examples with beam-profile variation, attenuating specimens, detector stripes, and invalid pixels.

Target traits:

- positive transmission
- beam profile
- step wedge
- multiple attenuation depths
- detector stripes
- valid-region mask

Generator: `Generators/GenerateImageProcessingExampleData.py` with fixed seed `20260901`.

Generated metrics: `PaperInputMetrics.json`. Target contract: `Generators/PaperInputTargets.json`.

Known differences: The paper's synchrotron datasets are unavailable. The generated fields demonstrate calibration domains but not eigen-flat-field estimation or reported reconstruction errors.

See `Data/ImageProcessing_Examples/Provenance.json` for per-file transformations, checksums, dimensions, spacing, and licensing.
<!-- INPUT-DATA-STRATEGY:END -->

## Data flow

Transmission, residual, and mask volumes -> independent absolute, reciprocal, exponential, logarithmic, root, square, normalization, rescale, invert, threshold, and mask branches -> DREAM3D file.

The `.d3dpipeline` file is authoritative for executable arguments. The matching `.yaml` file repeats the ordered steps in a structured form for software, LLM, and MCP use.

## Step-by-step filter explanation and parameter rationale

### 1. `ReadMhaFileFilter`

Import the volume with its physical spacing and scalar type.

The selected values preserve the demonstrated input domain and create a named result that later steps can address explicitly.

Key parameters:

- `apply_image_transformation`: `false`
- `cell_attribute_matrix_name`: `"Cell Data"`
- `cropping_options_index`: `{"bounds_x_physical":[0.0,0.0],"bounds_x_voxels":[0,0],"bounds_y_physical":[0.0,0.0],"bounds_y_voxels":[0,0],"bounds_z_physical":[0.0,0.0],"bounds_z_voxels":[0,0],"crop_x":true,...`
- `file_name`: `"Data/ImageProcessing_Examples/am/calibration/transmission.mha"`
- `image_data_array_name`: `"Transmission"`
- `interpolation_type_index`: `0`
- `output_geometry_path`: `"Transmission Calibration"`
- `output_transformation_matrix_path`: `"MHA Image/TransformationMatrix"`
- `save_image_transformation`: `false`
- `transpose_transform_matrix`: `false`

Resolved inputs:

- `file_name` -> `"Data/ImageProcessing_Examples/am/calibration/transmission.mha"`

Declared outputs:

- `output_geometry_path` -> `"Transmission Calibration"`
- `output_transformation_matrix_path` -> `"MHA Image/TransformationMatrix"`

### 2. `ReadMhaFileFilter`

Import the volume with its physical spacing and scalar type.

The selected values preserve the demonstrated input domain and create a named result that later steps can address explicitly.

Key parameters:

- `apply_image_transformation`: `false`
- `cell_attribute_matrix_name`: `"Cell Data"`
- `cropping_options_index`: `{"bounds_x_physical":[0.0,0.0],"bounds_x_voxels":[0,0],"bounds_y_physical":[0.0,0.0],"bounds_y_voxels":[0,0],"bounds_z_physical":[0.0,0.0],"bounds_z_voxels":[0,0],"crop_x":true,...`
- `file_name`: `"Data/ImageProcessing_Examples/am/calibration/residual.mha"`
- `image_data_array_name`: `"Residual"`
- `interpolation_type_index`: `0`
- `output_geometry_path`: `"Residual Calibration"`
- `output_transformation_matrix_path`: `"MHA Image/TransformationMatrix"`
- `save_image_transformation`: `false`
- `transpose_transform_matrix`: `false`

Resolved inputs:

- `file_name` -> `"Data/ImageProcessing_Examples/am/calibration/residual.mha"`

Declared outputs:

- `output_geometry_path` -> `"Residual Calibration"`
- `output_transformation_matrix_path` -> `"MHA Image/TransformationMatrix"`

### 3. `ReadMhaFileFilter`

Import the volume with its physical spacing and scalar type.

The selected values preserve the demonstrated input domain and create a named result that later steps can address explicitly.

Key parameters:

- `apply_image_transformation`: `false`
- `cell_attribute_matrix_name`: `"Cell Data"`
- `cropping_options_index`: `{"bounds_x_physical":[0.0,0.0],"bounds_x_voxels":[0,0],"bounds_y_physical":[0.0,0.0],"bounds_y_voxels":[0,0],"bounds_z_physical":[0.0,0.0],"bounds_z_voxels":[0,0],"crop_x":true,...`
- `file_name`: `"Data/ImageProcessing_Examples/am/calibration/mask.mha"`
- `image_data_array_name`: `"Mask"`
- `interpolation_type_index`: `0`
- `output_geometry_path`: `"Calibration Mask"`
- `output_transformation_matrix_path`: `"MHA Image/TransformationMatrix"`
- `save_image_transformation`: `false`
- `transpose_transform_matrix`: `false`

Resolved inputs:

- `file_name` -> `"Data/ImageProcessing_Examples/am/calibration/mask.mha"`

Declared outputs:

- `output_geometry_path` -> `"Calibration Mask"`
- `output_transformation_matrix_path` -> `"MHA Image/TransformationMatrix"`

### 4. `AbsImageFilter`

Convert signed residuals to absolute calibration error.

The selected values preserve the demonstrated input domain and create a named result that later steps can address explicitly.

Key parameters:

- `input_image_data_path`: `"Residual Calibration/Cell Data/Residual"`
- `input_image_geometry_path`: `"Residual Calibration"`
- `output_array_name`: `"Absolute Residual"`

Resolved inputs:

- `input_image_data_path` -> `"Residual Calibration/Cell Data/Residual"`
- `input_image_geometry_path` -> `"Residual Calibration"`

Declared outputs:

- `output_array_name` -> `"Absolute Residual"`

### 5. `BoundedReciprocalImageFilter`

Calculate a bounded inverse response without division by zero.

The selected values preserve the demonstrated input domain and create a named result that later steps can address explicitly.

Key parameters:

- `input_image_data_path`: `"Transmission Calibration/Cell Data/Transmission"`
- `input_image_geometry_path`: `"Transmission Calibration"`
- `output_array_name`: `"Bounded Reciprocal"`

Resolved inputs:

- `input_image_data_path` -> `"Transmission Calibration/Cell Data/Transmission"`
- `input_image_geometry_path` -> `"Transmission Calibration"`

Declared outputs:

- `output_array_name` -> `"Bounded Reciprocal"`

### 6. `ExpImageFilter`

Apply an exponential response model to positive transmission values.

The selected values preserve the demonstrated input domain and create a named result that later steps can address explicitly.

Key parameters:

- `input_image_data_path`: `"Transmission Calibration/Cell Data/Transmission"`
- `input_image_geometry_path`: `"Transmission Calibration"`
- `output_array_name`: `"Exponential Response"`

Resolved inputs:

- `input_image_data_path` -> `"Transmission Calibration/Cell Data/Transmission"`
- `input_image_geometry_path` -> `"Transmission Calibration"`

Declared outputs:

- `output_array_name` -> `"Exponential Response"`

### 7. `ExpNegativeImageFilter`

Calculate exponential attenuation from transmission values.

The selected values preserve the demonstrated input domain and create a named result that later steps can address explicitly.

Key parameters:

- `input_image_data_path`: `"Transmission Calibration/Cell Data/Transmission"`
- `input_image_geometry_path`: `"Transmission Calibration"`
- `output_array_name`: `"Negative Exponential"`

Resolved inputs:

- `input_image_data_path` -> `"Transmission Calibration/Cell Data/Transmission"`
- `input_image_geometry_path` -> `"Transmission Calibration"`

Declared outputs:

- `output_array_name` -> `"Negative Exponential"`

### 8. `LogImageFilter`

Convert transmission to a natural-log attenuation field.

The selected values preserve the demonstrated input domain and create a named result that later steps can address explicitly.

Key parameters:

- `input_image_data_path`: `"Transmission Calibration/Cell Data/Transmission"`
- `input_image_geometry_path`: `"Transmission Calibration"`
- `output_array_name`: `"Natural Log Transmission"`

Resolved inputs:

- `input_image_data_path` -> `"Transmission Calibration/Cell Data/Transmission"`
- `input_image_geometry_path` -> `"Transmission Calibration"`

Declared outputs:

- `output_array_name` -> `"Natural Log Transmission"`

### 9. `Log10ImageFilter`

Convert transmission to a base-10 attenuation field.

The selected values preserve the demonstrated input domain and create a named result that later steps can address explicitly.

Key parameters:

- `input_image_data_path`: `"Transmission Calibration/Cell Data/Transmission"`
- `input_image_geometry_path`: `"Transmission Calibration"`
- `output_array_name`: `"Base-10 Log Transmission"`

Resolved inputs:

- `input_image_data_path` -> `"Transmission Calibration/Cell Data/Transmission"`
- `input_image_geometry_path` -> `"Transmission Calibration"`

Declared outputs:

- `output_array_name` -> `"Base-10 Log Transmission"`

### 10. `SqrtImageFilter`

Convert intensity-squared response to an amplitude-like field.

The selected values preserve the demonstrated input domain and create a named result that later steps can address explicitly.

Key parameters:

- `input_image_data_path`: `"Transmission Calibration/Cell Data/Transmission"`
- `input_image_geometry_path`: `"Transmission Calibration"`
- `output_array_name`: `"Square Root Transmission"`

Resolved inputs:

- `input_image_data_path` -> `"Transmission Calibration/Cell Data/Transmission"`
- `input_image_geometry_path` -> `"Transmission Calibration"`

Declared outputs:

- `output_array_name` -> `"Square Root Transmission"`

### 11. `SquareImageFilter`

Convert an amplitude-like response to an energy-like field.

The selected values preserve the demonstrated input domain and create a named result that later steps can address explicitly.

Key parameters:

- `input_image_data_path`: `"Transmission Calibration/Cell Data/Transmission"`
- `input_image_geometry_path`: `"Transmission Calibration"`
- `output_array_name`: `"Squared Transmission"`

Resolved inputs:

- `input_image_data_path` -> `"Transmission Calibration/Cell Data/Transmission"`
- `input_image_geometry_path` -> `"Transmission Calibration"`

Declared outputs:

- `output_array_name` -> `"Squared Transmission"`

### 12. `NormalizeToConstantImageFilter`

Scale the image so its total intensity equals a chosen calibration constant.

The selected values preserve the demonstrated input domain and create a named result that later steps can address explicitly.

Key parameters:

- `constant`: `100.0`
- `input_image_data_path`: `"Transmission Calibration/Cell Data/Transmission"`
- `input_image_geometry_path`: `"Transmission Calibration"`
- `output_array_name`: `"Constant-Normalized Transmission"`

Resolved inputs:

- `input_image_data_path` -> `"Transmission Calibration/Cell Data/Transmission"`
- `input_image_geometry_path` -> `"Transmission Calibration"`

Declared outputs:

- `output_array_name` -> `"Constant-Normalized Transmission"`

### 13. `RescaleIntensityImageFilter`

Rescale transmission into an 8-bit display range.

The selected values preserve the demonstrated input domain and create a named result that later steps can address explicitly.

Key parameters:

- `input_image_data_path`: `"Transmission Calibration/Cell Data/Transmission"`
- `input_image_geometry_path`: `"Transmission Calibration"`
- `output_array_name`: `"Display Range"`
- `output_maximum`: `255.0`
- `output_minimum`: `0.0`

Resolved inputs:

- `input_image_data_path` -> `"Transmission Calibration/Cell Data/Transmission"`
- `input_image_geometry_path` -> `"Transmission Calibration"`

Declared outputs:

- `output_array_name` -> `"Display Range"`

### 14. `InvertIntensityImageFilter`

Invert a normalized transmission image so dense regions appear bright.

The selected values preserve the demonstrated input domain and create a named result that later steps can address explicitly.

Key parameters:

- `input_image_data_path`: `"Transmission Calibration/Cell Data/Transmission"`
- `input_image_geometry_path`: `"Transmission Calibration"`
- `maximum`: `1.0`
- `output_array_name`: `"Inverted Transmission"`

Resolved inputs:

- `input_image_data_path` -> `"Transmission Calibration/Cell Data/Transmission"`
- `input_image_geometry_path` -> `"Transmission Calibration"`

Declared outputs:

- `output_array_name` -> `"Inverted Transmission"`

### 15. `ThresholdImageFilter`

Reject saturated and underexposed transmission values.

Threshold and extrema values are tuned to the included fixture and must be recalibrated for a different material or sensor. Topology and foreground conventions must match the mask and the intended definition of object contact.

Key parameters:

- `input_image_data_path`: `"Transmission Calibration/Cell Data/Transmission"`
- `input_image_geometry_path`: `"Transmission Calibration"`
- `lower`: `0.1`
- `output_array_name`: `"Valid Transmission Range"`
- `outside_value`: `0.0`
- `upper`: `0.9`

Resolved inputs:

- `input_image_data_path` -> `"Transmission Calibration/Cell Data/Transmission"`
- `input_image_geometry_path` -> `"Transmission Calibration"`

Declared outputs:

- `output_array_name` -> `"Valid Transmission Range"`

### 16. `MaskImageFilter`

Apply a valid-detector-region mask to the calibrated transmission image.

Topology and foreground conventions must match the mask and the intended definition of object contact.

Key parameters:

- `input_image_data_path`: `"Transmission Calibration/Cell Data/Transmission"`
- `input_image_geometry_path`: `"Transmission Calibration"`
- `mask_image_data_path`: `"Calibration Mask/Cell Data/Mask"`
- `output_array_name`: `"Masked Transmission"`
- `outside_value`: `0.0`

Resolved inputs:

- `input_image_data_path` -> `"Transmission Calibration/Cell Data/Transmission"`
- `input_image_geometry_path` -> `"Transmission Calibration"`
- `mask_image_data_path` -> `"Calibration Mask/Cell Data/Mask"`

Declared outputs:

- `output_array_name` -> `"Masked Transmission"`

### 17. `WriteDREAM3DFilter`

Save the complete workflow result.

The selected values preserve the demonstrated input domain and create a named result that later steps can address explicitly.

Key parameters:

- `compression_level`: `5`
- `export_file_path`: `"Data/Output/ImageProcessing_Examples/11_Radiography_Calibration/radiography_calibration.dream3d"`
- `use_compression`: `true`
- `write_xdmf_file`: `false`

Declared outputs:

- `export_file_path` -> `"Data/Output/ImageProcessing_Examples/11_Radiography_Calibration/radiography_calibration.dream3d"`

## Outputs and how to interpret them

- `Data/Output/ImageProcessing_Examples/11_Radiography_Calibration/radiography_calibration.dream3d`

Intermediate arrays remain in the final DREAM3D file. Compare them in order. A plausible final mask does not prove that every upstream assumption is correct.

## How to adapt the pipeline

- Apply dark-field and flat-field correction before logarithmic attenuation.
- Keep logarithm inputs strictly positive and define how saturated pixels are handled.
- Record calibration units and reference intensity with the output.

Keep input and output paths relative when the pipeline must run from a DREAM3D-NX Anaconda environment or an installed application bundle.

## Failure modes and quality checks

- Zero or negative transmission is outside the logarithm domain.
- Exponential input can overflow if it is not scaled.
- Normalization can hide absolute exposure changes that matter to quality control.
- Review intermediate arrays, not only the final writer output.
- Compare measurements with an independent reference before production use.

## Scientific basis and annotated references

Workflow reproduction status: `illustrative`.

The paper's synchrotron datasets are unavailable. The generated fields demonstrate calibration domains but not eigen-flat-field estimation or reported reconstruction errors.

Paper dataset availability: The original paper input is unavailable or not redistributable. A deterministic paper-inspired synthetic input is included.

Dataset license: No separate dataset license is stated.

Original or paper-linked dataset included in the example archive: `no`.

Paper-inspired synthetic fallback included in the example archive: `yes`.

1. Vincent Van Nieuwenhove, Jan De Beenhouwer, Francesco De Carlo, Lucia Mancini, Federica Marone, Jan Sijbers. “Dynamic intensity normalization using eigen flat fields in X-ray imaging.” *Optics Express* (2015). [10.1364/OE.23.027975](https://doi.org/10.1364/OE.23.027975). Workflow reference for X-ray detector normalization, Beer-Lambert logarithmic attenuation, masking, and preparation of quantitative projection intensities.
2. D. F. Swinehart. “The Beer-Lambert Law.” *Journal of Chemical Education* (1962). [10.1021/ed039p333](https://doi.org/10.1021/ed039p333). Physical basis for logarithmic conversion of transmission radiographs to attenuation.

## Guidance for an LLM or MCP assistant

Use this document to explain intent and tradeoffs. Use the `.yaml` sidecar to retrieve exact structured metadata. Use the `.d3dpipeline` file as the executable source of truth, and use the `.py` companion for Anaconda execution.

Before adapting this workflow, ask:

- What imaging system, material, and acquisition mode produced the data?
- What are the voxel or pixel spacing and physical units?
- What is the smallest feature that must be retained or reported?
- Which independent measurement or labeled dataset will validate the result?

Constraints:

- Transmission input is positive for logarithm and square-root operations.
- The mask uses nonzero values for valid detector pixels.
- The fixture illustrates mathematics and is not a complete detector dark-field and flat-field calibration.
- Keep all file paths relative to the DREAM3D-NX application binary directory.
- Run preflight and execute the complete pipeline after any parameter or DataPath change.
- Treat example parameter values as starting points, not production acceptance limits.

Related examples:

- `(16) Phase and Angle Field Transforms.d3dpipeline`

When a user asks for a new pipeline, preserve DataPath consistency between steps, validate every mathematical domain, and run the result end to end with representative data.
