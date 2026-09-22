# Grayscale Surface-Defect Morphology

Executable pipeline: `(11) Grayscale Surface-Defect Morphology.d3dpipeline`

> **Validation warning:** Do not treat this example as a validated acceptance procedure. Adapt and validate it for the scanner, material, resolution, and quality requirement.

## Purpose and real-world setting

A surface-inspection engineer needs to isolate scratches, pits, particles, or inclusions whose spatial scale differs from the background texture. The pipeline keeps each morphology result as an independent diagnostic branch.

Compares grayscale morphology, reconstruction, top-hat transforms, morphological gradient, fill-hole, grind-peak, and H-convex operations for bright and dark surface defects.

## When to use this workflow

- Enhance small bright or dark surface defects
- Remove peaks or fill dark basins
- Compare classical morphology with reconstruction-based morphology


## Input data and assumptions

- Defect scale can be represented by a structuring element.
- Intensity order is meaningful even if absolute calibration is not available.
- Independent branches are used for comparison; they are not a prescribed serial chain.

<!-- INPUT-DATA-STRATEGY:BEGIN -->
### Input source strategy

Strategy: `synthetic-paper-inspired`.

Paper visual target: Scale-covered steel billet surface with nonuniform texture, long dark defects, pits, and bright inclusions.

Target traits:

- rolling texture
- nonuniform brightness
- long dark defects
- dark pits
- bright inclusions

Generator: `Generators/GenerateImageProcessingExampleData.py` with fixed seed `20260901`.

Generated metrics: `PaperInputMetrics.json`. Target contract: `Generators/PaperInputTargets.json`.

Known differences: Production-line billet images are unavailable. The generated texture has analogous defect scales but no real oxide-scale physics.

See `Data/ImageProcessing_Examples/Provenance.json` for per-file transformations, checksums, dimensions, spacing, and licensing.
<!-- INPUT-DATA-STRATEGY:END -->

## Data flow

Grayscale image -> independent dilation, erosion, filling, peak removal, opening, closing, reconstruction, top-hat, gradient, and H-convex products -> DREAM3D file.

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
- `file_name`: `"Data/ImageProcessing_Examples/materials/grayscale_defects.png"`
- `image_data_array_name`: `"Defect Intensity"`
- `image_data_type_index`: `0`
- `length_unit_index`: `6`
- `origin`: `[0.0,0.0,0.0]`
- `origin_spacing_processing_index`: `1`
- `output_geometry_path`: `"Grayscale Defects"`
- `spacing`: `[1.0,1.0,1.0]`

Resolved inputs:

- `file_name` -> `"Data/ImageProcessing_Examples/materials/grayscale_defects.png"`

Declared outputs:

- `output_geometry_path` -> `"Grayscale Defects"`

### 2. `GrayscaleDilateImageFilter`

Expand local bright regions across the structuring element.

Scale parameters control which spatial structures are treated as noise, background, or a retained feature.

Key parameters:

- `input_image_data_path`: `"Grayscale Defects/Cell Data/Defect Intensity"`
- `input_image_geometry_path`: `"Grayscale Defects"`
- `kernel_radius`: `[3,3,0]`
- `kernel_type_index`: `1`
- `output_array_name`: `"Local Maximum"`

Resolved inputs:

- `input_image_data_path` -> `"Grayscale Defects/Cell Data/Defect Intensity"`
- `input_image_geometry_path` -> `"Grayscale Defects"`

Declared outputs:

- `output_array_name` -> `"Local Maximum"`

### 3. `GrayscaleErodeImageFilter`

Expand local dark regions across the structuring element.

Scale parameters control which spatial structures are treated as noise, background, or a retained feature.

Key parameters:

- `input_image_data_path`: `"Grayscale Defects/Cell Data/Defect Intensity"`
- `input_image_geometry_path`: `"Grayscale Defects"`
- `kernel_radius`: `[3,3,0]`
- `kernel_type_index`: `1`
- `output_array_name`: `"Local Minimum"`

Resolved inputs:

