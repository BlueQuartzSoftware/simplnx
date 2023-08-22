# Find Surface Area to Volume

## Group (Subgroup)

Statistics (Morphological)

## Description

This **Filter** calculates the ratio of surface area to volume for each **Feature** in an **Image Geometry**. First, all the boundary **Cells** are found for each **Feature**. Next, the surface area for each face that is in contact with a different **Feature** is totalled. This number is divided by the volume of each **Feature**, calculated by taking the number of **Cells** of each **Feature** and multiplying by the volume of a **Cell**.

*Note:* The surface area will be the surface area of the **Cells** in contact with the neighboring **Feature** and will be influenced by the aliasing of the structure.  As a result, the surface area to volume will likely be over-estimated with respect to the *real* **Feature**.

This filter also optionally calculate the [Sphericity](https://en.wikipedia.org/wiki/Sphericity) of each feature.

![Equation for Sphericity used in the filter](Images/Sphericity_Equation.png)

## Parameters

| Name | Type |
|------|------|
| Calculate Sphericity | Boolean |

## Required Geometry

Image

## Required Objects

| Kind | Default Name | Type | Component Dimensions | Description |
|------|--------------|------|----------------------|-------------|
| **Cell Attribute Array** | FeatureIds | int32_t | (1) | Specifies to which **Feature** each **Cell** belongs |
| **Feature Attribute Array** | NumCells |  int32_t | (1) | Number of **Cells** that are owned by the **Feature**. This value does not place any distinction between **Cells** that may be of a different size |

## Created Objects

| Kind | Default Name | Type | Component Dimensions | Description |
|------|--------------|------|----------------------|-------------|
| **Feature Attribute Array** | SurfaceAreaVolumeRatio | float | (1) | Ratio of surface area to volume for each **Feature**. The units are inverse length |
| **Feature Attribute Array** | Sphericity | float | (1) | The sphericity of each feature |

## Example Pipelines

+ (01) SmallIN100 Morphological Statistics

## License & Copyright

Please see the description file distributed with this **Plugin**

## DREAM3DNX Help

Check out our GitHub community page at [DREAM3DNX-Issues](https://github.com/BlueQuartzSoftware/DREAM3DNX-Issues) to report bugs, ask the community for help, discuss features, or get help from the developers.

## Python Filter Arguments

+ module: complex
+ Class Name: FindSurfaceAreaToVolumeFilter
+ Displayed Name: Find Surface Area to Volume & Sphericity

| argument key | Human Name | Description | Parameter Type |
|--------------|------------|-------------|----------------|
| calculate_sphericity | Calculate Sphericity | Whether or not to calculate the sphericity of each Feature | complex.BoolParameter |
| feature_ids_path | Cell Feature Ids | Specifies to which Feature each cell belongs | complex.ArraySelectionParameter |
| num_cells_array_path | Number of Cells | Number of Cells that are owned by the Feature. This value does not place any distinction between Cells that may be of a different size | complex.ArraySelectionParameter |
| selected_image_geometry | Selected Image Geometry | The target geometry | complex.GeometrySelectionParameter |
| sphericity_array_name | Sphericity Array Name | The sphericity of each feature | complex.DataObjectNameParameter |
| surface_area_volume_ratio_array_name | Surface Area to Volume Ratio | Ratio of surface area to volume for each Feature. The units are inverse length | complex.DataObjectNameParameter |

