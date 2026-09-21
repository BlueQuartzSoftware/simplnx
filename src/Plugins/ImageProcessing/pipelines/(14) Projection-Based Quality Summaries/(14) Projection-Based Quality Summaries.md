# Projection-Based Quality Summaries

Executable pipeline: `(14) Projection-Based Quality Summaries.d3dpipeline`

> **Validation warning:** Do not treat this example as a validated acceptance procedure. Adapt and validate it for the scanner, material, resolution, and quality requirement.

## Purpose and real-world setting

An inspector must screen a volume before opening every slice. Different projections reveal dense inclusions, void paths, average structure, variation, or accumulated signal, so the pipeline creates each summary without changing the source.

Creates binary, maximum, mean, median, minimum, standard-deviation, and sum projections from one volume for rapid quality review.

## When to use this workflow

- Create rapid 2-D summaries of a 3-D scan
- Compare projection statistics for defect visibility
- Generate overview images for reports or triage


## Input data and assumptions

- The chosen projection axis matches the part orientation.
- Projection collapses depth and cannot locate a feature along that axis.
- Binary projection requires a mask threshold that has already been validated.

<!-- INPUT-DATA-STRATEGY:BEGIN -->
### Input source strategy

Strategy: `synthetic-paper-inspired-shared`.

Paper visual target: Projection views of a 3-D volume in which extrema, average signal, accumulated signal, and variance reveal different structures.

Target traits:

- 3-D attenuation volume
- multiple pore depths
- dense coupon boundary
- depth-dependent projection response

Generator: `Generators/GenerateImageProcessingExampleData.py` with fixed seed `20260901`.

Generated metrics: `PaperInputMetrics.json`. Target contract: `Generators/PaperInputTargets.json`.

Known differences: The generated AM coupon replaces the paper's nuclear-medicine data and demonstrates projection behavior rather than its clinical display results.

See `Data/ImageProcessing_Examples/Provenance.json` for per-file transformations, checksums, dimensions, spacing, and licensing.
<!-- INPUT-DATA-STRATEGY:END -->

## Data flow

Volume -> binary mask -> binary projection plus maximum, mean, median, minimum, standard-deviation, and sum projections -> DREAM3D file.

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
- `output_geometry_path`: `"Projection Volume"`
- `output_transformation_matrix_path`: `"MHA Image/TransformationMatrix"`
- `save_image_transformation`: `false`
- `transpose_transform_matrix`: `false`

Resolved inputs:

- `file_name` -> `"Data/ImageProcessing_Examples/am/xct_porosity.mha"`

Declared outputs:

- `output_geometry_path` -> `"Projection Volume"`
- `output_transformation_matrix_path` -> `"MHA Image/TransformationMatrix"`

### 2. `BinaryThresholdImageFilter`

Create a binary pore mask for occupancy projection.

Threshold and extrema values are tuned to the included fixture and must be recalibrated for a different material or sensor. Topology and foreground conventions must match the mask and the intended definition of object contact.

Key parameters:

- `input_image_data_path`: `"Projection Volume/Cell Data/XCT Intensity"`
- `input_image_geometry_path`: `"Projection Volume"`
- `inside_value`: `1`
- `lower_threshold`: `0.0`
- `output_array_name`: `"Pore Mask"`
- `outside_value`: `0`
- `upper_threshold`: `10000.0`

Resolved inputs:

- `input_image_data_path` -> `"Projection Volume/Cell Data/XCT Intensity"`
- `input_image_geometry_path` -> `"Projection Volume"`

Declared outputs:

- `output_array_name` -> `"Pore Mask"`

### 3. `BinaryProjectionImageFilter`

Mark each column that contains at least one pore voxel.

Topology and foreground conventions must match the mask and the intended definition of object contact.

Key parameters:

- `background_value`: `0.0`
- `foreground_value`: `1.0`
- `input_image_data_path`: `"Projection Volume/Cell Data/Pore Mask"`
- `input_image_geometry_path`: `"Projection Volume"`
- `output_array_name`: `"Pore Occupancy"`
- `output_image_geometry_name`: `"Binary Pore Projection"`
- `projection_dimension`: `2`
- `remove_original_geometry`: `false`

Resolved inputs:

- `input_image_data_path` -> `"Projection Volume/Cell Data/Pore Mask"`
- `input_image_geometry_path` -> `"Projection Volume"`

Declared outputs:

