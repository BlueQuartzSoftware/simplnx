# AM Melt-Pool and Track Inspection

Executable pipeline: `(07) AM Melt-Pool and Track Inspection.d3dpipeline`

> **Validation warning:** Do not treat this example as a validated acceptance procedure. Adapt and validate it for the scanner, material, resolution, and quality requirement.

## Purpose and real-world setting

A process engineer analyzes a thermal or near-infrared frame from a laser scan. The hot core is clear, but the cooler connected boundary and ejected particles need consistent segmentation.

Enhances a thermal image, creates automatic intensity classes and a connected double-threshold mask, then labels and outlines melt-pool and spatter regions.

## When to use this workflow

- Measure melt-pool extent from a thermal frame
- Separate the primary track from spatter regions
- Compare Otsu classes with a connected hysteresis-style threshold


## Input data and assumptions

- Higher intensity represents hotter or more emissive material.
- The input is a calibrated or stable relative-intensity image.
- Thresholds do not convert intensity directly to temperature.

<!-- INPUT-DATA-STRATEGY:BEGIN -->
### Input source strategy

Strategy: `original-paper-linked`.

Paper visual target: NIST single-track thermography on an IN625 powder layer, showing a saturated hot core, trailing thermal wake, and ejected hot particles.

Target traits:

- measured camera signal
- single scan track
- hot core
- thermal wake
- spatter