- `input_image_data_path` -> `"Grayscale Defects/Cell Data/Defect Intensity"`
- `input_image_geometry_path` -> `"Grayscale Defects"`

Declared outputs:

- `output_array_name` -> `"Local Minimum"`

### 4. `GrayscaleFillholeImageFilter`

Fill dark holes that do not connect to the image boundary.

Topology and foreground conventions must match the mask and the intended definition of object contact.

Key parameters:

- `fully_connected`: `true`
- `input_image_data_path`: `"Grayscale Defects/Cell Data/Defect Intensity"`
- `input_image_geometry_path`: `"Grayscale Defects"`
- `output_array_name`: `"Filled Dark Holes"`

Resolved inputs:

- `input_image_data_path` -> `"Grayscale Defects/Cell Data/Defect Intensity"`
- `input_image_geometry_path` -> `"Grayscale Defects"`

Declared outputs:

- `output_array_name` -> `"Filled Dark Holes"`

### 5. `GrayscaleGrindPeakImageFilter`

Remove bright peaks that do not connect to the image boundary.

Topology and foreground conventions must match the mask and the intended definition of object contact.

Key parameters:

- `fully_connected`: `true`
- `input_image_data_path`: `"Grayscale Defects/Cell Data/Defect Intensity"`
- `input_image_geometry_path`: `"Grayscale Defects"`
- `output_array_name`: `"Ground Peaks"`

Resolved inputs:

- `input_image_data_path` -> `"Grayscale Defects/Cell Data/Defect Intensity"`
- `input_image_geometry_path` -> `"Grayscale Defects"`

Declared outputs:

- `output_array_name` -> `"Ground Peaks"`

### 6. `GrayscaleMorphologicalClosingImageFilter`

Close small dark defects and narrow troughs.

Scale parameters control which spatial structures are treated as noise, background, or a retained feature.

Key parameters:

- `input_image_data_path`: `"Grayscale Defects/Cell Data/Defect Intensity"`
- `input_image_geometry_path`: `"Grayscale Defects"`
- `kernel_radius`: `[3,3,0]`
- `kernel_type_index`: `1`
- `output_array_name`: `"Grayscale Closed"`
- `safe_border`: `true`

Resolved inputs:

- `input_image_data_path` -> `"Grayscale Defects/Cell Data/Defect Intensity"`
- `input_image_geometry_path` -> `"Grayscale Defects"`

Declared outputs:

- `output_array_name` -> `"Grayscale Closed"`

### 7. `GrayscaleMorphologicalOpeningImageFilter`

Remove small bright defects and narrow peaks.

Scale parameters control which spatial structures are treated as noise, background, or a retained feature.

Key parameters:

- `input_image_data_path`: `"Grayscale Defects/Cell Data/Defect Intensity"`
- `input_image_geometry_path`: `"Grayscale Defects"`
- `kernel_radius`: `[3,3,0]`
- `kernel_type_index`: `1`
- `output_array_name`: `"Grayscale Opened"`
- `safe_border`: `true`

Resolved inputs:

- `input_image_data_path` -> `"Grayscale Defects/Cell Data/Defect Intensity"`
- `input_image_geometry_path` -> `"Grayscale Defects"`

Declared outputs:

- `output_array_name` -> `"Grayscale Opened"`

### 8. `OpeningByReconstructionImageFilter`

Remove small bright regions without distorting the surviving regions.

Scale parameters control which spatial structures are treated as noise, background, or a retained feature. Topology and foreground conventions must match the mask and the intended definition of object contact.

Key parameters:

- `fully_connected`: `true`
- `input_image_data_path`: `"Grayscale Defects/Cell Data/Defect Intensity"`
- `input_image_geometry_path`: `"Grayscale Defects"`
- `kernel_radius`: `[3,3,0]`
- `kernel_type_index`: `1`
- `output_array_name`: `"Opening Reconstruction"`
- `preserve_intensities`: `true`

Resolved inputs:

- `input_image_data_path` -> `"Grayscale Defects/Cell Data/Defect Intensity"`
- `input_image_geometry_path` -> `"Grayscale Defects"`

