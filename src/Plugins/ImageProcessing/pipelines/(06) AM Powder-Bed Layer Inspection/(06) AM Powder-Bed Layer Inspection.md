# AM Powder-Bed Layer Inspection

Executable pipeline: `(06) AM Powder-Bed Layer Inspection.d3dpipeline`

> **Validation warning:** Do not treat this example as a validated acceptance procedure. Adapt and validate it for the scanner, material, resolution, and quality requirement.

## Purpose and real-world setting

A process-monitoring engineer reviews images captured after recoating. Illumination changes across the build plate, and the engineer needs a compact anomaly mask that can feed alarms or later classification.

Enhances a short stack of powder-bed images and detects small bright layer anomalies such as streaks, clusters, and disturbed powder.

## When to use this workflow

- Find recoater streaks and powder clusters
- Normalize local contrast across uneven lighting
- Create layer-wise anomaly masks for downstream classification


## Input data and assumptions

- Anomalies of interest are bright at the selected lighting angle.
- All layer images have the same dimensions and acquisition geometry.
- The structuring element is larger than isolated defects but smaller than broad illumination trends.

<!-- INPUT-DATA-STRATEGY:BEGIN -->
### Input source strategy

Strategy: `synthetic-paper-inspired`.

Paper visual target: Post-recoat powder-bed images with streaks, bare regions, and powder agglomerates under nonuniform illumination.

Target traits:

- directional illumination
- recoater streaks
- bare patches
- bright agglomerates
- layer-to-layer variation

Generator: `Generators/GenerateImageProcessingExampleData.py` with fixed seed `20260901`.

Generated metrics: `PaperInputMetrics.json`. Target contract: `Generators/PaperInputTargets.json`.

Known differences: The production images and trained classifier are unavailable. The five generated layers reproduce visual anomaly classes but not the paper's machine, material, or classifier accuracy.

See `Data/ImageProcessing_Examples/Provenance.json` for per-file transformations, checksums, dimensions, spacing, and licensing.
<!-- INPUT-DATA-STRATEGY:END -->

## Data flow

Numbered layer images -> locally equalized volume -> median denoise -> white top-hat response -> automatic connected-component threshold -> opened anomaly mask -> TIFF stack and DREAM3D file.

The `.d3dpipeline` file is authoritative for executable arguments. The matching `.yaml` file repeats the ordered steps in a structured form for software, LLM, and MCP use.

## Step-by-step filter explanation and parameter rationale

### 1. `ReadImageStackFilter`

Import five consecutive layer images as a physically spaced volume.

Spacing and unit settings control the physical meaning of distance, size, and position.

Key parameters:

- `cell_attribute_matrix_name`: `"Cell Data"`
- `change_image_data_type`: `false`
- `change_origin`: `false`
- `change_spacing`: `true`
- `color_weights`: `[0.21250000596046448,0.715399980545044,0.07209999859333038]`
- `convert_to_gray_scale`: `false`
- `cropping_options`: `{"bounds_x_physical":[0.0,0.0],"bounds_x_voxels":[0,0],"bounds_y_physical":[0.0,0.0],"bounds_y_voxels":[0,0],"bounds_z_physical":[0.0,0.0],"bounds_z_voxels":[0,0],"crop_x":true,...`
- `exact_xy_dimensions`: `[100,100]`
- `image_data_array_name`: `"Layer Intensity"`
- `image_data_type_index`: `0`
- `image_transform_index`: `0`
- `input_file_list_object`: `{"end_index":4,"file_extension":".tif","file_prefix":"layer_","file_suffix":"","increment_index":1,"input_path":"Data/ImageProcessing_Examples/am/powder_bed_layers","ordering":0...`
- `origin`: `[0.0,0.0,0.0]`
- `origin_spacing_processing_index`: `1`
- `output_image_geometry_path`: `"Powder Bed Layers"`
- `resample_images_index`: `0`
- `scaling`: `100.0`
- `spacing`: `[0.05000000074505806,0.05000000074505806,0.05000000074505806]`

Resolved inputs:

- `input_file_list_object` -> `{"end_index":4,"file_extension":".tif","file_prefix":"layer_","file_suffix":"","increment_index":1,"input_path":"Data/ImageProcessing_Examples/am/powder_bed_layers","ordering":0...`

Declared outputs:

- `output_image_geometry_path` -> `"Powder Bed Layers"`

### 2. `AdaptiveHistogramEqualizationImageFilter`

Enhance local layer contrast under nonuniform illumination.

Scale parameters control which spatial structures are treated as noise, background, or a retained feature.