Original source: [https://s3.amazonaws.com/nist-midas/1858/20170213_PowderPlate1_SingleLine.zip](https://s3.amazonaws.com/nist-midas/1858/20170213_PowderPlate1_SingleLine.zip).

Original source SHA-512: `096d3a5c1d2a25225394cdf3125e563232d51411cf20a1619d219e22b46d256ec4d76aaadff5ea1d7e3b134be0a89505ee3c08f71126c828d251695b5fb0e18b`.

Known differences: The archive stores one percentile-scaled peak frame rather than the complete 56-frame camera-signal sequence or calibrated radiant temperature.

See `Data/ImageProcessing_Examples/Provenance.json` for per-file transformations, checksums, dimensions, spacing, and licensing.
<!-- INPUT-DATA-STRATEGY:END -->

## Data flow

Thermal image -> normalized response -> sigmoid contrast -> Otsu classes plus double-threshold mask -> connected labels -> size-sorted regions -> labeled contours -> review image and DREAM3D file.

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
- `change_spacing`: `true`
- `cropping_options`: `{"bounds_x_physical":[0.0,0.0],"bounds_x_voxels":[0,0],"bounds_y_physical":[0.0,0.0],"bounds_y_voxels":[0,0],"bounds_z_physical":[0.0,0.0],"bounds_z_voxels":[0,0],"crop_x":true,...`
- `file_name`: `"Data/ImageProcessing_Examples/am/melt_pool_track.tif"`
- `image_data_array_name`: `"Thermal Intensity"`
- `image_data_type_index`: `0`
- `length_unit_index`: `6`
- `origin`: `[0.0,0.0,0.0]`
- `origin_spacing_processing_index`: `1`
- `output_geometry_path`: `"Melt Pool Image"`
- `spacing`: `[0.0333,0.0472,1.0]`

Resolved inputs:

- `file_name` -> `"Data/ImageProcessing_Examples/am/melt_pool_track.tif"`

Declared outputs:

- `output_geometry_path` -> `"Melt Pool Image"`

### 2. `NormalizeImageFilter`

Normalize the thermal response before nonlinear contrast enhancement.

The selected values preserve the demonstrated input domain and create a named result that later steps can address explicitly.

Key parameters:

- `input_image_data_path`: `"Melt Pool Image/Cell Data/Thermal Intensity"`
- `input_image_geometry_path`: `"Melt Pool Image"`
- `output_array_name`: `"Normalized Intensity"`

Resolved inputs:

- `input_image_data_path` -> `"Melt Pool Image/Cell Data/Thermal Intensity"`
- `input_image_geometry_path` -> `"Melt Pool Image"`

Declared outputs:

- `output_array_name` -> `"Normalized Intensity"`

### 3. `SigmoidImageFilter`

Increase contrast around the melt-pool response range.

The selected values preserve the demonstrated input domain and create a named result that later steps can address explicitly.

Key parameters:

- `alpha`: `-0.5`
- `beta`: `0.0`
- `input_image_data_path`: `"Melt Pool Image/Cell Data/Normalized Intensity"`
- `input_image_geometry_path`: `"Melt Pool Image"`
- `output_array_name`: `"Sigmoid Contrast"`
- `output_maximum`: `255.0`
- `output_minimum`: `0.0`

Resolved inputs:

- `input_image_data_path` -> `"Melt Pool Image/Cell Data/Normalized Intensity"`
- `input_image_geometry_path` -> `"Melt Pool Image"`

Declared outputs:

- `output_array_name` -> `"Sigmoid Contrast"`

### 4. `OtsuMultipleThresholdsImageFilter`

Partition background, track, and hot-core intensity classes automatically.

Threshold and extrema values are tuned to the included fixture and must be recalibrated for a different material or sensor.

Key parameters:

- `input_image_data_path`: `"Melt Pool Image/Cell Data/Sigmoid Contrast"`
- `input_image_geometry_path`: `"Melt Pool Image"`
- `label_offset`: `0`
- `number_of_histogram_bins`: `128`
- `number_of_thresholds`: `2`
- `output_array_name`: `"Otsu Regions"`
- `return_bin_midpoint`: `false`
- `valley_emphasis`: `true`

Resolved inputs:

- `input_image_data_path` -> `"Melt Pool Image/Cell Data/Sigmoid Contrast"`
- `input_image_geometry_path` -> `"Melt Pool Image"`

Declared outputs:

- `output_array_name` -> `"Otsu Regions"`

### 5. `DoubleThresholdImageFilter`

Create a connected melt-pool mask with a high-confidence core and a lower support threshold.

Threshold and extrema values are tuned to the included fixture and must be recalibrated for a different material or sensor. Topology and foreground conventions must match the mask and the intended definition of object contact.

Key parameters:

- `fully_connected`: `true`
- `input_image_data_path`: `"Melt Pool Image/Cell Data/Thermal Intensity"`
- `input_image_geometry_path`: `"Melt Pool Image"`
- `inside_value`: `1`
- `output_array_name`: `"Track Mask"`
- `outside_value`: `0`
- `threshold1`: `40.0`
- `threshold2`: `90.0`
- `threshold3`: `150.0`
- `threshold4`: `255.0`

Resolved inputs:

- `input_image_data_path` -> `"Melt Pool Image/Cell Data/Thermal Intensity"`
- `input_image_geometry_path` -> `"Melt Pool Image"`

Declared outputs:

- `output_array_name` -> `"Track Mask"`

### 6. `ConnectedComponentImageFilter`

Label the melt pool and separated spatter regions.

Topology and foreground conventions must match the mask and the intended definition of object contact.

Key parameters:

- `fully_connected`: `true`
- `input_image_data_path`: `"Melt Pool Image/Cell Data/Track Mask"`
- `input_image_geometry_path`: `"Melt Pool Image"`
- `output_array_name`: `"Track Labels"`

Resolved inputs:

- `input_image_data_path` -> `"Melt Pool Image/Cell Data/Track Mask"`
- `input_image_geometry_path` -> `"Melt Pool Image"`

Declared outputs:

- `output_array_name` -> `"Track Labels"`

### 7. `RelabelComponentImageFilter`

Sort the melt pool and spatter regions by area.

The selected values preserve the demonstrated input domain and create a named result that later steps can address explicitly.

Key parameters:

- `input_image_data_path`: `"Melt Pool Image/Cell Data/Track Labels"`
- `input_image_geometry_path`: `"Melt Pool Image"`
- `minimum_object_size`: `8`
- `output_array_name`: `"Sorted Track Labels"`
- `sort_by_object_size`: `true`

Resolved inputs:

- `input_image_data_path` -> `"Melt Pool Image/Cell Data/Track Labels"`
- `input_image_geometry_path` -> `"Melt Pool Image"`

Declared outputs:

- `output_array_name` -> `"Sorted Track Labels"`

### 8. `LabelContourImageFilter`

Extract the boundaries of the labeled melt-pool regions.

Topology and foreground conventions must match the mask and the intended definition of object contact.

Key parameters:

- `background_value`: `0.0`
- `fully_connected`: `true`
- `input_image_data_path`: `"Melt Pool Image/Cell Data/Sorted Track Labels"`
- `input_image_geometry_path`: `"Melt Pool Image"`
- `output_array_name`: `"Track Contours"`

Resolved inputs:

- `input_image_data_path` -> `"Melt Pool Image/Cell Data/Sorted Track Labels"`
- `input_image_geometry_path` -> `"Melt Pool Image"`

Declared outputs:

- `output_array_name` -> `"Track Contours"`

### 9. `WriteImageFilter`

Write the labeled track contours for review.

The selected values preserve the demonstrated input domain and create a named result that later steps can address explicitly.

Key parameters:

- `add_scale_bar`: `false`
- `create_color_table`: `true`
- `file_name`: `"Data/Output/ImageProcessing_Examples/03_AM_Melt_Pool/track_contours.tif"`
- `flip_mode_index`: `0`
- `image_array_path`: `"Melt Pool Image/Cell Data/Track Contours"`
- `index_offset`: `0`
- `input_image_geometry_path`: `"Melt Pool Image"`
- `invalid_color_value`: `[0,0,0]`
- `leading_digit_character`: `"0"`
- `mask_array_path`: `""`
- `plane_index`: `0`
- `selected_preset`: `"Black-Body Radiation"`
- `total_index_digits`: `3`
- `use_mask`: `false`

Resolved inputs:

- `input_image_geometry_path` -> `"Melt Pool Image"`

Declared outputs:

- `file_name` -> `"Data/Output/ImageProcessing_Examples/03_AM_Melt_Pool/track_contours.tif"`

### 10. `WriteDREAM3DFilter`

Save the complete melt-pool analysis.

The selected values preserve the demonstrated input domain and create a named result that later steps can address explicitly.

Key parameters:

- `compression_level`: `5`
- `export_file_path`: `"Data/Output/ImageProcessing_Examples/03_AM_Melt_Pool/melt_pool_inspection.dream3d"`
- `use_compression`: `true`
- `write_xdmf_file`: `false`

Declared outputs:

- `export_file_path` -> `"Data/Output/ImageProcessing_Examples/03_AM_Melt_Pool/melt_pool_inspection.dream3d"`

## Outputs and how to interpret them

- `Data/Output/ImageProcessing_Examples/03_AM_Melt_Pool/track_contours.tif`
- `Data/Output/ImageProcessing_Examples/03_AM_Melt_Pool/melt_pool_inspection.dream3d`

Intermediate arrays remain in the final DREAM3D file. Compare them in order. A plausible final mask does not prove that every upstream assumption is correct.

## How to adapt the pipeline

- Calibrate thresholds for exposure time, spectral response, and emissivity.
- Use physical pixel spacing when reporting melt-pool dimensions.
- Validate connectivity and minimum area against known spatter and sensor noise.

Keep input and output paths relative when the pipeline must run from a DREAM3D-NX Anaconda environment or an installed application bundle.

## Failure modes and quality checks

- Sensor saturation hides hot-core gradients.
- Plume radiation and reflections can look like melt-pool pixels.
- A single frame cannot establish cooling rate without timing data.
- Review intermediate arrays, not only the final writer output.
- Compare measurements with an independent reference before production use.

## Scientific basis and annotated references

Workflow reproduction status: `illustrative`.

The archive stores one percentile-scaled peak frame rather than the complete 56-frame camera-signal sequence or calibrated radiant temperature.

Paper dataset availability: Original paper-linked input data is publicly available and a measured-derived input is included in this archive.

Dataset license: NIST public data terms; review the dataset record for third-party restrictions.

Original or paper-linked dataset included in the example archive: `yes`.

Dataset record: [https://doi.org/10.18434/M31931](https://doi.org/10.18434/M31931)

1. Brandon M. Lane, Shawn P. Moylan, Eric P. Whitenton, Li Ma. “Thermographic Measurements of the Commercial Laser Powder Bed Fusion Process at NIST.” *Rapid Prototyping Journal* (2016). [10.1108/RPJ-11-2015-0161](https://doi.org/10.1108/RPJ-11-2015-0161). Primary industrial basis for thermal-image analysis of laser powder-bed fusion melt pools.

## Guidance for an LLM or MCP assistant

Use this document to explain intent and tradeoffs. Use the `.yaml` sidecar to retrieve exact structured metadata. Use the `.d3dpipeline` file as the executable source of truth, and use the `.py` companion for Anaconda execution.

Before adapting this workflow, ask:

- What imaging system, material, and acquisition mode produced the data?
- What are the voxel or pixel spacing and physical units?
- What is the smallest feature that must be retained or reported?
- Which independent measurement or labeled dataset will validate the result?

Constraints:

- Higher intensity represents hotter or more emissive material.
- The input is a calibrated or stable relative-intensity image.
- Thresholds do not convert intensity directly to temperature.
- Keep all file paths relative to the DREAM3D-NX application binary directory.
- Run preflight and execute the complete pipeline after any parameter or DataPath change.
- Treat example parameter values as starting points, not production acceptance limits.

Related examples:

- `(06) AM Powder-Bed Layer Inspection.d3dpipeline`
- `(13) Denoising and Edge Detection.d3dpipeline`

When a user asks for a new pipeline, preserve DataPath consistency between steps, validate every mathematical domain, and run the result end to end with representative data.
