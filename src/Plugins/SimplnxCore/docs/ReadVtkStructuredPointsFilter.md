# Read Vtk Structured Points File

## Group (Subgroup)

IO (Input)

## Description

This filter reads a *STRUCTURED_POINTS* 3D array from a legacy `.vtk` file and stores it in an **Image Geometry** (a regular grid of equally-sized voxels). A *STRUCTURED_POINTS* file is a general form of an **Image Geometry** in which data values can be attached either to each voxel or to the corner points of each voxel. The file can be either binary or ASCII. The currently supported VTK attribute types are *SCALARS* and *VECTORS*; other attribute types are not read correctly and may cause the filter to fail.

### Point Data vs. Cell Data

A *STRUCTURED_POINTS* file can store its values in two ways, and a single file may contain one or both:

- ***CELL_DATA***: one value per voxel, conceptually located at the voxel's center. This is the most common case and maps directly onto an **Image Geometry** **Cell Attribute Matrix**.
- ***POINT_DATA***: values located at the eight corner (vertex) points of each voxel, so there are more point values than there are voxels.

The filter can read either or both, controlled by the *Read Point Data* and *Read Cell Data* options. To keep the data easy to visualize and compatible with the many DREAM3D-NX analysis tools that require an **Image Geometry**, the filter imports *both* kinds of data into their own separate **Image Geometry**, since both a cell-center grid and a corner-point grid form a structured rectilinear grid.

When both are read, two independent outputs are created: the **Cell Data** is placed under the **Image Geometry** named by the *Data Container [Cell Data]* parameter (default `VTK Cell Data`), and the **Point Data** is placed under the **Image Geometry** named by the *Data Container [Point Data]* parameter (default `VTK Point Data`). Each gets its own **Cell Attribute Matrix** to hold the imported arrays.

### Spacing, Origin, and Units

The grid dimensions, spacing, and origin are read directly from the `.vtk` file's `DIMENSIONS`, `SPACING`, and `ORIGIN` records. The spacing and origin are plain numbers with no embedded unit, so their length unit is whatever the file's author intended (dimensionless to the reader).

### Example Input

    # vtk DataFile Version 2.0
    GrainIds Stored in Vtk File
    ASCII
    DATASET STRUCTURED_POINTS
    DIMENSIONS 3 4 6
    SPACING 1 1 1
    ORIGIN 0 0 0
    POINT_DATA 72
    SCALARS GrainIds char 1
    LOOKUP_TABLE default
    0 0 0 0 0 0 0 0 0 0 0 0
    0 5 10 15 20 25 25 20 15
    10 5 0 0 10 20 30 40 50
    50 40 30 20 10 0 0 10 20
    30 40 50 50 40 30 20 10 0
    0 5 10 15 20 25 25 20 15
    10 5 0 0 0 0 0 0 0 0 0 0 0 0 0

### Required Input Sources

None — this filter reads directly from a `.vtk` file on disk.

% Auto generated parameter table will be inserted here

## Example Pipelines

## License & Copyright

Please see the description file distributed with this **Plugin**

## DREAM3D-NX Help

If you need help, need to file a bug report or want to request a new feature, please head over to the [DREAM3DNX-Issues](https://github.com/BlueQuartzSoftware/DREAM3DNX-Issues/discussions) GitHub site where the community of DREAM3D-NX users can help answer your questions.
