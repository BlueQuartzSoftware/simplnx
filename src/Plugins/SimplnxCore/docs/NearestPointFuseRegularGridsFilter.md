# Fuse Regular Grids (Nearest Point)

## Group (Subgroup)

Sampling (Resolution)

## Description

This filter copies the **Cell** data from one **Image Geometry** onto another by matching each cell to the spatially nearest cell in the other grid. It is used to bring the attribute arrays of two separately-sampled volumes into a single grid so they can be analyzed together.

### Reference vs. Sampling Geometry

The filter works with two **Image Geometries**:

- The **Reference** geometry is the grid that is kept. Its cells are the destination.
- The **Sampling** geometry is the grid that is read from. Its cells are the source of the copied data.

The two grids can have different origins, spacings, and dimensions. The filter finds the center of each *Reference* cell and copies the values of the *Sampling* cell that contains that point. When the center lies on a shared face, the cell with the higher index supplies the values. The sampling grid includes its minimum face and excludes its maximum face on each axis.

The *Sampling* geometry is left unchanged. The *Reference* geometry keeps its own geometry but gains a copy of the sampling geometry's attribute arrays.

### Parameter Guidance

- **Use Custom Fill Value** applies to reference cells whose centers lie outside the sampling grid. These cells receive *0* by default. Enable this option to choose a different fill value.

### Required Input Sources

- **Reference Image Geometry** and **Sampling Image Geometry** -- two **Image Geometries** (each with a **Cell Attribute Matrix**), typically created by separate import or resampling steps.

**Sampling Cell Attribute Matrix** must have one tuple per cell of **Sampling Image Geometry**. **Reference Cell Attribute Matrix** must have one tuple per cell of **Reference Image Geometry**.

## Algorithm

The filter copies numeric and Boolean **Data Arrays** without interpolation. For in-memory arrays, it traverses the reference grid cell by cell. When a source or destination array is out-of-core, the filter calculates a source index for each X, Y, and Z coordinate. It reads each required sampling row into a buffer and writes each completed reference row in one operation. The buffers hold one source row, one reference row, and one index per coordinate on each reference axis.

% Auto generated parameter table will be inserted here

## License & Copyright

Please see the description file distributed with this **Plugin**

## DREAM3D-NX Help

If you need help, need to file a bug report or want to request a new feature, please head over to the [DREAM3DNX-Issues](https://github.com/BlueQuartzSoftware/DREAM3DNX-Issues/discussions) GitHub site where the community of DREAM3D-NX users can help answer your questions.
