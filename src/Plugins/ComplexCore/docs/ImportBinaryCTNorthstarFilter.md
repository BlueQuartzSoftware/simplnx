# Import North Star Imaging CT File (Binary) #

## Group (Subgroup) ##

IOFilters (Input)

## Description ##

This **Filter** will import a NorthStar Imaging data set consisting of a single .nsihdr and one or more .nsidat files. The data is read into an Image Geometry. The user can import a subvolume instead of reading the entire data set into memory.

The user should note that when using the subvolume feature that the ending voxels are **inclusive**.

The .nsihdr file will be read during preflight and the .nsidat file(s) will be extracted from there. The expectation is that the .nsidat files are in the same directory as the .nsihdr files.

## Parameters ##

| Name | Type | Description |
|------|------|-------------|
| Input Header File | String | Path to the .nsihdr files |
| Length Unit | integer | Enumeration value into the units table. Default is mm |
| ImportSubVolume | Boolean | Is a subvolume being imported instead of the entire volume |
| Starting Voxel | 3xInteger | The voxel indices to start the subvolume import at. |
| Ending Voxel | 3xInteger | The voxel indices to end the subvolume import at (Inclusive). |
| DataContainer Name | String | Name of the DataContaienr |
| AttributeMatrix Name | String | Name of the AttributeMatrix |
| Density Array Name | String | Name of the Density data array |

## Required Geometry ###

Not Applicable

## Required Objects ##

Not Applicable

## Created Objects ##

| Kind | Default Name | Type | Component Dimensions | Description |
|------|--------------|------|----------------------|-------------|
| **Data Container** | CT DataContainer | DataContainer | N/A |  |
| **Attribute Matrix** | CT Scan Data | Attribute Matrix | N/A |  |
| **Element Attribute Array** | Density | float | (1) | Density Data|

## License & Copyright ##

Please see the description file distributed with this plugin.

## DREAM3D Mailing Lists ##

If you need more help with a filter, please consider asking your question on the DREAM3D Users mailing list:
[https://groups.google.com/forum/?hl=en#!forum/dream3d-users](https://groups.google.com/forum/?hl=en#!forum/dream3d-users)
