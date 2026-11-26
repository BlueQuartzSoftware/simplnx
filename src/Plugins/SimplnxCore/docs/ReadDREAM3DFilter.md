# Read DREAM3D-NX File

## Group (Subgroup)

IO (Input)

## Description

This filter reads data from a `.dream3d` file into the pipeline. A `.dream3d` file is an HDF5-based file (HDF5 is a portable, self-describing container format for large scientific datasets) that stores a complete DREAM3D-NX **DataStructure** — its geometries, **Attribute Matrix** groups, and **Data Array**s.

### Selective (Partial) Import

The filter does not have to read the entire file. Through the *Import File Path* parameter, the user can browse the structure of the selected `.dream3d` file and choose exactly which data objects to import. Selecting only the objects that are needed avoids loading large arrays that the rest of the pipeline does not use, which saves memory and time. To import everything, simply select all objects.

If an **Include List** path does not exist in the file, preflight reports a warning that names the missing path. Existing ancestors of that path are still imported. They may be empty groups. DREAM3D 6.5.171 did not report this warning.

### How Imported Objects Are Added

The selected objects are added into the pipeline's current **DataStructure** at the same paths they had in the file. If an object being imported has the same path as an object already present in the current **DataStructure**, the existing object is replaced (overwritten) by the imported one, so take care when reading into a non-empty **DataStructure**. Any pipeline metadata stored in the file may also be read when the file is opened.

### Legacy Files

This filter can also read **legacy** `.dream3d` files — those written by the older DREAM3D 6.x / SIMPL-era applications. Such files use an earlier internal layout, and this filter converts them to the current **DataStructure** format on import.

When simplnx converts a DREAM.3D 6.x **Data Container Reader** pipeline, it saves the legacy selection with the pipeline. A checked data container keeps its geometry, including mesh vertices and connectivity. Within that container, the reader imports the checked Attribute Matrices and Data Arrays. An unchecked container and its contents are skipped.

If the input file later gains a Data Array or Attribute Matrix inside a saved container, the reader skips that addition. A new top-level data container is imported with its contents. An empty legacy selection tree reads the entire file.

If you converted the pipeline with an earlier version of simplnx, convert the original SIMPL JSON again to recover its checkbox settings. The earlier conversion discarded the unchecked flags.

### Use Default Origin/Spacing for Invalid Values

Stored image origin components must be finite. Stored spacing components must be finite and greater than zero. By default, the filter rejects invalid values in image geometries selected for import.

Enable **Use Default Origin/Spacing for Invalid Values** to use 0 for each invalid origin component and 1 for each invalid spacing component in the loaded image geometry. These defaults use the same length unit as the stored values. The file is not changed. The filter reports warning `502693` for each imported image geometry that uses a default. The warning lists the file, geometry path, stored values, and defaults used.

The filter checks bounds on each Rectilinear Grid Geometry selected for import. Each bounds array must have one component per tuple, with at least two finite values in strictly increasing order. The default-values option applies only to Image Geometry metadata.

### Required Input Sources

None — this filter reads directly from a `.dream3d` file on disk.

% Auto generated parameter table will be inserted here

## Example Pipelines

ALL

## License & Copyright

Please see the description file distributed with this **Plugin**

## DREAM3D-NX Help

If you need help, need to file a bug report or want to request a new feature, please head over to the [DREAM3DNX-Issues](https://github.com/BlueQuartzSoftware/DREAM3DNX-Issues/discussions) GitHub site where the community of DREAM3D-NX users can help answer your questions.
