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
| Input Header File | String | Path to the .nsihdr file |
| Length Unit | integer | Enumeration value into the units table. Default is mm |
| Import Subvolume | Boolean | Import a subvolume instead of the entire volume |
| Starting Voxel | 3xInteger | The voxel indices to start the subvolume import at. |
| Ending Voxel | 3xInteger | The voxel indices to end the subvolume import at (Inclusive). |

## Required Geometry ###

Not Applicable

## Required Objects ##

Not Applicable

## Created Objects ##

| Kind | Default Name | Type | Component Dimensions | Description |
|------|--------------|------|----------------------|-------------|
| **Image Geometry** | CT Image Geometry | ImageGeom | N/A |  |
| **Attribute Matrix** | CT Scan Data | Attribute Matrix | N/A |  |
| **Data Array** | Density | float | (1) | Density Data |

## License & Copyright ##

Please see the description file distributed with this plugin.

## DREAM3DNX Help

Check out our GitHub community page at [DREAM3DNX-Issues](https://github.com/BlueQuartzSoftware/DREAM3DNX-Issues) to report bugs, ask the community for help, discuss features, or get help from the developers.

## Python Filter Arguments

+ module: complex
+ Class Name: ImportBinaryCTNorthstarFilter
+ Displayed Name: Import North Star Imaging CT (.nsihdr/.nsidat)

| argument key | Human Name | Description | Parameter Type |
|--------------|------------|-------------|----------------|
| cell_attribute_matrix_name | Cell Attribute Matrix Name | The name used to create the Cell Attribute Matrix. | complex.DataObjectNameParameter |
| density_array_name | Density Array | The name used to create the Density data array. | complex.DataObjectNameParameter |
| end_voxel_coord | Ending XYZ Voxel for Subvolume | The ending subvolume voxel (inclusive) | complex.VectorInt32Parameter |
| image_geometry_path | Image Geometry Path | The path that will be used to create the Image Geometry. | complex.DataGroupCreationParameter |
| import_subvolume | Import Subvolume | Import a subvolume instead of the entire volume | complex.BoolParameter |
| input_header_file | Input Header File | The path to the .nsihdr file | complex.FileSystemPathParameter |
| length_unit | Length Unit | The length unit that will be set into the created image geometry | complex.ChoicesParameter |
| start_voxel_coord | Starting XYZ Voxel for Subvolume | The starting subvolume voxel | complex.VectorInt32Parameter |

