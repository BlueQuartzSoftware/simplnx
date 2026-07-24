# Write Avizo Uniform Coordinate

## Group (Subgroup)

IO (Output)

## Description

This filter writes an **Image Geometry** and its **Cell** *FeatureIds* array to a file that can be read by **Avizo**. Avizo is a commercial 3D visualization and analysis package. Its native data format is **AmiraMesh**, a header-plus-data file (commonly given an `.am` extension).

The filter requires two inputs: an **Image Geometry**, and an Int32 *FeatureIds* **Cell** **Data Array** that labels which **Feature** (for example, which grain) each cell belongs to. The *FeatureIds* values are produced earlier in the pipeline by a segmentation filter such as [Segment Features (Scalar)](ScalarSegmentFeaturesFilter.md); this filter does not compute them. If no valid *FeatureIds* array is supplied, the filter will not execute.

The data is written in the **uniform-coordinate** form: the file records a single uniform spacing together with the grid's bounding box (minimum and maximum X, Y, and Z), rather than listing per-axis coordinate values. This is the compact counterpart to the [Write Avizo Rectilinear Coordinate](WriteAvizoRectilinearCoordinateFilter.md) filter, which writes explicit per-axis coordinate arrays. Use the uniform variant when the grid has uniform spacing (the usual case for an **Image Geometry**).

The *Write Binary File* parameter selects the on-disk encoding: *false* (the default) writes the data section as ASCII text, while *true* writes it in little-endian binary, producing smaller files.

**Units caveat:** the AmiraMesh header always records the coordinate units as `microns`, regardless of the actual units stored on the input **Image Geometry**. If the geometry uses different units, adjust them in Avizo after import.

### Example AmiraMesh Header

    # AmiraMesh BINARY-LITTLE-ENDIAN 2.1
    # Dimensions in x-, y-, and z-direction
    define Lattice
    define Coordinates
    Parameters {
        DREAM3DParams {
            Author "DREAM3D",
            DateTime "Mon Jun 1 10:01:14 2015"
        }
        Units {
            Coordinates "microns"
        }
        Content "int, uniform coordinates",
        # Bounding Box is xmin xmax ymin ymax zmin zmax
        BoundingBox 0 64 0 64 0 64,
        CoordType "uniform"
    }
    Lattice { int FeatureIds } = @1
    # Data section follows

### Required Input Sources

- **Image Geometry** -- the geometry whose dimensions and bounding box are written.
- **Cell Feature Ids** -- an Int32 per-cell label array, produced by a segmentation filter such as [Segment Features (Scalar)](ScalarSegmentFeaturesFilter.md).

% Auto generated parameter table will be inserted here

## Example Pipelines

## License & Copyright

Please see the description file distributed with this **Plugin**

## DREAM3D-NX Help

If you need help, need to file a bug report or want to request a new feature, please head over to the [DREAM3DNX-Issues](https://github.com/BlueQuartzSoftware/DREAM3DNX-Issues/discussions) GitHub site where the community of DREAM3D-NX users can help answer your questions.
