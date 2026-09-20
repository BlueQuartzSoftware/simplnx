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

## Algorithm

For each triangle face in the surface mesh, the algorithm checks whether the two adjacent features are in the same phase with a cubic crystal structure. If so, the misorientation between the two features' average quaternions is computed using all symmetry operator pairs. A face is flagged as a twin boundary if any symmetric equivalent produces a misorientation within the user-defined angle and axis tolerances of the Sigma 3 twin relationship (60 degrees about <111>).

When coherence computation is enabled, the crystal direction parallel to the face normal is determined and compared with the misorientation axis. The minimum angular deviation across all valid symmetry pairs is stored as the incoherence value.

### In-Core Path

Feature-level arrays (phases, average quaternions) and face-level arrays (labels, normals) are accessed through the AbstractDataStore API. The twin boundary check is parallelized using `ParallelDataAlgorithm`.

### Out-of-Core Path

All input arrays are bulk-read into local `std::vector` caches at startup: ensemble-level crystal structures, feature-level phases and average quaternions, and face-level labels and normals. The parallel workers (`CalculateTwinBoundaryImpl` and `CalculateTwinBoundaryWithIncoherenceImpl`) operate entirely on these local vectors with zero OOC virtual dispatch in the hot loop.

Output is accumulated into local vectors and bulk-written to the DataStores via `copyFromBuffer` after the parallel computation completes.

### Performance

Pre-caching all arrays into contiguous local vectors enables safe parallel execution without any thread contending for OOC page locks. Face-level data scales with surface area (not volume), so it typically fits in memory even for large datasets. The parallel twin boundary check across all symmetry operator pairs is the dominant compute cost and benefits from the contention-free data access.

% Auto generated parameter table will be inserted here

## Example Pipelines

## License & Copyright

Please see the description file distributed with this **Plugin**

## DREAM3D-NX Help

If you need help, need to file a bug report or want to request a new feature, please head over to the [DREAM3DNX-Issues](https://github.com/BlueQuartzSoftware/DREAM3DNX-Issues/discussions) GitHub site where the community of DREAM3D-NX users can help answer your questions.
