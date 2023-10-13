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

% Auto generated parameter table will be inserted here

## License & Copyright

Please see the description file distributed with this **Plugin**

## DREAM3D-NX Help

If you need help, need to file a bug report or want to request a new feature, please head over to the [DREAM3DNX-Issues](https://github.com/BlueQuartzSoftware/DREAM3DNX-Issues) GItHub site where the community of DREAM3D-NX users can help answer your questions.
