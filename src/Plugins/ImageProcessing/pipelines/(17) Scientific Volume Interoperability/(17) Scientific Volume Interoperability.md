# Scientific Volume Interoperability

Executable pipeline: `(17) Scientific Volume Interoperability.d3dpipeline`

> **Validation warning:** Do not treat this example as a validated acceptance procedure. Adapt and validate it for the scanner, material, resolution, and quality requirement.

## Purpose and real-world setting

A research team receives volumes from different analysis ecosystems. The team needs a reproducible conversion into DREAM3D-NX while retaining spacing and affine intent and producing files that other tools can inspect.

Imports MHA and NIfTI volumes into separate geometries, preserves key spatial metadata, and writes stable image and DREAM3D outputs.

## When to use this workflow

- Exchange volume data with ITK-based MHA workflows
- Import NIfTI data with affine and scaling metadata
- Round-trip selected arrays to common image formats


## Input data and assumptions

- Coordinate conventions are reviewed before combining the two geometries.
- NIfTI affine and scaling fields are valid.
- File conversion does not prove semantic equivalence of array names or units.

<!-- INPUT-DATA-STRATEGY:BEGIN -->
### Input source strategy

Strategy: `tested-format-fixture`.

Paper visual target: NIfTI-1 format fields, affine orientation, scaling, and single-file versus dual-file interoperability rather than a canonical specimen image.

Target traits:

- valid MHA
- valid compressed NIfTI
- affine metadata
- scaling metadata

Known differences: The workflow paper defines a file format, not an input specimen. Small conformance fixtures are retained because visual similarity is not scientifically applicable.

See `Data/ImageProcessing_Examples/Provenance.json` for per-file transformations, checksums, dimensions, spacing, and licensing.
<!-- INPUT-DATA-STRATEGY:END -->

## Data flow

MHA and NIfTI files -> separate image geometries -> MHA round-trip and NIfTI TIFF series -> combined DREAM3D file.

The `.d3dpipeline` file is authoritative for executable arguments. The matching `.yaml` file repeats the ordered steps in a structured form for software, LLM, and MCP use.

## Step-by-step filter explanation and parameter rationale

### 1. `ReadMhaFileFilter`

Import the volume with its physical spacing and scalar type.

The selected values preserve the demonstrated input domain and create a named result that later steps can address explicitly.

Key parameters:

- `apply_image_transformation`: `false`
- `cell_attribute_matrix_name`: `"Cell Data"`
- `cropping_options_index`: `{"bounds_x_physical":[0.0,0.0],"bounds_x_voxels":[0,0],"bounds_y_physical":[0.0,0.0],"bounds_y_voxels":[0,0],"bounds_z_physical":[0.0,0.0],"bounds_z_voxels":[0,0],"crop_x":true,...`
- `file_name`: `"Data/ImageProcessing_Examples/io/scientific/sample.mha"`
- `image_data_array_name`: `"MHA Data"`
- `interpolation_type_index`: `0`
- `output_geometry_path`: `"MHA Volume"`
- `output_transformation_matrix_path`: `"MHA Image/TransformationMatrix"`
- `save_image_transformation`: `false`
- `transpose_transform_matrix`: `false`

Resolved inputs:

- `file_name` -> `"Data/ImageProcessing_Examples/io/scientific/sample.mha"`

Declared outputs:

- `output_geometry_path` -> `"MHA Volume"`
- `output_transformation_matrix_path` -> `"MHA Image/TransformationMatrix"`

### 2. `ReadNIfTIFileFilter`

Import a compressed NIfTI volume with its affine metadata and scaling.

The selected values preserve the demonstrated input domain and create a named result that later steps can address explicitly.

Key parameters:

- `apply_scaling_transform`: `true`
- `cell_attribute_matrix_name`: `"Cell Data"`
- `cropping_options_index`: `{"bounds_x_physical":[0.0,0.0],"bounds_x_voxels":[0,0],"bounds_y_physical":[0.0,0.0],"bounds_y_voxels":[0,0],"bounds_z_physical":[0.0,0.0],"bounds_z_voxels":[0,0],"crop_x":true,...`
- `image_data_array_name`: `"NIfTI Data"`
- `input_file_path`: `"Data/ImageProcessing_Examples/io/scientific/sample.nii.gz"`
- `output_image_geometry_path`: `"NIfTI Volume"`
- `use_affine_if_present`: `true`

Resolved inputs:

- `input_file_path` -> `"Data/ImageProcessing_Examples/io/scientific/sample.nii.gz"`

Declared outputs:

- `output_image_geometry_path` -> `"NIfTI Volume"`

### 3. `WriteImageFilter`

Write the imported MHA volume as a new MHA file.

The selected values preserve the demonstrated input domain and create a named result that later steps can address explicitly.

Key parameters:

- `add_scale_bar`: `false`
- `create_color_table`: `false`
- `file_name`: `"Data/Output/ImageProcessing_Examples/13_Scientific_IO/mha_round_trip.mha"`
- `flip_mode_index`: `0`
- `image_array_path`: `"MHA Volume/Cell Data/MHA Data"`
- `index_offset`: `0`
- `input_image_geometry_path`: `"MHA Volume"`
- `invalid_color_value`: `[0,0,0]`
- `leading_digit_character`: `"0"`
- `mask_array_path`: `""`
- `plane_index`: `0`
- `selected_preset`: `"Black-Body Radiation"`
- `total_index_digits`: `3`
- `use_mask`: `false`

