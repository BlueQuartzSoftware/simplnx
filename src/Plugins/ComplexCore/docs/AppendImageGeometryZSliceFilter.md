# Append Z-Slice 


## Group (Subgroup)

Sampling (Memory/Management)


## Description

This filter allows the user to append an Image Geometry onto the "end" of another Image Geometry. The input and 
destination **ImageGeometry** objects must have the same X&Y dimensions. Optional Checks for equal **Resolution** values 
can also be performed.

For example, if the user has an already existing **Image Geometry** that is 100 voxels in the *X* direction by 200 pixels in the 
*Y* direction and composed of 5 *Z* slices then appending another data set that is the same dimensions in X & Y but contains
10 *Z* slices then the resulting **Image Geometry** will have a total of 15 *Z* slices.


## Parameters

| Name             | Type | Description |
|------------------|------| ------------|
| Check Resolution | Boolean | Checks to make sure the spacing for the input geometry and destination geometry match |
| Save as new geometry | Boolean | Instead of overwritting the existing destination geometry when appending the input data, save the results to a new geometry |
| New Image Geometry | DataPath | The full path to the new image geometry where the appended results are stored when the save as new geometry is on |

## Required Geometry

Image

## Required Arrays

| Kind | Default Name | Type | Component Dimensions | Description |
|------|--------------|------|----------------------|-------------|
| **Attribute Matrix** | Input Cell Data | Cell | N/A | The incoming cell data that is to be appended. |
| **Attribute Matrix** | Destination Cell Data | Cell | N/A | The destination cell data that is the final location for the appended data. |



## Example Pipelines



## License & Copyright

Please see the description file distributed with this **Plugin**


## DREAM.3D Mailing Lists

If you need more help with a **Filter**, please consider asking your question on the [DREAM.3D Users Google group!](https://groups.google.com/forum/?hl=en#!forum/dream3d-users)









## Python Filter Arguments

+ module: complex
+ Class Name: AppendImageGeometryZSliceFilter
+ Displayed Name: Append Z Slice (Image Geometry)

| argument key | Human Name | Description | Parameter Type |
|--------------|------------|-------------|----------------|
| check_resolution | Check Spacing | Checks to make sure the spacing for the input geometry and destination geometry match | complex.BoolParameter |
| destination_geometry | Destination Image Geometry | The destination image geometry (cell data) that is the final location for the appended data. | complex.GeometrySelectionParameter |
| input_geometry | Input Image Geometry | The incoming image geometry (cell data) that is to be appended. | complex.GeometrySelectionParameter |
| new_geometry | New Image Geometry | The path to the new geometry with the combined data from the input & destination geometry | complex.DataGroupCreationParameter |
| save_as_new_geometry | Save as new geometry | Save the combined data as a new geometry instead of appending the input data to the destination geometry | complex.BoolParameter |

