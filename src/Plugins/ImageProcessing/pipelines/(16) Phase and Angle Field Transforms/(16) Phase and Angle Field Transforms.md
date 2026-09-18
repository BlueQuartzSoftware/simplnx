# Phase and Angle Field Transforms

Executable pipeline: `(16) Phase and Angle Field Transforms.d3dpipeline`

> **Validation warning:** Do not treat this example as a validated acceptance procedure. Adapt and validate it for the scanner, material, resolution, and quality requirement.

## Purpose and real-world setting

An optical-metrology engineer converts between phase and sinusoidal detector responses. Separate phase and normalized fixtures prevent invalid inverse-function inputs and keep tangent away from singularities.

Applies forward trigonometric functions to a phase field and inverse trigonometric functions to a normalized response field with valid mathematical domains.

## When to use this workflow

- Create sine and cosine components from phase
- Convert normalized responses back to principal angles
- Demonstrate safe domains for trigonometric image filters


## Input data and assumptions

- Phase values are in radians.
- Inverse sine and inverse cosine inputs remain in [-1, 1].
- Principal-value inverse functions do not perform phase unwrapping.

<!-- INPUT-DATA-STRATEGY:BEGIN -->
### Input source strategy

Strategy: `synthetic-paper-inspired`.

Paper visual target: Smooth interferometric phase containing carrier tilt and a localized surface-height feature, with normalized sinusoidal response.

Target traits:

- radian phase
- carrier tilt
- localized phase bump
- valid inverse-function domain
- no tangent singularity

Generator: `Generators/GenerateImageProcessingExampleData.py` with fixed seed `20260901`.

Generated metrics: `PaperInputMetrics.json`. Target contract: `Generators/PaperInputTargets.json`.

Known differences: The paper's interferograms are unavailable. The generated analytical field tests the phase mathematics but not phase-step nonlinearity or experimental noise.

See `Data/ImageProcessing_Examples/Provenance.json` for per-file transformations, checksums, dimensions, spacing, and licensing.
<!-- INPUT-DATA-STRATEGY:END -->

## Data flow

Phase-radian volume -> sine, cosine, and tangent branches; normalized response volume -> inverse sine, inverse cosine, and inverse tangent branches -> DREAM3D file.

The `.d3dpipeline` file is authoritative for executable arguments. The matching `.yaml` file repeats the ordered steps in a structured form for software, LLM, and MCP use.

## Step-by-step filter explanation and parameter rationale

### 1. `ReadMhaFileFilter`

Import the volume with its physical spacing and scalar type.

The selected values preserve the demonstrated input domain and create a named result that later steps can address explicitly.

Key parameters:

- `apply_image_transformation`: `false`
- `cell_attribute_matrix_name`: `"Cell Data"`
- `cropping_options_index`: `{"bounds_x_physical":[0.0,0.0],"bounds_x_voxels":[0,0],"bounds_y_physical":[0.0,0.0],"bounds_y_voxels":[0,0],"bounds_z_physical":[0.0,0.0],"bounds_z_voxels":[0,0],"crop_x":true,...`
- `file_name`: `"Data/ImageProcessing_Examples/am/calibration/phase_radians.mha"`
- `image_data_array_name`: `"Phase Radians"`
- `interpolation_type_index`: `0`
- `output_geometry_path`: `"Phase Field"`
- `output_transformation_matrix_path`: `"MHA Image/TransformationMatrix"`
- `save_image_transformation`: `false`
- `transpose_transform_matrix`: `false`

Resolved inputs:

- `file_name` -> `"Data/ImageProcessing_Examples/am/calibration/phase_radians.mha"`

Declared outputs:

- `output_geometry_path` -> `"Phase Field"`
- `output_transformation_matrix_path` -> `"MHA Image/TransformationMatrix"`

### 2. `ReadMhaFileFilter`

Import the volume with its physical spacing and scalar type.

The selected values preserve the demonstrated input domain and create a named result that later steps can address explicitly.

Key parameters:

