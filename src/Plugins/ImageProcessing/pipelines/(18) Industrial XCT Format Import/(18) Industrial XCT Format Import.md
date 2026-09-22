# Industrial XCT Format Import

Executable pipeline: `(18) Industrial XCT Format Import.d3dpipeline`

> **Validation warning:** Do not treat this example as a validated acceptance procedure. Adapt and validate it for the scanner, material, resolution, and quality requirement.

## Purpose and real-world setting

A metrology laboratory receives reconstructed XCT volumes from several scanner vendors. The engineer needs one analysis environment while preserving each format's dimensions, spacing, origin, and scalar values.

Imports representative North Star Imaging, Volume Graphics, and Zeiss TXM datasets into separate DREAM3D-NX image geometries.

## When to use this workflow

- Import North Star Imaging header and binary pairs
- Import Volume Graphics VGI data
- Import Zeiss TXM volumes for common downstream analysis


## Input data and assumptions

- Compound formats include all companion files at the expected relative paths.
- Vendor metadata is trusted only after a dimensional check.
- The three imports stay separate because they represent alternative sources, not registered copies.

<!-- INPUT-DATA-STRATEGY:BEGIN -->
### Input source strategy

Strategy: `tested-format-fixture`.

Paper visual target: Industrial XCT volumes with calibrated dimensions and internal material structure across vendor formats.

Target traits:

- valid compound formats
- vendor metadata
- dimensions
- spacing
- scalar values

Known differences: The review has no common multi-vendor dataset. Tested format fixtures are retained because import correctness is more important than visual imitation.

See `Data/ImageProcessing_Examples/Provenance.json` for per-file transformations, checksums, dimensions, spacing, and licensing.
<!-- INPUT-DATA-STRATEGY:END -->

## Data flow

North Star header and data, Volume Graphics VGI and raw data, and Zeiss TXM -> three independent image geometries -> one DREAM3D file.

The `.d3dpipeline` file is authoritative for executable arguments. The matching `.yaml` file repeats the ordered steps in a structured form for software, LLM, and MCP use.

## Step-by-step filter explanation and parameter rationale

### 1. `ReadBinaryCTNorthstarFilter`

Import a compound North Star Imaging XCT volume from its header and binary data files.

Spacing and unit settings control the physical meaning of distance, size, and position.

Key parameters:

- `cell_attribute_matrix_name`: `"Cell Data"`
- `density_array_name`: `"Density"`
- `end_voxel_coord`: `[1,1,1]`
- `import_subvolume`: `false`
- `input_header_file`: `"Data/ImageProcessing_Examples/io/northstar/sample.nsihdr"`
- `input_image_geometry_path`: `"North Star XCT"`
- `length_unit_index`: `6`
- `start_voxel_coord`: `[0,0,0]`

Resolved inputs:

- `input_header_file` -> `"Data/ImageProcessing_Examples/io/northstar/sample.nsihdr"`
- `input_image_geometry_path` -> `"North Star XCT"`

### 2. `ReadVolumeGraphicsFileFilter`

Import a Volume Graphics header and its companion raw volume.

The selected values preserve the demonstrated input domain and create a named result that later steps can address explicitly.

Key parameters:

- `cell_attribute_matrix_name`: `"Cell Data"`
- `density_array_name`: `"Density"`
- `output_image_geometry_path`: `"Volume Graphics XCT"`
- `vg_header_file`: `"Data/ImageProcessing_Examples/io/volume_graphics/VolumeGraphicsTest.vgi"`

Resolved inputs:

- `vg_header_file` -> `"Data/ImageProcessing_Examples/io/volume_graphics/VolumeGraphicsTest.vgi"`

Declared outputs:

- `output_image_geometry_path` -> `"Volume Graphics XCT"`

### 3. `ReadZeissTxmFileFilter`

Import a Zeiss TXM XCT volume with its stored geometry metadata.

The selected values preserve the demonstrated input domain and create a named result that later steps can address explicitly.

Key parameters:

