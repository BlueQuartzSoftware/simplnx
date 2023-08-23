# Find Feature Centroids 


## Group (Subgroup) ##

Generic (Misc)

## Description ##

This **Filter** calculates the *centroid* of each **Feature** by determining the average X, Y, and Z position of all the **Cells** belonging to the **Feature**. Note that **Features** that intersect the outer surfaces of the sample will still have *centroids* calculated, but they will be *centroids* of the truncated part of the **Feature** that lies inside the sample.

## Parameters ##

None

## Required Geometry ##

Image

## Required Objects ##

| Kind | Default Name | Type | Component Dimensions | Description |
|------|--------------|------|----------------------|-------------|
| **Cell Attribute Array** | FeatureIds | int32_t | (1) | Specifies to which **Feature** each **Cell** belongs |

## Created Objects ##

| Kind | Default Name | Type | Component Dimensions | Description |
|------|--------------|------|----------------------|-------------|
| **Feature Attribute Array** | Centroids | float | (3) | X, Y, Z coordinates of **Feature** center of mass |

## Example Pipelines ##

+ (01) SmallIN100 Morphological Statistics
+ InsertTransformationPhase
+ (06) SmallIN100 Synthetic

## License & Copyright ##

Please see the description file distributed with this **Plugin**

## DREAM3DNX Help

Check out our GitHub community page at [DREAM3DNX-Issues](https://github.com/BlueQuartzSoftware/DREAM3DNX-Issues) to report bugs, ask the community for help, discuss features, or get help from the developers.

## Python Filter Arguments

+ module: complex
+ Class Name: FindFeatureCentroidsFilter
+ Displayed Name: Find Feature Centroids

| argument key | Human Name | Description | Parameter Type |
|--------------|------------|-------------|----------------|
| centroids_array_path | Centroids | DataPath to create the 'Centroids' output array | complex.DataObjectNameParameter |
| feature_attribute_matrix | Cell Feature Attribute Matrix | The cell feature attribute matrix | complex.AttributeMatrixSelectionParameter |
| feature_ids_path | Cell Feature Ids | Specifies to which Feature each cell belongs | complex.ArraySelectionParameter |
| selected_image_geometry | Selected Image Geometry | The target geometry whose Features' centroids will be calculated | complex.GeometrySelectionParameter |