- `apply_image_transformation`: `false`
- `cell_attribute_matrix_name`: `"Cell Data"`
- `cropping_options_index`: `{"bounds_x_physical":[0.0,0.0],"bounds_x_voxels":[0,0],"bounds_y_physical":[0.0,0.0],"bounds_y_voxels":[0,0],"bounds_z_physical":[0.0,0.0],"bounds_z_voxels":[0,0],"crop_x":true,...`
- `file_name`: `"Data/ImageProcessing_Examples/am/calibration/normalized_response.mha"`
- `image_data_array_name`: `"Normalized Response"`
- `interpolation_type_index`: `0`
- `output_geometry_path`: `"Normalized Response"`
- `output_transformation_matrix_path`: `"MHA Image/TransformationMatrix"`
- `save_image_transformation`: `false`
- `transpose_transform_matrix`: `false`

Resolved inputs:

- `file_name` -> `"Data/ImageProcessing_Examples/am/calibration/normalized_response.mha"`

Declared outputs:

- `output_geometry_path` -> `"Normalized Response"`
- `output_transformation_matrix_path` -> `"MHA Image/TransformationMatrix"`

### 3. `SinImageFilter`

Calculate the sine component of the phase field.

The selected values preserve the demonstrated input domain and create a named result that later steps can address explicitly.

Key parameters:

- `input_image_data_path`: `"Phase Field/Cell Data/Phase Radians"`
- `input_image_geometry_path`: `"Phase Field"`
- `output_array_name`: `"Sine"`

Resolved inputs:

- `input_image_data_path` -> `"Phase Field/Cell Data/Phase Radians"`
- `input_image_geometry_path` -> `"Phase Field"`

Declared outputs:

- `output_array_name` -> `"Sine"`

### 4. `CosImageFilter`

Calculate the cosine component of the phase field.

The selected values preserve the demonstrated input domain and create a named result that later steps can address explicitly.

Key parameters:

- `input_image_data_path`: `"Phase Field/Cell Data/Phase Radians"`
- `input_image_geometry_path`: `"Phase Field"`
- `output_array_name`: `"Cosine"`

Resolved inputs:

- `input_image_data_path` -> `"Phase Field/Cell Data/Phase Radians"`
- `input_image_geometry_path` -> `"Phase Field"`

Declared outputs:

- `output_array_name` -> `"Cosine"`

### 5. `TanImageFilter`

Calculate the tangent of a phase field that avoids singularities.

The selected values preserve the demonstrated input domain and create a named result that later steps can address explicitly.

Key parameters:

- `input_image_data_path`: `"Phase Field/Cell Data/Phase Radians"`
- `input_image_geometry_path`: `"Phase Field"`
- `output_array_name`: `"Tangent"`

Resolved inputs:

- `input_image_data_path` -> `"Phase Field/Cell Data/Phase Radians"`
- `input_image_geometry_path` -> `"Phase Field"`

Declared outputs:

- `output_array_name` -> `"Tangent"`

### 6. `AsinImageFilter`

Convert a normalized sine response to an angle.

The selected values preserve the demonstrated input domain and create a named result that later steps can address explicitly.

Key parameters:

- `input_image_data_path`: `"Normalized Response/Cell Data/Normalized Response"`
- `input_image_geometry_path`: `"Normalized Response"`
- `output_array_name`: `"Inverse Sine"`

Resolved inputs:

- `input_image_data_path` -> `"Normalized Response/Cell Data/Normalized Response"`
- `input_image_geometry_path` -> `"Normalized Response"`

Declared outputs:

- `output_array_name` -> `"Inverse Sine"`

### 7. `AcosImageFilter`

Convert a normalized cosine response to an angle.

The selected values preserve the demonstrated input domain and create a named result that later steps can address explicitly.

Key parameters:

- `input_image_data_path`: `"Normalized Response/Cell Data/Normalized Response"`
- `input_image_geometry_path`: `"Normalized Response"`
- `output_array_name`: `"Inverse Cosine"`

Resolved inputs:

- `input_image_data_path` -> `"Normalized Response/Cell Data/Normalized Response"`
- `input_image_geometry_path` -> `"Normalized Response"`

Declared outputs:

- `output_array_name` -> `"Inverse Cosine"`

