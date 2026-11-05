# Write Avizo Uniform Coordinate

## Group (Subgroup)

IO (Output)

## Description

This filter writes an **Image Geometry** and its **Cell** *FeatureIds* array to a file that can be read by **Avizo**. Avizo is a commercial 3D visualization and analysis package. Its native data format is **AmiraMesh**, a header-plus-data file (commonly given an `.am` extension).

The filter requires two inputs: an **Image Geometry**, and an Int32 *FeatureIds* **Cell** **Data Array** that labels which **Feature** (for example, which grain) each cell belongs to. The *FeatureIds* values are produced earlier in the pipeline by a segmentation filter such as [Segment Features (Scalar)](ScalarSegmentFeaturesFilter.md); this filter does not compute them. If no valid *FeatureIds* array is supplied, the filter will not execute.

The file uses the **uniform-coordinate** form. Its bounding box records the first and last cell centers along each axis. Avizo derives uniform sample spacing from these bounds. The [Write Avizo Rectilinear Coordinate](WriteAvizoRectilinearCoordinateFilter.md) filter writes explicit per-axis coordinate arrays. Use the uniform variant when the grid has uniform spacing, as an **Image Geometry** does.

The *Write Binary File* parameter selects the on-disk encoding: *false* (the default) writes the data section as ASCII text, while *true* writes it in little-endian binary, producing smaller files.

**Units caveat:** the AmiraMesh header always records the coordinate units as `microns`, regardless of the actual units stored on the input **Image Geometry**. If the geometry uses different units, adjust them in Avizo after import.

### Example AmiraMesh Header

For dimensions `{3, 2, 1}`, spacing `{2, 3, 4}` microns, and origin `{10, -5, 2}` microns, the cell centers give:

- X: `11, 13, 15` microns.
- Y: `-3.5, -0.5` microns.
- Z: `4` microns.

The bounding box is `11 15 -3.5 -0.5 4 4`. Each endpoint is a sample center: `origin + spacing / 2` through `origin + (dimension - 0.5) * spacing`.

    # AmiraMesh BINARY-LITTLE-ENDIAN 2.1
    # Dimensions in x-, y-, and z-direction
    define Lattice 3 2 1
    Parameters {
        DREAM3DParams {
            Author "DREAM3D",
            DateTime "Mon Jun 1 10:01:14 2015"
        }
        Units {
            Coordinates "microns"
        }
        Content "3x2x1 int, uniform coordinates",
        # Bounding Box is xmin xmax ymin ymax zmin zmax
        BoundingBox 11 15 -3.5 -0.5 4 4
        CoordType "uniform"
    }
    Lattice { int FeatureIds } = @1
    # Data section follows

### Required Input Sources

- **Image Geometry** -- the geometry whose dimensions and bounding box are written.
- **Cell Feature Ids** -- an Int32 per-cell label array, produced by a segmentation filter such as [Segment Features (Scalar)](ScalarSegmentFeaturesFilter.md).

**Cell Feature Ids** must have one tuple per cell of the selected geometry. It can be in any Attribute Matrix.

% Auto generated parameter table will be inserted here

## Example Pipelines

## License & Copyright

Please see the description file distributed with this **Plugin**

## DREAM3D-NX Help

If you need help, need to file a bug report or want to request a new feature, please head over to the [DREAM3DNX-Issues](https://github.com/BlueQuartzSoftware/DREAM3DNX-Issues/discussions) GitHub site where the community of DREAM3D-NX users can help answer your questions.
