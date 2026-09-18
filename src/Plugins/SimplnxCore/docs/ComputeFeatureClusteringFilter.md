# Compute Feature Clustering

## Group (Subgroup)

Statistics (Morphological)

## Description

This **Filter** computes the **radial distribution function (RDF)** of a set of **Features**. A **Feature** is a contiguous region of like-segmented cells (for example a grain or particle). An **RDF** is a histogram that describes how the **Features** are spatially distributed relative to each other: for each distance bin, it counts how many other **Feature** centroids lie at that separation distance, then normalizes that count against the count expected if the same number of **Features** were scattered randomly in the same volume. Values above 1.0 at a given distance indicate the **Features** are more likely to be found at that separation than random (clustering or ordering); values below 1.0 indicate they are less likely (exclusion). Use this **Filter** to detect and quantify clustering, ordering, or repulsion in a population of **Features**.

An **Ensemble** is a group of **Features** that share a common phase (a distinct material or crystal structure). This **Filter** operates on a single **Ensemble** at a time, specified by the *Phase Index* parameter, and the resulting **RDF** is stored as **Ensemble** data.

All distances in this **Filter** are physical lengths expressed in the same length units as the **Image Geometry** spacing (for example microns). The algorithm proceeds as follows:

1. Find the straight-line (Euclidean) distance from the current **Feature** centroid to every other **Feature** centroid of the selected phase.
2. Put all calculated distances in a clustering list.
3. Repeat steps 1-2 for all **Features**.
4. Sort the distances into the specified number of bins, all equally sized in distance from the minimum separation to the maximum separation. For example, if the user chooses 10 bins, the minimum separation is 10 microns, and the maximum separation is 80 microns, each bin spans 7 microns.
5. Normalize the **RDF** by the probability of finding the **Features** at that distance if they were distributed randomly in the bounding box.

The **Filter** also outputs the clustering list (every inter-**Feature** distance) and the minimum and maximum separation distances (both in physical length units).

*Note:* Because the algorithm iterates over all **Features**, each distance is double-counted. For example, the distance from **Feature** 1 to **Feature** 2 is counted along with the identical distance from **Feature** 2 to **Feature** 1.

### Required Input Sources

- **Centroids** -- the X, Y, Z coordinates of each Feature's center of mass, produced by [Compute Feature Centroids](ComputeFeatureCentroidsFilter.md).
- **Phases** -- the phase (Ensemble) that each Feature belongs to, produced by [Compute Feature Phases](ComputeFeaturePhasesFilter.md).

### Performance

This filter has O(n^2) complexity in the number of features of the target phase. The feature-level arrays (phases, centroids) are accessed in the inner pairwise loop. For out-of-core (OOC) data, per-element virtual dispatch inside this quadratic loop would be prohibitively expensive. The algorithm bulk-reads the entire FeaturePhases and Centroids arrays into local `std::vector` caches via `copyIntoBuffer()` at the start. The RDF histogram is also accumulated into a local vector and written back to the output DataStore in a single `copyFromBuffer()` call after normalization.

% Auto generated parameter table will be inserted here

## Example Pipelines

+ PorosityAnalysis

## License & Copyright

Please see the description file distributed with this **Plugin**

## DREAM3D-NX Help

If you need help, need to file a bug report or want to request a new feature, please head over to the [DREAM3DNX-Issues](https://github.com/BlueQuartzSoftware/DREAM3DNX-Issues/discussions) GitHub site where the community of DREAM3D-NX users can help answer your questions.
