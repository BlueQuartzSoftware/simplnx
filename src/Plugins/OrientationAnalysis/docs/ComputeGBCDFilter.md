# Compute GBCD

## Group (Subgroup)

Statistics (Crystallography)

## Description

This **Filter** computes the Grain Boundary Character Distribution (GBCD) for a **Triangle Geometry**. The GBCD is a statistical description of the types of grain boundaries present in a polycrystalline material. It answers the question: *"What fraction of the total grain boundary area has a given misorientation and boundary plane orientation?"*

### What is the GBCD?

In a polycrystalline material, grains meet at boundaries. Each boundary can be described by five parameters:

- **3 parameters** for the *misorientation* -- the rotational difference between the crystal orientations of the two grains on either side of the boundary
- **2 parameters** for the *boundary plane normal* -- the direction the boundary surface faces, expressed in the crystal reference frame

The GBCD is a histogram over this 5-dimensional space. It records how much grain boundary area exists for each combination of misorientation and boundary plane orientation, normalized so that a random distribution would give a value of 1.0. Results are reported in *multiples of a random distribution* (MRD), so a value of 2.0 means there is twice as much boundary of that type as would be expected in a random polycrystal.

### How This Filter Works

1. For each triangular face on the grain boundary mesh, the filter identifies the two grains on either side.
2. It computes the misorientation between the two grains and determines the boundary plane normal in the crystal reference frame.
3. Crystal symmetry is applied to map each boundary into a unique representation.
4. The boundary is placed into the appropriate bin of the 5D histogram, weighted by its area.
5. The result is normalized by the total boundary area.

The **GBCD Resolution** parameter controls the bin size of the histogram. Smaller values give finer resolution but require more boundary data for statistical reliability.

### Note

Only boundaries between grains of the same phase are included in the calculation. The GBCD can be visualized by using the [Write GBCD Pole Figure (GMT)](WriteGBCDGMTFileFilter.md) or [Compute GBCD Pole Figure](ComputeGBCDPoleFigureFilter.md) filters.

For an alternative approach to computing the GBCD using continuous metrics instead of binning, see the [Compute GBCD (Metric-Based Approach)](ComputeGBCDMetricBasedFilter.md) filter.

### Required Input Sources

This filter operates on a grain-boundary surface mesh and requires the following upstream steps:

- **Triangle Geometry** -- produced by a surface meshing filter such as [Quick Surface Mesh](../SimplnxCore/QuickSurfaceMeshFilter.md), optionally followed by [Laplacian Smoothing](../SimplnxCore/LaplacianSmoothingFilter.md).
- **Face Labels** -- produced alongside the mesh by [Quick Surface Mesh](../SimplnxCore/QuickSurfaceMeshFilter.md).
- **Face Normals** -- produced by [Compute Triangle Normals](../SimplnxCore/TriangleNormalFilter.md).
- **Face Areas** -- produced by [Compute Triangle Areas](../SimplnxCore/ComputeTriangleAreasFilter.md).
- **Feature Euler Angles / Feature Phases** -- the feature-level average Euler angles and phases. Averages are produced by [Compute Average Orientations](ComputeAvgOrientationsFilter.md); phases by [Compute Feature Phases](../SimplnxCore/ComputeFeaturePhasesFilter.md).
- **Crystal Structures** -- ensemble-level array read from EBSD data or created by [Create Ensemble Info](CreateEnsembleInfoFilter.md).

## References

[1] G. S. Rohrer, "Grain boundary energy anisotropy: a review," *Journal of Materials Science*, vol. 46, pp. 5881-5895, 2011. DOI: [10.1007/s10853-011-5677-3](https://doi.org/10.1007/s10853-011-5677-3)

[2] G. S. Rohrer, "Measuring and Interpreting the Structure of Grain-Boundary Networks," *Journal of the American Ceramic Society*, vol. 94, no. 3, pp. 633-646, 2011. DOI: [10.1111/j.1551-2916.2011.04384.x](https://doi.org/10.1111/j.1551-2916.2011.04384.x)

% Auto generated parameter table will be inserted here

## Example Pipelines

+ `(08) Small IN100 GBCD`

## License & Copyright

Please see the description file distributed with this **Plugin**

## DREAM3D-NX Help

If you need help, need to file a bug report or want to request a new feature, please head over to the [DREAM3DNX-Issues](https://github.com/BlueQuartzSoftware/DREAM3DNX-Issues/discussions) GitHub site where the community of DREAM3D-NX users can help answer your questions.
