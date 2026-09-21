# Pore Distance and Wall-Thickness Metrology

Executable pipeline: `(12) Pore Distance and Wall-Thickness Metrology.d3dpipeline`

> **Validation warning:** Do not treat this example as a validated acceptance procedure. Adapt and validate it for the scanner, material, resolution, and quality requirement.

## Purpose and real-world setting

A metrology engineer needs distance-to-boundary information for pores, walls, channels, or coatings. The pipeline preserves several algorithm outputs so precision, sign convention, spacing, and speed can be compared.

Compares approximate, Danielsson, signed Danielsson, signed Maurer, iso-contour, and zero-crossing distance products from one controlled mask fixture.

## When to use this workflow

- Estimate local pore radius or wall clearance
- Compare exact and approximate distance maps
- Prepare signed level-set and zero-crossing products


## Input data and assumptions

- The binary mask represents the intended material interface.
- Image spacing is correct when physical distance is requested.
- A distance map alone is not a complete wall-thickness algorithm; local thickness requires an additional geometric interpretation.

<!-- INPUT-DATA-STRATEGY:BEGIN -->
### Input source strategy

Strategy: `synthetic-paper-inspired-shared`.

Paper visual target: Binary crack mask, Euclidean distance map, top-hat response, and skeleton used for local crack-width measurement.

Target traits:

- variable-width cracks
- branching topology
- known foreground convention
- gaps and edge curvature

Generator: `Generators/GenerateImageProcessingExampleData.py` with fixed seed `20260901`.

Generated metrics: `PaperInputMetrics.json`. Target contract: `Generators/PaperInputTargets.json`.

Known differences: The generated mask is shared with skeletonization and the pipeline compares distance implementations rather than reproducing the paper's complete curvature-aware width estimator.

See `Data/ImageProcessing_Examples/Provenance.json` for per-file transformations, checksums, dimensions, spacing, and licensing.
<!-- INPUT-DATA-STRATEGY:END -->

## Data flow

Distance fixture -> independent approximate, Danielsson, signed Danielsson, signed Maurer, and iso-contour distance branches -> zero crossing of a signed field -> DREAM3D file.

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
- `file_name`: `"Data/ImageProcessing_Examples/materials/binary_mask.png"`
- `image_data_array_name`: `"Mask"`
- `image_data_type_index`: `0`
- `length_unit_index`: `6`
- `origin`: `[0.0,0.0,0.0]`
- `origin_spacing_processing_index`: `1`
- `output_geometry_path`: `"Distance Mask"`
- `spacing`: `[1.0,1.0,1.0]`

Resolved inputs:

- `file_name` -> `"Data/ImageProcessing_Examples/materials/binary_mask.png"`

Declared outputs:

- `output_geometry_path` -> `"Distance Mask"`

### 2. `ApproximateSignedDistanceMapImageFilter`

Calculate a fast approximate signed distance from the mask boundary.

Topology and foreground conventions must match the mask and the intended definition of object contact.

Key parameters:

- `input_image_data_path`: `"Distance Mask/Cell Data/Mask"`
- `input_image_geometry_path`: `"Distance Mask"`
- `inside_value`: `1.0`
- `output_array_name`: `"Approximate Signed Distance"`
- `outside_value`: `0.0`

Resolved inputs:

- `input_image_data_path` -> `"Distance Mask/Cell Data/Mask"`
- `input_image_geometry_path` -> `"Distance Mask"`

Declared outputs:

- `output_array_name` -> `"Approximate Signed Distance"`

### 3. `DanielssonDistanceMapImageFilter`

Calculate unsigned physical distance to the nearest foreground voxel.

Spacing and unit settings control the physical meaning of distance, size, and position.

Key parameters:

- `input_image_data_path`: `"Distance Mask/Cell Data/Mask"`
- `input_image_geometry_path`: `"Distance Mask"`
- `input_is_binary`: `true`
- `output_array_name`: `"Danielsson Distance"`
- `squared_distance`: `false`
- `use_image_spacing`: `true`

Resolved inputs:

- `input_image_data_path` -> `"Distance Mask/Cell Data/Mask"`
- `input_image_geometry_path` -> `"Distance Mask"`

Declared outputs:

- `output_array_name` -> `"Danielsson Distance"`

### 4. `SignedDanielssonDistanceMapImageFilter`

Calculate signed Danielsson distance for inside and outside metrology.

Spacing and unit settings control the physical meaning of distance, size, and position.

Key parameters:

- `input_image_data_path`: `"Distance Mask/Cell Data/Mask"`
- `input_image_geometry_path`: `"Distance Mask"`
- `inside_is_positive`: `false`
- `output_array_name`: `"Signed Danielsson Distance"`
- `squared_distance`: `false`
- `use_image_spacing`: `true`

Resolved inputs:

- `input_image_data_path` -> `"Distance Mask/Cell Data/Mask"`
- `input_image_geometry_path` -> `"Distance Mask"`

Declared outputs:

- `output_array_name` -> `"Signed Danielsson Distance"`

### 5. `SignedMaurerDistanceMapImageFilter`

Calculate an exact signed Euclidean distance map.

Topology and foreground conventions must match the mask and the intended definition of object contact. Spacing and unit settings control the physical meaning of distance, size, and position.

Key parameters:

- `background_value`: `0.0`
- `input_image_data_path`: `"Distance Mask/Cell Data/Mask"`
- `input_image_geometry_path`: `"Distance Mask"`
- `inside_is_positive`: `false`
- `output_array_name`: `"Signed Maurer Distance"`
- `squared_distance`: `false`
- `use_image_spacing`: `true`

Resolved inputs:

- `input_image_data_path` -> `"Distance Mask/Cell Data/Mask"`
- `input_image_geometry_path` -> `"Distance Mask"`

