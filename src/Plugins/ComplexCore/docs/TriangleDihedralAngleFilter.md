# TriangleDihedralAngleFilter


## Group (Subgroup) ##

Surface Meshing (Misc)

## Description ##

This **Filter** computes the minimum dihedral angle of each **Triangle** in a **Triangle Geometry** by utilizing matrix mathematics

## Parameters ##

None

## Required Geometry ##

Triangle

## Required Objects ##

None

## Created Objects ##

| Kind | Default Name | Type | Component Dimensions | Description |
|------|--------------|------|----------------------|-------------|
| **Face Attribute Array**  | dihedralAngles | double array | (1) | Specifies the minimum dihedral angle of each triangle of each **Face** |


## Example Pipelines ##

+ (03) SmallIN100 Mesh Statistics
+ Triangle_Face_Data_Demo.d3dpipeline

## License & Copyright ##

Please see the description file distributed with this **Plugin**

## DREAM.3D Mailing Lists ##

If you need more help with a **Filter**, please consider asking your question on the [DREAM.3D Users Google group!](https://groups.google.com/forum/?hl=en#!forum/dream3d-users)


## Python Filter Arguments

+ module: complex
+ Class Name: TriangleDihedralAngleFilter
+ Displayed Name: Calculate Triangle Minimum Dihedral Angle

| argument key | Human Name | Description | Parameter Type |
|--------------|------------|-------------|----------------|
| surface_mesh_triangle_dihedral_angles_array_name | Created Dihedral Angles | The name of the array storing the calculated dihedral angles | complex.DataObjectNameParameter |
| tri_geometry_data_path | Triangle Geometry | The complete path to the Geometry for which to calculate the dihedral angles | complex.GeometrySelectionParameter |

