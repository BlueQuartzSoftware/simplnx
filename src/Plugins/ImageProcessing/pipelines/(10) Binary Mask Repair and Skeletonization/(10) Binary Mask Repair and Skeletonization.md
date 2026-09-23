# Binary Mask Repair and Skeletonization

Executable pipeline: `(10) Binary Mask Repair and Skeletonization.d3dpipeline`

> **Validation warning:** Do not treat this example as a validated acceptance procedure. Adapt and validate it for the scanner, material, resolution, and quality requirement.

## Purpose and real-world setting

An analyst has a segmented crack, channel, or pore network with small gaps and specks. The analyst needs cleaned masks, object-wise morphology, and a centerline representation for connectivity measurements.

Demonstrates binary cleanup, reconstruction, topology-preserving thinning, inversion, and label-preserving object morphology on a defect or network mask.

## When to use this workflow

- Repair a binary defect mask
- Extract a skeleton for path and connectivity analysis
- Compare binary morphology with label-preserving object morphology


## Input data and assumptions

- Foreground value 1 represents the structure of interest.
- Kernel sizes are expressed in voxels and must be related to physical spacing.
- Skeleton topology is meaningful only after segmentation artifacts are removed.

<!-- INPUT-DATA-STRATEGY:BEGIN -->
### Input source strategy

Strategy: `synthetic-paper-inspired`.

Paper visual target: Thresholded desiccation-crack networks with branches, variable widths, gaps, and segmentation debris before skeletonization.

Target traits:

- connected crack network
- multiple branch nodes
- variable line width
- small gaps
- isolated specks

Generator: `Generators/GenerateImageProcessingExampleData.py` with fixed seed `20260901`.

Generated metrics: `PaperInputMetrics.json`. Target contract: `Generators/PaperInputTargets.json`.

Known differences: The soil images are unavailable. The generated mask captures network topology but not wetting-drying evolution or measured soil texture.

See `Data/ImageProcessing_Examples/Provenance.json` for per-file transformations, checksums, dimensions, spacing, and licensing.
<!-- INPUT-DATA-STRATEGY:END -->

## Data flow

Binary mask -> dilation, erosion, opening, closing, and reconstruction branches -> thinning and inversion -> connected labels -> object dilation and erosion -> DREAM3D file.

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
- `output_geometry_path`: `"Binary Mask"`
- `spacing`: `[1.0,1.0,1.0]`

Resolved inputs:

- `file_name` -> `"Data/ImageProcessing_Examples/materials/binary_mask.png"`

Declared outputs:

- `output_geometry_path` -> `"Binary Mask"`

### 2. `BinaryDilateImageFilter`

Expand the mask to bridge narrow breaks.

Scale parameters control which spatial structures are treated as noise, background, or a retained feature. Topology and foreground conventions must match the mask and the intended definition of object contact.

Key parameters:

- `background_value`: `0.0`
- `boundary_to_foreground`: `false`
- `foreground_value`: `1.0`
- `input_image_data_path`: `"Binary Mask/Cell Data/Mask"`
- `input_image_geometry_path`: `"Binary Mask"`
- `kernel_radius`: `[2,2,0]`
- `kernel_type_index`: `1`
- `output_array_name`: `"Dilated Mask"`

Resolved inputs:

- `input_image_data_path` -> `"Binary Mask/Cell Data/Mask"`
- `input_image_geometry_path` -> `"Binary Mask"`

Declared outputs:

- `output_array_name` -> `"Dilated Mask"`

### 3. `BinaryErodeImageFilter`

Shrink the mask to remove thin attachments.

Scale parameters control which spatial structures are treated as noise, background, or a retained feature. Topology and foreground conventions must match the mask and the intended definition of object contact.

Key parameters:

- `background_value`: `0.0`
- `boundary_to_foreground`: `true`
- `foreground_value`: `1.0`
- `input_image_data_path`: `"Binary Mask/Cell Data/Mask"`
- `input_image_geometry_path`: `"Binary Mask"`
- `kernel_radius`: `[2,2,0]`
- `kernel_type_index`: `1`
- `output_array_name`: `"Eroded Mask"`

Resolved inputs:

- `input_image_data_path` -> `"Binary Mask/Cell Data/Mask"`
- `input_image_geometry_path` -> `"Binary Mask"`

Declared outputs:

- `output_array_name` -> `"Eroded Mask"`

### 4. `BinaryMorphologicalOpeningImageFilter`

Remove small isolated foreground objects.

Scale parameters control which spatial structures are treated as noise, background, or a retained feature. Topology and foreground conventions must match the mask and the intended definition of object contact.

Key parameters:

- `background_value`: `0.0`
- `foreground_value`: `1.0`
- `input_image_data_path`: `"Binary Mask/Cell Data/Mask"`
- `input_image_geometry_path`: `"Binary Mask"`
- `kernel_radius`: `[2,2,0]`
- `kernel_type_index`: `1`
- `output_array_name`: `"Opened Mask"`

