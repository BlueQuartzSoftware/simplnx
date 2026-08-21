# Fuse Regular Grids (Nearest Point)

## Group (Subgroup)

Sampling (Resolution)

## Description

This filter copies the **Cell** data from one **Image Geometry** onto another by matching each cell to the spatially nearest cell in the other grid. It is used to bring the attribute arrays of two separately-sampled volumes into a single grid so they can be analyzed together.

### Reference vs. Sampling Geometry

The filter works with two **Image Geometries**:

- The **Reference** geometry is the grid that is kept. Its cells are the destination.
- The **Sampling** geometry is the grid that is read from. Its cells are the source of the copied data.

For each cell in the *Reference* geometry, the filter finds the cell in the *Sampling* geometry whose center is physically closest (using the two grids' origins and spacings), and copies all of that sampling cell's attribute arrays onto the reference cell. **No interpolation is performed** — the nearest sampling value is taken as-is. Because the match is by physical position, the two grids may have different spacings, dimensions, or extents; only their overlap in space is meaningful.

The *Sampling* geometry is left unchanged. The *Reference* geometry keeps its own geometry but gains a copy of the sampling geometry's attribute arrays.

### Parameter Guidance

- **Use Custom Fill Value** — controls what happens to reference cells that fall outside the sampling grid (no nearby sampling cell). When off, copied arrays are filled with *0* there; when on, the user supplies the fill value.

### Required Input Sources

- **Reference Image Geometry** and **Sampling Image Geometry** -- two **Image Geometries** (each with a **Cell Attribute Matrix**), typically created by separate import or resampling steps.

% Auto generated parameter table will be inserted here

## License & Copyright

Please see the description file distributed with this **Plugin**

## DREAM3D-NX Help

If you need help, need to file a bug report or want to request a new feature, please head over to the [DREAM3DNX-Issues](https://github.com/BlueQuartzSoftware/DREAM3DNX-Issues/discussions) GitHub site where the community of DREAM3D-NX users can help answer your questions.
