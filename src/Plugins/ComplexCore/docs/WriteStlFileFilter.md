# Export STL Files From Triangle Geometry

## Group (Subgroup)

IO (Output)

## Description

This **Filter** will write a binary STL File for each unique **Feature** Id in the associated **Triangle** geometry. The STL files will be named with the [Feature_Id].stl. The user can designate an optional prefix for the files.

## Parameters

| Name | Type | Description |
|------|------|------|
| Group by Feature Phases | bool | When true this allows the STL files to be further broken down by Phases |
| Output STL Directory | File Path | The output directory path where all of the individual STL files will be saved |
| STL File Prefix | String | Optional Prefix to use when creating the STL file names |

## Required Geometry

Triangle

## Required Objects

| Kind | Default Name | Type | Component Dimensions | Description |
|------|--------------|------|----------------------|-------------|
| **Face Data Array** | Face Labels | int32 | (2)  | Specifies which **Features** are on either side of each **Face** |
| **Phases Data Array** | Feature Phases | int32 | (2)  | Specifies which **Phases** of **Features** are on either side of each **Face** |
| **Normals Data Array** | Face Normals | int32 | (2)  | These are the face normals to be printed to the stl |

## Created Objects

None

## License & Copyright

Please see the description file distributed with this **Plugin**

## DREAM.3D Mailing Lists

If you need more help with a **Filter**, please consider asking your question on the [DREAM.3D Users Google group!](https://groups.google.com/forum/?hl=en#!forum/dream3d-users)


## Python Filter Arguments

+ module: complex
+ Class Name: WriteStlFileFilter
+ Displayed Name: Export STL Files from Triangle Geometry

| argument key | Human Name | Description | Parameter Type |
|--------------|------------|-------------|----------------|
| feature_ids_path | Face labels | The triangle feature ids array to order/index files by | complex.ArraySelectionParameter |
| feature_phases_path | Feature Phases | The feature phases array to further order/index files by | complex.ArraySelectionParameter |
| group_by_feature | Group by Feature Phases | Further partition the stl files by feature phases | complex.BoolParameter |
| output_stl_directory | Output STL Directory | Directory to dump the STL file(s) to | complex.FileSystemPathParameter |
| output_stl_prefix | STL File Prefix | The prefix name of created files (other values will be appended later - including the .stl extension) | complex.StringParameter |
| triangle_geom_path | Selected Triangle Geometry | The geometry to print | complex.GeometrySelectionParameter |

