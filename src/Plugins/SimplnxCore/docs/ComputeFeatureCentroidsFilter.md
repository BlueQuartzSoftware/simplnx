# Compute Feature Centroids

## Group (Subgroup)

Generic (Morphological)

## Description

This **Filter** calculates the *centroid* of each **Feature** by determining the average X, Y, and Z position (in physical coordinates) of all the **Cells** belonging to the **Feature**. The per-cell coordinates are accumulated using Kahan compensated summation to limit floating-point round-off on features with large cell counts. An *Is Periodic* option is available: when enabled, a **Feature** that spans the full extent of an axis (touching both opposing faces of the **Image Geometry**) is treated as wrapping around to the opposite face, and its centroid on that axis is computed as a minimum-image (circular) mean — the cell positions are unwrapped across the boundary before averaging and the result is mapped back into the domain. This depends on where the **Feature**'s cells actually lie, so it is correct for features whose mass wraps asymmetrically and never places the centroid outside the sample; a feature that fills the whole domain along an axis falls back to the ordinary average on that axis. When *Is Periodic* is disabled, **Features** that intersect the outer surfaces of the sample will still have *centroids* calculated, but they will be *centroids* of the truncated part of the **Feature** that lies inside the sample.

A **Feature** with no **Cells** (an unused Feature Id) keeps a centroid of (0, 0, 0).

% Auto generated parameter table will be inserted here

## Example Pipelines

+ (03) Small IN100 Morphological Statistics
+ InsertTransformationPhase
+ (06) SmallIN100 Synthetic

## License & Copyright

Please see the description file distributed with this **Plugin**

## DREAM3D-NX Help

If you need help, need to file a bug report or want to request a new feature, please head over to the [DREAM3DNX-Issues](https://github.com/BlueQuartzSoftware/DREAM3DNX-Issues/discussions) GitHub site where the community of DREAM3D-NX users can help answer your questions.