Key parameters:

- `alpha`: `0.5`
- `beta`: `0.5`
- `input_image_data_path`: `"Powder Bed Layers/Cell Data/Layer Intensity"`
- `input_image_geometry_path`: `"Powder Bed Layers"`
- `output_array_name`: `"Local Contrast"`
- `radius`: `[5,5,1]`

Resolved inputs:

- `input_image_data_path` -> `"Powder Bed Layers/Cell Data/Layer Intensity"`
- `input_image_geometry_path` -> `"Powder Bed Layers"`

Declared outputs:

- `output_array_name` -> `"Local Contrast"`

### 3. `MedianImageFilter`

Remove isolated camera noise without averaging anomaly edges.

Scale parameters control which spatial structures are treated as noise, background, or a retained feature.

Key parameters:

- `input_image_data_path`: `"Powder Bed Layers/Cell Data/Local Contrast"`
- `input_image_geometry_path`: `"Powder Bed Layers"`
- `output_array_name`: `"Median Denoised"`
- `radius`: `[1,1,0]`

Resolved inputs:

- `input_image_data_path` -> `"Powder Bed Layers/Cell Data/Local Contrast"`
- `input_image_geometry_path` -> `"Powder Bed Layers"`

Declared outputs:

- `output_array_name` -> `"Median Denoised"`

### 4. `WhiteTopHatImageFilter`

Extract bright powder clusters and streaks that are smaller than the structuring element.

Scale parameters control which spatial structures are treated as noise, background, or a retained feature.

Key parameters:

- `input_image_data_path`: `"Powder Bed Layers/Cell Data/Median Denoised"`
- `input_image_geometry_path`: `"Powder Bed Layers"`
- `kernel_radius`: `[7,7,0]`
- `kernel_type_index`: `1`
- `output_array_name`: `"Bright Anomalies"`
- `safe_border`: `true`

Resolved inputs:

- `input_image_data_path` -> `"Powder Bed Layers/Cell Data/Median Denoised"`
- `input_image_geometry_path` -> `"Powder Bed Layers"`

Declared outputs:

- `output_array_name` -> `"Bright Anomalies"`

### 5. `ThresholdMaximumConnectedComponentsImageFilter`

Select an automatic threshold that retains many meaningful layer anomalies.

Topology and foreground conventions must match the mask and the intended definition of object contact.

Key parameters:

- `input_image_data_path`: `"Powder Bed Layers/Cell Data/Bright Anomalies"`
- `input_image_geometry_path`: `"Powder Bed Layers"`
- `inside_value`: `1`
- `minimum_object_size_in_pixels`: `16`
- `output_array_name`: `"Automatic Anomaly Mask"`
- `outside_value`: `0`
- `upper_boundary`: `255.0`

Resolved inputs:

- `input_image_data_path` -> `"Powder Bed Layers/Cell Data/Bright Anomalies"`
- `input_image_geometry_path` -> `"Powder Bed Layers"`

Declared outputs:

- `output_array_name` -> `"Automatic Anomaly Mask"`

### 6. `BinaryMorphologicalOpeningImageFilter`

Remove single-pixel detections from the anomaly mask.

Scale parameters control which spatial structures are treated as noise, background, or a retained feature. Topology and foreground conventions must match the mask and the intended definition of object contact.

Key parameters:

- `background_value`: `0.0`
- `foreground_value`: `1.0`
- `input_image_data_path`: `"Powder Bed Layers/Cell Data/Automatic Anomaly Mask"`
- `input_image_geometry_path`: `"Powder Bed Layers"`
- `kernel_radius`: `[1,1,0]`
- `kernel_type_index`: `1`
- `output_array_name`: `"Cleaned Anomaly Mask"`

Resolved inputs:

- `input_image_data_path` -> `"Powder Bed Layers/Cell Data/Automatic Anomaly Mask"`
- `input_image_geometry_path` -> `"Powder Bed Layers"`

Declared outputs:

- `output_array_name` -> `"Cleaned Anomaly Mask"`

### 7. `WriteImageFilter`

Write the cleaned anomaly masks as a TIFF stack.

The selected values preserve the demonstrated input domain and create a named result that later steps can address explicitly.

Key parameters:

