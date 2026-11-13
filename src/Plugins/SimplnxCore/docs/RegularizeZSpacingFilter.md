# Regularize Z Spacing

## Group (Subgroup)

Sampling (Resolution)

## Description

This **Filter** resamples an **Image Geometry** that has *irregular* spacing along the Z axis onto a *regular* (uniform) Z spacing. This situation commonly arises from serial-sectioning data collection where the physical distance removed between successive sections is not constant.

The current physical Z position of every original slice boundary is read from a whitespace-delimited text file. The file must contain **(ZPoints + 1)** floating point values, where `ZPoints` is the number of cells (slices) along the Z axis of the selected **Image Geometry**. The first value is the position of the bottom boundary of the first slice, and the last value is the total Z extent of the volume.

The **Filter** computes a new number of Z slices as:

    newZPoints = floor(totalZExtent / newZSpacing)

(with a minimum of 1). For each new, evenly spaced Z plane the **Filter** selects the original slice whose boundary interval contains that plane's position and copies the entire XY slab of cell data from the original slice into the new slice. The X and Y dimensions, spacing, and the geometry origin are unchanged; only the Z dimension count and Z spacing are updated.

### Input File Format

The input file is a simple text file with one floating point value per line (or whitespace-delimited). For an **Image Geometry** with 5 Z slices the file must contain 6 values, for example:

    0.0
    1.0
    3.0
    6.0
    9.5
    12.0

The values must be monotonically non-decreasing, and the final value (the total Z extent) must be greater than zero. The file must have a `.txt`, `.dat`, or `.csv` extension.

### Output Geometry

When *Perform In Place* is enabled the original **Image Geometry** is replaced by the resampled result. When it is disabled the resampled result is written to the *Created Image Geometry* path and the original geometry is left unchanged.

## Feature Attribute Matrix

A **Feature** is a group of cells with the same Feature Id. A **Feature Attribute Matrix** stores one row for each Feature Id. Resampling along Z can remove features and change their cell counts. Treat every feature value computed before this filter as invalid for the resampled geometry. Renumbering does not recompute these values.

*Clear Feature Attribute Matrix* is on by default. It recreates the selected matrix with no arrays. Numeric arrays, string arrays, and **NeighborLists** are cleared. Recompute any arrays needed by later filters. A later filter that selects a cleared array fails preflight until that array is created again. Pipelines converted from DREAM.3D 6.x start with Clear off, because the old filter copied the Feature data unchanged.

### Clearing and Renumbering Modes

| Clear Feature Attribute Matrix | Renumber Features | Result |
|---|---|---|
| On | On | Remaining Feature Ids become 1 through N. The empty matrix has N+1 rows, including row 0. |
| On | Off (default) | The empty matrix keeps its source tuple shape. Feature Ids keep their original labels. Rows for removed features are unused. |
| Off | Hidden and ignored | All Attribute Matrices other than the Cell Attribute Matrix are copied unchanged. Feature Ids keep their original labels. Copied feature values still describe the original geometry. |

*Feature Attribute Matrix*, *Renumber Features*, and *Cell Feature Ids* are shown only when Clear is on. Both selection paths are required, even when Renumber is off. Select an `int32` array with one component for *Cell Feature Ids*. When Clear and Renumber are both on, this array must belong to the geometry's Cell Attribute Matrix. It must have one tuple per cell. Then every source Feature Id must be nonnegative and smaller than the source Feature Attribute Matrix tuple count. This check also covers slices that the resample does not use. When Clear is off, both selections and any saved Renumber setting are ignored.

Other Attribute Matrices, including the **Ensemble Attribute Matrix**, are copied unchanged in every mode. To keep the source feature values for comparison, turn *Perform In Place* off. Select a new *Created Image Geometry* path.

### Preflight Information and Errors

**Cleared Feature Data** lists the full paths of all children of the selected Feature Attribute Matrix. Use this list to decide what to recompute. For example, run [Compute Feature Neighbors](ComputeFeatureNeighborsFilter.md) again if later filters need neighbor lists. When Renumber is on, this value also describes the new Feature Ids and matrix size.

When Clear is off, **Feature Data Not Cleared** explains that copied feature values describe the original geometry. These values are information, not warnings.

When Clear is on, the selected Feature Attribute Matrix must be a direct child of *Selected Image Geometry*. It must not be that geometry's Cell Attribute Matrix. These checks apply even when Renumber is off. The Cell Feature Ids location and tuple-count checks apply only when Clear and Renumber are both on.

- **-67431**: The Feature Attribute Matrix is not a direct child of the selected Image Geometry. Select an Attribute Matrix that is a direct child of the geometry, or turn off 'Clear Feature Attribute Matrix'.
- **-67432**: The selected matrix is the geometry's Cell Attribute Matrix. Select the matrix that holds feature data, or turn Clear off.
- **-67433**: With Clear and Renumber on, Cell Feature Ids must belong to the geometry's Cell Attribute Matrix. Select the array from that matrix.
- **-67434**: With Clear and Renumber on, Cell Feature Ids must have one tuple per cell. Select an array with the same tuple count as the geometry's cell count.

When Clear Feature Attribute Matrix is on, error **-67460** rejects a matrix registered as Cell, Vertex, Edge, Face, or Polyhedron data by any geometry. This also applies to matrices shared through links. Select a Feature Attribute Matrix that is not registered as element data, or turn Clear off.

% Auto generated parameter table will be inserted here

## Example Pipelines

## License & Copyright

Please see the description file distributed with this **Plugin**

## DREAM3D-NX Help

If you need help, need to file a bug report or want to request a new feature, please head over to the [DREAM3DNX-Issues](https://github.com/BlueQuartzSoftware/DREAM3DNX-Issues/discussions) GitHub site where the community of DREAM3D-NX users can help answer your questions.
