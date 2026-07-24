# Write GBCD Triangles File

## Group (Subgroup)

IO (Output)

## Description

This filter writes a plain-text file describing the **grain boundary character distribution (GBCD)** for each triangle in a surface mesh. A **grain boundary** is the internal interface where two grains (individual crystals) meet inside a material; a surface mesh represents those interfaces as a collection of triangles in a **Triangle Geometry**.

For every boundary triangle the filter writes one line per triangle containing:

- The **average crystal orientation** of the grain on each side of the boundary, expressed as a set of Bunge **Euler angles** (phi1, PHI, phi2) in *radians*.
- The triangle **normal**, a unit direction (three unitless direction cosines) pointing perpendicular to the triangle face.
- The triangle **surface area**, in the squared units of the mesh vertex coordinates (square microns for typical EBSD data).

### Inward / Outward (Left / Right) Average Orientations

Each boundary triangle separates exactly two grains. The "right hand" (outward) and "left hand" (inward) average orientations are simply the average crystal orientations of those two grains, one on each side of the triangle. Listing both sides lets downstream tools reconstruct the **misorientation** (the rotation relating the two grains) across the boundary.

### File Format

The file begins with comment lines describing the ten data columns, followed by one row per triangle. Each row has ten space-separated values:

    # Column 1-3:    right hand (outward) average orientation (phi1, PHI, phi2 in RADIANS)
    # Column 4-6:    left hand (inward) average orientation (phi1, PHI, phi2 in RADIANS)
    # Column 7-9:    triangle normal (unitless direction cosines)
    # Column 10:     triangle surface area
    0.2662 0.6970 4.4347 0.7993 0.6738 3.5200 0.0000 0.8829 -0.4696 0.0240
    0.2662 0.6970 4.4347 0.7993 0.6738 3.5200 0.4532 0.3203 -0.8319 0.0211
    0.2662 0.6970 4.4347 0.7993 0.6738 3.5200 1.0000 0.0000 0.0000 0.0312
    0.7993 0.6738 3.5200 0.2662 0.6970 4.4347 0.0000 -0.7792 0.6268 0.0182
       ..

The file format was originally defined by Prof. Gregory S. Rohrer (Carnegie Mellon University) for use with the Rohrer group's stereological grain-boundary analysis tools. See G. S. Rohrer, "Grain boundary energy anisotropy: a review," *Journal of Materials Science* 46 (2011) 5881-5895, and the associated GBCD/stereology software distributed by his group.

### Required Input Sources

- **Face Labels** -- produced by the surface-meshing step, [Create Surface Mesh (QuickMesh)](../SimplnxCore/QuickSurfaceMeshFilter.md). Identifies the two **Features** (grains) on either side of each triangle.
- **Face Normals** -- produced by [Compute Triangle Normals](../SimplnxCore/TriangleNormalFilter.md).
- **Face Areas** -- produced by [Compute Triangle Areas](../SimplnxCore/ComputeTriangleAreasFilter.md).
- **Average Euler Angles** -- per-**Feature** average orientations produced by [Compute Feature Average Orientations](ComputeAvgOrientationsFilter.md).

Phase and orientation data for the grains ultimately come from an EBSD reader such as [Read H5EBSD File](ReadH5EbsdFilter.md), [Read EDAX EBSD Data (.ang)](ReadAngDataFilter.md), or [Read Oxford Instr. EBSD Data (.ctf)](ReadCtfDataFilter.md).

% Auto generated parameter table will be inserted here

## Example Pipelines

## License & Copyright

Please see the description file distributed with this **Plugin**

## DREAM3D-NX Help

If you need help, need to file a bug report or want to request a new feature, please head over to the [DREAM3DNX-Issues](https://github.com/BlueQuartzSoftware/DREAM3DNX-Issues/discussions) GitHub site where the community of DREAM3D-NX users can help answer your questions.
