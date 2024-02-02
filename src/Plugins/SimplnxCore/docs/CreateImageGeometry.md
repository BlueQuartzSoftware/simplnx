# Create Geometry (Image)  

## This filter should be considered deprecated. Use the "Create Geometry" filter instead

## Group (Subgroup)

Core (Generation)

## Description

This **Filter** creates an **Image Geometry** specifically for the representation of a 3D rectilinear grid of voxels (3D) or pixels
(2D). Each axis can have its starting point (origin), resolution, and length defined for the **Geometry**. The **Data Container** in which to place the **Image Geometry** must be specified.

An **Image Geometry** is a *grid-like* **Geometry**, and is the simplest and most widely used of the basic **Geometry** types.  An **Image Geometry** is a *regular, rectilinear grid*; if the *dimenionality* of the image is *d*, then only *3*d* numbers are needed to completely define the **Geometry**: three *d*-vectors for the *dimensions*, *origin*, and *spacing*.

- Dimensions define the extents of the grid. Stored as unsigned 64 bit integers. The dimensions are also known as the **extents** and are *zero* based thus a dimension with a value of 10 has extents from 0-9.
- Spacing defines the physical distance between grid planes for each orthogonal direction (constant along a given direction). Stored as 32 bit floating point numbers. Spacing has been known in the past as *resolution* but this term is ambiguous so spacing is used. A value of "microns per pixel" is a good example of "Spacing" units.
- Origin defines the physical location of the *bottom left* grid point in *d*-dimensional space. Stored as 32 bit floating point numbers.

All **Image Geometries** in **DREAM3D-NX** are defined as 3D images.  A 2D image is assumed when one of the dimension values is exactly 1; the 2D image is then considered a plane.  Most **DREAM3D-NX** **Filters** will properly take account for the **Image** dimension if it matters (for example, the Find Feature Shapes **Filter** accounts for whether the **Image** is 2D or 3D when computing values such as *aspect ratios* or *axis Euler angles*).  No dimension may be negative or equal to 0.  The spacing must be a positive, non-zero value. The Origin has no value restrictions.  This **Filter** requires the user to enter the nine values for the dimenions, origin, and spacing.

Since all **Image Geometries** are implicitly 3D (even when plane-like), the fundamental building-block of an image is a *voxel*, which is a 3D object; therefore, the basic **Element** type for an **Image Geometry** is **Cell**.  **Attribute Arrays** associated with **Image Cells** are assumed to raster *x-y-z*, fastest to slowest.

### Example Usage

If you are reading in raw binary data that represents data on a regular grid, the user will need to run this
filter first to create a description of the **Geometry**.

% Auto generated parameter table will be inserted here

## Example Pipelines

## License & Copyright

Please see the description file distributed with this **Plugin**

## DREAM3D-NX Help

If you need help, need to file a bug report or want to request a new feature, please head over to the [DREAM3DNX-Issues](https://github.com/BlueQuartzSoftware/DREAM3DNX-Issues/discussions) GItHub site where the community of DREAM3D-NX users can help answer your questions.
