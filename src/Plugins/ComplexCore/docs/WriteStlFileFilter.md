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

| Kind                      | Default Name | Type     | Comp Dims | Description                                 |
|---------------------------|--------------|----------|--------|---------------------------------------------|
| Face Data Array | Face Labels | int32 | (2)  | Specifies which **Features** are on either side of each Face |
| Phases Data Array | Feature Phases | int32 | (2)  | Specifies which **Phases** of **Features** are on either side of each Face |
| Normals Data Array | Face Normals | int32 | (2)  | These are the face normals to be printed to the stl |

## Created Objects

None

## License & Copyright

Please see the description file distributed with this **Plugin**

## DREAM3DNX Help

Check out our GitHub community page at [DREAM3DNX-Issues](https://github.com/BlueQuartzSoftware/DREAM3DNX-Issues) to report bugs, ask the community for help, discuss features, or get help from the developers.