Resolved inputs:

- `input_image_data_path` -> `"Binary Mask/Cell Data/Mask"`
- `input_image_geometry_path` -> `"Binary Mask"`

Declared outputs:

- `output_array_name` -> `"Opened Mask"`

### 5. `BinaryMorphologicalClosingImageFilter`

Fill small gaps while preserving larger regions.

Scale parameters control which spatial structures are treated as noise, background, or a retained feature. Topology and foreground conventions must match the mask and the intended definition of object contact.

Key parameters:

- `foreground_value`: `1.0`
- `input_image_data_path`: `"Binary Mask/Cell Data/Mask"`
- `input_image_geometry_path`: `"Binary Mask"`
- `kernel_radius`: `[2,2,0]`
- `kernel_type_index`: `1`
- `output_array_name`: `"Closed Mask"`
- `safe_border`: `true`

Resolved inputs:

- `input_image_data_path` -> `"Binary Mask/Cell Data/Mask"`
- `input_image_geometry_path` -> `"Binary Mask"`

Declared outputs:

- `output_array_name` -> `"Closed Mask"`

### 6. `BinaryOpeningByReconstructionImageFilter`

Remove small objects and restore each surviving object to its original shape.

Scale parameters control which spatial structures are treated as noise, background, or a retained feature. Topology and foreground conventions must match the mask and the intended definition of object contact.

Key parameters:

- `background_value`: `0.0`
- `foreground_value`: `1.0`
- `fully_connected`: `true`
- `input_image_data_path`: `"Binary Mask/Cell Data/Mask"`
- `input_image_geometry_path`: `"Binary Mask"`
- `kernel_radius`: `[2,2,0]`
- `kernel_type_index`: `1`
- `output_array_name`: `"Reconstructed Opening"`

Resolved inputs:

- `input_image_data_path` -> `"Binary Mask/Cell Data/Mask"`
- `input_image_geometry_path` -> `"Binary Mask"`

Declared outputs:

- `output_array_name` -> `"Reconstructed Opening"`

### 7. `BinaryThinningImageFilter`

Reduce foreground tracks to one-pixel centerlines.

The selected values preserve the demonstrated input domain and create a named result that later steps can address explicitly.

Key parameters:

- `input_image_data_path`: `"Binary Mask/Cell Data/Mask"`
- `input_image_geometry_path`: `"Binary Mask"`
- `output_array_name`: `"Skeleton"`

Resolved inputs:

- `input_image_data_path` -> `"Binary Mask/Cell Data/Mask"`
- `input_image_geometry_path` -> `"Binary Mask"`

Declared outputs:

- `output_array_name` -> `"Skeleton"`

### 8. `NotImageFilter`

Invert the binary mask for background-focused processing.

The selected values preserve the demonstrated input domain and create a named result that later steps can address explicitly.

Key parameters:

- `input_image_data_path`: `"Binary Mask/Cell Data/Mask"`
- `input_image_geometry_path`: `"Binary Mask"`
- `output_array_name`: `"Inverse Mask"`

Resolved inputs:

- `input_image_data_path` -> `"Binary Mask/Cell Data/Mask"`
- `input_image_geometry_path` -> `"Binary Mask"`

Declared outputs:

- `output_array_name` -> `"Inverse Mask"`

### 9. `ConnectedComponentImageFilter`

Assign one label to each mask object.

Topology and foreground conventions must match the mask and the intended definition of object contact.

Key parameters:

- `fully_connected`: `true`
- `input_image_data_path`: `"Binary Mask/Cell Data/Mask"`
- `input_image_geometry_path`: `"Binary Mask"`
- `output_array_name`: `"Object Labels"`

Resolved inputs:

- `input_image_data_path` -> `"Binary Mask/Cell Data/Mask"`
- `input_image_geometry_path` -> `"Binary Mask"`

Declared outputs:

- `output_array_name` -> `"Object Labels"`

### 10. `DilateObjectMorphologyImageFilter`

Expand label 1 without treating all labels as one binary object.

Scale parameters control which spatial structures are treated as noise, background, or a retained feature.

Key parameters:

- `input_image_data_path`: `"Binary Mask/Cell Data/Object Labels"`
- `input_image_geometry_path`: `"Binary Mask"`
- `kernel_radius`: `[2,2,0]`
- `kernel_type_index`: `1`
- `object_value`: `1.0`
- `output_array_name`: `"Dilated Label 1"`

Resolved inputs:

- `input_image_data_path` -> `"Binary Mask/Cell Data/Object Labels"`
- `input_image_geometry_path` -> `"Binary Mask"`

Declared outputs:

- `output_array_name` -> `"Dilated Label 1"`

### 11. `ErodeObjectMorphologyImageFilter`

Shrink label 1 while preserving the other label values.

Scale parameters control which spatial structures are treated as noise, background, or a retained feature. Topology and foreground conventions must match the mask and the intended definition of object contact.