Declared outputs:

- `output_array_name` -> `"Opening Reconstruction"`

### 9. `ClosingByReconstructionImageFilter`

Fill small dark regions without distorting the surviving regions.

Scale parameters control which spatial structures are treated as noise, background, or a retained feature. Topology and foreground conventions must match the mask and the intended definition of object contact.

Key parameters:

- `fully_connected`: `true`
- `input_image_data_path`: `"Grayscale Defects/Cell Data/Defect Intensity"`
- `input_image_geometry_path`: `"Grayscale Defects"`
- `kernel_radius`: `[3,3,0]`
- `kernel_type_index`: `1`
- `output_array_name`: `"Closing Reconstruction"`
- `preserve_intensities`: `true`

Resolved inputs:

- `input_image_data_path` -> `"Grayscale Defects/Cell Data/Defect Intensity"`
- `input_image_geometry_path` -> `"Grayscale Defects"`

Declared outputs:

- `output_array_name` -> `"Closing Reconstruction"`

### 10. `BlackTopHatImageFilter`

Extract dark surface defects smaller than the structuring element.

Scale parameters control which spatial structures are treated as noise, background, or a retained feature.

Key parameters:

- `input_image_data_path`: `"Grayscale Defects/Cell Data/Defect Intensity"`
- `input_image_geometry_path`: `"Grayscale Defects"`
- `kernel_radius`: `[3,3,0]`
- `kernel_type_index`: `1`
- `output_array_name`: `"Dark Defect Response"`
- `safe_border`: `true`

Resolved inputs:

- `input_image_data_path` -> `"Grayscale Defects/Cell Data/Defect Intensity"`
- `input_image_geometry_path` -> `"Grayscale Defects"`

Declared outputs:

- `output_array_name` -> `"Dark Defect Response"`

### 11. `WhiteTopHatImageFilter`

Extract bright surface defects smaller than the structuring element.

Scale parameters control which spatial structures are treated as noise, background, or a retained feature.

Key parameters:

- `input_image_data_path`: `"Grayscale Defects/Cell Data/Defect Intensity"`
- `input_image_geometry_path`: `"Grayscale Defects"`
- `kernel_radius`: `[3,3,0]`
- `kernel_type_index`: `1`
- `output_array_name`: `"Bright Defect Response"`
- `safe_border`: `true`

Resolved inputs:

- `input_image_data_path` -> `"Grayscale Defects/Cell Data/Defect Intensity"`
- `input_image_geometry_path` -> `"Grayscale Defects"`

Declared outputs:

- `output_array_name` -> `"Bright Defect Response"`

### 12. `MorphologicalGradientImageFilter`

Measure local grayscale contrast around defect boundaries.

Scale parameters control which spatial structures are treated as noise, background, or a retained feature.

Key parameters:

- `input_image_data_path`: `"Grayscale Defects/Cell Data/Defect Intensity"`
- `input_image_geometry_path`: `"Grayscale Defects"`
- `kernel_radius`: `[3,3,0]`
- `kernel_type_index`: `1`
- `output_array_name`: `"Morphological Gradient"`

Resolved inputs:

- `input_image_data_path` -> `"Grayscale Defects/Cell Data/Defect Intensity"`
- `input_image_geometry_path` -> `"Grayscale Defects"`

Declared outputs:

- `output_array_name` -> `"Morphological Gradient"`

### 13. `HConvexImageFilter`

Retain bright peaks that rise at least 20 intensity units above their surroundings.

Threshold and extrema values are tuned to the included fixture and must be recalibrated for a different material or sensor. Topology and foreground conventions must match the mask and the intended definition of object contact.

Key parameters:

- `fully_connected`: `true`
- `height`: `20.0`
- `input_image_data_path`: `"Grayscale Defects/Cell Data/Defect Intensity"`
- `input_image_geometry_path`: `"Grayscale Defects"`
- `output_array_name`: `"Prominent Bright Defects"`

Resolved inputs:

- `input_image_data_path` -> `"Grayscale Defects/Cell Data/Defect Intensity"`
- `input_image_geometry_path` -> `"Grayscale Defects"`

Declared outputs:

- `output_array_name` -> `"Prominent Bright Defects"`

### 14. `WriteDREAM3DFilter`

Save the complete workflow result.

The selected values preserve the demonstrated input domain and create a named result that later steps can address explicitly.

Key parameters:

- `compression_level`: `5`
- `export_file_path`: `"Data/Output/ImageProcessing_Examples/07_Grayscale_Morphology/grayscale_defects.dream3d"`
- `use_compression`: `true`
- `write_xdmf_file`: `false`

Declared outputs:

- `export_file_path` -> `"Data/Output/ImageProcessing_Examples/07_Grayscale_Morphology/grayscale_defects.dream3d"`

## Outputs and how to interpret them

- `Data/Output/ImageProcessing_Examples/07_Grayscale_Morphology/grayscale_morphology.dream3d`

Intermediate arrays remain in the final DREAM3D file. Compare them in order. A plausible final mask does not prove that every upstream assumption is correct.

## How to adapt the pipeline

- Choose structuring-element shape from defect geometry.
- Choose radius from the largest defect that should remain in a top-hat response.
- Use reconstruction variants when preserving surviving object shapes is important.

Keep input and output paths relative when the pipeline must run from a DREAM3D-NX Anaconda environment or an installed application bundle.

## Failure modes and quality checks

- A radius below the defect scale suppresses the wrong background.
- Texture at the same scale as defects produces false responses.
- Border handling can change detections near the field edge.
- Review intermediate arrays, not only the final writer output.
- Compare measurements with an independent reference before production use.

## Scientific basis and annotated references

Workflow reproduction status: `method_reproduction`.

Production-line billet images are unavailable. The generated texture has analogous defect scales but no real oxide-scale physics.

Paper dataset availability: The original paper input is unavailable or not redistributable. A deterministic paper-inspired synthetic input is included.

Dataset license: No separate dataset license is stated.

Original or paper-linked dataset included in the example archive: `no`.

Paper-inspired synthetic fallback included in the example archive: `yes`.

1. Dongwook Lee, Young-il Kang, Changhyun Park, Sangchul Won. “Defect Detection Algorithm in Steel Billets Using Morphological Top-Hat filter.” *IFAC Proceedings Volumes* (2009). [10.3182/20091014-3-CL-4011.00038](https://doi.org/10.3182/20091014-3-CL-4011.00038). Workflow reference for industrial surface-defect detection under nonuniform brightness using grayscale morphology and top-hat background suppression.
2. Luc Vincent. “Morphological grayscale reconstruction in image analysis: Applications and efficient algorithms.” *IEEE Transactions on Image Processing* (1993). [10.1109/83.217222](https://doi.org/10.1109/83.217222). Foundational reference for reconstruction, extrema suppression, and top-hat morphology.

## Guidance for an LLM or MCP assistant

Use this document to explain intent and tradeoffs. Use the `.yaml` sidecar to retrieve exact structured metadata. Use the `.d3dpipeline` file as the executable source of truth, and use the `.py` companion for Anaconda execution.

Before adapting this workflow, ask:

- What imaging system, material, and acquisition mode produced the data?
- What are the voxel or pixel spacing and physical units?
- What is the smallest feature that must be retained or reported?
- Which independent measurement or labeled dataset will validate the result?

Constraints:

- Defect scale can be represented by a structuring element.
- Intensity order is meaningful even if absolute calibration is not available.
- Independent branches are used for comparison; they are not a prescribed serial chain.
- Keep all file paths relative to the DREAM3D-NX application binary directory.
- Run preflight and execute the complete pipeline after any parameter or DataPath change.
- Treat example parameter values as starting points, not production acceptance limits.

Related examples:

- `(06) AM Powder-Bed Layer Inspection.d3dpipeline`
- `(13) Denoising and Edge Detection.d3dpipeline`

When a user asks for a new pipeline, preserve DataPath consistency between steps, validate every mathematical domain, and run the result end to end with representative data.