Declared outputs:

- `output_array_name` -> `"Signed Maurer Distance"`

### 6. `IsoContourDistanceImageFilter`

Calculate a narrow signed band around the binary iso-contour.

The selected values preserve the demonstrated input domain and create a named result that later steps can address explicitly.

Key parameters:

- `far_value`: `10.0`
- `input_image_data_path`: `"Distance Mask/Cell Data/Mask"`
- `input_image_geometry_path`: `"Distance Mask"`
- `level_set_value`: `0.5`
- `output_array_name`: `"Iso-Contour Distance"`

Resolved inputs:

- `input_image_data_path` -> `"Distance Mask/Cell Data/Mask"`
- `input_image_geometry_path` -> `"Distance Mask"`

Declared outputs:

- `output_array_name` -> `"Iso-Contour Distance"`

### 7. `ZeroCrossingImageFilter`

Locate the zero crossings of the signed distance field.

Topology and foreground conventions must match the mask and the intended definition of object contact.

Key parameters:

- `background_value`: `0`
- `foreground_value`: `1`
- `input_image_data_path`: `"Distance Mask/Cell Data/Signed Maurer Distance"`
- `input_image_geometry_path`: `"Distance Mask"`
- `output_array_name`: `"Distance Zero Crossings"`

Resolved inputs:

- `input_image_data_path` -> `"Distance Mask/Cell Data/Signed Maurer Distance"`
- `input_image_geometry_path` -> `"Distance Mask"`

Declared outputs:

- `output_array_name` -> `"Distance Zero Crossings"`

### 8. `WriteDREAM3DFilter`

Save the complete workflow result.

The selected values preserve the demonstrated input domain and create a named result that later steps can address explicitly.

Key parameters:

- `compression_level`: `5`
- `export_file_path`: `"Data/Output/ImageProcessing_Examples/08_Distance_Metrology/distance_metrology.dream3d"`
- `use_compression`: `true`
- `write_xdmf_file`: `false`

Declared outputs:

- `export_file_path` -> `"Data/Output/ImageProcessing_Examples/08_Distance_Metrology/distance_metrology.dream3d"`

## Outputs and how to interpret them

- `Data/Output/ImageProcessing_Examples/08_Distance_Metrology/distance_metrology.dream3d`

Intermediate arrays remain in the final DREAM3D file. Compare them in order. A plausible final mask does not prove that every upstream assumption is correct.

## How to adapt the pipeline

- Select sign convention before combining distances with masks.
- Enable image spacing for physical units and verify anisotropic voxels.
- Benchmark exact and approximate methods at the production volume size.

Keep input and output paths relative when the pipeline must run from a DREAM3D-NX Anaconda environment or an installed application bundle.

## Failure modes and quality checks

- Wrong background value reverses or corrupts distances.
- Squared-distance output can be mistaken for linear distance.
- Voxel discretization limits accuracy at thin or oblique walls.
- Review intermediate arrays, not only the final writer output.
- Compare measurements with an independent reference before production use.

## Scientific basis and annotated references

Workflow reproduction status: `illustrative`.

The generated mask is shared with skeletonization and the pipeline compares distance implementations rather than reproducing the paper's complete curvature-aware width estimator.

Paper dataset availability: The original paper input is unavailable or not redistributable. A deterministic paper-inspired synthetic input is included.

Dataset license: No separate dataset license is stated.

Original or paper-linked dataset included in the example archive: `no`.

Paper-inspired synthetic fallback included in the example archive: `yes`.

1. Miguel Carrasco, Gerardo Araya-Letelier, Ramiro Velázquez, Paolo Visconti. “Image-Based Automated Width Measurement of Surface Cracking.” *Sensors* (2021). [10.3390/s21227534](https://doi.org/10.3390/s21227534). Workflow reference for binary-interface distance transforms, topological skeletons, and physically calibrated local-width measurements.
2. Calvin R. Maurer Jr., Rensheng Qi, Vijay Raghavan. “A linear time algorithm for computing exact Euclidean distance transforms of binary images in arbitrary dimensions.” *IEEE Transactions on Pattern Analysis and Machine Intelligence* (2003). [10.1109/TPAMI.2003.1177156](https://doi.org/10.1109/TPAMI.2003.1177156). Exact Euclidean distance-transform basis for physical pore and wall-distance measurements.
3. Per-Erik Danielsson. “Euclidean distance mapping.” *Computer Graphics and Image Processing* (1980). [10.1016/0146-664X(80)90054-4](https://doi.org/10.1016/0146-664X(80)90054-4). Foundational vector-based Euclidean distance-map method.

## Guidance for an LLM or MCP assistant

Use this document to explain intent and tradeoffs. Use the `.yaml` sidecar to retrieve exact structured metadata. Use the `.d3dpipeline` file as the executable source of truth, and use the `.py` companion for Anaconda execution.

Before adapting this workflow, ask:

- What imaging system, material, and acquisition mode produced the data?
- What are the voxel or pixel spacing and physical units?
- What is the smallest feature that must be retained or reported?
- Which independent measurement or labeled dataset will validate the result?

Constraints:

- The binary mask represents the intended material interface.
- Image spacing is correct when physical distance is requested.
- A distance map alone is not a complete wall-thickness algorithm; local thickness requires an additional geometric interpretation.
- Keep all file paths relative to the DREAM3D-NX application binary directory.
- Run preflight and execute the complete pipeline after any parameter or DataPath change.
- Treat example parameter values as starting points, not production acceptance limits.

Related examples:

- `(05) AM XCT Porosity Segmentation.d3dpipeline`
- `(10) Binary Mask Repair and Skeletonization.d3dpipeline`

When a user asks for a new pipeline, preserve DataPath consistency between steps, validate every mathematical domain, and run the result end to end with representative data.
