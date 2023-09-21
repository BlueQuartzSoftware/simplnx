# Sample Triangle Geometry on Uncertain Regular Grid

## Group (Subgroup)

Sampling (Resolution)

## Description

This **Filter** "samples" a triangulated surface mesh on a rectilinear grid, but with "uncertainty" in the absolute position of the **Cells**.  The "uncertainty" is meant to simulate the possible positioning error in a sampling probe.  The user can specify the number of **Cells** along the X, Y, and Z directions in addition to the resolution in each direction and origin to define a rectilinear grid.  The sampling, with "uncertainty", is then performed by the following steps:

1. Determine the bounding box and **Triangle** list of each **Feature** by scanning all **Triangles** and noting the **Features** on either side of the **Triangle**
2. For each **Cell** in the rectilinear grid, perturb the location of the **Cell** by generating a three random numbers between [-1, 1] and multiplying them by the three uncertainty values (one for each direction)
3. For each perturbed **Cell** in the rectilinear grid, determine which bounding box(es) they fall in (*Note:* the bounding box of multiple **Features** can overlap)
4. For each bounding box a **Cell** falls in, check against that **Feature's** **Triangle** list to determine if the **Cell** falls within that n-sided polyhedra. (*Note:* if the surface mesh is conformal, then each **Cell** will only belong to one **Feature**, but if not, the last **Feature** the **Cell** is found to fall inside of will *own* the **Cell**)
5. Assign the **Feature** number that the **Cell** falls within to the *Feature Ids* array in the new rectilinear grid geometry

**Note that the unperturbed grid is where the *Feature Ids* actually live, but the perturbed locations are where the Cells are sampled from.  Essentially, the *Feature Ids* are stored where the user *thinks* the sampling took place, not where it actually took place!**

## Parameters

| Name | Type | Description |
|------|------|-------------|
| Use Seed | bool | If *true* user may set a set a seed for random generation |
| Seed | uint64 | This is the seed fed into the random generator |
| Dimensions | uint64 | Number of **Cells** along each axis |
| Resolution | float32 (3x) | The resolution values (dx, dy, dz) |
| Origin | float32 (3x) | The origin of the sampling volume |
| Uncertainty | float32 (3x) | Vector of uncertainty values associated with X, Y and Z positions of **Cells |

## Required Geometry

Triangle

## Required Objects

| Type | Default Name | Type | Comp Dims | Description |
|------|--------------|-------------|---------|-----|
| Data Array | Face Labels | int32 | (2) | Specifies which **Features** are on either side of each **Face**. |

## Created Objects

| Kind                      | Default Name | Type     | Comp. Dims | Description                                 |
|---------------------------|--------------|----------|------------|---------------------------------------------|
| Image Geometry | Image Geometry | N/A | N/A | Created **Image Geometry** name and *DataPath* |
|   Attribute Matrix   | Cell Data | Cell | N/A | Created **Cell Attribute Matrix** name |
| Data Array | Feature Ids | int32 | (1) | Specifies to which **Feature** each **Cell** belongs |

## License & Copyright

Please see the description file distributed with this **Plugin**

## DREAM3DNX Help

Check out our GitHub community page at [DREAM3DNX-Issues](https://github.com/BlueQuartzSoftware/DREAM3DNX-Issues) to report bugs, ask the community for help, discuss features, or get help from the developers.