- `output_array_name` -> `"Pore Occupancy"`
- `output_image_geometry_name` -> `"Binary Pore Projection"`

### 4. `MaximumProjectionImageFilter`

Show the maximum reconstructed intensity along each column.

The selected values preserve the demonstrated input domain and create a named result that later steps can address explicitly.

Key parameters:

- `input_image_data_path`: `"Projection Volume/Cell Data/XCT Intensity"`
- `input_image_geometry_path`: `"Projection Volume"`
- `output_array_name`: `"Maximum Intensity"`
- `output_image_geometry_name`: `"Maximum XCT Projection"`
- `projection_dimension`: `2`
- `remove_original_geometry`: `false`

Resolved inputs:

- `input_image_data_path` -> `"Projection Volume/Cell Data/XCT Intensity"`
- `input_image_geometry_path` -> `"Projection Volume"`

Declared outputs:

- `output_array_name` -> `"Maximum Intensity"`
- `output_image_geometry_name` -> `"Maximum XCT Projection"`

### 5. `MeanProjectionImageFilter`

Show the average reconstructed intensity along each column.

The selected values preserve the demonstrated input domain and create a named result that later steps can address explicitly.

Key parameters:

- `input_image_data_path`: `"Projection Volume/Cell Data/XCT Intensity"`
- `input_image_geometry_path`: `"Projection Volume"`
- `output_array_name`: `"Mean Intensity"`
- `output_image_geometry_name`: `"Mean XCT Projection"`
- `projection_dimension`: `2`
- `remove_original_geometry`: `false`

Resolved inputs:

- `input_image_data_path` -> `"Projection Volume/Cell Data/XCT Intensity"`
- `input_image_geometry_path` -> `"Projection Volume"`

Declared outputs:

- `output_array_name` -> `"Mean Intensity"`
- `output_image_geometry_name` -> `"Mean XCT Projection"`

### 6. `MedianProjectionImageFilter`

Show a robust median intensity summary along each column.

The selected values preserve the demonstrated input domain and create a named result that later steps can address explicitly.

Key parameters:

- `input_image_data_path`: `"Projection Volume/Cell Data/XCT Intensity"`
- `input_image_geometry_path`: `"Projection Volume"`
- `output_array_name`: `"Median Intensity"`
- `output_image_geometry_name`: `"Median XCT Projection"`
- `projection_dimension`: `2`
- `remove_original_geometry`: `false`

Resolved inputs:

- `input_image_data_path` -> `"Projection Volume/Cell Data/XCT Intensity"`
- `input_image_geometry_path` -> `"Projection Volume"`

Declared outputs:

- `output_array_name` -> `"Median Intensity"`
- `output_image_geometry_name` -> `"Median XCT Projection"`

### 7. `MinimumProjectionImageFilter`

Show the lowest reconstructed intensity along each column.

The selected values preserve the demonstrated input domain and create a named result that later steps can address explicitly.

Key parameters:

- `input_image_data_path`: `"Projection Volume/Cell Data/XCT Intensity"`
- `input_image_geometry_path`: `"Projection Volume"`
- `output_array_name`: `"Minimum Intensity"`
- `output_image_geometry_name`: `"Minimum XCT Projection"`
- `projection_dimension`: `2`
- `remove_original_geometry`: `false`

Resolved inputs:

- `input_image_data_path` -> `"Projection Volume/Cell Data/XCT Intensity"`
- `input_image_geometry_path` -> `"Projection Volume"`

Declared outputs:

- `output_array_name` -> `"Minimum Intensity"`
- `output_image_geometry_name` -> `"Minimum XCT Projection"`

### 8. `StandardDeviationProjectionImageFilter`

Show through-thickness intensity variation along each column.

The selected values preserve the demonstrated input domain and create a named result that later steps can address explicitly.

Key parameters:

- `input_image_data_path`: `"Projection Volume/Cell Data/XCT Intensity"`
- `input_image_geometry_path`: `"Projection Volume"`
- `output_array_name`: `"Intensity Deviation"`
- `output_image_geometry_name`: `"Deviation XCT Projection"`
- `projection_dimension`: `2`
- `remove_original_geometry`: `false`

Resolved inputs:

- `input_image_data_path` -> `"Projection Volume/Cell Data/XCT Intensity"`
- `input_image_geometry_path` -> `"Projection Volume"`

Declared outputs:

- `output_array_name` -> `"Intensity Deviation"`
- `output_image_geometry_name` -> `"Deviation XCT Projection"`

