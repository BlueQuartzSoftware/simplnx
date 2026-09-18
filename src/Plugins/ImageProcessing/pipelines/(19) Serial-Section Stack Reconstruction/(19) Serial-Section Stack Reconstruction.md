# Serial-Section Stack Reconstruction

Executable pipeline: `(19) Serial-Section Stack Reconstruction.d3dpipeline`

> **Validation warning:** Do not treat this example as a validated acceptance procedure. Adapt and validate it for the scanner, material, resolution, and quality requirement.

## Purpose and real-world setting

A materials scientist has polished and imaged consecutive sections through a specimen. The pipeline assembles the numbered images into a volume with in-plane and section spacing ready for registration, segmentation, and 3-D analysis.

Imports an ordered set of two-dimensional sections as a physically spaced three-dimensional grayscale image and writes preview and DREAM3D outputs.

## When to use this workflow

- Assemble automated serial-section images
- Set anisotropic physical spacing
- Create a volume for later alignment and microstructure reconstruction


## Input data and assumptions

- File numbering matches physical section order.
- Images have consistent dimensions, orientation, and exposure.
- This example imports the stack but does not perform section-to-section registration.

<!-- INPUT-DATA-STRATEGY:BEGIN -->
### Input source strategy

Strategy: `synthetic-paper-inspired`.

Paper visual target: Registered serial metallography sections with persistent grains and boundaries that evolve gradually through depth.

Target traits:

- eight ordered sections
- persistent grains
- bright grain boundaries
- small section motion
- acquisition noise

Generator: `Generators/GenerateImageProcessingExampleData.py` with fixed seed `20260901`.

Generated metrics: `PaperInputMetrics.json`. Target contract: `Generators/PaperInputTargets.json`.

Known differences: The paper's case-study stacks are unavailable. The generated stack models section continuity without polishing damage, missing material, or registration correction.

See `Data/ImageProcessing_Examples/Provenance.json` for per-file transformations, checksums, dimensions, spacing, and licensing.
<!-- INPUT-DATA-STRATEGY:END -->

## Data flow

Numbered section TIFF files -> grayscale conversion -> ordered image geometry with calibrated spacing -> preview TIFF stack and DREAM3D file.

The `.d3dpipeline` file is authoritative for executable arguments. The matching `.yaml` file repeats the ordered steps in a structured form for software, LLM, and MCP use.

## Step-by-step filter explanation and parameter rationale

### 1. `ReadImageStackFilter`

Import eight numbered serial sections as a physically spaced grayscale volume.

Spacing and unit settings control the physical meaning of distance, size, and position.

Key parameters:

- `cell_attribute_matrix_name`: `"Cell Data"`
- `change_image_data_type`: `false`
- `change_origin`: `false`
- `change_spacing`: `true`
- `color_weights`: `[0.21250000596046448,0.715399980545044,0.07209999859333038]`
- `convert_to_gray_scale`: `true`
- `cropping_options`: `{"bounds_x_physical":[0.0,0.0],"bounds_x_voxels":[0,0],"bounds_y_physical":[0.0,0.0],"bounds_y_voxels":[0,0],"bounds_z_physical":[0.0,0.0],"bounds_z_voxels":[0,0],"crop_x":true,...`
- `exact_xy_dimensions`: `[256,256]`
- `image_data_array_name`: `"Section Intensity"`
- `image_data_type_index`: `0`
- `image_transform_index`: `0`
- `input_file_list_object`: `{"end_index":7,"file_extension":".tif","file_prefix":"section_","file_suffix":"","increment_index":1,"input_path":"Data/ImageProcessing_Examples/io/serial_stack","ordering":0,"p...`
- `origin`: `[0.0,0.0,0.0]`
- `origin_spacing_processing_index`: `1`
- `output_image_geometry_path`: `"Serial Sections"`
- `resample_images_index`: `0`
- `scaling`: `100.0`
- `spacing`: `[0.5,0.5,1.0]`

Resolved inputs:

- `input_file_list_object` -> `{"end_index":7,"file_extension":".tif","file_prefix":"section_","file_suffix":"","increment_index":1,"input_path":"Data/ImageProcessing_Examples/io/serial_stack","ordering":0,"p...`

Declared outputs:

- `output_image_geometry_path` -> `"Serial Sections"`

### 2. `WriteImageFilter`

Write the reconstructed sections as a TIFF stack.

The selected values preserve the demonstrated input domain and create a named result that later steps can address explicitly.

