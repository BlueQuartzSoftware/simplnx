# Compute Twin Boundaries

## Group (Subgroup)

Statistics (Crystallography)

## Description

This **Filter** identifies twin boundaries on a **Triangle Geometry** surface mesh. A twin boundary is a special type of grain boundary where two grains share a specific crystallographic relationship -- their crystal lattices are mirror images of each other across the boundary plane.

### What is a Twin Boundary?

Twins are grains that share a highly symmetric orientation relationship. The most common type is the &Sigma;3 twin, where the two grains are related by a 60-degree rotation about the <111> crystal direction. Twin boundaries are significant because they tend to have very low energy and distinct mechanical properties compared to general grain boundaries. They are particularly common in FCC metals such as copper, nickel, and austenitic stainless steel.

### How This Filter Works

1. For each **Triangle** on the grain boundary mesh, the filter computes the misorientation between the two **Features** on either side using their average orientations.
2. If the misorientation axis and angle match the &Sigma;3 twin relationship within the user-defined tolerances, the **Triangle** is flagged as a twin boundary.
3. If **Compute Coherence** is enabled, the filter additionally measures the *incoherence* -- the angle (in degrees) between the boundary plane normal and the misorientation axis, both expressed in the crystal reference frame. A perfectly coherent twin has an incoherence of 0 degrees, meaning the boundary plane is exactly the twin plane. Higher incoherence values indicate a boundary that has the correct misorientation but is not on the ideal twin plane.

### Note

Only boundaries between **Features** of the same phase are evaluated.

### Output Type for Twin Boundaries Array

- **boolean [0]**: Stores the result as true/false
- **uint8 [1]**: Stores the result as 1/0

### Required Input Sources

This filter operates on a grain-boundary surface mesh and requires the following upstream steps:

- **Face Labels** -- produced by a surface meshing filter such as [Quick Surface Mesh](../SimplnxCore/QuickSurfaceMeshFilter.md).
- **Face Normals** (only when *Find Coherence* is enabled) -- produced by [Compute Triangle Normals](../SimplnxCore/TriangleNormalFilter.md).
- **Average Quaternions** -- produced by [Compute Average Orientations](ComputeAvgOrientationsFilter.md).
- **Feature Phases** -- produced by [Compute Feature Phases](../SimplnxCore/ComputeFeaturePhasesFilter.md).
- **Crystal Structures** -- ensemble-level array read from EBSD data or created by [Create Ensemble Info](CreateEnsembleInfoFilter.md).

% Auto generated parameter table will be inserted here

## Example Pipelines

## License & Copyright

Please see the description file distributed with this **Plugin**

## DREAM3D-NX Help

If you need help, need to file a bug report or want to request a new feature, please head over to the [DREAM3DNX-Issues](https://github.com/BlueQuartzSoftware/DREAM3DNX-Issues/discussions) GitHub site where the community of DREAM3D-NX users can help answer your questions.
