# Merge Twins 


## Group (Subgroup) ##

Reconstruction (Grouping)

## Description ##

*THIS FILTER ONLY WORKS ON CUBIC-HIGH (m3m) Laue Classes.*

This **Filter** groups neighboring **Features** that are in a twin relationship with each other (currently only FCC &sigma; = 3 twins).  The algorithm for grouping the **Features** is analogous to the algorithm for segmenting the **Features** - only the average orientation of the **Features** are used instead of the orientations of the individual **Elements**.  The user can specify a tolerance on both the *axis* and the *angle* that defines the twin relationship (i.e., a tolerance of 1 degree for both tolerances would allow the neighboring **Features** to be grouped if their misorientation was between 59-61 degrees about an axis within 1 degree of <111>, since the Sigma 3 twin relationship is 60 degrees about <111>).


## Parameters ##

| Name | Type | Description |
|------|------| ----------- |
| Axis Tolerance (Degrees) | float | Tolerance allowed when comparing the axis part of the axis-angle representation of the misorientation |
| Angle Tolerance (Degrees) | float | Tolerance allowed when comparing the angle part of the axis-angle representation of the misorientation |

## Required Geometry ##

Not Applicable

## Required Objects ##

| Kind | Default Name | Type | Component Dimensions | Description |
|------|--------------|------|----------------------|-------------|
| **Feature Attribute Array** | NonContiguousNeighbors | List of int32_t | (1) | List of non-contiguous neighbors for each **Feature**. Only needed if _Use Non-Contiguous Neighbors_ is checked |
| **Feature Attribute Array** | NeighborList | List of int32_t | (1) | List of neighbors for each **Feature** |
| **Element Attribute Array** | FeatureIds | int32_t | (1) | Specifies to which **Feature** each **Element** belongs |
| **Feature Attribute Array** | Phases | int32_t | (1) | Specifies to which **Ensemble** each **Feature** belongs |
| **Feature Attribute Array** | AvgQuats | float| (4) | Specifies the average orientation of the **Feature** in quaternion representation |
| **Ensemble Attribute Array** | CrystalStructures | uint32_t | (1) | Enumeration representing the crystal structure for each **Ensemble** |

## Created Objects ##

| Kind | Default Name | Type | Component Dimensions | Description |
|------|--------------|------|----------------------|-------------|
| **Element Attribute Array** | ParentIds | int32_t | (1) | Specifies to which *parent* each **Element** belongs |
| **Attribute Matrix** | NewFeatureData | Feature | N/A | Created **Feature Attribute Matrix** name |
| **Feature Attribute Array** | ParentIds | int32_t | (1) | Specifies to which *parent* each **Feature** belongs |
| **Feature Attribute Array** | Active | bool | (1) | Specifies if the **Feature** is still in the sample (*true* if the **Feature** is in the sample and *false* if it is not). At the end of the **Filter**, all **Features** will be *Active* |


## Example Pipelines ##

+ (10) SmallIN100 Full Reconstruction
+ (06) SmallIN100 Postsegmentation Processing

## License & Copyright ##

Please see the description file distributed with this **Plugin**

## DREAM3DNX Help

Check out our GitHub community page at [DREAM3DNX-Issues](https://github.com/BlueQuartzSoftware/DREAM3DNX-Issues) to report bugs, ask the community for help, discuss features, or get help from the developers.

## Python Filter Arguments

+ module: OrientationAnalysis
+ Class Name: MergeTwinsFilter
+ Displayed Name: Merge Twins

| argument key | Human Name | Description | Parameter Type |
|--------------|------------|-------------|----------------|
| active_array_name | Active | The name of the array specifying if the Feature is still in the sample (true if the Feature is in the sample and false if it is not). At the end of the Filter, all Features will be Active | complex.DataObjectNameParameter |
| angle_tolerance | Angle Tolerance (Degrees) | Tolerance allowed when comparing the angle part of the axis-angle representation of the misorientation | complex.Float32Parameter |
| avg_quats_array_path | Average Quaternions | Specifies the average orientation of each Feature in quaternion representation | complex.ArraySelectionParameter |
| axis_tolerance | Axis Tolerance (Degrees) | Tolerance allowed when comparing the axis part of the axis-angle representation of the misorientation | complex.Float32Parameter |
| cell_parent_ids_array_name | Parent Ids | The name of the array specifying to which parent each cell belongs | complex.DataObjectNameParameter |
| contiguous_neighbor_list_array_path | Contiguous Neighbor List | List of contiguous neighbors for each Feature. | complex.NeighborListSelectionParameter |
| crystal_structures_array_path | Crystal Structures | Enumeration representing the crystal structure for each Ensemble | complex.ArraySelectionParameter |
| feature_ids_path | Cell Feature Ids | Specifies to which Feature each cell belongs | complex.ArraySelectionParameter |
| feature_parent_ids_array_name | Parent Ids | The name of the array specifying to which parent each Feature belongs | complex.DataObjectNameParameter |
| feature_phases_array_path | Phases | Specifies to which Ensemble each cell belongs | complex.ArraySelectionParameter |
| new_cell_feature_attribute_matrix_name | Feature Attribute Matrix | The name of the created cell feature attribute matrix | complex.DataObjectNameParameter |
| non_contiguous_neighbor_list_array_path | Non-Contiguous Neighbor List | List of non-contiguous neighbors for each Feature. | complex.NeighborListSelectionParameter |
| use_non_contiguous_neighbors | Use Non-Contiguous Neighbors | Whether to use a list of non-contiguous or contiguous neighbors for each feature when merging | complex.BoolParameter |