Resolved inputs:

- `input_image_geometry_path` -> `"MHA Volume"`

Declared outputs:

- `file_name` -> `"Data/Output/ImageProcessing_Examples/13_Scientific_IO/mha_round_trip.mha"`

### 4. `WriteImageFilter`

Write the imported NIfTI volume as a TIFF slice series.

The selected values preserve the demonstrated input domain and create a named result that later steps can address explicitly.

Key parameters:

- `add_scale_bar`: `false`
- `create_color_table`: `false`
- `file_name`: `"Data/Output/ImageProcessing_Examples/13_Scientific_IO/nifti_slice.tif"`
- `flip_mode_index`: `0`
- `image_array_path`: `"NIfTI Volume/Cell Data/NIfTI Data"`
- `index_offset`: `0`
- `input_image_geometry_path`: `"NIfTI Volume"`
- `invalid_color_value`: `[0,0,0]`
- `leading_digit_character`: `"0"`
- `mask_array_path`: `""`
- `plane_index`: `0`
- `selected_preset`: `"Black-Body Radiation"`
- `total_index_digits`: `3`
- `use_mask`: `false`

Resolved inputs:

- `input_image_geometry_path` -> `"NIfTI Volume"`

Declared outputs:

- `file_name` -> `"Data/Output/ImageProcessing_Examples/13_Scientific_IO/nifti_slice.tif"`

### 5. `WriteDREAM3DFilter`

Save the complete workflow result.

The selected values preserve the demonstrated input domain and create a named result that later steps can address explicitly.

Key parameters:

- `compression_level`: `5`
- `export_file_path`: `"Data/Output/ImageProcessing_Examples/13_Scientific_IO/scientific_volumes.dream3d"`
- `use_compression`: `true`
- `write_xdmf_file`: `false`

Declared outputs:

- `export_file_path` -> `"Data/Output/ImageProcessing_Examples/13_Scientific_IO/scientific_volumes.dream3d"`

## Outputs and how to interpret them

- `Data/Output/ImageProcessing_Examples/13_Scientific_IO/mha_round_trip.mha`
- `Data/Output/ImageProcessing_Examples/13_Scientific_IO/nifti_slice.tif`
- `Data/Output/ImageProcessing_Examples/13_Scientific_IO/scientific_volumes.dream3d`

Intermediate arrays remain in the final DREAM3D file. Compare them in order. A plausible final mask does not prove that every upstream assumption is correct.

## How to adapt the pipeline

- Verify axis order, handedness, origin, spacing, and units with a known landmark.
- Choose whether to apply NIfTI scaling and affine transforms based on the source convention.
- Use a lossless output type when quantitative values must be preserved.

Keep input and output paths relative when the pipeline must run from a DREAM3D-NX Anaconda environment or an installed application bundle.

## Failure modes and quality checks

- A correct file read can still produce a mirrored or rotated interpretation.
- Writing to an integer image type can lose quantitative precision.
- NIfTI orientation conventions differ across software and must be tested with landmarks.
- Review intermediate arrays, not only the final writer output.
- Compare measurements with an independent reference before production use.

## Scientific basis and annotated references

Workflow reproduction status: `method_reproduction`.

The example reproduces MHA and NIfTI import and export behavior with repository fixtures rather than a paper result.

Paper dataset availability: NIfTI-1 is a format standard rather than a paper-specific experimental dataset.

Dataset license: Dataset-specific terms apply to each NIfTI sample.

Original or paper-linked dataset included in the example archive: `no`.

Dataset record: [https://nifti.nimh.nih.gov/nifti-1/data](https://nifti.nimh.nih.gov/nifti-1/data)

1. Robert W. Cox, John Ashburner, Hester Breman, Kate Fissell, Christian Haselgrove, Colin J. Holmes, Jack L. Lancaster, David E. Rex, Stephen M. Smith, Jeffrey B. Woodward, Stephen C. Strother. “A (Sort of) New Image Data Format Standard: NIfTI-1.” *Human Brain Mapping annual meeting* (2004). [NITRC paper PDF](https://www.nitrc.org/docman/view.php/26/204/TheNIfTI1Format2004.pdf). Authoritative scientific description of NIfTI-1 interoperability and affine metadata.

## Guidance for an LLM or MCP assistant

Use this document to explain intent and tradeoffs. Use the `.yaml` sidecar to retrieve exact structured metadata. Use the `.d3dpipeline` file as the executable source of truth, and use the `.py` companion for Anaconda execution.

Before adapting this workflow, ask:

- What imaging system, material, and acquisition mode produced the data?
- What are the voxel or pixel spacing and physical units?
- What is the smallest feature that must be retained or reported?
- Which independent measurement or labeled dataset will validate the result?

Constraints:

- Coordinate conventions are reviewed before combining the two geometries.
- NIfTI affine and scaling fields are valid.
- File conversion does not prove semantic equivalence of array names or units.
- Keep all file paths relative to the DREAM3D-NX application binary directory.
- Run preflight and execute the complete pipeline after any parameter or DataPath change.
- Treat example parameter values as starting points, not production acceptance limits.

Related examples:

- `(18) Industrial XCT Format Import.d3dpipeline`
- `(19) Serial-Section Stack Reconstruction.d3dpipeline`

When a user asks for a new pipeline, preserve DataPath consistency between steps, validate every mathematical domain, and run the result end to end with representative data.