- `cell_attribute_matrix_name`: `"Cell Data"`
- `cropping_options_index`: `{"bounds_x_physical":[0.0,0.0],"bounds_x_voxels":[0,0],"bounds_y_physical":[0.0,0.0],"bounds_y_voxels":[0,0],"bounds_z_physical":[0.0,0.0],"bounds_z_voxels":[0,0],"crop_x":true,...`
- `ct_data_array_name`: `"CT Data"`
- `input_file_path`: `"Data/ImageProcessing_Examples/io/zeiss/sample.txm"`
- `output_image_geometry_path`: `"Zeiss XCT"`

Resolved inputs:

- `input_file_path` -> `"Data/ImageProcessing_Examples/io/zeiss/sample.txm"`

Declared outputs:

- `output_image_geometry_path` -> `"Zeiss XCT"`

### 4. `WriteDREAM3DFilter`

Save the complete workflow result.

The selected values preserve the demonstrated input domain and create a named result that later steps can address explicitly.

Key parameters:

- `compression_level`: `5`
- `export_file_path`: `"Data/Output/ImageProcessing_Examples/14_Industrial_XCT/industrial_xct_imports.dream3d"`
- `use_compression`: `true`
- `write_xdmf_file`: `false`

Declared outputs:

- `export_file_path` -> `"Data/Output/ImageProcessing_Examples/14_Industrial_XCT/industrial_xct_imports.dream3d"`

## Outputs and how to interpret them

- `Data/Output/ImageProcessing_Examples/14_Industrial_XCT/industrial_xct_imports.dream3d`

Intermediate arrays remain in the final DREAM3D file. Compare them in order. A plausible final mask does not prove that every upstream assumption is correct.

## How to adapt the pipeline

- Check one known dimension after import.
- Keep vendor sidecar and raw files together when moving data.
- Use subvolume import only after voxel coordinates are checked against full dimensions.

Keep input and output paths relative when the pipeline must run from a DREAM3D-NX Anaconda environment or an installed application bundle.

## Failure modes and quality checks

- A missing companion raw file makes a valid header unusable.
- Vendor length units can differ from the downstream analysis assumption.
- Incorrect byte order, scalar type, or data-file order corrupts density values.
- Review intermediate arrays, not only the final writer output.
- Compare measurements with an independent reference before production use.

## Scientific basis and annotated references

Workflow reproduction status: `illustrative`.

The example demonstrates three vendor import paths with tested fixtures. It does not reproduce a dimensional-metrology result from the review.

Paper dataset availability: The review contains industrial examples but no single reusable multi-vendor source dataset.

Dataset license: Vendor and specimen data terms vary.

Original or paper-linked dataset included in the example archive: `no`.

1. Jean-Pierre Kruth, Markus Bartscher, Simone Carmignato, Robert Schmitt, Leonardo De Chiffre, Albert Weckenmann. “Computed tomography for dimensional metrology.” *CIRP Annals* (2011). [10.1016/j.cirp.2011.05.006](https://doi.org/10.1016/j.cirp.2011.05.006). Primary industrial context for XCT ingestion, dimensional metrology, and material inspection.

## Guidance for an LLM or MCP assistant

Use this document to explain intent and tradeoffs. Use the `.yaml` sidecar to retrieve exact structured metadata. Use the `.d3dpipeline` file as the executable source of truth, and use the `.py` companion for Anaconda execution.

Before adapting this workflow, ask:

- What imaging system, material, and acquisition mode produced the data?
- What are the voxel or pixel spacing and physical units?
- What is the smallest feature that must be retained or reported?
- Which independent measurement or labeled dataset will validate the result?

Constraints:

- Compound formats include all companion files at the expected relative paths.
- Vendor metadata is trusted only after a dimensional check.
- The three imports stay separate because they represent alternative sources, not registered copies.
- Keep all file paths relative to the DREAM3D-NX application binary directory.
- Run preflight and execute the complete pipeline after any parameter or DataPath change.
- Treat example parameter values as starting points, not production acceptance limits.

Related examples:

- `(17) Scientific Volume Interoperability.d3dpipeline`
- `(14) Projection-Based Quality Summaries.d3dpipeline`

When a user asks for a new pipeline, preserve DataPath consistency between steps, validate every mathematical domain, and run the result end to end with representative data.
