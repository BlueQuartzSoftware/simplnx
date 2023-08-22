# Find GBCD 


## Group (Subgroup) ##

Statistics (Crystallographic)

## Description ##

This **Filter** computes the 5D grain boundary character distribution (GBCD) for a **Triangle Geometry**, which is the relative area of grain boundary for a given misorientation and normal. The GBCD can be visualized by using either the [Write GBCD Pole Figure (GMT)](@ref visualizegbcdgmt) or the [Write GBCD Pole Figure (VTK)](@ref visualizegbcdpolefigure) **Filters**.

## Parameters ##

| Name | Type | Description |
|------|------| ----------- |
| GBCD Resolution (Degrees) | float | The resolution in degrees for the GBCD calculation |

## Required Geometry ##

Image + Triangle

## Required Objects (From Triangle Geometry) ##

| Kind | Default Name | Type | Component Dimensions | Description |
|------|--------------|------|----------------------|-------------|
| **Face Attribute Array** | FaceLabels | int32_t | (2) | Specifies which **Features** are on either side of each **Face** |
| **Face Attribute Array**  | FaceNormals | double | (3) | Specifies the normal of each **Face** |
| **Face Attribute Array**  | FaceAreas | double | (1) | Specifies the area of each **Face** |

## Required Objects (From Image Geometry) ##

| Kind | Default Name | Type | Component Dimensions | Description |
|------|--------------|------|----------------------|-------------|
| **Feature Attribute Array** | AvgEulerAngles | float | (3) | Three angles defining the orientation of the **Feature** in Bunge convention (Z-X-Z) |
| **Feature Attribute Array** | Phases | int32_t | (1) | Specifies to which phase each **Feature** belongs |
| **Ensemble Attribute Array** | CrystalStructures | uint32_t | (1) | Enumeration representing the crystal structure for each **Ensemble** |

## Created Objects ##

| Kind | Default Name | Type | Component Dimensions | Description |
|------|--------------|------|----------------------|-------------|
| **Ensemble Attribute Array** | GBDC | Double | (n,m,l,o,q,r) | 5 parameter GBCD data. The 6<sup>th</sup> component is used internally to track the northern vs. southern hemisphere of the Lambert sphere |


## Example Pipelines ##

+ (04) SmallIN100 GBCD

## License & Copyright ##

Please see the description file distributed with this **Plugin**

## DREAM3DNX Help

Check out our GitHub community page at [DREAM3DNX-Issues](https://github.com/BlueQuartzSoftware/DREAM3DNX-Issues) to report bugs, ask the community for help, discuss features, or get help from the developers.

## Python Filter Arguments

+ module: OrientationAnalysis
+ Class Name: FindGBCDFilter
+ Displayed Name: Find GBCD

| argument key | Human Name | Description | Parameter Type |
|--------------|------------|-------------|----------------|
| crystal_structures_array_path | Crystal Structures | Enumeration representing the crystal structure for each Ensemble | complex.ArraySelectionParameter |
| face_ensemble_attribute_matrix_name | Face Ensemble Attribute Matrix | The name of the created face ensemble attribute matrix | complex.DataObjectNameParameter |
| feature_euler_angles_array_path | Average Euler Angles | Array specifying three angles defining the orientation of the Feature in Bunge convention (Z-X-Z) | complex.ArraySelectionParameter |
| feature_phases_array_path | Phases | Specifies to which phase each Feature belongs | complex.ArraySelectionParameter |
| gbcd_array_name | GBCD | 5 parameter GBCD data. The 6th component is used internally to track the northern vs. southern hemisphere of the Lambert sphere | complex.DataObjectNameParameter |
| gbcd_resolution | GBCD Spacing (Degrees) | The resolution in degrees for the GBCD calculation | complex.Float32Parameter |
| surface_mesh_face_areas_array_path | Face Areas | Array specifying the area of each Face | complex.ArraySelectionParameter |
| surface_mesh_face_labels_array_path | Face Labels | Array specifying which Features are on either side of each Face | complex.ArraySelectionParameter |
| surface_mesh_face_normals_array_path | Face Normals | Array specifying the normal of each Face | complex.ArraySelectionParameter |
| triangle_geometry | Triangle Geometry | Path to the triangle geometry for which to calculate the GBCD | complex.GeometrySelectionParameter |

