# Reverse Triangle Winding

## Group (Subgroup)

Surface Meshing (Connectivity/Arrangement)

## Description

This **Filter** reverses the **winding** of every **Triangle** in a **Triangle Geometry**. The *winding* is the order in which a triangle's three vertices are listed; that order determines which way the triangle's normal points. Reversing the winding therefore *flips* the direction of the triangle normals so they point the opposite way.

Some analysis routines require the normals to point "away" from the center of a **Feature**. This **Filter** lets you flip the winding so the normals point in the required direction. After reversing the winding, recompute normals with [Compute Triangle Normals](TriangleNormalFilter.md) so they reflect the new vertex order. To check whether a mesh has consistent winding before or after this operation, use [Verify Triangle Winding](VerifyTriangleWindingFilter.md).

### Required Input Sources

- **Triangle Geometry** -- the surface mesh whose triangle winding will be reversed.

% Auto generated parameter table will be inserted here

## License & Copyright

Please see the description file distributed with this **Plugin**

## DREAM3D-NX Help

If you need help, need to file a bug report or want to request a new feature, please head over to the [DREAM3DNX-Issues](https://github.com/BlueQuartzSoftware/DREAM3DNX-Issues/discussions) GitHub site where the community of DREAM3D-NX users can help answer your questions.
