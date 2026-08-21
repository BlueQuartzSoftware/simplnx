# Read DREAM3D-NX File

## Group (Subgroup)

IO (Input)

## Description

This filter reads data from a `.dream3d` file into the pipeline. A `.dream3d` file is an HDF5-based file (HDF5 is a portable, self-describing container format for large scientific datasets) that stores a complete DREAM3D-NX **DataStructure** — its geometries, **Attribute Matrix** groups, and **Data Array**s.

### Selective (Partial) Import

The filter does not have to read the entire file. Through the *Import File Path* parameter, the user can browse the structure of the selected `.dream3d` file and choose exactly which data objects to import. Selecting only the objects that are needed avoids loading large arrays that the rest of the pipeline does not use, which saves memory and time. To import everything, simply select all objects.

### How Imported Objects Are Added

The selected objects are added into the pipeline's current **DataStructure** at the same paths they had in the file. If an object being imported has the same path as an object already present in the current **DataStructure**, the existing object is replaced (overwritten) by the imported one, so take care when reading into a non-empty **DataStructure**. Any pipeline metadata stored in the file may also be read when the file is opened.

### Legacy Files

This filter can also read **legacy** `.dream3d` files — those written by the older DREAM3D 6.x / SIMPL-era applications. Such files use an earlier internal layout, and this filter converts them to the current **DataStructure** format on import.

### Required Input Sources

None — this filter reads directly from a `.dream3d` file on disk.

% Auto generated parameter table will be inserted here

## Example Pipelines

ALL

## License & Copyright

Please see the description file distributed with this **Plugin**

## DREAM3D-NX Help

If you need help, need to file a bug report or want to request a new feature, please head over to the [DREAM3DNX-Issues](https://github.com/BlueQuartzSoftware/DREAM3DNX-Issues/discussions) GitHub site where the community of DREAM3D-NX users can help answer your questions.
