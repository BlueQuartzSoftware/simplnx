# Find Feature Neighbor Misorientations 


## Group (Subgroup) ##

Statistics (Crystallographic)

## Description ##

This **Filter** determines, for each **Feature**, the misorientations with each of the **Features** that are in contact with it.  The misorientations are stored as a list (for each **Feature**) of angles (in degrees).  The axis of the misorientation is not stored by this **Filter**.

The user can also calculate the average misorientation between the feature and all contacting features.

### Notes ###

__NOTE:__ Only features with identical crystal structures will be calculated. If two features have different crystal structures then a value of NaN is set for the misorientation.

## Parameters ##

| Name | Type | Description |
|------|------|-------------|
| Find Average Misorientation Per Feature | bool | Specifies if the *average* of the misorienations with the neighboring **Features** should be stored for each **Feature** |

## Required Geometry ##

Not Applicable

## Required Objects ##

| Kind | Default Name | Type | Component Dimensions | Description |
|------|--------------|------|----------------------|-------------|
| **Feature Attribute Array** | NeighborLists | List of int32_t | (1) | List of the contiguous neighboring **Features** for a given **Feature** |
| **Feature Attribute Array** | AvgQuats | float | (4) | Defines the average orientation of the **Feature** in quaternion representation |
| **Feature Attribute Array** | Phases | int32_t | (1) | Specifies to which **Ensemble** each **Feature** belongs |
| **Ensemble Attribute Array** | CrystalStructures | uint32_t | (1) | Enumeration representing the crystal structure for each **Ensemble** |

## Created Objects ##

| Kind | Default Name | Type | Component Dimensions | Description |
|------|--------------|------|----------------------|-------------|
| **Feature Attribute Array** | MisorientationLists | List of float | (1) | List of the misorientation angles with the contiguous neighboring **Features** for a given **Feature** |
| **Feature Attribute Array** | AvgMisorientation | float | (1) | Number weighted average of neighbor misorientations. Only created if _Find Average Misorientation Per Feature_ is checked |


## Example Pipelines ##

+ (05) SmallIN100 Crystallographic Statistics

## License & Copyright ##

Please see the description file distributed with this **Plugin**

## DREAM3DNX Help

Check out our GitHub community page at [DREAM3DNX-Issues](https://github.com/BlueQuartzSoftware/DREAM3DNX-Issues) to report bugs, ask the community for help, discuss features, or get help from the developers.

## Python Filter Arguments

+ module: OrientationAnalysis
+ Class Name: FindMisorientationsFilter
+ Displayed Name: Find Feature Neighbor Misorientations

| argument key | Human Name | Description | Parameter Type |
|--------------|------------|-------------|----------------|
| avg_misorientations_array_name | Average Misorientations | The name of the array containing the number weighted average of neighbor misorientations. | complex.DataObjectNameParameter |
| avg_quats_array_path | Feature Average Quaternions | Defines the average orientation of the Feature in quaternion representation | complex.ArraySelectionParameter |
| crystal_structures_array_path | Crystal Structures | Enumeration representing the crystal structure for each Ensemble | complex.ArraySelectionParameter |
| feature_phases_array_path | Feature Phases | Specifies to which Ensemble each Feature belongs | complex.ArraySelectionParameter |
| find_avg_misors | Find Average Misorientation Per Feature | Specifies if the average of the misorienations with the neighboring Features should be stored for each Feature | complex.BoolParameter |
| misorientation_list_array_name | Misorientation List | The name of the data object containing the list of the misorientation angles with the contiguous neighboring Features for a given Feature | complex.DataObjectNameParameter |
| neighbor_list_array_path | Feature Neighbor List | List of the contiguous neighboring Features for a given Feature | complex.NeighborListSelectionParameter |

