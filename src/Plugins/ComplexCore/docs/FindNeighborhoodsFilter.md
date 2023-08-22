# Find Feature Neighborhoods 


## Group (Subgroup) ##

Statistics (Morphological)

## Description ##

This **Filter** determines the number of **Features**, for each **Feature**, whose *centroids* lie within a distance equal to a user defined multiple of the average *Equivalent Sphere Diameter* (*average of all **Features**).  The algorithm for determining the number of **Features** is given below:

1. Define a sphere centered at the **Feature**'s *centroid* and with radius equal to the average equivalent sphere diameter multiplied by the user defined multiple
2. Check every other **Feature**'s *centroid* to see if it lies within the sphere and keep count and list of those that satisfy
3. Repeat 1. & 2. for all **Features**

## Parameters ##

| Name | Type | Description |
|------|------| ----------- |
| Multiples of Average Diameter | float | Defines the search radius to use when looking for "neighboring" **Features** |

## Required Geometry ##

Image 

## Required Objects ##

| Kind | Default Name | Type | Component Dimensions | Description |
|------|--------------|------|----------------------|-------------|
| **Feature Attribute Array** | EquivalentDiameters | float | (1) | Diameter of a sphere with the same volume as the **Feature** |
| **Feature Attribute Array** | Phases | int32_t | (1) | Specifies to which **Ensemble** each **Feature** belongs |
| **Feature Attribute Array** | Centroids | float | (3) | X, Y, Z coordinates of **Feature** center of mass |

## Created Objects ##

| Kind | Default Name | Type | Component Dimensions | Description |
|------|--------------|------|----------------------|-------------|
| **Feature Attribute Array** | Neighborhoods | int32_t | (1) | Number of **Features** that have their centroid within the user specified multiple of equivalent sphere diameters from each **Feature** |
| **Feature Attribute Array** | NeighborhoodLists | List of int32_t | (1) | List of the **Features** whose centroids are within the user specified multiple of equivalent sphere diameter from each **Feature** |

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
+ Class Name: FindNeighborhoodsFilter
+ Displayed Name: Find Feature Neighborhoods

| argument key | Human Name | Description | Parameter Type |
|--------------|------------|-------------|----------------|
| centroids_array_path | Centroids | Path to the array specifying the X, Y, Z coordinates of Feature center of mass | complex.ArraySelectionParameter |
| equivalent_diameters_array_path | Equivalent Diameters | Path to the array specifying the diameter of a sphere with the same volume as the Feature | complex.ArraySelectionParameter |
| feature_phases_array_path | Phases | Path to the array specifying to which Ensemble each Feature belongs | complex.ArraySelectionParameter |
| multiples_of_average | Multiples of Average Diameter | Defines the search radius to use when looking for 'neighboring' Features | complex.Float32Parameter |
| neighborhood_list_array_name | NeighborhoodList | List of the Features whose centroids are within the user specified multiple of equivalent sphere diameter from each Feature | complex.DataObjectNameParameter |
| neighborhoods_array_name | Neighborhoods | Number of Features that have their centroid within the user specified multiple of equivalent sphere diameters from each Feature | complex.DataObjectNameParameter |
| selected_image_geometry_path | Selected Image Geometry | The target geometry | complex.GeometrySelectionParameter |

