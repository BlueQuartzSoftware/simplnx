# Compute Feature Neighbor Misorientations

## Group (Subgroup)

Statistics (Crystallography)

## Description

This **Filter** computes the misorientation angle between each **Feature** (grain) and its neighboring **Features**. Misorientation is the angular difference between the crystal orientations of two grains -- it describes how much one grain's crystal lattice is rotated relative to another's.

![Fig. 1: The misorientation between two grains is the rotation relating their crystal orientations; its angle is the misorientation angle. Because a crystal has symmetry-equivalent orientations, the smallest equivalent angle — the disorientation — is what the filter reports.](Images/Misorientation_Concept.png)

For each **Feature**, the filter produces a list of misorientation angles (in degrees), one for each neighboring **Feature** it shares a boundary with. The axis of the misorientation is not stored, only the angle.

Optionally, the filter can also compute the average misorientation between a **Feature** and all of its neighbors. This average value provides a quick summary of how differently oriented a grain is compared to its surroundings.

### Note

Only **Features** with identical crystal structures are compared. If two neighboring **Features** have different crystal structures, a value of NaN is stored for their misorientation, since misorientation between different crystal systems is not physically meaningful.

### Required Input Sources

- **Neighbor List** -- produced by [Compute Feature Neighbors](../SimplnxCore/ComputeFeatureNeighborsFilter.md) or [Compute Feature Neighborhoods](../SimplnxCore/ComputeNeighborhoodsFilter.md).
- **Average Quaternions** -- produced by [Compute Average Orientations](ComputeAvgOrientationsFilter.md).
- **Feature Phases** -- produced by [Compute Feature Phases](../SimplnxCore/ComputeFeaturePhasesFilter.md).
- **Crystal Structures** -- ensemble-level array read from EBSD data or created by [Create Ensemble Info](CreateEnsembleInfoFilter.md).

% Auto generated parameter table will be inserted here

## Example Pipelines

+ `(05) SmallIN100 Crystallographic Statistics`

## License & Copyright

Please see the description file distributed with this **Plugin**

## DREAM3D-NX Help

If you need help, need to file a bug report or want to request a new feature, please head over to the [DREAM3DNX-Issues](https://github.com/BlueQuartzSoftware/DREAM3DNX-Issues/discussions) GitHub site where the community of DREAM3D-NX users can help answer your questions.
