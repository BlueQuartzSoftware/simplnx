# Compute Feature Centroids from Triangle Geometry

## Group (Subgroup)

Statistics (Morphological)

## Description

This **Filter** determines the centroids of each **Feature** in a **Triangle Geometry**. The centroids are determined
using the following algorithm:

1. Query each triangle within the **Triangle Geometry** to determine its two owners
2. Add the 3 nodes of that triangle to the set of nodes bounding those two owners (*Note that a set will only allow each
   node to be entered once for a given owner*)
3. For each **Feature**, find the average (x,y,z) coordinate from the set of nodes that bound it

## Is Periodic

When **Is Periodic** is enabled, features whose nodes wrap across opposing boundaries of the mesh are handled with a
minimum-image (periodic) centroid rather than the plain average. For any axis on which a feature touches both opposing
faces of the geometry's bounding box, the centroid component on that axis is computed by unwrapping the feature's nodes
across the boundary (using the largest empty gap in their distribution) before averaging, and mapping the result back
into the domain. This depends on where the feature's mass actually lies, so it is correct for asymmetrically-wrapped
features and never places the centroid outside the domain. Axes on which the feature does not wrap keep the ordinary
average, and a feature that fills the whole domain along an axis falls back to the ordinary average on that axis. When
**Is Periodic** is disabled (the default), the plain average from step 3 is reported.

> **Note:** Enabling **Is Periodic** assumes the periodic domain equals the geometry's bounding box. An informational
> message is emitted for each feature that is adjusted, since a manual review may be warranted.

% Auto generated parameter table will be inserted here

## Example Pipelines

## License & Copyright

Please see the description file distributed with this plugin.

## DREAM3D-NX Help

If you need help, need to file a bug report or want to request a new feature, please head over to the [DREAM3DNX-Issues](https://github.com/BlueQuartzSoftware/DREAM3DNX-Issues/discussions) GitHub site where the community of DREAM3D-NX users can help answer your questions.