### 9. `SumProjectionImageFilter`

Integrate reconstructed intensity along each column.

The selected values preserve the demonstrated input domain and create a named result that later steps can address explicitly.

Key parameters:

- `input_image_data_path`: `"Projection Volume/Cell Data/XCT Intensity"`
- `input_image_geometry_path`: `"Projection Volume"`
- `output_array_name`: `"Integrated Intensity"`
- `output_image_geometry_name`: `"Sum XCT Projection"`
- `projection_dimension`: `2`
- `remove_original_geometry`: `false`

Resolved inputs:

- `input_image_data_path` -> `"Projection Volume/Cell Data/XCT Intensity"`
- `input_image_geometry_path` -> `"Projection Volume"`

Declared outputs:

- `output_array_name` -> `"Integrated Intensity"`
- `output_image_geometry_name` -> `"Sum XCT Projection"`

### 10. `WriteDREAM3DFilter`

Save the complete workflow result.

The selected values preserve the demonstrated input domain and create a named result that later steps can address explicitly.

Key parameters:

- `compression_level`: `5`
- `export_file_path`: `"Data/Output/ImageProcessing_Examples/10_Projection_QA/projection_summaries.dream3d"`
- `use_compression`: `true`
- `write_xdmf_file`: `false`

Declared outputs:

- `export_file_path` -> `"Data/Output/ImageProcessing_Examples/10_Projection_QA/projection_summaries.dream3d"`

## Outputs and how to interpret them

- `Data/Output/ImageProcessing_Examples/10_Projections/projection_summaries.dream3d`

Intermediate arrays remain in the final DREAM3D file. Compare them in order. A plausible final mask does not prove that every upstream assumption is correct.

## How to adapt the pipeline

- Select the projection axis from the manufacturing or inspection direction.
- Use maximum projection for bright inclusions and minimum projection for dark voids.
- Use standard deviation to find depth-wise variation and motion artifacts.

Keep input and output paths relative when the pipeline must run from a DREAM3D-NX Anaconda environment or an installed application bundle.

## Failure modes and quality checks

- Overlapping features become indistinguishable after depth collapse.
- A single extreme voxel can dominate a maximum or minimum projection.
- Sum projection depends on the number of slices and needs normalization for cross-scan comparison.
- Review intermediate arrays, not only the final writer output.
- Compare measurements with an independent reference before production use.

## Scientific basis and annotated references

Workflow reproduction status: `illustrative`.

The generated AM coupon replaces the paper's nuclear-medicine data and demonstrates projection behavior rather than its clinical display results.

Paper dataset availability: The original paper input is unavailable or not redistributable. A deterministic paper-inspired synthetic input is included.

Dataset license: No separate dataset license is stated.

Original or paper-linked dataset included in the example archive: `no`.

Paper-inspired synthetic fallback included in the example archive: `yes`.

1. Jerold W. Wallis, Tom R. Miller, Charles A. Lerner, Eric C. Kleerup. “Three-Dimensional Display in Nuclear Medicine.” *IEEE Transactions on Medical Imaging* (1989). [10.1109/42.41482](https://doi.org/10.1109/42.41482). Peer-reviewed basis for projection and volume-display summaries of three-dimensional image data.

## Guidance for an LLM or MCP assistant

Use this document to explain intent and tradeoffs. Use the `.yaml` sidecar to retrieve exact structured metadata. Use the `.d3dpipeline` file as the executable source of truth, and use the `.py` companion for Anaconda execution.

Before adapting this workflow, ask:

- What imaging system, material, and acquisition mode produced the data?
- What are the voxel or pixel spacing and physical units?
- What is the smallest feature that must be retained or reported?
- Which independent measurement or labeled dataset will validate the result?

Constraints:

- The chosen projection axis matches the part orientation.
- Projection collapses depth and cannot locate a feature along that axis.
- Binary projection requires a mask threshold that has already been validated.
- Keep all file paths relative to the DREAM3D-NX application binary directory.
- Run preflight and execute the complete pipeline after any parameter or DataPath change.
- Treat example parameter values as starting points, not production acceptance limits.

Related examples:

- `(05) AM XCT Porosity Segmentation.d3dpipeline`
- `(18) Industrial XCT Format Import.d3dpipeline`

When a user asks for a new pipeline, preserve DataPath consistency between steps, validate every mathematical domain, and run the result end to end with representative data.