Key parameters:

- `add_scale_bar`: `false`
- `create_color_table`: `false`
- `file_name`: `"Data/Output/ImageProcessing_Examples/15_Serial_Sections/section_preview.tif"`
- `flip_mode_index`: `0`
- `image_array_path`: `"Serial Sections/Cell Data/Section Intensity"`
- `index_offset`: `0`
- `input_image_geometry_path`: `"Serial Sections"`
- `invalid_color_value`: `[0,0,0]`
- `leading_digit_character`: `"0"`
- `mask_array_path`: `""`
- `plane_index`: `0`
- `selected_preset`: `"Black-Body Radiation"`
- `total_index_digits`: `3`
- `use_mask`: `false`

Resolved inputs:

- `input_image_geometry_path` -> `"Serial Sections"`

Declared outputs:

- `file_name` -> `"Data/Output/ImageProcessing_Examples/15_Serial_Sections/section_preview.tif"`

### 3. `WriteDREAM3DFilter`

Save the complete workflow result.

The selected values preserve the demonstrated input domain and create a named result that later steps can address explicitly.

Key parameters:

- `compression_level`: `5`
- `export_file_path`: `"Data/Output/ImageProcessing_Examples/15_Serial_Sections/serial_sections.dream3d"`
- `use_compression`: `true`
- `write_xdmf_file`: `false`

Declared outputs:

- `export_file_path` -> `"Data/Output/ImageProcessing_Examples/15_Serial_Sections/serial_sections.dream3d"`

## Outputs and how to interpret them

- `Data/Output/ImageProcessing_Examples/15_Serial_Sections/section_preview.tif`
- `Data/Output/ImageProcessing_Examples/15_Serial_Sections/serial_sections.dream3d`

Intermediate arrays remain in the final DREAM3D file. Compare them in order. A plausible final mask does not prove that every upstream assumption is correct.

## How to adapt the pipeline

- Set prefix, padding, start, end, and ordering from the acquisition log.
- Use measured in-plane calibration and material-removal depth for spacing.
- Register sections before quantitative 3-D feature measurements.

Keep input and output paths relative when the pipeline must run from a DREAM3D-NX Anaconda environment or an installed application bundle.

## Failure modes and quality checks

- A missing or duplicated file changes the physical stack.
- Reversed order mirrors the section axis.
- Uncorrected drift, rotation, or polishing distortion creates false 3-D features.
- Review intermediate arrays, not only the final writer output.
- Compare measurements with an independent reference before production use.

## Scientific basis and annotated references

Workflow reproduction status: `method_reproduction`.

The paper's case-study stacks are unavailable. The generated stack models section continuity without polishing damage, missing material, or registration correction.

Paper dataset availability: The original paper input is unavailable or not redistributable. A deterministic paper-inspired synthetic input is included.

Dataset license: No separate dataset license is stated.

Original or paper-linked dataset included in the example archive: `no`.

Paper-inspired synthetic fallback included in the example archive: `yes`.

1. Jonathan E. Spowart. “Automated serial sectioning for 3-D analysis of microstructures.” *Scripta Materialia* (2006). [10.1016/j.scriptamat.2006.01.019](https://doi.org/10.1016/j.scriptamat.2006.01.019). Primary materials-science workflow for constructing three-dimensional microstructures from serial sections.

## Guidance for an LLM or MCP assistant

Use this document to explain intent and tradeoffs. Use the `.yaml` sidecar to retrieve exact structured metadata. Use the `.d3dpipeline` file as the executable source of truth, and use the `.py` companion for Anaconda execution.

Before adapting this workflow, ask:

- What imaging system, material, and acquisition mode produced the data?
- What are the voxel or pixel spacing and physical units?
- What is the smallest feature that must be retained or reported?
- Which independent measurement or labeled dataset will validate the result?

Constraints:

- File numbering matches physical section order.
- Images have consistent dimensions, orientation, and exposure.
- This example imports the stack but does not perform section-to-section registration.
- Keep all file paths relative to the DREAM3D-NX application binary directory.
- Run preflight and execute the complete pipeline after any parameter or DataPath change.
- Treat example parameter values as starting points, not production acceptance limits.

Related examples:

- `(20) Fiji Microscopy Montage Import.d3dpipeline`
- `(17) Scientific Volume Interoperability.d3dpipeline`

When a user asks for a new pipeline, preserve DataPath consistency between steps, validate every mathematical domain, and run the result end to end with representative data.
