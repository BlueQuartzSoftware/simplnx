# Read North Star Imaging CT (.nsihdr/.nsidat)

## Group (Subgroup)

IO (Input)

## Description

This **Filter** will import a NorthStar Imaging computed tomography (CT) data set consisting of a single .nsihdr and one or more .nsidat files. The data is read into an **Image Geometry**. The scan intensities are stored in a single-component `float32` array (default name *Density*) inside a **Cell Attribute Matrix** (default name *CT Scan Data*).

The user can import a subvolume instead of reading the entire data set into memory. The subvolume start and end values are specified in **voxels** (zero-based), and both the starting and ending voxels are **inclusive**.

The .nsihdr file will be read during preflight and the .nsidat file(s) will be extracted from there. The expectation is that the .nsidat files are in the same directory as the .nsihdr files.

### Length Unit

The *Length Unit* parameter sets the physical units associated with the **Image Geometry** spacing. This is for descriptive purposes only and does not affect the numerical values. The available choices are:

Yoctometer, Zeptometer, Attometer, Femtometer, Picometer, Nanometer, Micrometer, Millimeter, Centimeter, Decimeter, Meter, Decameter, Hectometer, Kilometer, Megameter, Gigameter, Terameter, Petameter, Exameter, Zettameter, Yottameter, Angstrom, Mil, Inch, Foot, Mile, Fathom, Unspecified, Unknown.

## Required Input Sources

None — this filter reads directly from a `.nsihdr`/`.nsidat` file set on disk.

### Geometry Metadata Overrides

**Set Origin** replaces the input origin with the X, Y, and Z coordinates you provide. **Set Spacing** replaces the input spacing with the X, Y, and Z spacings you provide. Origins must be finite. Spacing must be finite and greater than zero.

The header minimum defines the origin. The filter divides the range from the minimum to the maximum on each axis by that axis's voxel count to get the spacing. Each voxel count must be greater than zero.

If a coordinate cannot be converted to float32, the error gives the header path, coordinate, value, and settings to enable. Enable **Set Spacing** if the maximum coordinate cannot be converted or the calculated spacing is outside the float32 range. Enable both **Set Origin** and **Set Spacing** if the minimum coordinate cannot be converted. The filter applies the overrides before it calculates a cropped volume origin.

% Auto generated parameter table will be inserted here

## Example Pipelines

+ 18_Industrial_XCT_Format_Import.d3dpipeline

## License & Copyright

Please see the description file distributed with this plugin.

## DREAM3D-NX Help

If you need help, need to file a bug report or want to request a new feature, please head over to the [DREAM3DNX-Issues](https://github.com/BlueQuartzSoftware/DREAM3DNX-Issues/discussions) GitHub site where the community of DREAM3D-NX users can help answer your questions.