Key parameters:

- `background_value`: `0.0`
- `input_image_data_path`: `"Binary Mask/Cell Data/Object Labels"`
- `input_image_geometry_path`: `"Binary Mask"`
- `kernel_radius`: `[2,2,0]`
- `kernel_type_index`: `1`
- `object_value`: `1.0`
- `output_array_name`: `"Eroded Label 1"`

Resolved inputs:

- `input_image_data_path` -> `"Binary Mask/Cell Data/Object Labels"`
- `input_image_geometry_path` -> `"Binary Mask"`

Declared outputs:

- `output_array_name` -> `"Eroded Label 1"`

### 12. `WriteDREAM3DFilter`

Save the complete workflow result.

The selected values preserve the demonstrated input domain and create a named result that later steps can address explicitly.

Key parameters:

- `compression_level`: `5`
- `export_file_path`: `"Data/Output/ImageProcessing_Examples/06_Binary_Mask/binary_mask_repair.dream3d"`
- `use_compression`: `true`
- `write_xdmf_file`: `false`

Declared outputs:

- `export_file_path` -> `"Data/Output/ImageProcessing_Examples/06_Binary_Mask/binary_mask_repair.dream3d"`

## Outputs and how to interpret them

- `Data/Output/ImageProcessing_Examples/06_Binary_Morphology/binary_morphology.dream3d`

Intermediate arrays remain in the final DREAM3D file. Compare them in order. A plausible final mask does not prove that every upstream assumption is correct.

## How to adapt the pipeline

- Choose each kernel from the smallest physical gap or speck to change.
- Compare the skeleton with the cleaned mask before computing network statistics.
- Use object morphology when labels must remain distinct.

Keep input and output paths relative when the pipeline must run from a DREAM3D-NX Anaconda environment or an installed application bundle.

## Failure modes and quality checks

- Closing can connect defects that are physically separate.
- Opening can remove real narrow ligaments.
- Thinning preserves digital topology, not subvoxel centerline accuracy.
- Review intermediate arrays, not only the final writer output.
- Compare measurements with an independent reference before production use.

## Scientific basis and annotated references

Workflow reproduction status: `method_reproduction`.

The soil images are unavailable. The generated mask captures network topology but not wetting-drying evolution or measured soil texture.

Paper dataset availability: The original paper input is unavailable or not redistributable. A deterministic paper-inspired synthetic input is included.

Dataset license: No separate dataset license is stated.

Original or paper-linked dataset included in the example archive: `no`.

Paper-inspired synthetic fallback included in the example archive: `yes`.

1. Ce Wang, Zhan-yu Zhang, Wei Qi, Shi-min Fan. “Morphological Approach to Quantifying Soil Cracks: Application to Dynamic Crack Patterns during Wetting-Drying Cycles.” *Soil Science Society of America Journal* (2018). [10.2136/sssaj2017.03.0088](https://doi.org/10.2136/sssaj2017.03.0088). Workflow reference for binary segmentation, morphological cleanup, skeletonization, outline extraction, distance transforms, and crack-network analysis.
2. Ta-Chih Lee, Rangasami L. Kashyap, Chong-Nam Chu. “Building skeleton models via 3-D medial surface/axis thinning algorithms.” *CVGIP: Graphical Models and Image Processing* (1994). [10.1006/cgip.1994.1042](https://doi.org/10.1006/cgip.1994.1042). Foundational 3-D thinning method for topology-preserving skeleton extraction.
3. Luc Vincent. “Morphological grayscale reconstruction in image analysis: Applications and efficient algorithms.” *IEEE Transactions on Image Processing* (1993). [10.1109/83.217222](https://doi.org/10.1109/83.217222). Foundational reference for reconstruction, extrema suppression, and top-hat morphology.

## Guidance for an LLM or MCP assistant

Use this document to explain intent and tradeoffs. Use the `.yaml` sidecar to retrieve exact structured metadata. Use the `.d3dpipeline` file as the executable source of truth, and use the `.py` companion for Anaconda execution.

Before adapting this workflow, ask:

- What imaging system, material, and acquisition mode produced the data?
- What are the voxel or pixel spacing and physical units?
- What is the smallest feature that must be retained or reported?
- Which independent measurement or labeled dataset will validate the result?

Constraints:

- Foreground value 1 represents the structure of interest.
- Kernel sizes are expressed in voxels and must be related to physical spacing.
- Skeleton topology is meaningful only after segmentation artifacts are removed.
- Keep all file paths relative to the DREAM3D-NX application binary directory.
- Run preflight and execute the complete pipeline after any parameter or DataPath change.
- Treat example parameter values as starting points, not production acceptance limits.

Related examples:

- `(05) AM XCT Porosity Segmentation.d3dpipeline`
- `(12) Pore Distance and Wall-Thickness Metrology.d3dpipeline`

When a user asks for a new pipeline, preserve DataPath consistency between steps, validate every mathematical domain, and run the result end to end with representative data.
