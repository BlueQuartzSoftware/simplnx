# Compute Feature Volumes from Triangle Geometry

## Group (Subgroup)

Statistics (Morphological)

## Description

This **Filter** computes the enclosed volume of each **Feature** in a **Triangle Geometry**. The result is the volume of
each surface meshed **Feature**, or alternatively the volume of each unique polyhedron defined by the given _Face
Labels_ array. The volume of any generic polyhedron can be computed using the following algorithm:

1. Triangulate each face of the polyhedron (in this case, each face is already a triangle within the **Triangle
   Geometry**)
2. For each triangular face, ensure the normals are all consistent (this **Filter** uses the convention where normals
   point inwards; note that the actual winding of the **Triangle Geometry** is not modified)
3. For each triangular face, create a tetrahedron where the fourth vertex is the origin
4. Compute the signed volume of each tetrahedron
5. Sum the signed tetrahedra volumes to obtain the volume of the enclosing polyhedron

This computation is _not_ the same as the Find Feature Sizes for **Triangle Geometries**, which computes the sum of the
unit element sizes for a set of **Features** (thus, the Find Feature Sizes would compute the _area_
of **Features** in a **Triangle Geometry**, whereas this **Filter** is specialized to compute the enclosed volumes of **
Features** in a surface mesh).

% Auto generated parameter table will be inserted here

## Example Pipelines

## License & Copyright

Please see the description file distributed with this plugin.

## DREAM3D-NX Help

If you need help, need to file a bug report or want to request a new feature, please head over to the [DREAM3DNX-Issues](https://github.com/BlueQuartzSoftware/DREAM3DNX-Issues/discussions) GitHub site where the community of DREAM3D-NX users can help answer your questions.
