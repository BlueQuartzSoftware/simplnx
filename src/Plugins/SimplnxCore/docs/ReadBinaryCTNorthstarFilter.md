# Read North Star Imaging CT (.nsihdr/.nsidat)

## Group (Subgroup)

IOFilters (Input)

## Description

This **Filter** will import a NorthStar Imaging data set consisting of a single .nsihdr and one or more .nsidat files. The data is read into an Image Geometry. The user can import a subvolume instead of reading the entire data set into memory.

The user should note that when using the subvolume feature that the ending voxels are **inclusive**.

The .nsihdr file will be read during preflight and the .nsidat file(s) will be extracted from there. The expectation is that the .nsidat files are in the same directory as the .nsihdr files.

### Length Unit

The *Length Unit* parameter sets the physical units associated with the **Image Geometry** spacing. This is for descriptive purposes only and does not affect the numerical values. The available choices are:

Yoctometer, Zeptometer, Attometer, Femtometer, Picometer, Nanometer, Micrometer, Millimeter, Centimeter, Decimeter, Meter, Decameter, Hectometer, Kilometer, Megameter, Gigameter, Terameter, Petameter, Exameter, Zettameter, Yottameter, Angstrom, Mil, Inch, Foot, Mile, Fathom, Unspecified, Unknown.

% Auto generated parameter table will be inserted here

## License & Copyright

Please see the description file distributed with this plugin.

## DREAM3D-NX Help

If you need help, need to file a bug report or want to request a new feature, please head over to the [DREAM3DNX-Issues](https://github.com/BlueQuartzSoftware/DREAM3DNX-Issues/discussions) GitHub site where the community of DREAM3D-NX users can help answer your questions.