### 8. `AtanImageFilter`

Convert a normalized tangent-like response to an angle.

The selected values preserve the demonstrated input domain and create a named result that later steps can address explicitly.

Key parameters:

- `input_image_data_path`: `"Normalized Response/Cell Data/Normalized Response"`
- `input_image_geometry_path`: `"Normalized Response"`
- `output_array_name`: `"Inverse Tangent"`

Resolved inputs:

- `input_image_data_path` -> `"Normalized Response/Cell Data/Normalized Response"`
- `input_image_geometry_path` -> `"Normalized Response"`

Declared outputs:

- `output_array_name` -> `"Inverse Tangent"`

### 9. `WriteDREAM3DFilter`

Save the complete workflow result.

The selected values preserve the demonstrated input domain and create a named result that later steps can address explicitly.

Key parameters:

- `compression_level`: `5`
- `export_file_path`: `"Data/Output/ImageProcessing_Examples/12_Phase_Angles/phase_and_angle_transforms.dream3d"`
- `use_compression`: `true`
- `write_xdmf_file`: `false`

Declared outputs:

- `export_file_path` -> `"Data/Output/ImageProcessing_Examples/12_Phase_Angles/phase_and_angle_transforms.dream3d"`

## Outputs and how to interpret them

- `Data/Output/ImageProcessing_Examples/12_Phase_Angles/phase_and_angle_transforms.dream3d`

Intermediate arrays remain in the final DREAM3D file. Compare them in order. A plausible final mask does not prove that every upstream assumption is correct.

## How to adapt the pipeline

- Convert degree inputs to radians before forward transforms.
- Clamp only when calibration error is known; do not silently hide out-of-domain data.
- Use atan2 and phase unwrapping in a later workflow when quadrant and continuity matter.

Keep input and output paths relative when the pipeline must run from a DREAM3D-NX Anaconda environment or an installed application bundle.

## Failure modes and quality checks

- Tangent diverges near odd multiples of pi over two.
- Inverse sine and cosine fail outside [-1, 1].
- Principal angles lose cycle count and quadrant information.
- Review intermediate arrays, not only the final writer output.
- Compare measurements with an independent reference before production use.

## Scientific basis and annotated references

Workflow reproduction status: `method_reproduction`.

The paper's interferograms are unavailable. The generated analytical field tests the phase mathematics but not phase-step nonlinearity or experimental noise.

Paper dataset availability: The original paper input is unavailable or not redistributable. A deterministic paper-inspired synthetic input is included.

Dataset license: No separate dataset license is stated.

Original or paper-linked dataset included in the example archive: `no`.

Paper-inspired synthetic fallback included in the example archive: `yes`.

1. P. Hariharan, B. F. Oreb, T. Eiju. “Digital phase-shifting interferometry: A simple error-compensating phase calculation algorithm.” *Applied Optics* (1987). [10.1364/AO.26.002504](https://doi.org/10.1364/AO.26.002504). Primary scientific workflow for converting sinusoidal intensity measurements into phase fields.

## Guidance for an LLM or MCP assistant

Use this document to explain intent and tradeoffs. Use the `.yaml` sidecar to retrieve exact structured metadata. Use the `.d3dpipeline` file as the executable source of truth, and use the `.py` companion for Anaconda execution.

Before adapting this workflow, ask:

- What imaging system, material, and acquisition mode produced the data?
- What are the voxel or pixel spacing and physical units?
- What is the smallest feature that must be retained or reported?
- Which independent measurement or labeled dataset will validate the result?

Constraints:

- Phase values are in radians.
- Inverse sine and inverse cosine inputs remain in [-1, 1].
- Principal-value inverse functions do not perform phase unwrapping.
- Keep all file paths relative to the DREAM3D-NX application binary directory.
- Run preflight and execute the complete pipeline after any parameter or DataPath change.
- Treat example parameter values as starting points, not production acceptance limits.

Related examples:

- `(15) Radiography Intensity Calibration.d3dpipeline`

When a user asks for a new pipeline, preserve DataPath consistency between steps, validate every mathematical domain, and run the result end to end with representative data.