- `add_scale_bar`: `false`
- `create_color_table`: `false`
- `file_name`: `"Data/Output/ImageProcessing_Examples/02_AM_Powder_Bed/anomaly_mask.tif"`
- `flip_mode_index`: `0`
- `image_array_path`: `"Powder Bed Layers/Cell Data/Cleaned Anomaly Mask"`
- `index_offset`: `0`
- `input_image_geometry_path`: `"Powder Bed Layers"`
- `invalid_color_value`: `[0,0,0]`
- `leading_digit_character`: `"0"`
- `mask_array_path`: `""`
- `plane_index`: `0`
- `selected_preset`: `"Black-Body Radiation"`
- `total_index_digits`: `3`
- `use_mask`: `false`

Resolved inputs:

- `input_image_geometry_path` -> `"Powder Bed Layers"`

Declared outputs:

- `file_name` -> `"Data/Output/ImageProcessing_Examples/02_AM_Powder_Bed/anomaly_mask.tif"`

### 8. `WriteDREAM3DFilter`

Save the layer inspection results.

The selected values preserve the demonstrated input domain and create a named result that later steps can address explicitly.

Key parameters:

- `compression_level`: `5`
- `export_file_path`: `"Data/Output/ImageProcessing_Examples/02_AM_Powder_Bed/powder_bed_inspection.dream3d"`
- `use_compression`: `true`
- `write_xdmf_file`: `false`

Declared outputs:

- `export_file_path` -> `"Data/Output/ImageProcessing_Examples/02_AM_Powder_Bed/powder_bed_inspection.dream3d"`

## Outputs and how to interpret them

- `Data/Output/ImageProcessing_Examples/02_AM_Powder_Bed/anomaly_mask.tif`
- `Data/Output/ImageProcessing_Examples/02_AM_Powder_Bed/powder_bed_inspection.dream3d`

Intermediate arrays remain in the final DREAM3D file. Compare them in order. A plausible final mask does not prove that every upstream assumption is correct.

## How to adapt the pipeline

- Match image spacing and file numbering to the monitoring system.
- Choose the top-hat radius from the expected anomaly width.
- Validate the automatic threshold and minimum object size against labeled production layers.

Keep input and output paths relative when the pipeline must run from a DREAM3D-NX Anaconda environment or an installed application bundle.

## Failure modes and quality checks

- A lighting change can reverse anomaly contrast.
- Large bare regions can be removed by a top-hat radius that is too small.
- Motion between layers can make a 3-D review misleading even when each 2-D mask is correct.
- Review intermediate arrays, not only the final writer output.
- Compare measurements with an independent reference before production use.

## Scientific basis and annotated references

Workflow reproduction status: `illustrative`.

The production images and trained classifier are unavailable. The five generated layers reproduce visual anomaly classes but not the paper's machine, material, or classifier accuracy.

Paper dataset availability: The original paper input is unavailable or not redistributable. A deterministic paper-inspired synthetic input is included.

Dataset license: No separate dataset license is stated.

Original or paper-linked dataset included in the example archive: `no`.

Paper-inspired synthetic fallback included in the example archive: `yes`.

1. Luke Scime, Jack Beuth. “Anomaly detection and classification in a laser powder bed additive manufacturing process using a trained computer vision algorithm.” *Additive Manufacturing* (2018). [10.1016/j.addma.2017.11.009](https://doi.org/10.1016/j.addma.2017.11.009). Primary industrial basis for finding layer-wise powder-bed anomalies from optical images.
2. Luc Vincent. “Morphological grayscale reconstruction in image analysis: Applications and efficient algorithms.” *IEEE Transactions on Image Processing* (1993). [10.1109/83.217222](https://doi.org/10.1109/83.217222). Foundational reference for reconstruction, extrema suppression, and top-hat morphology.

## Guidance for an LLM or MCP assistant

Use this document to explain intent and tradeoffs. Use the `.yaml` sidecar to retrieve exact structured metadata. Use the `.d3dpipeline` file as the executable source of truth, and use the `.py` companion for Anaconda execution.

Before adapting this workflow, ask:

- What imaging system, material, and acquisition mode produced the data?
- What are the voxel or pixel spacing and physical units?
- What is the smallest feature that must be retained or reported?
- Which independent measurement or labeled dataset will validate the result?

Constraints:

- Anomalies of interest are bright at the selected lighting angle.
- All layer images have the same dimensions and acquisition geometry.
- The structuring element is larger than isolated defects but smaller than broad illumination trends.
- Keep all file paths relative to the DREAM3D-NX application binary directory.
- Run preflight and execute the complete pipeline after any parameter or DataPath change.
- Treat example parameter values as starting points, not production acceptance limits.

Related examples:

- `(07) AM Melt-Pool and Track Inspection.d3dpipeline`

When a user asks for a new pipeline, preserve DataPath consistency between steps, validate every mathematical domain, and run the result end to end with representative data.
